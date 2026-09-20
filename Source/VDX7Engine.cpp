#include "VDX7Engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>

VDX7Engine::VDX7Engine()
    : dx7_(toSynth_, toGui_)
{
    toSynth_ = &appToSynth_;
    toGui_ = &nullToGui_;

    // Same default curve used by VDX7/Retromulator: exponent 0.4.
    for (int i = 0; i < 128; ++i)
    {
        const auto x = static_cast<float>(i) / 127.0f;
        velocityMap_[static_cast<std::size_t>(i)] =
            static_cast<uint8_t>(127.0f * std::pow(x, 0.4f) + 0.5f);
    }
}

bool VDX7Engine::loadRomImage(const uint8_t* data, std::size_t size,
                              const uint8_t* optionalVoices,
                              std::size_t optionalVoicesSize)
{
    // Reject invalid input without changing the running instrument.
    if (data == nullptr || (size != kFirmwareSize && size != kCombinedRomSize)
        || (optionalVoices != nullptr && optionalVoicesSize != kFactoryVoicesSize))
        return false;

    const uint8_t* firmware = nullptr;
    const uint8_t* voices = nullptr;
    std::size_t voicesSize = 0;

    if (size == kFirmwareSize)
    {
        firmware = data;
        if (optionalVoices != nullptr && optionalVoicesSize >= kFactoryVoicesSize)
        {
            voices = optionalVoices;
            voicesSize = optionalVoicesSize;
        }
    }
    else if (size == kCombinedRomSize)
    {
        firmware = data;
        voices = data + kFirmwareSize;
        voicesSize = kFactoryVoicesSize;
    }
    else
    {
        return false;
    }

    std::vector<uint8_t> newFactoryVoices;
    if (voices != nullptr && voicesSize >= kFactoryVoicesSize)
        newFactoryVoices.assign(voices, voices + kFactoryVoicesSize);

    if (!dx7_.loadFirmware(firmware, kFirmwareSize))
        return false;

    // loadVoices(nullptr, 0) does NOT clear the core's previous pointer.
    dx7_.loadVoices(emptyFactoryBank_.data(), emptyFactoryBank_.size());
    factoryVoices_ = std::move(newFactoryVoices);
    if (!factoryVoices_.empty())
        dx7_.loadVoices(factoryVoices_.data(), factoryVoices_.size());
    activeMidiNotes_.fill(false);
    sustainDown_ = false;
    midiExpression_ = 1.0f;
    currentBank_ = -1;
    currentProgram_ = 0;
    dx7_.midiSerialRx.flush();
    dx7_.midiSerialTx.flush();
    dx7_.haveMsg = false;
    dx7_.byte1Sent = false;
    dx7_.pitchBendOffset = 0;
    dx7_.midiVolume = 7;
    dx7Emu::Message discarded;
    while (toSynth_->pop(discarded)) {}

    boot();
    loaded_ = dx7_.isRomLoaded();

    if (loaded_ && hasFactoryVoices())
    {
        selectFactoryBank(0);
        selectProgram(0);
    }

    resetAudioState();
    return loaded_;
}

void VDX7Engine::boot()
{
    dx7_.start();
    if (!dx7_.isRomLoaded())
        return;

    // Retromulator's VDX7 adapter performs the same firmware warm-up.
    for (int i = 0; i < 3000000; ++i)
        dx7_.run();

    dx7_.initControllers();
    dx7_.midiFilter.set_f(static_cast<float>(10.6 / kNativeSampleRate));
}

void VDX7Engine::prepare(double hostSampleRate)
{
    hostSampleRate_ = std::isfinite(hostSampleRate) && hostSampleRate > 1000.0 ? hostSampleRate : 48000.0;
    resampler_.prepare(hostSampleRate_);
    resetAudioState();
}

void VDX7Engine::resetAudioState()
{
    nativePos_ = 0;
    nativeCount_ = 0;
    resampler_.reset();
    dx7_.midiFilter.reset();
}

void VDX7Engine::resetMidiLifecycle()
{
    if (!loaded_) { resetAudioState(); return; }
    dx7_.midiSerialRx.flush();
    dx7_.midiSerialTx.flush();
    dx7_.haveMsg = false;
    dx7_.byte1Sent = false;
    dx7Emu::Message discarded;
    while (toSynth_->pop(discarded)) {}
    allNotesOff();
    toSynth_->porta(false);
    // Let the unmodified firmware consume releases (up to 128*3 serial bytes)
    // before resetting the sound generator. This is lifecycle work, NOT audio.
    std::array<float, 256> scratchLeft {}, scratchRight {};
    for (int remaining = static_cast<int>(hostSampleRate_ * 0.25); remaining > 0; remaining -= 256)
        render(scratchLeft.data(), scratchRight.data(), std::min(256, remaining));
    dx7_.midiSerialRx.flush();
    dx7_.midiSerialTx.flush();
    dx7_.haveMsg = false;
    dx7_.byte1Sent = false;
    // EGS owns envelopes, phases and analog filter history. Reconstruct it in
    // its existing storage to stop old tails; firmware/RAM/factory data stay put.
    std::destroy_at(&dx7_.egs);
    std::construct_at(&dx7_.egs, dx7_.memory + 0x3000);
    resetAudioState();
    selectProgram(currentProgram_);
}

