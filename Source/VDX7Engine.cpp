#include "VDX7Engine.h"
#include "VDX7MidiValidation.h"

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

    releaseRetirementProfile_ = isReleaseRetirementFirmware(firmware, kFirmwareSize);
    releaseHistoryDirty_ = false;

    // loadVoices(nullptr, 0) does NOT clear the core's previous pointer.
    dx7_.loadVoices(emptyFactoryBank_.data(), emptyFactoryBank_.size());
    factoryVoices_ = std::move(newFactoryVoices);
    if (!factoryVoices_.empty())
        dx7_.loadVoices(factoryVoices_.data(), factoryVoices_.size());
    hostResetInProgress_ = false;
    hostResetMuted_ = false;
    activeMidiNotes_.fill(0);
    midiReleaseBudget_.fill(0);
    sustainDown_ = false;
    midiRecovering_ = false;
    midiOverloadCount_ = 0;
    controllerRefreshMessages_ = 0;
    pitchBendRefresh_ = false;
    portamentoRefresh_ = false;
    lastPitchBendInput_ = 64;
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

void VDX7Engine::beginHostReset()
{
    beginMidiReset(false);
}

void VDX7Engine::beginMidiReset(bool releaseEveryPitch)
{
    hostResetInProgress_ = loaded_;
    hostResetMuted_ = true;
    resetAudioState();
    if (!loaded_) return;

    // Drop old adapter input, but let an already-started sub-CPU handshake
    // finish. Its second byte must not be mistaken for a new message.
    dx7_.midiSerialRx.flush();
    dx7_.midiSerialTx.flush();
    dx7Emu::Message ignored;
    for (int n = 0; n < 1024 && toSynth_->pop(ignored); ++n) {}
    midiRecovering_ = false;
    dx7_.sustain(false);
    dx7_.porta(false);
    sustainDown_ = false;

    // Include earlier, already-released notes whose firmware Note Off could
    // have been discarded from the adapter FIFO. Running status encodes the
    // same 128*16 releases in at most 1 + 128*16*2 serial bytes.
    // Public reset needs only received pitches. Stopped-device lifecycle also
    // sends at least one release per pitch, preserving its prior cleanup scope.
    activeMidiNotes_ = midiReleaseBudget_;
    if (releaseEveryPitch)
        for (auto& count : activeMidiNotes_) count = std::max<uint8_t>(count, 1);
    allNotesOff(true);
    toSynth_->porta(false);
}

void VDX7Engine::advanceHostReset(int sampleBudget)
{
    if (!hostResetInProgress_ || sampleBudget <= 0) return;
    if (!loaded_) { hostResetInProgress_ = false; return; }

    std::array<float, 64> left {}, right {};
    for (int remaining = sampleBudget; remaining > 0;)
    {
        const int count = std::min(remaining, static_cast<int>(left.size()));
        render(left.data(), right.data(), count);
        remaining -= count;

        // These firmware-ring locations are the same ones used by the existing
        // mode transaction. Also include the SCI receive register and the
        // sub-CPU handshake: adapter-empty alone is not a completed reset.
        const bool pending = dx7_.midiSerialRx.readIdx != dx7_.midiSerialRx.writeIdx
            || (dx7_.TRCSR & (1u << dx7Emu::HD6303R::RDRF)) != 0
            || dx7_.memory[0xee] != dx7_.memory[0xf0]
            || dx7_.memory[0xef] != dx7_.memory[0xf1]
            || dx7_.memory[0xf6] != 0
            || dx7_.haveMsg || !appToSynth_.lfq.wasEmpty();
        if (pending) continue;

        // Releases have reached the firmware. Now discard DSP tails without
        // changing the packed voice bank or global battery-RAM settings.
        dx7_.sustain(false);
        dx7_.porta(false);
        sustainDown_ = false;
        activeMidiNotes_.fill(0);
        std::destroy_at(&dx7_.egs);
        std::construct_at(&dx7_.egs, dx7_.memory + 0x3000);
        resetAudioState();
        controllerRefreshMessages_ = 2;
        pitchBendRefresh_ = true;
        portamentoRefresh_ = true;
        hostResetInProgress_ = false;
        selectProgram(currentProgram_);
        return;
    }
}

