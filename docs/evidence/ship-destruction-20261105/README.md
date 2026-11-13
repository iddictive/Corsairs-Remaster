# Ship plane-cut investigation

The full ship destruction goal remains open. A native offscreen geometry probe
uses the authored Brig1 hull (44,206 contact parts) and an already prepared
material state. A transverse central plane produced two raw surface fragments,
but the original material preparation rejected the cut in `trimReleasedCells`.
That function attempted to subtract the released half after `clipCells` had
already clipped the retained cells to the opposite half-space. The original
`CutByLaser` result and timings are in `baseline-output.txt`.

The source change skips that redundant subtraction for plane cuts only. In the
isolated diagnostic, a locally renamed copy of the modified `prepareExact`
prepared the first cut in 93.669 ms, staged it in 2.291 ms and committed
material revision 1 to 2. A second cut prepared in 75.645 ms and committed
revision 3. The first cut's retained cell vertices and released mesh vertices
were checked against the cut plane: maximum retained error 0.000000477 and
minimum released error -0.000000238; 70,632 and 41,064 vertices respectively.
That scan is a one-off geometry oracle, not per-frame physics work.
The retained candidate output is in `candidate-output.txt`.

The instrumented follow-up in `timing-output.txt` measured 60.657 ms for
structure seeding and 22.124 ms for encoding a 13,650,816-byte material
payload inside 93.803 ms total preparation. The separate raw geometry prepare
was 6.520 ms and cell clipping 5.345 ms; those stand-alone calls precede the
timed preparation and must not be summed as its subspans. This is a single
warm offscreen fixture, not frame time or a before/after optimization result.
The retained `replay.py ... timing` replay independently reproduced those
observations in `replay-output.txt`.
The `seed-profile` follow-up separately rebuilt contact material in 3.061 ms
and measured the coarse graph in 58.432 ms; these duplicate operations are
deliberately inside the enclosing prepare time (`seed-profile-output.txt`).

The `encode` probe compares an unordered vertex lookup with the existing
ordered lookup on the same retained plane-cut state. Three encoded outputs were
byte-identical; timing fell from 22.381–22.615 ms to 18.910–20.877 ms
(`encode-output.txt`). This isolated gain leaves the 60 ms structure rebuild
and full-path cost unresolved; peak-memory impact was not isolated. The
candidate was not adopted. Select it with `replay.py <scratch> encode`.

The `reconcile` probe compares the existing incremental structural reconciler
with the current full seed after each of two plane cuts. The first seed took
67.619 ms and incremental reconciliation 219.304 ms; the second took 55.785 ms
and 120.489 ms. Maximum first-cut cell/bond differences were 0.000000298 and
0.000001013; after the second cut they were 0.000504503 and 0.000021607,
with no changed alive flags. The comparisons cover coarse cell/bond values,
not contact projection or SAVE parity. Because the plane path currently marks
all authored cells as changed, this does not establish sparse-update cost for
local holes. The candidate was rejected for full plane cuts; full output is in
`reconcile-output.txt` and `replay.py <scratch> reconcile` reproduces it.

The `coarse` probe estimates retained graph health from a plane and the 417
existing coarse cell/bond bounding boxes. The estimation alone took 0.006 ms
on each cut, versus 87.591 and 68.739 ms for exact seeding in that run. It is
only an estimate: the worst cell error was 0.517 and 0.433, the worst bond
error 0.034 and 0.147, and the second cut disagreed on two alive flags. It
does not update the contact-material cache or save state and is not a runtime
implementation. It was rejected as a standalone replacement. The locality of
the plane (18 and 20 crossing cells) motivates testing a hybrid that keeps
unaffected cells and measures exact material only near the boundary; the
derived contact cache, previously damaged state, bond continuity, serialized
restore, and complete scene cost must still be addressed. Results and source
are `coarse-output.txt` and `coarse-probe.cpp.txt`.

The `hybrid-axis` and `hybrid-diagonal` prototypes classify the existing coarse
cells against a flat cutting plane, retain pre-existing state away from the
cut, construct the actual retained contact material, and measure tetrahedra
only around crossing cells and their bond neighbors. Against full seed on the
same captured material, the first axis-aligned cut measured 62.285 ms versus
18.607 ms; its second measured 50.563 ms versus 8.983 ms. The corresponding
diagonal cuts measured 61.902 versus 19.580 ms and 53.725 versus 24.913 ms.
The maximum observed cell/bond discrepancy was 0.000124156 and no alive flag
changed. These are same-process substage comparisons; full preparation timings
include both implementations and do not measure the candidate's full path.
The prototype uses the exact retained contact material but has not been used
as the live published state. Its input was a plane cut after a previously valid
cut, not an independently damaged hull, and rough planes take a full-seed
fallback. It has not been tested for contact queries, SAVE/restore, output
latency, interactions with other damage types or installed gameplay. Source and
raw outputs are retained as `hybrid-*`; select `hybrid-axis` or
`hybrid-diagonal` in `replay.py`.

The separate `hybrid-full` prototype actually publishes the hybrid structure
and uses it for the following cut; `hybrid-full-baseline` publishes the full
seed. On the same oblique fixture, preparation measured 52.609 versus 94.694 ms
for the first cut and 51.266 versus 79.425 ms for the second; both staged and
committed two revisions with matching fragment counts and plane-side checks.
These are separate, unpaired process runs in a synthetic GEOM fixture, not a
full ship/weapon/frame benchmark. The two outputs and source are retained under
those names. Actual contact/flood/saving behavior and pre-existing local damage
remain unverified, so this candidate has not been added to the engine.

