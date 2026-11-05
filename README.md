# BaakProduction Master — Windows and macOS build project

**Build preparation only. No Windows or macOS plug-in binaries have been generated yet.**

Requested targets:
- Windows x64: VST3 plug-in and standalone EXE, built with Visual Studio 2022.
- macOS 11+: universal Intel + Apple Silicon VST3, AU and standalone app.

There are no Linux binaries in this package.

## Native build workflow
The workflow in `.github/workflows/build.yml` has separate Windows and macOS jobs. It uses a pinned JUCE revision, compiles the plug-ins, runs DSP and processor/editor checks, then packages only the matching platform outputs. The macOS job checks both CPU architectures and runs Audio Unit validation on the runner architecture. ZIP downloads are produced only after all required steps succeed.

The workflow has not been run. It may expose platform-specific compiler or host issues that still need fixing. macOS outputs use ad-hoc signatures, not Apple Developer ID signing/notarization.

To run through GitHub Actions, place this project's files at the root of a repository, enable Actions, then run **Windows and macOS plugins** from the Actions page. No public release is created by the workflow. It uploads private workflow artifacts accessible according to the repository's permissions.

Expected downloads after successful runs:
- `BaakProductionMaster-Windows-x64.zip`
- `BaakProductionMaster-macOS-Universal.zip`

## Local Windows build
Install Visual Studio 2022 with Desktop development with C++, plus CMake. Extract `third_party/JUCE-source.tar.gz` here, then run:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DJUCE_DIR=./JUCE
cmake --build build --config Release --parallel 2
ctest --test-dir build -C Release --output-on-failure
python tools/package_release.py --platform windows --artifacts build/BaakMaster_artefacts/Release --output dist
```

## Local Mac build
Install Xcode command-line tools and CMake. Extract the included JUCE archive here, then run:

```sh
cmake -S . -B build -DJUCE_DIR=./JUCE -DCMAKE_BUILD_TYPE=Release '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --config Release --parallel 3
ctest --test-dir build -C Release --output-on-failure
python3 tools/package_release.py --platform macos --artifacts build/BaakMaster_artefacts/Release --output dist
```

## Current product status
Alpha 0.1: EQ, dynamics, saturation, stereo controls, sample-peak limiting, Simple/Pro interface, snapshots, meters and rule-based Master Coach. It does not include trained AI, validated true-peak limiting or reference-file comparison. No Windows or Mac DAW compatibility claim is made before those builds are run and tested.

Original code is MIT. JUCE-based combined binaries use the AGPLv3 option unless an appropriate commercial JUCE licence is obtained. The exact dependency source and notices are included.
