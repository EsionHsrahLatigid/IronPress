# Design

## DSP

IronPress uses `Source/dsp/FoundationDSP.*` as the product DSP core. Despite the generated foundation filename, this is the IronPress compressor:

- feed-forward log-domain gain computer;
- peak/RMS detector morph;
- threshold, ratio, attack, release, knee, sidechain HPF, lookahead, pump, makeup, mix, and output controls;
- fixed maximum lookahead delay allocated in `prepare`, reported to the host through `setLatencySamples`;
- sidechain filter and delay storage allocated only during `prepare`;
- finite input sanitization, bounded gain, and final digital amplitude guard.

The audio callback and direct DSP callees do not perform file, network, logging, lock, or heap allocation work in steady state. Parameter changes are smoothed, detector logs clamp near silence, and reset clears delay/filter/envelope state for deterministic renders.

## UI

The UI uses the shared DHN9 monochrome 8-bit system: 8 px grid, grayscale palette, no external images, no external fonts, and no `GenericAudioProcessorEditor`. The editor default size is 960 x 544 and the minimum is 720 x 432.

Every parameter has a visible slider with a stable component ID, accessible name, description, and tooltip. The visual motif is a grayscale press/gain-reduction meter: horizontal parameter rails, a top reduction meter, and a blocky press bed at the bottom.

## Safety Boundaries

IronPress is intentionally aggressive and can create abrupt level changes. It is not a mastering compressor and does not claim hearing safety. The plugin guards against NaN, Inf, and excessive internal digital amplitude; users still need conservative monitoring levels and downstream protection.
