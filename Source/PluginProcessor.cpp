#include "PluginProcessor.h"
#include "VDX7Sysex.h"
#include "VDX7MidiValidation.h"
#include "PluginEditor.h"

#include <cstring>
#include <optional>

namespace
{
constexpr const char* kStateType = "VDX7STATE";
constexpr const char* kParameterStateType = "PARAMETERS";
// Suppress only callbacks caused by this thread's patch publication. Host
// automation arriving on another thread must still enter the edit mailbox.
thread_local const VDX7AudioProcessor* publishingVoice = nullptr;
thread_local const juce::String* publishingParameterID = nullptr;
thread_local float publishingParameterValue = 0;

constexpr std::array<const char*, VDX7VoiceData::kParameterCount> kOperatorParameterSuffixes
{
    "Rate1", "Rate2", "Rate3", "Rate4",
    "Level1", "Level2", "Level3", "Level4",
    "OutputLevel", "Coarse", "Fine", "Detune",
    "RateScaling", "VelocitySensitivity", "AmplitudeModSensitivity",
    "OscillatorMode", "Breakpoint", "LeftScaleDepth", "RightScaleDepth",
    "LeftScaleCurve", "RightScaleCurve"
};

constexpr std::array<const char*, VDX7VoiceData::kParameterCount> kOperatorParameterNames
{
    "EG Rate 1", "EG Rate 2", "EG Rate 3", "EG Rate 4",
    "EG Level 1", "EG Level 2", "EG Level 3", "EG Level 4",
    "Output Level", "Coarse", "Fine", "Detune",
    "Rate Scaling", "Velocity Sensitivity", "Amplitude Mod Sensitivity",
    "Oscillator Mode", "Keyboard Breakpoint", "Left Scale Depth", "Right Scale Depth",
    "Left Scale Curve", "Right Scale Curve"
};

constexpr std::array<const char*, VDX7VoiceData::kVoiceParameterCount>
    kVoiceParameterSuffixes
{
    "PitchRate1", "PitchRate2", "PitchRate3", "PitchRate4",
    "PitchLevel1", "PitchLevel2", "PitchLevel3", "PitchLevel4",
    "Algorithm", "Feedback", "OscillatorKeySync", "LfoSpeed", "LfoDelay",
    "PitchModDepth", "AmplitudeModDepth", "LfoKeySync", "LfoWaveform",
    "PitchModSensitivity", "Transpose"
};

constexpr std::array<const char*, VDX7VoiceData::kVoiceParameterCount>
    kVoiceParameterNames
{
    "Pitch EG Rate 1", "Pitch EG Rate 2", "Pitch EG Rate 3", "Pitch EG Rate 4",
    "Pitch EG Level 1", "Pitch EG Level 2", "Pitch EG Level 3", "Pitch EG Level 4",
    "Algorithm", "Feedback", "Oscillator Key Sync", "LFO Speed", "LFO Delay",
    "Pitch Modulation Depth", "Amplitude Modulation Depth", "LFO Key Sync",
    "LFO Waveform", "Pitch Modulation Sensitivity", "Transpose"
};

float defaultOperatorValue(VDX7VoiceData::Parameter parameter)
{
    switch (parameter)
    {
        case VDX7VoiceData::Parameter::rate1:
        case VDX7VoiceData::Parameter::rate2:
        case VDX7VoiceData::Parameter::rate3:
        case VDX7VoiceData::Parameter::rate4:
        case VDX7VoiceData::Parameter::level1:
        case VDX7VoiceData::Parameter::level2:
        case VDX7VoiceData::Parameter::level3:
        case VDX7VoiceData::Parameter::outputLevel:
            return 99.0f;
        case VDX7VoiceData::Parameter::coarse:
            return 1.0f;
        case VDX7VoiceData::Parameter::breakpoint:
            return 39.0f;
        default:
            return 0.0f;
    }
}

float defaultVoiceValue(VDX7VoiceData::VoiceParameter parameter)
{
    switch (parameter)
    {
        case VDX7VoiceData::VoiceParameter::pitchRate1:
        case VDX7VoiceData::VoiceParameter::pitchRate2:
        case VDX7VoiceData::VoiceParameter::pitchRate3:
        case VDX7VoiceData::VoiceParameter::pitchRate4:
            return 99.0f;
        case VDX7VoiceData::VoiceParameter::pitchLevel1:
        case VDX7VoiceData::VoiceParameter::pitchLevel2:
        case VDX7VoiceData::VoiceParameter::pitchLevel3:
        case VDX7VoiceData::VoiceParameter::pitchLevel4:
            return 50.0f;
        case VDX7VoiceData::VoiceParameter::algorithm:
        case VDX7VoiceData::VoiceParameter::oscillatorKeySync:
        case VDX7VoiceData::VoiceParameter::lfoKeySync:
            return 1.0f;
        case VDX7VoiceData::VoiceParameter::lfoSpeed:
            return 35.0f;
        case VDX7VoiceData::VoiceParameter::pitchModSensitivity:
            return 3.0f;
        default:
            return 0.0f;
    }
}
}

juce::String VDX7ParameterIDs::operatorParameter(
    int operatorIndex, VDX7VoiceData::Parameter parameter)
{
    const int safeOperator = juce::jlimit(0, VDX7VoiceData::kOperatorCount - 1,
                                          operatorIndex);
    const int safeParameter = juce::jlimit(0, VDX7VoiceData::kParameterCount - 1,
                                           static_cast<int>(parameter));
    return "op" + juce::String(safeOperator + 1)
         + kOperatorParameterSuffixes[static_cast<std::size_t>(safeParameter)];
}

juce::String VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter parameter)
{
    const int safeParameter = juce::jlimit(0, VDX7VoiceData::kVoiceParameterCount - 1,
                                           static_cast<int>(parameter));
    return "voice" + juce::String(
        kVoiceParameterSuffixes[static_cast<std::size_t>(safeParameter)]);
}

VDX7AudioProcessor::VDX7AudioProcessor(bool detectRom)
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters_(*this, nullptr, kParameterStateType, createParameterLayout())
{
    masterVolumeParameter_ = parameters_.getRawParameterValue(VDX7ParameterIDs::masterVolume);
    pitchWheelParameter_ = parameters_.getRawParameterValue(VDX7ParameterIDs::pitchWheel);
    modWheelParameter_ = parameters_.getRawParameterValue(VDX7ParameterIDs::modWheel);

    jassert(masterVolumeParameter_ != nullptr);
    jassert(pitchWheelParameter_ != nullptr);
    jassert(modWheelParameter_ != nullptr);

    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
            auto& id = operatorParameterIDs_[static_cast<std::size_t>(op)]
                                            [static_cast<std::size_t>(p)];
            id = VDX7ParameterIDs::operatorParameter(op, parameter);
            operatorParameterValues_[static_cast<std::size_t>(op)]
                                    [static_cast<std::size_t>(p)] =
                parameters_.getRawParameterValue(id);
            jassert(operatorParameterValues_[static_cast<std::size_t>(op)]
                                            [static_cast<std::size_t>(p)] != nullptr);
            parameters_.addParameterListener(id, this);
        }
    }

    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
        auto& id = voiceParameterIDs_[static_cast<std::size_t>(p)];
        id = VDX7ParameterIDs::voiceParameter(parameter);
        voiceParameterValues_[static_cast<std::size_t>(p)] =
            parameters_.getRawParameterValue(id);
        jassert(voiceParameterValues_[static_cast<std::size_t>(p)] != nullptr);
        parameters_.addParameterListener(id, this);
    }

    outputGain_.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(masterVolumeParameter_->load()));
    detectRom_ = detectRom;
    keyboardState_.addListener(this);
    if (detectRom_) autoDetectRom();
    startTimerHz(30); // Processor-owned: publication does not require an editor.
}

VDX7AudioProcessor::~VDX7AudioProcessor()
{
    stopTimer();
    keyboardState_.removeListener(this);
    for (const auto& operatorIDs : operatorParameterIDs_)
        for (const auto& id : operatorIDs)
            parameters_.removeParameterListener(id, this);
    for (const auto& id : voiceParameterIDs_)
        parameters_.removeParameterListener(id, this);
}

