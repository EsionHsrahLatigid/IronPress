#include "TestSupport.h"
#include "dsp/FoundationDSP.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <vector>

namespace
{
using ironpress::dsp::FoundationDSP;

float maxAbs(const std::vector<float>& values);

float renderAlternating(FoundationDSP::Parameters parameters, float value, int samples, int channel = 0)
{
    FoundationDSP dsp;
    dsp.prepare(48000.0, 1024, 2);
    dsp.setTargets(parameters);
    dsp.reset();

    std::vector<float> left(static_cast<std::size_t>(samples), 0.0f);
    std::vector<float> right(static_cast<std::size_t>(samples), 0.0f);
    for (int i = 0; i < samples; ++i)
    {
        left[static_cast<std::size_t>(i)] = (i % 2 == 0 ? value : -value);
        right[static_cast<std::size_t>(i)] = left[static_cast<std::size_t>(i)];
    }
    float* channels[] { left.data(), right.data() };
    dsp.processBlock(channels, 2, samples);
    return channel == 0 ? left.back() : right.back();
}

float renderSine(FoundationDSP::Parameters parameters, float frequency, float amplitude, int samples)
{
    FoundationDSP dsp;
    dsp.prepare(48000.0, 1024, 1);
    dsp.setTargets(parameters);
    dsp.reset();

    std::vector<float> buffer(static_cast<std::size_t>(samples), 0.0f);
    for (int i = 0; i < samples; ++i)
        buffer[static_cast<std::size_t>(i)] = amplitude * std::sin(2.0f * 3.14159265358979323846f * frequency * static_cast<float>(i) / 48000.0f);
    float* channels[] { buffer.data() };
    dsp.processBlock(channels, 1, samples);
    return maxAbs(buffer);
}

float maxAbs(const std::vector<float>& values)
{
    float result = 0.0f;
    for (const auto value : values)
        result = std::max(result, std::abs(value));
    return result;
}
} // namespace

