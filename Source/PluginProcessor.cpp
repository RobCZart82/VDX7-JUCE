#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cstring>

namespace
{
constexpr const char* kStateType = "VDX7STATE";
}

VDX7AudioProcessor::VDX7AudioProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true))
{
    autoDetectRom();
}

void VDX7AudioProcessor::prepareToPlay(double sampleRate, int)
{
    std::scoped_lock lock(engineMutex_);
    currentSampleRate_ = sampleRate;
    engine_.prepare(sampleRate);
}

void VDX7AudioProcessor::releaseResources()
{
}

bool VDX7AudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    return out == juce::AudioChannelSet::mono() || out == juce::AudioChannelSet::stereo();
}

void VDX7AudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();

    std::scoped_lock lock(engineMutex_);
    if (!engine_.isLoaded())
    {
        midi.clear();
        return;
    }

    auto* left = buffer.getWritePointer(0);
    auto* right = buffer.getNumChannels() > 1 ? buffer.getWritePointer(1) : left;
    const int total = buffer.getNumSamples();
    int cursor = 0;

    for (const auto metadata : midi)
    {
        const int eventPos = juce::jlimit(0, total, metadata.samplePosition);
        if (eventPos > cursor)
        {
            engine_.render(left + cursor, right + cursor, eventPos - cursor);
            cursor = eventPos;
        }

        const auto message = metadata.getMessage();
        if (message.isSysEx())
        {
            engine_.handleSysex(message.getRawData(), static_cast<std::size_t>(message.getRawDataSize()));
        }
        else
        {
            const auto* raw = message.getRawData();
            engine_.handleMidi(raw, message.getRawDataSize());
        }
    }

    if (cursor < total)
        engine_.render(left + cursor, right + cursor, total - cursor);

    midi.clear();
}

juce::AudioProcessorEditor* VDX7AudioProcessor::createEditor()
{
    return new VDX7AudioProcessorEditor(*this);
}

int VDX7AudioProcessor::getCurrentProgram()
{
    std::scoped_lock lock(engineMutex_);
    return engine_.currentProgram();
}

void VDX7AudioProcessor::setCurrentProgram(int index)
{
    std::scoped_lock lock(engineMutex_);
    if (engine_.isLoaded())
        engine_.selectProgram(index);
}

const juce::String VDX7AudioProcessor::getProgramName(int index)
{
    std::scoped_lock lock(engineMutex_);
    if (!engine_.isLoaded())
        return "Program " + juce::String(index + 1);

    const int old = engine_.currentProgram();
    if (index == old)
        return juce::String(engine_.currentProgramName());

    // Avoid altering the emulated machine just to query a DAW menu name.
    return "Program " + juce::String(index + 1);
}

void VDX7AudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    juce::ValueTree state { juce::Identifier(kStateType) };

    {
        std::scoped_lock lock(engineMutex_);
        state.setProperty("romPath", romFile_.getFullPathName(), nullptr);
        state.setProperty("bank", engine_.currentBank(), nullptr);
        state.setProperty("program", engine_.currentProgram(), nullptr);

        std::vector<uint8_t> ram;
        if (engine_.saveRam(ram) && !ram.empty())
        {
            juce::MemoryBlock block(ram.data(), ram.size());
            state.setProperty("ram", block.toBase64Encoding(), nullptr);
        }
    }

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

    const juce::File savedRom(state.getProperty("romPath").toString());
    if (savedRom.existsAsFile())
        loadRomFromFile(savedRom, nullptr);
    else if (!isRomLoaded())
        autoDetectRom();

    std::scoped_lock lock(engineMutex_);
    if (!engine_.isLoaded())
        return;

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

    if (rom.size() == VDX7Engine::kFirmwareSize)
    {
        // Optional companion file created by the README/package workflow.
        const auto companion = file.getSiblingFile("dx7_factory_voices_32KB.bin");
        if (companion.existsAsFile())
            readFile(companion, voices);
    }

    std::scoped_lock lock(engineMutex_);
    const bool ok = engine_.loadRomImage(rom.data(), rom.size(),
                                          voices.empty() ? nullptr : voices.data(), voices.size());
    if (!ok)
    {
        statusText_ = "Invalid ROM: expected 16 KB firmware or 48 KB combined image";
        if (error != nullptr) *error = statusText_;
        return false;
    }

    engine_.prepare(currentSampleRate_);
    romFile_ = file;
    statusText_ = engine_.hasFactoryVoices()
        ? "DX7 firmware loaded + 8 factory banks"
        : "DX7 firmware loaded (factory voice image not found)";
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

    std::scoped_lock lock(engineMutex_);
    if (!engine_.isLoaded())
    {
        if (error != nullptr) *error = "Load the DX7 firmware first";
        return false;
    }

    if (!engine_.loadSyxBank(data.data(), data.size()))
    {
        if (error != nullptr) *error = "Expected a valid 4104-byte Yamaha DX7 32-voice bulk SysEx bank";
        return false;
    }

    statusText_ = "Loaded SysEx bank: " + file.getFileName();
    return true;
}

bool VDX7AudioProcessor::selectFactoryBank(int bank)
{
    std::scoped_lock lock(engineMutex_);
    const bool ok = engine_.selectFactoryBank(bank);
    if (ok)
        statusText_ = "Factory bank " + bankName(bank);
    return ok;
}

void VDX7AudioProcessor::selectProgramFromUi(int program)
{
    setCurrentProgram(program);
}

bool VDX7AudioProcessor::isRomLoaded() const
{
    std::scoped_lock lock(engineMutex_);
    return engine_.isLoaded();
}

bool VDX7AudioProcessor::hasFactoryVoices() const
{
    std::scoped_lock lock(engineMutex_);
    return engine_.hasFactoryVoices();
}

int VDX7AudioProcessor::getCurrentBank() const
{
    std::scoped_lock lock(engineMutex_);
    return engine_.currentBank();
}

juce::String VDX7AudioProcessor::getCurrentPatchName() const
{
    std::scoped_lock lock(engineMutex_);
    if (!engine_.isLoaded()) return "---";
    const auto name = juce::String(engine_.currentProgramName());
    return name.isNotEmpty() ? name : "(unnamed)";
}

juce::String VDX7AudioProcessor::getRomPath() const
{
    std::scoped_lock lock(engineMutex_);
    return romFile_.existsAsFile() ? romFile_.getFullPathName() : "No ROM selected";
}

juce::String VDX7AudioProcessor::getStatusText() const
{
    std::scoped_lock lock(engineMutex_);
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
