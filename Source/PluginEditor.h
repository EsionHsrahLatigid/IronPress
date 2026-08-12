#pragma once

#include <ehl/juce_design/EhlDesign.h>
#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <memory>

class IronPressAudioProcessor;

class IronPressAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                           private juce::Timer
{
public:
    explicit IronPressAudioProcessorEditor(IronPressAudioProcessor&);
    ~IronPressAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;
    juce::String getTooltip() { return tooltipText; }

    static constexpr int defaultWidth = ehl::juce_design::Metrics::defaultWidth;
    static constexpr int defaultHeight = ehl::juce_design::Metrics::defaultHeight;
    static constexpr int minimumWidth = ehl::juce_design::Metrics::minimumWidth;
    static constexpr int minimumHeight = ehl::juce_design::Metrics::minimumHeight;

private:
    friend struct EditorTestAccess;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;

    void addControl(int index, const juce::String& parameterId, const juce::String& labelText, const juce::String& tip);
    void timerCallback() override;
    void updateParameterDisplay();

    IronPressAudioProcessor& ownerProcessor;
    ehl::juce_design::LookAndFeel ehlLookAndFeel;
    ehl::juce_design::ParameterDisplay parameterDisplay { ehl::juce_design::DisplayKind::compressor };
    juce::TooltipWindow tooltipWindow { this, 700 };
    juce::String tooltipText;
    std::array<juce::Slider, 12> sliders;
    std::array<juce::Label, 12> labels;
    std::array<std::unique_ptr<SliderAttachment>, 12> attachments;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(IronPressAudioProcessorEditor)
};
