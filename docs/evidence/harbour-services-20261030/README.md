# Direct sea port services

The player remains in the sea scene when selecting Store or Shipyard. The
commands follow Moor in that order and reuse the existing town-service icons
and interfaces. Store stock, prices, money, cargo, repairs and ship purchases
retain their ordinary owners. No merchant entity is manufactured and no
automatic walk or shore reload precedes the interface.

Admission uses the actual currently accessible sea reload locator, town
fast-travel table and existing path/door checks. The same landing disease/quest
predicate serves ordinary Moor and both shortcuts. Night closure, merchant
schedule, disabled doors, hostile towns, held harbours, combat, boarding,
interface/dialogue locks and a different quay remain negative cases.

After shipyard changes, the explicit interface-exit callback reconciles only
changed player hulls and sold/new companions. Unchanged hulls retain their native
actors. A changed live hull retains physical position, heading, task and camera;
the native mounted replay is the acceptance owner for model, rigging and cannon
state. Buying an empty-slot companion uses ordinary group admission and its safe
start-position check. Selling the final player hull returns the captain to the
actual quay; a rejected native refresh uses that same recoverable shore path.

`services-replay.py.txt` executes the actual service script, command registry,
landing admission, fast-path resolver, door and login-time predicates in the
existing script VM. `services-result.json` records the result. Native UI launch,
actor presence, finite fixture identifiers/relations and reconciliation messages
are explicit receipt seams. It covers direct dispatch without a location change,
the availability negatives above, unchanged close, repair, sold companion,
empty-slot purchase, final-hull sale and rejected native refresh. The native
case-insensitive location resolver is a fixture seam; production compares its
canonical indices because authored merchant/location capitalization differs.

`return-replay.py.txt` reproduces the original own-quay rejection after enabling
town alarm, then exercises the same Reload pre-fader boundary after repair.
`return-before.json` and `return-after.json` retain both outcomes. Exact berth,
held harbour, quest/reload locks, boarding, missing/sunk hull and interior
negatives remain. Its final pass also compiles the complete canonical PROGRAM
projection against the current engine's shared headers.

Run either procedure from the repository root through the existing managed
scratch runner: 1 GiB, 20,000 entries, 180 seconds and 10 GiB free-space reserve.
The scripts require `CODEX_SCRATCH_DIR`, the prepared project runtime and built
`script-vm-probe`. The native `live-hulls-replay.py` documents its larger bounded
component budget. No procedure opens the game UI, changes a real SAVE or installs
the app. Script/native component evidence does not claim rendered menu or live
gameplay acceptance.

The mounted replay passed 22 native assertions with empty error logs: authored
Manowar1 to Lugger1 geometry/mass, retained pose/velocity/sails/task/camera,
same-type repair, preflight and post-mount failure rollback, sold-companion
registry/borrower cleanup, and old bombs versus new balls/powder conservation.
Its operation/negative batch took 34.055 ms warmed; this is not a frame-time or
whole-game FPS measurement. The initial fixture used the unsuffixed catalogue
name rather than the generated RealShip model; that rejected input is retained
separately as `live-hulls-before-resource-projection.json`.

Speed scope: service lookup runs only when the command menu is requested;
reconciliation runs only after an explicit shipyard operation. No new sea/world
simulation cadence or per-frame actor scan is introduced. The temporary exit
handler waits only for an existing repair fade to finish. Native geometry
rebuild cost belongs to that explicit changed-hull operation.
