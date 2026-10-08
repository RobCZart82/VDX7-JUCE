#include "PluginEditor.h"
#include "VDX7AboutPanel.h"
#include "VDX7GuiScale.h"
#include "VDX7InitVoice.h"
#include "VDX7Sysex.h"

#include <array>
#include <cmath>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

struct VDX7RegressionAccess
{
    static juce::ComboBox& bank(VDX7AudioProcessorEditor& e) { return e.bank_; }
    static juce::Label& status(VDX7AudioProcessorEditor& e) { return e.status_; }
    static void refresh(VDX7AudioProcessorEditor& e) { e.refresh(true); }
    static VDX7AudioProcessor::ImportedBankSnapshot catalog(VDX7AudioProcessorEditor& e) { return e.bankCatalog_; }
    static void choose(VDX7AudioProcessorEditor& e, int id,
                       const VDX7AudioProcessor::ImportedBankSnapshot& token)
    { e.applyBankChoice(id, token); }
};

namespace
{
void require(bool condition, const char* message)
{
    if (!condition) throw std::runtime_error(message);
}

bool cancelExpectedDialog(const juce::String& title)
{
    bool presented = false;
    const auto started = juce::Time::getMillisecondCounter();
    std::function<void()> inspect;
    inspect = [&]
    {
        auto* modal = juce::ModalComponentManager::getInstance()->getModalComponent(0);
        if (modal != nullptr && modal->getName() == title)
        {
            presented = true;
            modal->exitModalState(0);
            juce::Timer::callAfterDelay(30, [] { juce::MessageManager::getInstance()->stopDispatchLoop(); });
        }
        else if (juce::Time::getMillisecondCounter() - started >= 2000)
            juce::MessageManager::getInstance()->stopDispatchLoop();
        else juce::Timer::callAfterDelay(10, inspect);
    };
    juce::Timer::callAfterDelay(10, inspect);
    juce::MessageManager::getInstance()->runDispatchLoop();
    return presented;
}

void checkImportedBankUi(const juce::File& firmware = {}, bool cancelConfirmation = false,
                         const juce::File& preview = {})
{
    struct Folder
    {
        juce::File file = juce::File::getSpecialLocation(juce::File::tempDirectory)
            .getNonexistentChildFile("vdx7-imported-gui", {}, false);
        Folder() { require(file.createDirectory().wasOk(), "isolated imported GUI folder"); }
        ~Folder() { file.deleteRecursively(); }
    } folder;
    const auto prefix = juce::String::charToString(0x1f3b9) + "VeryLongIdenticalBankPrefix";
    for (int n = 1; n <= 2; ++n)
    {
        auto voice = vdx7InitVoice();
        voice[0] = static_cast<uint8_t>(n);
        std::vector<uint8_t> bank;
        for (int slot = 0; slot < 32; ++slot) bank.insert(bank.end(), voice.begin(), voice.end());
        const auto syx = VDX7Sysex::encode(bank);
        require(folder.file.getChildFile(prefix + juce::String(n) + ".syx")
            .replaceWithData(syx.data(), syx.size()), "write synthetic imported GUI bank");
    }
    require(folder.file.getChildFile("broken.syx").replaceWithData("invalid", 7), "write rejected startup GUI fixture");
    auto p = std::make_unique<VDX7AudioProcessor>(false, juce::File(), folder.file);
    juce::String report;
    require(p->getImportedBankSnapshot() && p->getImportedBankSnapshot()->banks.size() == 2,
            "startup prepares imported GUI catalog before editor construction");
    VDX7AudioProcessorEditor editor(*p);
    require(VDX7RegressionAccess::status(editor).getTooltip().contains("Imported Banks startup scan")
        && VDX7RegressionAccess::status(editor).getTooltip().contains("broken.syx"),
        "startup diagnostics remain visible in the status tooltip without firmware");
    auto& bank = VDX7RegressionAccess::bank(editor);
    require(bank.getNumItems() == 11, "bank selector includes two imported banks below existing choices");
    require(!bank.isEnabled(), "imported bank selection remains disabled without firmware");
    require(bank.getItemText(9) != bank.getItemText(10), "equal shortened prefixes remain distinguishable");
    require(bank.getItemText(9).length() <= 15 && bank.getItemText(10).length() <= 15,
            "imported labels fit the planned character limit");
    const auto oldCatalog = p->getImportedBankSnapshot();
    int importedRows = 0;
    for (juce::PopupMenu::MenuItemIterator it(*bank.getRootMenu()); it.next();)
    {
        const auto& item = it.getItem();
        if (item.itemID < 100) continue;
        auto* row = dynamic_cast<VDX7ImportedBankMenuItem*>(item.customComponent.get());
        require(row != nullptr && row->getTooltip().contains(prefix)
            && row->getTooltip().contains(oldCatalog->banks[static_cast<std::size_t>(importedRows)].contentId),
            "popup row tooltip contains full Unicode filename and content identity");
        int width = 0, height = 0;
        row->getIdealSize(width, height);
        require(width == 260 && height == 26, "imported menu rows have bounded layout");
        row->setSize(width, height);
        require(row->createComponentSnapshot(row->getLocalBounds()).isValid(), "imported popup row renders");
        if (importedRows == 0 && preview != juce::File())
        {
            row->setLookAndFeel(&editor.getLookAndFeel());
            juce::FileOutputStream stream(preview);
            require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(
                row->createComponentSnapshot(row->getLocalBounds()), stream), "imported row visual QA written");
            row->setLookAndFeel(nullptr);
        }
        ++importedRows;
    }
    require(importedRows == 2, "both imported popup rows are checked");
    juce::Label popupLabel;
    const auto options = editor.getLookAndFeel().getOptionsForComboBoxPopupMenu(bank, popupLabel);
    require(options.getMaximumNumColumns() == 1 && options.getMinimumWidth() == 260,
            "bank popup stays single-column and bounded-width with JUCE scrolling");
    if (firmware != juce::File())
    {
        // This executable's opt-in harness uses JUCE dialogs, not native OS UI.
        juce::LookAndFeel::getDefaultLookAndFeel().setUsingNativeAlertWindows(false);
        require(p->loadRomFromFile(firmware), "opt-in private firmware loaded");
        require(p->selectImportedBank(oldCatalog, oldCatalog->banks[0].contentId, 7, report),
                "select imported bank for display test");
        VDX7RegressionAccess::refresh(editor);
        require(bank.isEnabled() && bank.getSelectedId() == 100
            && bank.getTooltip().contains(oldCatalog->banks[0].fileName), "live imported origin and full tooltip displayed");
        if (cancelConfirmation)
        {
            require(p->renameVoice("GUI EDIT"), "dirty voice before imported-bank confirmation");
            juce::MemoryBlock before;
            p->getStateInformation(before);
            VDX7RegressionAccess::choose(editor, 101, oldCatalog);
            const bool confirmationPresented = cancelExpectedDialog("Unexported voice edits");
            juce::MemoryBlock after;
            p->getStateInformation(after);
            require(confirmationPresented && before == after
                && p->getImportedBankSelection().index == 0 && bank.getSelectedId() == 100,
                "cancelled dirty-bank replacement preserves full processor state and displayed bank");
            return;
        }
        bank.showPopup();
        require(bank.isPopupActive(), "standard JUCE bank popup opened");
        require(p->refreshImportedBanks(folder.file, report), "replace catalog during open popup");
        VDX7RegressionAccess::refresh(editor);
        require(VDX7RegressionAccess::catalog(editor) == oldCatalog,
                "open popup keeps its displayed generation even when processor catalog changes");
        bank.hidePopup();
        bank.setSelectedId(101, juce::dontSendNotification); // Queued result from that popup.
        VDX7RegressionAccess::refresh(editor);
        require(VDX7RegressionAccess::catalog(editor) == oldCatalog, "pending result retains old popup token");
        bank.onChange();
        require(p->getCurrentProgram() == 7 && p->getImportedBankSelection().index == 0,
                "old popup choice cannot select another bank from a refreshed list");
        const bool errorPresented = cancelExpectedDialog("Imported bank not applied");
        require(errorPresented, "stale imported choice presents an error");
        VDX7RegressionAccess::refresh(editor);
        // Keyboard choices are asynchronous in JUCE. A timer tick must not
        // overwrite the pending ID or remap it before onChange runs.
        bank.setSelectedId(101, juce::dontSendNotification);
        VDX7RegressionAccess::refresh(editor);
        require(bank.getSelectedId() == 101, "timer preserves pending keyboard selection");
        bank.onChange();
        require(p->getCurrentProgram() == 0 && p->getImportedBankSelection().index == 1,
                "keyboard selection applies the displayed bank through the processor API");
        require(bank.getSelectedId() == 101, "bank selector follows the applied imported origin");
        VDX7RegressionAccess::choose(editor, 0, p->getImportedBankSnapshot());
        require(p->getImportedBankSelection().index == 1, "popup cancellation leaves imported selection unchanged");
    }
    require(p->refreshImportedBanks(folder.file.getChildFile("missing"), report), "empty explicit refresh");
    VDX7RegressionAccess::refresh(editor);
    require(bank.getNumItems() == (firmware == juce::File() ? 9 : 10),
            "refresh removes stale rows but preserves the selected project bank and factory/USER choices");
}

