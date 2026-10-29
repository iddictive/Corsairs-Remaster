# Legacy island geometry ownership

The reported island flicker has a reproduced component failure: moving the
camera 2cm changes 7959 pixels in the affected slope by at least 32 RGB levels.
Reversing regional draw order changes 10479 pixels there without changing the
coverage mask. Different regional GMs repeat terrain at the same world pose.
This is independent of the earlier far-water depth fix.

## Repair contract

Keep the current legacy world frame, logical ports, ships, forts, model/node
identities, native lighting lookup, authored paths and depth-grid coordinates.
Give source-proven coincident copied surfaces one physical owner. Distinct
regional variants and independently posed regional coasts remain present.

The canonical component/variant authority is
`src/assets/islands/seamless-sea/recipe.json` with its hash-pinned inventory.
Its inactive global atlas is an alternative implementation, not this delivery:
Mainland and Cuba legacy placements differ materially from their shared-body
bindings. Enabling that atlas would move existing coasts and is excluded.

The legacy projection therefore retains each source's native body at its
current pose. A copied face can be removed only when a declared provider has
the same oriented geometry at the standard legacy world pose within recorded
native source/float roundoff. Canonical provider and admitted variant faces are
protected; unmatched faces remain. The generated correspondence records bind
both source face IDs and pinned assets. Draw order, camera position and tone
are not ownership inputs.

Reuse the existing GM range/BSP extractor. Retain original header/object bounds,
vertex buffers, material/object IDs, hourly COL indices, labels and source BSP
IDs; only draw ranges and active BSP references change. A separate resource
projection preserves the original model name for children and lighting.
Existing native depth grids and authored path coordinates retain their original
mapping: removal transfers coincident geometry ownership rather than removing
aggregate physical coast. Trace/collision must prove the retained provider.

Enable the projection only for the matching complete legacy resident plan.
Unknown/custom placements, a partial unplanned scene and active atlas mode
retain their existing geometry. Activation is transient and resets on each
scene mount; it is not a SAVE migration or a renderer preference. The existing
single-island restore path may validate its already-supported subsequent
complete resident plan before loading the root; entity/fort restore order stays
unchanged. Missing or partial projection assets leave the original scene active.
An unexpected projected load failure records its path, retries the original
resource and disables further projection for that scene. Every provider retains
its native faces in both paths, preserving coast after a copied surface has
already transferred ownership.

After the complete resident scene mounts, the admitted correspondence links bind
each provider to its former source regions. Terrain and seabed use the minimum
of their own and those sources' raw AABB distance fades. This is non-recursive
and changes no navigation, foam or depth bounds. An entire provider body can
remain visible farther than before; that is the accepted experimental tradeoff
for preserving exposure when its former copies disappear. Reflections retain
their original resources and independent visibility lifecycle.

Exposure links use the authoritative region ID received in
`MSG_ISLAND_WORLD_STATE`. A geometry stem is not that identity: the shipped
`Barbados` region loads model `barbados`. Binding stages the complete result
before replacing existing links. Missing metadata returns failure with a compact
diagnostic, preserves existing links and temporarily pauses distance fading;
a subsequent world-state message or explicit bind retries the operation. This
identity is independent of the disabled static-world material consumer.

Current `world_damage_policy.hpp` disables static-world material editing and
filters its old saved attributes. Preserve that policy. Re-enabling it requires
an explicit source-fingerprint/material checkpoint migration; retaining vertex
indices alone would not make pruned geometry compatible with that dormant
consumer.

## Verification and cost

The baseline and disposable cause-isolation inputs are retained in private
application diagnostics; user SAVE bytes and screenshots are excluded from Git.
The initial repeated-face diagnostic reduced slope temporal/order changes to
one pixel; canonical component extraction reduced order changes to zero.
Those 1x component results establish the mechanism, not final runtime acceptance.

The final experimental package passes the marked near-slope component using the
production ISLAND exposure binding and fade methods: order changes fall from
10479 pixels to 1–2; the 2cm temporal change falls from 7875 to 1. Near-slope
coverage loses zero pixels. All four native 27-root Trace samples are
bit-identical, including three actual landmark hits. This is the confirmed 1x
path; it does not claim an MSAA or byte-identical live-camera replay.

