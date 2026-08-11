#include "TestSupport.h"
#include "PluginEditor.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>
#include <string>

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    return test_support::run("ironpress_editor_tests", [] {
        IronPressAudioProcessor processor;
        std::unique_ptr<juce::AudioProcessorEditor> editor(processor.createEditor());
        auto* custom = dynamic_cast<IronPressAudioProcessorEditor*>(editor.get());
        test_support::check(custom != nullptr, "custom editor type, not GenericAudioProcessorEditor");
        test_support::check(dynamic_cast<juce::GenericAudioProcessorEditor*>(editor.get()) == nullptr, "not GenericAudioProcessorEditor");
        test_support::check(editor->getWidth() == IronPressAudioProcessorEditor::defaultWidth, "default width");
        test_support::check(editor->getHeight() == IronPressAudioProcessorEditor::defaultHeight, "default height");
        test_support::check(editor->getComponentID() == "ironpress-editor", "component id");
        test_support::check(editor->getName().isNotEmpty(), "accessible name");
        test_support::check(custom->getTooltip().isNotEmpty(), "tooltip");
        test_support::check(editor->getWantsKeyboardFocus(), "keyboard focus");
        constexpr const char* sliderIds[] {
            "ironpress-threshold",
            "ironpress-ratio",
            "ironpress-attack",
            "ironpress-release",
            "ironpress-knee",
            "ironpress-detector",
            "ironpress-sidechainHpf",
            "ironpress-lookahead",
            "ironpress-pump",
            "ironpress-makeup",
            "ironpress-mix",
            "ironpress-output",
        };
        for (const auto* id : sliderIds)
        {
            auto* component = editor->findChildWithID(id);
            test_support::check(component != nullptr, std::string("visible parameter control: ") + id);
            test_support::check(component->getName().isNotEmpty(), std::string("control accessible name: ") + id);
            test_support::check(component->getDescription().isNotEmpty(), std::string("control description: ") + id);
        }

        juce::Image image(juce::Image::RGB, 320, 200, true);
        juce::Graphics g(image);
        editor->setBounds(0, 0, image.getWidth(), image.getHeight());
        editor->paint(g);
        const auto first = image.getPixelAt(0, 0);
        bool varied = false;
        for (int y = 0; y < image.getHeight(); y += 16)
            for (int x = 0; x < image.getWidth(); x += 16)
                varied = varied || image.getPixelAt(x, y) != first;
        test_support::check(varied, "software paint uses monochrome palette and procedural motif");
    });
}
