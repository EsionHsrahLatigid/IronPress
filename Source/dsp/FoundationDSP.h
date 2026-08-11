#pragma once

#include <cstddef>
#include <vector>

namespace ironpress::dsp
{
class FoundationDSP
{
public:
    struct Parameters
    {
        float thresholdDb { -24.0f };
        float ratio { 8.0f };
        float attackMs { 8.0f };
        float releaseMs { 90.0f };
        float kneeDb { 8.0f };
        float detectorMorph { 0.0f };
        float sidechainHpfHz { 80.0f };
        float lookaheadMs { 10.0f };
        float pump { 0.35f };
        float makeupDb { 6.0f };
        float mix { 1.0f };
        float outputDb { -3.0f };
    };

    void prepare(double sampleRate, int maxBlockSize, int channels) noexcept;
    void reset() noexcept;
    void setTargets(const Parameters& parameters) noexcept;
    void processBlock(float* const* channels, int numChannels, int numSamples) noexcept;

    float processSample(float input, int channel) noexcept;
    int preparedChannels() const noexcept { return channels_; }
    int latencySamples() const noexcept { return maxLookaheadSamples_; }
    float currentGainDb() const noexcept { return gainDbCurrent_; }

    static Parameters sanitizeParameters(const Parameters& parameters) noexcept;
    static float computeGainDb(float inputLevelDb, float thresholdDb, float ratio, float kneeDb) noexcept;
    static float computeStaticOutputDb(float inputLevelDb, float thresholdDb, float ratio, float kneeDb) noexcept;
    static int maxLookaheadSamplesForRate(double sampleRate) noexcept;

private:
    struct ChannelState
    {
        std::vector<float> delay;
        int writeIndex { 0 };
        float hpfX1 { 0.0f };
        float hpfY1 { 0.0f };
        float rms { 0.0f };
    };

    static float sanitize(float value) noexcept;
    static float clamp(float value, float lo, float hi) noexcept;
    static float dbToLinear(float db) noexcept;
    static float linearToDb(float linear) noexcept;
    static float timeCoefficient(float timeMs, double sampleRate) noexcept;
    static float smooth(float current, float target, float coefficient) noexcept;
    static float sidechainHighpass(float input, ChannelState& state, float cutoffHz, double sampleRate) noexcept;
    static float readDelay(const ChannelState& state, int delaySamples) noexcept;
    static void writeDelay(ChannelState& state, float value) noexcept;

    Parameters current_;
    Parameters target_;
    double sampleRate_ { 44100.0 };
    int channels_ { 0 };
    int maxLookaheadSamples_ { 441 };
    float gainDbCurrent_ { 0.0f };
    std::vector<ChannelState> state_;
};
} // namespace ironpress::dsp