juce::AudioProcessorValueTreeState::ParameterLayout VDX7AudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { VDX7ParameterIDs::masterVolume, 1 },
        "Master Volume",
        juce::NormalisableRange<float> { -60.0f, 6.0f, 0.1f, 0.45f },
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { VDX7ParameterIDs::pitchWheel, 1 },
        "Pitch Wheel",
        juce::NormalisableRange<float> { -1.0f, 1.0f, 0.001f },
        0.0f));

    layout.add(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID { VDX7ParameterIDs::modWheel, 1 },
        "Mod Wheel",
        juce::NormalisableRange<float> { 0.0f, 1.0f, 0.001f },
        0.0f));

    // Keep the original 90 parameter indices stable for existing DAW projects.
    for (int pass = 0; pass < 2; ++pass)
    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        for (int p = pass == 0 ? 0 : 15;
             p < (pass == 0 ? 15 : VDX7VoiceData::kParameterCount); ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
            const auto id = VDX7ParameterIDs::operatorParameter(op, parameter);
            const auto name = "Operator " + juce::String(op + 1) + " "
                            + kOperatorParameterNames[static_cast<std::size_t>(p)];
            layout.add(std::make_unique<juce::AudioParameterFloat>(
                juce::ParameterID { id, 1 }, name,
                juce::NormalisableRange<float> {
                    static_cast<float>(VDX7VoiceData::parameterMinimum(parameter)),
                    static_cast<float>(VDX7VoiceData::parameterMaximum(parameter)), 1.0f },
                defaultOperatorValue(parameter)));
        }
    }


    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
        const auto id = VDX7ParameterIDs::voiceParameter(parameter);
        layout.add(std::make_unique<juce::AudioParameterFloat>(
            juce::ParameterID { id, 1 },
            kVoiceParameterNames[static_cast<std::size_t>(p)],
            juce::NormalisableRange<float> {
                static_cast<float>(VDX7VoiceData::voiceParameterMinimum(parameter)),
                static_cast<float>(VDX7VoiceData::voiceParameterMaximum(parameter)), 1.0f },
            defaultVoiceValue(parameter)));
    }

    return layout;
}

void VDX7AudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    processLoadMeasurer_.reset(sampleRate, samplesPerBlock);
    std::unique_lock lock(engineMutex_);
    currentSampleRate_ = sampleRate;
    deferredMidi_.clear();
    keyboardQueue_.discard();
    clearKeyboardSnapshot();
    engine_.resetMidiLifecycle();
    engine_.prepare(sampleRate);
    outputGain_.reset(sampleRate, 0.02);
    outputGain_.setCurrentAndTargetValue(
        juce::Decibels::decibelsToGain(masterVolumeParameter_->load()));
    lastPitchMsb_ = -1;
    lastModValue_ = -1;
    const int latency = engine_.latencySamples();
    lock.unlock(); // Host notification can re-enter state callbacks.
    setLatencySamples(latency);
}

void VDX7AudioProcessor::releaseResources()
{
    processLoadMeasurer_.reset();
    std::scoped_lock lock(engineMutex_);
    deferredMidi_.clear();
    keyboardQueue_.discard();
    clearKeyboardSnapshot();
    engine_.resetMidiLifecycle();
    clearMeters();
}

bool VDX7AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void VDX7AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    std::optional<juce::AudioProcessLoadMeasurer::ScopedTimer> loadTimer;
    if (buffer.getNumSamples() > 0)
        loadTimer.emplace(processLoadMeasurer_, buffer.getNumSamples());
    buffer.clear();
    const int inputChannel = midiInputChannel_.load();
    if (inputChannel != audioMidiInputChannel_)
    {
        audioMidiInputChannel_ = inputChannel;
        channelReleasePending_ = true;
        deferredMidi_.clear(); // Do not replay old-channel events after switching.
        clearKeyboardSnapshot();
    }
    std::array<VDX7KeyboardQueue::Event, VDX7KeyboardQueue::capacity> keyboardEvents;
    std::size_t keyboardCount = 0;
    if (keyboardQueue_.recoverOverflow()) deferredMidi_.requestPanic();
    while (keyboardCount < keyboardEvents.size() && keyboardQueue_.pop(keyboardEvents[keyboardCount]))
        ++keyboardCount;
    const int total = buffer.getNumSamples();
    // A long transaction may silence a block, but never blocks audio.
    std::unique_lock lock(engineMutex_, std::try_to_lock);
    if (!lock.owns_lock() && total > 0 && engineLoaded_.load(std::memory_order_acquire))
    {
        contendedAudioBlocks_.fetch_add(1, std::memory_order_relaxed);
        contendedAudioSamples_.fetch_add(static_cast<uint64_t>(total), std::memory_order_relaxed);
    }
    const bool useDeferred = deferredMidi_.active() || !lock.owns_lock();
    if (engineLoaded_.load(std::memory_order_acquire) && useDeferred)
    {
        for (std::size_t i = 0; i < keyboardCount; ++i)
            deferredMidi_.push(keyboardEvents[i].data(), 3, 0);
        for (const auto event : midi)
        {
            if (event.numBytes <= 0 || !VDX7MidiValidation::acceptsHostEvent(
                    event.data, static_cast<std::size_t>(event.numBytes), inputChannel))
                continue;
            deferredMidi_.push(event.data, static_cast<std::size_t>(event.numBytes),
                               juce::jlimit(0, total, event.samplePosition));
        }
        // Do not replay an arbitrarily old performance after a long transaction.
        deferredMidi_.advanceInputBlock(total,
            static_cast<uint64_t>(std::max(currentSampleRate_ * 2.0, static_cast<double>(total))));
    }
    else if (!engineLoaded_.load(std::memory_order_acquire)) deferredMidi_.clear();
    if (!lock.owns_lock() || !engine_.isLoaded())
    {
        midi.clear();
        clearMeters();
        return;
    }

    if (channelReleasePending_)
    {
        engine_.allNotesOff(); // Also releases sustain before new-channel notes.
        clearKeyboardSnapshot();
        channelReleasePending_ = false;
    }
    if (applyOperatorParameters() | applyVoiceParameters())
        engine_.reloadCurrentProgram();
    applyPendingCommands();
    applyPerformanceControls();

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;
    int cursor = 0;

    bool programMemoryChanged = false;
    auto deliver = [&](const uint8_t* data, std::size_t size, int eventPos)
    {
        if (eventPos > cursor)
        {
            engine_.render(left + cursor, right + cursor, eventPos - cursor);
            cursor = eventPos;
        }

        programMemoryChanged |= handleMidiEventLocked(data, static_cast<int>(size));
    };
    if (useDeferred)
        deferredMidi_.renderBlock(total, deliver,
            [&] { engine_.allNotesOff(); clearKeyboardSnapshot(); });
    else
    {
        for (std::size_t i = 0; i < keyboardCount; ++i)
            deliver(keyboardEvents[i].data(), 3, 0);
        for (const auto event : midi)
            if (event.numBytes > 0 && VDX7MidiValidation::acceptsHostEvent(
                    event.data, static_cast<std::size_t>(event.numBytes), inputChannel))
                deliver(event.data, static_cast<std::size_t>(event.numBytes),
                        juce::jlimit(0, total, event.samplePosition));
    }

    if (programMemoryChanged)
        updateEngineSnapshot();

    if (cursor < total)
        engine_.render(left + cursor, right + cursor, total - cursor);
    if (!engine_.hasHeldMidiNotes()) deferredMidi_.resetIfEmpty();
    publishPerformanceDisplay();
    midiOverloadSnapshot_.store(engine_.midiOverloadCount(), std::memory_order_relaxed);

    outputGain_.setTargetValue(
        juce::Decibels::decibelsToGain(masterVolumeParameter_->load()));
    for (int sample = 0; sample < total; ++sample)
    {
        const float gain = outputGain_.getNextValue();
        for (int channel = 0; channel < buffer.getNumChannels(); ++channel)
            buffer.getWritePointer(channel)[sample] *= gain;
    }

    updateMeters(buffer);

    midi.clear();
}

