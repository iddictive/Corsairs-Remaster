# Corsairs — Iddictive Remaster for Mac

Native Apple Silicon / Metal version of **City of Abandoned Ships**, based on GPK 1.3.2 AT + ReConstruction 1.4.1.

[Download the macOS app](https://drive.google.com/drive/folders/1JDiOAg3S3OgKAATjjA7TZfIHW39u2Wi8) · [Screenshots and project case](https://iddictive.us/cases/corsairs-native-metal/) · [Build inputs and release notes](https://github.com/iddictive/Corsairs-Remaster/releases/tag/experimental-metal)

The game runs directly on Apple Silicon through a native Metal renderer. The current package includes the engine, Python and runtime libraries; playing does not require Wine, CrossOver, Xcode or Homebrew. It is an experimental build: the player has reached the menu, sea, deck and prison scenes, while broader gameplay and cross-machine testing continues.

## Play

Requires **Apple Silicon and macOS 15 or newer**. Intel Macs are unsupported.

1. Download `Corsairs-Iddictive-Remaster-macOS.zip` from the Google Drive folder.
2. Extract it and move `Corsairs Iddictive Remaster.app` to Applications.
3. Open the app. Saves and settings stay in `~/Library/Application Support/Iddictive Corsairs`, independently of the app, so replacing the app preserves your progress.

The application and nested dependencies are ad-hoc signed; the package is not notarized. Resolution changes require a game restart if the display develops borders or incomplete interface graphics. The original unsigned package had a CoreAudio/TCC startup stall; the native launcher and complete bundle signature corrected the observed packaging defect in the subsequent player replay.

## What changed

- Native Metal rendering, lighting and shadow work, weather-driven sea and sky, FXAA, texture delivery and GPU skinning.
- Walking on your ship’s deck, with ship motion and deck contact.
- Crime, reputation, surrender and custody changes.
- Shared squad supplies and cabin officer equipment adapters.
- Progression balancing, per-hit firearm damage limits and consistent trade-journal units.
- A self-contained macOS app, external player state and a revised main menu.

The case explains the changes with screenshots. These source and runtime features have different playtesting coverage; an installed patch is not a full gameplay acceptance claim.

## Build

Requires Apple Silicon, macOS 15+, Xcode command-line tools and Python 3.10+. Install the downloadable app first: it supplies game assets for local development staging.

```sh
git clone https://github.com/iddictive/Corsairs-Remaster.git
cd Corsairs-Remaster
curl -fL -o build-inputs.tar.gz \
  https://github.com/iddictive/Corsairs-Remaster/releases/download/experimental-metal/Corsairs-Metal-build-inputs.tar.gz
tar -xzf build-inputs.tar.gz -C experiments/native-metal
python3 tools/prepare_metal_inputs.py --check
experiments/native-metal/run.sh --stage-only
experiments/native-metal/run.sh --launch-installed
```

The input archive pins engine source, headers, dependencies, tools and reviewed originals. Its SHA-256 is `2e7f68269941db86aef14a4130a91054c59ac473b34119333df30f4efbd877a8`. It is not the full game download.

Development staging has separate saves and never imports or overwrites the installed app’s saves. [Development instructions](docs/DEVELOPMENT.md) explain the owners, staging and packaging commands.

## Scope and attribution

`experiments/native-metal` is the supported runtime. `experiments/native-storm` retains source adapters required by the build; its old runtime is unsupported. Original engine source and dependency notices are included in the pinned inputs. This repository does not grant ownership of third-party game content.
