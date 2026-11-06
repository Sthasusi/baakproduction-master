# BAD-I Master Pro — quick start

Place BAD-I Master Pro on a stereo mix bus or master. Begin with Clean Master, play a representative section and adjust Loudness gradually. Set the ceiling in Advanced. The displayed peak is a sample peak, not a true-peak measurement.

## Main controls
- Bass, Mid, Air: three-band tone adjustment. Mid frequency selects the mid bell center.
- Bass/Mid/Top control: gentle band compression amount.
- Loudness: limiter drive. Character blends soft pre-clipping. Glue mixes parallel bus compression. Density raises quieter details with a silence gate.
- Saturation: nonlinear coloration. Character selector chooses Soft, Rounded, Edge or Asymmetric; Colour region targets Full, Bass, Mid or Top.
- Tape: separate saturation; Tape tone blends warm darkening and tighter bass.
- Cohesion: envelope-dependent saturation. Tone color: broad tonal tilt.
- Sheen: high-frequency harmonic shaping.
- De-harsh: adaptive attenuation in five fixed bands from 1.5 to 11 kHz. De-mud: dynamic reduction near 300 Hz.
- Width: 100% is unchanged; 0% is mono. Bass mono reduces low-frequency side information.
- Advanced reveals low cut, ceiling, compression attack/release/threshold/ratio and limiter release.

## Comparison
Unity attenuates the processed signal toward input short-term loudness. Allow time for the three-second measurement and gain to settle. It never boosts the output. Bypass is latency-aligned. Auto input calibrates to approximately -18 short-term LUFS when audio is playing.

A/B/C: Shift-click to store; click to recall. Undo/Redo tracks knob drags, presets, snapshots and button changes; direct numeric text edits are not currently captured by the UI undo stack. Double-click a knob resets its value; edit the numeric text underneath for precise entry. Hold Shift for fine adjustment. Hover for help.

## Reference
Select Reference and Load reference. Choose a supported audio file. Audition reference plays it instead of the master output; Master returns to your processed track. Drag the seek slider and use the reference level control. The whole file loops. The reference is session-only and is not copied into your project. Meters continue displaying the master, explicitly labeled during reference audition.

Spectrum: actual input/output frequency content. Dynamics: eight seconds of loudness and gain reduction. EQ: static three-band response only. Stereo: mid/side scope. Ears only hides graphs.

## Release limits
Alpha 0.2. No AI/assistant. No certified true-peak processing, beat detection or exact Master Plan emulation. Test in your DAW before relying on it for delivery. Windows unsigned; Mac ad-hoc signed, not notarized. Standalone processes an audio device, not an audio-file playlist.
