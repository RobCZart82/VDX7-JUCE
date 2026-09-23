// LOCAL EXPERIMENT ONLY. This runner deliberately changes branch decisions in
// a private, verified v1.8 machine. It is NOT linked into the plugin, a CPU fix,
// a ROM patch, or proof of corrected production behavior. Native characterization
// and the failing production acceptance test remain independent and unchanged.
#include "dx7.h"
#include "VDX7VoiceData.h"
#include "VDX7MonoTrace.h"
#include <array>
#include <cmath>
#include <fstream>
#include <iterator>
#include <memory>

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

enum class Candidate { native, twoDecisions, withLookup, withLegato };
static std::array<uint64_t, 6> completeDecisionChanges {};

class Experiment
{
public:
    struct Sound { float peak = 0; double hz = 0; };
    Experiment(const std::vector<uint8_t>& image, Candidate selected, int transpose = 0)
        : machine(toSynth, toGui), candidate(selected)
    {
        require(image.size() == 16384 || image.size() == 49152, "explicit local image size");
        uint64_t fingerprint = UINT64_C(14695981039346656037);
        for (int i = 0; i < 16384; ++i)
            fingerprint = (fingerprint ^ image[i]) * UINT64_C(1099511628211);
        require(fingerprint == UINT64_C(0x20dd25e47a496ba0), "experiment requires verified v1.8 image");
        std::copy_n(image.data(), firmware.size(), firmware.begin());
        require(machine.loadFirmware(image.data(), 16384), "load private firmware");
        VDX7MonoTrace::verifyImage(machine);
        // Additional occupied-slot checks in lookup and legato traversal.
        struct Site { uint16_t pc; const char* op; int operand; };
        for (auto s : { Site{0xd6bb, "bne ", 6}, Site{0xd6ce, "beq ", 6},
                       Site{0xd6e0, "beq ", 6} })
        {
            const auto& inst = machine.instructions[machine.memory[s.pc]];
            require(std::strcmp(inst.op, s.op) == 0 && inst.bytes == 2
                    && std::strcmp(inst.mode, "im") == 0
                    && machine.memory[s.pc + 1] == s.operand, "candidate legato instruction map");
        }
        // Build our own fast-release sine patch; no factory voice data needed.
        using P = VDX7VoiceData::Parameter;
        using V = VDX7VoiceData::VoiceParameter;
        const auto voice = [&](V param, int value)
            { VDX7VoiceData::setVoiceParameter(bank.data(), 128, param, value); };
        voice(V::algorithm, 32);
        voice(V::transpose, transpose);
        for (auto param : {V::pitchRate1, V::pitchRate2, V::pitchRate3, V::pitchRate4}) voice(param, 99);
        for (auto param : {V::pitchLevel1, V::pitchLevel2, V::pitchLevel3, V::pitchLevel4}) voice(param, 50);
        for (int op = 0; op < 6; ++op)
        {
            const auto parameter = [&](P param, int value)
                { VDX7VoiceData::setOperatorParameter(bank.data(), 128, op, param, value); };
            parameter(P::outputLevel, op == 0 ? 99 : 0);
            parameter(P::coarse, 1);
            parameter(P::detune, 0);
            for (auto param : {P::rate1, P::rate2, P::rate3, P::rate4}) parameter(param, 99);
            for (auto param : {P::level1, P::level2, P::level3}) parameter(param, 99);
            parameter(P::level4, 0);
        }
        require(machine.loadVoices(bank.data(), bank.size()), "load synthetic sine bank");
        machine.start();
        for (int i = 0; i < 3000000; ++i) machine.run();
        machine.initControllers();
        machine.sustain(false);
        machine.porta(false);
        machine.setBank(0, false);
        machine.tune(0);
        send(0xc0, 0);
        pump();
        mode(true);
    }

