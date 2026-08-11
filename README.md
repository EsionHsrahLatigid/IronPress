# IronPress

IronPress is an EHL Digital Harsh Noise compressor built as a JUCE audio effect. It is a violent dynamics instrument: a feed-forward log-domain compressor with peak/RMS detector morphing, soft/hard knee behavior, sidechain high-pass filtering, fixed reported lookahead latency, bounded pump/makeup, dry/wet mix, and output trim.

It is not a mastering compressor, loudness processor, true-peak limiter, or hearing-safety device. The output guard prevents non-finite samples and excessive digital amplitude inside the plugin, but it does not guarantee safe SPL, safe monitoring levels, or listener protection.

## Identity

- Product: `IronPress`
- Repository slug: `ironpress`
- Bundle ID: `jp.ehl.ironpress`
- Manufacturer: `EsionHsrahLatigid`
- Manufacturer code: `EHL_`
- Plugin code: `IrPr`

## Parameters

- `threshold` / Threshold: dB level where gain reduction begins.
- `ratio` / Ratio: feed-forward compression ratio above threshold.
- `attack` / Attack: envelope attack time in milliseconds.
- `release` / Release: envelope release time in milliseconds.
- `knee` / Knee: soft-knee width in dB.
- `detector` / Peak RMS Morph: peak-to-RMS detector blend.
- `sidechainHpf` / Sidechain HPF: detector high-pass frequency.
- `lookahead` / Lookahead: detector lookahead amount within the fixed reported latency.
- `pump` / Pump: aggressive release acceleration for pumping.
- `makeup` / Makeup: bounded makeup gain.
- `mix` / Mix: dry/compressed blend.
- `output` / Output: final output trim before the digital guard.

## Build

```sh
cmake --preset engine-debug
cmake --build --preset engine-debug
ctest --preset engine-debug --output-on-failure

cmake --preset plugin-release -DEHL_JUCE_SOURCE_DIR=/path/to/JUCE
cmake --build --preset plugin-release --target ehl_stage_products
ctest --preset plugin-release --output-on-failure
```

The project pins JUCE to `91ad83ae34a81e0833b1a2b0866f54846370ae53` when network FetchContent is used. Set `EHL_JUCE_SOURCE_DIR` for offline builds.

Stable artifacts:

```text
artifacts/plugin-release/macos-arm64/standalone/ironpress_standalone_plugin.app
artifacts/plugin-release/macos-arm64/vst3/ironpress_vst3_plugin.vst3
artifacts/plugin-release/macos-arm64/au/ironpress_au_plugin.component
artifacts/plugin-release/macos-arm64/ARTIFACTS.txt

artifacts/plugin-release/windows-x64/standalone/ironpress_standalone_plugin.exe
artifacts/plugin-release/windows-x64/vst3/ironpress_vst3_plugin.vst3
artifacts/plugin-release/windows-x64/ARTIFACTS.txt
```

## Tests

Targets are fixed for CI and humans:

- `ironpress_dsp_tests`
- `ironpress_plugin_tests`
- `ironpress_editor_tests`
- `ehl_stage_products`

The DSP tests cover static curve points below/above threshold and through the knee, attack/release response, peak/RMS distinction, sidechain HPF effect, exact impulse latency, gain bounds, reset determinism, silence, NaN sanitization, and mono/stereo processing.
