# AI approach to islands

The requested behavior is for AI ships to avoid knowingly blocked routes and
retain obstacle braking across controller update boundaries. This is separate
from the physical recovery of a hull that is already grounded or embedded.

## Proven defects and changes

- `ISLAND::LocalMovePoint` copied the requested destination to the fallback
  result before normalizing its Y coordinate to the sea trace plane. An
  ordinary AI request starts at Y0, so the missing-detour equality check then
  compared Y0 with Y0.1 and skipped the existing ocean route fallback. Normalize
  the plane first, then initialize the default result.
- `AIShipMoveController::Move` accepted a blocked route even when the result
  retained the original destination. It now stops and holds its present point
  in that case. A real detour is retained; clear legs and scenes without an
  island retain direct travel. The existing two-second planning cadence still
  runs while stopped, so a later successful plan resumes travel normally.
- `AIShipTouchController::Execute` applied its speed multiplier only on cached
  frames. The speed controller can commit its two-second sail update on the
  scan frame itself, causing full sails even when the newly observed obstacle
  requires zero. Apply the multiplier after both branches.

## Native component observations

[Raw observations](observations.json) bind the baseline revision, candidate
controller hashes and output. The [frozen disposable producer](replay.py.txt)
extracts the actual controller functions and sea-plane setup, supplies stubbed
ship/trace inputs, compiles C++20 at `-O2` and observes their decisions. It does
not render, run gameplay, load a save or register a maintained test target.

The independent contracts are: a declared obstruction without a detour must not
become a travel command, and obstacle braking must hold on the same frame as a
sail-state commit. Baseline output shows `stopped=0,target=100` for the blocked
leg and `factor=0,committed=1` for scan-frame braking. Candidate output shows
`stopped=1,target=0` and `factor=0,committed=0`. The missing-route gate becomes
reachable. Real detours, unobstructed legs, no-island travel and the cached
planning interval retain their outputs. A seeded valid route is invalidated to
a hold and then resumes after the next allowed successful plan.

To reproduce from the repository root, supply an empty task-owned temporary
directory through `CODEX_SCRATCH_DIR` and run `python3 replay.py.txt [revision]`.
Omitting the revision reads the current sources; the baseline is
`e106aac45f510eca8208e61a0fdd12b45a6c02c9`. The retained source adds the seeded
retry cases after the original baseline run, as the raw observations state.
Native targets `sea_ai` and `island` compile successfully through the configured
build. Only existing deprecation warnings occur.

## Cost and acceptance limits

Planning keeps its two-second cadence. The touch correction adds one scalar
multiply only on existing scan frames; the hold decision adds two XZ equality
comparisons on blocked plans. Neither adds terrain queries, allocation or
per-frame graph building. The repaired sea-plane setup can activate the existing
loaded ocean PTC fallback in a SeaAtlas scene where it was previously skipped;
that restored search work is not a measured whole-scene speed improvement.

These observations prove the controller decisions and compilation, not complete
battle avoidance, physical recovery from an existing overlap, or the exact
reported gameplay scene. No recovery teleport, custom route search, new save
field, controller cadence change or player steering change is introduced.
