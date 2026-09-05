#include "PluginEditor.h"
#include "VDX7MechanicalDrawing.h"
#include <iostream>
#include <cmath>
#include <stdexcept>

static void require(bool ok, const char* message)
{
    if (!ok) throw std::runtime_error(message);
}

static float value(VDX7AudioProcessor& p, const juce::String& id)
{
    return p.parameters().getRawParameterValue(id)->load();
}

static void set(VDX7AudioProcessor& p, const juce::String& id, float v)
{
    auto* parameter = p.parameters().getParameter(id);
    require(parameter != nullptr, "missing host parameter");
    parameter->setValueNotifyingHost(parameter->convertTo0to1(v));
}

static juce::MemoryBlock save(VDX7AudioProcessor& p)
{
    juce::MemoryBlock data;
    p.getStateInformation(data);
    return data;
}

static juce::MemoryBlock ram(const juce::MemoryBlock& state)
{
    auto xml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
    require(xml != nullptr, "state XML");
    juce::MemoryBlock result;
    require(result.fromBase64Encoding(juce::ValueTree::fromXml(*xml)["ram"].toString()),
            "state RAM decoding");
    return result;
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        VDX7AudioProcessor original;
        require(original.getParameters().size() == 148, "148 host parameters");
        for (int op = 0; op < 6; ++op)
            for (int p = 0; p < 15; ++p)
            {
                const auto id = VDX7ParameterIDs::operatorParameter(
                    op, static_cast<VDX7VoiceData::Parameter>(p));
                require(original.parameters().getParameter(id)->getParameterIndex()
                            == 3 + op * 15 + p, "legacy parameter index changed");
            }
        if (!original.isRomLoaded())
        {
            std::cout << "SKIP: user ROM required for processor integration tests\n";
            return 77;
        }

        original.prepareToPlay(48000, 256);
        // Check patch/host coherence without creating an editor.
        original.selectProgramFromUi(3);
        auto state = save(original);
        auto bank = ram(state);
        require(original.getCurrentProgram() == 3, "pending program saved");
        const auto* voice = static_cast<const uint8_t*>(bank.getData()) + 3 * 128;
        for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
            require(juce::roundToInt(value(original, VDX7ParameterIDs::voiceParameter(parameter)))
                        == VDX7VoiceData::getVoiceParameter(voice, 128, parameter),
                    "program sync without editor");
        }

        // Exercise all packed fields, then save before running another block.
        for (int op = 0; op < 6; ++op)
            for (int p = 0; p < VDX7VoiceData::kParameterCount; ++p)
            {
                const auto parameter = static_cast<VDX7VoiceData::Parameter>(p);
                const int lo = VDX7VoiceData::parameterMinimum(parameter);
                const int hi = VDX7VoiceData::parameterMaximum(parameter);
                set(original, VDX7ParameterIDs::operatorParameter(op, parameter),
                    float(lo + (op * 13 + p * 7) % (hi - lo + 1)));
            }
        for (int p = 0; p < VDX7VoiceData::kVoiceParameterCount; ++p)
        {
            const auto parameter = static_cast<VDX7VoiceData::VoiceParameter>(p);
            const int lo = VDX7VoiceData::voiceParameterMinimum(parameter);
            const int hi = VDX7VoiceData::voiceParameterMaximum(parameter);
            set(original, VDX7ParameterIDs::voiceParameter(parameter),
                float(lo + (p * 11) % (hi - lo + 1)));
        }
        state = save(original);
        VDX7AudioProcessor restored;
        restored.setStateInformation(state.getData(), int(state.getSize()));
        require(restored.getCurrentProgram() == 3, "program restore");
        require(ram(state) == ram(save(restored)), "RAM round trip before rendering");
        for (auto* parameter : original.getParameters())
        {
            auto* ranged = dynamic_cast<juce::RangedAudioParameter*>(parameter);
            require(ranged != nullptr, "ranged parameter");
            require(std::abs(ranged->getValue()
                     - restored.parameters().getParameter(ranged->paramID)->getValue()) < 0.0001f,
                    "parameter state round trip");
        }

        // Reloading the UI must not discard a fresh edit while transport is stopped.
        require(restored.hasUnexportedEdits(), "dirty state survives project restore");
        require(restored.renameVoice("VDX TEST"), "rename voice");
        require(restored.getCurrentPatchName() == "VDX TEST", "name snapshot");
        require(!restored.renameVoice("TOO LONG NAME") && !restored.renameVoice(""), "invalid name rejected");
        auto beforeCopy = ram(save(restored));
        require(!restored.pasteOperator(1), "paste without clipboard rejected");
        require(restored.copyOperator(0) && restored.pasteOperator(5), "copy OP1 to OP6");
        auto afterCopy = ram(save(restored));
        const auto* a = static_cast<const uint8_t*>(beforeCopy.getData());
        const auto* b = static_cast<const uint8_t*>(afterCopy.getData());
        for (size_t i=0;i<afterCopy.getSize();++i)
            require(b[i] == (i>=3*128 && i<3*128+17 ? a[i+5*17] : a[i]),
                    "operator paste preserves other fields");
        juce::TemporaryFile singleFile(".syx"), bankFile(".syx"), badFile(".syx");
        juce::String error;
        require(restored.exportSyx(singleFile.getFile(),false,error), "single voice export");
        require(singleFile.getFile().getSize()==163 && !restored.hasUnexportedEdits(), "single export acknowledges voice");
        require(restored.renameVoice("EDIT AGAIN"), "second edit");
        restored.selectProgramFromUi(4);
        save(restored);
        require(!restored.isCurrentVoiceModified() && restored.hasUnexportedEdits(), "other voice remains dirty");
        auto beforeImport=ram(save(restored));
        require(restored.loadSyxFromFile(singleFile.getFile(),&error), "single voice import");
        auto afterImport=ram(save(restored));
        a=static_cast<const uint8_t*>(beforeImport.getData());
        b=static_cast<const uint8_t*>(afterImport.getData());
        for (size_t i=0;i<afterImport.getSize();++i)
            if (i<4*128 || i>=5*128) require(a[i]==b[i], "single import preserves other slots and system RAM");
        require(restored.getCurrentPatchName()=="VDX TEST", "single voice name imported");
        require(restored.hasUnexportedEdits(), "single import preserves other dirty flags");
        require(restored.exportSyx(bankFile.getFile(),true,error), "bank export");
        require(bankFile.getFile().getSize()==4104 && !restored.hasUnexportedEdits(), "bank export acknowledges all voices");
        auto exportedBank=ram(save(restored));
        require(restored.renameVoice("TEMP"), "edit before bank restore");
        require(restored.loadSyxFromFile(bankFile.getFile(),&error), "bank import");
        auto importedBank=ram(save(restored));
        require(std::memcmp(exportedBank.getData(),importedBank.getData(),4096)==0,"bank file round trip");
        require(badFile.getFile().replaceWithText("not sysex"), "invalid fixture write");
        require(!restored.loadSyxFromFile(badFile.getFile(),&error), "invalid import rejected");
        require(ram(save(restored))==importedBank,"invalid import preserves RAM");
        require(restored.renameVoice("UNSAVED"), "edit before failed export");
        require(!restored.exportSyx(badFile.getFile().getChildFile("missing/out.syx"),true,error)
                && restored.hasUnexportedEdits(), "failed export retains dirty flag");

        const auto speed = VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::lfoSpeed);
        set(restored, speed, 47);
        {
            std::unique_ptr<juce::AudioProcessorEditor> editor(restored.createEditor());
            require(editor->getWidth() == 1200 && editor->getHeight() == 925,
                    "Default editor includes extra chassis height");
            require(juce::roundToInt(value(restored, speed)) == 47, "editor erased pending automation");
            int switchIndex = 0;
            for (auto* child : editor->getChildren())
                if (auto* slider = dynamic_cast<juce::Slider*>(child))
                    if (bool(slider->getProperties().getWithDefault("vdx7SyncSwitch", false)))
                    {
                        require(switchIndex < 2, "Unexpected sync switch");
                        const auto id = VDX7ParameterIDs::voiceParameter(switchIndex++ == 0
                            ? VDX7VoiceData::VoiceParameter::oscillatorKeySync
                            : VDX7VoiceData::VoiceParameter::lfoKeySync);
                        const auto original = value(restored, id);
                        require(slider->getSliderStyle() == juce::Slider::LinearVertical,
                                "Sync switch must move vertically");
                        require(slider->getMinimum() == 0 && slider->getMaximum() == 1
                                && slider->getInterval() == 1, "Sync has exactly two positions");
                        for (int position : { 0, 1 })
                        {
                            set(restored, id, float(position));
                            require(slider->getValue() == position, "Automation updates sync switch");
                            slider->setValue(1 - position, juce::sendNotificationSync);
                            require(value(restored, id) == 1 - position, "Switch updates existing parameter");
                        }
                        set(restored, id, original);
                    }
            require(switchIndex == 2, "Two sync switches present");
            juce::Slider* modeSwitch = nullptr;
            for (auto* child : editor->getChildren())
                if (auto* slider = dynamic_cast<juce::Slider*>(child))
                    if (bool(slider->getProperties().getWithDefault("vdx7ModeSwitch", false)))
                        modeSwitch = slider;
            require(modeSwitch != nullptr, "Horizontal oscillator switch exists");
            require(modeSwitch->getSliderStyle() == juce::Slider::LinearHorizontal
                    && modeSwitch->getInterval() == 1, "Oscillator switch is binary/horizontal");
            const auto modeID = VDX7ParameterIDs::operatorParameter(
                0, VDX7VoiceData::Parameter::oscillatorMode);
            const auto originalMode = value(restored, modeID);
            for (int position : { 0, 1 })
            {
                set(restored, modeID, float(position));
                require(modeSwitch->getValue() == position, "Mode automation updates switch");
                modeSwitch->setValue(1 - position, juce::sendNotificationSync);
                require(value(restored, modeID) == 1 - position, "Switch updates oscillator mode");
            }
            set(restored, modeID, originalMode);
            for (const int width : { 960, 1440, 1600 })
            {
                editor->setSize(width, juce::roundToInt(width * 1110.0 / 1440.0));
                int lcdSelectors = 0;
                const float scale = float(width) / 1440.0f;
                const juce::Rectangle<float> lcdArea(382 * scale, 190 * scale,
                                                      550 * scale, 86 * scale);
                for (auto* child : editor->getChildren())
                    if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                        if (bool(box->getProperties().getWithDefault("vdx7LcdCombo", false)))
                        {
                            ++lcdSelectors;
                            require(lcdArea.contains(box->getBounds().toFloat()),
                                    "LCD selector outside display");
                            require(box->findColour(juce::ComboBox::backgroundColourId).isTransparent(),
                                    "LCD selector obscures display background");
                        }
                require(lcdSelectors == 2, "Exactly two integrated LCD selectors");
                VDX7AlgorithmView* diagram=nullptr;
                for (auto* child:editor->getChildren())
                    if (auto* view=dynamic_cast<VDX7AlgorithmView*>(child)) diagram=view;
                require(diagram!=nullptr,"Algorithm view exists");
                const auto algoID=VDX7ParameterIDs::voiceParameter(VDX7VoiceData::VoiceParameter::algorithm);
                for (int algorithm=1;algorithm<=32;++algorithm)
                {
                    set(restored,algoID,float(algorithm));
                    require(diagram->algorithm()==algorithm,"Algorithm parameter updates diagram");
                    auto beforeSelection=ram(save(restored));
                    for (int op=0;op<6;++op)
                    {
                        const auto bounds=diagram->nodeBounds(op);
                        require(diagram->getLocalBounds().toFloat().contains(bounds),"Node fits diagram");
                        require(diagram->operatorAt(bounds.getCentre())==op,"Node hit test");
                        diagram->onOperatorSelected(op);
                        require(modeSwitch->getValue() == value(restored,
                            VDX7ParameterIDs::operatorParameter(op,
                                VDX7VoiceData::Parameter::oscillatorMode)),
                            "Mode switch follows selected operator");
                        require(diagram->selectedOperator()==op,"Diagram selection updates editor");
                        for (int other=op+1;other<6;++other)
                            require(!bounds.intersects(diagram->nodeBounds(other)),"Nodes do not overlap");
                    }
                    require(beforeSelection==ram(save(restored)),"Selecting node does not edit voice RAM");
                }
                set(restored,algoID,4);
                for (auto* child : editor->getChildren())
                    if (child->isVisible())
                        require(!child->getBounds().isEmpty()
                                && editor->getLocalBounds().contains(child->getBounds()),
                                "visible control outside editor or missing bounds");
                if (argc > 1)
                {
                    auto path = juce::File(argv[1]).getChildFile(
                        "VDX7-v0.6.6-" + juce::String(width) + ".png");
                    auto stream = path.createOutputStream();
                    require(stream != nullptr, "snapshot file");
                    require(stream->setPosition(0) && stream->truncate().wasOk(), "snapshot reset");
                    juce::PNGImageFormat png;
                    require(png.writeImageToStream(
                        editor->createComponentSnapshot(editor->getLocalBounds()), *stream),
                        "snapshot render");
                }
            }
        }

        if (argc>1)
        {
            juce::Image sheet(juce::Image::RGB,960,1312,true);
            juce::Graphics g(sheet);
            g.fillAll(juce::Colour(0xff0b191f));
            for (int a=1;a<=32;++a)
            {
                const int x=((a-1)%4)*240, y=((a-1)/4)*164;
                VDX7AlgorithmView view;
                view.setSize(200,124);
                view.setState(a,(a-1)%6,7);
                g.setColour(juce::Colour(0xffe0eef0));
                g.setFont(juce::Font(juce::FontOptions(14.0f)));
                g.drawText("ALGORITHM "+juce::String(a),x+12,y+4,210,24,juce::Justification::centredLeft);
                g.drawImageAt(view.createComponentSnapshot(view.getLocalBounds()),x+12,y+28);
            }
            auto file=juce::File(argv[1]).getChildFile("VDX7-v0.6.6-algorithms.png");
            auto stream=file.createOutputStream();
            require(stream && stream->setPosition(0) && stream->truncate().wasOk(),"Algorithm sheet file");
            require(juce::PNGImageFormat().writeImageToStream(sheet,*stream),"Algorithm sheet render");
        }

        {
            auto renderWheel=[](float value)
            {
                juce::Image image(juce::Image::ARGB,48,138,true);
                juce::Graphics graphics(image);
                VDX7MechanicalDrawing::wheel(graphics,{0,0,48,138},value,false);
                return image;
            };
            auto a=renderWheel(0.30f), b=renderWheel(0.31f), repeated=renderWheel(0.30f);
            int movingRibPixels=0;
            for (int y=0;y<138;++y) for (int x=0;x<48;++x)
            {
                require(a.getPixelAt(x,y)==repeated.getPixelAt(x,y),"Wheel rendering is deterministic");
                if (x<5 || x>42) require(a.getPixelAt(x,y)==b.getPixelAt(x,y),"Wheel frame is stationary");
                // This region excludes the cyan stripe at these two positions.
                if (x>12 && x<35 && y>25 && y<60 && a.getPixelAt(x,y)!=b.getPixelAt(x,y))
                    ++movingRibPixels;
            }
            require(movingRibPixels>20,"Actual rib texture moves, not only the marker");
        }
        if (argc>1)
        {
            juce::Image sheet(juce::Image::RGB,800,320,true);
            juce::Graphics g(sheet); g.fillAll(juce::Colour(0xff102026));
            for (int frame=0;frame<9;++frame)
            {
                const float x=20.0f+frame*85.0f;
                VDX7MechanicalDrawing::wheel(g,{x,35,64,220},frame/8.0f,false);
                g.setColour(juce::Colours::white);
                g.setFont(juce::Font(juce::FontOptions(12.0f)));
                g.drawText(juce::String(frame*12.5f,1)+"%",int(x),10,64,20,juce::Justification::centred);
            }
            VDX7MechanicalDrawing::faderCap(g,{45,275,90,38},false,false);
            VDX7MechanicalDrawing::faderCap(g,{175,275,90,38},true,false);
            VDX7MechanicalDrawing::faderCap(g,{305,275,90,38},true,true);
            auto file=juce::File(argv[1]).getChildFile("VDX7-v0.6.6-wheel-frames.png");
            auto stream=file.createOutputStream();
            require(stream && stream->setPosition(0) && stream->truncate().wasOk(),"Wheel sheet file");
            require(juce::PNGImageFormat().writeImageToStream(sheet,*stream),"Wheel sheet render");
        }
        // A legacy/RAM-only state must restore fields absent from its parameter tree.
        auto legacyXml = juce::AudioProcessor::getXmlFromBinary(state.getData(), int(state.getSize()));
        auto legacy = juce::ValueTree::fromXml(*legacyXml);
        legacy.removeAllChildren(nullptr);
        juce::MemoryBlock legacyState;
        juce::AudioProcessor::copyXmlToBinary(*legacy.createXml(), legacyState);
        restored.setStateInformation(legacyState.getData(), int(legacyState.getSize()));
        require(ram(state) == ram(save(restored)), "RAM-only state restoration");

        // Run a factory voice through real firmware without opening an audio device.
        VDX7AudioProcessor render;
        for (const double rate : { 44100.0, 48000.0, 96000.0 })
        for (const int size : { 64, 128, 256 })
        {
            render.prepareToPlay(rate, size);
            juce::AudioBuffer<float> audio(2, size);
            juce::MidiBuffer midi;
            float peak = 0.0f;
            for (int block = 0; block < int(rate) / size; ++block)
            {
                if (block == 20)
                    midi.addEvent(juce::MidiMessage::noteOn(1, 60, uint8_t(100)), 0);
                render.processBlock(audio, midi);
                peak = std::max(peak, audio.getMagnitude(0, 0, size));
                for (int ch = 0; ch < 2; ++ch)
                    for (int sample = 0; sample < size; ++sample)
                        require(std::isfinite(audio.getSample(ch, sample)), "non-finite audio");
            }
            require(peak > 0.00001f, "silent factory voice render");
            midi.addEvent(juce::MidiMessage::allNotesOff(1), 0);
            render.processBlock(audio, midi);
        }
        std::cout << "PASS: legacy indices, headless program sync, 145 voice values, RAM/state round trip, "
                     "32 diagrams and all node hit tests at three sizes, nonmutating operator selection, "
                     "rename, copy/paste isolation, single/bank file export/import, failed I/O protection, dirty tracking, "
                     "RAM-only restore, stopped-transport edit, editor bounds, non-silent finite "
                     "44.1/48/96 kHz rendering at 64/128/256 samples\n";
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
    return 0;
}
