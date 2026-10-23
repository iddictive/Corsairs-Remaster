# Mooring after world-map travel — 2026-10-23

The player reported being unable to moor after reaching a port via the map.
`Ship_CheckMainCharacter` compared the ship's current sea-world position with
the raw island-local reload locator. Native `LOCATOR::LocateForI` publishes
`label.m[3][0/2]` unchanged. The resident atlas relocates the physical island
and player into the common sea frame. Fort registration and existing sea
teleport distance checks already apply `Sea_RegionX/Z`; mooring omitted it.
This proves a coordinate mismatch, although the player's exact preceding save,
port and route were not replayed.

The repair obtains the existing `Sea_RegionFrame` once for the current island
and transforms reload points with its canonical point functions. Raw reload
attributes and land destinations remain unchanged. Unframed sea uses identity;
legacy resident translation and rotated atlas frames use their existing owner.
Enemy-distance, fort, global/local lock and radius admission remain intact.
No sea/map admission, fleet, collision, save or travel policy is replaced.

## Reproduce the bounded CPU observation

Set `TASK_SCRATCH_DIR` to a disposable directory, then run from repository root:

```sh
python3 docs/evidence/port-mooring-20261023/replay.py
```

The script selects the actual mooring block from `AIShip.c` and actual frame
helpers from `worldmap_coords.c`. It runs them through the existing real script
VM, with surrounding admission callbacks stubbed. The pre-repair source is
bound to `26b8405a1e58b8ef1511621a8c915b9b5356c3ad`. Ordinary unframed sea and
the required world-map origin attributes are seeded before evaluating the
resident states. Expected points are independently specified: translation
`(100,200) -> (10100,-19800)` and rotation/translation to `(9800,-19900)`.

[Retained results](results.json): baseline `Run=2` rejects the translated port;
current `Run=0`, both with zero script errors. Current also preserves ordinary
sea, rejects outside/exact-radius points, nearby enemies, global/local locks,
fort locators and missing coordinates, while publishing the correct port/land
destination on success. [Initial baseline](baseline.json) predates the edit;
its benchmark used an unframed sea and is not the atlas timing comparison.

The comparable benchmark uses 27 resident regions, the matching region last,
one reload and 10000 far-position admission calls. Baseline/current whole VM
process wall time was 0.0441/0.2583 s: approximately 0.0214 ms added per call.
The actual owner schedules this check once per second; it creates one frame
per check rather than searching regions for every locator. These component
measurements do not establish full-game frame pacing or visual acceptance.

VM SHA-256: `b3e137d5c3408d8e7a5f0b822c93918d69e63bc8e13e1dbbfab6ddfe50b3528d`.
Repaired canonical `AIShip.c` SHA-256:
`238839416f1df0d2dc1caeb74060f547cef4650c4ea120badc8398dfb122cba5`.
Host: Python 3.14.7, arm64 Release VM built with Apple Clang 21/macOS 15 target.

Targeted delivery completed with `python3 tools/sync_metal_gameplay.py apply
--path PROGRAM/sea_ai/AIShip.c`. Runtime and installed bytes match the canonical
CP1251/CRLF projection, SHA-256
`4c054270dd432a13465722d38de5159d72413a9d5b6955e01c79b6ed2667fa44`.
The follow-up targeted check reports zero pending files. The ordinary delivery
transaction sealed and verified the app; no engine rebuild was needed.
Player mooring replay remains a separate acceptance layer; the game was not
launched or controlled for this investigation.
