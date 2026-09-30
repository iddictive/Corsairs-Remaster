# Corsairs — Iddictive Remaster for Mac

Native Apple Silicon / Metal build of City of Abandoned Ships, GPK 1.3.2 AT + ReConstruction 1.4.1, with rendering and gameplay changes.

[Download the app and read the case](https://iddictive.us/cases/corsairs-native-metal/)

**Experimental build:** a September 30 player launch hung during CoreAudio/TCC initialization alongside macOS WindowServer watchdog resets. A native launcher and a complete bundle signature address the observed packaging defect; a new player startup replay is pending. This is not a stable release.

## Play

Requires an Apple Silicon Mac and macOS 15 or newer. Unzip the app, move it to Applications and open it. Python and runtime libraries are bundled; Xcode and Homebrew are not needed to play. Saves and settings live separately in `~/Library/Application Support/Iddictive Corsairs` and survive replacement of the app. The app and nested runtime are ad-hoc signed and verified; the app is not notarized.

## Build

Requires Apple Silicon, macOS 15+, Xcode command-line tools, and Python 3.10+.

1. Install the downloadable app in `/Applications/Corsairs Iddictive Remaster.app`; it supplies the game data for development staging.
2. Download `Corsairs-Metal-build-inputs.tar.gz` from [the release](https://github.com/iddictive/Corsairs-Remaster/releases/tag/experimental-metal) and extract it into `experiments/native-metal` (creating `inputs/`). The manifest pins engine source, headers, dependencies, tools and reviewed originals.
3. Run `experiments/native-metal/run.sh --stage-only`, then `experiments/native-metal/run.sh --launch-installed` to play your development candidate.

Development staging creates independent empty saves. It does not import or overwrite your installed application's saves. `experiments/native-storm` contains source adapters required by the Metal build; its old runtime is not distributed or supported.

Rendering and gameplay changes remain subject to real-scene playtesting. Build and packaging checks do not prove full gameplay stability. Original engine source and dependency notices are included in the pinned build inputs; this repository has no blanket license granting ownership of third-party game content.
