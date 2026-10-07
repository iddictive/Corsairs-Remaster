# Development

## Owners

`experiments/native-metal/build.sh` owns the ordered engine patch stack and canonical build. `run.sh` owns staging and launch. `backend.mm` and the Metal shader headers own rendering. `src/gameplay` owns complete editable scripts and interface definitions registered in its manifest, including the trade journal, main menu, graphics settings and independently retained sea/ship/fort behavior; `tools/gameplay_sources.py` projects them to runtime files. `tools/sync_metal_gameplay.py` delivers these sources and the remaining domain generators; `metal_graphics_settings.py` owns runtime settings synchronization and launch environment.

Captured `experiments/native-metal/inputs` are the portable build dependency owner. The manifest verifies pinned engine source, baseline scripts, headers and toolchain. Builds with those inputs do not depend on the original developer’s machine or old engine checkout. Keep inputs and generated `.cache` out of Git.

## Edit and stage

Stop the development game before rebuilding or staging. Preserve the ordered patch stack; do not replace complete PROGRAM or RESOURCE trees. Unknown script bytes must be reviewed rather than overwritten.

```sh
python3 tools/prepare_metal_inputs.py --check
experiments/native-metal/run.sh --stage-only
python3 tools/sync_metal_gameplay.py check
```

For a complete update, close the game and run `experiments/native-metal/run.sh --stage-only` once. This applies the ordered engine patches, builds, stages the reviewed scripts and techniques, and updates the installed app when present. Then reopen the game. Unknown runtime or installed-app edits stop delivery for review rather than being overwritten; a signing failure is an error, not a successful install.

The installer also delivers the built `resource/shared/messages.h` to the installed app and development runtime in the same rollback transaction as the engine. The script VM reads this header during startup; updating the engine and PROGRAM alone can leave new command names undefined and abort before the menu. Header compilation checks must use the delivered app header, not the source checkout.

`tools/delivery_state.py` owns delivery receipts, admission, atomic rollback and outer-bundle sealing. The installed receipt is `Contents/Resources/.delivery-state.json`; runtime staging records its own `.cache/runtime/.delivery-state.json`. Native staging, script delivery and fresh export record every managed file, including an unchanged file's first adoption. Later versions accept recorded predecessors automatically. `tools/gameplay/delivery-bootstrap.json` and the source manifests freeze admission for installations predating receipts; do not extend them per release. Domain generators still validate their inputs, anchors and reviewed outputs independently of destination admission. Missing or corrupt ownership evidence never authorizes an unknown edit. Receipt paths exclude SAVE, settings, logs and assets.

The dev helper uses the same transaction and player lock. It records pushed edits as development-owned until reverted or incorporated into canonical content, so normal staging refuses to overwrite them and a noop preserves their owner. Unchanged delivery skips runtime/app writes and outer-bundle signing. A bundle move or checkout-cache cleanup preserves the installed receipt; a fresh exported app carries the validated staging content and source identities into its receipt after nested code is signed and before the outer seal.

For settings UI work only, `experiments/native-metal/run.sh --settings-hmr` watches the existing settings adapter. Close and reopen Settings after a sync. Only `option_sl.c`, `option_screen.c`, and `option_screen.ini` are hot-delivered. General gameplay scripts, textures and technique changes use normal staging and restart; engine code and embedded Metal shaders also need the native rebuild. Do not run normal staging alongside the settings watcher.

Use `python3 tools/sync_metal_gameplay.py apply` for a reviewed script-only delivery. Staging receipts prove build and delivery, not interactive gameplay. Replay the changed action on a suitable save and check the nearest unaffected action before calling a feature accepted.

Edit registered files directly in `src/gameplay`; they are UTF-8 source files with LF endings. The manifest declares each file's runtime encoding: scripts currently use UTF-8 and the trade-journal INI uses CP1251. Script/header/INI projection restores CRLF. To deliver one independent file without running unrelated legacy composers:

```sh
python3 tools/sync_metal_gameplay.py apply --path PROGRAM/weather/Init/Evening.c
```

The manifest registers paths and frozen pre-receipt bootstrap hashes. Do not add a hash for each new source revision: the delivered predecessor is recorded automatically. A new registered file uses an empty bootstrap list; an existing unrecorded destination with different bytes needs explicit provenance before its first delivery. Missing recorded files and unknown edits reject. Content-addressed backups preserve replaced bytes. Full staging consumes the same sources, and export rejects an unstaged source before copying the app.

The hero dialogue has two existing product variants, declared in `src/gameplay/variants.json`: standard and boarding-loot controls. Their complete source files live in `src/gameplay/variants/mainhero`; military callbacks compose on those sources once. Initial adoption identifies the existing variant from frozen reviewed bytes, then the receipt retains its relative source identity. Edit the selected source and use ordinary full script delivery; later edits and fresh exports need no added historical hashes. A delivery does not switch variants. These composed variant sources are outside the single-file `--path` and dev `promote` contracts.

`gameplay/` remains a disposable installed-content workspace. After a snapshotted dev edit, `python3 tools/dev_runtime.py promote PROGRAM/weather/Init/Evening.c` moves a registered file's edit into its canonical source, stages cache and installed bytes, and reconciles the dev snapshot in one rollback transaction. It refuses a changed canonical predecessor or unknown installed bytes. Commit the resulting source diff normally. Files still owned by legacy composers must be migrated with their owner before promotion; the helper does not create a second source of truth for them.

## Package an app

```sh
python3 tools/export_metal_app.py --check
python3 tools/export_metal_app.py --output "dist/Corsairs Iddictive Remaster.app"
codesign --verify --deep --strict "dist/Corsairs Iddictive Remaster.app"
```

The exporter includes runtime dependencies and game assets, uses the native launcher and signs the complete bundle. It excludes saves, userdata, logs and repository source. Player state remains external to the replaceable app. Test the archive after native extraction into a different path; inspect library closure and strict signatures before distributing it.

## Verification and troubleshooting

Normal staging builds only the engine. Diagnostic executables remain available as explicit targets; for example, `cmake --build experiments/native-metal/.cache/build --target gpu-skinning-parity-probe` followed by `experiments/native-metal/.cache/build/bin/gpu-skinning-parity-probe`. Run only the diagnostics relevant to the change.

Existing focused probes live beside the native renderer and interface adapters. Run the relevant probe for your change and stage through `run.sh`; do not infer visible correctness from compilation alone.

The installed application writes launcher output to `~/Library/Application Support/Iddictive Corsairs/logs/launch.log` and rotates the previous session to `launch.log.1` at every start, so player disk usage stays bounded to the last two runs. Staging syncs the reviewed launcher resource into the played app the same way it syncs the engine. Preserve SAVE before manually editing profile data. Restart after a resolution change if interface resources or screen proportions appear incomplete.

The supported platform is Apple Silicon / macOS 15+. Engine/library deployment floors and a successful local replay do not prove every other Mac configuration works. Ship candidate behavior as experimental until its actual scene and compatibility checks pass.
