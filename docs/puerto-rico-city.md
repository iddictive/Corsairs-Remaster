# Puerto Rico city in the sea scene

## Status and contract

October16 candidate. The player requested one installed example: the actual
walkable San Juan town at native scale replaces the sea-town decoration, with
the surrounding coast fitted to it. The separate naval fort, its guns, locators
and collision must survive. The clarified result includes walking inhabitants
visible from sea and coherent proportions among ships, buildings and people.
The player subsequently rejected the added background neighborhoods. Preserve
the original compact core at native scale; emit no replacement neighborhoods.
Landing remains the existing scene transition. Seamless landing and ship-fire
damage to city guards have not been implemented or accepted.

## Owners and delivery

`tools/prepare_puerto_rico_city.py` owns the deterministic GM projection;
`src/assets/islands/puerto-rico` owns its placement and exact input/output hashes.
Original game inputs remain in the native input owner, never in a temporary
directory. Generated GM files belong to the native cache and normal resource
delivery receipts. The native integration owner serializes Git, staging
and installed-app mutation. Native population reuses Location/PtcData/NPCharacter
through an explicitly separate background context; player SAVE/config stays out
of scope. New native source requires the shared canonical build/stage batch.

The finite CPU candidate contains the native waterfront core, its underwater
child and four pedestrian routes on the original1165-triangle navigation patch.
No background houses, streets or foundations are emitted. Its14 prepared
resources total44,154,064 bytes. This is construction evidence;
native scene appearance and population behavior are still unaccepted.
The city resources and activation script are not installed.

Reuse Storm's existing `geometry` labels and MODEL child transforms. Retain the
city's original geometry/BSP and materials. Remove only the original sea-town
draws and their collision references. A local terrain replacement lowers the
town footprint and returns continuously to the original coast; its collision
must use the same resulting triangles. Reflection uses the same target surface.

## Geometry evidence

The source sea town is objects4/5, 2755 triangles, BSP faces60263..63017.
The native town is100365 triangles, approximately114x112 world units; its
seabed is500 triangles. Placement is near(1100,0,650), with no scale enlargement.
Sea reload_1 is offshore at(1107.026,0,852.127); it is not a valid town anchor.
The source shore rises15–25m behind the waterfront.

Native units already agree: a citizen is approximately1.87m, a frigate hull53.48m
and a man-of-war hull69.49m. The original town is a compact
level, only about1.64 man-of-war lengths wide. Uniform enlargement would break
doors/people/crew proportions. Preserve the core at1:1. The attempted native-size
house expansion was rejected for its appearance and removed, including its
streets and foundations; that approach is historical evidence below.

The local50m collar touches23 primary beach/jungle triangles. Its closest
distance to the fort AABB exceeds570m. The fort's4160-triangle GM and its45
gun/mortar locators are separate inputs and excluded from output mutation.
The sea seabed and original shore/fort locators remain byte-identical.

The upstream Blender GM exporter has no BSP writer, so a full export would
lose the existing collision contract. Preserve source BSP partitions while
pruning removed face references; generate collision only for the small local
terrain replacement. Source: storm-devs/storm-engine tools/blender-gm-export.

The rejected urban lots/streets sampled the same retained primary faces and float32-rounded
replacement faces used by the island. The former stock-ground sampler left
streets above the lowered collar; actual slope admission rejects those lots.
Its road PTC used native float32 coordinates before vertex welding and split
existing collinear edge portals. Decimal pre-rounding created disconnected
vertex IDs for byte-identical native coordinates; real height steps stay distinct.

The old simplified reflection ground covers54 of the initial67 house bases by
more than2m (worst40.2m). Reflection therefore reuses the exact primary terrain
and blend, omitting only the underwater seabed child and unused root BSP.
All67 samples, the input identity and barycentric procedure are retained in
[`reflection-comparison-20261016.json`](evidence/puerto-rico-city/reflection-comparison-20261016.json).
This adds approximately1.6MiB to the prepared resources compared with the first
candidate; maintaining a second height field would require further remeshing
while retaining a source of visible geometric drift.

`NODER::Trace` uses the rigid `CMatrix::MulToInv` inverse. House children keep
rigid1:1 transforms. Foundation dimensions are baked into a single world-space
GM with identity transform; nonuniform label scaling would break collision.
Generated street/foundation object bounding spheres had to be recalculated from
their emitted vertices, otherwise per-object culling uses the source town's bounds.

