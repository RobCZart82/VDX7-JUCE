#include "VDX7ImportedBanks.h"
#include "VDX7InitVoice.h"
#include "VDX7Sysex.h"
#include <juce_cryptography/juce_cryptography.h>
#include <algorithm>
#include <iostream>
#include <stdexcept>
#if JUCE_MAC || JUCE_LINUX
#include <sys/stat.h>
#endif

static void require(bool ok, const char* message)
{ if (!ok) throw std::runtime_error(message); }

void testImportedBankState();

static std::vector<uint8_t> bank(int coarse)
{
    auto seed = vdx7InitVoice();
    require(VDX7VoiceData::setOperatorParameter(seed.data(), seed.size(), 0,
        VDX7VoiceData::Parameter::coarse, coarse), "synthetic bank parameter");
    std::vector<uint8_t> packed;
    for (int i = 0; i < 32; ++i) packed.insert(packed.end(), seed.begin(), seed.end());
    return packed;
}

int main()
{
    using namespace VDX7ImportedBanks;
    const auto folder = juce::File::getSpecialLocation(juce::File::tempDirectory)
        .getNonexistentChildFile("vdx7-imported-bank-test", {}, false);
    try
    {
        require(folder.createDirectory().wasOk(), "isolated folder");
        auto put = [&](const juce::String& name, const std::vector<uint8_t>& bytes)
        { require(folder.getChildFile(name).replaceWithData(bytes.data(), bytes.size()), "synthetic file write"); };
        const auto first = bank(2), second = bank(3), factory = bank(4);
        VDX7FactoryBanks::ReferenceHashes references;
        references[0] = juce::SHA256(factory.data(), factory.size()).toHexString();
        const auto message = VDX7Sysex::encode(first);
        put("a-long-prefix-bank-alpha.syx", message);
        auto channel = message; channel[2] = 15;
        put("duplicate.SYX", channel);
        put("a-long-prefix-bank-beta.syx", VDX7Sysex::encode(second));
        put("rom1a.syx", VDX7Sysex::encode(factory));
        put("single.syx", VDX7Sysex::encode(std::vector<uint8_t>(first.begin(), first.begin()+128)));
        auto invalid = message; invalid[100] ^= 1; put("checksum.syx", invalid);
        invalid = message; invalid.pop_back(); put("truncated.syx", invalid);
        invalid = message; invalid[1] = 0x42; put("foreign.syx", invalid);
        put("oversized.syx", std::vector<uint8_t>(4105, 0));
        auto legacy = bank(5); legacy[0] = 127; legacy[16] = 100;
        put("legacy.syx", VDX7Sysex::encode(legacy));
        invalid = message; invalid[6+14] = 100;
        unsigned sum = 0;
        for (auto it = invalid.begin()+6; it != invalid.end()-2; ++it) sum += *it;
        invalid[invalid.size()-2] = uint8_t((128-(sum&127))&127);
        put("invalid-parameter.syx", invalid);
        require(folder.getChildFile("nested").createDirectory().wasOk(), "nested folder");
        require(folder.getChildFile("nested/ignored.syx").replaceWithData(message.data(), message.size()), "nested bank");
        const auto result = scan(folder, {}, references);
        require(result.complete() && result.banks.size() == 3 && result.warnings.size() == 8,
            "valid banks, deduplication, factory separation and bad files");
        require(result.banks[0].fileName == "a-long-prefix-bank-alpha.syx"
            && result.banks[0].packed == first && result.banks[1].packed == second
            && result.banks[2].packed == legacy, "deterministic order and unchanged raw legacy bytes");
        require(result.banks[0].displayName == result.banks[1].displayName
            && result.banks[0].contentId != result.banks[1].contentId
            && result.banks[0].fileName != result.banks[1].fileName,
            "colliding short labels never collapse different content");
        require(result.banks[0].contentId == juce::SHA256(first.data(), first.size()).toHexString(),
            "identity uses payload, independent of MIDI channel/filename");
        juce::MemoryBlock unchanged;
        require(folder.getChildFile("a-long-prefix-bank-alpha.syx").loadFileAsData(unchanged)
            && unchanged == juce::MemoryBlock(message.data(), message.size()), "source not rewritten");
        // Code points keep this filesystem test independent of MSVC's source
        // execution encoding while still testing accented letters and emoji.
        const auto unicode = juce::String::charToString(0x00c1) + "rv"
            + juce::String::charToString(0x00ed) + "zt"
            + juce::String::charToString(0x0171) + "r"
            + juce::String::charToString(0x0151)
            + juce::String::charToString(0x1f3b9) + "abcdefghijkl.syx";
        const auto label = displayName(unicode);
        require(label.length() == 15 && label.endsWith(juce::String::charToString(0x2026))
            && label.contains(juce::String::charToString(0x1f3b9)), "Unicode truncation retains whole code points");
        require(displayName(".syx") == "Unnamed bank" && !displayName("a\nb.syx").containsChar('\n'),
            "empty/control-character label handling");
        const auto unicodeBytes = VDX7Sysex::encode(bank(6));
        put(unicode, unicodeBytes);
        const auto unicodeScan = scan(folder, {}, references);
        require(unicodeScan.complete() && unicodeScan.banks.size() == 4
            && unicodeScan.banks.back().fileName == unicode, "Unicode filesystem path round trip");
        require(scan(folder.getChildFile("missing")).complete()
            && !folder.getChildFile("missing").exists(), "missing first-run folder stays absent");
        require(scan(folder.getChildFile("checksum.syx")).status == ScanStatus::pathError,
            "non-directory cannot replace catalog");
        const auto originalName = folder.getChildFile("a-long-prefix-bank-alpha.syx");
        const auto renamed = folder.getChildFile("rom4b.syx"); // Deliberately misleading factory filename.
        require(originalName.moveFileTo(renamed), "rename isolated synthetic bank");
        const auto renameScan = scan(folder, {}, references);
        const auto moved = std::find_if(renameScan.banks.begin(), renameScan.banks.end(), [&](const auto& entry)
        { return entry.contentId == result.banks[0].contentId; });
        // duplicate.SYX sorts first and wins deduplication after this rename.
        require(renameScan.complete() && moved != renameScan.banks.end()
            && moved->packed == first && renameScan.banks.size() == 4,
            "rename and misleading factory filename cannot change content classification");
        require(renamed.moveFileTo(originalName), "restore isolated fixture name");
        require(scan(folder, [] { return true; }).status == ScanStatus::cancelled,
            "pre-scan cancellation");
        int queries = 0;
        const auto midway = scan(folder, [&] { return ++queries == 20; }, references);
        require(midway.status == ScanStatus::cancelled && midway.banks.empty(),
            "mid-scan cancellation discards partial catalog");
        queries = 0;
        require(scan(folder, [&] { ++queries; return false; }, references).complete(), "count cancellation boundaries");
        const int finalQuery = queries;
        queries = 0;
        const auto late = scan(folder, [&] { return ++queries == finalQuery; }, references);
        require(late.status == ScanStatus::cancelled && late.banks.empty(), "final cancellation cannot publish collected banks");
        const auto links = folder.getChildFile("links");
        require(links.createDirectory().wasOk(), "link-test directory");
        const auto link = links.getChildFile("external.syx");
        if (folder.getChildFile("a-long-prefix-bank-alpha.syx").createSymbolicLink(link, false))
        {
            const auto linked = scan(links);
            require(linked.complete() && linked.banks.empty() && linked.warnings.size() == 1,
                "observed symbolic-link file is not followed");
        }
        else std::cout << "NOT RUN: OS did not permit synthetic symbolic-link creation\n";
        const auto folderLink = folder.getChildFile("folder-link");
        if (links.createSymbolicLink(folderLink, false))
            require(scan(folderLink).status == ScanStatus::pathError, "symbolic-link root is not a regular folder");
        else std::cout << "NOT RUN: OS did not permit synthetic folder-link creation\n";
#if JUCE_MAC || JUCE_LINUX
        const auto fifo = links.getChildFile("pipe.syx");
        require(::mkfifo(fifo.getFullPathName().toRawUTF8(), 0600) == 0, "isolated FIFO creation");
        require(scan(links).banks.empty(), "non-regular files are rejected before blocking file open");
#endif
        const auto limit = folder.getChildFile("limit");
        require(limit.createDirectory().wasOk(), "limit directory");
        for (int i = 0; i < maxFiles; ++i)
            require(limit.getChildFile(juce::String(i)+".syx").replaceWithData(message.data(), message.size()), "limit fixture");
        const auto exactLimit = scan(limit);
        require(exactLimit.complete() && exactLimit.banks.size() == 1
            && exactLimit.warnings.size() == maxFiles - 1, "exact 128-candidate boundary remains usable");
        require(limit.getChildFile("overflow.syx").replaceWithData(message.data(), message.size()), "overflow fixture");
        const auto excessive = scan(limit);
        require(excessive.status == ScanStatus::limitReached && excessive.banks.empty(),
            "129 SysEx candidates cannot publish an arbitrary partial scan");
        const auto clutter = folder.getChildFile("clutter");
        require(clutter.createDirectory().wasOk(), "clutter directory");
        for (int i = 0; i < maxDirectoryEntries; ++i)
            require(clutter.getChildFile(juce::String(i)+".txt").replaceWithText("x"), "clutter fixture");
        require(scan(clutter).complete(), "exact directory-entry boundary remains usable");
        require(clutter.getChildFile("overflow.txt").replaceWithText("x"), "clutter overflow fixture");
        require(scan(clutter).status == ScanStatus::limitReached, "non-SysEx directory enumeration is also bounded");
        require(scan(folder, {}, references).banks.size() == 4 && result.banks.size() == 3,
            "isolated immutable scan results and no recursive scan of limit folders");
        require(defaultFolder() != VDX7FactoryBanks::defaultFolder()
            && defaultFolder().getFileName() == "Imported Banks"
            && defaultFolder().getParentDirectory() == VDX7FactoryBanks::defaultFolder().getParentDirectory(),
            "separate macOS/Windows application-data library path");
        require(folder.deleteRecursively(), "clean isolated fixtures");
        testImportedBankState();
        std::cout << "PASS: bounded imported bank catalog, content identity, Unicode, validation, cancellation and isolation\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        folder.deleteRecursively();
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