The reflection projection was removed: its independently faded MODELR roots do
not inherit the terrain visibility owner. Existing reflection assets and layer
membership remain unchanged. The final projection has 13 terrain/seabed GMs;
all their bytes match the previously verified candidate. Independently posed
Cuba2, Caracas and Beliz retain their original counts. Atlas, partial,
custom-pose and missing-assets plans retain originals; disappearance after
admission retries the original.

Projection alone lost 8572 coverage pixels in a far-view witness because a
retained provider faded before its former copy. The visibility adapter reduces
that normal shaded-frame difference to 224 pixels. A solid-white GEOS control,
with texture alpha, blending and distance fading removed, still loses 282
pixels in a narrow far-horizon band. This is not proved to be opacity-only or
harmless rounding. The remaining physical coverage discrepancy is unresolved;
the package does not establish acceptance for all coasts. Cyclic-only triangle
correspondence preserves winding, and the transferred-face participation scan
found no visible/collision flag mismatches.

The Corsairs integration owner retains the existing island repair for follow-up
after this experimental installation. The next decision is to explain those
282 solid-white pixels against the retained counterpart geometry and unchanged
poses before changing source admission or claiming complete coast coverage.
Normal old-SAVE startup acceptance belongs to the simulation/integration owner;
installed gameplay remains the user's check. No game UI observation is authorized.

The production candidate must pass the same slope/order/2cm falsifier, native
Trace/collision and reflection ownership, and a separated-port/coast negative.
Verify the old SAVE through normal startup with the existing simulation owner,
including original model/COL identity and unchanged poses. Verify partial/custom
legacy and active-atlas gates do not select the projection. User checks gameplay;
no game UI automation or observation is authorized.

Preparation is offline and hash-reusable. Runtime adds one scene-plan validation,
a mapped file lookup per geometry load, one binding after resident mount and
34 bounded raw AABB fade evaluations per frame. It adds no render pass, draw or
sample. Native draw/BSP work decreases. Representative terrain-only preparation
took 34.9 seconds with 1.52GB peak resident memory; 14 delivery files total about
80.8MB. One paired correctness run loaded the same 27 model roots in 273ms
before and 247ms after projection. These single-run times are not a benchmark
or a whole-game performance claim.

## Retained reproduction

The private application diagnostic bundle `island-visibility-20261029` owns the
exact inputs and procedures; it is excluded from source publication. Its
`accepted-exposure-replay.py` builds a disposable native component against the
canonical archives and runs both near and far cases. Run it through the existing
`codex-run-scratch run` entrypoint with 536870912 bytes, 400 files, 360 seconds
and an 8589934592-byte reserve. No SAVE or game UI is executed.

The normal procedure retains `accepted-exposure-near-output.txt` and
`accepted-exposure-far-output.txt`, with lossless PPM frames in the matching
directories. `ISLAND_GEOMETRY_CONTROL=1` selects the solid-white far control;
its raw result is `accepted-exposure-far-geometry-output.txt` and its frames are
in `accepted-exposure-far-geometry`. The strict disposable coverage assertion
returns exit 88 for the unresolved far difference; it must not be reported as
a passing all-coasts test. `terrain-exposure-witness-input.json` pins the far
camera/target and source dependencies; `accepted-exposure-native.cpp.txt` retains
the emitted component source. Earlier projection-only evidence remains under
`terrain-exposure-*`. These private inputs must remain available to the
integration owner until the unresolved coverage requirement is closed.

The startup crash regression is retained as `region-identity-bootstrap.py`,
its emitted native source and `region-identity-bootstrap-output.txt` in that
same private bundle. It loads all 27 islands through production
`LOAD_GEO`, `WORLD_STATE` and `ATTACH` messages using the shipped model stems,
then proves the 34 links, missing-ID atomic preservation and automatic retry.
It does not substitute manually assigned canonical names for the loading path.
