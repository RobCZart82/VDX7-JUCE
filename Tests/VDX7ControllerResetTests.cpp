#include "VDX7Engine.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

static void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

struct VDX7RegressionAccess
{
    static void pump(VDX7Engine& engine, int blocks = 16)
    {
        std::array<float, 1024> left {}, right {};
        for (int i = 0; i < blocks; ++i)
            engine.render(left.data(), right.data(), static_cast<int>(left.size()));
    }

    static void cc(VDX7Engine& engine, int controller, int value)
    {
        const uint8_t message[] {0xb0, static_cast<uint8_t>(controller), static_cast<uint8_t>(value)};
        engine.handleMidi(message, 3);
    }

    static void pressure(VDX7Engine& engine, int value)
    {
        const uint8_t message[] {0xd0, static_cast<uint8_t>(value)};
        engine.handleMidi(message, 2);
    }

    static std::pair<bool, bool> pumpAndObserveData(VDX7Engine& engine)
    {
        bool zero = false, later = false;
        for (int sample = 0; sample < 256 * 1024; ++sample)
        {
            float discarded = 0;
            engine.render(&discarded, nullptr, 1);
            if (engine.dx7_.haveMsg
                && engine.dx7_.msg.byte1 == static_cast<uint8_t>(dx7Emu::Message::CtrlID::data))
            {
                zero |= engine.dx7_.msg.byte2 == 0;
                later |= zero && engine.dx7_.msg.byte2 == 39;
            }
        }
        return {zero, later};
    }

    static int ownership(const VDX7Engine& engine, int note, int flag)
    {
        int count = 0;
        for (int voice = 0; voice < 16; ++voice)
            count += engine.dx7_.memory[0x20b0 + 2 * voice] == note
                && (engine.dx7_.memory[0x20b1 + 2 * voice] & flag) != 0;
        return count;
    }

    static std::vector<int> persistent(const VDX7Engine& engine)
    {
        std::vector<int> result {engine.masterTune()};
        for (int field = 0; field < 4; ++field) result.push_back(engine.getPlaySetting(field));
        for (int field = 0; field < 2; ++field) result.push_back(engine.getPitchBendSetting(field));
        for (int source = 0; source < 4; ++source)
            for (int field = 0; field < 4; ++field)
                result.push_back(engine.getControllerSetting(source, field));
        return result;
    }

    static std::unique_ptr<VDX7Engine> fixture(const std::vector<uint8_t>& image)
    {
        auto engine = std::make_unique<VDX7Engine>();
        require(engine->loadRomImage(image.data(), image.size()), "load local controller-reset ROM");
        require(engine->releaseRetirementProfile_, "controller-reset RAM assertions require verified v1.8 profile");
        engine->prepare(48000);
        require(engine->setMasterTune(23), "set persistent tuning fixture");
        require(engine->setPlaySetting(1, 1) && engine->setPlaySetting(2, 1)
                && engine->setPlaySetting(3, 73), "set persistent play fixture");
        require(engine->setPitchBendSetting(0, 7) && engine->setPitchBendSetting(1, 3),
                "set persistent bend fixture");
        for (int source = 0; source < 4; ++source)
        {
            require(engine->setControllerSetting(source, 0, 62 + source), "set controller range fixture");
            for (int field = 1; field < 4; ++field)
                require(engine->setControllerSetting(source, field, (source + field) & 1),
                        "set controller assignment fixture");
        }
        pump(*engine);
        cc(*engine, 64, 127);
        cc(*engine, 65, 127);
        cc(*engine, 2, 113);
        cc(*engine, 4, 114);
        cc(*engine, 6, 115);
        pressure(*engine, 116);
        cc(*engine, 1, 127);
        const uint8_t bend[] {0xe0, 127, 127};
        engine->handleMidi(bend, 3);
        cc(*engine, 11, 0);
        pump(*engine);
        const uint8_t on60[] {0x90, 60, 100}, off60[] {0x80, 60, 0}, on62[] {0x90, 62, 100};
        engine->handleMidi(on60, 3);
        pump(*engine);
        engine->handleMidi(off60, 3);
        pump(*engine);
        engine->handleMidi(on62, 3);
        pump(*engine);
        require(ownership(*engine, 60, 1) == 1 && ownership(*engine, 62, 2) == 1,
                "reset fixture needs one sustained/released and one physically held voice");
        require(engine->appToSynth_.lfq.wasEmpty(), "fixture controller FIFO has drained");
        return engine;
    }

