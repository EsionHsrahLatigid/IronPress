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
    setResizeLimits(minimumWidth, minimumHeight, defaultWidth * 2, defaultHeight * 2);
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

void IronPressAudioProcessorEditor::addControl(int index, const juce::String& parameterId, const juce::String& labelText, const juce::String& tip)
{
    auto& slider = sliders[static_cast<std::size_t>(index)];
    slider.setSliderStyle(juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle(juce::Slider::TextBoxRight, false, 72, 22);
    slider.setColour(juce::Slider::trackColourId, juce::Colour(0xff8a8a86));
    slider.setColour(juce::Slider::backgroundColourId, juce::Colour(0xff2a2a2a));
    slider.setColour(juce::Slider::thumbColourId, juce::Colour(0xfff2f2f0));
    slider.setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xfff2f2f0));
    slider.setColour(juce::Slider::textBoxBackgroundColourId, juce::Colour(0xff050505));
    slider.setColour(juce::Slider::textBoxOutlineColourId, juce::Colour(0xff8a8a86));
    slider.setComponentID("ironpress-" + parameterId);
    slider.setName("IronPress " + labelText);
    slider.setTitle(labelText);
    slider.setDescription(tip);
    slider.setTooltip(tip);
    slider.setWantsKeyboardFocus(true);
    addAndMakeVisible(slider);

    auto& label = labels[static_cast<std::size_t>(index)];
    label.setText(labelText, juce::dontSendNotification);
    label.setJustificationType(juce::Justification::centredLeft);
    label.setColour(juce::Label::textColourId, juce::Colour(0xfff2f2f0));
    label.setComponentID("ironpress-label-" + parameterId);
    label.setName(labelText);
    label.setTooltip(tip);
    addAndMakeVisible(label);

    attachments[static_cast<std::size_t>(index)] = std::make_unique<SliderAttachment>(ownerProcessor.parameters, parameterId, slider);
}

void IronPressAudioProcessorEditor::paint(juce::Graphics& g)
{
    const auto area = getLocalBounds();
    g.fillAll(juce::Colour(0xff050505));

    g.setColour(juce::Colour(0xfff2f2f0));
    g.setFont(juce::FontOptions(24.0f, juce::Font::bold));
    g.drawText("IronPress", 32, 16, area.getWidth() - 64, 32, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff8a8a86));
    g.setFont(juce::FontOptions(12.0f));
    g.drawText("COMPRESSOR", 32, 48, area.getWidth() - 64, 16, juce::Justification::centredLeft);

    g.setColour(juce::Colour(0xff2a2a2a));
    g.drawHorizontalLine(72, 32.0f, static_cast<float>(area.getWidth() - 32));
}

void IronPressAudioProcessorEditor::resized()
{
    auto area = getLocalBounds().reduced(32);
    area.removeFromTop(48);

    const int rowHeight = 32;
    const int rowGap = 8;
    const int columnGap = 24;
    const int columnWidth = (area.getWidth() - columnGap) / 2;
    for (int i = 0; i < static_cast<int>(sliders.size()); ++i)
    {
        const int column = i / 6;
        const int row = i % 6;
        const int x = area.getX() + column * (columnWidth + columnGap);
        const int y = area.getY() + row * (rowHeight + rowGap);
        labels[static_cast<std::size_t>(i)].setBounds(x, y, 86, rowHeight);
        sliders[static_cast<std::size_t>(i)].setBounds(x + 90, y, columnWidth - 90, rowHeight);
    }
}
