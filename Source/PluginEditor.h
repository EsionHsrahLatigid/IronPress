#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

class IronPressAudioProcessor;

class IronPressAudioProcessorEditor final : public juce::AudioProcessorEditor
{
public:
    explicit IronPressAudioProcessorEditor(IronPressAudioProcessor&);
    ~IronPressAudioProcessorEditor() override = default;

    void paint(juce::Graphics&) override;
    void resized() override;
    juce::String getTooltip() { return tooltipText; }

    static constexpr int defaultWidth = 960;
    static constexpr int defaultHeight = 544;
    static constexpr int minimumWidth = 720;
    static constexpr int minimumHeight = 432;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void addControl(int index, const juce::String& parameterId, const juce::String& labelText, const juce::String& tip);

    IronPressAudioProcessor& ownerProcessor;
    juce::TooltipWindow tooltipWindow { this, 700 };
    juce::String tooltipText;
    std::array<juce::Slider, 12> sliders;
    std::array<juce::Label, 12> labels;
    std::array<std::unique_ptr<SliderAttachment>, 12> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IronPressAudioProcessorEditor)
};
