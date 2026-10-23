# Development

## Owners

`experiments/native-metal/engine` owns ordinary C++ engine edits, its CMake
definition and upstream GPLv3 license. The initial import preserves947 source,
technique and CMake inputs byte-for-byte from verified batch `69f1dd4`; three
patch backup files were excluded. `build.sh` compiles the tracked sources
directly and stages the verified unsigned executable for script delivery. It
never extracts `engine.tar`, applies patches or rewrites sources.
The former patch stack remains in Git history; `.cache/storm` is historical
evidence. `run.sh` owns resource staging and launch. `backend.mm` and adjacent Metal
headers own rendering. `src/gameplay` owns every managed complete script and
interface definition; `tools/gameplay_sources.py` projects them to runtime files.
`tools/sync_metal_gameplay.py` delivers these files and prepared assets directly;
`metal_graphics_settings.py` owns settings and the launch environment.

Captured `experiments/native-metal/inputs` own dependency headers, baseline
game data and toolchain. Existing payloads remain compatible; their historical
engine archive, patch originals and baseline build files are unused. Keep inputs and generated
`.cache` out of Git. `tools/metal_engine_source.py` owns compiler readiness from
the real engine/renderer input fingerprint and built/staged binary hashes.
Finder `.DS_Store` metadata is excluded: browsing the source tree must not
invalidate an otherwise unchanged compilation or interrupt installation.
An edit after a build invalidates readiness; a build never rolls it back.

## Rebuild on another Mac

Use the Git source, matching complete build-input archive and the standalone
game app. The app supplies the full game resources; player SAVE/settings remain
in their separate profile. Install the app in `/Applications`, extract the input
archive into `experiments/native-metal` so that it contains `inputs/`, then run:

```sh
python3 tools/prepare_metal_inputs.py --check
experiments/native-metal/run.sh --stage-only
```

With no development runtime, the normal launcher calls
`tools/restore_metal_runtime.py`. It restores reviewed material originals and
retains validated PROGRAM/RESOURCE delivery receipts, excluding app-only launcher
files and the executable, so
the first source upgrade can admit the installed predecessor. It creates empty
development SAVE and never imports the played profile.

To retain the current complete inputs before moving machines:

```sh
python3 tools/prepare_metal_inputs.py --export dist/Corsairs-Metal-build-inputs.tar.gz
```

Export refuses an occupied destination and excludes vendor Git administration
metadata. SDK pkg-config prefixes resolve relative to their copied metadata,
so they do not retain the capturing Mac's path. It includes the captured build inputs,
city/rowboat originals verified against their owning recipes, and the verified
prepared movie cache and music. Restoration reuses that media for its matching
source recipe; an older payload without prepared media still needs FFmpeg and
the pinned music download. The receiving build requires Apple Silicon/macOS15+,
full selected Xcode/Metal SDK and Python3.10+; CMake/Ninja/SDL are in the inputs.
The game app itself stays on the established Drive download; source and required
build inputs have their separate GitHub owners.

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

When its compiler check reports `build/stage pending`, `experiments/native-metal/build.sh`
refreshes both the build receipt and staged executable without launching the game
or running diagnostic probes. Then repeat the targeted script delivery.

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
pointing directly to its explicit handoff destination. A local handoff may use
only `dist/Corsairs-Iddictive-Remaster-macOS.zip`; versioned release trees are
rejected. Remove the local ZIP after its handoff is complete, retaining the compact
receipt. Player instructions come from
`docs/install-ru.txt`; `--installation-text` can select a reviewed alternative.
This producer exports directly into temporary packaging
storage under `dist`, preserves native ZIP64 metadata, verifies native extraction, signatures
and relocated staging with an isolated profile, then retains only the ZIP and
compact receipt. Payload/extracted apps and intermediate archives are removed on
success, ordinary failure and SIGTERM. ZIP rewriting excludes volatile Finder
`.DS_Store` files and their AppleDouble records. Cleanup permits one late Finder
file in its own root; any other newly created file remains an error. The focused
native metadata/cleanup probe and full frozen23ac release replay pass: native
extraction, complete payload comparison, strict signatures and isolated relocated
staging. This does not accept the player's gameplay reports.
Only the verified final ZIP is written to an external destination; packaging
payloads never enter a cloud-synced export directory. Cross-volume delivery copies
the verified ZIP with exclusive output ownership and removes a failed partial
copy. The installed player app remains the only supported game instance. See
`dev-runtime.md` for source, cache and retained-evidence storage roles.

### Why the ZIP producer keeps a frozen payload

The historical packaging investigation verified this route with macOS system
libarchive 3.7.4. Native `ditto` supplies AppleDouble metadata; the rewriter
preserves those members and recovers POSIX modes and symlink identity from the
unchanged signed payload. The streaming reader alone cannot recover the native
central directory's symlink modes.

Three simpler routes failed: publishing the intermediate `ditto` ZIP left
ZIP32 end records wrapping above 4 GiB or 65,535 members; bare libarchive ZIP
creation omitted required AppleDouble metadata; forcing local ZIP64 headers
broke native `ditto` extraction of small members. Let libarchive select ZIP64
automatically, and keep the payload unchanged until extraction, metadata/link
comparison and strict signature verification finish. A changed producer or
library needs a bounded native metadata/link check before a full release.

## Technical investigations

Archived findings with continuing maintenance value are retained in
[renderer lifetime and material-cut investigations](metal-renderer-investigations.md) and the frozen
[San Juan port investigation](seamless-san-juan.md#historical-message-and-old-save-regressions).
The [attribute-tree lifetime and lookup contract](attribute-class-lifetime.md)
records the independent parent-chain and non-mutating lookup failures.
The [F2 section dispatch evidence](evidence/menu-sections-20261023/README.md)
records the stale deferred-target reproduction and bounded CPU method check.
The [spatial audio range evidence](evidence/spatial-audio-20261023/README.md)
records the distant-collision mixer reproduction and finite-range repair.
The [world-map mooring evidence](evidence/port-mooring-20261023/README.md)
records local/world reload coordinates and the retained admission guards.

## Verification and troubleshooting

Normal staging builds only the engine. Diagnostic executables remain available as explicit targets; for example, `cmake --build experiments/native-metal/.cache/build --target gpu-skinning-parity-probe` followed by `experiments/native-metal/.cache/build/bin/gpu-skinning-parity-probe`. Run only the diagnostics relevant to the change.

Existing focused probes live beside the native renderer and interface adapters. Run the relevant probe for your change and stage through `run.sh`; do not infer visible correctness from compilation alone.

The installed application writes launcher output to `~/Library/Application Support/Iddictive Corsairs/logs/launch.log` and rotates the previous session to `launch.log.1` at every start, so player disk usage stays bounded to the last two runs. Staging syncs the reviewed launcher resource into the played app the same way it syncs the engine. Preserve SAVE before manually editing profile data. Restart after a resolution change if interface resources or screen proportions appear incomplete.

The supported platform is Apple Silicon / macOS 15+. Engine/library deployment floors and a successful local replay do not prove every other Mac configuration works. Ship candidate behavior as experimental until its actual scene and compatibility checks pass.