    static void fill(VDX7Engine& engine, int messages)
    {
        const auto overloads = engine.midiOverloadCount();
        for (int i = 0; i < messages; ++i) cc(engine, 2, 127);
        require(engine.midiOverloadCount() == overloads, "capacity fixture contains only accepted controls");
        require(engine.appToSynth_.lfq.wasFull() == (messages == 1024), "controller FIFO capacity fixture");
    }

    static void checkReset(const std::vector<uint8_t>& image, int messages)
    {
        auto engine = fixture(image);
        const auto savedSettings = persistent(*engine);
        std::vector<uint8_t> before, after;
        require(engine->saveRam(before), "save packed voices before reset");
        fill(*engine, messages);
        const auto overloads = engine->midiOverloadCount();
        cc(*engine, 121, 0);
        require(engine->midiOverloadCount() == overloads && engine->midiExpression_ == 1.0f,
                "CC121 resets expression without serial-overflow recovery");
        const auto dataDelivery = pumpAndObserveData(*engine);
        require(engine->appToSynth_.lfq.wasEmpty(), "controller FIFO drains after reset");
        require((engine->dx7_.P_CRT_PEDALS_LCD & 3) == 0 && !engine->sustainDown_,
                "CC121 eventually resets actual firmware sustain and portamento");
        for (const int address : {0x2339, 0x233b, 0x233d})
            require(engine->dx7_.memory[address] == 0, "CC121 eventually resets all non-wheel analog sources");
        require(dataDelivery.first, "CC121 submits data-entry zero to firmware");
        require(engine->dx7_.memory[0x232a] == 128 && engine->dx7_.memory[0x2337] == 0,
                "CC121 eventually resets firmware pitch and modulation");
        require(ownership(*engine, 60, 1) == 0 && ownership(*engine, 62, 2) == 1
                && engine->activeMidiNotes_[62] == 1,
                "CC121 releases sustained voices but preserves physically held notes");
        require(persistent(*engine) == savedSettings, "CC121 preserves all persistent PERFORMANCE settings");
        require(engine->saveRam(after) && std::equal(before.begin(), before.begin() + 4096, after.begin()),
                "CC121 preserves packed voices");
        std::cout << "PASS: CC121 controller FIFO occupancy=" << messages << '\n';
    }

    static void checkLaterInput(const std::vector<uint8_t>& image, bool repeatedReset)
    {
        auto engine = fixture(image);
        const auto savedSettings = persistent(*engine);
        fill(*engine, 1024);
        const auto overloads = engine->midiOverloadCount();
        cc(*engine, 121, 0);
        if (repeatedReset)
        {
            cc(*engine, 64, 127);
            cc(*engine, 65, 127);
            cc(*engine, 2, 97);
            pressure(*engine, 96);
            cc(*engine, 121, 0); // This later reset supersedes the first group's values.
        }
        cc(*engine, 64, 127);
        cc(*engine, 65, 127);
        cc(*engine, 2, 37);
        cc(*engine, 4, 38);
        cc(*engine, 6, 39);
        pressure(*engine, 40);
        const auto dataDelivery = pumpAndObserveData(*engine);
        require(engine->midiOverloadCount() == overloads, "post-reset accepted controllers do not force overflow");
        require((engine->dx7_.P_CRT_PEDALS_LCD & 3) == 3 && engine->sustainDown_,
                "newer sustain and portamento inputs follow the reset OFF edge");
        require(engine->dx7_.memory[0x233b] == 74 && engine->dx7_.memory[0x2339] == 76
                && engine->dx7_.memory[0x233d] == 80,
                "newer breath, foot and aftertouch values follow reset zeros");
        require(dataDelivery.first && dataDelivery.second, "data-entry zero precedes later accepted data input");
        require(ownership(*engine, 60, 1) == 0 && ownership(*engine, 62, 2) == 1,
                "later pedal ON must not erase the reset OFF edge for old sustained notes");
        require(persistent(*engine) == savedSettings, "ordered reset replay preserves persistent settings");
        std::cout << "PASS: CC121 later-input ordering; repeated reset=" << repeatedReset << '\n';
    }

