# Metal renderer investigations

These investigations preserve cache, lifetime and geometry failures that matter
when changing the Metal backend. Current source owners below were checked at
`f220709eaf22b23cedd57231fbf906e54e32ce56`; measurements describe the historical
fixtures below. They are not current player FPS or
stability guarantees.

## Preserve resource lifetime when removing allocations

`experiments/native-metal/backend.mm` owns `RawBufferCache`, `residentBuffer`,
`dynamicBuffer`, the frame arena and the skinning source map.

- Raw immutable uploads share one identity/revision LRU across vertex and index
  consumers. The historical two-map implementation cleared everything when a
  missing identity arrived at 512 entries. Installed disassembly and a live
  October 10 sample confirmed that capacity branch actually executed. A fixed
  512-entry LRU still thrashes on a cyclic 600-resource workload; the replacement
  uses selective eviction, a 512 MiB byte budget and 8,192 entries. Oversized
  inputs remain drawable without becoming resident. Changed revisions publish
  new immutable bytes while encoded commands retain previous buffers.
- Dynamic sea sails use completion-owned arena slices for both vertex and index
  rewrites. Repeated partitions and reflection faces share one upload per
  identity/revision within a submission; a rewrite receives a different slice.
  Clear the submission lookup on Present and synchronous readback. Location
  shadows can consume an earlier registry snapshot, so location geometry keeps
  immutable residency. Logical index/base addressing must be validated before
  adding physical arena offsets.
- Stable AVERTEX0 data uploads once into each of the three existing flight slots;
  each pose still publishes its own palette. Geometry Unlock/Release invalidates
  the source before mutation or recycled-address reuse. Cached maximum bone index
  must still reject a later skeleton with fewer bones. CPU bounds must read the
  actual shared vertex source and current palette: the facade's allocated CPU
  byte vector can be zero-filled even when the GPU source is correct.

Historical component checks illustrate the mechanisms: eight warmed sweeps of
600 unchanged 4 KiB resources produced 4,800 repeated allocations with whole-map
clearing and zero with the bounded replacement. For a 6,000-vertex animated source,
120 submissions over six frames fell from 120 uploads/31.68 MB to three
uploads/0.792 MB. These are allocation/copy counts, not game speedups.

The earlier resource-owned residency candidate also eliminated repeated copies
in its small fixture, but the player reported new long stutters after delivery;
it was rolled back. Passive telemetry contained 279 ms intervals outside the
intrusive sampler window. Component success therefore did not accept its frame
pacing, and blaming every stutter on sampling was unsupported.

## Registry reuse has two different keys

`experiments/native-metal/point_shadow_registry.hpp` owns caster registration,
GPU tables, ICBs and their completion lifetimes. A moving instance changes its
exact transform/pose key while retaining its geometry identity. Rebuilding the
entire exact-key map for each recycled caster made maintenance `O(R*N)`.
Prepare FIFO queues of previous slots by immutable geometry once per frame;
skip exact-key slots already claimed and update only the selected recycled key.
Two current-frame instances sharing one mesh must remain separate casters.

The September 30 component replay measured registry work, after warm-up over
12 frames: 100/400/800 casters changed from 0.203/3.614/16.908 ms per frame to
0.010/0.045/0.097 ms. This does not predict whole-frame cadence.

The 24 completion-owned GPU resource slots serve eight cubes across three
frames in flight. Reuse immutable preparation tables only within the same
command buffer and observation epoch; retain independent cube ICBs, face data
and argument buffers. A changed palette offset, new observation, scene, command
or reset invalidates sharing even when the old geometry key still matches.
Declare the unique set of indirectly accessed resources and retain it until
completion. The October 12 eight-cube fixture preserved full depth readbacks,
reduced preparations eight to one, and measured warmed CPU encoding averages
1.083 to 0.496 ms; first-use allocation was excluded.

## Diagnose corruption at the writer

Two historical faults must remain distinct:

- `experiments/native-metal/engine/src/libs/sea_foam/src/seafoam.cpp::SEAFOAM::AddShip` indexed a fixed
  64-record array without admission. The actual record layout showed index 64
  overwriting the adjacent count and INI pointer fields; a layout probe reproduced
  the reported pointer words. Guard capacity before indexing/incrementing. A
  missing INI or null check cannot repair a pointer overwritten earlier.
  The original crash's registration count was unavailable, so the layout proof
  establishes the overwrite mechanism rather than the complete crash history.
- A later `IndexedSpanFrameCache::erase` fault occurred while a sea-foam indexed
  draw evicted a revision. The bad tree link identifies where corruption became
  visible, not its writer or a specific `find(end)` mistake. Unchanged-header
  ASan/UBSan stress passed 300,000 mixed revision/topology operations, 40,000 LRU
  insertions and 192 crossed invalidations; the production allocator variant
  also passed. A partially instrumented engine-linked GEOM/Device/TCarcass trial
  passed actual cut/upload/release and slot reuse. None reproduced the corrupting
  write or proved player stability. Disabling the cache would hide the consumer
  without establishing a repair.

`indexed_span_frame_cache.hpp` owns both identity memberships, recency, revisions
and payload accounting. Erasing a span must unlink both buffer owners. Capacity
must evict the oldest key and admit new work: the former full-cache policy made
an unchanged 8,193rd span rescan on every request after admitting 8,192 entries.
Clear all bookkeeping together on reset. The current source's invalidation and
LRU rules are the owner; historical stress success cannot exonerate unrelated
writes or concurrency.