    void mode(bool mono)
    {
        send(0xb0, mono ? 126 : 127, mono ? 1 : 0);
        pump();
        require(machine.memory[0x20a9] == int(mono), "native mode selection");
    }
    void send(int status, int first, int second = -1)
    {
        auto& q = machine.midiSerialRx;
        const int used = (q.writeIdx - q.readIdx) & (q.size - 1);
        require(q.size - 1 - used >= (second >= 0 ? 3 : 2), "experiment serial capacity");
        q.write(uint8_t(status | (machine.getMidiRxChannel() & 15)));
        q.write(uint8_t(first));
        if (second >= 0) q.write(uint8_t(second));
    }
    Sound note(int key, bool on, bool zeroVelocity = false, int repetitions = 1, int samples = 24548)
    {
        for (int i = 0; i < repetitions; ++i)
            send(on || zeroVelocity ? 0x90 : 0x80, key, on ? 100 : 0);
        return pump(samples);
    }
    std::array<int, 3> counts() const
    {
        std::array<int, 3> c {0, 0, machine.memory[0x8e]};
        for (int i = 0; i < 16; ++i)
        {
            c[0] += (machine.memory[0x2168 + i] & 0x80) != 0;
            c[1] += (machine.memory[0x20b1 + 2 * i] & 2) != 0;
        }
        return c;
    }
    int heldKey(int slot) const { return machine.memory[0x20b0 + 2 * slot]; }
    int target() const { return VDX7MonoTrace::word(machine, 0x20d0); }
    int lastKey() const { return machine.memory[0x90]; }
    int activePolyTarget() const
    {
        for (int i = 0; i < 16; ++i)
            if (machine.memory[0x20b1 + 2 * i] & 2)
                return VDX7MonoTrace::word(machine, uint16_t(0x20d0 + 2 * i));
        throw std::runtime_error("no active POLY reference voice");
    }
    void sustain(bool enabled) { machine.sustain(enabled); pump(); }
    void portamento(int value) { send(0xb0, 5, value); machine.porta(value != 0); pump(); }

    Sound pump(int total = 24548)
    {
        Sound sound;
        float previous = 0;
        bool havePrevious = false;
        double firstCrossing = -1, lastCrossing = -1;
        int crossings = 0, frames = 0;
        while (frames < total)
        {
            // Applied BEFORE a branch: HD6303R::step executes that instruction
            // before checking interrupts, so a saved interrupt CCR is not edited.
            decisions();
            machine.run();
            std::array<float, 16> samples {};
            int emitted = 0;
            machine.egs.clock(samples.data(), emitted, machine.inst->cycles * 4);
            require(emitted <= int(samples.size()) && !machine.halt, "native execution/sample bound");
            for (int i = 0; i < emitted; ++i, ++frames)
            {
                require(std::isfinite(samples[i]), "nonfinite experiment audio");
                if (frames < total / 2) continue;
                sound.peak = std::max(sound.peak, std::abs(samples[i]));
                if (havePrevious && previous <= 0 && samples[i] > 0)
                {
                    const double crossing = frames - 1 + (-previous / (samples[i] - previous));
                    if (crossings++ == 0) firstCrossing = crossing;
                    lastCrossing = crossing;
                }
                previous = samples[i];
                havePrevious = true;
            }
        }
        if (crossings > 1) sound.hz = (crossings - 1) * 49096.0 / (lastCrossing - firstCrossing);
        require(std::equal(firmware.begin(), firmware.end(), machine.memory + 0xc000),
                "experiment must not change ROM bytes");
        require(machine.midiSerialRx.empty() && (machine.TRCSR & (1u << dx7Emu::HD6303R::RDRF)) == 0
                && machine.memory[0xee] == machine.memory[0xf0]
                && machine.memory[0xef] == machine.memory[0xf1]
                && machine.memory[0xe7] == 0 && machine.memory[0xf6] == 0,
                "experiment input still pending");
        if (candidate == Candidate::withLegato && machine.memory[0x20a9] == 1)
        {
            const auto c = counts();
            require(c[1] == c[2], "candidate settled active-slot/count consistency");
        }
        return sound;
    }
    uint64_t overrides = 0;
private:
    void decisions()
    {
        if (candidate == Candidate::native || machine.memory[0x20a9] != 1) return;
        const auto x = machine.IX;
        const bool slot = x >= 0x20b0 && x < 0x20d0 && (x & 1) == 0;
        const bool occupied = slot && (machine.memory[x + 1] & 2) != 0;
        bool z = machine.Z;
        int site = -1;
        switch (machine.PC)
        {
            case 0xd593: site = 0; require(slot, "allocation pointer"); z = !occupied; break;
            case 0xd645: site = 1; z = !(machine.B != 0 && occupied
                              && machine.memory[x] == machine.memory[0x81]); break;
            case 0xd6a8:
                site = 2;
                if (candidate >= Candidate::withLookup)
                    z = occupied && machine.A == machine.memory[x];
                break;
            case 0xd6bb: case 0xd6ce: case 0xd6e0:
                site = machine.PC == 0xd6bb ? 3 : machine.PC == 0xd6ce ? 4 : 5;
                if (candidate == Candidate::withLegato)
                { require(slot, "legato pointer"); z = !occupied; }
                break;
            default: return;
        }
        overrides += z != machine.Z;
        if (candidate == Candidate::withLegato) completeDecisionChanges[site] += z != machine.Z;
        machine.Z = z;
    }
    dx7Emu::App_ToSynth queue;
    dx7Emu::NullToGui gui;
    dx7Emu::ToSynth* toSynth = &queue;
    dx7Emu::ToGui* toGui = &gui;
    std::array<uint8_t, 4096> bank {};
    std::array<uint8_t, 16384> firmware {};
    dx7Emu::DX7 machine;
    Candidate candidate;
};

