# Development

## Owners

`experiments/native-metal/engine` owns ordinary C++ engine edits, its CMake
definition and upstream GPLv3 license. The initial import preserves947 source,
technique and CMake inputs byte-for-byte from verified batch `69f1dd4`; three
patch backup files were excluded. `build.sh` compiles the tracked sources
directly. It never extracts `engine.tar`, applies patches or rewrites sources.
The former patch stack remains in Git history; `.cache/storm` is historical
evidence. `run.sh` owns staging and launch. `backend.mm` and adjacent Metal
headers own rendering. `src/gameplay` owns every managed complete script and
interface definition; `tools/gameplay_sources.py` projects them to runtime files.
`tools/sync_metal_gameplay.py` delivers these files and prepared assets directly;
`metal_graphics_settings.py` owns settings and the launch environment.

Captured `experiments/native-metal/inputs` own dependency headers, baseline
game data and toolchain. Existing payloads remain compatible; their historical
engine archive, patch originals and baseline build files are unused. Keep inputs and generated
`.cache` out of Git. `tools/metal_engine_source.py` owns compiler readiness from
the real engine/renderer input fingerprint and built/staged binary hashes.
An edit after a build invalidates readiness; a build never rolls it back.

## Edit and stage

Close the game before staging. Edit tracked engine files directly; do not
replace complete PROGRAM or RESOURCE trees. Unknown runtime edits require review.

```sh
python3 tools/prepare_metal_inputs.py --check
experiments/native-metal/run.sh --stage-only
python3 tools/sync_metal_gameplay.py check
```

For a complete update, close the game and run `experiments/native-metal/run.sh --stage-only` once. It builds directly from tracked engine/renderer sources, stages reviewed scripts and techniques, and updates the installed app when present. Then reopen the game. Unknown runtime or installed-app edits stop delivery for review rather than being overwritten; a signing failure is an error, not a successful install.

`src/assets/video/manifest.json` owns15 original WMV identities and their H.264/AAC
recipe. Canonical staging prepares `.wmv.mp4` siblings with FFmpeg once, then
reuses SHA-verified cached outputs. Original videos remain intact; AVFoundation
plays the derived files with the original script IDs, aspect, audio and completion
events. Exact movie paths join ordinary delivery receipts/export. FFmpeg is a
preparation tool, not a shipped runtime dependency. The startup-video preference
still controls startup; story videos are no longer skipped by the historical
Wine workaround.

Puerto Rico's sea-town candidate uses
`tools/prepare_puerto_rico_city.py` before ordinary content synchronization.
`src/assets/islands/puerto-rico/{layout,manifest}.json` pin placement, stock
inputs and14 exact outputs; generators remain source and GM/PTC outputs remain
in the native cache. Delivery receipts adopt the two stock island predecessors
and track new children/INI/PTC without replacing resource trees. The native
SeaTown factory is required by `sea.c`; its inhabitants are transient and
recreated on island load. The fort/locators/seabed are protected inputs.
See `docs/puerto-rico-city.md` for candidate status and accepting replay.

The installer also delivers the built `resource/shared/messages.h` to the installed app and development runtime in the same rollback transaction as the engine. The script VM reads this header during startup; updating the engine and PROGRAM alone can leave new command names undefined and abort before the menu. Header compilation checks must use the delivered app header, not the source checkout.

`tools/delivery_state.py` owns delivery receipts, admission, atomic rollback and outer-bundle sealing. The installed receipt is `Contents/Resources/.delivery-state.json`; runtime staging records its own `.cache/runtime/.delivery-state.json`. Native staging, script delivery and fresh export record every managed file, including an unchanged file's first adoption. Later versions accept recorded predecessors automatically. The source manifests freeze admission for installations predating receipts; do not extend them per release. Prepared assets validate their manifests independently of destination admission. Missing or corrupt ownership evidence never authorizes an unknown edit. Receipt paths exclude SAVE, settings and logs.

The dev helper uses the same transaction and player lock. It records pushed edits as development-owned until reverted or incorporated into canonical content, so normal staging refuses to overwrite them and a noop preserves their owner. Unchanged delivery skips runtime/app writes and outer-bundle signing. A bundle move or checkout-cache cleanup preserves the installed receipt; a fresh exported app carries the validated staging content and source identities into its receipt after nested code is signed and before the outer seal.

For settings UI work only, `experiments/native-metal/run.sh --settings-hmr` watches the existing settings adapter. Close and reopen Settings after a sync. Only `option_sl.c`, `option_screen.c`, and `option_screen.ini` are hot-delivered. General gameplay scripts, textures and technique changes use normal staging and restart; engine code and embedded Metal shaders also need the native rebuild. Do not run normal staging alongside the settings watcher.