bool VDX7AudioProcessor::handleMidiEventLocked(const uint8_t* data, int size)
{
    if (data == nullptr || size <= 0) return false;
    if (data[0] == 0xf0)
    {
        if (!engine_.handleSysex(data, static_cast<std::size_t>(size))) return false;
        modifiedVoices_.store(0xffffffffu); // Incoming bank has not been exported.
        return true;
    }
    const auto bankRevision = engine_.factoryBankLoadRevision();
    if (!VDX7MidiValidation::isChannelMessage(data, static_cast<std::size_t>(size))) return false;
    engine_.handleMidi(data, size);
    const bool bankChange = engine_.factoryBankLoadRevision() != bankRevision;
    if (bankChange) modifiedVoices_.store(0);
    if (engine_.isMidiRecovering())
    {
        clearKeyboardSnapshot();
        return bankChange;
    }
    if (size == 3 && data[1] < 128)
    {
        const auto mask = static_cast<uint16_t>(1u << (data[0] & 15));
        const auto status = data[0] & 0xf0;
        if (status == 0x90 && data[2] != 0) keyboardSnapshot_[data[1]].fetch_or(mask);
        else if (status == 0x80 || (status == 0x90 && data[2] == 0))
            keyboardSnapshot_[data[1]].fetch_and(static_cast<uint16_t>(~mask));
        else if (status == 0xb0 && data[1] == 123) clearKeyboardSnapshot();
    }
    return bankChange || (size == 2 && (data[0] & 0xf0) == 0xc0);
}

void VDX7AudioProcessor::applyPendingCommands()
{
    VDX7EditQueue::Command command;
    bool selectionChanged = false, edited = false;
    for (std::size_t n = 0; n < VDX7EditQueue::capacity && editQueue_.pop(command); ++n)
    {
        bool changed = false;
        switch (command.kind)
        {
            case VDX7EditQueue::Kind::bank:
                selectionChanged = true;
                // Factory banks replace the single internal RAM bank, just as
                // before. Earlier edits must never migrate into this new bank.
                engine_.selectFactoryBank(command.value);
                modifiedVoices_.store(0);
                edited = false;
                break;
            case VDX7EditQueue::Kind::program:
                selectionChanged = true;
                engine_.selectProgram(command.value);
                edited = false;
                break;
            case VDX7EditQueue::Kind::op:
                changed = engine_.setOperatorParameter(command.index / VDX7VoiceData::kParameterCount,
                    static_cast<VDX7VoiceData::Parameter>(command.index % VDX7VoiceData::kParameterCount),
                    command.value);
                break;
            case VDX7EditQueue::Kind::voice:
                changed = engine_.setVoiceParameter(static_cast<VDX7VoiceData::VoiceParameter>(command.index),
                                                    command.value);
                break;
        }
        if (changed)
        {
            markVoiceModified(); edited = true;
            voicePublicationNeeded_.store(true, std::memory_order_release);
        }
    }
    if (edited) engine_.reloadCurrentProgram();
    if (selectionChanged)
        updateEngineSnapshot();
}

bool VDX7AudioProcessor::applyOperatorParameters()
{
    const uint64_t dirtyLow = operatorParameterDirty_[0].exchange(0, std::memory_order_acq_rel);
    const uint64_t dirtyHigh = operatorParameterDirty_[1].exchange(0, std::memory_order_acq_rel);
    if (dirtyLow == 0 && dirtyHigh == 0)
        return false;

    bool changed = false;
    constexpr int totalParameters = VDX7VoiceData::kOperatorCount
                                  * VDX7VoiceData::kParameterCount;
    for (int index = 0; index < totalParameters; ++index)
    {
        const uint64_t mask = uint64_t { 1 } << (index % 64);
        const bool dirty = index < 64 ? (dirtyLow & mask) != 0 : (dirtyHigh & mask) != 0;
        if (!dirty)
            continue;

        const int op = index / VDX7VoiceData::kParameterCount;
        const int p = index % VDX7VoiceData::kParameterCount;
        const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
        const auto* value = &pendingOperatorValues_[static_cast<std::size_t>(op)]
                                                    [static_cast<std::size_t>(p)];
        if (value != nullptr)
            changed = engine_.setOperatorParameter(
                op, parameter, juce::roundToInt(value->load())) || changed;
    }

    if (changed) markVoiceModified();
    return changed;
}

bool VDX7AudioProcessor::applyVoiceParameters()
{
    const uint32_t dirty = voiceParameterDirty_.exchange(0, std::memory_order_acq_rel);
    if (dirty == 0)
        return false;

    bool changed = false;
    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        if ((dirty & (uint32_t { 1 } << p)) == 0)
            continue;

        const auto* value = &pendingVoiceValues_[static_cast<std::size_t>(p)];
        if (value != nullptr)
            changed = engine_.setVoiceParameter(
                static_cast<VDX7VoiceData::VoiceParameter>(p),
                juce::roundToInt(value->load())) || changed;
    }

    if (changed) markVoiceModified();
    return changed;
}

void VDX7AudioProcessor::applyPerformanceControls()
{
    const int pitchMsb = juce::jlimit(0, 127,
        juce::roundToInt((pitchWheelParameter_->load() + 1.0f) * 63.5f));
    if (pitchMsb != lastPitchMsb_)
    {
        const uint8_t message[] { 0xE0, 0x00, static_cast<uint8_t>(pitchMsb) };
        engine_.handleMidi(message, 3);
        lastPitchMsb_ = pitchMsb;
    }

    const int modulation = juce::jlimit(0, 127,
        juce::roundToInt(modWheelParameter_->load() * 127.0f));
    if (modulation != lastModValue_)
    {
        const uint8_t message[] { 0xB0, 0x01, static_cast<uint8_t>(modulation) };
        engine_.handleMidi(message, 3);
        lastModValue_ = modulation;
    }
}

void VDX7AudioProcessor::updateEngineSnapshot() noexcept
{
    publishPerformanceDisplay();
    engineLoaded_.store(engine_.isLoaded(), std::memory_order_release);
    midiOverloadSnapshot_.store(engine_.midiOverloadCount(), std::memory_order_relaxed);
    factoryVoicesAvailable_.store(engine_.hasFactoryVoices(), std::memory_order_release);
    currentBankSnapshot_.store(engine_.currentBank(), std::memory_order_release);
    currentProgramSnapshot_.store(engine_.currentProgram(), std::memory_order_release);

    char name[11] {};
    engine_.copyCurrentProgramName(name, sizeof(name));
    patchNameRevision_.fetch_add(1, std::memory_order_acq_rel);
    for (std::size_t i = 0; i < patchNameSnapshot_.size(); ++i)
        patchNameSnapshot_[i].store(name[i], std::memory_order_relaxed);
    patchNameRevision_.fetch_add(1, std::memory_order_release);
    voicePublicationNeeded_.store(true, std::memory_order_release);
    operatorVoiceRevision_.fetch_add(1, std::memory_order_release);
}

void VDX7AudioProcessor::clearMeters() noexcept
{
    outputPeakLeft_.store(0.0f, std::memory_order_release);
    outputPeakRight_.store(0.0f, std::memory_order_release);
}

void VDX7AudioProcessor::updateMeters(const juce::AudioBuffer<float>& buffer) noexcept
{
    const int samples = buffer.getNumSamples();
    const float left = buffer.getNumChannels() > 0 ? buffer.getMagnitude(0, 0, samples) : 0.0f;
    const float right = buffer.getNumChannels() > 1 ? buffer.getMagnitude(1, 0, samples) : left;
    outputPeakLeft_.store(left, std::memory_order_release);
    outputPeakRight_.store(right, std::memory_order_release);
}

float VDX7AudioProcessor::getOutputPeak(int channel) const noexcept
{
    return channel == 0
        ? outputPeakLeft_.load(std::memory_order_acquire)
        : outputPeakRight_.load(std::memory_order_acquire);
}