void VDX7Engine::render(float* left, float* right, int numSamples)
{
    if (numSamples <= 0)
        return;

    if (!loaded_)
    {
        if (left != nullptr) std::fill(left, left + numSamples, 0.0f);
        if (right != nullptr) std::fill(right, right + numSamples, 0.0f);
        return;
    }

    for (int i = 0; i < numSamples; ++i)
    {
        const float out = resampler_.sample([this] { return nextNativeSample(); });
        if (left != nullptr) left[i] = out;
        if (right != nullptr) right[i] = out;

    }
}

float VDX7Engine::nextNativeSample()
{
    if (nativePos_ >= nativeCount_)
    {
        nativeCount_ = generateNative(nativeBlock_.data());
        nativePos_ = 0;
        if (nativeCount_ <= 0)
            return 0.0f;
    }

    const float raw = nativeBlock_[static_cast<std::size_t>(nativePos_++)];
    const float midiVolume = std::min(1.0f,
        dx7_.midiVolTab[dx7_.midiVolume] * midiExpression_);
    return raw * volume_ * dx7_.midiFilter.operate(midiVolume);
}

int VDX7Engine::generateNative(float* out)
{
    if (!loaded_ || out == nullptr)
        return 0;

    int outCount = 0;
    dx7Emu::Message msg;

    // Advance only to the next actual EGS sample. Rendering a 512-sample
    // future here prevented intervening host MIDI events from reaching the
    // machine in time. CPU instructions remain atomic and every emitted
    // sample is retained (including instruction-boundary overshoot).
    while (outCount == 0)
    {
        if (!dx7_.haveMsg && toSynth_->pop(msg))
            processQueuedMessage(msg);

        dx7_.run();
        const int cycles = (dx7_.inst != nullptr && dx7_.inst->cycles > 0) ? dx7_.inst->cycles : 1;

        dx7_.egs.clock(out, outCount, 4 * cycles);
    }

    return outCount;
}

void VDX7Engine::processQueuedMessage(dx7Emu::Message msg)
{
    using CtrlID = dx7Emu::Message::CtrlID;

    switch (CtrlID(msg.byte1))
    {
        case CtrlID::volume:
            volume_ = static_cast<float>(std::pow(2.0, msg.byte2 / 127.0) - 1.0);
            break;

        case CtrlID::sustain:
            dx7_.sustain(msg.byte2 != 0);
            break;

        case CtrlID::porta:
            dx7_.porta(msg.byte2 != 0);
            break;

        case CtrlID::cartridge:
            dx7_.cartPresent(msg.byte2 != 0);
            break;

        case CtrlID::cartridge_num:
            dx7_.setBank(msg.byte2, true);
            break;

        case CtrlID::protect:
            dx7_.cartWriteProtect(msg.byte2 != 0);
            break;

        case CtrlID::pitchbend:
        {
            uint8_t pbRange = dx7_.memory[0x2076] & 0x0F;
            if (pbRange == 0) pbRange = 2;
            const int centered = static_cast<int>(msg.byte2) - 64;
            dx7_.pitchBendOffset = static_cast<int16_t>(centered * 1365 * pbRange / 63);
            dx7_.msg = msg;
            dx7_.haveMsg = true;
            break;
        }

        case CtrlID::modulate:
            dx7_.msg = msg;
            dx7_.haveMsg = true;
            break;

        default:
            // The original VDX7 sub-CPU protocol stores keyboard velocity inverted.
            if (msg.byte1 > 158 && msg.byte2 != 0)
                msg.byte2 = static_cast<uint8_t>(128 - msg.byte2);
            dx7_.msg = msg;
            dx7_.haveMsg = true;
            break;
    }
}

void VDX7Engine::handleMidi(const uint8_t* data, int size)
{
    if (!loaded_ || data == nullptr || size <= 0)
        return;

    parseMidiBytes(data, size);
}

bool VDX7Engine::handleSysex(const uint8_t* data, std::size_t size)
{
    // No unchecked serial fallback: live parameter/single-voice dumps are not
    // supported yet. Single voices remain available through file import.
    return loadSyxBank(data, size);
}