void checkWhiteKeyHover()
{
    juce::MidiKeyboardState state;
    VDX7Keyboard keyboard(state);
    for (int factor : { 1, 2 })
    {
        juce::Image normal(juce::Image::ARGB, 40 * factor, 138 * factor, true);
        juce::Image hover(juce::Image::ARGB, 40 * factor, 138 * factor, true);
        for (bool over : { false, true })
        {
            juce::Graphics g(over ? hover : normal);
            g.addTransform(juce::AffineTransform::scale(float(factor)));
            keyboard.drawWhiteNote(60, g, { 0, 0, 40, 138 }, false, over, {}, {});
        }
        for (int y = 0; y < 138 * factor; ++y)
            for (int x = 0; x < 40 * factor; ++x)
                if (y < 11 * factor || y >= 127 * factor)
                    require(normal.getPixelAt(x, y) == hover.getPixelAt(x, y),
                            "white hover stays off key margins");
        require(normal.getPixelAt(20 * factor, 70 * factor) != hover.getPixelAt(20 * factor, 70 * factor),
                "white key retains visible hover feedback");
    }
}
}

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI gui;
    try
    {
        if (argc == 3 && juce::String(argv[1]) == "--imported-bank-preview")
        {
            checkImportedBankUi({}, false, juce::File(argv[2]));
            std::cout << "PASS: imported bank row visual QA and ROM-free UI checks\n";
            return 0;
        }
        if (argc == 3 && juce::String(argv[1]) == "--imported-bank-rom")
        {
            checkImportedBankUi(juce::File(argv[2]));
            std::cout << "PASS: imported bank UI, private firmware selection, stale popup and keyboard scheduling\n";
            return 0;
        }
        if (argc == 3 && juce::String(argv[1]) == "--imported-bank-cancel-rom")
        {
            checkImportedBankUi(juce::File(argv[2]), true);
            std::cout << "PASS: imported bank dirty confirmation and cancel preserve full project state\n";
            return 0;
        }
        // Optional local visual QA, not part of headless/hosted CI.
        if (argc == 3 && juce::String(argv[1]) == "--settings-preview")
        {
            auto processor = std::make_unique<VDX7AudioProcessor>(false);
            std::unique_ptr<juce::AudioProcessorEditor> editor(processor->createEditor());
            bool opened = false;
            for (auto* child : editor->getChildren())
                if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    if (button->getButtonText() == "SETTINGS") { button->onClick(); opened = true; break; }
            require(opened, "settings button opens dialog");
            auto* dialog = dynamic_cast<juce::AlertWindow*>(juce::ModalComponentManager::getInstance()->getModalComponent(0));
            require(dialog != nullptr, "settings modal exists");
            int bankButtons = 0;
            for (auto* child : dialog->getChildren())
            {
                require(dialog->getLocalBounds().contains(child->getBounds()), "settings child fits dialog");
                if (auto* button = dynamic_cast<juce::TextButton*>(child))
                    if (button->getButtonText() == "Bank folder" || button->getButtonText() == "Refresh banks") ++bankButtons;
            }
            require(bankButtons == 2, "both factory library tools are visible in settings");
            juce::FileOutputStream stream {juce::File(argv[2])};
            require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(
                dialog->createComponentSnapshot(dialog->getLocalBounds()), stream), "settings preview written");
            dialog->exitModalState(0); // No settings/files/installed plug-ins changed.
            juce::Timer::callAfterDelay(50, [] { juce::MessageManager::getInstance()->stopDispatchLoop(); });
            juce::MessageManager::getInstance()->runDispatchLoop();
            std::cout << "PASS: Settings bank tools fit the dialog\n";
            return 0;
        }
        // Optional local measurement only, never part of the timed CI test.
        // Keep instances alive briefly so the caller can sample process RSS.
        if (argc == 3 && juce::String(argv[1]) == "--benchmark-editors")
        {
            const int count = juce::String(argv[2]).getIntValue();
            require(count == 0 || count == 1 || count == 4 || count == 8, "benchmark count is 0/1/4/8");
            std::vector<std::unique_ptr<VDX7AudioProcessor>> processors;
            std::vector<std::unique_ptr<juce::AudioProcessorEditor>> editors;
            const auto started = juce::Time::getMillisecondCounterHiRes();
            for (int i = 0; i < count; ++i)
            {
                processors.push_back(std::make_unique<VDX7AudioProcessor>(false));
                editors.emplace_back(processors.back()->createEditor());
            }
            std::cout << "Editors=" << count << " create_ms="
                      << juce::Time::getMillisecondCounterHiRes() - started << std::endl;
            juce::Thread::sleep(1000);
            return 0;
        }
        checkImportedBankUi();
        checkWhiteKeyHover();
        for (std::size_t i = 0; i < VDX7GuiScale::presets.size(); ++i)
        {
            const auto& preset = VDX7GuiScale::presets[i];
            require(VDX7GuiScale::indexForWidth(preset.width) == static_cast<int>(i),
                    "each fixed Settings width selects its matching GUI size");
            const auto& selected = VDX7GuiScale::atIndex(static_cast<int>(i));
            require(selected.percentage == preset.percentage && selected.width == preset.width
                    && selected.height == preset.height,
                    "fixed Settings sizes keep their approved dimensions");
        }
        require(VDX7GuiScale::indexForWidth(750) == 0
                && VDX7GuiScale::indexForWidth(1050) == 1
                && VDX7GuiScale::indexForWidth(1350) == 2,
                "Settings size selection keeps nearest-preset tie behavior");

        VDX7AboutPanel about;
        require(about.hasVectorLogos(), "About loads the VDX7, GYR and signature vectors");
        for (auto* child : about.getChildren())
            require(about.getLocalBounds().contains(child->getBounds()),
                    "About controls remain inside their original panel");
        require(about.createComponentSnapshot(about.getLocalBounds()).isValid(),
                "About panel renders its vector artwork");
        {
            VDX7WheelSlider pitch(true);
            VDX7WheelSlider mod(false);
            pitch.setRange(0.0, 1.0, 0.001);
            mod.setRange(0.0, 1.0, 0.001);
            pitch.setSize(40, 140);
            mod.setSize(40, 140);
            pitch.setValue(0.5, juce::dontSendNotification);
            mod.setValue(0.5, juce::dontSendNotification);

            const auto now = juce::Time::getCurrentTime();
            const auto sendScroll = [now](juce::Slider& slider)
            {
                const juce::MouseEvent event {
                    juce::Desktop::getInstance().getMainMouseSource(), {10.0f, 10.0f},
                    juce::ModifierKeys {}, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                    &slider, &slider, now, {10.0f, 10.0f}, now, 0, false
                };
                slider.mouseWheelMove(event, {0.0f, 1.0f, false, true, false});
            };

            require(!pitch.isScrollWheelEnabled(), "pitch wheel ignores scroll/trackpad input");
            require(mod.isScrollWheelEnabled(), "mod wheel retains scroll/trackpad input");
            sendScroll(pitch);
            sendScroll(mod);
            require(std::abs(pitch.getValue() - 0.5) < 0.0001,
                    "pitch scroll cannot leave a persistent bend");
            require(std::abs(mod.getValue() - 0.5) > 0.0001,
                    "mod scroll continues to change and retain its value");

            require(pitch.keyPressed(juce::KeyPress(juce::KeyPress::upKey)),
                    "pitch wheel remains adjustable from the keyboard");
            require(std::abs(pitch.getValue() - 0.501) < 0.0001,
                    "keyboard input still changes pitch-wheel value");

            pitch.setValue(0.75, juce::sendNotificationSync);
            const juce::MouseEvent release {
                juce::Desktop::getInstance().getMainMouseSource(), {10.0f, 10.0f},
                juce::ModifierKeys {}, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
                &pitch, &pitch, now, {10.0f, 10.0f}, now, 0, false
            };
            pitch.mouseUp(release);
            require(std::abs(pitch.getValue()) < 0.0001,
                    "pitch drag release still springs back to centre");
        }

        VDX7AudioProcessor processor(false);
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        require(editor != nullptr, "editor creation without firmware");
        require(!editor->isResizable(), "host must not resize the editor window");
        int saveAsButtons = 0;
        bool algorithmDisabled = false;
        bool hasCornerResizer = false;
        juce::TextButton* editTab = nullptr;
        juce::TextButton* performanceTab = nullptr;
        VDX7PerformancePanel* performancePanel = nullptr;
        for (auto* child : editor->getChildren())
        {
            if (auto* button = dynamic_cast<juce::TextButton*>(child))
            {
                if (button->getButtonText() == "EDIT") editTab = button;
                if (button->getButtonText() == "PERFORMANCE") performanceTab = button;
                if (button->getButtonText() == "SAVE AS...")
                {
                    ++saveAsButtons;
                    require(!button->isEnabled(), "Save As is disabled without firmware");
                }
            }
            if (child->getName() == "Performance controllers")
                performancePanel = dynamic_cast<VDX7PerformancePanel*>(child);
            if (auto* box = dynamic_cast<juce::ComboBox*>(child))
                if (box->getName() == "Algorithm")
                {
                    algorithmDisabled = true;
                    require(!box->isEnabled(), "algorithm selector is disabled without firmware");
                }
            hasCornerResizer |= dynamic_cast<juce::ResizableCornerComponent*>(child) != nullptr;
        }
        require(saveAsButtons == 1, "one Save As control is present without firmware");
        require(algorithmDisabled, "algorithm selector is present without firmware");
        require(!hasCornerResizer, "bottom-right drag resizer is absent");
        require(editTab && performanceTab && performancePanel,
                "EDIT, PERFORMANCE and its panel are present");
        require(editTab->getToggleState() && !performanceTab->getToggleState()
                && !performancePanel->isVisible(), "editor opens in EDIT mode");

        constexpr std::array<const char*, 6> performanceComboNames
        {{
            "Play mode", "Portamento mode", "Glissando", "Portamento time",
            "Pitch bend range", "Pitch bend step"
        }};
        std::array<int, performanceComboNames.size()> performanceCombos {};
        int controllerRanges = 0;
        int controllerAssignments = 0;
        for (auto* child : performancePanel->getChildren())
        {
            if (auto* combo = dynamic_cast<juce::ComboBox*>(child))
                for (std::size_t i = 0; i < performanceComboNames.size(); ++i)
                    if (combo->getName() == performanceComboNames[i]) ++performanceCombos[i];
            if (auto* slider = dynamic_cast<juce::Slider*>(child))
                if (slider->getName().startsWith("Controller ")
                    && slider->getName().endsWith(" range")) ++controllerRanges;
            if (auto* button = dynamic_cast<juce::ToggleButton*>(child))
                if (button->getName().startsWith("Controller ")
                    && button->getName().contains(" assignment ")) ++controllerAssignments;
        }
        for (const auto count : performanceCombos)
            require(count == 1, "each named PERFORMANCE selector exists exactly once");
        require(controllerRanges == 4, "all four PERFORMANCE controller range controls exist");
        require(controllerAssignments == 12, "all twelve PERFORMANCE assignment switches exist");

        const juce::Colour accent(0xff71685b);
        constexpr std::array<float, 3> decorationY { 22.0f, 34.0f, 46.0f };
        for (const auto& preset : VDX7GuiScale::presets)
        {
            const int width = preset.width;
            const int height = preset.height;
            editor->setSize(width, height);
            require(editor->getWidth() == width && editor->getHeight() == height,
                    "editor accepts each fixed Settings preset size");
            for (auto* child : editor->getChildren())
                require(editor->getLocalBounds().contains(child->getBounds()),
                        "GUI child bounds remain inside each fixed preset size");
            const auto image = editor->createComponentSnapshot(editor->getLocalBounds());
            require(image.isValid(), "header snapshot at supported scale");
            const float scaleX = float(width) / 1440.0f;
            const float scaleY = float(height) / 1110.0f;
            const int removedSeparatorX = juce::roundToInt(594.0f * scaleX);
            const int removedSeparatorY = juce::roundToInt(450.0f * scaleY);
            require(image.getPixelAt(removedSeparatorX, removedSeparatorY).getARGB() != accent.getARGB(),
                    "unneeded separator beside the global curve display is absent");
            const int operatorSeparatorX = juce::roundToInt(692.0f * scaleX);
            const int operatorSeparatorY = juce::roundToInt(600.0f * scaleY);
            require(image.getPixelAt(operatorSeparatorX, operatorSeparatorY).getARGB() == accent.getARGB(),
                    "operator controls and envelope sliders have a section separator");
            // The top accent now has an intentional gap for the centre screw.
            const int headerX = juce::roundToInt(100.0f * scaleX);
            const int dividerX = juce::roundToInt(100.0f * scaleX);
            const int dividerY = juce::roundToInt(176.0f * scaleY);
            require(image.getPixelAt(dividerX, dividerY).getARGB() == accent.getARGB(),
                    "header accent test uses the existing section-divider color");

            for (const auto y : decorationY)
            {
                const int pixelY = static_cast<int>(std::floor(y * scaleY));
                require(image.getPixelAt(headerX, pixelY).getARGB() == accent.getARGB(),
                        "all three header accents are visible in divider color");
            }
            if (argc == 2 && width == 1200)
            {
                juce::FileOutputStream stream { juce::File(argv[1]) };
                require(stream.openedOk() && juce::PNGImageFormat().writeImageToStream(image, stream),
                        "write optional GUI preview");
            }

            require(performanceTab->isEnabled() && performanceTab->isVisible(),
                    "PERFORMANCE tab is available in EDIT view");
            performanceTab->onClick();
            require(performanceTab->getToggleState(), "PERFORMANCE tab becomes active");
            require(!editTab->getToggleState(), "EDIT tab deactivates in PERFORMANCE view");
            require(performancePanel->isVisible(), "PERFORMANCE tab shows its panel");
            for (auto* child : editor->getChildren())
                require(editor->getLocalBounds().contains(child->getBounds()),
                        "PERFORMANCE view remains inside each fixed preset size");
            for (auto* child : performancePanel->getChildren())
                require(performancePanel->getLocalBounds().contains(child->getBounds()),
                        "PERFORMANCE controls remain inside their section");
            const auto performanceImage = editor->createComponentSnapshot(editor->getLocalBounds());
            require(performanceImage.isValid(), "PERFORMANCE view renders at every supported size");
            editTab->onClick();
            require(editTab->getToggleState() && !performanceTab->getToggleState()
                    && !performancePanel->isVisible(), "EDIT tab restores the editor view");
        }
        std::cout << "PASS: About artwork and EDIT/PERFORMANCE views render at all five fixed GUI sizes\n";
        return 0;
    }
    catch (const std::exception& error)
    {
        std::cerr << "FAIL: " << error.what() << '\n';
        return 1;
    }
}
