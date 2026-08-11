#include "dsp/FoundationDSP.h"

#include <algorithm>
#include <cmath>

namespace ironpress::dsp
{
namespace
{
constexpr float minimumDb = -120.0f;
constexpr float maximumMakeupDb = 24.0f;
constexpr float maximumOutputDb = 6.0f;
constexpr float maximumDigitalAmplitude = 4.0f;
constexpr float maxLookaheadMs = 10.0f;
constexpr float parameterSmoothing = 0.0015f;
} // namespace

void FoundationDSP::prepare(double sampleRate, int, int channels) noexcept
{
    sampleRate_ = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 44100.0;
    channels_ = clamp(static_cast<float>(channels), 0.0f, 2.0f) > 1.0f ? 2 : (channels > 0 ? 1 : 0);
    maxLookaheadSamples_ = maxLookaheadSamplesForRate(sampleRate_);

    state_.assign(static_cast<std::size_t>(std::max(1, channels_)), {});
    for (auto& channel : state_)
        channel.delay.assign(static_cast<std::size_t>(maxLookaheadSamples_ + 1), 0.0f);

    reset();
}

void FoundationDSP::reset() noexcept
{
    gainDbCurrent_ = 0.0f;
    current_ = sanitizeParameters(target_);
    for (auto& channel : state_)
    {
        std::fill(channel.delay.begin(), channel.delay.end(), 0.0f);
        channel.writeIndex = 0;
        channel.hpfX1 = 0.0f;
        channel.hpfY1 = 0.0f;
        channel.rms = 0.0f;
    }
}

void FoundationDSP::setTargets(const Parameters& parameters) noexcept
{
    target_ = sanitizeParameters(parameters);
}

void FoundationDSP::processBlock(float* const* channels, int numChannels, int numSamples) noexcept
{
    if (channels == nullptr || numSamples <= 0)
        return;

    const int activeChannels = std::min({ numChannels, channels_, static_cast<int>(state_.size()) });
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float detectorPeak = 0.0f;
        float detectorRms = 0.0f;

        current_.thresholdDb = smooth(current_.thresholdDb, target_.thresholdDb, parameterSmoothing);
        current_.ratio = smooth(current_.ratio, target_.ratio, parameterSmoothing);
        current_.attackMs = smooth(current_.attackMs, target_.attackMs, parameterSmoothing);
        current_.releaseMs = smooth(current_.releaseMs, target_.releaseMs, parameterSmoothing);
        current_.kneeDb = smooth(current_.kneeDb, target_.kneeDb, parameterSmoothing);
        current_.detectorMorph = smooth(current_.detectorMorph, target_.detectorMorph, parameterSmoothing);
        current_.sidechainHpfHz = smooth(current_.sidechainHpfHz, target_.sidechainHpfHz, parameterSmoothing);
        current_.lookaheadMs = smooth(current_.lookaheadMs, target_.lookaheadMs, parameterSmoothing);
        current_.pump = smooth(current_.pump, target_.pump, parameterSmoothing);
        current_.makeupDb = smooth(current_.makeupDb, target_.makeupDb, parameterSmoothing);
        current_.mix = smooth(current_.mix, target_.mix, parameterSmoothing);
        current_.outputDb = smooth(current_.outputDb, target_.outputDb, parameterSmoothing);

        const int lookaheadSamples = static_cast<int>(
            clamp(static_cast<float>(std::lround(current_.lookaheadMs * 0.001f * static_cast<float>(sampleRate_))),
                  0.0f,
                  static_cast<float>(maxLookaheadSamples_)));
        const int detectorDelay = maxLookaheadSamples_ - lookaheadSamples;

        for (int channel = 0; channel < activeChannels; ++channel)
        {
            auto& channelState = state_[static_cast<std::size_t>(channel)];
            const float input = clamp(sanitize(channels[channel][sample]), -maximumDigitalAmplitude, maximumDigitalAmplitude);
            writeDelay(channelState, input);

            const float detectorInput = readDelay(channelState, detectorDelay);
            const float sidechain = sidechainHighpass(detectorInput, channelState, current_.sidechainHpfHz, sampleRate_);
            const float absValue = std::abs(sidechain);
            detectorPeak = std::max(detectorPeak, absValue);

            constexpr float rmsCoefficient = 0.01f;
            channelState.rms += (absValue * absValue - channelState.rms) * rmsCoefficient;
            detectorRms = std::max(detectorRms, std::sqrt(std::max(0.0f, channelState.rms)));
        }

        const float detector = detectorPeak + (detectorRms - detectorPeak) * current_.detectorMorph;
        const float levelDb = linearToDb(detector);
        const float targetGainDb = computeGainDb(levelDb, current_.thresholdDb, current_.ratio, current_.kneeDb);
        const bool attacking = targetGainDb < gainDbCurrent_;
        const float coefficient = timeCoefficient(attacking ? current_.attackMs : current_.releaseMs / (1.0f + current_.pump * 3.0f), sampleRate_);
        gainDbCurrent_ += (targetGainDb - gainDbCurrent_) * coefficient;

        const int emittedDelay = maxLookaheadSamples_;
        const float gain = dbToLinear(gainDbCurrent_ + current_.makeupDb) * dbToLinear(current_.outputDb);

        for (int channel = 0; channel < activeChannels; ++channel)
        {
            auto& channelState = state_[static_cast<std::size_t>(channel)];
            const float dry = readDelay(channelState, emittedDelay);
            const float delayed = readDelay(channelState, emittedDelay);
            const float compressed = clamp(delayed * gain, -maximumDigitalAmplitude, maximumDigitalAmplitude);
            const float mixed = dry + (compressed - dry) * current_.mix;
            channels[channel][sample] = sanitize(clamp(mixed, -maximumDigitalAmplitude, maximumDigitalAmplitude));
        }

        for (int channel = activeChannels; channel < numChannels; ++channel)
            if (channels[channel] != nullptr)
                channels[channel][sample] = 0.0f;
    }
}