void VDX7Engine::parseMidiBytes(const uint8_t* data, int size)
{
    if (size < 1 || size > 3)
        return;

    const uint8_t status = data[0] & 0xF0;

    switch (status)
    {
        case 0x80: // Note off
        case 0x90: // Note on (velocity zero is note off)
            if (size == 3 && data[1] < 128 && data[2] < 128)
            {
                const bool on = status == 0x90 && data[2] != 0;
                // The sub-CPU keyboard protocol only supports 61 keys. Use the
                // firmware MIDI receiver for all pitches, preserving legacy omni
                // input by normalising host channels to its receive channel.
                dx7_.midiSerialRx.write(static_cast<uint8_t>((on ? 0x90 : 0x80)
                    | (dx7_.getMidiRxChannel() & 0x0f)));
                dx7_.midiSerialRx.write(data[1]);
                dx7_.midiSerialRx.write(on ? mapVelocity(data[2]) : 0);
                activeMidiNotes_[data[1]] = on;
            }
            return;

        case 0xB0: // CC
            if (size < 3) return;
            switch (data[1])
            {
                case 0:   return; // Bank MSB
                case 100: return; // RPN LSB
                case 101: return; // RPN MSB
                case 1: toSynth_->analog(dx7Emu::Message::CtrlID::modulate, data[2]); return;
                case 2: toSynth_->analog(dx7Emu::Message::CtrlID::breath, data[2]); return;
                case 4: toSynth_->analog(dx7Emu::Message::CtrlID::foot, data[2]); return;
                case 6: toSynth_->analog(dx7Emu::Message::CtrlID::data, data[2]); return;
                case 11:
                    midiExpression_ = static_cast<float>(data[2]) / 127.0f;
                    return;
                case 32:
                    if (hasFactoryVoices())
                        selectFactoryBank(data[2] % 8);
                    return;
                case 64:
                    sustainDown_ = data[2] >= 64;
                    toSynth_->sustain(data[2] >= 64);
                    return;
                case 65:
                    toSynth_->porta(data[2] >= 64);
                    return;
                case 123:
                    allNotesOff();
                    break;
                default:
                    break;
            }
            break;

        case 0xC0: // Program change
            if (size >= 2)
                currentProgram_ = std::clamp<int>(data[1], 0, 31);
            break; // Also deliver to firmware serial interface.

        case 0xD0: // Channel pressure
            if (size >= 2)
            {
                toSynth_->analog(dx7Emu::Message::CtrlID::aftertouch, data[1]);
                return;
            }
            break;

        case 0xE0: // Pitch bend. VDX7 adapter uses the MSB.
            if (size >= 3)
            {
                toSynth_->analog(dx7Emu::Message::CtrlID::pitchbend, data[2]);
                return;
            }
            break;

        default:
            break;
    }

    // Ignore system realtime clock traffic; the DX7 firmware does not use it.
    if (data[0] >= 0xF8)
        return;

    // Match the note path's single-part omni policy for all channel messages,
    // including program changes and firmware-handled CC7 volume.
    for (int i = 0; i < size; ++i)
        dx7_.midiSerialRx.write(i == 0 && data[0] < 0xf0
            ? static_cast<uint8_t>((data[0] & 0xf0) | (dx7_.getMidiRxChannel() & 0x0f))
            : (status == 0xc0 && i == 1 ? static_cast<uint8_t>(currentProgram_) : data[i]));
}

void VDX7Engine::allNotesOff()
{
    sustainDown_ = false;
    if (!loaded_) return;
    toSynth_->analog(dx7Emu::Message::CtrlID::sustain, 0);
    for (int i = 0; i < 128; ++i)
        if (activeMidiNotes_[i])
        {
            dx7_.midiSerialRx.write(static_cast<uint8_t>(0x80 | (dx7_.getMidiRxChannel() & 0x0f)));
            dx7_.midiSerialRx.write(static_cast<uint8_t>(i));
            dx7_.midiSerialRx.write(0);
        }
    activeMidiNotes_.fill(false);
}

bool VDX7Engine::hasHeldMidiNotes() const noexcept
{
    return sustainDown_ || std::any_of(activeMidiNotes_.begin(), activeMidiNotes_.end(),
                                     [](bool held) { return held; });
}

bool VDX7Engine::loadSyxBank(const uint8_t* data, std::size_t size)
{
    if (!loaded_ || data == nullptr || size != 4104)
        return false;

    // Standard Yamaha DX7 32-voice bulk dump. Byte 2 contains the MIDI
    // channel/device number, so accept any value there (0..15) rather than
    // requiring channel 1. This matches the Retromulator VDX7 adapter.
    if (data[0] != 0xF0 || data[1] != 0x43 || (data[2] & 0xF0) != 0x00 ||
        data[3] != 0x09 || data[4] != 0x20 || data[5] != 0x00 || data[4103] != 0xF7)
        return false;

    for (std::size_t i = 6; i <= 4102; ++i)
        if (data[i] >= 128) return false;
    int checksum = data[4102];
    for (int i = 0; i < 4096; ++i)
        checksum += data[6 + i];
    if ((checksum & 0x7F) != 0)
        return false;

    std::memcpy(dx7_.memory + 0x1000, data + 6, 4096);
    dx7_.midiSerialRx.flush();
    dx7_.tune(0);
    currentBank_ = -1;
    selectProgram(currentProgram_);
    return true;
}

