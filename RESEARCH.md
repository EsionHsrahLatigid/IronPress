# Research And Decision Map

IronPress follows the DHN9 G001 compressor decision: a feed-forward log-domain compressor based on the compressor taxonomy and implementation concerns summarized from Giannoulis, Massberg, and Reiss. The implementation keeps conventional detector, gain-computer, knee, and envelope semantics so the aggressive Digital Harsh Noise behavior remains testable and bounded.

## Evidence To Decision

- Giannoulis/Massberg/Reiss compressor taxonomy -> feed-forward detector before the gain stage, log-domain gain computer, threshold/ratio/knee controls, attack/release envelope smoothing.
- Giannoulis/Massberg/Reiss detector discussion -> peak/RMS morph control, with deterministic tests proving RMS reacts more slowly than peak detection.
- Compressor sidechain practice -> first-order sidechain high-pass filter to reduce low-frequency pumping, with a test showing reduced low-frequency detector action.
- Lookahead compressor practice -> prepare-time fixed delay storage and exact host latency report, with impulse tests verifying the reported sample delay.
- DHN9 Digital Harsh Noise goal -> exposed pump, high ratio range, bounded makeup gain, dry/wet mix, and hard output guard for extreme but finite dynamics.
- JUCE/Plitch foundation evidence -> APVTS state round-trip tests, custom editor tests, artifact staging, matched mono/stereo bus layouts, and no `GenericAudioProcessorEditor`.

## Rejected Alternatives

- Feedback compressor topology: rejected because deterministic lookahead and pump tests are clearer in a feed-forward design.
- Infinite-ratio limiter identity: rejected because BrickMaw owns limiter behavior in DHN9.
- Unbounded automatic makeup: rejected because silence-to-full-scale transitions must stay finite and bounded.

## Limits

IronPress does not implement mastering-grade metering, loudness normalization, true-peak limiting, program-adaptive release curves, or hearing protection. The digital guard is an internal finite-output invariant, not an SPL or monitoring-safety guarantee.