Use `python3 tools/sync_metal_gameplay.py apply` for a reviewed script-only delivery. Staging receipts prove build and delivery, not interactive gameplay. Replay the changed action on a suitable save and check the nearest unaffected action before calling a feature accepted.

Edit registered files directly in `src/gameplay`. New text sources use UTF-8 with LF endings; existing unchanged sources retain their bytes. The manifest declares each file's runtime encoding, including CP1251 resources. Script/header/INI projection restores CRLF. To deliver one independent file:

```sh
python3 tools/sync_metal_gameplay.py apply --path PROGRAM/weather/Init/Evening.c
```

The manifest registers paths and frozen pre-receipt bootstrap hashes. Do not add a hash for each new source revision: the delivered predecessor is recorded automatically. A new registered file uses an empty bootstrap list; an existing unrecorded destination with different bytes needs explicit provenance before its first delivery. Missing recorded files and unknown edits reject. Content-addressed backups preserve replaced bytes. Full staging consumes the same sources, and export rejects an unstaged source before copying the app.

The hero dialogue has two existing product variants, declared in `src/gameplay/variants.json`: standard and boarding-loot controls. Their complete source files, including military callbacks, live in `src/gameplay/variants/mainhero`. Initial adoption matches the current source or frozen reviewed predecessors, then the receipt retains its relative source identity. Edit the selected source and use ordinary full script delivery; later edits and fresh exports need no added historical hashes. A delivery does not switch variants. Variant sources remain outside the single-file `--path` and dev `promote` contracts.

`gameplay/` remains a disposable installed-content workspace. After a snapshotted dev edit, `python3 tools/dev_runtime.py promote PROGRAM/weather/Init/Evening.c` moves a registered file's edit into its canonical source, stages cache and installed bytes, and reconciles the dev snapshot in one rollback transaction. It refuses a changed canonical predecessor or unknown installed bytes. Commit the resulting source diff normally. Unregistered base content remains outside promotion.

Normal build, staging and launch never rewrite script source or apply text patches.
Town backdrop definitions are complete files under `src/gameplay/PROGRAM/locations/init`.
`layout-assets.json` binds physical fort/land locators used by the authored scripts.
The only generated text is the in-game changelog: `changelog_content.py` projects
`docs/changelog-ru.md` above `src/gameplay/changelog-original.txt`. Model, texture,
audio and movie preparation retain their existing asset manifests. Historical
Windows/native-storm patch tools are outside the supported Metal source workflow.

## Package an app

```sh
python3 tools/export_metal_app.py --check
python3 tools/export_metal_app.py --output "dist/Corsairs Iddictive Remaster.app"
codesign --verify --deep --strict "dist/Corsairs Iddictive Remaster.app"
```

The exporter includes runtime dependencies and game assets, uses the native launcher and signs the complete bundle. It excludes saves, userdata, logs and repository source. Player state remains external to the replaceable app. Test the archive after native extraction into a different path; inspect library closure and strict signatures before distributing it.

For a distributable ZIP, use `tools/package_metal_release.py` with `--output`
pointing to a new ZIP in `dist` and `--installation-text` pointing to the reviewed
player instructions. This producer exports directly into temporary packaging
storage, preserves native ZIP64 metadata, verifies native extraction, signatures
and relocated staging with an isolated profile, then retains only the ZIP and
compact receipt. Payload/extracted apps and intermediate archives are removed on
success, ordinary failure and SIGTERM. A full export/extraction replay of this
new orchestration remains pending; syntax and the shared cleanup lifecycle have
been checked. The installed player app remains the only supported game instance.

## Verification and troubleshooting

Normal staging builds only the engine. Diagnostic executables remain available as explicit targets; for example, `cmake --build experiments/native-metal/.cache/build --target gpu-skinning-parity-probe` followed by `experiments/native-metal/.cache/build/bin/gpu-skinning-parity-probe`. Run only the diagnostics relevant to the change.

Existing focused probes live beside the native renderer and interface adapters. Run the relevant probe for your change and stage through `run.sh`; do not infer visible correctness from compilation alone.

The installed application writes launcher output to `~/Library/Application Support/Iddictive Corsairs/logs/launch.log` and rotates the previous session to `launch.log.1` at every start, so player disk usage stays bounded to the last two runs. Staging syncs the reviewed launcher resource into the played app the same way it syncs the engine. Preserve SAVE before manually editing profile data. Restart after a resolution change if interface resources or screen proportions appear incomplete.

The supported platform is Apple Silicon / macOS 15+. Engine/library deployment floors and a successful local replay do not prove every other Mac configuration works. Ship candidate behavior as experimental until its actual scene and compatibility checks pass.