bool VDX7Engine::selectFactoryBank(int bankIndex)
{
    if (!loaded_ || !hasFactoryVoices() || bankIndex < 0 || bankIndex > 7)
        return false;

    dx7_.setBank(bankIndex, false);
    currentBank_ = bankIndex;
    selectProgram(currentProgram_);
    return true;
}

void VDX7Engine::selectProgram(int programIndex)
{
    currentProgram_ = std::clamp(programIndex, 0, 31);
    const uint8_t msg[2] = { 0xC0, static_cast<uint8_t>(currentProgram_) };
    parseMidiBytes(msg, 2);
}

std::string VDX7Engine::currentProgramName() const
{
    char buffer[11] {};
    copyCurrentProgramName(buffer, sizeof(buffer));
    std::string name(buffer);
    while (!name.empty() && name.back() == ' ')
        name.pop_back();
    return name;
}

void VDX7Engine::copyCurrentProgramName(char* destination, std::size_t capacity) const noexcept
{
    if (destination == nullptr || capacity == 0)
        return;

    destination[0] = '\0';
    if (!loaded_)
        return;

    const int program = std::clamp(currentProgram_, 0, 31);
    const uint8_t* voice = dx7_.memory + 0x1000 + program * 128;
    const auto length = std::min<std::size_t>(10, capacity - 1);

    for (std::size_t i = 0; i < length; ++i)
    {
        char c = static_cast<char>(voice[118 + static_cast<int>(i)]);
        destination[i] = (c >= 32 && c <= 126) ? c : ' ';
    }

    std::size_t trimmedLength = length;
    while (trimmedLength > 0 && destination[trimmedLength - 1] == ' ')
        --trimmedLength;
    destination[trimmedLength] = '\0';
}

int VDX7Engine::getOperatorParameter(int operatorIndex,
                                     VDX7VoiceData::Parameter parameter) const noexcept
{
    return VDX7VoiceData::getOperatorParameter(currentPackedVoice(),
                                                VDX7VoiceData::kPackedVoiceSize,
                                                operatorIndex, parameter);
}

bool VDX7Engine::setOperatorParameter(int operatorIndex,
                                      VDX7VoiceData::Parameter parameter,
                                      int value) noexcept
{
    return loaded_ && VDX7VoiceData::setOperatorParameter(
        currentPackedVoice(), VDX7VoiceData::kPackedVoiceSize,
        operatorIndex, parameter, value);
}

int VDX7Engine::getVoiceParameter(VDX7VoiceData::VoiceParameter parameter) const noexcept
{
    return VDX7VoiceData::getVoiceParameter(currentPackedVoice(),
                                             VDX7VoiceData::kPackedVoiceSize,
                                             parameter);
}

bool VDX7Engine::setVoiceParameter(VDX7VoiceData::VoiceParameter parameter,
                                   int value) noexcept
{
    return loaded_ && VDX7VoiceData::setVoiceParameter(
        currentPackedVoice(), VDX7VoiceData::kPackedVoiceSize, parameter, value);
}

void VDX7Engine::reloadCurrentProgram()
{
    if (loaded_)
        selectProgram(currentProgram_);
}

bool VDX7Engine::saveRam(std::vector<uint8_t>& out) const
{
    if (!loaded_)
        return false;
    return dx7_.saveRAM(out);
}

bool VDX7Engine::restoreRam(const std::vector<uint8_t>& in)
{
    if (!loaded_ || in.size() != kRamStateSize)
        return false;
    const bool ok = dx7_.restoreRAM(in);
    if (ok)
        selectProgram(currentProgram_);
    return ok;
}

uint8_t VDX7Engine::mapVelocity(uint8_t velocity) const
{
    return velocityMap_[static_cast<std::size_t>(velocity)];
}

uint8_t* VDX7Engine::currentPackedVoice() noexcept
{
    if (!loaded_)
        return nullptr;

    return dx7_.memory + 0x1000 + std::clamp(currentProgram_, 0, 31) * 128;
}

const uint8_t* VDX7Engine::currentPackedVoice() const noexcept
{
    if (!loaded_)
        return nullptr;

    return dx7_.memory + 0x1000 + std::clamp(currentProgram_, 0, 31) * 128;
}
