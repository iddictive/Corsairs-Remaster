# Common environment lighting

The native animated vertex shader separately accumulated lights after selecting
an unlit authored colour. Static/unlit geometry did not. With the same material,
pose and disabled lighting plus an enabled directional source, the native skin
changed d04080c0 to d060c0ff while the independent CPU/static result stayed
d04080c0. Clouds are disabled in this replay.

Posed and static geometry now share the existing raw lighting evaluator. Bone
position/normal transformation, UVs and alpha remain with the skin adapter;
its existing no-vertex-specular policy remains explicit. Static and forest
callers keep vertex specular. Lighting-disabled receivers no longer add lights.

The frozen replay derives from the existing gpu_skinning_offscreen_probe.cpp,
using two actual bone poses and native Metal64x64 readback. Eight combinations
cover lighting on/off, enabled source on/off and diffuse/ambient material vs
vertex-colour selection. It checks complete coverage/RGB/alpha parity and an
independent literal FFP colour oracle, with modern lighting both off and on:
32case renders pass. Source identities and raw results are retained below.

This is component evidence, without a new maintained test target. For a configured
native build with current libstorm_metal.a, set CODEX_SCRATCH_DIR to an existing
disposable directory and run the frozen Python source with repository as its
first argument. It derives compiler/link arguments from the existing
GPU-skinning target and uses no game loop, player state or game UI.

No new passes, textures, allocations or per-frame CPU work are added. Static
lighting arithmetic stays in the existing helper; disabled skin lighting removes
its duplicate light loop. Whole-scene frame pacing and the separate baked-land
hourly-lighting issue are not established by these colour observations.

The main-menu room had a second concrete divergence: the CPU evaluator excludes
outdoor sun from its existing world-space room region (z >= 22), while raw GPU
vertices still received that source. The native raw readback was ff54514d where
the existing CPU response was ff14110d. The same source cutoff now reaches both
evaluators. The CPU conversion cache includes this domain and its position
dependency. The window side, ordinary scene and exact boundary are preserved.

`menu-replay.py.txt` retains the native component replay and `menu-baseline.json`
the original mismatch. `menu-candidate.json` records the corrected 15 draws:
one persistent vertex buffer translated across the threshold through menu off,
on and off again. Both evaluators agree, including their RGB and alpha bytes.
This tests world-space cutoff and cache invalidation without relying on camera
position. The extra cutoff comparison runs inside the existing light loop; it
adds no pass, allocation, sample or buffer, and is disabled outside this domain.

The separate baked-land lifecycle mismatch was confirmed. The restored world
weather selects hour 9/morning9 while its saved baked path remains morning7.
Whr_UpdateWeatherHour updates live environment lights without updating the
baked path; ModelArray::UpdateLightPath only normalizes a string. Existing node
SetLightPath releases and recreates geometry, which would destroy native cut
state. The shader replay alone does not prove hourly appearance behavior.

The asset audit is reproducible with `col-audit.py.txt`. PuertoRico main has
60,741 colours (242,964 bytes); every colour changes between morning7 and 9,
with mean RGB (78.45,75.20,71.69) becoming (91.50,75.36,70.17). Fort, reflection
and seabed add 36,013 vertices. All 105 paired island COL assets contain
3,084,115 vertices/12,336,460 bytes, read in 13.6 ms warm in the observed batch.
This is an asset upper bound, not a resident count or a native upload timing.

The active-consumer audit found world damage disabled by the canonical
world_damage_policy.hpp, while ship material damage uses intrinsic models with
empty COL paths. No physical checkpoint migration is needed for the active
hourly receivers. The new geometry-owned appearance transaction changes only
render colours, keeps immutable original physical colours for later captures,
and rejects an overlapping active damage reconstruction rather than resetting
its state. Optional PN render buffers reuse the existing exact refiner.

A second concrete outdoor source mismatch exists in generated PuertoRico city
children: the city GM has 149,071 vertices, all RGB127, and no generated COL
siblings. The unchanged source town has matching authored COL streams: morning9
contains 3,271 distinct colours, mean RGB(47.49,48.30,46.68). `city-col-replay.py.txt`
proves the minimal producer candidate read-only: copy the four native town COL
streams, and carry the native island COL through the existing blend geometry
transformation. Every non-colour vertex/index byte remains unchanged, including
the blend. The five morning9 streams total672,108 bytes per primary/reflection
root. The observed pure probe took734ms; results are in `city-col-candidate.json`.
The canonical city producer now captures/hash-binds the 129 native COL source
streams beside its existing immutable GM inputs and emits 258 primary/reflection
COL siblings. All 14 previous geometry/navigation outputs retain their exact
SHA256 before the explicit day-only label addition below. The reviewed manifest includes source and target hashes; the existing
delivery owner discovers the new files through that manifest. These assets were
subsequently included in the user's requested experimental installation.

`city-producer-results.json` retains full producer timings and checks. Cold
generation is10.474s for272files/80.58MB (previous642.6ms for14files/44.15MB).
The reviewed recipe/input/output cache returns the same verified outputs in
85.9ms warm. Missing/changed outputs return to generation; source changes require
review. Invalid COL vertex counts are rejected. This adds native COL loading at
geometry creation, using the unchanged existing loop; no new per-frame work.
The source day-only membership for SanJuan_rinok now controls one explicit
DayOnly geometry label in each generated primary/reflection root. Its tracked
source hash invalidates the prepared output. Only the label flag byte changes;
geometry, indices, UVs and child identities remain intact. Existing NODER flags
hide that subtree from drawing, tracing and clipping at night and restore the
prior flags at day. Existing native-location model/lamp membership is preserved.

`appearance-native.cpp.txt` and `appearance-replay.py.txt` retain a disposable
production GEOM/NODER replay against actual PuertoRico assets. The fresh native
load at morning9 is the independent oracle: base vertices and expanded draw
triangles match byte-for-byte after an in-place morning7→9 update. Default PN0
keeps base VBO IDs and physical generation/revision. Same identity takes no
buffer locks; read failure and cancellation retain the previous palette;
legitimately absent COL equals the original native intrinsic fallback; a retry
recovers exactly. Existing-but-unreadable COL is distinguished by the owning
file service and rejected. Tree day→night→day preserves flag state and child
failure cancels the already-prepared parent. No production test target is added.

WhR hourly refresh prepares the full resident island aggregate, all resident
reflections and currently loaded location roots before committing native state
and the shared script paths. Invalid assets retain the previous appearance and
receive at most five one-second retry events. Initial scene load and cabin-sleep
refresh use the same phase/path contract. Whole PROGRAM compilation and the
no-entity global transition pass in `weather-result.json`; this is separate from
integrated old-save/resident aggregate timing and render acceptance. The later
normal-startup replay reaches 24 native ships and 27 resident islands. Its
morning9→7 aggregate rejects atomically in 36.87ms, retaining morning9 on all
27 residents. Morning9 and repeated morning9 pass with no mismatches in
0.117/0.099ms, with 27 reflections enlisted. The morning7 rejection remains
unresolved; component parity does not establish aggregate transition acceptance.

PN refinement remains optional and OFF by default. Its observed single-island
appearance rebuild costs approximately55–61ms, versus0.3–1.2ms for default PN0;
this is a material optional-mode cost, not a whole-scene/default performance
claim. Lightmap texture replacement remains the existing load-time owner; the
current modern outdoor shadow path replaces baked shadows when ready. No
general live lightmap swap, installed-game visual acceptance or settings change
is claimed.