static void incompleteCandidates(const std::vector<uint8_t>& image)
{
    auto two = std::make_unique<Experiment>(image, Candidate::twoDecisions);
    two->note(0, true, false, 2);
    require(two->counts() == std::array<int, 3>{2, 2, 2}, "two-decision allocation experiment");
    two->note(0, false, false, 2);
    // The second lookup stops at the newly cleared first slot, not the remaining
    // occupied zero-key slot. Allocation + early-return changes alone are unsafe.
    require(two->counts() == std::array<int, 3>{0, 1, 1}, "two decisions should expose empty-slot lookup");
    std::cout << "INCOMPLETE candidate: allocation + release only leaves MIDI/held/MONO 0/1/1\n";

    auto lookup = std::make_unique<Experiment>(image, Candidate::withLookup);
    lookup->note(0, true);
    const auto zeroTarget = lookup->target();
    lookup->note(60, true);
    lookup->note(60, false);
    require(lookup->counts() == std::array<int, 3>{1, 1, 1}, "lookup-only candidate ownership");
    require(lookup->target() != zeroTarget, "lookup-only candidate should expose skipped zero-key legato");
    std::cout << "INCOMPLETE candidate: lookup fixed, but 0 On -> 60 On -> 60 Off misses Note 0 return\n";
}

