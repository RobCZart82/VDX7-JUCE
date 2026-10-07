#include "VDX7ImportedBanks.h"
#include "VDX7BoundedFile.h"
#include "VDX7Sysex.h"
#include <juce_cryptography/juce_cryptography.h>
#include <algorithm>
#include <filesystem>

namespace VDX7ImportedBanks
{
static std::filesystem::path nativePath(const juce::File& file)
{
    const auto utf8 = file.getFullPathName().toStdString();
    return std::filesystem::path(std::u8string(utf8.begin(), utf8.end()));
}

static juce::String readable(const juce::String& text)
{
    juce::String result;
    for (auto character : text)
        result += (character < 32 || character == 127) ? juce::juce_wchar(' ') : character;
    return result;
}

juce::File defaultFolder()
{
    return VDX7FactoryBanks::defaultFolder().getSiblingFile("Imported Banks");
}

juce::String displayName(const juce::String& fileName)
{
    auto name = readable(fileName.upToLastOccurrenceOf(".", false, false)).trim();
    if (name.isEmpty()) name = "Unnamed bank";
    return name.length() > maxDisplayCharacters
        ? name.substring(0, maxDisplayCharacters - 1) + juce::String::charToString(0x2026) : name;
}

ScanResult scan(const juce::File& folder, const std::function<bool()>& shouldCancel,
                const VDX7FactoryBanks::ReferenceHashes& hashes)
{
    ScanResult result;
    const auto abort = [&](ScanStatus status, const char* warning)
    {
        result.status = status;
        result.banks.clear();
        result.warnings.add(warning);
        return result;
    };
    const auto cancelled = [&] { return shouldCancel && shouldCancel(); };
    if (cancelled()) return abort(ScanStatus::cancelled, "Imported bank scan cancelled.");
    // std::filesystem exposes enumeration errors instead of treating an unreadable
    // folder as an empty successful refresh. UTF-8 conversion also works on Windows.
    const auto path = nativePath(folder);
    std::error_code error;
    const auto root = std::filesystem::symlink_status(path, error);
    if (error == std::errc::no_such_file_or_directory
        || (!error && root.type() == std::filesystem::file_type::not_found))
        return result; // Supported first run, no folder is created.
    if (error || !std::filesystem::is_directory(root))
        return abort(ScanStatus::pathError, "Imported bank path must be a regular folder.");
    std::vector<juce::File> files;
    std::filesystem::directory_iterator iterator(path, error), end;
    if (error) return abort(ScanStatus::pathError, "Imported bank folder cannot be read.");
    int entries = 0;
    while (iterator != end)
    {
        if (cancelled()) return abort(ScanStatus::cancelled, "Imported bank scan cancelled.");
        if (++entries > maxDirectoryEntries)
            return abort(ScanStatus::limitReached, "Imported bank folder exceeds 512 directory entries.");
        std::u8string utf8;
        try { utf8 = iterator->path().u8string(); }
        catch (const std::filesystem::filesystem_error&)
        { return abort(ScanStatus::pathError, "Imported bank filename cannot be decoded."); }
        const juce::File file(juce::String::fromUTF8(reinterpret_cast<const char*>(utf8.data()), int(utf8.size())));
        const auto status = iterator->symlink_status(error);
        if (error) return abort(ScanStatus::pathError, "Imported bank entry cannot be inspected.");
        if (file.hasFileExtension("syx") && !std::filesystem::is_directory(status))
        {
            if (files.size() >= maxFiles)
                return abort(ScanStatus::limitReached, "Imported bank folder exceeds 128 SysEx files.");
            files.push_back(file); // Even rejected/symlink candidates count towards the limit.
        }
        iterator.increment(error);
        if (error) return abort(ScanStatus::pathError, "Imported bank enumeration failed.");
    }
    std::sort(files.begin(), files.end(), [](const auto& a, const auto& b)
    {
        const auto first = a.getFileName(), second = b.getFileName();
        const int insensitive = first.compareIgnoreCase(second);
        return insensitive != 0 ? insensitive < 0 : first.compare(second) < 0;
    });
    for (const auto& file : files)
    {
        if (cancelled()) return abort(ScanStatus::cancelled, "Imported bank scan cancelled.");
        const auto name = file.getFileName(), label = readable(name);
        std::vector<uint8_t> message, packed;
        const auto kind = std::filesystem::symlink_status(nativePath(file), error);
        if (error || !std::filesystem::is_regular_file(kind)
            || !VDX7BoundedFile::read(file, VDX7Sysex::kBankMessageSize, message)
            || message.size() != VDX7Sysex::kBankMessageSize || !VDX7Sysex::decode(message, packed))
        {
            result.warnings.add(label + ": invalid or unreadable 32-voice SysEx bank.");
            continue;
        }
        if (VDX7FactoryBanks::identify(packed, hashes) >= 0)
        {
            result.warnings.add(label + ": recognised factory bank; use the Factory Banks folder.");
            continue;
        }
        const auto id = juce::SHA256(packed.data(), packed.size()).toHexString();
        const auto duplicate = std::find_if(result.banks.begin(), result.banks.end(), [&](const auto& bank)
        { return bank.contentId == id && bank.packed == packed; });
        if (duplicate != result.banks.end())
        {
            result.warnings.add(label + ": duplicate of " + readable(duplicate->fileName) + ".");
            continue;
        }
        result.banks.push_back({id, name, displayName(name), std::move(packed)});
    }
    if (cancelled()) return abort(ScanStatus::cancelled, "Imported bank scan cancelled.");
    return result;
}
}
