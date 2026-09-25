#include "VDX7BoundedFile.h"
#include <array>
#include <iostream>
#include <stdexcept>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

int main()
{
    try
    {
        constexpr std::size_t limit = 1024;
        juce::TemporaryFile exactFile(".bin");
        std::array<uint8_t, limit> bytes {};
        for (std::size_t i = 0; i < bytes.size(); ++i)
            bytes[i] = static_cast<uint8_t>(i & 0xff);
        {
            juce::FileOutputStream stream(exactFile.getFile());
            require(stream.openedOk() && stream.write(bytes.data(), static_cast<int>(bytes.size())),
                    "write exact-limit input");
        }

        std::vector<uint8_t> result { 9, 8, 7 };
        require(VDX7BoundedFile::read(exactFile.getFile(), limit, result),
                "accept file exactly at limit");
        require(result.size() == limit && result.front() == bytes.front()
                && result.back() == bytes.back(), "exact-limit contents preserved");

        juce::TemporaryFile oversizedFile(".bin");
        std::array<uint8_t, limit + 1> oversized {};
        {
            juce::FileOutputStream stream(oversizedFile.getFile());
            require(stream.openedOk() && stream.write(oversized.data(), static_cast<int>(oversized.size())),
                    "write over-limit input");
        }
        result = { 1, 2, 3 };
        require(!VDX7BoundedFile::read(oversizedFile.getFile(), limit, result),
                "reject file over limit");
        require(result.empty(), "over-limit rejection clears destination");

        result = { 4 };
        require(!VDX7BoundedFile::read(exactFile.getFile().getSiblingFile("missing.bin"), limit, result),
                "reject missing input");
        require(result.empty(), "failed read clears destination");

        std::cout << "PASS: bounded file reader accepts exact limit, rejects oversize and missing files, and clears failed output\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