float FoundationDSP::processSample(float input, int channel) noexcept
{
    float value = input;
    float* channels[] { &value };
    (void) channel;
    processBlock(channels, 1, 1);
    return value;
}

FoundationDSP::Parameters FoundationDSP::sanitizeParameters(const Parameters& parameters) noexcept
{
    Parameters p;
    p.thresholdDb = clamp(sanitize(parameters.thresholdDb), -60.0f, 0.0f);
    p.ratio = clamp(sanitize(parameters.ratio), 1.0f, 40.0f);
    p.attackMs = clamp(sanitize(parameters.attackMs), 0.1f, 200.0f);
    p.releaseMs = clamp(sanitize(parameters.releaseMs), 5.0f, 1000.0f);
    p.kneeDb = clamp(sanitize(parameters.kneeDb), 0.0f, 36.0f);
    p.detectorMorph = clamp(sanitize(parameters.detectorMorph), 0.0f, 1.0f);
    p.sidechainHpfHz = clamp(sanitize(parameters.sidechainHpfHz), 20.0f, 1000.0f);
    p.lookaheadMs = clamp(sanitize(parameters.lookaheadMs), 0.0f, maxLookaheadMs);
    p.pump = clamp(sanitize(parameters.pump), 0.0f, 1.0f);
    p.makeupDb = clamp(sanitize(parameters.makeupDb), 0.0f, maximumMakeupDb);
    p.mix = clamp(sanitize(parameters.mix), 0.0f, 1.0f);
    p.outputDb = clamp(sanitize(parameters.outputDb), -24.0f, maximumOutputDb);
    return p;
}

