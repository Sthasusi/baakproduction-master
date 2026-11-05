# BaakProduction Master — quick start

BaakProduction Master is a stereo mix-bus and mastering effect: it shapes tone, controls dynamics, adjusts stereo width and raises perceived loudness. Version 0.1 is an experimental alpha.

## First session
1. Copy the entire plug-in bundle to the folder listed in INSTALL.txt. Rescan your DAW. Use AU in Logic, or VST3 in a compatible host.
2. Insert it on a stereo master or mix bus. Start with default settings and turn down your monitoring volume before increasing loudness.
3. Play a representative section for at least six seconds. Use Simple mode first.
4. Adjust Bass, Focus and Air in small increments. Use Glue for gentle compression and Warmth for saturation. Soften reduces dynamic high-frequency energy.
5. Increase Loudness gradually. Watch gain reduction and listen to kick, snare, vocal clarity and pumping. Ceiling defaults to -1 dBFS, a sample-peak setting rather than a true-peak guarantee.
6. Read Master Coach's suggestion. Apply changes one setting; Undo restores that change. Suggestions update from the current audio; they are optional and do not replace listening.
7. Compare the processed and original sound at similar perceived volume, using your DAW's gain control or a separate gain plug-in as needed. Automatic loudness matching is not included.
8. Check the entire song, including quiet passages and mono playback, before exporting.

## Controls
| Controls | Purpose |
| --- | --- |
| Input / Output trim | Gain before processing / attenuation after limiting |
| Bass / Focus / Air | Low shelf, broad mid bell, high shelf |
| Focus frequency / Low cut | Mid bell center / removal of sub-bass |
| Glue / threshold / ratio / attack / release | Parallel stereo-linked bus compression |
| Bass / Mid / Top control | Three-band compression amounts |
| Warmth / Character / Colour region | Saturation blend, waveshaper and frequency region |
| Soften | Dynamic attenuation above 4 kHz |
| Density | Up to 3 dB upward lift with a silence gate |
| Width / Bass mono | Mid/side width and reduction of low-frequency side content |
| Loudness / Ceiling / Limiter release | Drive and recovery of the lookahead sample-peak limiter |
| Bypass | Latency-aligned original audio |

Pro mode exposes detailed controls. Presets provide starting points; A/B/C snapshots let you compare settings. Project state is saved by the host.

## What Master Coach does
It checks measured limiter reduction, stereo correlation, ceiling settings and short-term loudness. It may suggest less limiter drive, neutral stereo width, more peak margin or a small loudness increase. Analysis stays local. It is rule-based, contains no trained AI model and cannot identify instruments or control other tracks in your DAW.

## Current limits
No certified true-peak or loudness compliance, reference-track matching, trained AI or AAX. The standalone app processes an audio device; it does not load song files. Windows builds are unsigned. Mac builds are ad-hoc signed and are not Apple notarized. Native build and interface tests do not establish compatibility with every DAW or certify audio quality.
