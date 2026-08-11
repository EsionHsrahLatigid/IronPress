#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "ParameterIDs.h"

#include <algorithm>
#include <cmath>

IronPressAudioProcessor::IronPressAudioProcessor()
    : AudioProcessor(BusesProperties()
          .withInput("Input", juce::AudioChannelSet::stereo(), true)
          .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

juce::AudioProcessorValueTreeState::ParameterLayout IronPressAudioProcessor::createParameterLayout()
{
    using Param = juce::AudioParameterFloat;
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;
    params.push_back(std::make_unique<Param>(ironpress::parameters::threshold, "Threshold", juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -24.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::ratio, "Ratio", juce::NormalisableRange<float>(1.0f, 40.0f, 0.1f), 8.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::attack, "Attack", juce::NormalisableRange<float>(0.1f, 200.0f, 0.1f, 0.45f), 8.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::release, "Release", juce::NormalisableRange<float>(5.0f, 1000.0f, 0.1f, 0.45f), 90.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::knee, "Knee", juce::NormalisableRange<float>(0.0f, 36.0f, 0.1f), 8.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::detector, "Peak RMS Morph", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::sidechainHpf, "Sidechain HPF", juce::NormalisableRange<float>(20.0f, 1000.0f, 0.1f, 0.35f), 80.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::lookahead, "Lookahead", juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 10.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::pump, "Pump", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.35f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::makeup, "Makeup", juce::NormalisableRange<float>(0.0f, 24.0f, 0.1f), 6.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::mix, "Mix", juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));
    params.push_back(std::make_unique<Param>(ironpress::parameters::output, "Output", juce::NormalisableRange<float>(-24.0f, 6.0f, 0.1f), -3.0f));
    return { params.begin(), params.end() };
}

void IronPressAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    dsp.prepare(sampleRate, samplesPerBlock, getTotalNumOutputChannels());
    setLatencySamples(dsp.latencySamples());
}

bool IronPressAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();
    if (mainIn != mainOut)
        return false;
    return mainOut == juce::AudioChannelSet::mono() || mainOut == juce::AudioChannelSet::stereo();
}

void IronPressAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused(midiMessages);
    juce::ScopedNoDenormals noDenormals;

    ironpress::dsp::FoundationDSP::Parameters targets;
    targets.thresholdDb = parameters.getRawParameterValue(ironpress::parameters::threshold)->load();
    targets.ratio = parameters.getRawParameterValue(ironpress::parameters::ratio)->load();
    targets.attackMs = parameters.getRawParameterValue(ironpress::parameters::attack)->load();
    targets.releaseMs = parameters.getRawParameterValue(ironpress::parameters::release)->load();
    targets.kneeDb = parameters.getRawParameterValue(ironpress::parameters::knee)->load();
    targets.detectorMorph = parameters.getRawParameterValue(ironpress::parameters::detector)->load();
    targets.sidechainHpfHz = parameters.getRawParameterValue(ironpress::parameters::sidechainHpf)->load();
    targets.lookaheadMs = parameters.getRawParameterValue(ironpress::parameters::lookahead)->load();
    targets.pump = parameters.getRawParameterValue(ironpress::parameters::pump)->load();
    targets.makeupDb = parameters.getRawParameterValue(ironpress::parameters::makeup)->load();
    targets.mix = parameters.getRawParameterValue(ironpress::parameters::mix)->load();
    targets.outputDb = parameters.getRawParameterValue(ironpress::parameters::output)->load();
    dsp.setTargets(targets);

    const int totalIn = getTotalNumInputChannels();
    const int totalOut = getTotalNumOutputChannels();
    for (int channel = totalIn; channel < totalOut; ++channel)
        buffer.clear(channel, 0, buffer.getNumSamples());

    float* channelPointers[2] { nullptr, nullptr };
    const int channelsToProcess = std::min(totalOut, 2);
    for (int channel = 0; channel < channelsToProcess; ++channel)
        channelPointers[channel] = buffer.getWritePointer(channel);
    dsp.processBlock(channelPointers, channelsToProcess, buffer.getNumSamples());
}

juce::AudioProcessorEditor* IronPressAudioProcessor::createEditor()
{
    return new IronPressAudioProcessorEditor(*this);
}

void IronPressAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = parameters.copyState().createXml())
        copyXmlToBinary(*state, destData);
}

void IronPressAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (auto xml = getXmlFromBinary(data, sizeInBytes))
        if (xml->hasTagName(parameters.state.getType()))
            parameters.replaceState(juce::ValueTree::fromXml(*xml));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new IronPressAudioProcessor();
}