void VDX7Engine::resetMidiLifecycle()
{
    beginMidiReset(true);
    // Non-RT lifecycle may advance up to two seconds of emulated audio, with
    // a fixed sample ceiling. Completion uses the same input-stage predicate
    // as public reset, NOT elapsed time. Never flush unconsumed releases or
    // abort a sub-CPU handshake at this bound: processBlock will finish the
    // pending reset muted and defer fresh MIDI via its existing bounded path.
    // This does not increase the two-second deferred-MIDI age limit.
    advanceHostReset(static_cast<int>(std::min(hostSampleRate_ * 2.0, 384000.0)));
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
        // A nonzero envelope L4 can otherwise become audible again after
        // program reload. Only a fresh accepted Note On opens the reset gate.
        const float audible = hostResetMuted_ ? 0.0f : out;
        if (left != nullptr) left[i] = audible;
        if (right != nullptr) right[i] = audible;

    }
    if (midiRecovering_)
    {
        // Restore the latest requested program after the complete release batch.
        dx7_.midiSerialRx.write(static_cast<uint8_t>(0xc0 | (dx7_.getMidiRxChannel() & 15)));
        dx7_.midiSerialRx.write(static_cast<uint8_t>(currentProgram_));
        midiRecovering_ = false;
    }
}

bool VDX7Engine::reserveMidi(int bytes)
{
    if (midiRecovering_) return false;
    const auto& queue = dx7_.midiSerialRx;
    const int occupied = (queue.writeIdx - queue.readIdx) & (queue.size - 1);
    // Leave one slot empty: the core uses readIdx == writeIdx for empty and
    // its unchecked writer would otherwise wrap over unread message bytes.
    if (bytes > queue.size - 1 - occupied || appToSynth_.lfq.wasFull())
    { recoverMidiOverflow(); return false; }
    return true;
}

void VDX7Engine::recoverMidiOverflow()
{
    ++midiOverloadCount_;
    midiRecovering_ = true;
    dx7_.midiSerialRx.flush();
    dx7Emu::Message ignored;
    for (int i = 0; i < 1024 && toSynth_->pop(ignored); ++i) {}
    // Do not abort a half-delivered sub-CPU message: its second byte must still
    // reach firmware. Only queued (not in-flight) controller messages are lost.
    dx7_.sustain(false);
    dx7_.porta(false);
    sustainDown_ = false;
    // Release every pitch, not just wrapper ownership: some previous bytes
    // may already have reached the firmware, or a note may be sustained.
    for (int note = 0; note < 128; ++note)
    for (int repeat = 0; repeat < std::max<int>(1, midiReleaseBudget_[note]); ++repeat)
    {
        dx7_.midiSerialRx.write(static_cast<uint8_t>(0x80 | (dx7_.getMidiRxChannel() & 15)));
        dx7_.midiSerialRx.write(static_cast<uint8_t>(note));
        dx7_.midiSerialRx.write(0);
    }
    activeMidiNotes_.fill(0);
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
        if (portamentoRefresh_ && !midiRecovering_
            && dx7_.midiSerialRx.readIdx == dx7_.midiSerialRx.writeIdx)
        {
            portamentoRefresh_ = false;
            dx7_.midiSerialRx.write(static_cast<uint8_t>(0xb0 | (dx7_.getMidiRxChannel() & 15)));
            dx7_.midiSerialRx.write(5);
            dx7_.midiSerialRx.write(static_cast<uint8_t>((getPlaySetting(3) * 128 + 99) / 100));
        }
        if (!dx7_.haveMsg)
        {
            if (toSynth_->pop(msg)) processQueuedMessage(msg);
            else if (pitchBendRefresh_)
            {
                pitchBendRefresh_ = false;
                processQueuedMessage({dx7Emu::Message::CtrlID::pitchbend,
                    lastPitchBendInput_});
            }
            else if (controllerRefreshMessages_ != 0)
            {
                // Firmware scales all four sources every second analog event.
                // Re-submit the latest firmware wheel input after queued input
                // drains; do not inject stale values into the bounded FIFO.
                --controllerRefreshMessages_;
                // IRQ expands the seven-bit sub-CPU value by one left shift.
                processQueuedMessage({dx7Emu::Message::CtrlID::modulate,
                    static_cast<uint8_t>(dx7_.memory[0x2337] >> 1)});
            }
        }

        retireCompletedReleaseHistory();
        dx7_.run();
        const int cycles = (dx7_.inst != nullptr && dx7_.inst->cycles > 0) ? dx7_.inst->cycles : 1;

        dx7_.egs.clock(out, outCount, 4 * cycles);
    }

    return outCount;
}