int main()
{
    return test_support::run("ironpress_dsp_tests", [] {
        const auto below = FoundationDSP::computeStaticOutputDb(-30.0f, -24.0f, 4.0f, 0.0f);
        const auto above = FoundationDSP::computeStaticOutputDb(-12.0f, -24.0f, 4.0f, 0.0f);
        test_support::check(std::abs(below - -30.0f) < 0.001f, "hard-knee below threshold unchanged");
        test_support::check(std::abs(above - -21.0f) < 0.001f, "hard-knee above threshold follows ratio");

        const auto kneeLow = FoundationDSP::computeStaticOutputDb(-28.0f, -24.0f, 4.0f, 12.0f);
        const auto hardLow = FoundationDSP::computeStaticOutputDb(-28.0f, -24.0f, 4.0f, 0.0f);
        test_support::check(kneeLow < hardLow, "soft knee starts compression below threshold");

        FoundationDSP::Parameters fast;
        fast.thresholdDb = -36.0f;
        fast.ratio = 20.0f;
        fast.attackMs = 1.0f;
        fast.releaseMs = 80.0f;
        fast.kneeDb = 0.0f;
        fast.lookaheadMs = 10.0f;
        fast.makeupDb = 0.0f;
        fast.outputDb = 0.0f;
        fast.mix = 1.0f;
        const float fastAttack = renderAlternating(fast, 1.0f, 2200);

        auto slow = fast;
        slow.attackMs = 80.0f;
        const float slowAttack = renderAlternating(slow, 1.0f, 2200);
        test_support::check(std::abs(fastAttack) < std::abs(slowAttack) * 0.75f, "faster attack reduces stepped signal more");

        FoundationDSP dspRelease;
        dspRelease.prepare(48000.0, 1024, 1);
        dspRelease.setTargets(fast);
        dspRelease.reset();
        std::vector<float> hotBlock(3000, 1.0f);
        for (int i = 0; i < static_cast<int>(hotBlock.size()); ++i)
            hotBlock[static_cast<std::size_t>(i)] = i % 2 == 0 ? 1.0f : -1.0f;
        float* hotChannels[] { hotBlock.data() };
        dspRelease.processBlock(hotChannels, 1, static_cast<int>(hotBlock.size()));
        const float gainAfterHot = dspRelease.currentGainDb();
        std::vector<float> quietBlock(3000, 0.001f);
        float* quietChannels[] { quietBlock.data() };
        dspRelease.processBlock(quietChannels, 1, static_cast<int>(quietBlock.size()));
        test_support::check(dspRelease.currentGainDb() > gainAfterHot, "release recovers gain after level drop");

        auto peakParams = fast;
        peakParams.detectorMorph = 0.0f;
        const float peakOut = renderAlternating(peakParams, 1.0f, 1200);
        auto rmsParams = peakParams;
        rmsParams.detectorMorph = 1.0f;
        const float rmsOut = renderAlternating(rmsParams, 1.0f, 1200);
        test_support::check(std::abs(rmsOut) > std::abs(peakOut), "RMS detector responds slower than peak detector");

        auto noHpf = fast;
        noHpf.sidechainHpfHz = 20.0f;
        const float lowPumps = renderSine(noHpf, 40.0f, 0.7f, 3000);
        auto highHpf = noHpf;
        highHpf.sidechainHpfHz = 1000.0f;
        const float hpfPumpsLess = renderSine(highHpf, 40.0f, 0.7f, 3000);
        test_support::check(std::abs(hpfPumpsLess) > std::abs(lowPumps), "sidechain HPF reduces low-frequency gain pumping");

        FoundationDSP latencyDsp;
        latencyDsp.prepare(48000.0, 512, 1);
        FoundationDSP::Parameters latencyParams;
        latencyParams.thresholdDb = 0.0f;
        latencyParams.ratio = 1.0f;
        latencyParams.kneeDb = 0.0f;
        latencyParams.lookaheadMs = 10.0f;
        latencyParams.makeupDb = 0.0f;
        latencyParams.outputDb = 0.0f;
        latencyParams.mix = 1.0f;
        latencyDsp.setTargets(latencyParams);
        latencyDsp.reset();
        std::vector<float> impulse(520, 0.0f);
        impulse[0] = 1.0f;
        float* impulseChannels[] { impulse.data() };
        latencyDsp.processBlock(impulseChannels, 1, static_cast<int>(impulse.size()));
        const auto impulseIt = std::find_if(impulse.begin(), impulse.end(), [] (float value) { return std::abs(value) > 0.5f; });
        test_support::check(static_cast<int>(std::distance(impulse.begin(), impulseIt)) == latencyDsp.latencySamples(), "impulse appears at exact reported lookahead latency");

        FoundationDSP boundDsp;
        boundDsp.prepare(48000.0, 512, 2);
        FoundationDSP::Parameters boundParams;
        boundParams.thresholdDb = -60.0f;
        boundParams.ratio = 40.0f;
        boundParams.makeupDb = 24.0f;
        boundParams.outputDb = 6.0f;
        boundParams.mix = 1.0f;
        boundDsp.setTargets(boundParams);
        boundDsp.reset();
        std::vector<float> boundLeft(4096, 4.0f);
        std::vector<float> boundRight(4096, -4.0f);
        float* boundChannels[] { boundLeft.data(), boundRight.data() };
        boundDsp.processBlock(boundChannels, 2, static_cast<int>(boundLeft.size()));
        test_support::check(maxAbs(boundLeft) <= 4.0001f && maxAbs(boundRight) <= 4.0001f, "bounded makeup and output guard");

        FoundationDSP a;
        FoundationDSP b;
        a.prepare(44100.0, 0, 2);
        b.prepare(44100.0, 0, 2);
        a.setTargets(boundParams);
        b.setTargets(boundParams);
        a.reset();
        b.reset();
        for (int i = 0; i < 128; ++i)
        {
            const float input = i == 7 ? std::numeric_limits<float>::quiet_NaN() : (i % 11 == 0 ? 0.75f : 0.0f);
            const float outA = a.processSample(input, 0);
            const float outB = b.processSample(input, 0);
            test_support::check(std::isfinite(outA), "NaN and silence path remains finite");
            test_support::check(outA == outB, "reset determinism");
        }

        FoundationDSP mono;
        mono.prepare(48000.0, 64, 1);
        mono.setTargets(fast);
        std::vector<float> monoBuffer(64, 0.2f);
        float* monoChannels[] { monoBuffer.data() };
        mono.processBlock(monoChannels, 1, 64);
        test_support::check(std::all_of(monoBuffer.begin(), monoBuffer.end(), [] (float value) { return std::isfinite(value); }), "mono processing finite");
    });
}