float FoundationDSP::computeGainDb(float inputLevelDb, float thresholdDb, float ratio, float kneeDb) noexcept
{
    const float level = clamp(sanitize(inputLevelDb), minimumDb, 24.0f);
    const float threshold = clamp(sanitize(thresholdDb), -60.0f, 0.0f);
    const float safeRatio = clamp(sanitize(ratio), 1.0f, 40.0f);
    const float knee = clamp(sanitize(kneeDb), 0.0f, 36.0f);
    const float over = level - threshold;

    float compressedOver = over;
    if (knee <= 0.001f)
    {
        compressedOver = over > 0.0f ? over / safeRatio : over;
    }
    else if (over <= -knee * 0.5f)
    {
        compressedOver = over;
    }
    else if (over >= knee * 0.5f)
    {
        compressedOver = over / safeRatio;
    }
    else
    {
        const float x = over + knee * 0.5f;
        compressedOver = over + (1.0f / safeRatio - 1.0f) * x * x / (2.0f * knee);
    }

    return clamp(compressedOver - over, -72.0f, 0.0f);
}

float FoundationDSP::computeStaticOutputDb(float inputLevelDb, float thresholdDb, float ratio, float kneeDb) noexcept
{
    return inputLevelDb + computeGainDb(inputLevelDb, thresholdDb, ratio, kneeDb);
}

int FoundationDSP::maxLookaheadSamplesForRate(double sampleRate) noexcept
{
    const double rate = std::isfinite(sampleRate) && sampleRate > 1000.0 ? sampleRate : 44100.0;
    return std::max(0, static_cast<int>(std::lround(rate * maxLookaheadMs * 0.001)));
}

float FoundationDSP::sanitize(float value) noexcept
{
    return std::isfinite(value) ? value : 0.0f;
}

float FoundationDSP::clamp(float value, float lo, float hi) noexcept
{
    return value < lo ? lo : (value > hi ? hi : value);
}

float FoundationDSP::dbToLinear(float db) noexcept
{
    return std::pow(10.0f, clamp(sanitize(db), -120.0f, 36.0f) / 20.0f);
}

float FoundationDSP::linearToDb(float linear) noexcept
{
    return 20.0f * std::log10(std::max(std::abs(sanitize(linear)), 0.000001f));
}

float FoundationDSP::timeCoefficient(float timeMs, double sampleRate) noexcept
{
    const float samples = std::max(1.0f, sanitize(timeMs) * 0.001f * static_cast<float>(sampleRate));
    return clamp(1.0f - std::exp(-1.0f / samples), 0.000001f, 1.0f);
}

float FoundationDSP::smooth(float current, float target, float coefficient) noexcept
{
    return current + (target - current) * coefficient;
}

float FoundationDSP::sidechainHighpass(float input, ChannelState& state, float cutoffHz, double sampleRate) noexcept
{
    const float cutoff = clamp(cutoffHz, 20.0f, 1000.0f);
    const float x = clamp(input, -maximumDigitalAmplitude, maximumDigitalAmplitude);
    const float rc = 1.0f / (2.0f * 3.14159265358979323846f * cutoff);
    const float dt = 1.0f / static_cast<float>(sampleRate);
    const float alpha = rc / (rc + dt);
    const float y = alpha * (state.hpfY1 + x - state.hpfX1);
    state.hpfX1 = x;
    state.hpfY1 = sanitize(y);
    return state.hpfY1;
}

float FoundationDSP::readDelay(const ChannelState& state, int delaySamples) noexcept
{
    if (state.delay.empty())
        return 0.0f;

    const int size = static_cast<int>(state.delay.size());
    int index = state.writeIndex - delaySamples - 1;
    while (index < 0)
        index += size;
    while (index >= size)
        index -= size;
    return state.delay[static_cast<std::size_t>(index)];
}

void FoundationDSP::writeDelay(ChannelState& state, float value) noexcept
{
    if (state.delay.empty())
        return;

    state.delay[static_cast<std::size_t>(state.writeIndex)] = sanitize(value);
    state.writeIndex = (state.writeIndex + 1) % static_cast<int>(state.delay.size());
}
} // namespace ironpress::dsp