## Attribute lookup must not intern or truncate its input

The separate [attribute lifetime and lookup contract](attribute-class-lifetime.md)
owns the October 17 fixed-buffer overrun, bounded-view behavior and sanitizer
limits. The independent shared-RNG/free-cloth
scheduling finding is preserved in [metal-sea-benchmark.md](metal-sea-benchmark.md).

## Material-cut topology: preserve the coordinate domain

Finite hull cuts can produce a plausible mesh while failing closed-volume and
detachment checks. The October 16, 2026 investigation isolated two different
quantization failures: original authored faces and repeatedly clipped cutter
walls require different projection rules.

This is historical CPU geometry evidence. It does not establish current game
appearance, GPU cost, frame rate or player acceptance. The broader cut lifecycle
belongs to [sea-phonk.md](sea-phonk.md); naval material ownership belongs to
[naval-ammunition-and-damage.md](naval-ammunition-and-damage.md).

### Source owners

The correction entered commit
`6e2cc7b8b6ed115c3446921066b97b15df782aab`.

- `experiments/native-metal/ship_laser_geometry.hpp`: `subtractFaces`,
  `clipOriginalMesh`, `closedCutVolume` and `detachEnclosed`.
- `experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp`:
  planar polygon subtraction and triangulation over the pinned Clipper inputs.

`clipOriginalMesh` selects `subtractFaces(..., authored=true)` for immutable
authored triangles. Generated cutter walls retain the default metric projection.
This distinction is part of the geometry contract; applying either projection
to every consumer reproduces a known failure.

### Why the original bow did not detach

The original fixture reported a completed cut, zero closed-cut volume and no
detached piece. Its mesh contained opposing triangles with areas around
`7.04e-11 m²` and source-edge slivers around `2.04e-9` and `3.43e-10 m²`.
Independent rounding to a `0.1 mm` position grid promoted nearly collinear
triangles into nonzero topology. A metric planar grid does not preserve an
arbitrary oblique authored edge. Floating-point restoration of subdivided
edges after triangulation introduced additional slivers.

For an authored source triangle, projection into barycentric coordinates maps
its corners to `(0,0)`, `(1,0)`, `(0,1)`. Integer clipping then preserves the
source edges as exact simplex equations. Attribute reconstruction keeps the
original interpolation contract; the authored path omits floating contour
restoration.

The narrower alternatives failed:

| Candidate | Original bow result | Reason to reject |
| --- | --- | --- |
| Remove restoration alone | 104 triangles; zero closed volume; one detached piece | Four directed boundary intervals still fail |
| Integer triangulation alone, restoration removed | 91 triangles; zero closed volume; no detached piece | Authored diagonal ribbons remain |
| Barycentric projection for every consumer | Bow passes | Repeated generated-wall clipping breaks the solid-ring case at dose index 4 |

The all-consumer variant produced two generated wall vertices only `6.0051 µm`
apart. They shared the same `0.1 mm` position key, creating opposing slivers and
one failing directed interval. Restricting barycentric projection to immutable
authored faces, while preserving metric projection and subdivision for generated
walls, fixes that separate regression without relaxing closure tolerances.

### Independent geometry checks

The bow fixture is a closed square-section mesh with sections `(z, half-width)`
`(-2,1)`, `(0,1)`, `(0.25,0.15)`, `(1,0.15)`. Each pair of neighboring sections
uses two triangles per side; both ends are capped. A separate object 9 is a
closed cube of half-width `0.1 m`, translated `4 m` along X.

Four finite beam samples start at `(0.16,y,0.6)`, for
`y = -0.15,-0.05,0.05,0.15`, point along `(-1,0,0)`, and have radius `0.13 m`.
Depth `0.05 m` must leave the bow attached. Depth `0.4 m` must detach exactly one
closed bow with maximum Z `1 m` and minimum Z greater than `0.4 m`; object 9 must
remain byte-identical and absent from that piece.

These independent analytic volumes define the expected result:

```text
original = 8 + 0.25*(4 + 0.6 + 0.09)/3 + 0.75*0.09 + 0.008
         = 8.4663333333 m³
removed  = 0.3*0.3*0.26 = 0.0234 m³
retained before detachment = 8.4429333333 m³
detached bow = 0.3*0.3*0.27 = 0.0243 m³
anchored body = 8.4186333333 m³
```

Both Earcut and the pinned Clipper integer triangulation passed the original bow
checks with the corrected projection. Replacing the triangulator alone was
therefore insufficient to explain or repair the defect.

Closure must account for edge subdivision: an exact-edge mismatch count is not
itself an open boundary. An independent directed-interval oracle quantized
positions at `0.1 mm`, ignored quantized degenerate triangles, and required two
oppositely directed coverings along every boundary interval. The corrected bow
had 92 triangles and 20 unmatched exact edges, but zero failing intervals.
After the domain correction, a repeated-cut ring dump had 439 triangles and 316
unmatched exact edges, also with zero failing intervals. The earlier universal
barycentric variant had 461 triangles and one failing interval.

The retained broader CPU checks covered solid and hollow rings, mast support,
partial notches, complete severing and closed detached pieces. They do not prove
native upload, collision publication, SAVE reconstruction or interactive cost.
The original fixture and oracle are not distributed here; the geometry and
analytic expectations above permit an independent reconstruction, while exact
historical triangle counts remain observations of the recorded inputs.
