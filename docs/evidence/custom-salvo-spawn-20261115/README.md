# Custom salvo and spawn controls

## Confirmed defects and repair
- Spawn.Volley (1..16) and FireInterval (0.5..20s) reach the engine correctly, but every custom round was delayed by an unrelated randomized0.2–0.35s. A16-round salvo took3–5.25s before frame quantization. Custom loadouts now attempt their bounded salvo in one visit; randomized traffic keeps the stagger.
- The64-live-missile-group cap formerly truncated the requested pending count. Pending rounds are now retained until capacity becomes available. This does not remove the cap or guarantee32 launches/s indefinitely. Missing targets and rejected routes still consume attempts.
- Spawn distances were hardcoded8250m(enemy)/600m(ally). The existing range slider is reused in the ship section: Auto preserves those defaults; explicit250–20000m applies to either side, without changing random/custom loadout selection. Existing safe-position search remains authoritative and may reject unavailable placement.

## Verification
From repo root:
```
python3 docs/evidence/custom-salvo-spawn-20261115/salvo_probe.py
python3 docs/evidence/custom-salvo-spawn-20261115/spawn_probe.py
python3 docs/evidence/volley-target-type-20261115/probe.py --repo .
```
The first probe compiles extracted scheduler expressions in a C++17 CPU harness: custom16, legacy spacing, cap/pending/resume, singleton. The second extracts the actual script radius helper into a C++17 harness: defaults, bounds,80slider steps and source-binding assertions. These are not full engine/script-runtime tests. The retained crash regression verifies the checked SHIP cast remains intact.

## Risks and acceptance still open
Custom launch work is bounded to16attempts per visited ship, at most8ships per controller frame. Terrain routing and visual emission may spike; GPU/frame-time results are not available. Timer is start-to-start, but capacity, target validity, terrain checks and scheduler visit cadence can delay launch. Native macOS/Metal compilation, actual debugger layout/interaction and game replay remain user-side. Source publication is not installation/staging; gameplay files must go through normal build/stage pipeline. No app/runtime/generated-copy edits were made.
