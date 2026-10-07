#include "VDX7VoiceData.h"
#include "VDX7Sysex.h"
#include "VDX7ValidationMessage.h"
#include "VDX7InitVoice.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace
{
void require(bool condition, const char* message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

void updateVmemChecksum(std::vector<uint8_t>& message)
{
    unsigned sum = 0;
    for (std::size_t i = 6; i < message.size() - 2; ++i)
        sum += message[i];
    message[message.size() - 2] = static_cast<uint8_t>((128 - (sum & 127)) & 127);
}
}

int main()
{
    auto init = vdx7InitVoice();
    require(VDX7VoiceData::hasValidPackedVoice(init.data(), init.size()), "init voice valid packed data");
    require(VDX7VoiceData::getVoiceParameter(init.data(), init.size(), VDX7VoiceData::VoiceParameter::algorithm) == 32,
        "init uses six parallel carriers");
    for (int op = 0; op < 6; ++op)
        require(VDX7VoiceData::getOperatorParameter(init.data(), init.size(), op, VDX7VoiceData::Parameter::outputLevel)
            == (op == 0 ? 99 : 0), "only OP1 audible in init");
    require(std::string(reinterpret_cast<const char*>(init.data()+118), 10) == "Init Prese", "ten-character stored name without dirty marker");
    const auto untouched = vdx7InitVoice();
    using P = VDX7VoiceData::Parameter;
    using V = VDX7VoiceData::VoiceParameter;
    for (int op = 0; op < 6; ++op)
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            const auto field = static_cast<P>(p);
            int expected = 0;
            if (field == P::rate1 || field == P::rate2 || field == P::rate3
                || field == P::level1 || field == P::level2 || field == P::level3) expected = 99;
            if (field == P::rate4) expected = 80;
            if (field == P::coarse) expected = 1;
            if (field == P::outputLevel && op == 0) expected = 99;
            require(VDX7VoiceData::getOperatorParameter(init.data(), init.size(), op, field) == expected,
                    "all init operator parameters follow the approved seed");
        }
    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        const auto field = static_cast<V>(p);
        const int expected = p <= int(V::pitchRate4) ? 99 : p <= int(V::pitchLevel4) ? 50
                           : field == V::algorithm ? 32 : 0;
        require(VDX7VoiceData::getVoiceParameter(init.data(), init.size(), field) == expected,
                "all init global voice parameters follow the approved seed");
    }
    const auto message = VDX7Sysex::encode(std::vector<uint8_t>(init.begin(), init.end()));
    std::vector<uint8_t> initDecoded;
    require(VDX7Sysex::decode(message, initDecoded)
        && std::equal(initDecoded.begin(), initDecoded.end(), init.begin(), init.end()), "init voice SysEx round trip");
    init[0] = 0;
    require(vdx7InitVoice() == untouched, "init values are independent copies");
    // Synthetic diagnostics only: no firmware or factory-bank bytes.
    std::array<uint8_t, 256> diagnosticVoices {};
    auto diagnostic = VDX7VoiceData::validatePackedVoices(nullptr, 256);
    require(diagnostic.code == VDX7VoiceData::ValidationCode::invalidInput, "diagnostic null input");
    diagnostic = VDX7VoiceData::validatePackedVoices(diagnosticVoices.data(), 255);
    require(diagnostic.code == VDX7VoiceData::ValidationCode::invalidInput, "diagnostic partial input");
    diagnosticVoices[128 + 12] = 15 << 3;
    diagnostic = VDX7VoiceData::validatePackedVoices(diagnosticVoices.data(), 256);
    require(diagnostic.code == VDX7VoiceData::ValidationCode::outOfRange
        && diagnostic.voice == 1 && diagnostic.byte == 12 && diagnostic.value == 15
        && diagnostic.maximum == 14, "diagnostic second voice detune");
    diagnosticVoices[140] = 0;
    require(vdx7ValidationMessage(diagnostic).find("Bank 1, voice 2, byte 12") != std::string::npos
        && vdx7ValidationMessage(diagnostic).find("allowed 0..14") != std::string::npos,
        "human-readable location and range");
    require(vdx7ValidationMessage({}).empty(), "valid input has no diagnostic message");
    diagnosticVoices.back() = 128;
    diagnostic = VDX7VoiceData::validatePackedVoices(diagnosticVoices.data(), 256);
    require(diagnostic.code == VDX7VoiceData::ValidationCode::nonSevenBit
        && diagnostic.voice == 1 && diagnostic.byte == 127, "diagnostic high-bit name");
    diagnosticVoices.back() = 0;
    diagnosticVoices[0] = 127;
    diagnosticVoices[16] = 100;
    require(VDX7VoiceData::validatePackedVoices(diagnosticVoices.data(), 256).ok(),
        "diagnostics preserve legacy exceptions");
    // Compare against the existing independent admission oracle for every
    // single-byte value/position, including reserved bits and legacy values.
    std::array<uint8_t, 128> oracleVoice {};
    for (std::size_t byte = 0; byte < oracleVoice.size(); ++byte)
        for (int value = 0; value < 256; ++value)
        {
            oracleVoice[byte] = static_cast<uint8_t>(value);
            require(VDX7VoiceData::validatePackedVoices(oracleVoice.data(), 128).ok()
                == VDX7VoiceData::hasValidPackedVoice(oracleVoice.data(), 128),
                "diagnostic acceptance matches existing validator");
            oracleVoice[byte] = 0;
        }
    std::vector<uint8_t> factory(256 * VDX7VoiceData::kPackedVoiceSize, 0);
    require(VDX7VoiceData::hasValidPackedVoices(factory.data(), factory.size()), "valid eight-bank image");
    require(!VDX7VoiceData::hasValidPackedVoices(nullptr, factory.size()), "null bank rejected");
    require(!VDX7VoiceData::hasValidPackedVoices(factory.data(), 0), "empty bank rejected");
    require(!VDX7VoiceData::hasValidPackedVoices(factory.data(), factory.size() - 1), "partial voice rejected");
    for (const int slot : {0, 31, 32, 255})
    {
        factory[slot * 128 + 12] = 15 << 3;
        require(!VDX7VoiceData::hasValidPackedVoices(factory.data(), factory.size()), "every bank detune validated");
        const auto detail = VDX7VoiceData::validatePackedVoices(factory.data(), factory.size());
        require(detail.code == VDX7VoiceData::ValidationCode::outOfRange
            && detail.voice == static_cast<std::size_t>(slot) && detail.byte == 12,
            "bank boundary diagnostic location");
        factory[slot * 128 + 12] = 0;
    }
    factory.back() = 128;
    require(!VDX7VoiceData::hasValidPackedVoices(factory.data(), factory.size()), "seven-bit names required");
    factory.back() = 127;
    require(VDX7VoiceData::hasValidPackedVoices(factory.data(), factory.size()), "seven-bit boundary accepted");
    std::array<uint8_t, VDX7VoiceData::kPackedVoiceSize> voice {};

    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
            const int minimum = VDX7VoiceData::parameterMinimum(parameter);
            const int maximum = VDX7VoiceData::parameterMaximum(parameter);
            for (int value = minimum; value <= maximum; ++value)
            {
                require(VDX7VoiceData::setOperatorParameter(
                            voice.data(), voice.size(), op, parameter, value),
                        "parameter write");
                require(VDX7VoiceData::getOperatorParameter(
                            voice.data(), voice.size(), op, parameter) == value,
                        "parameter round trip");
            }
        }
    }

    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::detune, 99);
    require(VDX7VoiceData::getOperatorParameter(
                voice.data(), voice.size(), 0, VDX7VoiceData::Parameter::detune) == 7,
            "upper range clamp");
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::outputLevel, -1);
    require(VDX7VoiceData::getOperatorParameter(
                voice.data(), voice.size(), 0, VDX7VoiceData::Parameter::outputLevel) == 0,
            "lower range clamp");

    // OP1 occupies the last packed operator block, while OP6 occupies the first.
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::outputLevel, 91);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 5,
                                        VDX7VoiceData::Parameter::outputLevel, 27);
    require(voice[5 * 17 + 14] == 91, "OP1 VMEM ordering");
    require(voice[14] == 27, "OP6 VMEM ordering");

    // Editing one packed bit-field must preserve its neighbours.
    voice[5 * 17 + 12] = 0x80;
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::rateScaling, 6);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::detune, -3);
    require((voice[5 * 17 + 12] & 0x80) != 0, "reserved bit preservation");
    require(VDX7VoiceData::getOperatorParameter(
                voice.data(), voice.size(), 0, VDX7VoiceData::Parameter::rateScaling) == 6,
            "rate-scaling preservation");

    voice[5 * 17 + 11] = 0xf0;
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::leftScaleCurve, 1);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::rightScaleCurve, 2);
    require(voice[5 * 17 + 11] == 0xf9, "scaling-curve bit packing");

    voice[5 * 17 + 15] = 0xc0;
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::oscillatorMode, 1);
    VDX7VoiceData::setOperatorParameter(voice.data(), voice.size(), 0,
                                        VDX7VoiceData::Parameter::coarse, 23);
    require(voice[5 * 17 + 15] == 0xef, "mode/coarse bit packing");

    for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
    {
        const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
        const int minimum = VDX7VoiceData::voiceParameterMinimum(parameter);
        const int maximum = VDX7VoiceData::voiceParameterMaximum(parameter);
        for (int value = minimum; value <= maximum; ++value)
        {
            require(VDX7VoiceData::setVoiceParameter(
                        voice.data(), voice.size(), parameter, value),
                    "voice parameter write");
            require(VDX7VoiceData::getVoiceParameter(
                        voice.data(), voice.size(), parameter) == value,
                    "voice parameter round trip");
        }
    }

    voice[111] = 0xf0;
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::feedback, 6);
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::oscillatorKeySync, 1);
    require(voice[111] == 0xfe, "feedback/key-sync bit packing");

    voice[116] = 0x80;
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::lfoKeySync, 1);
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::lfoWaveform, 4);
    VDX7VoiceData::setVoiceParameter(voice.data(), voice.size(),
                                     VDX7VoiceData::VoiceParameter::pitchModSensitivity, 5);
    require(voice[116] == 0xd9, "LFO packed bit preservation");

    // Canonical synthetic voice: no user ROM or factory patch in test fixtures.
    std::vector<uint8_t> synthetic(128, 0);
    for (int op=0;op<6;++op)
        for (int f=0;f<VDX7VoiceData::kParameterCount;++f)
        {
            auto p=static_cast<VDX7VoiceData::Parameter>(f);
            VDX7VoiceData::setOperatorParameter(synthetic.data(),128,op,p,
                VDX7VoiceData::parameterMaximum(p));
        }
    for (int f=0;f<VDX7VoiceData::kVoiceParameterCount;++f)
    {
        auto p=static_cast<VDX7VoiceData::VoiceParameter>(f);
        VDX7VoiceData::setVoiceParameter(synthetic.data(),128,p,
            VDX7VoiceData::voiceParameterMaximum(p));
    }
    for (int i=118;i<128;++i) synthetic[i]='A'+i-118;
    auto single=VDX7Sysex::encode(synthetic);
    require(single.size()==163 && single[3]==0 && single[4]==1 && single[5]==27,"VCED header");
    require(single[6+20]==14 && single[6+17]==1 && single[6+18]==31,"VCED detune/mode/coarse positions");
    require(single[140]==31 && single[150]==48 && single[151]=='A',"VCED algorithm/transpose/name positions");
    std::vector<uint8_t> decoded;
    for (int device=0;device<16;++device)
    {
        single[2]=static_cast<uint8_t>(device);
        require(VDX7Sysex::decode(single,decoded) && decoded==synthetic,"VCED round trip all devices");
    }
    auto broken=single;
    broken[6]^=1;
    require(!VDX7Sysex::decode(broken,decoded) && decoded==synthetic,"bad checksum is nonmutating");
    broken=single; broken[6]|=128;
    require(!VDX7Sysex::decode(broken,decoded),"reject non-7-bit data");
    broken=single; broken.pop_back();
    require(!VDX7Sysex::decode(broken,decoded),"reject truncated message");
    broken=single; broken[2]=16;
    require(!VDX7Sysex::decode(broken,decoded),"reject invalid device");
    broken=single; broken[6]=100;
    unsigned sum=0; for (int i=6;i<161;++i) sum+=broken[i];
    broken[161]=static_cast<uint8_t>((128-(sum&127))&127);
    require(!VDX7Sysex::decode(broken,decoded),"reject out-of-range VCED field even with valid checksum");
    std::vector<uint8_t> bank(4096,0);
    for (int i=0;i<32;++i) std::copy(synthetic.begin(),synthetic.end(),bank.begin()+i*128);
    auto bankMessage=VDX7Sysex::encode(bank);
    require(bankMessage.size()==4104 && bankMessage[3]==9,"VMEM header");
    require(VDX7Sysex::decode(bankMessage,decoded) && decoded==bank,"VMEM full bank round trip");

    // Synthetic reproduction of archived VMEM exceptions; no factory data.
    auto legacyVoice = synthetic;
    for (int block = 0; block < 6; ++block)
    {
        for (int field = 0; field < 8; ++field) legacyVoice[block * 17 + field] = 127;
        legacyVoice[block * 17 + 16] = 100;
    }
    require(VDX7VoiceData::hasValidPackedVoice(legacyVoice.data(), legacyVoice.size()),
            "bounded legacy EG/fine values accepted");
    const auto originalLegacy = legacyVoice;
    for (int op = 0; op < 6; ++op)
        for (const auto parameter : {VDX7VoiceData::Parameter::rate1,
                                     VDX7VoiceData::Parameter::level4,
                                     VDX7VoiceData::Parameter::fine})
            require(VDX7VoiceData::getOperatorParameter(legacyVoice.data(),128,op,parameter) == 99,
                    "legacy import does not expand editor ranges");
    require(legacyVoice == originalLegacy, "passive editor reads preserve raw data");
    auto legacySingle = VDX7Sysex::encode(legacyVoice);
    require(legacySingle.size() == 163 && legacySingle[6] == 127 && legacySingle[25] == 100,
            "VCED export retains raw legacy values instead of UI-clamped values");
    for (int device = 0; device < 16; ++device)
    {
        legacySingle[2] = static_cast<uint8_t>(device);
        require(VDX7Sysex::decode(legacySingle,decoded) && decoded == legacyVoice,
                "legacy VCED lossless round trip on every device channel");
    }
    auto legacyBank = bank;
    for (int slot = 0; slot < 32; ++slot)
        std::copy(legacyVoice.begin(),legacyVoice.end(),legacyBank.begin() + slot * 128);
    const auto legacyMessage = VDX7Sysex::encode(legacyBank);
    require(VDX7Sysex::decode(legacyMessage,decoded) && decoded == legacyBank,
            "legacy values in every bank slot round trip unchanged");
    require(VDX7VoiceData::hasValidPackedVoices(legacyBank.data(),legacyBank.size()),
            "legacy complete image validation agrees with SysEx validation");
    for (int block = 0; block < 6; ++block)
        for (int field : {0,1,2,3,4,5,6,7,16})
            for (int raw = 100; raw <= 127; ++raw)
            {
                auto probe = synthetic;
                probe[block * 17 + field] = static_cast<uint8_t>(raw);
                const bool supported = field == 16 ? raw == 100 : raw == 127;
                require(VDX7VoiceData::hasValidPackedVoice(probe.data(),probe.size()) == supported,
                        "exhaustive legacy boundary: no blanket 100..127 acceptance");
                require(!VDX7Sysex::encode(probe).empty() == supported,
                        "VCED export uses the same bounded storage policy");
            }
    auto editedLegacy = legacyVoice;
    require(VDX7VoiceData::setOperatorParameter(editedLegacy.data(),128,0,
                VDX7VoiceData::Parameter::coarse,3), "edit unrelated canonical field");
    require(editedLegacy[5 * 17] == 127 && editedLegacy[5 * 17 + 16] == 100,
            "unrelated edit preserves imported raw exceptions");
    VDX7VoiceData::setOperatorParameter(editedLegacy.data(),128,0,VDX7VoiceData::Parameter::rate1,127);
    VDX7VoiceData::setOperatorParameter(editedLegacy.data(),128,0,VDX7VoiceData::Parameter::fine,100);
    require(editedLegacy[5 * 17] == 99 && editedLegacy[5 * 17 + 16] == 99,
            "explicit editing still clamps to canonical ranges");
    for (int field : {0,16,118})
    {
        auto highBit = legacyVoice;
        highBit[field] = 128;
        require(!VDX7VoiceData::hasValidPackedVoice(highBit.data(),highBit.size()),
                "legacy policy never admits non-seven-bit voice data");
    }
    auto corruptLegacy = legacyMessage;
    corruptLegacy[corruptLegacy.size()-2] ^= 1;
    require(!VDX7Sysex::decode(corruptLegacy,decoded) && decoded == legacyBank,
            "legacy bank checksum rejection remains nonmutating");
    corruptLegacy = legacyMessage;
    corruptLegacy.pop_back();
    require(!VDX7Sysex::decode(corruptLegacy,decoded) && decoded == legacyBank,
            "truncated legacy bank rejected without replacing output");
    decoded = bank;

    // A valid Yamaha checksum only proves transport integrity; it does not
    // make semantically out-of-range VMEM parameter bytes valid.
    std::vector<std::pair<std::size_t, uint8_t>> invalidVmemFields;
    for (int operatorIndex = 0; operatorIndex < VDX7VoiceData::kOperatorCount; ++operatorIndex)
    {
        const auto opOffset = static_cast<std::size_t>(
            (VDX7VoiceData::kOperatorCount - 1 - operatorIndex)
            * VDX7VoiceData::kPackedOperatorSize);
        for (int field = 0; field < 8; ++field)
            invalidVmemFields.emplace_back(opOffset + static_cast<std::size_t>(field), 100);
        for (int field : {8, 9, 10, 14})
            invalidVmemFields.emplace_back(opOffset + static_cast<std::size_t>(field), 100);
        invalidVmemFields.emplace_back(opOffset + 16, 101); // Only fine 100 is a legacy exception.
        invalidVmemFields.emplace_back(opOffset + 12, 0x78); // Detune nibble 15.
    }
    for (int field = 102; field <= 109; ++field)
        invalidVmemFields.emplace_back(static_cast<std::size_t>(field), 100);
    for (int field = 112; field <= 115; ++field)
        invalidVmemFields.emplace_back(static_cast<std::size_t>(field), 100);
    invalidVmemFields.emplace_back(116, 0x0c); // LFO waveform 6.
    invalidVmemFields.emplace_back(117, 49);   // Transpose beyond +24.
    for (const auto& invalidField : invalidVmemFields)
    {
        auto invalidVmem = bankMessage;
        invalidVmem[6 + invalidField.first] = invalidField.second;
        updateVmemChecksum(invalidVmem);
        require(!VDX7Sysex::decode(invalidVmem, decoded),
                "reject checksum-valid VMEM with an out-of-range semantic field");
        require(decoded == bank, "semantic-invalid VMEM rejection preserves decoded destination");

        auto invalidPackedBank = bank;
        invalidPackedBank[invalidField.first] = invalidField.second;
        require(VDX7Sysex::encode(invalidPackedBank).empty(),
                "reject out-of-range semantic field when exporting packed VMEM");
    }

    auto invalidPackedVoice = synthetic;
    invalidPackedVoice[12] = static_cast<uint8_t>((invalidPackedVoice[12] & 0x87) | 0x78);
    require(!VDX7VoiceData::hasValidOperatorDetune(invalidPackedVoice.data(), invalidPackedVoice.size()),
            "packed voice rejects invalid detune nibble");
    require(VDX7Sysex::encode(invalidPackedVoice).empty(),
            "VCED encoder rejects invalid packed detune");
    for (int voiceIndex = 0; voiceIndex < 32; ++voiceIndex)
        for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
        {
            auto invalidPackedBank = bank;
            const auto offset = static_cast<std::size_t>(
                voiceIndex * VDX7VoiceData::kPackedVoiceSize
                + (VDX7VoiceData::kOperatorCount - 1 - op)
                    * VDX7VoiceData::kPackedOperatorSize + 12);
            invalidPackedBank[offset] = static_cast<uint8_t>((invalidPackedBank[offset] & 0x87) | 0x78);
            require(VDX7Sysex::encode(invalidPackedBank).empty(),
                    "VMEM encoder rejects invalid detune in every voice/operator slot");
        }

    // A bulk-bank checksum does not make every packed voice field valid.
    // Probe the invalid detune code on all six operators and ensure rejection
    // is transactional (the previous decoded output remains unchanged).
    for (int op = 0; op < VDX7VoiceData::kOperatorCount; ++op)
    {
        auto invalidBank = bankMessage;
        const auto packedOperatorOffset =
            (VDX7VoiceData::kOperatorCount - 1 - op) * VDX7VoiceData::kPackedOperatorSize;
        const auto detuneByte = static_cast<std::size_t>(6 + packedOperatorOffset + 12);
        invalidBank[detuneByte] = static_cast<uint8_t>((invalidBank[detuneByte] & 0x87) | 0x78);
        unsigned bankSum = 0;
        for (int i = 6; i < 4102; ++i) bankSum += invalidBank[static_cast<std::size_t>(i)];
        invalidBank[4102] = static_cast<uint8_t>((128 - (bankSum & 127)) & 127);
        require(!VDX7Sysex::decode(invalidBank, decoded),
                "reject checksum-valid VMEM with invalid operator detune");
        require(decoded == bank, "invalid VMEM rejection is nonmutating");
    }

    require(!VDX7Sysex::decode({},decoded) && VDX7Sysex::encode({}).empty(),"reject empty input");
    std::cout << "VDX7 voice-data and SysEx tests passed\n";
    return 0;
}