double VDX7AudioProcessor::getCpuUsagePercent() const
{
    return processLoadMeasurer_.getLoadAsPercentage();
}

uint32_t VDX7AudioProcessor::getOperatorVoiceRevision() const noexcept
{
    return operatorVoiceRevision_.load(std::memory_order_acquire);
}

bool VDX7AudioProcessor::synchroniseOperatorParametersFromEngine()
{
    // Non-RT only: never wait for another publisher or recurse when a host
    // notification synchronously asks for project state.
    if (publishingParameters_.exchange(true, std::memory_order_acq_rel))
        return false;
    struct EndPublication
    {
        std::atomic<bool>& active;
        std::atomic<uint32_t>& revision;
        bool changed = false;
        ~EndPublication()
        {
            if (changed) revision.fetch_add(1, std::memory_order_release);
            active.store(false, std::memory_order_release);
        }
    } endPublication {publishingParameters_, operatorVoiceRevision_};
    std::array<int, VDX7VoiceData::kOperatorCount * VDX7VoiceData::kParameterCount> values {};
    std::array<int, VDX7VoiceData::kVoiceParameterCount> voiceValues {};
    {
        std::scoped_lock lock(engineMutex_);
        if (!engine_.isLoaded()) return false;
        flushVoiceEditsLocked();
        voicePublicationNeeded_.store(false, std::memory_order_release);
        for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
            for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
                values[static_cast<std::size_t>(op * VDX7VoiceData::kParameterCount + p)] =
                    engine_.getOperatorParameter(op, static_cast<VDX7VoiceData::Parameter>(p));

        for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
            voiceValues[static_cast<std::size_t>(p)] = engine_.getVoiceParameter(
                static_cast<VDX7VoiceData::VoiceParameter>(p));
    }

    // No engine lock is held while calling the host.
    const juce::ScopedValueSetter<const VDX7AudioProcessor*> publishing(publishingVoice, this);
    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            const auto& id = operatorParameterIDs_[static_cast<std::size_t>(op)]
                                                  [static_cast<std::size_t>(p)];
            if (voicePublicationNeeded_.load(std::memory_order_acquire) || editQueue_.pending())
                return false; // Retry the newer state on the next non-RT poll.
            if (auto* parameter = parameters_.getParameter(id))
            {
                const float normalised = parameter->convertTo0to1(
                    static_cast<float>(values[static_cast<std::size_t>(
                        op * VDX7VoiceData::kParameterCount + p)]));
                if (std::abs(parameter->getValue() - normalised) > 0.0001f)
                {
                    const juce::ScopedValueSetter<const juce::String*> ownParameter(publishingParameterID, &id);
                    const juce::ScopedValueSetter<float> ownValue(publishingParameterValue,
                        parameter->convertFrom0to1(normalised));
                    parameter->setValueNotifyingHost(normalised);
                    endPublication.changed = true;
                }
            }
        }
    }
    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        if (auto* parameter = parameters_.getParameter(
                voiceParameterIDs_[static_cast<std::size_t>(p)]))
        {
            if (voicePublicationNeeded_.load(std::memory_order_acquire) || editQueue_.pending())
                return false;
            const float normalised = parameter->convertTo0to1(
                static_cast<float>(voiceValues[static_cast<std::size_t>(p)]));
            if (std::abs(parameter->getValue() - normalised) > 0.0001f)
            {
                const auto& id = voiceParameterIDs_[static_cast<std::size_t>(p)];
                const juce::ScopedValueSetter<const juce::String*> ownParameter(publishingParameterID, &id);
                const juce::ScopedValueSetter<float> ownValue(publishingParameterValue,
                    parameter->convertFrom0to1(normalised));
                parameter->setValueNotifyingHost(normalised);
                endPublication.changed = true;
            }
        }
    }
    return true;
}

void VDX7AudioProcessor::timerCallback()
{
    mirrorKeyboardOnMessageThread();
    if (voicePublicationNeeded_.load(std::memory_order_acquire) || editQueue_.pending())
        synchroniseOperatorParametersFromEngine();
}

namespace { thread_local const VDX7AudioProcessor* mirroringKeyboard = nullptr; }

void VDX7AudioProcessor::handleNoteOn(juce::MidiKeyboardState*, int channel, int note, float velocity)
{
    if (mirroringKeyboard == this) return;
    keyboardUiHeld_[static_cast<std::size_t>(note)].fetch_or(static_cast<uint16_t>(1u << (channel - 1)));
    keyboardQueue_.push({static_cast<uint8_t>(0x90 | (channel - 1)),
        static_cast<uint8_t>(note), static_cast<uint8_t>(juce::jlimit(1, 127, juce::roundToInt(velocity * 127)))});
}

void VDX7AudioProcessor::handleNoteOff(juce::MidiKeyboardState*, int channel, int note, float)
{
    if (mirroringKeyboard == this) return;
    keyboardUiHeld_[static_cast<std::size_t>(note)].fetch_and(static_cast<uint16_t>(~(1u << (channel - 1))));
    keyboardQueue_.push({static_cast<uint8_t>(0x80 | (channel - 1)), static_cast<uint8_t>(note), 0});
}

void VDX7AudioProcessor::clearKeyboardSnapshot() noexcept
{
    for (auto& note : keyboardSnapshot_) note.store(0, std::memory_order_relaxed);
}

void VDX7AudioProcessor::mirrorKeyboardOnMessageThread()
{
    const juce::ScopedValueSetter<const VDX7AudioProcessor*> guard(mirroringKeyboard, this);
    for (int note = 0; note < 128; ++note)
    {
        const auto channels = keyboardSnapshot_[static_cast<std::size_t>(note)].load()
                            | keyboardUiHeld_[static_cast<std::size_t>(note)].load();
        for (int channel = 1; channel <= 16; ++channel)
        {
            const bool down = (channels & (1u << (channel - 1))) != 0;
            if (down != keyboardState_.isNoteOn(channel, note))
                keyboardState_.processNextMidiEvent(down
                    ? juce::MidiMessage::noteOn(channel, note, juce::uint8(100))
                    : juce::MidiMessage::noteOff(channel, note));
        }
    }
    // JUCE also keeps an indirect-event buffer for UI notes. Our listener
    // already delivered those via the bounded queue, so discard its copy here.
    juce::MidiBuffer unused;
    keyboardState_.processNextMidiBuffer(unused, 0, 1, false);
}

void VDX7AudioProcessor::parameterChanged(const juce::String& parameterID, float newValue)
{
    if (publishingVoice == this
        && (publishingParameterID == nullptr
            || (*publishingParameterID == parameterID && newValue == publishingParameterValue)))
        return;

    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            if (operatorParameterIDs_[static_cast<std::size_t>(op)]
                                     [static_cast<std::size_t>(p)] != parameterID)
                continue;

            const int index = op * VDX7VoiceData::kParameterCount + p;
            if (engineLoaded_.load(std::memory_order_acquire))
            {
                editQueue_.push({VDX7EditQueue::Kind::op, index, juce::roundToInt(newValue)});
                voicePublicationNeeded_.store(true, std::memory_order_release);
                return;
            }
            pendingOperatorValues_[static_cast<std::size_t>(op)][static_cast<std::size_t>(p)]
                .store(newValue, std::memory_order_relaxed);
            operatorParameterDirty_[static_cast<std::size_t>(index / 64)].fetch_or(
                uint64_t { 1 } << (index % 64), std::memory_order_release);
            return;
        }
    }

    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        if (voiceParameterIDs_[static_cast<std::size_t>(p)] != parameterID)
            continue;

        if (engineLoaded_.load(std::memory_order_acquire))
        {
            editQueue_.push({VDX7EditQueue::Kind::voice, p, juce::roundToInt(newValue)});
            voicePublicationNeeded_.store(true, std::memory_order_release);
            return;
        }
        pendingVoiceValues_[static_cast<std::size_t>(p)].store(newValue, std::memory_order_relaxed);
        voiceParameterDirty_.fetch_or(uint32_t { 1 } << p, std::memory_order_release);
        return;
    }
}

