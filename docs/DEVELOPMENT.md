# Development

## Owners

`experiments/native-metal/build.sh` owns the ordered engine patch stack and canonical build. `run.sh` owns staging and launch. `backend.mm` and the Metal shader headers own rendering. `tools/sync_metal_gameplay.py` owns reviewed gameplay delivery; `metal_graphics_settings.py` and `metal_menu_branding.py` own their exact-hash interface transforms.

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

For settings UI work only, `experiments/native-metal/run.sh --settings-hmr` watches the existing settings adapter. Close and reopen Settings after a sync. Only `option_sl.c`, `option_screen.c`, and `option_screen.ini` are hot-delivered. General gameplay scripts, textures and technique changes use normal staging and restart; engine code and embedded Metal shaders also need the native rebuild. Do not run normal staging alongside the settings watcher.

Use `python3 tools/sync_metal_gameplay.py apply` for a reviewed script-only delivery. Staging receipts prove build and delivery, not interactive gameplay. Replay the changed action on a suitable save and check the nearest unaffected action before calling a feature accepted.

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
