#include "PluginProcessor.h"
#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

struct TimelineEvent
{
    std::array<uint8_t, 3> bytes {};
    int sample = 0;
    bool operator==(const TimelineEvent&) const = default;
};

static std::vector<TimelineEvent> inspectTimeline(const VDX7DeferredMidi& source)
{
    // Inspect detached audio-owned storage without delivering anything to the
    // firmware or depending on its asynchronous serial-consumption timing.
    auto timeline = std::make_unique<VDX7DeferredMidi>(source);
    std::vector<TimelineEvent> events;
    timeline->renderBlock(256, [&](const uint8_t* data, std::size_t size, int sample)
    {
        require(size == 3, "timeline fixture uses complete three-byte messages");
        events.push_back({{data[0], data[1], data[2]}, sample});
    }, [] { require(false, "ordered post-CC120 input must not become a queue panic"); });
    return events;
}

static void checkTimelineWithoutRom()
{
    const uint8_t on[] {0x90, 62, 100}, off[] {0x80, 62, 0};
    const uint8_t panic[] {0xb0, 120, 0}, expression[] {0xb0, 11, 64};
    VDX7DeferredMidi direct, replayed;
    require(direct.push(on, sizeof(on), 24), "queue direct reset follower");
    direct.pauseDirectBlock(64, 8, 4096);
    require(replayed.push(panic, sizeof(panic), 8)
            && replayed.push(on, sizeof(on), 24), "queue replayed reset and follower");
    replayed.advanceInputBlock(64, 4096, VDX7DeferredMidi::Playback::rendering);
    replayed.renderBlock(64, [](const uint8_t* bytes, std::size_t, int sample) -> bool
    {
        require(bytes[1] == 120 && sample == 8, "replayed reset reaches the same sample");
        return false;
    }, [] { require(false, "replayed reset cannot panic an ordered queue"); });
    for (auto* timeline : {&direct, &replayed})
    {
        require(timeline->push(expression, sizeof(expression), 0)
                && timeline->push(off, sizeof(off), 40), "next block follows the reset tail");
        timeline->advanceInputBlock(64, 4096, VDX7DeferredMidi::Playback::paused);
    }
    const std::vector<TimelineEvent> expected {
        {{0x90, 62, 100}, 16}, {{0xb0, 11, 64}, 56}, {{0x80, 62, 0}, 96}};
    require(inspectTimeline(direct) == expected && inspectTimeline(replayed) == expected,
            "direct and deferred resets preserve identical anchors and future input");

    VDX7DeferredMidi empty;
    empty.pauseDirectBlock(64, 8, 56);
    require(empty.active() && empty.push(on, sizeof(on), 12),
            "empty reset tail retains time without consuming event capacity");
    require(inspectTimeline(empty) == std::vector<TimelineEvent> {{{0x90, 62, 100}, 68}},
            "future note uses the completed direct callback's input start");

    VDX7DeferredMidi bounded;
    for (int i = 0; i < 256; ++i)
        require(bounded.push(on, sizeof(on), 24), "all reset followers fit the fixed capacity");
    bounded.pauseDirectBlock(64, 8, 4096);
    require(!bounded.push(off, sizeof(off)), "actual capacity overflow still requests panic");
    bounded.pauseDirectBlock(64, 8, 4096);
    int panics = 0;
    bounded.renderBlock(64, [](const uint8_t*, std::size_t, int)
    {
        require(false, "capacity panic must discard all queued input");
    }, [&] { ++panics; });
    require(panics == 1 && !bounded.active(), "clock anchoring cannot erase overflow panic");
    std::cout << "PASS: ROM-free reset-anchor equivalence, future input and fixed capacity\n";
}

struct VDX7RegressionAccess
{
    static std::unique_ptr<VDX7AudioProcessor> create(const juce::File& rom)
    {
        auto processor = std::make_unique<VDX7AudioProcessor>(false);
        require(processor->loadRomFromFile(rom), "load local timeline fixture ROM");
        processor->prepareToPlay(48000.0, 64);
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        for (int block = 0; block < 256; ++block) processor->processBlock(audio, midi);
        midi.addEvent(juce::MidiMessage::noteOn(1, 60, juce::uint8(100)), 0);
        processor->processBlock(audio, midi);
        require(processor->engine_.activeMidiNotes_[60] != 0,
                "timeline fixture starts with a held note");
        return processor;
    }

