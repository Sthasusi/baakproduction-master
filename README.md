# BAD-I Master Pro

Original stereo mastering plug-in by BaakProduction, version 0.2.0 alpha. Windows x64 VST3 and standalone; macOS universal Intel/Apple Silicon VST3, Audio Unit and standalone. No assistant or AI features.

## Changes from BaakProduction Master
New ivory/silver/gold fixed control panel, charcoal knobs, scalable layout, dark theme, large loudness control and advanced parameter view. Live input/output FFT spectrum, dynamics history, static EQ response, stereo scope and minimal listening display. Real measurements only.

New processing includes tape-style saturation with warm/tight tone, envelope-dependent cohesion saturation, tonal tilt, pre-limiter soft-clipping character, high-band harmonic processing, dynamic 300 Hz de-mud and five-band adaptive de-harsh. The existing three-band EQ, band dynamics, parallel bus compression, density, stereo width/bass mono and linked lookahead sample-peak limiter remain available. Processing is internally 2x oversampled.

Unity provides attenuation-only short-term LUFS matching, with bounded gain and gradual transitions. Auto input sets a starting gain near -18 short-term LUFS. It is not a recommendation system. Reference page loads local audio, auditions the reference independently, loops the whole track and supports seeking and separate reference gain. Metering remains on the master during reference audition and is labeled accordingly.

## Honest scope
This is an original implementation, not Master Plan 2's proprietary engine. It does not include certified true-peak limiting, selectable oversampling or low-latency engines, beat-grid reference loops, reference spectral overlays, phase display, automatic resonance-frequency tracking or automatic reference matching. De-harsh uses five fixed detection bands with adaptive attenuation. EQ display shows only the static three-band EQ. Snapshots use click to recall and Shift-click to store, not an automated snapshot morph. Reference audio is session-only and must be reloaded after reopening a project. Windows binaries are unsigned; Mac bundles are ad-hoc signed, not Apple-notarized.

## Installation and compatibility
Read INSTALL.txt and QUICK_START.md in each package. Product name, bundle ID and plug-in code are distinct from BaakProduction Master 0.1, so both can stay installed and old sessions retain the old plug-in. BAD-I Master Pro supports stereo input/output.

Native CI compiles and tests each platform, checks both Mac architecture slices and signatures, and runs Apple auval on Intel. A successful CI result does not establish compatibility with every DAW or listening quality on every system.

## Source build
Use CMake 3.22+ and a C++17 toolchain. JUCE is fetched at pinned revision d60b7a78030661574a615b586d9325ba7a7fa7dd, or supply JUCE_DIR.

Windows: `cmake -S . -B build -G "Visual Studio 17 2022" -A x64`, then `cmake --build build --config Release`.

Mac: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`, then `cmake --build build`.

Run `ctest --test-dir build -C Release --output-on-failure`. The workflow additionally runs processor/editor tests and renders the native interface.

Original code is MIT; JUCE combined binaries use the AGPLv3 option unless appropriately commercially licensed. See LICENSE.txt and THIRD_PARTY.md.