juce::AudioProcessorEditor* VDX7AudioProcessor::createEditor()
{
    return new VDX7AudioProcessorEditor(*this);
}

int VDX7AudioProcessor::getCurrentProgram()
{
    return currentProgramSnapshot_.load(std::memory_order_acquire);
}

void VDX7AudioProcessor::setCurrentProgram(int index)
{
    if (!engineLoaded_.load(std::memory_order_acquire))
        return;

    const int program = juce::jlimit(0, 31, index);
    if (editQueue_.push({VDX7EditQueue::Kind::program, 0, program}))
        currentProgramSnapshot_.store(program, std::memory_order_release);
}

const juce::String VDX7AudioProcessor::getProgramName(int index)
{
    if (!engineLoaded_.load(std::memory_order_acquire))
        return "Program " + juce::String(index + 1);

    if (index == getCurrentProgram())
        return getCurrentPatchName();

    // Avoid altering the emulated machine just to query a DAW menu name.
    return "Program " + juce::String(index + 1);
}

void VDX7AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    synchroniseOperatorParametersFromEngine();
    juce::ValueTree state { juce::Identifier(kStateType) };
    std::array<int, VDX7VoiceData::kOperatorCount * VDX7VoiceData::kParameterCount> operatorValues {};
    std::array<int, VDX7VoiceData::kVoiceParameterCount> voiceValues {};
    bool loaded = false;

    {
        std::scoped_lock lock(engineMutex_);
        // A project saved while its firmware is missing must retain its sound.
        if (pendingRestore_.isValid())
        {
            pendingRestore_.setProperty("midiInputChannel", midiInputChannel_.load(), nullptr);
            capturePendingRestoreEditsLocked();
            if (auto xml = pendingRestore_.createXml())
                copyXmlToBinary(*xml, destData);
            return;
        }
        // Capture even the most recent GUI/automation edits if the host asks
        // for state before another audio block has had a chance to run.
        if (applyOperatorParameters() | applyVoiceParameters())
            engine_.reloadCurrentProgram();
        applyPendingCommands();
        state.setProperty("bank", engine_.currentBank(), nullptr);
        state.setProperty("midiInputChannel", midiInputChannel_.load(), nullptr);
        state.setProperty("program", engine_.currentProgram(), nullptr);
        state.setProperty("modifiedVoices", static_cast<juce::int64>(modifiedVoices_.load()), nullptr);

        std::vector<uint8_t> ram;
        if (engine_.saveRam(ram) && !ram.empty())
        {
            juce::MemoryBlock block(ram.data(), ram.size());
            state.setProperty("ram", block.toBase64Encoding(), nullptr);
        }
        loaded = engine_.isLoaded();
        if (loaded)
        {
            for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
                for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
                    operatorValues[op * VDX7VoiceData::kParameterCount + p] =
                        engine_.getOperatorParameter(op, static_cast<VDX7VoiceData::Parameter>(p));
            for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
                voiceValues[p] = engine_.getVoiceParameter(static_cast<VDX7VoiceData::VoiceParameter>(p));
        }
    }

    {
        std::scoped_lock lock(metadataMutex_);
        state.setProperty("romPath", romFile_.getFullPathName(), nullptr);
    }

    auto parameterState = parameters_.copyState();
    // A headless/reentrant save must not serialize a half-published host view.
    // Patch this detached copy from the same engine snapshot as the saved RAM.
    if (loaded)
    {
        auto setSavedValue = [&](const juce::String& id, int value) {
            auto child = parameterState.getChildWithProperty("id", id);
            if (child.isValid()) child.setProperty("value", static_cast<float>(value), nullptr);
        };
        for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
            for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
                setSavedValue(operatorParameterIDs_[op][p], operatorValues[op * VDX7VoiceData::kParameterCount + p]);
        for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
            setSavedValue(voiceParameterIDs_[p], voiceValues[p]);
    }
    state.addChild(parameterState, -1, nullptr);

    if (auto xml = state.createXml())
        copyXmlToBinary(*xml, destData);
}

void VDX7AudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml == nullptr)
        return;

    auto state = juce::ValueTree::fromXml(*xml);
    if (!state.isValid() || state.getType().toString() != kStateType)
        return;

    const auto ramText = state.getProperty("ram").toString();
    if (ramText.isNotEmpty())
    {
        juce::MemoryBlock ram;
        if (!ram.fromBase64Encoding(ramText) || ram.getSize() != VDX7Engine::kRamStateSize)
            return; // Malformed state must not replace a usable/pending project.
    }

    {
        std::scoped_lock lock(engineMutex_);
        pendingRestore_ = state.createCopy();
        const int channel = static_cast<int>(state.getProperty("midiInputChannel", 0));
        midiInputChannel_.store(channel >= 0 && channel <= 16 ? channel : 0);
        operatorParameterDirty_[0].store(0);
        operatorParameterDirty_[1].store(0);
        voiceParameterDirty_.store(0);
    }
    const auto parameterState = state.getChildWithName(juce::Identifier(kParameterStateType));
    if (parameterState.isValid())
    {
        // Packed RAM is authoritative for voice settings, including older states
        // which did not expose every field as a host parameter.
        const juce::ScopedValueSetter<const VDX7AudioProcessor*> publishing(publishingVoice, this);
        const juce::ScopedValueSetter<const juce::String*> allParameters(publishingParameterID, nullptr);
        parameters_.replaceState(parameterState);
    }

    const juce::File savedRom(state.getProperty("romPath").toString());
    if (savedRom.existsAsFile())
        loadRomFromFile(savedRom, nullptr);
    else if (!isRomLoaded() && detectRom_)
        autoDetectRom();

    {
        std::scoped_lock lock(engineMutex_);
        if (engine_.isLoaded() && pendingRestore_.isValid())
        {
            restoreSavedStateLocked(pendingRestore_);
            pendingRestore_ = {};
        }
    }
    if (!isRomLoaded())
    {
        std::scoped_lock lock(metadataMutex_);
        statusText_ = "Project preserved: load compatible ROM to restore its sound";
    }
    synchroniseOperatorParametersFromEngine();
}

void VDX7AudioProcessor::capturePendingRestoreEditsLocked()
{
    if (!pendingRestore_.isValid()) return;
    const auto previous = pendingRestore_.getChildWithName(juce::Identifier(kParameterStateType));
    if (previous.isValid()) pendingRestore_.removeChild(previous, nullptr);
    pendingRestore_.addChild(parameters_.copyState(), -1, nullptr);

    // Explicit edits made AFTER restore override RAM; untouched host defaults
    // never override authoritative packed RAM, including legacy projects.
    auto edits = pendingRestore_.getChildWithName("DeferredVoiceEdits");
    const auto low = operatorParameterDirty_[0].exchange(0);
    const auto high = operatorParameterDirty_[1].exchange(0);
    const auto voice = voiceParameterDirty_.exchange(0);
    if ((low | high | voice) == 0) return;
    if (!edits.isValid())
    {
        edits = juce::ValueTree("DeferredVoiceEdits");
        pendingRestore_.addChild(edits, -1, nullptr);
    }
    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            const int index = op * VDX7VoiceData::kParameterCount + p;
            if (((index < 64 ? low : high) & (uint64_t{1} << (index % 64))) != 0)
                edits.setProperty(juce::Identifier("op" + juce::String(index)),
                    juce::roundToInt(pendingOperatorValues_[op][p].load()), nullptr);
        }
    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
        if ((voice & (uint32_t{1} << p)) != 0)
            edits.setProperty(juce::Identifier("voice" + juce::String(p)),
                juce::roundToInt(pendingVoiceValues_[p].load()), nullptr);
}

