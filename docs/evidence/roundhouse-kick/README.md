# Roundhouse native animation evidence

The generator authors a spinning heel kick and front/back recovery clips in
Storm's AN format. The ordinary resource delivery owner consumes
`prepare_roundhouse.paths()` and `prepare_roundhouse.prepared()`; no original
animation bank is replaced. The generated metadata owner is
`RESOURCE/INI/roundhouse-rigs.ini`.

`prepared()` reconstructs a missing cache from the committed compact inputs,
compares every output hash with the committed manifest, then reads it back.
That normal staging path writes only the cache. It rejects changed maintained
inputs and changed existing outputs; it never rewrites the tracked manifest.

`src/assets/roundhouse/recipe.json` owns choreography, timing and contact
landmarks. `rigs.json` retains exact float32 bind-zero/guard snapshots, topology,
semantic body mapping and original AN/ANI hashes. Six `support_mesh` snapshots
also retain exact positions, two bone IDs and skin weights with GM hashes.
Generation and offline rendering share the same two-weight deformation owner in
`prepare_roundhouse.py`; production generation never reads this evidence folder.
These sources are
sufficient to reconstruct every companion bank without full original AN copies.
The three man inputs refer to the existing reviewed rowboat originals.

## Reproduce

From the repository root, with Python 3.10+:

```sh
python3 tools/prepare_roundhouse.py
python3 tools/prepare_roundhouse.py --check
python3 docs/evidence/roundhouse-kick/render.py --motion
```

Pillow is needed only for the offline sheets. Source capture is deliberate:

```sh
python3 tools/prepare_roundhouse.py --source-root experiments/native-metal/.cache/runtime --capture-sources
python3 docs/evidence/roundhouse-kick/render.py --capture-meshes-from experiments/native-metal/.cache/runtime
```

The first command changes the maintained compact sources and therefore needs a
review of their diff. The second refuses to replace a different retained mesh.
Mesh inputs and provenance are retained in `inputs/meshes.json`; the six stock
GM inputs cover the six distinct hero rigs. The authoring generator and helper
hashes are bound by `src/assets/roundhouse/manifest.json`.

## Delivered contract

- 24 distinct human rigs and 51 declared ANI aliases; 49 generated resources.
- Six hero rigs have `roundhouse_kick`, `getup_front` and `getup_back`. The other
  18 human rigs have the two recovery actions. Hero aliases `man_A` and
  `one_leg_man` resolve to the exact man rig.
- Every bank preserves its source bind frame zero byte-for-byte as float32.
  Actions start after frame zero. Kick duration is 1.6 seconds; heel contact is
  at 0.9 seconds. Recovery takes 2 seconds. Samples are 60 Hz.
- Metadata body order is pelvis, chest, head, left upper arm/forearm/hand,
  right upper arm/forearm/hand, left thigh/calf/foot, right thigh/calf/foot.
  `bodyN_joint`, `bodyN_end` and `bodyN_parent` are the sole mapping source.
- Nonhumanoids are explicit `supported = 0`. The existing `man_C.ani` names an
  absent source (`man_РЎ.an`) and is also explicit unsupported; the generator
  does not substitute the unrelated `man_C.an` file.
- Heel offsets are foot-joint-local. Head offsets are head-joint-local.
  `contact_x/y/z` records the authored heel contact in the actor model frame.
  Source guard offsets differ by rig, so a fixed sword range is insufficient.

## Evidence and limits

[Kick phases](kick-phases.png) and the [short motion](kick-motion.gif) show six real skinned stock meshes through guard,
wind-up, chamber/turn, heel sweep, face-height contact, follow-through and exact
guard recovery. Each light attacker faces a darker, stationary same-rig opponent
at 0.7 m actor separation. The opponent uses its actual source fight-guard pose,
including its rig-specific root/head offset, with native X reflection and yaw pi.
The orange circle marks that opponent's head contact point.
[Recovery phases](getup-phases.png) show front/back recovery for the man mesh.

The renderer applies the native two-weight skinning contract, inverse bind and
X reflection to serialized generated AN frames. It draws depth-sorted shaded
triangles in Pillow; these are offline diagnostic images, not Metal screenshots.
Textures, weapon attachments, scene collision and victim reaction are outside
these images.

`proof.json` records exact output reconstruction, prepared-resource readback and
rejection of a changed recipe or output. Its clean-cache probe denied access to
original runtime/input AN assets and verified that the tracked manifest stayed
unchanged. The heel sweep is checked against the real opponent head over
0.688–1.216 s with the native 0.22 m head volume. All six rigs enter that volume
at 0.7 m separation; the 1.4 m negative stays outside it. No native reach is
extended. Detailed values belong to the maintained asset manifest.

The rejected authoring kept a world-transported knee pole while the pelvis
spun, and independently fixed the boot's world orientation. The man2 knee
remained a valid local-X hinge, but the ankle acquired about 168 degrees of
compound rotation. Scalar contact precision therefore concealed bad anatomy.
The correction authors hip/knee/ankle rotations in the bind rig's local axes,
inherits the boot from the calf, turns the pelvis/chest toward impact and uses
root placement to close contact. The early heel arc rises before impact; the
free hand protects the head and the weapon arm counterbalances, retaining local
hand/finger poses. Source guard arms had previously been copied unchanged,
which left Jessika/Yoko's hands low during the action. Head gaze reacquires the
target after the back of the turn rather than swapping neck directions there.
`continuity.json` records every serialized action's adjacent local rotation;
it is a diagnostic and does not prove anatomical quality.

[Jessika's same-pose views](jessika-sweep-views.png) trace the hip/knee/ankle in
the queried 0.70 s sweep from the author camera, front and side. The author view
foreshortens the raised leg over the torso/arm. The serialized
[joint measurement](jessika-sweep-anatomy.json) has 43.18 degrees of striking
knee flex and -35.30 degrees of ankle flex, with less than 0.000003 degrees of
off-axis rotation at either joint. The support knee bends 82.24 degrees while
its boot pivots on the floor. This query did not establish an anatomical twist,
so it did not change choreography. The normal renderer command regenerates the
three views from that exact serialized frame.

`mesh-metrics.json` retains real-mesh floor bounds for kick and both recovery
clips. The initial torso-only recovery clearance missed boots, knees and arms,
allowing up to 17.4 cm penetration. The six captured hero meshes now use their
exact skinned minimum Y for every recovery frame, with 5 mm clearance. Their
unchanged final guard frame retains its original small sole penetration (up to
8.7 mm on Jessika). Other human banks use articulated body/toe/finger bounds;
no rendered claim is made for uncaptured clothing meshes.

[Native pose API evidence](native-pose-api-probe.md) and
[animation CPU comparison](animation-cpu-probe.md) check separate native contracts;
they do not establish visual anatomy. The independent backend's
[real-rig inputs](backend-real-rigs-inputs.json) and
[raw results](backend-real-rigs-raw.txt) include the corrected output-X alignment
audit. Its native-skinning oracle passes all 24 rigs and all 12,873 vertices and
normals across the six retained GM meshes (maximum error 1.53 micrometers).
The old unreflected bridge misses rendered joints by up to 1.35 m; its earlier
self-roundtrip did not expose that error and is superseded by this oracle.
Native whole-scene gameplay acceptance, collision and ragdoll/getup blending
remain the integration owner's responsibility. No game window was opened.
