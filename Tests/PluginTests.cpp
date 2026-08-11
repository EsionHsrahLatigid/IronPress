#include "TestSupport.h"
#include "ParameterIDs.h"
#include "PluginProcessor.h"

#include <juce_events/juce_events.h>
#include <array>
#include <cmath>
#include <string>

int main()
{
    juce::ScopedJuceInitialiser_GUI juce;
    return test_support::run("ironpress_plugin_tests", [] {
        IronPressAudioProcessor processor;
        test_support::check(processor.getName() == "IronPress", "product name");
        test_support::check(!processor.acceptsMidi(), "compressor does not require MIDI");
        test_support::check(!processor.isMidiEffect(), "audio effect");
        test_support::check(processor.getTailLengthSeconds() == 0.0, "compressor has no tail");

        juce::AudioProcessor::BusesLayout stereo;
        stereo.inputBuses.add(juce::AudioChannelSet::stereo());
        stereo.outputBuses.add(juce::AudioChannelSet::stereo());
        test_support::check(processor.isBusesLayoutSupported(stereo), "stereo bus supported");

        juce::AudioProcessor::BusesLayout mono;
        mono.inputBuses.add(juce::AudioChannelSet::mono());
        mono.outputBuses.add(juce::AudioChannelSet::mono());
        test_support::check(processor.isBusesLayoutSupported(mono), "mono bus supported");

        juce::AudioProcessor::BusesLayout monoToStereo;
        monoToStereo.inputBuses.add(juce::AudioChannelSet::mono());
        monoToStereo.outputBuses.add(juce::AudioChannelSet::stereo());
        test_support::check(!processor.isBusesLayoutSupported(monoToStereo), "mono-to-stereo bus rejected");

        juce::AudioProcessor::BusesLayout stereoToMono;
        stereoToMono.inputBuses.add(juce::AudioChannelSet::stereo());
        stereoToMono.outputBuses.add(juce::AudioChannelSet::mono());
        test_support::check(!processor.isBusesLayoutSupported(stereoToMono), "stereo-to-mono bus rejected");

        constexpr const char* ids[] {
            ironpress::parameters::threshold,
            ironpress::parameters::ratio,
            ironpress::parameters::attack,
            ironpress::parameters::release,
            ironpress::parameters::knee,
            ironpress::parameters::detector,
            ironpress::parameters::sidechainHpf,
            ironpress::parameters::lookahead,
            ironpress::parameters::pump,
            ironpress::parameters::makeup,
            ironpress::parameters::mix,
            ironpress::parameters::output,
        };
        for (const auto* id : ids)
        {
            auto* parameter = processor.parameters.getParameter(id);
            test_support::check(parameter != nullptr, std::string("APVTS parameter exists: ") + id);
            test_support::check(parameter->getName(64).isNotEmpty(), std::string("parameter has host name: ") + id);
        }

        auto* threshold = processor.parameters.getParameter(ironpress::parameters::threshold);
        threshold->setValueNotifyingHost(threshold->convertTo0to1(-33.0f));
        juce::MemoryBlock state;
        processor.getStateInformation(state);
        threshold->setValueNotifyingHost(threshold->convertTo0to1(-12.0f));
        processor.setStateInformation(state.getData(), static_cast<int>(state.getSize()));
        test_support::check(std::abs(processor.parameters.getRawParameterValue(ironpress::parameters::threshold)->load() - -33.0f) < 0.01f, "state round-trip");

        const char invalid[] = "not xml";
        processor.setStateInformation(invalid, static_cast<int>(sizeof(invalid)));
        test_support::check(std::isfinite(processor.parameters.getRawParameterValue(ironpress::parameters::threshold)->load()), "invalid state ignored safely");

        processor.prepareToPlay(48000.0, 64);
        test_support::check(processor.getLatencySamples() == 480, "reported latency matches fixed maximum lookahead");
        juce::AudioBuffer<float> buffer(2, 64);
        for (int i = 0; i < 64; ++i)
        {
            buffer.setSample(0, i, i == 0 ? 1.0f : 0.0f);
            buffer.setSample(1, i, 0.1f);
        }
        juce::MidiBuffer midi;
        processor.processBlock(buffer, midi);
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
            for (int i = 0; i < buffer.getNumSamples(); ++i)
                test_support::check(std::isfinite(buffer.getSample(ch, i)), "processed samples finite");
    });
}