    static void checkInputAtAcknowledgment(const std::vector<uint8_t>& image, bool verifiedProfile,
                                          bool latestSustainOn = true)
    {
        auto engine = fixture(image);
        engine->releaseRetirementProfile_ = verifiedProfile;
        fill(*engine, 1024);
        cc(*engine, 121, 0);
        int samples = 0;
        while (std::any_of(engine->controllerResetZerosPending_.begin(),
                          engine->controllerResetZerosPending_.end(), [](bool value) { return value; })
               || engine->dx7_.haveMsg)
        {
            float discarded = 0;
            engine->render(&discarded, nullptr, 1);
            require(++samples < 256 * 1024, "reset zeros reach acknowledgment stage within bounded render");
        }
        require(engine->controllerResetActive_ && engine->controllerResetAwaitingPedals_,
                "pedal acknowledgment is distinct from zero-message delivery");
        cc(*engine, 64, 127);
        if (!latestSustainOn) cc(*engine, 64, 0);
        cc(*engine, 65, 127);
        cc(*engine, 2, 17);
        cc(*engine, 2, 37); // The newer accepted source value must win.
        pressure(*engine, 40);
        const uint8_t on72[] {0x90, 72, 100}, off72[] {0x80, 72, 0};
        engine->handleMidi(on72, 3);
        pump(*engine);
        require(!engine->controllerResetActive_, "acknowledgment resumes controller input");
        require((engine->dx7_.P_CRT_PEDALS_LCD & 3) == (latestSustainOn ? 3 : 2)
                && engine->sustainDown_ == latestSustainOn && engine->dx7_.memory[0x233b] == 74
                && engine->dx7_.memory[0x233d] == 80,
                "input accepted during pedal acknowledgment retains latest values");
        require(ownership(*engine, 60, 1) == 0 && ownership(*engine, 62, 2) == 1
                && ownership(*engine, 72, 2) == 1,
                "post-acknowledgment ON cannot revive an older sustained voice");
        engine->handleMidi(off72, 3);
        pump(*engine);
        if (latestSustainOn)
        {
            require(ownership(*engine, 72, 1) == 1, "new note follows the newer pedal ON input");
            cc(*engine, 64, 0);
            pump(*engine);
        }
        require(ownership(*engine, 72, 1) == 0 && ownership(*engine, 72, 2) == 0,
                "fresh post-reset note releases with the newer pedal OFF input");
        std::cout << "PASS: CC121 input during acknowledgment; verified profile=" << verifiedProfile
                  << "; latest sustain ON=" << latestSustainOn << '\n';
    }

    static void checkPanicSupersedesPedals(const std::vector<uint8_t>& image, bool hostReset)
    {
        auto engine = fixture(image);
        const auto savedSettings = persistent(*engine);
        fill(*engine, 1024);
        cc(*engine, 121, 0);
        cc(*engine, 64, 127);
        cc(*engine, 65, 127);
        cc(*engine, 2, 37);
        pressure(*engine, 40);
        const auto overloads = engine->midiOverloadCount();
        if (hostReset)
        {
            engine->beginHostReset();
            for (int i = 0; i < 256 && engine->isHostResetInProgress(); ++i)
                engine->advanceHostReset(1024);
            require(!engine->isHostResetInProgress(), "overlapping host reset completes");
        }
        else
        {
            cc(*engine, 7, 100); // Ordinary serial admission sees the saturated FIFO.
            require(engine->midiOverloadCount() == overloads + 1, "overlapping capacity recovery starts");
        }
        pump(*engine, 256);
        require((engine->dx7_.P_CRT_PEDALS_LCD & 3) == 0 && !engine->sustainDown_,
                "later host/panic reset supersedes pending pedal ON values");
        require(engine->dx7_.memory[0x233b] == 74 && engine->dx7_.memory[0x233d] == 80,
                "pending analog reset/input survives an adapter FIFO flush");
        require(ownership(*engine, 60, 1) == 0 && ownership(*engine, 62, 2) == 0
                && !engine->hasHeldMidiNotes(), "overlapping panic leaves no voice ownership");
        require(persistent(*engine) == savedSettings, "overlapping panic preserves persistent settings");
        std::cout << "PASS: CC121 pending input across panic; host reset=" << hostReset << '\n';
    }