static void candidateAcceptance(const std::vector<uint8_t>& image)
{
    // Independent native POLY pitch/audio reference. The failing MONO cleanup
    // is not allowed to hide discarded/transposed Note 0 in the experiment.
    std::array<int, 5> keys {0, 1, 60, 127, 72};
    std::array<int, 5> targets {};
    std::array<double, 5> frequencies {};
    for (std::size_t i = 0; i < keys.size(); ++i)
    {
        auto reference = std::make_unique<Experiment>(image, Candidate::native);
        reference->mode(false);
        const auto ref = reference->note(keys[i], true, false, 1, 98192);
        targets[i] = reference->activePolyTarget();
        frequencies[i] = ref.hz;
        auto corrected = std::make_unique<Experiment>(image, Candidate::withLegato);
        const auto sound = corrected->note(keys[i], true, false, 1, 98192);
        require(corrected->heldKey(0) == keys[i] && corrected->counts() == std::array<int, 3>{1, 1, 1}
                && sound.peak > 1e-4f && ref.peak > 1e-4f && ref.hz > 1
                && std::abs(sound.hz / ref.hz - 1) < 0.002 && corrected->target() == targets[i],
                "candidate note must sound at original pitch, not be dropped/transposed");
        const auto off = corrected->note(keys[i], false);
        require(corrected->counts() == std::array<int, 3>{} && off.peak < 1e-5f,
                "candidate isolated note release");
        std::cout << "EXPERIMENT pitch=" << keys[i] << " nativePolyHz=" << ref.hz
                  << " candidateMonoHz=" << sound.hz << " releasePeak=" << off.peak << '\n';
        // Enabling the candidate must have no effect on POLY execution.
        auto poly = std::make_unique<Experiment>(image, Candidate::withLegato);
        poly->mode(false);
        const auto polySound = poly->note(keys[i], true, false, 1, 98192);
        require(poly->overrides == 0 && poly->activePolyTarget() == targets[i]
                && polySound.peak == ref.peak && polySound.hz == ref.hz,
                "POLY control must remain native");
        require(poly->note(keys[i], false).peak < 1e-5f, "POLY control release");
    }
    // Native zero/one share the lowest pitch-table entry with neutral transpose.
    // A +12 patch control separates them while the incoming/held MIDI key stays
    // zero/one. Do not silently weaken the oracle to permit a note-one substitute.
    std::array<int, 2> shiftedTargets {};
    std::array<double, 2> shiftedHz {};
    for (int key : {0, 1})
    {
        auto reference = std::make_unique<Experiment>(image, Candidate::native, 12);
        reference->mode(false);
        const auto ref = reference->note(key, true, false, 1, 98192);
        shiftedTargets[key] = reference->activePolyTarget();
        shiftedHz[key] = ref.hz;
        auto corrected = std::make_unique<Experiment>(image, Candidate::withLegato, 12);
        const auto sound = corrected->note(key, true, false, 1, 98192);
        require(corrected->heldKey(0) == key && corrected->target() == shiftedTargets[key]
                && sound.peak > 1e-4f && ref.hz > 1 && std::abs(sound.hz / ref.hz - 1) < 0.002,
                "candidate shifted-patch pitch reference");
        std::cout << "EXPERIMENT transpose=12 key=" << key << " referenceHz=" << ref.hz
                  << " candidateHz=" << sound.hz << '\n';
        require(corrected->note(key, false).peak < 1e-5f
                && corrected->counts() == std::array<int, 3>{}, "shifted-patch control release");
    }
    require(shiftedTargets[0] != shiftedTargets[1] && shiftedHz[1] > shiftedHz[0] * 1.04,
            "Note 0/1 oracle must discriminate pitch substitution");

    for (int repeats : {1, 2, 16, 17, 32})
        for (bool sequential : {false, true})
            for (bool velocityZero : {false, true})
            {
                auto e = std::make_unique<Experiment>(image, Candidate::withLegato);
                if (sequential)
                    for (int i = 0; i < repeats; ++i)
                    {
                        require(e->note(0, true).peak > 1e-4f && e->heldKey(0) == 0
                                && e->target() == targets[0]
                                && e->counts() == std::array<int, 3>{1, 1, 1},
                                "every sequential zero must really allocate/play");
                        require(e->note(0, false, velocityZero).peak < 1e-5f
                                && e->counts() == std::array<int, 3>{},
                                "every sequential zero must release");
                    }
                else
                {
                    const auto on = e->note(0, true, false, repeats);
                    const auto allocated = e->counts();
                    require(allocated[1] == std::min(repeats, 16) && on.peak > 1e-4f
                            && e->target() == targets[0], "stacked zeros must fill available slots");
                    for (int slot = 0; slot < allocated[1]; ++slot)
                        require(e->heldKey(slot) == 0, "stacked zero identity");
                    require(e->note(0, false, velocityZero, repeats).peak < 1e-5f, "stacked zero silence");
                }
                require(e->counts() == std::array<int, 3>{}, "candidate repeated zero cleanup");
                const auto on = e->note(72, true, false, 1, 98192);
                require(e->heldKey(0) == 72 && e->counts() == std::array<int, 3>{1, 1, 1}
                        && e->target() == targets[4] && on.peak > 1e-4f
                        && std::abs(on.hz / frequencies[4] - 1) < 0.002,
                        "candidate subsequent 72 must genuinely sound");
                const auto off = e->note(72, false, velocityZero);
                require(e->counts() == std::array<int, 3>{} && off.peak < 1e-5f,
                        "candidate subsequent 72 release");
                std::cout << "EXPERIMENT repeats=" << repeats << " sequential=" << sequential
                          << " zeroVelocity=" << velocityZero << " after72Hz=" << on.hz << '\n';
            }

    // Exercise holes and all first/minimum/maximum legato traversals, in both
    // priority directions. No recovery/reset between any of these events.
    std::array<int, 3> order {0, 60, 127};
    do
    {
        for (int firstOff : order)
            for (bool reverseRemainder : {false, true})
            {
                auto e = std::make_unique<Experiment>(image, Candidate::withLegato);
                for (int key : order) e->note(key, true);
                const bool upward = order[1] > order[0];
                e->note(firstOff, false);
                std::vector<int> remaining;
                for (int key : order) if (key != firstOff) remaining.push_back(key);
                const int expected = upward ? std::max(remaining[0], remaining[1])
                                            : std::min(remaining[0], remaining[1]);
                const auto expectedTarget = [&](int key)
                    { return targets[key == 0 ? 0 : key == 60 ? 2 : 3]; };
                require(e->counts() == std::array<int, 3>{2, 2, 2} && e->lastKey() == expected
                        && e->target() == expectedTarget(expected), "candidate three-key priority");
                if (reverseRemainder) std::reverse(remaining.begin(), remaining.end());
                e->note(remaining[0], false);
                require(e->counts() == std::array<int, 3>{1, 1, 1}
                        && e->lastKey() == remaining[1] && e->target() == expectedTarget(remaining[1]),
                        "candidate three-key final held pitch");
                require(e->note(remaining[1], false).peak < 1e-5f
                        && e->counts() == std::array<int, 3>{}, "candidate three-key release");
                require(e->note(0, false).peak < 1e-5f && e->counts() == std::array<int, 3>{},
                        "unmatched zero Off must not invent/decrement ownership");
            }
    } while (std::next_permutation(order.begin(), order.end()));

    for (bool zeroFirst : {false, true})
        for (bool releaseZeroFirst : {false, true})
            for (int porta : {0, 64})
                for (bool sustain : {false, true})
                {
                    auto e = std::make_unique<Experiment>(image, Candidate::withLegato);
                    e->portamento(porta);
                    e->sustain(sustain);
                    e->note(zeroFirst ? 0 : 60, true);
                    e->note(zeroFirst ? 60 : 0, true);
                    e->note(releaseZeroFirst ? 0 : 60, false);
                    const int remaining = releaseZeroFirst ? 60 : 0;
                    require(e->counts() == std::array<int, 3>{1, 1, 1}
                            && e->lastKey() == remaining && e->target() == targets[remaining == 0 ? 0 : 2],
                            "candidate mixed legato must return to remaining pitch including zero");
                    const auto held = e->note(remaining, false);
                    require(e->counts() == std::array<int, 3>{}, "candidate mixed legato count cleanup");
                    require(sustain ? held.peak > 1e-4f : held.peak < 1e-5f,
                            "candidate must distinguish intended sustain from stuck voice");
                    e->sustain(false);
                    require(e->pump().peak < 1e-5f, "candidate sustain release");
                }
    for (auto changes : completeDecisionChanges)
        require(changes > 0, "each candidate decision needs exercised evidence");
    std::cout << "EXPERIMENT decision changes (allocation/release/lookup/first/highest/lowest):";
    for (auto changes : completeDecisionChanges) std::cout << ' ' << changes;
    std::cout << "\nPASS: local branch-decision EXPERIMENT only; production MONO acceptance remains OPEN\n";
}

int main(int argc, char** argv)
{
    try
    {
        require(argc == 2, "supply explicit private v1.8 ROM path");
        std::ifstream file(argv[1], std::ios::binary);
        const std::vector<uint8_t> image((std::istreambuf_iterator<char>(file)), {});
        // Check rejection controls without executing altered/unknown firmware.
        require(image.size() == 16384 || image.size() == 49152, "explicit private image size");
        for (int changed : {0, 8192, 16383})
        {
            auto unknown = image;
            unknown[changed] ^= 1;
            bool rejected = false;
            try { auto invalid = std::make_unique<Experiment>(unknown, Candidate::withLegato); }
            catch (const std::runtime_error& error)
            { rejected = std::string(error.what()) == "experiment requires verified v1.8 image"; }
            require(rejected, "changed-image candidate must fail before boot");
        }
        incompleteCandidates(image);
        candidateAcceptance(image);
        return 0;
    }
    catch (const std::exception& error)
    { std::cerr << "FAIL: " << error.what() << '\n'; return 1; }
}
