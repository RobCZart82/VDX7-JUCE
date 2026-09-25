#include "PluginProcessor.h"
#include "VDX7Sysex.h"
#include <cmath>
#include <future>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <thread>

// Processor/engine fixtures are heap-owned so nested, multi-instance tests fit
// the default Windows stack without changing object lifetimes or assertions.
static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

struct VDX7RegressionAccess
{
    static void checkInputChannel(VDX7AudioProcessor& p)
    {
        juce::AudioBuffer<float> audio(2, 256);
        juce::MidiBuffer midi;
        require(!p.setMidiInputChannelFromUi(-1) && !p.setMidiInputChannelFromUi(17), "channel bounds");
        require(p.getMidiInputChannel() == 0, "legacy OMNI default");
        for (int selected : {1, 16, 7})
        {
            require(p.setMidiInputChannelFromUi(selected), "set input channel");
            for (int ch = 1; ch <= 16; ++ch)
                midi.addEvent(juce::MidiMessage::noteOn(ch, 40+ch, uint8_t(100)), 0);
            p.processBlock(audio, midi);
            for (int ch = 1; ch <= 16; ++ch)
                require(p.engine_.activeMidiNotes_[40+ch] == (ch == selected), "only selected channel sounds");
        }
        const int program = p.engine_.currentProgram();
        const float expression = p.engine_.midiExpression_;
        midi.addEvent(juce::MidiMessage::programChange(1, (program+1)%32), 0);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 11, 0), 0);
        p.processBlock(audio, midi);
        require(p.engine_.currentProgram() == program && p.engine_.midiExpression_ == expression,
                "rejected channel cannot change program or expression");
        midi.addEvent(juce::MidiMessage::controllerEvent(7, 64, 127), 0);
        p.processBlock(audio, midi);
        require(p.engine_.sustainDown_, "selected channel sustain");
        midi.addEvent(juce::MidiMessage::noteOn(7, 70, uint8_t(100)), 0);
        contend(p, midi); // Pending old-channel note must never replay.
        p.setMidiInputChannelFromUi(2);
        midi.addEvent(juce::MidiMessage::noteOn(2, 72, uint8_t(100)), 0);
        contend(p, midi); // Change while engine lock is unavailable.
        p.processBlock(audio, midi);
        require(!p.engine_.sustainDown_ && !p.engine_.activeMidiNotes_[47]
                && !p.engine_.activeMidiNotes_[70] && p.engine_.activeMidiNotes_[72],
                "switch clears old timeline and sustain, preserves new-channel event");
        p.keyboardState_.noteOn(1, 75, 1.0f);
        p.processBlock(audio, midi);
        require(p.engine_.activeMidiNotes_[75], "UI keyboard bypasses host channel filter");
        p.keyboardState_.noteOff(1, 75, 0.0f);
        p.setMidiInputChannelFromUi(0);
        p.processBlock(audio, midi);
        require(!p.engine_.hasHeldMidiNotes(), "return to OMNI releases notes");
    }
    static void checkStateRestoreDropsDeferredMidi(VDX7AudioProcessor& p)
    {
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 59, uint8_t(100)), 0);
        p.processBlock(audio, midi);
        require(p.engine_.activeMidiNotes_[59], "note sounds before state restore");

        juce::MemoryBlock state;
        p.getStateInformation(state);

        midi.clear();
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
        contend(p, midi);
        require(p.deferredMidi_.active(), "contention defers note before state restore");

        p.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        juce::MidiBuffer empty;
        p.processBlock(audio, empty);
        require(!p.engine_.activeMidiNotes_[59] && !p.engine_.activeMidiNotes_[60]
                    && !p.engine_.hasHeldMidiNotes(),
                "state restore releases old notes and drops pre-restore deferred MIDI");
    }
    static void checkInvalidMidi(VDX7AudioProcessor& p)
    {
        const std::vector<std::vector<uint8_t>> invalid {
            {0x90}, {0x90, 60}, {0x90, 60, 128}, {0x80, 128, 0},
            {0xb0, 11, 255}, {0xb0, 64, 128}, {0xc0}, {0xc0, 255},
            {0xc0, 3, 0}, {0xd0, 128}, {0xe0, 128, 64}, {0x40, 60, 100},
            {0xf1, 0}, {0xf2, 0, 0}, {0xf3, 0}, {0xf6}, {0xf8}, {0xff}
        };
        p.engine_.resetMidiLifecycle();
        const auto program = p.engine_.currentProgram();
        const auto bank = p.engine_.currentBank();
        const auto expression = p.engine_.midiExpression_;
        const auto revision = p.engine_.factoryBankLoadRevision();
        const auto write = p.engine_.dx7_.midiSerialRx.writeIdx;
        const auto overload = p.engine_.midiOverloadCount();
        for (const auto& event : invalid)
        {
            p.engine_.handleMidi(event.data(), static_cast<int>(event.size()));
            require(!p.handleMidiEventLocked(event.data(), static_cast<int>(event.size())),
                    "invalid MIDI cannot publish program state");
        }
        require(p.engine_.currentProgram() == program && p.engine_.currentBank() == bank
                && p.engine_.midiExpression_ == expression && p.engine_.factoryBankLoadRevision() == revision,
                "invalid MIDI preserves engine metadata");
        require(p.engine_.dx7_.midiSerialRx.writeIdx == write && p.engine_.midiOverloadCount() == overload,
                "invalid MIDI never enters serial queue");
        for (const auto& note : p.keyboardSnapshot_) require(note.load() == 0, "invalid MIDI cannot light a key");

        // The reported lost Note Off must not become a mirrored/retriggered UI note.
        p.keyboardState_.noteOn(1, 60, 1.0f);
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        p.processBlock(audio, midi);
        for (int i = 0; i < 140; ++i)
        { p.keyboardState_.noteOn(1, 61, 1.0f); p.keyboardState_.noteOff(1, 61, 0.0f); }
        p.keyboardState_.noteOff(1, 60, 0.0f); // Dropped queue event still clears UI ownership.
        p.processBlock(audio, midi);
        p.mirrorKeyboardOnMessageThread();
        require(!p.keyboardState_.isNoteOn(1, 60) && !p.keyboardState_.isNoteOn(1, 61),
                "overflow plus UI releases does not leave keys lit");
        require(!p.engine_.hasHeldMidiNotes(), "overflow plus releases reconciles engine notes");
        VDX7KeyboardQueue::Event event;
        require(!p.keyboardQueue_.pop(event), "mirroring does not retrigger MIDI");
        for (int i = 0; i < 300; ++i)
            midi.addEvent(juce::MidiMessage::midiClock(), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 62, uint8_t(100)), 0);
        contend(p, midi);
        p.processBlock(audio, midi);
        require(p.engine_.activeMidiNotes_[62], "ignored clock traffic cannot overflow deferred queue");
        midi.addEvent(juce::MidiMessage::noteOff(1, 62), 0);
        p.processBlock(audio, midi);
        // contend() delays a 256-sample block; preserve that timeline rather
        // than expecting a later host note-off in the first 64-sample block.
        for (int block = 0; block < 8; ++block) p.processBlock(audio, midi);
        require(!p.engine_.hasHeldMidiNotes(), "valid note-off works after ignored traffic");
    }
    static void checkSupportedNoteRange(VDX7AudioProcessor& p)
    {
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        for (const bool correction : {false, true})
        {
            require(p.setMonoCorrectionFromUi(correction), "select each MONO compatibility mode");
            const int queuedBefore = p.engine_.dx7_.midiSerialRx.writeIdx;
            for (const auto note : {0, 11, 121, 127})
            {
                const uint8_t on[] {0x90, static_cast<uint8_t>(note), 100};
                const uint8_t off[] {0x80, static_cast<uint8_t>(note), 0};
                const uint8_t zeroVelocityOn[] {0x90, static_cast<uint8_t>(note), 0};
                require(!p.handleMidiEventLocked(on, 3) && !p.handleMidiEventLocked(off, 3)
                        && !p.handleMidiEventLocked(zeroVelocityOn, 3),
                        "out-of-range note-on and release events are rejected at processor boundary");
            }
            require(p.engine_.dx7_.midiSerialRx.writeIdx == queuedBefore
                    && !p.engine_.hasHeldMidiNotes(),
                    "rejected range cannot enter firmware or leave a held-note record");

            // The built-in keyboard has a separate collection path; exercise
            // its lower out-of-range event while the engine lock is contended.
            p.keyboardState_.noteOn(1, 0, 1.0f);
            midi.clear();
            VDX7RegressionAccess::contend(p, midi);
            require(!p.deferredMidi_.active() && !p.engine_.hasHeldMidiNotes(),
                    "unsupported GUI key must not enter deferred MIDI or reach firmware");
            p.keyboardState_.noteOff(1, 0, 0.0f);
            midi.clear();
            p.processBlock(audio, midi);
            require(!p.engine_.hasHeldMidiNotes(), "unsupported GUI key release remains inert");

            // Also flood out-of-range events while rendering is deferred: they
            // must not consume deferred capacity or trigger its overflow panic.
            midi.clear();
            for (int i = 0; i < 160; ++i)
            {
                midi.addEvent(juce::MidiMessage::noteOn(1, i % 12, uint8_t(100)), 0);
                midi.addEvent(juce::MidiMessage::noteOff(1, i % 12), 1);
            }
            VDX7RegressionAccess::contend(p, midi);
            require(!p.deferredMidi_.active() && !p.engine_.isMidiRecovering(),
                    "filtered pitch events do not fill delayed MIDI or trigger panic");

            midi.clear();
            for (const auto note : {12, 60, 120})
                midi.addEvent(juce::MidiMessage::noteOn(1, note, uint8_t(100)), 0);
            p.processBlock(audio, midi);
            for (const auto note : {12, 60, 120})
                require(p.engine_.activeMidiNotes_[note], "supported boundary note reaches firmware");
            for (const auto note : {12, 60, 120})
                midi.addEvent(juce::MidiMessage::noteOff(1, note), 0);
            p.processBlock(audio, midi);
            require(!p.engine_.hasHeldMidiNotes(), "supported boundary notes release cleanly");
        }
        require(!p.getMonoCorrectionStatus().requested, "native mode restored after range acceptance");
    }
    static void checkFactoryCc32DeferredCapacity(VDX7AudioProcessor& p, bool hasFactoryVoices)
    {
        require(p.factoryVoicesAvailable_.load(std::memory_order_acquire) == hasFactoryVoices,
                "factory-image control has expected availability");
        p.prepareToPlay(48000, 256);
        juce::AudioBuffer<float> audio(2, 256);
        juce::MidiBuffer events;
        const int bankEvents = hasFactoryVoices ? 255 : 256;
        for (int i = 0; i < bankEvents; ++i)
            events.addEvent(juce::MidiMessage::controllerEvent(1, 32, i % 8), 0);
        events.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);

        // Hold the actual engine mutex on another thread so processBlock must
        // decide whether to consume deferred event capacity before engine-side
        // no-factory rejection can run.
        contend(p, events);
        require(p.deferredMidi_.active(), "contended processBlock creates deferred timeline");
        events.clear();
        p.processBlock(audio, events);
        require(p.engine_.activeMidiNotes_[60], hasFactoryVoices
                    ? "available CC32 flood fits with the following note"
                    : "unavailable CC32 flood cannot evict the following note");
        if (hasFactoryVoices)
            require(p.engine_.currentBank() == 6, "available deferred CC32 requests retain order");
        events.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        p.processBlock(audio, events);
        require(!p.engine_.hasHeldMidiNotes(), "note releases after deferred CC32 control");
    }
    static void checkDeferredPartitionCapacity(VDX7AudioProcessor& p)
    {
        constexpr int eventCount = 257;
        constexpr int blockSize = 64;
        constexpr int aggregateBlockSize = eventCount * blockSize;
        juce::AudioBuffer<float> shortAudio(2, blockSize);
        juce::AudioBuffer<float> aggregateAudio(2, aggregateBlockSize);
        juce::MidiBuffer events;

        // Both scenarios start with the same held voice and one skipped
        // 64-sample callback. Compare one large contended event batch with the
        // identical time-spaced CC stream delivered over successful blocks.
        p.prepareToPlay(48000, blockSize);
        events.addEvent(juce::MidiMessage::controllerEvent(1, 11, 25), 0);
        events.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
        p.processBlock(shortAudio, events);
        require(p.engine_.activeMidiNotes_[60], "partition test starts with a held note");
        const float baselineExpression = 25.0f / 127.0f;
        events.clear();
        contend(p, events, blockSize);
        require(p.deferredMidi_.active(), "initial skipped block establishes deferred timeline");

        events.clear();
        for (int i = 0; i < eventCount; ++i)
            events.addEvent(juce::MidiMessage::controllerEvent(1, 11, i % 128), i * blockSize);
        contend(p, events, aggregateBlockSize);
        require(p.deferredMidi_.active(), "oversized delayed batch enters bounded overflow state");
        events.clear();
        p.processBlock(aggregateAudio, events);
        require(!p.engine_.hasHeldMidiNotes(), "overflow recovery releases the pre-existing held note");
        require(!p.deferredMidi_.active(), "overflow panic is retired without replaying stale events");
        require(std::abs(p.engine_.midiExpression_ - baselineExpression) < 0.00001f,
                "oversized batch is dropped atomically rather than partly delivered");

        // Capacity overflow is recoverable; fresh input works normally.
        events.addEvent(juce::MidiMessage::controllerEvent(1, 11, 42), 0);
        events.addEvent(juce::MidiMessage::noteOn(1, 61, uint8_t(100)), 0);
        p.processBlock(shortAudio, events);
        require(p.engine_.activeMidiNotes_[61]
                    && std::abs(p.engine_.midiExpression_ - 42.0f / 127.0f) < 0.00001f,
                "normal MIDI recovers after deferred queue overflow");
        events.clear();
        events.addEvent(juce::MidiMessage::noteOff(1, 61), 0);
        p.processBlock(shortAudio, events);
        require(!p.engine_.hasHeldMidiNotes(), "post-overflow recovery note releases");

        // The same time-spaced CC sequence remains below capacity when each
        // successful callback can render the delayed timeline.
        p.prepareToPlay(48000, blockSize);
        events.clear();
        events.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
        p.processBlock(shortAudio, events);
        events.clear();
        contend(p, events, blockSize);
        require(p.deferredMidi_.active(), "partitioned control starts with same deferred lag");
        for (int i = 0; i < eventCount; ++i)
        {
            events.clear();
            events.addEvent(juce::MidiMessage::controllerEvent(1, 11, i % 128), 0);
            p.processBlock(shortAudio, events);
            require(std::abs(p.engine_.midiExpression_ - (i % 128) / 127.0f) < 0.00001f,
                    "each partitioned CC is delivered in order without panic");
        }
        events.clear();
        events.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        p.processBlock(shortAudio, events);
        require(!p.engine_.hasHeldMidiNotes(), "partitioned control releases held voice");
        require(!p.deferredMidi_.active(), "partitioned control drains without recovery panic");
    }
    static void checkSerialOverflow(VDX7Engine& e)
    {
        e.dx7_.midiSerialRx.flush();
        for (int i = 0; i < 8190; ++i) e.dx7_.midiSerialRx.write(0xf8);
        const auto revision = e.factoryBankLoadRevision();
        for (uint8_t value : {8, 15, 127})
        {
            const uint8_t bank[] {0xb0, 32, value};
            e.handleMidi(bank, 3);
        }
        require(!e.isMidiRecovering() && e.factoryBankLoadRevision() == revision,
                "unsupported banks are inert even with a full serial queue");
        const uint8_t note[] {0x90, 60, 100};
        e.handleMidi(note, 3);
        require(!e.hasHeldMidiNotes(), "serial overflow must reconcile note ownership");
        const int pending = (e.dx7_.midiSerialRx.writeIdx - e.dx7_.midiSerialRx.readIdx) & 8191;
        require(pending >= 384 && pending < 8191, "serial overflow must enqueue complete recovery messages");
        for (int pitch = 0; pitch < 128; ++pitch)
        {
            uint8_t status = 0, noteNumber = 0, velocity = 1;
            require(e.dx7_.midiSerialRx.read(status) && e.dx7_.midiSerialRx.read(noteNumber)
                    && e.dx7_.midiSerialRx.read(velocity), "complete recovery note-off");
            require((status & 0xf0) == 0x80 && noteNumber == pitch && velocity == 0,
                    "recovery releases every MIDI pitch");
        }
        e.handleMidi(note, 3);
        require(!e.hasHeldMidiNotes(), "same-timestamp events stay dropped during recovery");
        std::array<float, 256> left {}, right {};
        e.render(left.data(), right.data(), 256);
        require(!e.isMidiRecovering() && e.midiOverloadCount() == 1, "serial recovery completes");
        dx7Emu::Message ignored;
        while (e.toSynth_->pop(ignored)) {}
        e.processQueuedMessage({dx7Emu::Message::CtrlID::modulate, 7});
        for (int i = 0; i < 10000 && !e.dx7_.byte1Sent; ++i) e.dx7_.run();
        require(e.dx7_.byte1Sent && e.dx7_.haveMsg, "reach real in-flight controller handshake");
        for (int i = 0; i < 1024; ++i) e.toSynth_->analog(dx7Emu::Message::CtrlID::modulate, 127);
        const uint8_t pedal[] {0xb0, 64, 127};
        e.handleMidi(pedal, 3);
        require(e.isMidiRecovering() && e.midiOverloadCount() == 2 && !e.hasHeldMidiNotes(),
                "controller saturation must not lose pedal reconciliation");
        require((e.dx7_.P_CRT_PEDALS_LCD & 3) == 0, "recovery releases sustain and portamento");
        require(e.dx7_.byte1Sent && e.dx7_.haveMsg && e.dx7_.msg.byte2 == 7,
                "overload must preserve in-flight controller handshake");
        e.render(left.data(), right.data(), 1);
        std::vector<uint8_t> bank;
        require(e.saveRam(bank), "capture bank for live SysEx regression");
        bank.resize(4096);
        const auto sysex = VDX7Sysex::encode(bank);
        const int beforeBank = (e.dx7_.midiSerialRx.writeIdx - e.dx7_.midiSerialRx.readIdx) & 8191;
        std::vector<uint8_t> queuedBeforeBank;
        for (int i = 0; i < beforeBank; ++i)
            queuedBeforeBank.push_back(e.dx7_.midiSerialRx.buffer[(e.dx7_.midiSerialRx.readIdx + i) & 8191]);
        require(e.handleSysex(sysex.data(), sysex.size()), "live bank after recovery starts draining");
        const int afterBank = (e.dx7_.midiSerialRx.writeIdx - e.dx7_.midiSerialRx.readIdx) & 8191;
        // No explicit time request in this fixture: keep the original +2
        // program-only ordering. Check every old byte, not only queue length.
        require(beforeBank >= 384 && afterBank == beforeBank + 2,
                "live bank must preserve queued recovery note-offs");
        for (int i = 0; i < beforeBank; ++i)
            require(e.dx7_.midiSerialRx.buffer[(e.dx7_.midiSerialRx.readIdx + i) & 8191] == queuedBeforeBank[i],
                    "live bank must preserve every queued recovery byte");
        const std::array<uint8_t, 2> appended {
            static_cast<uint8_t>(0xc0 | (e.dx7_.getMidiRxChannel() & 15)),
            static_cast<uint8_t>(e.currentProgram()) };
        for (int i = 0; i < 2; ++i)
            require(e.dx7_.midiSerialRx.buffer[(e.dx7_.midiSerialRx.readIdx + beforeBank + i) & 8191] == appended[i],
                    "recovery must append only program without an explicit time request");
        e.resetMidiLifecycle();
        require(!e.isMidiRecovering(), "restart during recovery reconciles lifecycle");
        e.handleMidi(note, 3);
        require(e.hasHeldMidiNotes(), "fresh note accepted after recovery");
        e.allNotesOff();
    }
    static void checkLiveBankPreservesMasterTune(VDX7Engine& e)
    {
        std::vector<uint8_t> bank;
        require(e.saveRam(bank), "capture bank for tuning-preservation SysEx");
        bank.resize(4096);
        const auto sysex = VDX7Sysex::encode(bank);
        for (int tuning : {-256, -1, 0, 1, 255})
        {
            require(e.setMasterTune(tuning), "set tuning before live bank import");
            require(e.handleSysex(sysex.data(), sysex.size()), "import valid live bank");
            require(e.masterTune() == tuning, "live bank preserves master tuning");
        }
    }
    static void publishWithoutEditor(VDX7AudioProcessor& p) { p.timerCallback(); }
    static bool invalidRom(VDX7AudioProcessor& p)
    { return p.loadRomData(juce::File(), std::vector<uint8_t>(7), nullptr); }
    static void checkControllers(VDX7Engine& e)
    {
        for (int cc : {64, 65}) for (int v : {0, 1, 63, 64, 127})
        {
            uint8_t event[] {0xb0, static_cast<uint8_t>(cc), static_cast<uint8_t>(v)};
            e.handleMidi(event, 3);
            dx7Emu::Message m;
            while (e.toSynth_->pop(m)) e.processQueuedMessage(m);
            require(bool(e.dx7_.P_CRT_PEDALS_LCD & (cc == 64 ? 1 : 2)) == (v >= 64),
                    "CC switch threshold");
        }
        for (int volume : {1, 7}) for (int expression : {0, 64, 127})
        {
            uint8_t event[] {0xb0, 11, static_cast<uint8_t>(expression)};
            e.handleMidi(event, 3);
            e.dx7_.midiVolume = static_cast<uint8_t>(volume);
            float result = 0;
            for (int i = 0; i < 30000; ++i)
            {
                e.nativePos_ = 0; e.nativeCount_ = 1; e.nativeBlock_[0] = 1;
                result = e.nextNativeSample();
            }
            const float expected = e.dx7_.midiVolTab[volume] * expression / 127.0f;
            require(std::abs(result - expected) < 0.0001f, "Expression gain including silence");
        }
        e.resetAudioState();
    }
    static void contend(VDX7AudioProcessor& p, juce::MidiBuffer& events, int blockSize = 256)
    {
        std::promise<void> locked, release;
        auto untilRelease = release.get_future();
        auto worker = std::thread([&]
        {
            std::scoped_lock lock(p.engineMutex_);
            locked.set_value();
            untilRelease.wait();
        });
        locked.get_future().wait();
        juce::AudioBuffer<float> audio(2, blockSize);
        p.processBlock(audio, events);
        release.set_value();
        worker.join();
    }
    static bool noteActive(VDX7AudioProcessor& p, int note)
    { return p.engine_.activeMidiNotes_[note]; }
    static void checkProgramBytes(VDX7Engine& e)
    {
        for (int program : {0, 31, 32, 127})
        {
            e.dx7_.midiSerialRx.flush();
            const uint8_t message[] {0xc0, static_cast<uint8_t>(program)};
            e.handleMidi(message, 2);
            uint8_t status = 0, value = 0;
            require(e.dx7_.midiSerialRx.read(status) && e.dx7_.midiSerialRx.read(value), "firmware program bytes");
            require((status & 0xf0) == 0xc0 && value == std::min(program, 31)
                    && e.currentProgram() == value, "program metadata and firmware input agree");
        }
    }
};