    static void checkInFlightInterruption(const std::vector<uint8_t>& image, bool hostReset)
    {
        auto engine = fixture(image);
        const auto savedSettings = persistent(*engine);
        fill(*engine, 1024);
        cc(*engine, 121, 0);
        int samples = 0;
        while (engine->controllerResetZerosPending_[5] || !engine->dx7_.haveMsg
               || !engine->dx7_.byte1Sent
               || engine->dx7_.msg.byte1 != static_cast<uint8_t>(dx7Emu::Message::CtrlID::aftertouch)
               || engine->dx7_.msg.byte2 != 0)
        {
            float discarded = 0;
            engine->render(&discarded, nullptr, 1);
            require(++samples < 256 * 1024, "reach half-delivered mandatory aftertouch reset");
        }
        if (hostReset)
        {
            engine->beginHostReset();
            for (int i = 0; i < 256 && engine->isHostResetInProgress(); ++i)
                engine->advanceHostReset(1024);
            require(!engine->isHostResetInProgress(), "in-flight controller interruption completes host reset");
        }
        else
        {
            const auto overloads = engine->midiOverloadCount();
            for (int i = 0; i < 3000; ++i) cc(*engine, 7, 100);
            require(engine->midiOverloadCount() == overloads + 1,
                    "valid serial backlog triggers in-flight controller recovery");
        }
        pump(*engine, 256);
        require(!engine->controllerResetActive_ && (engine->dx7_.P_CRT_PEDALS_LCD & 3) == 0
                && engine->dx7_.memory[0x2339] == 0 && engine->dx7_.memory[0x233b] == 0
                && engine->dx7_.memory[0x233d] == 0,
                "adapter flush preserves and retries a half-delivered mandatory reset");
        require(!engine->dx7_.haveMsg && !engine->dx7_.byte1Sent,
                "controller interruption leaves no half-completed sub-CPU handshake");
        require(persistent(*engine) == savedSettings, "in-flight interruption preserves persistent settings");
        std::cout << "PASS: CC121 half-delivered analog interruption; host reset=" << hostReset << '\n';
    }

    static void checkPartialLaterInterruption(const std::vector<uint8_t>& image, bool hostReset)
    {
        auto engine = fixture(image);
        const auto savedSettings = persistent(*engine);
        fill(*engine, 1024);
        cc(*engine, 121, 0);
        cc(*engine, 64, 127);
        cc(*engine, 65, 127);
        cc(*engine, 2, 37);
        cc(*engine, 4, 38);
        cc(*engine, 6, 39);
        pressure(*engine, 40);
        int samples = 0;
        while (!engine->controllerResetActive_ || engine->controllerResetAwaitingPedals_
               || engine->controllerAfterReset_[2] >= 0 || engine->controllerAfterReset_[3] < 0
               || !engine->dx7_.haveMsg || !engine->dx7_.byte1Sent
               || engine->dx7_.msg.byte1 != static_cast<uint8_t>(dx7Emu::Message::CtrlID::breath)
               || engine->dx7_.msg.byte2 != 37)
        {
            float discarded = 0;
            engine->render(&discarded, nullptr, 1);
            require(++samples < 256 * 1024, "reach partially delivered post-reset controller values");
        }
        if (hostReset)
        {
            engine->beginHostReset();
            for (int i = 0; i < 256 && engine->isHostResetInProgress(); ++i)
                engine->advanceHostReset(1024);
            require(!engine->isHostResetInProgress(), "partial-post controller host reset completes");
        }
        else
        {
            const auto overloads = engine->midiOverloadCount();
            for (int i = 0; i < 3000; ++i) cc(*engine, 7, 100);
            require(engine->midiOverloadCount() == overloads + 1, "partial-post capacity recovery starts");
        }
        pump(*engine, 256);
        require((engine->dx7_.P_CRT_PEDALS_LCD & 3) == 0 && !engine->sustainDown_,
                "partial-post host/panic reset supersedes older pedal ON values");
        require(engine->dx7_.memory[0x233b] == 74 && engine->dx7_.memory[0x2339] == 76
                && engine->dx7_.memory[0x233d] == 80,
                "reset retry preserves both delivered and undelivered latest analog values");
        require(!engine->controllerResetActive_ && persistent(*engine) == savedSettings,
                "partial-post retry completes without changing persistent settings");
        std::cout << "PASS: CC121 partial-post controller interruption; host reset=" << hostReset << '\n';
    }