The `hybrid-prior` and `hybrid-manowar` fixtures seed latent damage by reducing
every seventh cell's health to 35% and every eleventh bond's health to 40%
before the first cut; they do not simulate a prior projectile or an altered
material snapshot. On Brig1, no alive flags changed and the maximum first-cut
cell/bond errors were 0.000000060/0.000001073. On Manowar1 (asset SHA-256
`7e33b7ae163d1e5ca2faab7c016cba9935623166b1279c497279ecfd5b41669d`),
the maximum errors were 0.000541508/0.003865361, with no alive changes. The
Manowar seed took 83.339 ms and hybrid 24.410 ms, while checkpoint encoding
still took 63.204 ms for 30,772,648 bytes. Preparation included both paths
and took 224.788 ms. The large checkpoint, surface work, real impact, prior
geometry edits and full-path responsiveness remain unresolved.

`hybrid-hole` first applies an actual local thermal cut via GEOM, then compares
hybrid and full graph seeds on two plane cuts of that damaged material. On this
fixture the prior beam changed the mesh in 257.988 ms; hybrid graph costs were
26.300/28.001 ms versus full seed 74.579/60.791 ms, with no alive differences
and maximum cell/bond errors at or below 0.000001073. `hybrid-dead` seeds
inactive graph cells and bonds before cutting: no alive differences and max
physical-bond error 0.000001371. The latter uses a corrected prototype that
zeros physical bond capacity only when a cell is physically absent, rather
than merely logically dead; the earlier prototype did not preserve that
distinction. These are GEOM fixtures, not gameplay or whole-frame timings.

`hybrid-sweep` tests twelve plane directions and offsets on the same pre-damaged
Brig1 state. Although all trials retained identical alive flags, trial 9 had a
0.493881941 absolute bond-health discrepancy and trial 11 a 0.037517190
physical-bond discrepancy. Therefore the hybrid is rejected as currently
implemented; the discrepancy's mechanism must be established before another
candidate is adopted. The detailed output and independent source are retained.

Plane-cell clipping now leaves wholly retained authored cells and prior edits
unchanged, using conservative original bounds; rough and finite-beam cuts keep
their previous path. `clip-fast-brig` and `clip-fast-manowar` compared every
material cell against the original clip, then decoded both saved variants and
compared every cell again. Both comparisons passed on the first and second
cuts; retained source and selected raw lines are in `clip-fast-*` and
`clip-fast-output.txt`. The candidate is integrated in `ship_hull_material.hpp`.
In separately built private GEOM archives, the real `CutByLaser` path committed
two Brig1 revisions: candidate 88.399/70.954 ms and 5,646,736/6,206,440 bytes;
baseline with the same plane correctness repair 98.338/80.815 ms and
13,836,720/12,518,472 bytes. See `live-geometry-output.txt`. These are unpaired
single runs, use a static archive baseline of unresolved original revision,
and do not measure SHIP's asynchronous queue, multiple ships, physical bodies
or game frame time. Existing child modules and a current compiler were used;
no installed game was changed. `python3
docs/evidence/ship-destruction-20261105/live-geometry-replay.py <empty-scratch>
baseline` or `candidate` builds the GEOM object in a private archive and
replays the exact driver. The baseline header preserves the plane-cut repair
but uses the previous full cell clipping; it is evidence, not a source owner.

This does not prove the actual queued SHIP weapon path, save/restore, fragment
motion, buoyancy, visual acceptance, or frame time. The prepared probe copies
`prepareExact` under a distinct name to prevent the already linked static
library's inline definition from masking the candidate; it is not an integrated
engine build. The synthetic fixture uses the prior compiled archives, whose
exact source revision was not established. `libgeometry.a` SHA-256 was
`638d9de72e4656bb327f9ea6ad06ea93de2bb254812481e352e132b0730603d7`,
`libship.a` was `36f8875aa6874dfc8c2401a1772e46fae7754ba1e2f1c7730812e4c2b0ba1ce0`;
the Brig1 GM was `0dec3e566390bd410486861ad58844b7589f34684fe87520f1568b5e9a93eaec`.
The working source contained concurrent unrelated edits. The final physical
system must use bounded simplified collision and buoyancy representations and
must be measured through the full consumer path under representative load.

`python3 docs/evidence/ship-destruction-20261105/replay.py <empty-scratch-directory>`
rebuilds the retained standalone source against the local configured archives
and runtime assets. It creates an offscreen Metal fixture; it does not open or
control the game. The checked-in probe embeds the candidate implementation and
is diagnostic evidence, not a permanent regression test. Add `timing` after the
scratch directory to select the instrumented version.

## Laser connectivity, 2026-11-05

`laser-beam-sample.txt` samples an offscreen Brig1 local beam in the native GEOM
fixture. Of 219 samples in the cut, 191 were in convex connectivity rebuilding.
`reconnect-compare.hpp.txt` retains both the original and candidate algorithms
and checks each directed edge in its original order. `reconnect-probe.cpp.txt`
applies six local beam doses, including two no-change doses, and then encodes
and decodes the damaged material. `reconnect-output.txt` records the result:
four changed reconnects had identical directed edges and took 206–277 ms before
and 17–21 ms after. Whole-graph initial preparation took about 3.9 s in both.
The fixture used the local Ninja geometry archive, Metal runtime and Brig1 GM;
the compare header was injected through a Clang VFS overlay when compiling
`geom_static.cpp` into a private archive and the probe was linked against it
using `build-probe.py.txt`. This is GEOM elapsed time, not game frame timing.

The round trip preserved every encoded material cell and the component count
(6/6), but 140 cells had different adjacency after full restoration. On both
the initial build and post-restore rebuild, the candidate matched the original
algorithm's directed edges exactly for the same input; therefore this probe
does not attribute that restore discrepancy to the optimization. The identity
and practical effect of the restored connections remain unresolved. No claim
of accepted in-game fragment visuals, SAVE continuation, or FPS follows.