static juce::MemoryBlock save(VDX7AudioProcessor& p)
{ juce::MemoryBlock s; p.getStateInformation(s); return s; }
static juce::ValueTree decode(const juce::MemoryBlock& s)
{
    auto xml = juce::AudioProcessor::getXmlFromBinary(s.getData(), static_cast<int>(s.getSize()));
    require(xml != nullptr, "state XML");
    return juce::ValueTree::fromXml(*xml);
}
static juce::MemoryBlock encode(const juce::ValueTree& s)
{
    juce::MemoryBlock result;
    auto xml = s.createXml();
    juce::AudioProcessor::copyXmlToBinary(*xml, result);
    return result;
}

static void checkEditOrdering(const juce::File& romFile)
{
    for (bool bankSwitch : {false, true})
        for (bool editFirst : {false, true})
            for (bool audioFlush : {false, true})
            for (bool operatorEdit : {false, true})
            {
                auto pStorage = std::make_unique<VDX7AudioProcessor>(false);
                auto& p = *pStorage;
                require(p.loadRomFromFile(romFile), "ordering ROM");
                p.selectProgramFromUi(3);
                const auto before = decode(save(p));
                juce::MemoryBlock originalRam;
                require(originalRam.fromBase64Encoding(before["ram"].toString()), "ordering initial RAM");
                auto* feedback = p.parameters().getParameter(operatorEdit
                    ? VDX7ParameterIDs::operatorParameter(5, VDX7VoiceData::Parameter::outputLevel)
                    : VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::feedback));
                const int editValue = (juce::roundToInt(feedback->convertFrom0to1(feedback->getValue())) + 1)
                    % (operatorEdit ? 100 : 8);
                auto patchExpected = [&](juce::MemoryBlock& ram, int program) {
                    auto* voice = static_cast<uint8_t*>(ram.getData()) + program*128;
                    if (operatorEdit)
                        VDX7VoiceData::setOperatorParameter(voice, 128, 5,
                            VDX7VoiceData::Parameter::outputLevel, editValue);
                    else VDX7VoiceData::setVoiceParameter(voice, 128,
                            VDX7VoiceData::VoiceParameter::feedback, editValue);
                };
                auto edit = [&] { feedback->setValueNotifyingHost(feedback->convertTo0to1(editValue)); };
                auto change = [&] {
                    if (bankSwitch) require(p.selectFactoryBank(1), "ordering bank selection");
                    else p.selectProgramFromUi(4);
                };
                if (editFirst) { edit(); change(); } else { change(); edit(); }
                if (audioFlush)
                {
                    juce::AudioBuffer<float> audio(2, 64);
                    juce::MidiBuffer midi;
                    p.processBlock(audio, midi);
                }
                juce::MemoryBlock actual;
                require(actual.fromBase64Encoding(decode(save(p))["ram"].toString()), "ordering final RAM");
                if (bankSwitch)
                {
                    auto referenceStorage = std::make_unique<VDX7AudioProcessor>(false);
                    auto& reference = *referenceStorage;
                    require(reference.loadRomFromFile(romFile), "reference ROM");
                    reference.selectProgramFromUi(3);
                    reference.selectFactoryBank(1);
                    juce::MemoryBlock expected;
                    require(expected.fromBase64Encoding(decode(save(reference))["ram"].toString()), "bank reference RAM");
                    // Factory-bank loading replaces the single internal RAM bank:
                    // an earlier edit must not leak into the newly loaded bank.
                    if (!editFirst)
                        patchExpected(expected, 3);
                    require(std::memcmp(actual.getData(), expected.getData(), 4096) == 0,
                            "bank switch/edit ordering preserves destination bank");
                }
                else
                {
                    patchExpected(originalRam, editFirst ? 3 : 4);
                    require(std::memcmp(actual.getData(), originalRam.getData(), 4096) == 0,
                            "program switch/edit ordering preserves correct voice");
                }
            }

    auto rapidStorage = std::make_unique<VDX7AudioProcessor>(false);
    auto& rapid = *rapidStorage;
    require(rapid.loadRomFromFile(romFile), "rapid selection ROM");
    juce::MemoryBlock expected;
    require(expected.fromBase64Encoding(decode(save(rapid))["ram"].toString()), "rapid original RAM");
    auto* parameter = rapid.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
        VDX7VoiceData::VoiceParameter::feedback));
    int value = juce::roundToInt(parameter->convertFrom0to1(parameter->getValue()));
    for (int program = 0; program < 32; ++program)
    {
        rapid.setCurrentProgram(program);
        value = (value + 1) % 8;
        parameter->setValueNotifyingHost(parameter->convertTo0to1(value));
        VDX7VoiceData::setVoiceParameter(static_cast<uint8_t*>(expected.getData()) + program*128,
            128, VDX7VoiceData::VoiceParameter::feedback, value);
    }
    juce::MemoryBlock actual;
    require(actual.fromBase64Encoding(decode(save(rapid))["ram"].toString()), "rapid final RAM");
    require(std::memcmp(expected.getData(), actual.getData(), 4096) == 0,
            "32 successive program/edit pairs are not coalesced or misdirected");
}