    static void checkAllNotesOffSupersedesSustain(const std::vector<uint8_t>& image, bool directRequest)
    {
        auto engine = fixture(image);
        fill(*engine, 1024);
        cc(*engine, 121, 0);
        cc(*engine, 64, 127);
        int samples = 0;
        while (std::any_of(engine->controllerResetZerosPending_.begin(),
                          engine->controllerResetZerosPending_.end(), [](bool pending) { return pending; })
               || engine->dx7_.haveMsg)
        {
            float discarded = 0;
            engine->render(&discarded, nullptr, 1);
            require(++samples < 256 * 1024, "reach pending sustain after reset zero delivery");
        }
        require(engine->controllerAfterReset_[0] == 1, "fixture retains a post-reset pedal ON request");
        if (directRequest) engine->allNotesOff();
        else cc(*engine, 123, 0);
        pump(*engine, 256);
        require((engine->dx7_.P_CRT_PEDALS_LCD & 1) == 0 && !engine->sustainDown_
                && !engine->hasHeldMidiNotes(), "later All Notes Off supersedes an older delayed pedal ON");
        require(ownership(*engine, 60, 1) == 0 && ownership(*engine, 62, 1) == 0
                && ownership(*engine, 62, 2) == 0, "All Notes Off releases held and sustained voices");
        std::cout << "PASS: CC121 delayed pedal followed by All Notes Off; direct request=" << directRequest << '\n';
    }
};

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "usage: vdx7_controller_reset_tests local-ROM-path");
        std::ifstream input(argv[1], std::ios::binary);
        require(input.good(), "open local controller-reset ROM");
        const std::vector<uint8_t> image((std::istreambuf_iterator<char>(input)), {});
        for (const int occupancy : {0, 1018, 1020, 1023, 1024})
            VDX7RegressionAccess::checkReset(image, occupancy);
        VDX7RegressionAccess::checkLaterInput(image, false);
        VDX7RegressionAccess::checkLaterInput(image, true);
        VDX7RegressionAccess::checkInputAtAcknowledgment(image, true);
        VDX7RegressionAccess::checkInputAtAcknowledgment(image, false);
        VDX7RegressionAccess::checkInputAtAcknowledgment(image, true, false);
        VDX7RegressionAccess::checkPanicSupersedesPedals(image, false);
        VDX7RegressionAccess::checkPanicSupersedesPedals(image, true);
        VDX7RegressionAccess::checkInFlightInterruption(image, false);
        VDX7RegressionAccess::checkInFlightInterruption(image, true);
        VDX7RegressionAccess::checkPartialLaterInterruption(image, false);
        VDX7RegressionAccess::checkPartialLaterInterruption(image, true);
        VDX7RegressionAccess::checkAllNotesOffSupersedesSustain(image, false);
        VDX7RegressionAccess::checkAllNotesOffSupersedesSustain(image, true);
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
