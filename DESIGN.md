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

The UI uses the strict DHN9 simple monochrome 8-bit contract: 4 px base spacing with 8 px major spacing, four-level palette `#050505`, `#2A2A2A`, `#8A8A86`, `#F2F2F0`, no external images, no external fonts, and no `GenericAudioProcessorEditor`. The editor default size remains 960 x 544 and the minimum remains 720 x 432.

Every parameter has a visible slider with a stable component ID, accessible name, description, and tooltip. The paint layer is intentionally minimal: product name at `y=16`, compact function label at `y=48`, and one 1 px divider at `y=72`; controls start at absolute `y=80`. Do not add a full-canvas grid, tagline, package ID, decorative motif, fake visualizer, fake meter, panel frame, outer border, or parameter-driven atmospheric drawing. DSP behavior, parameter IDs, bundle identity, accessibility, and host automation identity are unchanged.

## Safety Boundaries

IronPress is intentionally aggressive and can create abrupt level changes. It is not a mastering compressor and does not claim hearing safety. The plugin guards against NaN, Inf, and excessive internal digital amplitude; users still need conservative monitoring levels and downstream protection.