void VDX7AudioProcessor::restoreSavedStateLocked(const juce::ValueTree& state)
{
        const int bank = static_cast<int>(state.getProperty("bank", -1));
        const int program = static_cast<int>(state.getProperty("program", 0));

        const auto ramText = state.getProperty("ram").toString();
        if (ramText.isNotEmpty())
        {
            juce::MemoryBlock block;
            if (block.fromBase64Encoding(ramText) && block.getSize() == VDX7Engine::kRamStateSize)
            {
                std::vector<uint8_t> ram(block.getSize());
                std::memcpy(ram.data(), block.getData(), block.getSize());
                if (engine_.restoreRam(ram))
                    engine_.setCurrentBankMarker(bank);
            }
        }
        else if (bank >= 0)
        {
            engine_.selectFactoryBank(bank);
        }

        engine_.selectProgram(program);
        modifiedVoices_.store(static_cast<uint32_t>(static_cast<juce::int64>(
            state.getProperty("modifiedVoices", juce::int64(ramText.isNotEmpty() ? -1 : 0)))));
        const auto edits = state.getChildWithName("DeferredVoiceEdits");
        bool changed = false;
        for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
            for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
            {
                const juce::Identifier key("op" + juce::String(op * VDX7VoiceData::kParameterCount + p));
                if (edits.hasProperty(key))
                    changed |= engine_.setOperatorParameter(op, static_cast<VDX7VoiceData::Parameter>(p),
                                                            static_cast<int>(edits[key]));
            }
        for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
        {
            const juce::Identifier key("voice" + juce::String(p));
            if (edits.hasProperty(key))
                changed |= engine_.setVoiceParameter(static_cast<VDX7VoiceData::VoiceParameter>(p),
                                                     static_cast<int>(edits[key]));
        }
        if (changed)
        {
            engine_.reloadCurrentProgram();
            modifiedVoices_.fetch_or(uint32_t{1} << engine_.currentProgram());
        }
        operatorParameterDirty_[0].store(0, std::memory_order_release);
        operatorParameterDirty_[1].store(0, std::memory_order_release);
        voiceParameterDirty_.store(0, std::memory_order_release);
        editQueue_.discard();
        lastPitchMsb_ = -1;
        lastModValue_ = -1;
        updateEngineSnapshot();
}

bool VDX7AudioProcessor::readFile(const juce::File& file, std::vector<uint8_t>& data)
{
    data.clear();
    if (!file.existsAsFile())
        return false;

    juce::MemoryBlock block;
    if (!file.loadFileAsData(block))
        return false;

    const auto* begin = static_cast<const uint8_t*>(block.getData());
    data.assign(begin, begin + block.getSize());
    return true;
}

bool VDX7AudioProcessor::loadRomData(const juce::File& file, const std::vector<uint8_t>& rom, juce::String* error)
{
    std::vector<uint8_t> voices;
    bool ignoredCompanion = false;

    if (rom.size() == VDX7Engine::kFirmwareSize)
    {
        // Optional companion file created by the README/package workflow.
        const auto companion = file.getSiblingFile("dx7_factory_voices_32KB.bin");
        if (companion.exists())
        {
            ignoredCompanion = !companion.existsAsFile()
                || companion.getSize() != VDX7Engine::kFactoryVoicesSize
                || !readFile(companion, voices)
                || voices.size() != VDX7Engine::kFactoryVoicesSize;
            if (ignoredCompanion) voices.clear();
        }
    }

    {
        std::scoped_lock lock(engineMutex_);
        capturePendingRestoreEditsLocked();
        const bool ok = engine_.loadRomImage(rom.data(), rom.size(),
                                             voices.empty() ? nullptr : voices.data(), voices.size());
        if (!ok)
        {
            std::scoped_lock metadataLock(metadataMutex_);
            statusText_ = "Invalid ROM: expected 16 KB firmware or 48 KB combined image";
            if (error != nullptr) *error = statusText_;
            return false;
        }

        engine_.prepare(currentSampleRate_);
        modifiedVoices_.store(0);
        operatorParameterDirty_[0].store(0, std::memory_order_release);
        operatorParameterDirty_[1].store(0, std::memory_order_release);
        voiceParameterDirty_.store(0, std::memory_order_release);
        editQueue_.discard();
        lastPitchMsb_ = -1;
        lastModValue_ = -1;
        if (pendingRestore_.isValid())
        {
            restoreSavedStateLocked(pendingRestore_);
            pendingRestore_ = {};
        }
        else
            updateEngineSnapshot();
    }

    {
        std::scoped_lock lock(metadataMutex_);
        romFile_ = file;
        statusText_ = factoryVoicesAvailable_.load(std::memory_order_acquire)
            ? "DX7 firmware loaded + 8 factory banks"
            : "DX7 firmware loaded (factory voice image not found)";
        if (ignoredCompanion)
            statusText_ = "DX7 firmware loaded; invalid or unreadable optional factory voice image ignored";
    }
    synchroniseOperatorParametersFromEngine();
    return true;
}

bool VDX7AudioProcessor::loadRomFromFile(const juce::File& file, juce::String* error)
{
    std::vector<uint8_t> data;
    if (!readFile(file, data))
    {
        if (error != nullptr) *error = "Could not read ROM file";
        return false;
    }
    return loadRomData(file, data, error);
}

bool VDX7AudioProcessor::loadSyxFromFile(const juce::File& file, juce::String* error)
{
    std::vector<uint8_t> data;
    if (!readFile(file, data))
    {
        if (error != nullptr) *error = "Could not read SysEx file";
        return false;
    }

    std::vector<uint8_t> packed;
    if (!VDX7Sysex::decode(data, packed))
    {
        if (error != nullptr) *error = "Expected one valid DX7 voice (163 bytes) or bank (4104 bytes), with a valid checksum.";
        return false;
    }
    if (!loadPackedVoices(packed, error)) return false;
    {
        std::scoped_lock lock(metadataMutex_);
        statusText_ = "Loaded SysEx: " + file.getFileName();
    }
    return true;
}

juce::File VDX7AudioProcessor::userBankFile()
{
    auto root = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory);
#if JUCE_MAC
    root = root.getChildFile("Application Support");
#endif
    return root.getChildFile("VDX7-JUCE").getChildFile("User Banks").getChildFile("USER.vub");
}

bool VDX7AudioProcessor::captureUserPatch(VDX7UserBank::Voice& voice, juce::String& error)
{
    std::scoped_lock lock(engineMutex_);
    if (!engine_.isLoaded()) { error = "Load the firmware first."; return false; }
    flushVoiceEditsLocked();
    std::vector<uint8_t> ram;
    if (!engine_.saveRam(ram)) { error = "Cannot capture voice RAM."; return false; }
    std::copy_n(ram.begin() + engine_.currentProgram() * 128, 128, voice.begin());
    return true;
}

bool VDX7AudioProcessor::loadUserBank(const juce::File& file, juce::String& error)
{
    VDX7UserBank::Snapshot bank;
    if (auto result = VDX7UserBank::load(file, bank); result.failed())
    { error = result.getErrorMessage(); return false; }
    if (!bank.exists || bank.occupiedMask == 0)
    { error = "No saved USER patches yet. Use SAVE AS to save your first patch."; return false; }
    std::vector<uint8_t> packed;
    for (const auto& voice : bank.voices) packed.insert(packed.end(), voice.begin(), voice.end());
    int firstOccupied = 0;
    while (!bank.occupied(firstOccupied)) ++firstOccupied;
    if (!loadPackedVoices(packed, &error, firstOccupied)) return false;
    {
        std::scoped_lock lock(metadataMutex_);
        statusText_ = "Loaded USER bank as editable CUSTOM copy";
    }
    return true;
}

