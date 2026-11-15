# Fort-target regression, 2026-11-15

Base: 46d951019e0aa34fe0cdef15bd0bd052096dad60. Integrated with the corrected staggered salvo scheduler.

## Confirmed cause
`AI_FORT::Load` calls `ResetPhonkOrdnance`, incrementing `phonkOrdnanceGeneration`. Target selection includes a live hostile fort and stores its MODEL entity ID. Launch calls `fortOrdnanceOwner(id, 0)`, whose strict generation match rejects every loaded fort with generation > 0. No SHIP exists at that MODEL ID, so both valid-target flags are false and each queued round is silently consumed without launch.

Fix: store fort generation alongside the selected target, pass it to the existing strict resolver. Clear stored fort generation on ship selection. Do not weaken lookup into a generation wildcard: a fort reset during pending salvo must still invalidate it.

`2c4097f` fixed unsafe SHIP virtual dispatch for a fort MODEL but did not fix the hardcoded generation. `46d951` changed scheduler capacity/timing, not this resolver. This is a pre-existing independent failure exposed after load, not proof that every fresh-map targeting failure has the same cause.

## Verification
`python3 docs/evidence/fort-target-generation-20261115/probe.py` from repo root.
Compiles extracted production target-selection block and actual `fortOrdnanceOwner` implementation against a small fixture using C++17 -Wall -Wextra -Werror -fsanitize=undefined.
Candidate PASS: fresh fort generation0; loaded generation1; stale queued generation rejected after another reset; next selection succeeds at new generation; missing character/model/fort rejected; ship selection unaffected.
Unmodified base FAIL on loaded generation1 assertion (expected mutation/baseline failure).

## Pipeline review, not runtime evidence
- Debug registered ships run in player's active ordnance controller, at most8 ships/frame, with elapsed accumulation. Dead/unregistered shooters clear pending rounds.
- Both hostile ships and live normal-mode forts are eligible. Fort hostility uses `Helper.isEnemy` -> safe relation table, with surrender semantics; missing relation entries are neutral. No unjustified force-enemy change made.
- Debug salvo target collection has no extra fort-specific range filter. Ship normal auto-target path separately uses hostileTargets(range), which includes undamaged fort gun locations.
- Fort target point uses first cannon position +10m and randomized warhead coverage spread. Ship route uses no ship receiver for fort and tests island collision; land obstruction can lift Kalibr by250/600/1200m, otherwise rejects. Real fort geometry may cause additional rejected trajectories; not reproduced in a scene, so not changed or claimed fixed.
- Nearby blast safety and terrain checks retained.

## Limits / acceptance
No macOS Metal build or game runtime available in this Linux work environment. User should verify a loaded save with a hostile live normal-mode fort and a debug missile ship, then fresh map, checking missiles launch toward the fort and other hostile targets continue working. Cannot promise every fort is hit; range/terrain/relations can legitimately prevent an attack. No release performed.
