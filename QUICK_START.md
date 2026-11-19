# BAD-I Master Pro — Windows quick start

1. Close your DAW. Run **Install-Windows.cmd** from the extracted package to install the VST3 into the standard shared Windows folder. Windows requests administrator permission for that folder. The installer makes a dated backup if an older BAD-I bundle exists.
2. Reopen the DAW and rescan VST3 plug-ins. Insert **BAD-I Master Pro** on a stereo master bus.
3. Choose Default, play a representative passage, and click Auto input if useful. This sets a starting input level near -18 LUFS; it does not choose a master for you.
4. Increase Loud gradually. Use TP and a suitable ceiling in Settings. Watch the measured reduction and listen to transients.
5. Compare with Bypass and Unity. Allow several seconds for Unity's short-term loudness measurement to settle.
6. Before exporting, select **Master** on the Reference page and **Normal** monitoring. Reference audition and monitor filters affect the audible plug-in output.

The standalone executable processes an audio input device. Use its audio settings to select your device; it is not a track-rendering application. The VST3 supports stereo input and output.

## Controls

| Control | Actual BAD-I processing |
|---|---|
| In / Out | Input gain and post-limiter attenuation |
| Low / Mid / High | 120 Hz shelf, movable broad bell, 8 kHz shelf |
| C beside each EQ knob | Amount of linked band compression |
| Loud | Drive into the linked lookahead limiter |
| Presence | Blend soft clipping before limiting |
| Smooth | Parallel stereo bus compression |
| Body | Gated upward gain, up to 3 dB on quieter details |
| Thick / Color / region | Four original saturation curves, selected frequency region |
| Tape / Warm-tight | Independent saturation blend and tonal preparation |
| Silk | High-frequency nonlinear shaping |
| Glow | Envelope-dependent saturation |
| Calm | Adaptive attenuation in five fixed high-frequency bands |
| Clean | Dynamic low-mid reduction near 300 Hz |
| Aura | Warm-to-bright tonal tilt |
| Wide | Mid/side width: 100% unchanged; 0% mono |
| DC | DC-blocking high-pass stage |
| TP | Reconstructed-peak protection with additional headroom |
| LL | 1x processing, shorter limiter lookahead after audio restart |
| Unity | Gradually attenuates processed output toward input loudness |

Settings reveals low cut, bass mono, output ceiling, compressor timing/threshold/ratio, limiter release and oversampling. Some sounds differ from similarly named controls in the reference product.

## Interaction

Drag controls; hold Shift for fine movement. Double-click for numeric entry and Alt-click to reset. Hover information is disabled. Click a label to return a section to its default, then click again to restore it. Drag a mastering control to see its numeric value; double-click any mastering knob or slider to type it. The editor can be resized between 1050×630 and 2100×1260.

A/B/C switches comparisons while saving the slot you leave. Shift-click stores the current settings into that slot. Choose preset opens search, category filters and favorites; the plus symbol marks a favorite. User presets are saved under the user's AppData/BaakProduction/BAD-I Master Pro/Presets folder by default.

## Reference

Open or drop an audio file. The background analysis fills its waveform, spectrum, full-file loudness/RMS/peak statistics and estimated BPM. Click Reference to audition; Master returns to your processed signal. Set A and Set B use the current playhead. Beat snap rounds markers to a tempo grid when a reliable tempo estimate exists. Loop edges use a short fade. The grid does not identify musical downbeats.

Reference measurements include the reference gain trim, except PLR, which is gain-independent. The reference file is linked by path, not embedded in your DAW session. Keep it at the saved location.

## Reading the views

Metering combines spectrum, stereo image, waveform envelope and loudness/level readings. Spectrum compares input and output, with optional normalization or enlarged differences. Gain shows a scrolling peak envelope and limiter-reduction trace with selectable history. Ears Only shows input/output integrated LUFS.

EQ's cyan fill shows live output-minus-input level across frequency, including compression, saturation, width and overall gain. Its silver curve shows the static Low/Mid/High, low cut, Aura and DC filters. Show phase adds those filters' phase in amber; it is not a phase model of the nonlinear chain. Release controls analysis averaging. The visible range is ±12 dB. Quiet/unexcited bands have no live fill, and silence clears the live trace. A neutral or bypassed signal correctly produces a flat live response.

Reset meters clears loudness and held peaks. The reduction indicator reports light/moderate/heavy limiting from the measured reduction; it does not decide what loudness is musically correct.

## Version 0.3.2 limits

The DSP is independently implemented. No exact Master Plan sound, certified loudness/true-peak compliance, automated snapshot switching or legacy vendor engine is claimed. Quality and low-latency changes require the DAW to restart audio processing or reload the plug-in; the footer shows a pending change. The Windows build is unsigned.