bool VDX7Engine::isReleaseRetirementFirmware(const uint8_t* data, std::size_t size)
{
    // Compatibility fingerprint, not a security hash. Unknown/modified ROMs
    // retain conservative history. No firmware bytes are embedded here.
    if (data == nullptr || size != kFirmwareSize) return false;
    uint64_t fingerprint = UINT64_C(14695981039346656037);
    for (std::size_t i = 0; i < size; ++i)
        fingerprint = (fingerprint ^ data[i]) * UINT64_C(1099511628211);
    return fingerprint == UINT64_C(0x20dd25e47a496ba0);
}

void VDX7Engine::retireCompletedReleaseHistory()
{
    // Observe BEFORE the next CPU instruction, at the v1.8 main-loop entry.
    // The preceding MIDI/pedal dispatch has returned; never inspect a partially
    // completed ownership update. POLY only until MONO/legato is validated.
    if (!releaseHistoryDirty_ || !releaseRetirementProfile_ || dx7_.PC != 0xc708
        || hostResetInProgress_ || midiRecovering_ || sustainDown_) return;
    const auto& m = dx7_.memory;
    if (m[0x20a9] != 0 || m[0xe7] != 0 || m[0xe8] != 0 || m[0xf6] != 0
        || (m[0x83] & 1) != 0 || (m[0x20a7] & 1) != 0
        || dx7_.midiSerialRx.readIdx != dx7_.midiSerialRx.writeIdx
        || (dx7_.TRCSR & (1u << dx7Emu::HD6303R::RDRF)) != 0
        || m[0xee] != m[0xf0] || m[0xef] != m[0xf1]
        || dx7_.haveMsg || dx7_.byte1Sent || !appToSynth_.lfq.wasEmpty()) return;
    // A held neighboring pitch must not keep completed release history alive.
    // Both tables matter: MIDI ownership and actual held/sustained voices can
    // disagree. Keep the entire high-water budget for any still-owned pitch.
    std::array<bool, 128> firmwareOwned{};
    for (int i = 0; i < 16; ++i)
    {
        if ((m[0x2168 + i] & 0x80) != 0)
            firmwareOwned[m[0x2168 + i] & 0x7f] = true;
        if ((m[0x20b1 + 2 * i] & 3) != 0)
        {
            const auto note = m[0x20b0 + 2 * i];
            if (note >= firmwareOwned.size()) return; // Unexpected RAM: stay conservative.
            firmwareOwned[note] = true;
        }
    }
    releaseHistoryDirty_ = false;
    for (std::size_t note = 0; note < midiReleaseBudget_.size(); ++note)
    {
        if (activeMidiNotes_[note] == 0 && !firmwareOwned[note])
            midiReleaseBudget_[note] = 0;
        releaseHistoryDirty_ |= midiReleaseBudget_[note] != 0;
    }
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
            // A fresh queued wheel event already refreshes the new settings.
            // Do not follow it with an older RAM value before firmware consumes it.
            pitchBendRefresh_ = false;
            lastPitchBendInput_ = msg.byte2;
            // The firmware owns range, step quantisation and the resulting bend.
            // Adding a second EGS offset bypassed zero range and double-counted bend.
            dx7_.pitchBendOffset = 0;
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
    if (size <= 0 || !VDX7MidiValidation::isChannelMessage(data, static_cast<std::size_t>(size)))
        return;

    if (data[0] >= 0xf8) return;
    // Unsupported bank requests must not trigger serial-overflow recovery.
    if ((data[0] & 0xf0) == 0xb0 && size == 3 && data[1] == 32
        && (data[2] >= 8 || !hasFactoryVoices())) return;
    if (!reserveMidi(size)) return;

    const uint8_t status = data[0] & 0xF0;

    switch (status)
    {
        case 0x80: // Note off
        case 0x90: // Note on (velocity zero is note off)
            if (size == 3 && data[1] < 128 && data[2] < 128)
            {
                const bool on = status == 0x90 && data[2] != 0;
                if (on) hostResetMuted_ = false;
                // The sub-CPU keyboard protocol only supports 61 keys. Use the
                // firmware MIDI receiver for all pitches, preserving legacy omni
                // input by normalising host channels to its receive channel.
                dx7_.midiSerialRx.write(static_cast<uint8_t>((on ? 0x90 : 0x80)
                    | (dx7_.getMidiRxChannel() & 0x0f)));
                dx7_.midiSerialRx.write(data[1]);
                dx7_.midiSerialRx.write(on ? mapVelocity(data[2]) : 0);
                auto& held = activeMidiNotes_[data[1]];
                if (on) {
                    releaseHistoryDirty_ = true;
                    held = std::min<int>(kMaxRepeatedNotes, held + 1);
                    midiReleaseBudget_[data[1]] = std::max(midiReleaseBudget_[data[1]], held);
                }
                else if (held != 0) --held;
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
                case 5: portamentoRefresh_ = false; break; // Native time/rate update.
                case 11:
                    midiExpression_ = static_cast<float>(data[2]) / 127.0f;
                    return;
                case 32:
                    if (data[2] < 8)
                        selectFactoryBank(data[2]);
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
                    if (midiRecovering_) return;
                    break; // Keep the original CC123 delivery to firmware too.
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

void VDX7Engine::allNotesOff(bool useRunningStatus)
{
    sustainDown_ = false;
    if (!loaded_) return;
    int releaseBytes = 3; // Include a possible following CC123.
    int releases = 0;
    for (auto held : activeMidiNotes_) releases += held;
    releaseBytes += useRunningStatus ? (releases > 0 ? 1 + releases * 2 : 0) : releases * 3;
    if (!reserveMidi(releaseBytes)) return;
    toSynth_->analog(dx7Emu::Message::CtrlID::sustain, 0);
    // Reset owns one contiguous serial batch: all releases have the same
    // channel/status. MIDI running status removes only repeated status bytes,
    // never Note Offs. Ordinary MIDI and overflow recovery keep their encoding.
    bool first = true;
    for (int i = 0; i < 128; ++i)
        for (int repeat = 0; repeat < activeMidiNotes_[i]; ++repeat)
        {
            if (!useRunningStatus || first)
                dx7_.midiSerialRx.write(static_cast<uint8_t>(0x80 | (dx7_.getMidiRxChannel() & 0x0f)));
            first = false;
            dx7_.midiSerialRx.write(static_cast<uint8_t>(i));
            dx7_.midiSerialRx.write(0);
        }
    activeMidiNotes_.fill(0);
}

bool VDX7Engine::hasHeldMidiNotes() const noexcept
{
    return sustainDown_ || std::any_of(activeMidiNotes_.begin(), activeMidiNotes_.end(),
                                     [](uint8_t held) { return held != 0; });
}

bool VDX7Engine::loadSyxBank(const uint8_t* data, std::size_t size)
{
    if (!loaded_ || midiRecovering_)
        return false;

    // Keep live admission and engine import on the same bounded validator.
    // It accepts standard 32-voice bulk data, including device IDs 0..15.
    if (!VDX7MidiValidation::isLiveBankSysex(data, size))
        return false;

    // Bulk bank SysEx supplies voice RAM only. Preserve the global firmware
    // tuning that lives outside that range, so importing a bank cannot alter a
    // project-level SETTINGS value.
    const auto tuning = masterTune();
    std::memcpy(dx7_.memory + 0x1000, data + 6, 4096);
    // Preserve queued note-offs, including an overload recovery still draining
    // through firmware. A bank replacement must not silently erase releases.
    dx7_.tune(tuning);
    currentBank_ = -1;
    selectProgram(currentProgram_);
    return true;
}

bool VDX7Engine::selectFactoryBank(int bankIndex)
{
    if (!loaded_ || !hasFactoryVoices() || bankIndex < 0 || bankIndex > 7)
        return false;

    dx7_.setBank(bankIndex, false);
    ++factoryBankLoadRevision_;
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

int VDX7Engine::getPlaySetting(int field) const noexcept
{
    constexpr int addresses[] {0x20a9, 0x20aa, 0x20ab, 0x257d};
    return loaded_ && field >= 0 && field < 4
        ? std::clamp(int(dx7_.memory[addresses[field]]), 0, field == 3 ? 99 : 1) : 0;
}

int VDX7Engine::masterTune() const noexcept
{
    return loaded_ ? std::clamp((int(dx7_.memory[0x2311]) << 8)
                              + int(dx7_.memory[0x2312]) - 256, -256, 255) : 0;
}

bool VDX7Engine::setMasterTune(int value) noexcept
{
    if (!loaded_ || value < -256 || value > 255) return false;
    dx7_.tune(value);
    return true;
}

bool VDX7Engine::setPlaySetting(int field, int value)
{
    if (!loaded_ || field < 0 || field > 3 || value < 0 || value > (field == 3 ? 99 : 1)) return false;
    if (getPlaySetting(field) == value) return true;
    if (field == 1 || field == 2)
    {
        dx7_.memory[field == 1 ? 0x20aa : 0x20ab] = static_cast<uint8_t>(value);
        return true;
    }
    if (field == 0)
    {
        // Do not leave a mode command queued behind a backlog after returning
        // a busy error. Drain earlier work before submitting the mode change.
        std::array<float, 64> left {}, right {};
        const auto pending = [this]
        {
            // Bytes leave the adapter FIFO before the firmware's own receive
            // ring has processed them. Both queues must be considered.
            return dx7_.midiSerialRx.readIdx != dx7_.midiSerialRx.writeIdx
                || dx7_.memory[0xee] != dx7_.memory[0xf0]
                || dx7_.memory[0xef] != dx7_.memory[0xf1]
                || dx7_.memory[0xf6] != 0;
        };
        for (int n = 0; n < 192 && pending(); ++n)
            render(left.data(), right.data(), 64);
        if (pending() || midiRecovering_) return false;
    }
    if (!reserveMidi(3)) return false;
    // Native CC5 computes the derived portamento rate. Its input maps to
    // floor(CC * 100 / 128); ceil(value * 128 / 100) reaches every 0-99 value.
    const uint8_t message[] {static_cast<uint8_t>(0xb0 | (dx7_.getMidiRxChannel() & 15)),
        static_cast<uint8_t>(field == 3 ? 5 : value ? 126 : 127),
        static_cast<uint8_t>(field == 3 ? (value * 128 + 99) / 100 : value)};
    for (auto byte : message) dx7_.midiSerialRx.write(byte);
    if (field == 3)
    {
        portamentoRefresh_ = false;
        // Persist the requested setting immediately; firmware updates its rate
        // through the queued CC before subsequent serial note events.
        dx7_.memory[0x257d] = static_cast<uint8_t>(value);
        return true;
    }
    // Do not change mono/poly RAM before the native command: it would bypass
    // firmware's mode-change voice reset. This path is UI-only, never audio.
    std::array<float, 64> left {}, right {};
    for (int n = 0; n < 192; ++n)
    {
        render(left.data(), right.data(), 64);
        if (getPlaySetting(0) == value)
        {
            // Complete the reset after its mode byte is written.
            render(left.data(), right.data(), 64);
            activeMidiNotes_.fill(0);
            resetAudioState();
            return true;
        }
    }
    return false;
}

int VDX7Engine::getPitchBendSetting(int field) const noexcept
{
    return loaded_ && field >= 0 && field < 2 ? std::clamp(int(dx7_.memory[0x2328 + field]), 0, 12) : 0;
}

bool VDX7Engine::setPitchBendSetting(int field, int value) noexcept
{
    if (!loaded_ || field < 0 || field > 1 || value < 0 || value > 12) return false;
    auto& target = dx7_.memory[0x2328 + field];
    if (target != value) { target = static_cast<uint8_t>(value); pitchBendRefresh_ = true; }
    return true;
}

int VDX7Engine::getControllerSetting(int controller, int field) const noexcept
{
    if (!loaded_ || controller < 0 || controller >= 4 || field < 0 || field >= 4)
        return 0;
    // Firmware order: wheel, foot, breath, aftertouch. The pinned core's foot /
    // breath field names are swapped; use the verified firmware locations.
    constexpr int offsets[] { 0, 2, 4, 6 };
    const int offset = offsets[controller];
    if (field == 0) return std::clamp(int(dx7_.memory[0x2336 + offset]), 0, 99);
    return (dx7_.memory[0x232e + offset] >> (field - 1)) & 1;
}

bool VDX7Engine::setControllerSetting(int controller, int field, int value) noexcept
{
    if (!loaded_ || controller < 0 || controller >= 4 || field < 0 || field >= 4
        || value < 0 || value > (field == 0 ? 99 : 1)) return false;
    constexpr int offsets[] { 0, 2, 4, 6 };
    const int offset = offsets[controller];
    auto& target = dx7_.memory[(field == 0 ? 0x2336 : 0x232e) + offset];
    const auto previous = target;
    if (field == 0) target = static_cast<uint8_t>(value);
    else
    {
        const int mask = 1 << (field - 1);
        target = static_cast<uint8_t>((target & ~mask) | (value != 0 ? mask : 0));
    }
    if (previous != target) controllerRefreshMessages_ = 2;
    return true;
}

bool VDX7Engine::restoreRam(const std::vector<uint8_t>& in)
{
    if (!loaded_ || in.size() != kRamStateSize)
        return false;
    const bool ok = dx7_.restoreRAM(in);
    if (ok)
    {
        controllerRefreshMessages_ = 2;
        pitchBendRefresh_ = true;
        portamentoRefresh_ = true;
        // Queue before selectProgram and future host notes, so the restored
        // rate is computed before their portamento starts. Keep the deferred
        // fallback only when an existing recovery/full FIFO prevents insertion.
        const auto& rx = dx7_.midiSerialRx;
        const int occupied = (rx.writeIdx - rx.readIdx) & (rx.size - 1);
        if (!midiRecovering_ && occupied <= rx.size - 4)
        {
            dx7_.midiSerialRx.write(static_cast<uint8_t>(0xb0 | (dx7_.getMidiRxChannel() & 15)));
            dx7_.midiSerialRx.write(5);
            dx7_.midiSerialRx.write(static_cast<uint8_t>((getPlaySetting(3) * 128 + 99) / 100));
            portamentoRefresh_ = false;
        }
        lastPitchBendInput_ = static_cast<uint8_t>(dx7_.memory[0x232a] >> 1);
        selectProgram(currentProgram_);
    }
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
