#include "VDX7StatusPresentation.h"

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
        const juce::String editOverflow = "Edit queue full: recovery needed";
        const juce::String midiOverload = "MIDI overload recovered: reduce density";
        const juce::String ordinary = "ROM loaded";
        const juce::String dirty = "Bank has unexported edits\nSAVE AS... to export";

        require(VDX7StatusPresentation::choose(editOverflow, true, ordinary) == editOverflow,
                "dirty edits must not hide edit-queue overflow");
        require(VDX7StatusPresentation::choose(midiOverload, true, ordinary) == midiOverload,
                "dirty edits must not hide MIDI-overload recovery");
        require(VDX7StatusPresentation::choose(editOverflow, false, ordinary) == editOverflow,
                "clean bank must still show critical overflow");
        require(VDX7StatusPresentation::choose({}, true, ordinary) == dirty,
                "dirty bank is shown when no critical status exists");
        require(VDX7StatusPresentation::choose({}, false, ordinary) == ordinary,
                "ordinary status is shown when no critical status or dirty bank exists");

        std::cout << "PASS: critical status always outranks dirty and ordinary status\n";
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
