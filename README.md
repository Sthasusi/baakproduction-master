# BAD-I Master Pro 0.3.3 — Windows

Native stereo mastering plug-in by BaakProduction. Windows 10/11 x64, VST3 and standalone. The interface uses dark navy surfaces, blue controls, cyan live EQ and complementary amber phase/high-reduction indicators, with cool silver text. No assistant or AI features.

## Working controls

- Input gain, automatic -18 LUFS starting level, output trim, latency-aligned bypass, attenuation-only Unity loudness comparison.
- Low and High shelving EQ; adjustable Mid bell; independently adjustable three-band dynamics.
- Loud limiter drive, Presence pre-clipping, Smooth parallel compression and gated upward Body processing.
- Four saturation curves with Full/Low/Mid/High targeting; independent Tape amount and tone; high-band Silk shaping; envelope-dependent Glow saturation.
- Adaptive five-band Calm attenuation, dynamic Clean around 300 Hz, Aura warm/bright tonal tilt, mid/side Wide and bass-mono control.
- DC filtering; 8-phase/64-tap reconstructed-peak detector and protective output stage with 0.6 dB reserve.
- Selectable 1x/2x/4x/8x oversampling; low-latency mode uses 1x and 1 ms limiter lookahead. Quality changes apply on the next host audio-engine preparation. A status message shows pending changes. TP protection retains 128 samples of delay in every mode.

## Metering and workflow

Six views: Metering, Spectrum, Gain, EQ, Reference and Ears Only. Live input/output FFT data; full, third-octave and Bark-spaced displays; normalization and difference display; stereo scope/correlation; RMS, estimated true peak, integrated/short-term/momentary LUFS and gain-reduction history. EQ now shows a live, latency-aligned stereo output-minus-input magnitude trace, including dynamics and saturation. Its white line shows Low/Mid/High, low cut, Aura and DC filters; optional amber phase covers those static filters only. Silence and unsupported frequency bins show no live measurement. Hover information is disabled throughout the interface. Mastering control values appear only while dragging; double-click opens numeric entry.

Reference import supports JUCE-readable WAV, AIFF, FLAC, MP3 and OGG files, background full-file loudness/peak/RMS analysis, waveform overview, average spectrum, audition, gain, waveform/slider seeking, explicit Play/Pause and Restart, reliable EOF replay, A/B loop markers, estimated tempo and optional tempo-grid snapping. Reference audio bypasses the mastering chain; master meters keep showing the master. Duration is available even when a file is loaded before the host prepares audio. File paths, reference gain and loop limits are stored with projects; missing files must be reselected. Reference playback requires audio callbacks from the host; an inline status prompts users to start the DAW when it is idle.

24 original factory presets, category/search filtering, favorites, user preset files, pagination, A/B/C comparisons, undo/redo, numeric entry, fine dragging and Alt-reset. Clicking a control label toggles its value between the default and its last setting. A/B/C saves the slot you leave; Shift-click stores into a slot.

## Boundaries

This is original DSP, not Musik Hack's proprietary algorithms or exact sound. Aura is a tonal tilt; Glow uses envelope-dependent saturation; Calm uses five fixed bands rather than moving resonance tracking. EQ phase view excludes dynamics and nonlinear stages. The gain graph is a peak envelope, not a sample-accurate waveform.

Snapshots are manual comparisons, not an automatable snapshot parameter. The reference tempo grid estimates tempo without downbeat alignment. Quality/LL switching is deferred to audio-engine preparation. The reduction indicator is measured gain reduction, not a prediction of ideal loudness. No legacy Master Plan engine or trained music model is included.

Peak and loudness calculations have regression tests but are not standards-certified. The peak guard is deliberately conservative. Windows binaries are unsigned. Host validation and synthetic tests do not replace listening tests on a representative music collection.

## Build and test

Install Visual Studio 2022 C++ Build Tools and CMake 3.22 or later:

```
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
build\baak_native_tests_artefacts\Release\baak_native_tests.exe C:\absolute\path\Interface.png
```

JUCE 9.0.5 is pinned to d60b7a78030661574a615b586d9325ba7a7fa7dd. Supply JUCE_DIR to use a local checkout. The Windows workflow compiles the plug-in and renders every view. The separate peak test reconstructs output at 16 phases with 96 taps, independently of the 8-phase production detector.

Original project source is MIT. Combined JUCE binaries use the AGPLv3 option; see LICENSE.txt, THIRD_PARTY.md and third_party/JUCE-LICENSE.md.

Design references: [official Master Plan page](https://www.musikhack.com/products/masterplan/) and the user's screenshots. No vendor binary code, graphical assets or presets were extracted.
