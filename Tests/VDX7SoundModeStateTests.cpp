#include "VDX7SoundModeState.h"
#include <juce_audio_processors/juce_audio_processors.h>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace State = VDX7SoundModeState;
using Mode = State::Mode;
static bool permissiveRead = false, shallowWrite = false;
static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

// Deliberately broken adapters are opt-in negative controls, not plugin code.
static bool readUnderTest(const juce::ValueTree& tree, Mode& desired)
{
    if (!permissiveRead) return State::read(tree, desired);
    if (!tree.hasType(State::rootType)) return false;
    desired = static_cast<int>(tree[State::modeProperty]) == 1 ? Mode::clean : Mode::classic;
    return true;
}
static juce::ValueTree write(const juce::ValueTree& tree, Mode desired)
{
    if (!shallowWrite) return State::writeDetached(tree, desired);
    if (!tree.hasType(State::rootType) || !State::isValid(desired)) return {};
    auto shared = tree;
    shared.setProperty(State::versionProperty, 1, nullptr);
    shared.setProperty(State::modeProperty, static_cast<int>(desired), nullptr);
    return shared;
}
static juce::ValueTree pair(const juce::var& version, const juce::var& mode)
{
    juce::ValueTree tree(State::rootType);
    tree.setProperty(State::versionProperty, version, nullptr);
    tree.setProperty(State::modeProperty, mode, nullptr);
    return tree;
}
static void rejectWithoutMutation(const juce::ValueTree& tree)
{
    const auto original = tree.createCopy();
    for (const auto prior : { Mode::classic, Mode::clean })
    {
        auto desired = prior;
        require(!readUnderTest(tree, desired), "invalid pair/root must be rejected, not coerced or defaulted");
        require(desired == prior, "rejection preserves prior desired mode");
    }
    require(tree.isEquivalentTo(original), "reader never mutates project payload");
}
static void testReader()
{
    juce::ValueTree legacy(State::rootType);
    auto desired = Mode::clean;
    require(readUnderTest(legacy, desired) && desired == Mode::classic, "legacy after Clean explicitly recalls Classic");
    for (const auto& version : { juce::var(1), juce::var(juce::int64(1)), juce::var("1") })
        for (const auto& value : { juce::var(0), juce::var(1), juce::var(juce::int64(0)),
                                  juce::var(juce::int64(1)), juce::var("0"), juce::var("1") })
        {
            auto tree = pair(version, value);
            desired = Mode::classic;
            require(readUnderTest(tree, desired) && static_cast<int>(desired) == value.toString().getIntValue(),
                    "exact int/int64/string version and mode are accepted");
        }
    const juce::var badValues[] = {
        true, false, 1.0, 0.0, 1.5, -1, 2, std::numeric_limits<juce::int64>::max(),
        "", "01", "+1", " 1", "1 ", "1\t", "1.0", "1x", "true", "Clean", "00", "-0",
        juce::var(), juce::var(juce::Array<juce::var> { juce::var(1) }),
        juce::var(new juce::DynamicObject()), juce::var(juce::MemoryBlock(4, true))
    };
    for (const auto& bad : badValues)
    {
        rejectWithoutMutation(pair(1, bad));
        rejectWithoutMutation(pair(bad, 1));
    }
    rejectWithoutMutation(pair(0, 1)); // Version 0 is not an older supported schema.
    auto partial = pair(1, 1);
    partial.removeProperty(State::modeProperty, nullptr);
    rejectWithoutMutation(partial);
    partial = pair(1, 1);
    partial.removeProperty(State::versionProperty, nullptr);
    rejectWithoutMutation(partial);
    rejectWithoutMutation({});
    rejectWithoutMutation(juce::ValueTree("PARAMETERS"));
    juce::ValueTree wrongRoot("WrongRoot");
    wrongRoot.addChild(pair(1, 1), -1, nullptr);
    rejectWithoutMutation(wrongRoot); // A valid nested pair is not a root schema.
    legacy.addChild(pair(1, 1), -1, nullptr);
    desired = Mode::clean;
    require(readUnderTest(legacy, desired) && desired == Mode::classic, "nested pair cannot change root mode");
}
static juce::ValueTree syntheticProject()
{
    juce::ValueTree tree(State::rootType);
    tree.setProperty("bank", 8, nullptr);
    tree.setProperty("program", 31, nullptr);
    tree.setProperty("modifiedVoices", juce::int64(0x80000001), nullptr);
    tree.setProperty("initVoices", juce::int64(4), nullptr);
    tree.setProperty("monoNoteZeroCorrection", true, nullptr);
    tree.setProperty("romPath", "missing-private-test-fixture.bin", nullptr);
    tree.setProperty("romIdentity", "synthetic-identity-not-firmware", nullptr);
    juce::MemoryBlock bytes(6144, true);
    for (size_t i = 0; i < bytes.getSize(); ++i)
        static_cast<unsigned char*>(bytes.getData())[i] = static_cast<unsigned char>(i % 128);
    tree.setProperty("ram", bytes.toBase64Encoding(), nullptr);
    for (const auto* name : { "PARAMETERS", "FactoryBanks", "ImportedBanks", "DeferredOperatorEdits" })
    {
        juce::ValueTree section(name);
        section.setProperty("opaqueMetadata", juce::String(name) + ": <&> Unicode \xc3\xa1", nullptr);
        juce::ValueTree child("Entry");
        child.setProperty("packed", bytes.toBase64Encoding(), nullptr);
        child.setProperty("value", 42, nullptr);
        section.addChild(child, -1, nullptr);
        tree.addChild(section, -1, nullptr);
    }
    return tree; // Opaque preservation fixture, NOT a semantically valid whole project.
}
static void testWriter()
{
    const auto captured = syntheticProject();
    const auto original = captured.createCopy();
    auto clean = write(captured, Mode::clean);
    require(clean.isValid() && captured.isEquivalentTo(original), "writer must not mutate captured/shared tree");
    require(clean[State::versionProperty].isInt() && clean[State::modeProperty].isInt(),
            "writer emits integer schema and desired mode");
    auto desired = Mode::classic;
    require(readUnderTest(clean, desired) && desired == Mode::clean, "Clean write/read");
    auto stripped = clean.createCopy();
    stripped.removeProperty(State::versionProperty, nullptr);
    stripped.removeProperty(State::modeProperty, nullptr);
    require(stripped.isEquivalentTo(original), "only the desired pair changes, all other payload preserved");
    require(clean.getNumProperties() == original.getNumProperties() + 2, "no runtime gain/revision/token properties serialized");
    clean.getChild(0).getChild(0).setProperty("value", 99, nullptr);
    require(captured.isEquivalentTo(original), "writer deep-copies children, not just the root handle");
    clean = write(captured, Mode::clean);
    const auto pendingOriginal = clean.createCopy();
    const auto latest = write(clean, Mode::classic);
    require(clean.isEquivalentTo(pendingOriginal) && readUnderTest(latest, desired) && desired == Mode::classic,
            "latest desired replaces stale pair in detached pending copy only");
    auto after = latest.createCopy();
    after.removeProperty(State::versionProperty, nullptr);
    after.removeProperty(State::modeProperty, nullptr);
    require(after.isEquivalentTo(original), "pending payload is preserved");
    require(!write(captured, static_cast<Mode>(255)).isValid() && captured.isEquivalentTo(original),
            "invalid desired cannot mutate/write a snapshot");
    require(!write({}, Mode::classic).isValid(), "invalid root write fails");
    juce::ValueTree wrong("WrongRoot");
    require(!write(wrong, Mode::clean).isValid() && wrong.getNumProperties() == 0, "wrong root write is nonmutating");
}
static juce::ValueTree binaryRoundTrip(const juce::ValueTree& tree)
{
    const auto xml = tree.createXml();
    require(xml != nullptr, "ValueTree XML creation");
    juce::MemoryBlock binary;
    juce::AudioProcessor::copyXmlToBinary(*xml, binary);
    require(binary.getSize() != 0, "actual JUCE project binary is nonempty");
    const auto restoredXml = juce::AudioProcessor::getXmlFromBinary(binary.getData(), static_cast<int>(binary.getSize()));
    require(restoredXml != nullptr, "actual JUCE binary XML decode");
    return juce::ValueTree::fromXml(*restoredXml);
}
static void testRoundTrips()
{
    const auto project = syntheticProject();
    const auto expectedPayload = binaryRoundTrip(project);
    for (const auto mode : { Mode::classic, Mode::clean })
    {
        const auto encoded = write(project, mode);
        for (const auto restored : { juce::ValueTree::fromXml(*encoded.createXml()), binaryRoundTrip(encoded) })
        {
            auto desired = mode == Mode::classic ? Mode::clean : Mode::classic;
            require(readUnderTest(restored, desired) && desired == mode, "actual XML/binary desired-mode round trip");
            require(restored[State::versionProperty].isString() && restored[State::modeProperty].isString(),
                    "XML decodes scalar properties as strings");
            auto payload = restored.createCopy();
            payload.removeProperty(State::versionProperty, nullptr);
            payload.removeProperty(State::modeProperty, nullptr);
            require(payload.isEquivalentTo(expectedPayload), "entire opaque project survives binary/XML normalization");
            juce::MemoryBlock ram;
            require(ram.fromBase64Encoding(payload["ram"].toString()) && ram.getSize() == 6144,
                    "synthetic RAM bytes survive serialization");
            for (size_t i = 0; i < ram.getSize(); ++i)
                require(static_cast<const unsigned char*>(ram.getData())[i] == i % 128, "every synthetic RAM byte preserved");
        }
    }
    auto desired = Mode::clean;
    require(readUnderTest(expectedPayload, desired) && desired == Mode::classic, "legacy binary resets previous Clean to Classic");
    for (const auto& bad : { pair("1x", "1"), pair("1", "01"), pair("2", "1") })
        rejectWithoutMutation(binaryRoundTrip(bad));
    auto partial = pair(1, 1);
    partial.removeProperty(State::modeProperty, nullptr);
    rejectWithoutMutation(binaryRoundTrip(partial));
    partial = pair(1, 1);
    partial.removeProperty(State::versionProperty, nullptr);
    rejectWithoutMutation(binaryRoundTrip(partial));
    // In-memory bool is rejected, but XML loses provenance and may encode it as
    // "1". We intentionally do not claim reconstruction of its original type.
    rejectWithoutMutation(pair(true, 1));
    const auto normalized = binaryRoundTrip(pair(true, 1));
    require(normalized[State::versionProperty].isString() && readUnderTest(normalized, desired),
            "validate decoded scalar text, not nonexistent XML type provenance");
}
int main(int argc, char** argv)
{
    try
    {
        if (argc == 2 && std::string_view(argv[1]) == "--permissive-read-negative-control") permissiveRead = true;
        else if (argc == 2 && std::string_view(argv[1]) == "--shallow-copy-negative-control") shallowWrite = true;
        else require(argc == 1, "unknown test argument");
        testReader();
        testWriter();
        testRoundTrips();
        std::cout << "PASS: JUCE sound-mode detached codec, strict properties and XML/binary round trips\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
