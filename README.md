# BaakProduction Master

A stereo mix-bus and mastering plug-in, alpha 0.1.

## Native downloads
The **Windows and macOS plugins** workflow builds separate Windows x64 and macOS universal artifacts. A workflow artifact is available only after its build and required tests succeed. Open the repository Actions tab, select a successful run, and download the matching artifact.

- Windows: VST3 bundle and standalone EXE.
- macOS: VST3, AU component and standalone app, each containing Intel and Apple Silicon architectures.
- macOS bundles are ad-hoc signed, not Apple-notarized; Windows binaries are unsigned.

The standalone application processes an audio device. It is not an audio-file player.

## Features
26 exposed parameters; Simple and Pro views; three-band EQ; bus and band dynamics; four saturation characters; stereo width and bass mono; lookahead sample-peak limiting; 2x oversampling; loudness metering; presets and A/B/C snapshots. Master Coach offers rule-based local audio analysis with Apply and Undo.

This alpha does **not** contain trained AI, validated true-peak limiting, or reference-track comparison. It is not a sonic clone of Master Plan 2. Platform build tests do not establish compatibility with every DAW or commercial mastering quality.

## Installation
Windows: copy the entire `BaakProduction Master.vst3` bundle into `C:\Program Files\Common Files\VST3\`, then rescan VST3 plug-ins in your DAW.

Mac VST3: copy the bundle into `~/Library/Audio/Plug-Ins/VST3/`.
Mac AU: copy the component into `~/Library/Audio/Plug-Ins/Components/`, then restart your DAW. Logic uses the AU component.

## Rebuild
Install CMake and the native C++ toolchain (Visual Studio 2022 C++ tools on Windows, Xcode tools on macOS). CMake fetches the exact pinned JUCE revision automatically.

Windows:
```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
```

Mac, universal:
```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
```

The workflow also exercises the native processor/editor, verifies binary formats, and validates the Audio Unit on the macOS runner architecture. It checks both Mac slices are present but does not substitute for separate listening and DAW tests on both CPU families.

## Licence
Original code is MIT; JUCE-based combined binaries use the AGPLv3 option unless an appropriate commercial JUCE licence is obtained. See LICENSE.txt and THIRD_PARTY.md for source and dependency notices.