bool VDX7AudioProcessor::loadPackedVoices(const std::vector<uint8_t>& packed, juce::String* error, int selectProgram)
{
    if (packed.size() != 128 && packed.size() != 4096) return false;
    {
        std::scoped_lock lock(engineMutex_);
        if (!engine_.isLoaded())
        {
            if (error != nullptr) *error = "Load the DX7 firmware first";
            return false;
        }

        flushVoiceEditsLocked();
        std::vector<uint8_t> ram;
        if (!engine_.saveRam(ram)) return false;
        const bool single = packed.size() == 128;
        const int offset = single ? engine_.currentProgram() * 128 : 0;
        std::copy(packed.begin(), packed.end(), ram.begin() + offset);
        if (!engine_.restoreRam(ram)) return false;
        engine_.setCurrentBankMarker(-1);
        if (selectProgram >= 0 && selectProgram < 32) engine_.selectProgram(selectProgram);
        if (single) modifiedVoices_.fetch_and(~(uint32_t(1) << engine_.currentProgram()));
        else modifiedVoices_.store(0);

        // A newly imported bank replaces the prior editable voice and requests.
        operatorParameterDirty_[0].store(0, std::memory_order_release);
        operatorParameterDirty_[1].store(0, std::memory_order_release);
        voiceParameterDirty_.store(0, std::memory_order_release);
        editQueue_.discard();
        updateEngineSnapshot();
    }

    synchroniseOperatorParametersFromEngine();
    updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    return true;
}

void VDX7AudioProcessor::markVoiceModified() noexcept
{
    modifiedVoices_.fetch_or(uint32_t(1) << engine_.currentProgram());
}

bool VDX7AudioProcessor::hasUnexportedEdits() const noexcept
{
    return editQueue_.hasEdits() || modifiedVoices_.load() != 0 || operatorParameterDirty_[0].load() != 0
        || operatorParameterDirty_[1].load() != 0 || voiceParameterDirty_.load() != 0;
}

bool VDX7AudioProcessor::isCurrentVoiceModified() const noexcept
{
    return editQueue_.hasEdits() || (modifiedVoices_.load() & (uint32_t(1) << currentProgramSnapshot_.load())) != 0
        || operatorParameterDirty_[0].load() != 0 || operatorParameterDirty_[1].load() != 0
        || voiceParameterDirty_.load() != 0;
}

void VDX7AudioProcessor::flushVoiceEditsLocked()
{
    if (applyOperatorParameters() | applyVoiceParameters()) engine_.reloadCurrentProgram();
    applyPendingCommands();
}

bool VDX7AudioProcessor::renameVoice(const juce::String& name)
{
    const auto clean = name.trim();
    if (clean.isEmpty() || clean.length() > 10) return false;
    for (auto c : clean) if (c < 32 || c > 126) return false;
    std::unique_lock lock(engineMutex_);
    if (!engine_.isLoaded()) return false;
    flushVoiceEditsLocked();
    std::vector<uint8_t> ram;
    if (!engine_.saveRam(ram)) return false;
    const int offset = engine_.currentProgram()*128+118;
    const auto text = clean.paddedRight(' ', 10);
    bool changed = false;
    for (int i=0;i<10;++i)
    {
        changed = changed || ram[offset+i] != static_cast<uint8_t>(text[i]);
        ram[offset+i] = static_cast<uint8_t>(text[i]);
    }
    if (changed)
    {
        engine_.restoreRam(ram);
        markVoiceModified();
        updateEngineSnapshot();
    }
    lock.unlock();
    if (changed) updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    synchroniseOperatorParametersFromEngine();
    return true;
}

bool VDX7AudioProcessor::copyOperator(int op)
{
    if (op < 0 || op > 5) return false;
    std::scoped_lock lock(engineMutex_);
    if (!engine_.isLoaded()) return false;
    flushVoiceEditsLocked();
    std::vector<uint8_t> ram;
    if (!engine_.saveRam(ram)) return false;
    const int offset = engine_.currentProgram()*128+(5-op)*17;
    std::copy_n(ram.begin()+offset,17,operatorClipboard_.begin());
    hasOperatorClipboard_.store(true);
    return true;
}

bool VDX7AudioProcessor::pasteOperator(int op)
{
    if (op < 0 || op > 5) return false;
    std::unique_lock lock(engineMutex_);
    if (!engine_.isLoaded() || !hasOperatorClipboard_.load()) return false;
    flushVoiceEditsLocked();
    std::vector<uint8_t> ram;
    if (!engine_.saveRam(ram)) return false;
    const int offset = engine_.currentProgram()*128+(5-op)*17;
    if (std::equal(operatorClipboard_.begin(),operatorClipboard_.end(),ram.begin()+offset)) return true;
    std::copy(operatorClipboard_.begin(),operatorClipboard_.end(),ram.begin()+offset);
    engine_.restoreRam(ram);
    markVoiceModified();
    updateEngineSnapshot();
    lock.unlock();
    updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    synchroniseOperatorParametersFromEngine();
    return true;
}

bool VDX7AudioProcessor::exportSyx(const juce::File& file, bool entireBank, juce::String& error)
{
    std::vector<uint8_t> packed;
    int program = 0;
    {
        std::scoped_lock lock(engineMutex_);
        if (!engine_.isLoaded()) { error = "Load the ROM first."; return false; }
        flushVoiceEditsLocked();
        std::vector<uint8_t> ram;
        if (!engine_.saveRam(ram)) { error = "Cannot capture voice RAM."; return false; }
        program = engine_.currentProgram();
        const int offset = entireBank ? 0 : program*128;
        packed.assign(ram.begin()+offset,ram.begin()+offset+(entireBank ? 4096 : 128));
    }
    auto data = VDX7Sysex::encode(packed);
    if (data.empty()) { error = "Voice data cannot be encoded as 7-bit SysEx."; return false; }
    // Never truncate the destination before the complete replacement is ready.
    juce::TemporaryFile temp(file);
    if (!temp.getFile().replaceWithData(data.data(),data.size()) || !temp.overwriteTargetFileWithTemporary())
    { error = "Could not save the SysEx file. Check the destination and permissions."; return false; }
    {
        std::scoped_lock lock(engineMutex_);
        flushVoiceEditsLocked();
        std::vector<uint8_t> current;
        if (engine_.saveRam(current))
        {
            const int offset = entireBank ? 0 : program*128;
            if (std::equal(packed.begin(),packed.end(),current.begin()+offset))
            {
                if (entireBank) modifiedVoices_.store(0);
                else modifiedVoices_.fetch_and(~(uint32_t(1)<<program));
            }
        }
    }
    {
        std::scoped_lock lock(metadataMutex_);
        statusText_ = "Saved SysEx: " + file.getFileName();
    }
    updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    return true;
}

void VDX7AudioProcessor::publishPerformanceDisplay() noexcept
{
    // 4 * (7-bit range + 3 assignments), 3 play flags + 7-bit time,
    // and two 4-bit bend values: one coherent 58-bit display frame.
    uint64_t packed = 0;
    for (int c = 0; c < 4; ++c) {
        packed |= uint64_t(engine_.getControllerSetting(c, 0) & 127) << (c * 10);
        for (int f = 1; f < 4; ++f)
            packed |= uint64_t(engine_.getControllerSetting(c, f) & 1) << (c * 10 + 6 + f);
    }
    for (int f = 0; f < 3; ++f) packed |= uint64_t(engine_.getPlaySetting(f) & 1) << (40 + f);
    packed |= uint64_t(engine_.getPlaySetting(3) & 127) << 43;
    for (int f = 0; f < 2; ++f) packed |= uint64_t(engine_.getPitchBendSetting(f) & 15) << (50 + f * 4);
    performanceDisplay_.store(packed, std::memory_order_release);
}

VDX7AudioProcessor::PerformanceDisplay VDX7AudioProcessor::getPerformanceDisplay() const noexcept
{
    const auto packed = performanceDisplay_.load(std::memory_order_acquire);
    PerformanceDisplay result;
    for (int c = 0; c < 4; ++c) {
        result.controllers[c * 4] = int((packed >> (c * 10)) & 127);
        for (int f = 1; f < 4; ++f) result.controllers[c * 4 + f] = int((packed >> (c * 10 + 6 + f)) & 1);
    }
    for (int f = 0; f < 3; ++f) result.play[f] = int((packed >> (40 + f)) & 1);
    result.play[3] = int((packed >> 43) & 127);
    for (int f = 0; f < 2; ++f) result.bend[f] = int((packed >> (50 + f * 4)) & 15);
    return result;
}