The first native run found two independent issues. Appending equal `geometry`
text at new string offsets does not create the same group: `GEOM::FindName`
returns a pointer-derived ID and `FindLabelG` compares that identity. Intern
equal names at one existing offset, including the original seabed's group.
The native fixture requires city/blend children to be discoverable and rejects
the removed street/foundation children.
Its immediate-pointer retirement assertion was invalid: EntityManager removes
validity/cache membership immediately but frees pointers at the frame fence.
Check `IsEntityValid` before the fence and pointer reclamation afterward.

Exact raw dimensions, input hashes/captured paths, source hashes, placements,
route topology, rejected approaches and reproduction commands are retained in
[`evidence/puerto-rico-city/generation-receiver-20261016.json`](evidence/puerto-rico-city/generation-receiver-20261016.json).
The subsequent name-interning revision is retained in
[`generation-labels-20261016.json`](evidence/puerto-rico-city/generation-labels-20261016.json).
The current core-only candidate, exact source/manifest identities and independent
terrain-area comparison are retained in
[`generation-core-20261016.json`](evidence/puerto-rico-city/generation-core-20261016.json).
The23 removed beach/jungle faces retain their full projected footprint after
subdivision (maximum area difference3.64e-12 square units). This does not accept
rendered seams, but contradicts missing terrain from the subdivision itself.
The player suggested the dark gaps may be original waterways. The exact
500-triangle `SanJuan_sb.gm` underwater child survives. Preserve the original
waterways and vertical placement; do not fill them or infer infinite depth
from the diagnostic water. The labels-v3 fixture lacks game weather and used
the original shader mode, so its black/blue ocean is not a player-runtime verdict.
The earlier67-house candidate and raw scale inputs remain in
[`generation-20261016.json`](evidence/puerto-rico-city/generation-20261016.json);
its stock-ground/independent-reflection placement is rejected.
An unexpected return to the initial generator source was captured in
[`source-reversion-20261016.json`](evidence/puerto-rico-city/source-reversion-20261016.json).
The writer is unresolved; the current receiver record retains complete generator
source bytes as well as hashes, and prepared output admission rejects source drift.
Captured original inputs live under the repository's native input owner declared
in that record; recipe and generators are versioned. Regenerate with the recorded
command; `--measure-scale` independently reports source model bounds and retains
the exact inputs. CPU construction used Python3, no game process or installed
resource mutation. The observed3.77s CPU construction is not a game-performance result.

The durable private native fixture is
[`evidence/puerto-rico-city/native/run_probe.py`](evidence/puerto-rico-city/native/run_probe.py).
After the integration owner releases coherent native libraries and the GPU,
run its `prepare`, `build --closure <retained-build>`, then `run` actions with
the same fresh `--name`.
It imports actual island/fort/frigate geometry, creates32 animated native
citizens and records movement, zero-delta, land-context and retirement checks.
Its hidden CORE has isolated userdata and no player SAVE. Current fixture frames
have a diagnostic backdrop; they cannot establish full ocean/lighting or player
scene acceptance. Outputs and exact command/library bindings are retained under
`.local-archives/metal-renderer-evidence/puerto-rico-city-20261016/<name>`.
The first runtime result, raw32-character movement and rejected frame identities
are retained in
[`native-receiver-v2-20261016.json`](evidence/puerto-rico-city/native-receiver-v2-20261016.json).

## Acceptance and rejection

Inspect the complete sea scene with the native renderer: the native town is
on the shore, the old oversized buildings are absent, the coast has no exposed
hole/raised reflection, and the fort remains visible. Trace the changed terrain
and native city, plus an unchanged fort surface. Verify protected file hashes
after installed delivery. Source/checksum/native diagnostic evidence alone does
not accept the player's sea scene. Do not launch/control the player's game.

Must not flatten the fort's headland, change its gun locators, enlarge the town
to the old decoration's scale, or claim seamless walking from the ship or
ship-to-town NPC combat.

## Unresolved

Candidate construction passes its exact geometry/navigation checks. Native
rendered inspection, population lifecycle and installed delivery are pending.
The labels-v3 run resolved54 geometry nodes, preserved all4160 fort triangles,
and passed first-generation movement, animation, zero-delta and land guards.
Its reactivated population retirement assertion failed. A core entity-cache
ordering defect is a source-backed hypothesis; the next diagnostic separates
cached IDs from live records before any engine change.
The raw run and frame identities are retained in
[`native-labels-v3-20261016.json`](evidence/puerto-rico-city/native-labels-v3-20261016.json).
Player sea/landing/fort replay remains the final acceptance boundary.
