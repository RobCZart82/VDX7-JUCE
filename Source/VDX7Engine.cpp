#include "VDX7Engine.h"

#include <algorithm>
#include <cmath>
#include <cstring>

VDX7Engine::VDX7Engine()
    : dx7_(toSynth_, toGui_)
{
    toSynth_ = &appToSynth_;
    toGui_ = &nullToGui_;

    cpuCyclesPerNativeSample_ = (9.4265e6 / 2.0 / 4.0) / kNativeSampleRate;

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
    loaded_ = false;
    factoryVoices_.clear();

    if (data == nullptr)
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

    if (!dx7_.loadFirmware(firmware, kFirmwareSize))
        return false;

    if (voices != nullptr && voicesSize >= kFactoryVoicesSize)
    {
        factoryVoices_.assign(voices, voices + kFactoryVoicesSize);
        if (!dx7_.loadVoices(factoryVoices_.data(), factoryVoices_.size()))
            factoryVoices_.clear();
    }

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
    hostSampleRate_ = hostSampleRate > 1000.0 ? hostSampleRate : 48000.0;
    resetAudioState();
}

void VDX7Engine::resetAudioState()
{
    nativePos_ = 0;
    nativeCount_ = 0;
    cpuCycleBudget_ = 0.0;
    resamplePhase_ = 0.0;
    resampleA_ = 0.0f;
    resampleB_ = 0.0f;
    resamplerPrimed_ = false;
    dx7_.midiFilter.reset();
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

    if (!resamplerPrimed_)
    {
        resampleA_ = nextNativeSample();
        resampleB_ = nextNativeSample();
        resamplerPrimed_ = true;
    }

    const double step = kNativeSampleRate / hostSampleRate_;

    for (int i = 0; i < numSamples; ++i)
    {
        const float out = resampleA_ + (resampleB_ - resampleA_) * static_cast<float>(resamplePhase_);
        if (left != nullptr) left[i] = out;
        if (right != nullptr) right[i] = out;

        resamplePhase_ += step;
        while (resamplePhase_ >= 1.0)
        {
            resampleA_ = resampleB_;
            resampleB_ = nextNativeSample();
            resamplePhase_ -= 1.0;
        }
    }
}

float VDX7Engine::nextNativeSample()
{
    if (nativePos_ >= nativeCount_)
    {
        nativeCount_ = generateNative(nativeBlock_.data(), kNativeBlockSize);
        nativePos_ = 0;
        if (nativeCount_ <= 0)
            return 0.0f;
    }

    const float raw = nativeBlock_[static_cast<std::size_t>(nativePos_++)];
    const float midiVolume = std::min(1.0f,
        dx7_.midiVolTab[dx7_.midiVolume] + midiExpression_ + 1.0e-18f);
    return raw * volume_ * dx7_.midiFilter.operate(midiVolume);
}

int VDX7Engine::generateNative(float* out, int maxSamples)
{
    if (!loaded_ || out == nullptr || maxSamples <= 0)
        return 0;

    cpuCycleBudget_ += cpuCyclesPerNativeSample_ * static_cast<double>(maxSamples);
    int outCount = 0;
    int discardCount = 0;
    dx7Emu::Message msg;

    while (cpuCycleBudget_ > 0.0)
    {
        if (!dx7_.haveMsg && toSynth_->pop(msg))
            processQueuedMessage(msg);

        dx7_.run();
        const int cycles = (dx7_.inst != nullptr && dx7_.inst->cycles > 0) ? dx7_.inst->cycles : 1;

        if (outCount < maxSamples)
        {
            dx7_.egs.clock(out, outCount, 4 * cycles);
        }
        else
        {
            if (discardCount >= kNativeBlockSize)
                discardCount = 0;
            dx7_.egs.clock(discardBlock_.data(), discardCount, 4 * cycles);
        }

        cpuCycleBudget_ -= static_cast<double>(cycles);
    }

    return std::min(outCount, maxSamples);
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

void VDX7Engine::handleSysex(const uint8_t* data, std::size_t size)
{
    if (!loaded_ || data == nullptr || size == 0)
        return;

    if (size == 4104 && loadSyxBank(data, size))
        return;

    for (std::size_t i = 0; i < size; ++i)
        dx7_.midiSerialRx.write(data[i]);
}

void VDX7Engine::parseMidiBytes(const uint8_t* data, int size)
{
    if (size < 1 || size > 3)
        return;

    const uint8_t status = data[0] & 0xF0;

    switch (status)
    {
        case 0x80: // Note off
            if (size >= 2 && data[1] >= 36 && data[1] <= 96)
                toSynth_->key_off(static_cast<uint8_t>(data[1] - 36));
            return;

        case 0x90: // Note on
            if (size >= 3 && data[1] >= 36 && data[1] <= 96)
            {
                if (data[2] == 0)
                    toSynth_->key_off(static_cast<uint8_t>(data[1] - 36));
                else
                    toSynth_->key_on(static_cast<uint8_t>(data[1] - 36), mapVelocity(data[2]));
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
                    {
                        currentBank_ = data[2] % 8;
                        dx7_.setBank(currentBank_, true);
                    }
                    return;
                case 64:
                    toSynth_->analog(dx7Emu::Message::CtrlID::sustain, data[2]);
                    return;
                case 65:
                    toSynth_->analog(dx7Emu::Message::CtrlID::porta, data[2]);
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

    for (int i = 0; i < size; ++i)
        dx7_.midiSerialRx.write(data[i]);
}

void VDX7Engine::allNotesOff()
{
    for (int i = 0; i < 61; ++i)
        toSynth_->key_off(static_cast<uint8_t>(i));
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
    const int p = std::clamp(currentProgram_, 0, 31);
    const uint8_t* voice = dx7_.memory + 0x1000 + p * 128;
    std::string name;
    name.reserve(10);
    for (int i = 0; i < 10; ++i)
    {
        char c = static_cast<char>(voice[118 + i]);
        if (c < 32 || c > 126) c = ' ';
        name.push_back(c);
    }
    while (!name.empty() && name.back() == ' ')
        name.pop_back();
    return name;
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