std::array<int, 16> VDX7AudioProcessor::getControllerSettings() const
{
    std::array<int, 16> result {};
    std::scoped_lock lock(engineMutex_);
    for (int controller = 0; controller < 4; ++controller)
        for (int field = 0; field < 4; ++field)
            result[controller * 4 + field] = engine_.getControllerSetting(controller, field);
    return result;
}

std::array<int, 2> VDX7AudioProcessor::getPitchBendSettings() const
{
    std::scoped_lock lock(engineMutex_);
    return {engine_.getPitchBendSetting(0), engine_.getPitchBendSetting(1)};
}

std::array<int, 4> VDX7AudioProcessor::getPlaySettings() const
{
    std::scoped_lock lock(engineMutex_);
    return {engine_.getPlaySetting(0), engine_.getPlaySetting(1), engine_.getPlaySetting(2), engine_.getPlaySetting(3)};
}

bool VDX7AudioProcessor::setPlaySettingFromUi(int field, int value)
{
    bool changed;
    {
        std::scoped_lock lock(engineMutex_);
        const int previous = engine_.getPlaySetting(field);
        if (!engine_.setPlaySetting(field, value)) return false;
        publishPerformanceDisplay();
        changed = previous != value;
    }
    if (changed) updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    return true;
}

int VDX7AudioProcessor::getMasterTune() const
{
    std::scoped_lock lock(engineMutex_);
    return engine_.masterTune();
}

bool VDX7AudioProcessor::setMidiInputChannelFromUi(int channel)
{
    if (channel < 0 || channel > 16) return false;
    if (midiInputChannel_.exchange(channel) != channel)
        updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    return true;
}

bool VDX7AudioProcessor::setMasterTuneFromUi(int value)
{
    bool changed;
    {
        std::scoped_lock lock(engineMutex_);
        changed = engine_.masterTune() != value;
        if (!engine_.setMasterTune(value)) return false;
    }
    if (changed) updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    return true;
}

bool VDX7AudioProcessor::setPitchBendSettingFromUi(int field, int value)
{
    bool changed;
    {
        std::scoped_lock lock(engineMutex_);
        const int previous = engine_.getPitchBendSetting(field);
        if (!engine_.setPitchBendSetting(field, value)) return false;
        publishPerformanceDisplay();
        changed = previous != value;
    }
    if (changed) updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    return true;
}

bool VDX7AudioProcessor::setControllerSettingFromUi(int controller, int field, int value)
{
    bool changed = false;
    {
        std::scoped_lock lock(engineMutex_);
        const int previous = engine_.getControllerSetting(controller, field);
        if (!engine_.setControllerSetting(controller, field, value)) return false;
        publishPerformanceDisplay();
        changed = previous != value;
    }
    // Do not mark a voice dirty: these globals are outside the packed voice bank.
    // Notification must remain outside engineMutex_ (host may re-enter state save).
    if (changed) updateHostDisplay(ChangeDetails{}.withNonParameterStateChanged(true));
    return true;
}

bool VDX7AudioProcessor::selectFactoryBank(int bank)
{
    if (!engineLoaded_.load(std::memory_order_acquire)
        || !factoryVoicesAvailable_.load(std::memory_order_acquire)
        || bank < 0 || bank > 7)
        return false;

    if (!editQueue_.push({VDX7EditQueue::Kind::bank, 0, bank})) return false;
    currentBankSnapshot_.store(bank, std::memory_order_release);
    {
        std::scoped_lock lock(metadataMutex_);
        statusText_ = "Factory bank " + bankName(bank);
    }
    return true;
}

void VDX7AudioProcessor::selectProgramFromUi(int program)
{
    setCurrentProgram(program);
}

bool VDX7AudioProcessor::isRomLoaded() const
{
    return engineLoaded_.load(std::memory_order_acquire);
}

bool VDX7AudioProcessor::hasFactoryVoices() const
{
    return factoryVoicesAvailable_.load(std::memory_order_acquire);
}

int VDX7AudioProcessor::getCurrentBank() const
{
    return currentBankSnapshot_.load(std::memory_order_acquire);
}

juce::String VDX7AudioProcessor::getCurrentPatchName() const
{
    if (!engineLoaded_.load(std::memory_order_acquire))
        return "---";

    char nameBuffer[11] {};
    for (;;)
    {
        const auto before = patchNameRevision_.load(std::memory_order_acquire);
        if ((before & 1u) != 0)
            continue;

        for (std::size_t i = 0; i < patchNameSnapshot_.size(); ++i)
            nameBuffer[i] = patchNameSnapshot_[i].load(std::memory_order_relaxed);

        const auto after = patchNameRevision_.load(std::memory_order_acquire);
        if (before == after)
            break;
    }

    const auto name = juce::String::fromUTF8(nameBuffer);
    return name.isNotEmpty() ? name : "(unnamed)";
}

juce::String VDX7AudioProcessor::getRomPath() const
{
    std::scoped_lock lock(metadataMutex_);
    return romFile_.existsAsFile() ? romFile_.getFullPathName() : "No ROM selected";
}

juce::String VDX7AudioProcessor::getStatusText() const
{
    if (editQueue_.overflowed())
        return "Edit queue full: further edits blocked; save/export accepted edits, then reload the project";
    if (midiOverloadSnapshot_.load(std::memory_order_relaxed) != 0)
        return "MIDI overload recovered: events dropped and notes released; reduce MIDI/automation density";
    std::scoped_lock lock(metadataMutex_);
    return statusText_;
}

juce::File VDX7AudioProcessor::getSuggestedRomFolder() const
{
#if JUCE_MAC
    return juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library").getChildFile("Application Support")
        .getChildFile("VDX7-JUCE").getChildFile("ROM");
#elif JUCE_WINDOWS
    return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
        .getChildFile("VDX7-JUCE").getChildFile("ROM");
#else
    return juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory)
        .getChildFile("VDX7-JUCE").getChildFile("ROM");
#endif
}

bool VDX7AudioProcessor::autoDetectRom()
{
    juce::Array<juce::File> candidates;

#if JUCE_MAC
    const auto appSupport = juce::File::getSpecialLocation(juce::File::userHomeDirectory)
        .getChildFile("Library").getChildFile("Application Support");
    const auto own = appSupport.getChildFile("VDX7-JUCE").getChildFile("ROM");
    const auto retro = appSupport.getChildFile("discoDSP").getChildFile("Retromulator").getChildFile("ROM");
    candidates.add(own.getChildFile("dx7.bin"));
    candidates.add(own.getChildFile("DX7-V1-8.OBJ"));
    candidates.add(retro.getChildFile("dx7.bin"));
    candidates.add(retro.getChildFile("DX7-V1-8.OBJ"));
#elif JUCE_WINDOWS
    const auto docs = juce::File::getSpecialLocation(juce::File::userDocumentsDirectory);
    const auto own = docs.getChildFile("VDX7-JUCE").getChildFile("ROM");
    const auto retro = docs.getChildFile("discoDSP").getChildFile("Retromulator").getChildFile("ROM");
    candidates.add(own.getChildFile("dx7.bin"));
    candidates.add(own.getChildFile("DX7-V1-8.OBJ"));
    candidates.add(retro.getChildFile("dx7.bin"));
    candidates.add(retro.getChildFile("DX7-V1-8.OBJ"));
#else
    const auto own = getSuggestedRomFolder();
    candidates.add(own.getChildFile("dx7.bin"));
    candidates.add(own.getChildFile("DX7-V1-8.OBJ"));
#endif

    for (const auto& f : candidates)
        if (f.existsAsFile() && loadRomFromFile(f, nullptr))
            return true;

    return false;
}

juce::String VDX7AudioProcessor::bankName(int index)
{
    static constexpr const char* names[] = { "ROM1A", "ROM1B", "ROM2A", "ROM2B", "ROM3A", "ROM3B", "ROM4A", "ROM4B" };
    return (index >= 0 && index < 8) ? names[index] : "Custom";
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new VDX7AudioProcessor();
}
