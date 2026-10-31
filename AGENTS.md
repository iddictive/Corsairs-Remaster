# Agent instructions for Corsairs

## Gameplay source ownership

**The only place to edit gameplay scripts, headers and INI files is `src/gameplay/`.**

Never edit files directly in:
- `experiments/native-metal/.cache/runtime/` — generated runtime copy (CRLF/CP1251), gitignored
- `/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/` — installed app copy
- `experiments/native-metal/inputs/gameplay/` — frozen historical snapshot

After editing `src/gameplay/`, deliver changes with:
```sh
cd tools && python3 sync_metal_gameplay.py apply
```

The delivery pipeline converts encoding (UTF-8 → CP1251 where declared in
`manifest.json`) and line endings (LF → CRLF for .c/.h/.ini/.txt), validates
delivery-state hashes, updates both the dev cache and the installed app, and
re-signs the app bundle. Zero pending in both targets is the only acceptable
end state.

Do not update delivery-state JSON files by hand. If sync fails with
"Unknown delivered-file edit", it means someone edited a runtime copy directly —
find the actual change, port it to `src/gameplay/`, and re-run sync.

## Build and stage

See `docs/DEVELOPMENT.md` for the full workflow. Key commands:
```sh
python3 tools/prepare_metal_inputs.py --check
experiments/native-metal/run.sh --stage-only        # engine + full staging
python3 tools/sync_metal_gameplay.py apply           # script-only delivery
python3 tools/sync_metal_gameplay.py apply --path PROGRAM/store/initGoods.c  # single file
```

## Evidence and cache

`experiments/native-metal/.cache/` is disposable generated output — never commit it.
`experiments/native-metal/inputs/` is frozen build input — never modify it.
`docs/evidence/` is retained investigation evidence — read-only unless extending.
