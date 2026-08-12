#include "PluginEditor.h"
#include "PluginProcessor.h"
#include "ParameterIDs.h"

namespace
{
struct ControlSpec
{
    const char* id;
    const char* label;
    const char* tip;
};

constexpr ControlSpec controls[] {
    { ironpress::parameters::threshold, "THRESH", "Threshold in dB before gain reduction begins." },
    { ironpress::parameters::ratio, "RATIO", "Compression ratio for signal above threshold." },
    { ironpress::parameters::attack, "ATTACK", "Envelope attack time in milliseconds." },
    { ironpress::parameters::release, "RELEASE", "Envelope release time in milliseconds." },
    { ironpress::parameters::knee, "KNEE", "Soft-knee width in dB." },
    { ironpress::parameters::detector, "PK/RMS", "Morphs detector from peak to RMS." },
    { ironpress::parameters::sidechainHpf, "SC HPF", "Sidechain high-pass frequency in Hz." },
    { ironpress::parameters::lookahead, "LOOK", "Detector lookahead in milliseconds within fixed reported latency." },
    { ironpress::parameters::pump, "PUMP", "Aggressive release acceleration for pumping." },
    { ironpress::parameters::makeup, "MAKEUP", "Bounded makeup gain in dB." },
    { ironpress::parameters::mix, "MIX", "Dry/compressed blend." },
    { ironpress::parameters::output, "OUTPUT", "Final output trim in dB before the digital guard." },
};

static_assert(std::size(controls) == 12);
} // namespace

IronPressAudioProcessorEditor::IronPressAudioProcessorEditor(IronPressAudioProcessor& p)
    : AudioProcessorEditor(&p), ownerProcessor(p),
      tooltipText("IronPress: feed-forward compressor controls with threshold, ratio, attack, release, knee, detector, sidechain, lookahead, pump, makeup, mix, and output.")
{
    setLookAndFeel(&ehlLookAndFeel);
    setResizeLimits(minimumWidth, minimumHeight,
                    ehl::juce_design::Metrics::maximumWidth,
                    ehl::juce_design::Metrics::maximumHeight);
    setResizable(true, true);
    setName("IronPress editor");
    setComponentID("ironpress-editor");
    setTitle("IronPress");
    setDescription("IronPress monochrome 8-bit compressor editor");
    setWantsKeyboardFocus(true);

    for (int i = 0; i < static_cast<int>(std::size(controls)); ++i)
        addControl(i, controls[i].id, controls[i].label, controls[i].tip);

    setSize(defaultWidth, defaultHeight);
}

IronPressAudioProcessorEditor::~IronPressAudioProcessorEditor()
{
    for (auto& slider : sliders)
        slider.setLookAndFeel(nullptr);
    for (auto& label : labels)
        label.setLookAndFeel(nullptr);
    tooltipWindow.setLookAndFeel(nullptr);
    setLookAndFeel(nullptr);
}

void IronPressAudioProcessorEditor::addControl(int index, const juce::String& parameterId, const juce::String& labelText, const juce::String& tip)
{
    auto& slider = sliders[static_cast<std::size_t>(index)];
    ehl::juce_design::styleSlider(slider);
    slider.setComponentID("ironpress-" + parameterId);
    slider.setName("IronPress " + labelText);
    slider.setTitle(labelText);
    slider.setDescription(tip);
    slider.setTooltip(tip);
    slider.setWantsKeyboardFocus(true);
    addAndMakeVisible(slider);

    auto& label = labels[static_cast<std::size_t>(index)];
    label.setText(labelText, juce::dontSendNotification);
    ehl::juce_design::styleLabel(label);
    label.setComponentID("ironpress-label-" + parameterId);
    label.setName(labelText);
    label.setTooltip(tip);
    addAndMakeVisible(label);

    attachments[static_cast<std::size_t>(index)] = std::make_unique<SliderAttachment>(ownerProcessor.parameters, parameterId, slider);
}

void IronPressAudioProcessorEditor::paint(juce::Graphics& g)
{
    ehl::juce_design::paintEditorChrome(g, getLocalBounds(), "IronPress", "COMPRESSOR");
}

void IronPressAudioProcessorEditor::resized()
{
    for (int i = 0; i < static_cast<int>(sliders.size()); ++i)
        ehl::juce_design::layoutLabelledControl(labels[static_cast<std::size_t>(i)],
                                                sliders[static_cast<std::size_t>(i)],
                                                ehl::juce_design::controlCell(getLocalBounds(), static_cast<std::size_t>(i)));
}