static void checkAudioPublication(const juce::File& romFile)
{
    auto pStorage = std::make_unique<VDX7AudioProcessor>(false);
    auto& p = *pStorage;
    require(p.loadRomFromFile(romFile), "publication ROM");
    struct Listener final : juce::AudioProcessorParameter::Listener
    {
        int calls = 0;
        VDX7AudioProcessor* reenter = nullptr;
        bool saved = false;
        bool coherent = true;
        juce::RangedAudioParameter* editOnNotify = nullptr;
        float editValue = 0;
        bool edited = false;
        void parameterValueChanged(int index, float) override
        {
            ++calls;
            if (editOnNotify != nullptr && !edited && index != editOnNotify->getParameterIndex())
            {
                edited = true;
                editOnNotify->setValueNotifyingHost(editValue);
            }
            if (reenter != nullptr)
            {
                juce::MemoryBlock state;
                reenter->getStateInformation(state);
                saved = state.getSize() > 0;
                const auto decoded = decode(state);
                juce::MemoryBlock ram;
                coherent &= ram.fromBase64Encoding(decoded["ram"].toString());
                if (ram.getSize() == VDX7Engine::kRamStateSize)
                {
                    const auto* voice = static_cast<const uint8_t*>(ram.getData())
                        + static_cast<int>(decoded["program"])*128;
                    for (int n = 0; n < VDX7VoiceData::kVoiceParameterCount; ++n)
                    {
                        const auto field = static_cast<VDX7VoiceData::VoiceParameter>(n);
                        const auto child = decoded.getChildWithName("PARAMETERS").getChildWithProperty(
                            "id", VDX7ParameterIDs::voiceParameter(field));
                        coherent &= static_cast<int>(child["value"]) ==
                            VDX7VoiceData::getVoiceParameter(voice, 128, field);
                    }
                }
                else coherent = false;
            }
        }
        void parameterGestureChanged(int, bool) override {}
    } listener;
    for (auto* parameter : p.getParameters()) parameter->addListener(&listener);
    juce::AudioBuffer<float> audio(2, 64);
    juce::MidiBuffer events;
    int audioCalls = 0;
    bool nonRealtimePublication = true;
    for (int kind = 0; kind < 3; ++kind)
    {
        listener.reenter = nullptr;
        events.clear();
        if (kind == 0) events.addEvent(juce::MidiMessage::programChange(1, 4), 0);
        else if (kind == 1) events.addEvent(juce::MidiMessage::controllerEvent(1, 32, 1), 0);
        else
        {
            juce::MemoryBlock ram;
            require(ram.fromBase64Encoding(decode(save(p))["ram"].toString()), "publication SysEx RAM");
            auto* bytes = static_cast<uint8_t*>(ram.getData());
            auto* voice = bytes + p.getCurrentProgram()*128;
            const auto field = VDX7VoiceData::VoiceParameter::algorithm;
            VDX7VoiceData::setVoiceParameter(voice, 128, field,
                (VDX7VoiceData::getVoiceParameter(voice, 128, field) + 1) % 32);
            auto sysex = VDX7Sysex::encode(std::vector<uint8_t>(bytes, bytes + 4096));
            events.addEvent(sysex.data(), static_cast<int>(sysex.size()), 0);
        }
        listener.calls = 0;
        listener.saved = false;
        p.processBlock(audio, events);
        audioCalls += listener.calls;
        listener.reenter = &p;
        if (kind == 1)
        {
            listener.editOnNotify = p.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
                VDX7VoiceData::VoiceParameter::algorithm));
            const int oldValue = juce::roundToInt(listener.editOnNotify->convertFrom0to1(
                listener.editOnNotify->getValue()));
            listener.editValue = listener.editOnNotify->convertTo0to1((oldValue + 1) % 32);
        }
        VDX7RegressionAccess::publishWithoutEditor(p);
        VDX7RegressionAccess::publishWithoutEditor(p); // Finish any reentrant edit's newer snapshot.
        nonRealtimePublication &= listener.calls > 0 && listener.saved;
        if (kind == 1)
            nonRealtimePublication &= listener.edited
                && listener.editOnNotify->getValue() == listener.editValue;
        listener.editOnNotify = nullptr;
    }
    for (auto* parameter : p.getParameters()) parameter->removeListener(&listener);
    require(audioCalls == 0, "audio callback must not publish host parameters");
    require(nonRealtimePublication, "editorless publisher permits reentrant host state request");
    require(listener.coherent, "reentrant saves contain voice parameters matching packed RAM");
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    if (argc != 2) { std::cout << "SKIP: supply your own ROM path\n"; return 77; }
    try
    {
        juce::File romFile(juce::String::fromUTF8(argv[1]));
        juce::MemoryBlock rom;
        require(romFile.loadFileAsData(rom), "read local ROM");
        checkAudioPublication(romFile);
        checkEditOrdering(romFile);
        {
            auto inputStorage = std::make_unique<VDX7AudioProcessor>(false);
            auto& input = *inputStorage;
            require(input.loadRomFromFile(romFile), "MIDI validation ROM");
            input.prepareToPlay(48000, 64);
            VDX7RegressionAccess::checkInvalidMidi(input);
            VDX7RegressionAccess::checkSupportedNoteRange(input);
            VDX7RegressionAccess::checkInputChannel(input);
            VDX7RegressionAccess::checkStateRestoreDropsDeferredMidi(input);
            VDX7RegressionAccess::checkFactoryCc32DeferredCapacity(input, true);
            VDX7RegressionAccess::checkDeferredPartitionCapacity(input);
        }
        auto engineStorage = std::make_unique<VDX7Engine>();
        auto& engine = *engineStorage;
        require(engine.loadRomImage(static_cast<const uint8_t*>(rom.getData()), rom.getSize()), "engine ROM");
        VDX7RegressionAccess::checkControllers(engine);
        VDX7RegressionAccess::checkProgramBytes(engine);
        VDX7RegressionAccess::checkLiveBankPreservesMasterTune(engine);
        VDX7RegressionAccess::checkSerialOverflow(engine);
        std::vector<uint8_t> before, after;
        engine.saveRam(before);
        const uint8_t invalid[7] {};
        require(!engine.loadRomImage(invalid, sizeof(invalid)), "reject bad ROM");
        require(engine.isLoaded() && engine.hasFactoryVoices(), "bad ROM preserves running engine");
        engine.saveRam(after);
        require(before == after, "bad ROM preserves RAM");
        require(engine.loadRomImage(static_cast<const uint8_t*>(rom.getData()), 16384), "firmware-only reload");
        require(!engine.hasFactoryVoices() && engine.currentBank() == -1, "no stale factory bank");

        // Temporary local firmware is removed by the directory guard,
        // including when the assertion throws. Never part of an artifact.
        const auto temporaryDirectory = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getNonexistentChildFile("vdx7-companion-regression", "", false);
        require(temporaryDirectory.createDirectory().wasOk(), "create private test directory");
        struct RemoveDirectory
        {
            juce::File directory;
            ~RemoveDirectory() { directory.deleteRecursively(); }
        } cleanup { temporaryDirectory };
        const auto firmwareFile = temporaryDirectory.getChildFile("firmware.bin");
        require(firmwareFile.replaceWithData(rom.getData(), 16384), "write temporary firmware");
        require(temporaryDirectory.getChildFile("dx7_factory_voices_32KB.bin")
                    .replaceWithData(invalid, sizeof(invalid)), "write invalid companion");
        auto companionStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& companion = *companionStorage;
        require(companion.loadRomFromFile(firmwareFile), "invalid optional companion permits firmware load");
        require(companion.isRomLoaded() && !companion.hasFactoryVoices(), "firmware-only companion fallback");
        require(companion.getStatusText().containsIgnoreCase("ignored"), "companion warning status");
        const auto companionFile = temporaryDirectory.getChildFile("dx7_factory_voices_32KB.bin");
        require(companionFile.replaceWithData(static_cast<const uint8_t*>(rom.getData()) + 16384,
                                             32768), "write valid temporary companion");
        require(companion.loadRomFromFile(firmwareFile) && companion.hasFactoryVoices(),
                "valid companion loads factory banks");
        require(companionFile.deleteFile(), "remove temporary companion");
        require(companion.loadRomFromFile(firmwareFile) && !companion.hasFactoryVoices(),
                "absent companion loads firmware only");
        VDX7RegressionAccess::checkFactoryCc32DeferredCapacity(companion, false);

        auto originalStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& original = *originalStorage;
        require(original.loadRomFromFile(romFile), "processor ROM");
        original.selectProgramFromUi(5);
        save(original);
        require(original.renameVoice("RESTORE1"), "edit name");
        auto* feedback = original.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
            VDX7VoiceData::VoiceParameter::feedback));
        feedback->setValueNotifyingHost(feedback->convertTo0to1(6));
        require(!VDX7RegressionAccess::invalidRom(original), "processor rejects invalid ROM");
        require(original.parameters().getRawParameterValue(VDX7ParameterIDs::voiceParameter(
            VDX7VoiceData::VoiceParameter::feedback))->load() == 6, "invalid ROM preserves pending UI edit");
        auto saved = decode(save(original));
        saved.setProperty("romPath", "/vdx7-regression-missing/firmware.bin", nullptr);
        auto missing = encode(saved);
        auto waitingStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& waiting = *waitingStorage;
        waiting.setStateInformation(missing.getData(), static_cast<int>(missing.getSize()));
        require(!waiting.isRomLoaded(), "missing ROM remains unloaded");
        require(decode(save(waiting)).isEquivalentTo(saved), "save preserves pending state");
        auto* master = waiting.parameters().getParameter("masterVolume");
        require(master != nullptr, "master parameter exists");
        master->setValueNotifyingHost(0.37f);
        const float editedMasterValue = master->getValue();
        const auto editedPending = decode(save(waiting));
        require(!editedPending.isEquivalentTo(saved), "missing-ROM re-save includes new host parameter");
        require(editedPending["ram"] == saved["ram"]
                && editedPending["bank"] == saved["bank"]
                && editedPending["program"] == saved["program"], "missing-ROM edit preserves sound data");
        auto restartedStorage = std::make_unique<VDX7AudioProcessor>(false);
        auto& restarted = *restartedStorage;
        auto pending = save(waiting);
        restarted.setStateInformation(pending.getData(), static_cast<int>(pending.getSize()));
        require(std::abs(restarted.parameters().getParameter("masterVolume")->getValue() - editedMasterValue)
                < 0.00001f, "missing-ROM master edit survives second restore");
        require(restarted.loadRomFromFile(romFile), "manual ROM resumes restore");
        require(std::abs(restarted.parameters().getParameter("masterVolume")->getValue() - editedMasterValue)
                < 0.00001f, "master edit survives firmware load");
        auto restored = decode(save(restarted));
        require(restored["ram"] == saved["ram"] && restored["program"] == saved["program"]
                && restored["bank"] == saved["bank"], "exact deferred RAM/bank/program restore");
        require(restarted.getCurrentPatchName() == "RESTORE1", "restored name");
        for (bool saveBeforeLoad : {false, true})
        {
            auto editedStorage = std::make_unique<VDX7AudioProcessor>(false);
            auto& edited = *editedStorage;
            edited.setStateInformation(missing.getData(), static_cast<int>(missing.getSize()));
            auto* editedFeedback = edited.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
                VDX7VoiceData::VoiceParameter::feedback));
            editedFeedback->setValueNotifyingHost(editedFeedback->convertTo0to1(2));
            auto* editedOperator = edited.parameters().getParameter(VDX7ParameterIDs::operatorParameter(
                5, VDX7VoiceData::Parameter::outputLevel));
            editedOperator->setValueNotifyingHost(editedOperator->convertTo0to1(42));
            if (saveBeforeLoad)
            {
                const auto checkpoint = save(edited);
                edited.setStateInformation(checkpoint.getData(), static_cast<int>(checkpoint.getSize()));
            }
            require(edited.loadRomFromFile(romFile), "load after offline voice edit");
            require(editedFeedback->getValue() == editedFeedback->convertTo0to1(2),
                    "explicit offline voice edit overrides saved RAM");
            require(editedOperator->getValue() == editedOperator->convertTo0to1(42),
                    "offline operator edit in upper dirty mask survives restore");
            require(edited.getCurrentPatchName() == "RESTORE1", "offline edit preserves remaining voice data");
        }
        auto malformed = saved.createCopy();
        malformed.setProperty("ram", "invalid-base64", nullptr);
        auto malformedBytes = encode(malformed);
        restarted.setStateInformation(malformedBytes.getData(), static_cast<int>(malformedBytes.getSize()));
        require(decode(save(restarted))["ram"] == saved["ram"], "malformed state preserves sound");
        for (auto* parameter : original.getParameters())
            if (parameter != original.parameters().getParameter("masterVolume"))
                require(std::abs(parameter->getValue() - restarted.getParameters()[parameter->getParameterIndex()]->getValue())
                        < 0.00001f, "restored host parameter");

        restarted.prepareToPlay(48000, 256);
        juce::AudioBuffer<float> audio(2, 256);
        juce::MidiBuffer events;
        events.addEvent(juce::MidiMessage::noteOn(1, 59, uint8_t(100)), 0);
        events.addEvent(juce::MidiMessage::noteOff(1, 59), 192);
        VDX7RegressionAccess::contend(restarted, events);
        juce::AudioBuffer<float> shortAudio(2, 64);
        events.clear();
        restarted.processBlock(shortAudio, events);
        require(VDX7RegressionAccess::noteActive(restarted, 59), "deferred note duration must not collapse");
        for (int i = 0; i < 3; ++i) restarted.processBlock(shortAudio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 59), "deferred note-off retains its later position");
        events.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
        restarted.processBlock(audio, events);
        require(VDX7RegressionAccess::noteActive(restarted, 60), "note starts");
        events.clear();
        events.addEvent(juce::MidiMessage::noteOff(1, 60), 0);
        events.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 1);
        VDX7RegressionAccess::contend(restarted, events);
        require(VDX7RegressionAccess::noteActive(restarted, 60), "busy engine defers note-off");
        events.clear(); restarted.processBlock(audio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 60), "deferred note-off delivered");

        events.addEvent(juce::MidiMessage::noteOn(1, 61, uint8_t(100)), 0);
        restarted.processBlock(audio, events);
        events.clear();
        for (int i = 0; i < 257; ++i)
            events.addEvent(juce::MidiMessage::noteOn(1, 62, uint8_t(100)), 0);
        VDX7RegressionAccess::contend(restarted, events);
        events.clear(); restarted.processBlock(audio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 61)
                && !VDX7RegressionAccess::noteActive(restarted, 62), "overflow releases without replaying stale ons");

        events.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
        restarted.processBlock(audio, events);
        events.addEvent(juce::MidiMessage::noteOn(1, 61, uint8_t(100)), 64);
        VDX7RegressionAccess::contend(restarted, events);
        const auto beforeRestart = decode(save(restarted));
        restarted.releaseResources();
        restarted.prepareToPlay(96000, 64);
        require(!VDX7RegressionAccess::noteActive(restarted, 60), "device restart clears old note ownership");
        events.clear();
        for (int i = 0; i < 16; ++i) restarted.processBlock(shortAudio, events);
        require(!VDX7RegressionAccess::noteActive(restarted, 61), "device restart discards deferred note-on");
        require(shortAudio.getMagnitude(0, 64) < 0.00001f, "device restart silences previous voices");
        const auto afterRestart = decode(save(restarted));
        juce::MemoryBlock ramBeforeRestart, ramAfterRestart;
        require(ramBeforeRestart.fromBase64Encoding(beforeRestart["ram"].toString())
                && ramAfterRestart.fromBase64Encoding(afterRestart["ram"].toString()), "restart RAM decode");
        require(std::memcmp(ramBeforeRestart.getData(), ramAfterRestart.getData(), 4096) == 0
                && beforeRestart["bank"] == afterRestart["bank"]
                && beforeRestart["program"] == afterRestart["program"], "restart preserves bank/program/voices");

        juce::MemoryBlock ram;
        require(ram.fromBase64Encoding(restored["ram"].toString()), "decode RAM for bank");
        const auto* start = static_cast<const uint8_t*>(ram.getData());
        std::vector<uint8_t> packed(start, start + 4096);
        packed[5 * 128 + 118] = 'L';
        VDX7VoiceData::setVoiceParameter(packed.data()+5*128, 128,
            VDX7VoiceData::VoiceParameter::algorithm, 31);
        auto sysex = VDX7Sysex::encode(packed);
        require(sysex.size() == 4104, "encode bank");
        events.addEvent(sysex.data(), static_cast<int>(sysex.size()), 0);
        restarted.processBlock(audio, events);
        require(restarted.getCurrentBank() == -1 && restarted.hasUnexportedEdits(), "live bank metadata");
        require(restarted.getCurrentPatchName().startsWith("L"), "live bank name refreshed");
        VDX7RegressionAccess::publishWithoutEditor(restarted);
        require(restarted.parameters().getRawParameterValue(VDX7ParameterIDs::voiceParameter(
            VDX7VoiceData::VoiceParameter::algorithm))->load() == 31, "live bank host parameter refreshed");
        sysex[6] |= 0x80;
        events.clear(); events.addEvent(sysex.data(), static_cast<int>(sysex.size()), 0);
        restarted.processBlock(audio, events);
        require(restarted.getCurrentPatchName().startsWith("L"), "invalid live bank ignored");
        const auto checkUnsupportedBanks = [&] {
            const int bank = restarted.getCurrentBank();
            const int program = restarted.getCurrentProgram();
            const bool dirty = restarted.hasUnexportedEdits();
            juce::MemoryBlock before;
            require(before.fromBase64Encoding(decode(save(restarted))["ram"].toString()), "before unsupported bank");
            for (int value : {8, 15, 127})
            {
                events.clear();
                events.addEvent(juce::MidiMessage::controllerEvent(1, 32, value), 0);
                restarted.processBlock(audio, events);
                juce::MemoryBlock after;
                require(after.fromBase64Encoding(decode(save(restarted))["ram"].toString()), "after unsupported bank");
                require(restarted.getCurrentBank() == bank && restarted.getCurrentProgram() == program,
                        "unsupported CC32 preserves bank and program");
                require(restarted.hasUnexportedEdits() == dirty, "unsupported CC32 preserves dirty state");
                require(std::memcmp(before.getData(), after.getData(), 4096) == 0,
                        "unsupported CC32 preserves working voices");
            }
        };
        checkUnsupportedBanks(); // Dirty CUSTOM working bank from live SysEx.
        for (int bank : {0, 7, 7}) // Same-bank reload must count as a real load.
        {
            auto* feedback = restarted.parameters().getParameter(VDX7ParameterIDs::voiceParameter(
                VDX7VoiceData::VoiceParameter::feedback));
            const int value = (juce::roundToInt(feedback->convertFrom0to1(feedback->getValue())) + 1) % 8;
            feedback->setValueNotifyingHost(feedback->convertTo0to1(value));
            save(restarted); // Flush the ordered edit before bank selection.
            require(restarted.hasUnexportedEdits(), "bank regression starts with edits");
            checkUnsupportedBanks();
            events.clear();
            events.addEvent(juce::MidiMessage::controllerEvent(1, 0, 0), 0);
            events.addEvent(juce::MidiMessage::controllerEvent(1, 32, bank), 0);
            events.addEvent(juce::MidiMessage::programChange(1, 5), 0);
            restarted.processBlock(audio, events);
            require(restarted.getCurrentBank() == bank && restarted.getCurrentProgram() == 5,
                    "CC0 CC32 program sequence");
            require(!restarted.hasUnexportedEdits(), "successful bank reload clears dirty state");
            juce::MemoryBlock loaded;
            require(loaded.fromBase64Encoding(decode(save(restarted))["ram"].toString()), "loaded bank RAM");
            require(std::memcmp(loaded.getData(), static_cast<const uint8_t*>(rom.getData())
                    + 16384 + bank*4096, 4096) == 0, "boundary bank bytes match factory");
        }
        events.clear();
        events.addEvent(juce::MidiMessage::controllerEvent(16, 32, 2), 0);
        restarted.processBlock(audio, events);
        require(restarted.getCurrentBank() == 2, "CC32 bank selection");
        juce::MemoryBlock changedRam;
        require(changedRam.fromBase64Encoding(decode(save(restarted))["ram"].toString()), "bank RAM");
        require(std::memcmp(changedRam.getData(), static_cast<const uint8_t*>(rom.getData())
                + 16384 + 2*4096, 4096) == 0, "CC32 and editor use the same internal bank");
        std::cout << "PASS: pedal thresholds, expression, invalid ROM, deferred restore, contention and live bank state\n";
        return 0;
    }
    catch (const std::exception& e) { std::cerr << "FAIL: " << e.what() << '\n'; return 1; }
}