    static void startReset(VDX7AudioProcessor& processor, bool followingNote = true,
                           bool secondReset = false)
    {
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 120, 0), 8);
        if (followingNote)
            midi.addEvent(juce::MidiMessage::noteOn(1, 62, juce::uint8(100)), 24);
        if (secondReset)
        {
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 120, 0), 40);
            midi.addEvent(juce::MidiMessage::noteOn(1, 64, juce::uint8(100)), 48);
        }
        processor.processBlock(audio, midi);
        require(processor.engine_.isHostResetInProgress(), "CC120 begins firmware reset");
    }

    static void finishReset(VDX7AudioProcessor& processor)
    {
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        int blocks = 0;
        while (processor.engine_.isHostResetInProgress() && blocks++ < 1500)
            processor.processBlock(audio, midi);
        require(!processor.engine_.isHostResetInProgress(), "bounded firmware reset completes");
    }

    static void checkNextBlockInput(const juce::File& rom)
    {
        auto processor = create(rom);
        startReset(*processor);
        require(inspectTimeline(processor->deferredMidi_) == std::vector<TimelineEvent> {
                    {{0x90, 62, 100}, 16}},
                "direct CC120 anchors deferred playback at sample 8, not block start");

        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 11, 64), 0);
        midi.addEvent(juce::MidiMessage::noteOn(1, 64, juce::uint8(100)), 12);
        midi.addEvent(juce::MidiMessage::noteOff(1, 62), 40);
        processor->processBlock(audio, midi);
        require(inspectTimeline(processor->deferredMidi_) == std::vector<TimelineEvent> {
                    {{0x90, 62, 100}, 16}, {{0xb0, 11, 64}, 56},
                    {{0x90, 64, 100}, 68}, {{0x80, 62, 0}, 96}},
                "next-block CC, Note On and Note Off retain absolute sample order");
        finishReset(*processor);
        for (int block = 0; block < 4; ++block) processor->processBlock(audio, midi);
        require(processor->engine_.activeMidiNotes_[62] == 0
                && processor->engine_.activeMidiNotes_[64] != 0
                && std::abs(processor->engine_.midiExpression_ - 64.0f / 127.0f) < 1.0e-6f,
                "post-reset notes and expression are delivered without silent eviction");

        // A release arriving after playback resumes must join the same delayed
        // input timeline, not disappear between an empty queue and a held note.
        midi.addEvent(juce::MidiMessage::noteOff(1, 64), 20);
        processor->processBlock(audio, midi);
        for (int block = 0; block < 1500 && processor->engine_.hasHeldMidiNotes(); ++block)
            processor->processBlock(audio, midi);
        require(!processor->engine_.hasHeldMidiNotes()
                && !processor->deferredMidi_.active(),
                "later Note Off releases the held voice and clears its completed timeline");
        std::cout << "PASS: direct CC120 and normal next-block MIDI retain ordering and ownership\n";
    }

    static void checkEmptyResetTail(const juce::File& rom)
    {
        auto processor = create(rom);
        startReset(*processor, false);
        require(processor->deferredMidi_.active(),
                "an empty reset tail retains the paused callback's time anchor");
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::noteOn(1, 64, juce::uint8(100)), 12);
        processor->processBlock(audio, midi);
        require(inspectTimeline(processor->deferredMidi_) == std::vector<TimelineEvent> {
                    {{0x90, 64, 100}, 68}},
                "first next-block note follows the empty CC120 reset tail at its original time");
        finishReset(*processor);
        for (int block = 0; block < 4; ++block) processor->processBlock(audio, midi);
        require(processor->engine_.activeMidiNotes_[64] != 0,
                "first note after an empty reset tail survives replay");

        auto empty = create(rom);
        startReset(*empty, false);
        finishReset(*empty);
        empty->processBlock(audio, midi);
        require(!empty->deferredMidi_.active(),
                "a completed reset with no notes discards obsolete timeline delay");
        std::cout << "PASS: empty CC120 tails preserve the anchor only while reset needs it\n";
    }

    static void checkPartitionEquivalence(const juce::File& rom)
    {
        auto split = create(rom);
        auto aggregate = create(rom);
        startReset(*split);
        juce::AudioBuffer<float> splitAudio(2, 64), aggregateAudio(2, 128);
        juce::MidiBuffer splitMidi, aggregateMidi;
        splitMidi.addEvent(juce::MidiMessage::controllerEvent(1, 11, 64), 0);
        splitMidi.addEvent(juce::MidiMessage::noteOn(1, 64, juce::uint8(100)), 12);
        splitMidi.addEvent(juce::MidiMessage::noteOff(1, 62), 40);
        split->processBlock(splitAudio, splitMidi);
        aggregateMidi.addEvent(juce::MidiMessage::controllerEvent(1, 120, 0), 8);
        aggregateMidi.addEvent(juce::MidiMessage::noteOn(1, 62, juce::uint8(100)), 24);
        aggregateMidi.addEvent(juce::MidiMessage::controllerEvent(1, 11, 64), 64);
        aggregateMidi.addEvent(juce::MidiMessage::noteOn(1, 64, juce::uint8(100)), 76);
        aggregateMidi.addEvent(juce::MidiMessage::noteOff(1, 62), 104);
        aggregate->processBlock(aggregateAudio, aggregateMidi);
        const std::vector<TimelineEvent> expected {
            {{0x90, 62, 100}, 16}, {{0xb0, 11, 64}, 56},
            {{0x90, 64, 100}, 68}, {{0x80, 62, 0}, 96}};
        require(inspectTimeline(split->deferredMidi_) == expected
                && inspectTimeline(aggregate->deferredMidi_) == expected,
                "64+64 and 128 sample callbacks retain identical post-CC120 MIDI timelines");
        std::cout << "PASS: split and aggregate callbacks preserve the same CC120 timeline\n";
    }

    static void checkRepeatedReset(const juce::File& rom)
    {
        auto processor = create(rom);
        startReset(*processor, true, true);
        require(inspectTimeline(processor->deferredMidi_) == std::vector<TimelineEvent> {
                    {{0x90, 62, 100}, 16}, {{0xb0, 120, 0}, 32}, {{0x90, 64, 100}, 40}},
                "a second CC120 keeps its position behind the first reset");
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 11, 64), 0);
        processor->processBlock(audio, midi);
        finishReset(*processor);
        processor->processBlock(audio, midi);
        require(processor->engine_.isHostResetInProgress(),
                "replayed second CC120 begins its own ordered reset");
        require(inspectTimeline(processor->deferredMidi_) == std::vector<TimelineEvent> {
                    {{0x90, 64, 100}, 8}, {{0xb0, 11, 64}, 24}},
                "second CC120 reanchors playback without disturbing later events");
        finishReset(*processor);
        processor->processBlock(audio, midi);
        require(processor->engine_.activeMidiNotes_[62] == 0
                && processor->engine_.activeMidiNotes_[64] != 0
                && std::abs(processor->engine_.midiExpression_ - 64.0f / 127.0f) < 1.0e-6f,
                "ordered repeated reset retires the intermediate note and delivers its followers");
        std::cout << "PASS: repeated CC120 resets reanchor deferred playback in order\n";
    }

    static int ownership(const VDX7Engine& engine, int note, int flag)
    {
        int count = 0;
        for (int voice = 0; voice < 16; ++voice)
            count += engine.dx7_.memory[0x20b0 + 2 * voice] == note
                && (engine.dx7_.memory[0x20b1 + 2 * voice] & flag) != 0;
        return count;
    }

    static void checkControllerResetNoteOrder(const juce::File& rom, bool sustainBeforeRelease)
    {
        auto processor = create(rom);
        require(processor->engine_.releaseRetirementProfile_,
                "controller/note ownership assertions require verified v1.8 firmware");
        juce::AudioBuffer<float> audio(2, 64);
        juce::MidiBuffer midi;
        for (int block = 0; block < 128; ++block) processor->processBlock(audio, midi);
        require(ownership(processor->engine_, 60, 2) == 1
                && (processor->engine_.dx7_.P_CRT_PEDALS_LCD & 1) == 0,
                "controller/note fixture has a physically held note with sustain OFF");
        const auto overloads = processor->engine_.midiOverloadCount();
        // Earlier accepted analog input makes CC121 a multi-callback operation
        // without filling a queue or triggering a capacity-recovery panic.
        const uint8_t breath[] {0xb0, 2, 127};
        for (int event = 0; event < 100; ++event) processor->engine_.handleMidi(breath, 3);
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 121, 0), 8);
        if (sustainBeforeRelease)
        {
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 16);
            midi.addEvent(juce::MidiMessage::noteOn(1, 72, juce::uint8(100)), 24);
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), 40);
            midi.addEvent(juce::MidiMessage::noteOff(1, 72), 48);
        }
        else
            midi.addEvent(juce::MidiMessage::noteOff(1, 60), 16);
        processor->processBlock(audio, midi);
        require(processor->engine_.controllerResetActive_, "earlier controller FIFO keeps CC121 in progress");
        const auto retained = inspectTimeline(processor->deferredMidi_);
        const std::vector<TimelineEvent> expected = sustainBeforeRelease
            ? std::vector<TimelineEvent> {{{0xb0, 64, 127}, 8}, {{0x90, 72, 100}, 16},
                                         {{0x80, 60, 0}, 32}, {{0x80, 72, 0}, 40}}
            : std::vector<TimelineEvent> {{{0x80, 60, 0}, 8}};
        int blocks = 0;
        while (processor->engine_.controllerResetActive_ && blocks++ < 1500)
            processor->processBlock(audio, midi);
        require(!processor->engine_.controllerResetActive_, "CC121 controller acknowledgment completes");
        for (int block = 0; block < 256; ++block) processor->processBlock(audio, midi);
        if (sustainBeforeRelease)
        {
            // Check the actual firmware effect before the detached timeline
            // assertion, so an old Processor reports the audible order defect.
            require(ownership(processor->engine_, 60, 1) == 1
                    && ownership(processor->engine_, 60, 2) == 0,
                    "CC121 then sustain ON precedes the held-note release");
            require(ownership(processor->engine_, 72, 1) == 1
                    && ownership(processor->engine_, 72, 2) == 0,
                    "fresh Note On/Off after CC121 and sustain ON remains sustained");
        }
        else
        {
            require(ownership(processor->engine_, 60, 1) == 0
                    && ownership(processor->engine_, 60, 2) == 0,
                    "release before a later pedal ON is consumed while sustain is OFF");
            // Cross a real render/firmware-dispatch boundary before the later
            // input. An immediate raw burst could mask opposite-order bugs via
            // the independently paced serial and physical-pedal transports.
            midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 127), 16);
            processor->processBlock(audio, midi);
            for (int block = 0; block < 256; ++block) processor->processBlock(audio, midi);
            require((processor->engine_.dx7_.P_CRT_PEDALS_LCD & 1) != 0
                    && ownership(processor->engine_, 60, 1) == 0
                    && ownership(processor->engine_, 60, 2) == 0,
                    "later sustain ON cannot resurrect an earlier released note");
        }
        require(retained == expected, "CC121 retains controller/note followers in their original sample order");
        require(processor->engine_.midiOverloadCount() == overloads,
                "controller reset ordering needs no overflow recovery");
        midi.addEvent(juce::MidiMessage::controllerEvent(1, 64, 0), 0);
        processor->processBlock(audio, midi);
        for (int block = 0; block < 256; ++block) processor->processBlock(audio, midi);
        require(ownership(processor->engine_, 60, 1) == 0
                && ownership(processor->engine_, 72, 1) == 0,
                "later pedal OFF releases all post-reset sustained ownership");
        std::cout << "PASS: CC121 controller/note timeline; sustain before release="
                  << sustainBeforeRelease << '\n';
    }
};

int main(int argc, char** argv)
{
    try
    {
        if (argc != 2) return 2;
        if (juce::String(argv[1]) == "--timeline-only")
        {
            checkTimelineWithoutRom();
            return 0;
        }
        juce::ScopedJuceInitialiser_GUI initialiser;
        const juce::File rom(argv[1]);
        VDX7RegressionAccess::checkNextBlockInput(rom);
        VDX7RegressionAccess::checkEmptyResetTail(rom);
        VDX7RegressionAccess::checkPartitionEquivalence(rom);
        VDX7RegressionAccess::checkRepeatedReset(rom);
        VDX7RegressionAccess::checkControllerResetNoteOrder(rom, true);
        VDX7RegressionAccess::checkControllerResetNoteOrder(rom, false);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
