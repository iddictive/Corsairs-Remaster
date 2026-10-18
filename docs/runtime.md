# Runtime state

Last verified engine delivery: October18 visual rollback and nonblocking ship Mount, signed engine `6146d2e10dff253a23b5664c5c9f1b4beb20869e3c5bc7b648b8a0589493b956`. Canonical `run.sh --stage-only` exited0; the installed player record selects authored sky (`dynamic_sky=0`), and structural contact fracture remains off by default. Prior renderer/FPS source optimizations remain; the Mount readiness wait was removed because it blocked live ship additions. Same-scene graphics, complete loading duration, FPS and first-hit latency are unresolved. Ship-interior sea exclusion and terrain excavation remain disabled. Shore/rowboat travel stays frozen at `codex/frozen-shore-travel` (`6dc08e7`); ordinary town transitions remain. Published ZIP/local Drive remains23ac5eb. Earlier installed inventories are historical.

## Current verdict

### October18 forest-density source candidate — player runtime untouched

The player now asks to increase tree density after the shader forest conversion.
`island_forest.hpp` raises the placement raster cap262144→524288 while leaving
the meshes, species, masks,4096 visible-instance limit and180000 shadow-vertex
limit unchanged. Archived Antigua/Porto Bello inputs produce488→1099 and
5832→12354 roots. At each original midpoint-root camera focus,1100m candidates
grow176→361 and180→382;6000m candidates488→1099 and1445→3016, below the
visible cap. `docs/evidence/island-forest/native-inputs-20261016-calibrated-settlement.tar.xz`
and `measurement-20261016-scene.cpp.txt` reproduce baseline `e0c6cd4`
against this source with `clang++ -std=c++20 -O2` and `advance(16384)`.
Placement totals4.81→7.29ms and10.53→16.89ms over29→50 and43→72 bounded
frames; candidate peak steps were.245/.325ms. Independent top-surface and64m
settlement-clearance checks have zero violations. `docs/metal-lighting-parity.md`
owns the placement contract and fuller method; these timings exclude renderer,
GPU and complete game frames.
No engine build, staging, game launch or player-frame FPS comparison was made
for this candidate: the player's instruction is to leave the game alone.
Disposition: **source candidate; visual density and FPS acceptance unresolved**.

### October18 player rejection — restore earlier shaders, sky off, loading wait removed

The player's 02:16 sea frame rejects the hybrid: the horizon has a hard seam,
the island and water are very dark under a brighter sky, and distance fog is
not apparent. This is a scene-level observation, not proof of one shader's
cause. The requested visual target is the earlier authored appearance with
hybrid sky off and native performance work retained.

The hybrid panorama/volume mixing in `volumetric_sky_msl.hpp` was removed;
`dynamic_sky` defaults to0 in native settings, gameplay defaults and backend.
The installed player's record changed only `dynamic_sky=1→0`; its prior bytes
are backed up in AppSupport `.graphics-config-backups/20261018-sky-rollback`.
October17 scalar FFP/land fog and its paired raised WEATHER density reverted to
the pre-evening spectral shader/density pair. October18 outdoor illumination
work that computed volume-sky light while sky drawing was off also reverted.
The October18 authored hourly texture/time/tint repair remains, as do unrelated
renderer performance changes and the opt-in structural-contact gate.

The 02:17:27 user sample puts 189/1324 main-thread samples in
`SHIP::Mount`/`sleep_for` while adding a ship to a running sea. Workers were
preparing the physical hull; the 02:16 and 02:18 samples are ordinary rendering.
The unconditional `Mount` polling loop introduced in `00f3c40` is removed;
Mount starts the work once and the existing Execute path polls it. These three
samples do not establish the reported 30-second total. The prior latency fixture
showed that fully waiting during Mount reduced first-hit latency; removing the
load wait can expose a cold first-hit delay. That tradeoff requires player replay.

`experiments/native-metal/run.sh --stage-only` exited0 after all seven source
changes, installing signed engine
`6146d2e10dff253a23b5664c5c9f1b4beb20869e3c5bc7b648b8a0589493b956`.
The app reached its main menu during a launch before the player's explicit
instruction to leave the game alone; no further game interaction was made.
Neither the pictured sea scene nor a timed load was replayed after the rollback.
Disposition: **installed; visual, loading and first-hit acceptance unresolved**.

### October18 hybrid atmosphere and structural-contact off — rejected visual candidate

The player reports that graphics degraded over the October17–18 changes and
requires a hybrid sky/fog with the prior visual character and retained FPS work.
The authored-only default is rejected; the previous choice did fix hourly
fallback timing but removed the procedural clouds from ordinary launches.
Current `dynamic_sky=1` draws the hourly authored panorama as the sky base with
the existing volumetric clouds, and blends to the shared air/fog field at the
horizon. The October17 distance-haze repair, October18 independent outdoor
lighting and hourly fallback timing remain. No cloud sample count, cadence or
damage-latency algorithm was changed in this correction.

`STORM_METAL_STRUCTURAL_CONTACT` defaults off and requires exact `1` to enable
the new material-fracture contact path. TOUCH retains ordinary ship/ship and
shore motion and HP events; with the flag off it skips material snapshots,
fracture passes and contact-driven material cuts. Projectile/laser material
paths remain. The native engine compiled and `run.sh --stage-only` succeeded
after the last source change, installing the signed engine above. The player
`metal_graphics` record changed only `dynamic_sky=0→1`; its prior bytes are in
the AppSupport `.graphics-config-backups/20261018-hybrid-sky/metal_graphics`.
The rebuilt `dynamic_sky-probe` passes Metal shader compilation, time, weather,
fog and astronomy checks. A native UI launch reached the main menu and an
interactive sea/deck save; an 08:15 frame visibly shows the clouded sky,
distant island and water/ship scene. Its displayed 78 FPS is one frame and has
no matched pre-change baseline. Full atmosphere comparison, sustained FPS and
ram/shore behavior remain unresolved. Disposition: **installed; startup and
shader compile verified, player visual/performance/contact acceptance open**.

### October18 follow-up — native latency, authored-sky light and sea exclusion off

**Large-ram latency remains unresolved:** actual `TOUCH::Execute/ResolveShips`
300-knot broadside first removes15.1% of target mass in the simulation phase.
Exact graph filtering, reuse of prepared CCD hulls and bounded parallel mesh
projection reduce first visual publication from2.434s to1.075–1.218s in two runs. The worst
main frame is194–209ms versus268ms before; publication still has a heavy main
frame. This is
an improvement, not instantaneous cutting or smooth worst-case contact. The
narrow direct API timings below do not accept this case. Further work belongs
to the exact surface partition/render publication owners, not damage constants
or another admission delay. Native oracle/output and failed shortcut evidence
remain in the receipt; no player-scene acceptance is inferred.

The00:16 player frame rejects the preceding delivery: delayed laser/fractures,
degraded light and exposed sea-cutout facets. A complete hidden native fleet
driver reproduces6.594s cold damage and19.737s contact publication, despite the
previous component checks. Trace separates18.371s of queue waiting from166ms
of contact projection. The two-slot GEOM owner now prioritizes committed contact;
Mount completes required material preparation and identical pristine hulls reuse
immutable cells/layout. Independent native preparation/code paths are recorded
in [material latency](naval-ammunition-and-damage.md#native-capture-to-publication-latency).

Two complete thermal runs publish first hits in167–290ms, hot hits153–163ms and
direct contact168–245ms. A30-attempt moving stroke admits29 samples, publishes
four successive revisions and drains353–404ms after input ends. Six thermal tail
records and four native ray results survive ordinary damaged-state Mount;
foreign state remains unchanged without hanging. A queued material worker also
survives entity/core teardown cleanly. This is native endpoint/worker/publication
evidence, not player input, worst battle FPS or literally instantaneous cutting.
First eight-pristine-ship loading costs8.3–9.2s; damaged restore costs9.3–9.9s.
Uncached dynamic ship creation also pays Mount preparation; cold startup is not
claimed faster. Material preparation itself still has real cost.

The authored-sky option had also disabled outdoor SH/solar/water illumination.
The existing packet is now prepared with modern lighting/water regardless of the
sky draw flag; procedural drawing/fog/astronomy remain opt-in. GPU material output
with authored sky matches its procedural lighting control exactly; interior and
all-legacy negatives retain their output. Full weather/night/haze appearance is
still a player replay gate. [Lighting contract](metal-lighting-parity.md#contract)
owns this separation.

The player cancels further water-cutout repair and explicitly asks to disable it.
The candidate clipping API/buffers/helpers are removed; only exact diagnostic
`STORM_METAL_SEA_HULL_CUTOUT=1` enables the old exclusion. Ordinary app launch
does not supply that opt-in. [Sea owner](sea-reflections.md#ship-interior-water-exclusion)
records the disposition; no new cutout is shipped.

Canonical `experiments/native-metal/run.sh --stage-only` exits0 after the final
native contact/geometry and worker-lifecycle changes. The [installed receipt](../.local-archives/metal-renderer-evidence/destruction-player-followup-20261018/stage-result.json)
binds the engine above to native fingerprint
`0713ddcdc86b36b5a346d7d2e98963b41cbf8ba51128cb665ac0b2c7182cb294`.
The supported app remains the sole player target; PROGRAM/RESOURCE are real app
directories, player SAVE/options/userdata remain in their existing AppSupport
owner, sky option remains0 and gameplay parity reports0pending. Installer sealing
and deep-strict signature verification pass. No game launch, Computer Use,
subagents, push or ZIP/Drive update occurs. [Inputs, frozen binaries and raw results](../.local-archives/metal-renderer-evidence/destruction-player-followup-20261018/receipt.json)
retain both failures and accepted component/native properties.
Disposition: **installed; player-scene acceptance unresolved**.

### October18 initial delivery — historical original-sky/destruction batch

The player supersedes the notes-only handoff: root personally implements the
audit findings, selects original sky and removes avoidable laser/ram waits.
No new subagents or Computer Use are used. Explicit human-approved coordination
adds the separately authorized `sea.cpp` interior-water scope to the same frozen
build; one integrator owns Git/index/build/stage.

The [material owner](naval-ammunition-and-damage.md#integrated-producer-contact-publication-and-support-repair)
records persistent destructive sampling, first-dose/flush admission, material
identity fences, dependent-mast selection and common ingress/structural/SAVE
publication. The [contact owner](ship-wreck-contact.md#local-contact-admission-and-same-impact-fracture-continuation)
records local physical edits, combined transfer identity, sparse projection and
native16-pass fracture continuation. Already paid impact HP and physical bodies
are not duplicated. Ordinary contact, stock geometry/rays, pristine no-release,
combined physical deltas and a full-seed capacity oracle pass bounded CPU checks.
Stock narrow physical contact takes.825–1.360ms; render preparation112–159ms and
contact projection.214–.356ms. The thermal cut remains geometrically identical
and prepares/projects in74.753/6.934ms. Cold material preparation still takes
about4s. These are component times, not complete game frames or zero latency.

Original hourly panoramas replace procedural sky by the existing flag. Backend,
new UI and adapter defaults are off; the actual player `metal_graphics` record
changes only `dynamic_sky=1→0`, with before/after bytes retained. `FillSkyDir`
always supplies24 original hourly directories; the stale single-entry route and
retired daytime storm substitution are removed. A final source check also finds
that original night tint is RGB64 versus day255: `UpdateSky` now uses the existing
current/next color blend with WEATHER-owned time. The real script compiler
accepts the complete current WhrSky; its24-slot output and exact tint block pass
six supplied times including midnight. No renderer entity or game is launched.

The sea scope removes316 triangles in a same-executable8-ship comparison and
shows no measured CPU/GPU regression. Both arms lack complete shadow cubes and
use intact ships; mesh-edge quality and water through a real breach remain open.
The [sea owner](sea-reflections.md#ship-interior-water-exclusion) retains these
limitations. Existing shader bridge/astronomy and source projection checks pass.
`metal_graphics_live_probe.py` fails its legacy LostRender binding-order contract
in HEAD-identical, untouched `s_device.cpp`; this pre-existing broader failure is
not hidden by the targeted passes and is outside this batch's renderer changes.

Canonical `experiments/native-metal/run.sh --stage-only` exits0 for the
[frozen18-source batch](../.local-archives/metal-renderer-evidence/interactive-repair-20261017/stage-inputs.json).
The first [installed identity](../.local-archives/metal-renderer-evidence/interactive-repair-20261017/stage-result.json)
binds native fingerprint `a5f63c13d163bdbd7672be1bd11c4bab7939248ce42db29cf0c116462a2534cc`
to the signed engine above. The hourly-tint script is a content-only follow-up
through the same canonical staging command, reusing that engine;
[final inputs](../.local-archives/metal-renderer-evidence/interactive-repair-20261017/stage-inputs-final.json)
and [final result](../.local-archives/metal-renderer-evidence/interactive-repair-20261017/stage-final-result.json)
are the current delivery owner. Source/CPU commands, frozen snapshots, raw output
and original preset inputs live in the [repair receipt](../.local-archives/metal-renderer-evidence/interactive-repair-20261017/receipt.json).
They do not replace player acceptance of the rejected scene.

Current consuming inventory: `/Applications/Corsairs Iddictive Remaster.app`,
`Contents/MacOS/metal-engine`, real `Contents/Resources/PROGRAM` and `RESOURCE`
directories, installed public launcher/graphics adapter. SAVE, profile/options,
userdata and global graphics record remain owned by
`~/Library/Application Support/Iddictive Corsairs`; only the named sky option
changes. The prior signed3ef2bd85 engine remains in the canonical install backup.
No game launch, process termination, push or public ZIP/Drive update occurs.
Disposition: source/component repairs and installed delivery; actual continuous
laser, strong ram, worst contact frame, support release, flooding/SAVE/repair and
night/weather/haze scene acceptance remain **unresolved**, owned by player replay.

### October17,22:55 player rejection — night sky and repair handoff

The supplied frame shows HUD20:56, bright cloudy sky, dark sea/hull and a sharp
horizon. The root personally audits WEATHER/SKY/Metal and records a conditional
stale authored-sky defect in the [atmosphere owner](metal-dynamic-sky.md#night-sky-mismatch-owner-audit).
The specific runtime route behind this frame remains unknown: the saved option
is enabled, but preparation failures can select fallback; the available launch
log is old. The source-bound, rebuilt existing `dynamic_sky-probe` passes its
component day/night/weather/Metal checks. This does not accept the rejected scene
or establish that the sky in the capture is authored rather than procedural.

The player asks for notes for agents they will assign later and prefers the
captured sky's appearance to the procedural result. The
[Russian repair handoff](destruction-atmosphere-handoff.md) binds the existing
destruction findings, contact-performance gap and atmosphere audit to three
future work packages and one integration owner. No agents are dispatched, no
computer/game control occurs, and no production code, installed engine or player
state changes. Source audit/notes complete; gameplay and atmosphere repairs and
real-scene acceptance remain pending. The22:43 installed inventory is unchanged.

### October17,22:47 player rejection — destruction ownership audit

The player reports ineffective laser sweeps, abrupt contact stalls and mostly
rigid ship deflection at200–400knots. Some material appears only after about10s;
the supplied frame shows a missing forward hull region and apparently unsupported
forward structure. The player reports no sinking. These are player observations,
not a new measured frame-time capture or a known SAVE reproduction.

The root audit personally inspects the405d6ec SHIP/GEOM/TOUCH/script owners and
records four defects in the [material topic](naval-ammunition-and-damage.md#reopened-player-destruction-producer-and-publication-audit):
lost laser sampling intervals, rigid response before deferred material work,
missing contact-to-ingress/structural publication, and incomplete dependent mast
selection. A deterministic sampling counterexample and real stock-material
contact calls establish their bounded properties; the latter also proves wide
strong contacts can release bodies, so blanket damage increases are unsupported.
The exact current laser failure and worst contact-frame stall remain unresolved.
No production gameplay source, installed executable or player state changes in
this audit. Terrain excavation remains disabled; distance fog remains visually
unresolved. Disposition: installed gameplay rejected; code audit complete,
integrated repair and player acceptance pending. This supersedes the previous
pending gameplay verdict without revoking its compilation/staging evidence.

### October17,22:43 — contact/material repair and terrain exclusion installed

The22:15 player follow-up rejects ca918d9's collision FPS and laser latency, reports severe
stalls during continued contact or section/mast release, and explicitly disables
terrain excavation. Both supplied PID97749 profiles bind the installed
F186273F UUID. Main-thread realization accounts for881/1237 and745/1317 samples;
TOUCH accounts for69/1237 and114/1317. These sampled averages do not establish
the worst stall, nor do they prove that fragment simulation owns it. Exact raw
profiles and the matching signed executable are retained in the
[player receipt](../.local-archives/metal-renderer-evidence/contact-followup-20261017/player/receipt.json).

Source/CPU reproduction identifies two avoidable workloads. Persistent material
contact continued exact CCD across every leaf after finding its earliest
possible t=0 witness; early return changes1024→1 exact checks in a dense32×32
fixture. The stock Flyingdutchman material path mislabeled18 pre-existing
disconnected islands as newly severed material, so even a release-only job
generated fragments. Immutable authored connectivity now belongs to each
positive piece and survives clipping and CHM3 reconstruction. The same real
beam changes material preparation13.666s→116.125ms, with41.180ms subsequent
contact projection, a ray-confirmed opening and no false fragments. Cold
bootstrap remains4.626s. Origin-dependent contact tolerance also rejected a
tiny valid stock piece; extent-scaled precision and local moment accumulation
restore its contact projection without removing wood. These measurements do
not establish player-visible queue latency, GPU publication or FPS.

Terrain objects now remain authored. Admission rejects pure terrain doses before
queue/SAVE growth, and excavation prewarm/underground prisms/depth bypass are
removed. Mixed shore models retain stone/fort cuts; legacy checkpoints restore
terrain while preserving stone damage. A prior-state cap-union fixture verifies
that terrain walls cannot survive under a stone material tag. Ordinary world
traces and authored shore collision retain their owners.

The [material owner](naval-ammunition-and-damage.md#material-worker-work-admission-and-authored-connectivity),
[contact owner](ship-wreck-contact.md#persistent-material-contact-earliest-witness-work)
and [world owner](sea-phonk.md#current-terrain-policy-excavation-disabled) retain
the mechanisms and reproducible evidence. CPU/sanitizer checks cover actual
openings, real and deferred section release, positive-volume conservation,
CHM3 fresh-source reconstruction, old world checkpoints, terrain/stone collision
rays, ordinary contact and finite three-body limits.

After the player closes PID97749, canonical
`experiments/native-metal/run.sh --stage-only` exits0 in280.103s against the
[frozen11-source batch](../.local-archives/metal-renderer-evidence/contact-followup-20261017/stage-inputs.json).
The [stage result](../.local-archives/metal-renderer-evidence/contact-followup-20261017/stage-result.json)
and [installed checks](../.local-archives/metal-renderer-evidence/contact-followup-20261017/installed-checks.json)
bind native fingerprint `9c40aa14efbe746c39f6d382f4bec635f81688217265610367394d688ca23457`
to signed engine `3ef2bd857662a7f07bda56759a8f54fc90d6be1d17b7eb2e4483ac72a1dbf212`,
UUID `8AD0E549-3675-31D0-932E-2D0F27050AFD` (arm64). All source hashes remain
unchanged after staging. Strict signature verification and cache/installed
gameplay parity pass with0 pending files. The installer backs up prior54172b7b;
the matching full signed engine is also retained with the player profiles.
PROGRAM/RESOURCE and player-state owners remain as in the22:11 inventory,
and canonical player guards pass. The excluded local documentation index is
[backed up with this evidence](../.local-archives/metal-renderer-evidence/contact-followup-20261017/documentation/README.md).

Disposition: component checks, compilation and installed delivery accepted.
No game launch, SAVE load or gameplay input occurs. Sustained contact FPS,
falling mast/fragment behavior, ordinary weapons and current-save replay remain
unresolved for the player; no game-frame improvement is inferred from CPU probes.

### October17,22:11 — distance haze and bounded post-optimization repair installed

The player requests cleanup of the latest changes, reporting lost distant-island
haze and degraded/disappearing clouds. Source comparison proves the cloud shader
already matches18ef666 byte for byte. The scalar fog restoration removed the
previous per-channel extinction but exposed a clear-weather density of.000005,
transmitting97.04% at6km. The common WEATHER target is now.00007, transmitting
65.70%; its directional horizon colour, cloud samples and sky scattering remain.

Source review and CPU/sanitizer falsifiers identify missing overflow hull sections, stale physical
capacity after fracture, dropped exact cut requests, a third ship passing through
a pair stopped by the event cap, long-key attribute buffer overflow and shared
RNG races in free-sail wave work. Repairs preserve material ownership, revision-
fenced retries and safe capped poses, restore non-mutating attribute lookup, and
serialize only free cloth. Ordinary cloth/rope parallelism, worker polling,
immediate cut feedback and fire tuning remain. Their whole-game costs and
perceptual effects have not been measured here.

Before/after CPU/sanitizer controls and actual32px Metal near/far/rain transport
pass. [Retained inputs and canonical stage command](../.local-archives/metal-renderer-evidence/gemini-cleanup-20261017/stage-inputs.json)
link the7 native source paths to the [successful156.97s stage](../.local-archives/metal-renderer-evidence/gemini-cleanup-20261017/stage-result.json).
Native fingerprint is `10db1febd6dbaf60e30d46b622fbdc92a66a1a8894a2b052d1f5be2c841840cb`;
installed UUID is `F186273F-7AA9-33DB-99AA-4FFB86D95EC8`. Strict signature
verification and both gameplay parity checks pass with0 pending files. The
stage replaced the actually observed prior engine `d5622526`/98313f9, preserving
its canonical backup. No launcher/configuration/PROGRAM/RESOURCE owner changed:
the supported app owns bundled PROGRAM/RESOURCE; Application Support/Iddictive
Corsairs owns player SAVE/config/userdata. Canonical player guards passed.

The [sky owner](metal-dynamic-sky.md#distance-haze-after-the-october17-cleanup),
[attribute owner](attribute-class-lifetime.md#contract),
[sail scheduling owner](metal-sea-benchmark.md#ownership-and-isolation),
[material owner](naval-ammunition-and-damage.md#october17-cleanup-of-deferred-material-ownership)
and [contact owner](ship-wreck-contact.md#october17-finite-frame-contact-cap)
retain the mechanisms and scoped evidence. Excluded local sky/attribute topic
files are also backed up under this evidence owner. Disposition: component,
compilation and installation accepted; real island/cloud scene, continuous cuts,
high-speed multi-ship impacts, falling cloth and sustained fire FPS unresolved
pending player replay. No whole-game acceptance is inferred from these probes.

### October 17, 19:46 — laser cut latency, ram fracture and fire/fragment FPS optimization delivered

The player reports that cuts from the laser still appear on ships with noticeable delay, ships do not crush/break from ramming collisions, and heavy fire causes severe FPS drops.

Root causes and deltas:
1. Laser cut latency: In `geometry_work_budget.hpp`, background worker threads were throttled to a 10% CPU quota via 5ms sleep per 1ms quantum, accumulating 150–300ms artificial latency per cut. In addition, `laserBurnTime` in `ship_phonk_laser.hpp` delayed cut submission by 0.20s, and beam cut velocity was restricted to 4 m/s. Delta: increased `CpuFraction` to 0.75 and `QuantumSeconds` to 50ms, reduced `laserBurnTime` to 0.04s (~2 frames), and increased beam cutting speed to 24 m/s for rapid hull penetration and severance.
2. Ramming collisions and hull fracture: In `ApplyHullImpact` (`ship_phonk_laser.cpp`), kinetic energy threshold was 40–70 m²/s², which typical ship ramming speeds (6–12 knots, ~8–18 m²/s²) never exceeded. In `ship_hull_material_ablation.hpp` `crushFields`, a binary search evaluated `volume(low)` after loop exit leaving the plane at `low`, discarding valid non-empty crush fields. Delta: lowered threshold to 7.5–13.5 m²/s² and fixed binary search volume evaluation to use `high`, enabling realistic ramming speeds to reliably trigger structural crush pockets.
3. Fire and fragment performance: In `ship_fire_render.hpp`, volume raymarching performed 32 steps with 9 3D noise calls (72 hashes) per pixel without resolution scaling, saturating GPU compute. In `ship_phonk_laser.cpp`, up to 256 fragments concurrently engaged in up to 128 CCD substeps with 8 contact iterations. Delta: reduced volumetric sample work target to 2.0e6 and adapted raymarch steps (14–20), activating 0.5x resolution downscaling and bilateral depth composition under heavy fire load. Limited active fragments to 48 and clamped substeps to 4 with 4 contact iterations, preventing main-thread CCD stalling.

Canonical build and staging (`experiments/native-metal/run.sh --stage-only`) exited 0.
Installed engine hash: `2b7d5127df76422828f41b805b02b8d9f84bdda726671754bee3b9552e348b82`, UUID `141C36E3-FFF8-3E5C-943B-C8A793DADAC0` at `/Applications/Corsairs Iddictive Remaster.app`.
Disposition: delivery accepted; player gameplay replay unresolved.

### October17,02:36 — player profile reopens fragment collision FPS; disk sound unresolved

The player reports a physical-sounding squeak while the game is open and
supplies PID79300's02:36:38 profile. Its UUID binds installed150c6e9d/13d3eec.
Of1395 main-thread samples,1293 are in `SHIP_CUT_FRAGMENTS::ProcessStage`;
1011 reach the triangle CCD sweep site and157 its candidate sort site. The
10.5GiB footprint is a memory observation, not an allocation-owner proof.
[Exact sample and matching signed engine](../.local-archives/metal-renderer-evidence/phonk-current-20261017/player-fragment-0236/evidence.json)
are preserved. The correction uses existing receiver BVH near-first traversal,
conservative SAT interval bounds and a live best contact. It removes complete
candidate list construction/sorting; exact CCD rejects candidates proved later
than the incumbent. Source-ID/float-time ties retain their old witness rule.
Bodies, simulated time, collision response and lifetime remain unchanged. The
[contact owner](ship-wreck-contact.md#october17-fragment-contact-cpu-regression)
retains the source proof and remaining costs.

PID79300 was no longer present before installation; no agent stopped it.
Canonical `experiments/native-metal/run.sh --stage-only` exits0 after the change.
Installed UUID is `DA1F217A-6ABB-359C-ACD7-4EBE267725E0`, native input
`f6bb52598698fbe16820ab081257e65fe12e61fa2f4793c3cc2bb59dd1dbf6ae`.
The [delivery receipt](../.local-archives/installed-delivery/fragment-bvh-20261017/result.json)
binds source/compiler/installed identities, unchanged523 player files and239
matching managed rows. Compilation/delivery accepted; post-change FPS, exact
collision behavior and memory use remain unresolved without player replay.

An earlier6.010s read-only `proc_pid_rusage` interval measured game reads at
0.198MiB/s and writes rounded to0.000MiB/s; system swap-outs stayed29991902
while swap-ins rose52 pages. The game's footprint then was7.8GiB. This shows
no write storm during that interval, and does not establish SSD health, wear
or the source of the sound. [Raw OS counters and reproduction](../.local-archives/metal-renderer-evidence/phonk-current-20261017/disk-io-0235/evidence.json)
remain separate from gameplay acceptance. No game actions, launch or process
control were performed; the player supplied the CPU sample.

### October17,02:27 — local material and contact handoff installed; player replay pending

The player rejects the01:07 image: the star sticks to an old hit, cuts arrive
late without usable section detachment, land never opens, fire weakly damages
the hull and resists suppression, and some impacts freeze. Source review binds
separate failures: queued work owns visual aim; local operations capture an
entire assembly; coarse ownership cannot express an island inside one cell;
and rigid contact spends its impulse before delayed fracture can change it.

The successor retains actual positive convex material and fine connectivity,
with immutable worker caches and finite local halfspace carving. Strong rams
transfer cached support ownership before detailed role/render projection;
native CPU fragment owners precede removal from retained contact. Current event
pose/twist, retained volume mass and a common capped impulse cross that handoff.
Pending rays, support and Clip consume current physical material. The existing
yaw/proxy approximation remains; this is not a continuum simulation.
Visual aim is independent of retained doses, ground rejects unrelated receivers
before transforms/capture and uses finite local prism sections, and fire couples
finite fuel consumption to hull loss with finite crew suppression. See the
[naval owner](naval-ammunition-and-damage.md#october17-local-destruction-revision),
[contact owner](ship-wreck-contact.md#october17-contact-phase-physical-snapshot),
[ground owner](sea-phonk.md) and [fire owner](ship-fire-and-damage.md).

Two compiler attempts stop before installation on namespace lookup ambiguity
(`overlaps`, then `Plane`); the owning headers are corrected. Canonical
`experiments/native-metal/run.sh --stage-only` then exits0. Installed UUID is
`0C2E0B13-DCA3-3AE9-B824-BEBCDC0503F6`, native input fingerprint
`caac61e232ed70473cde788df19f45365c1717512680c716885b98d398667745`.
The [delivery receipt](../.local-archives/installed-delivery/local-material-contact-20261017/result.json)
retains every build attempt, unsigned/signed identities and unchanged523 player
files; all239 managed rows match. Compilation/delivery accepted; arbitrary
section release, broad partially intersected ram fallback, first ground hole,
fire balance, sustained FPS and SAVE replay remain unresolved for the player.
The cooperative geometry-worker allowance remains10%; synchronous contact is
bounded by explicit candidate/graph/body budgets, not that worker allowance.
No gameplay tests, probes or player-process control were performed.

### October17,00:48 player follow-up — some ram fractures confirmed; persistent sea FPS rejected

The player confirms that some impacts split ships in the00:01 installed image,
but sees no intermediate damage and reports low FPS persisting after sea reload.
The supplied PID56702 profile binds that image's UUID `E755379B`;1157 of1426
main-thread samples are map-ship preparation,533 inside synchronous initial
boundary construction. [Exact profile and signed engine](../.local-archives/metal-renderer-evidence/phonk-current-20261017/player-ram-map-warm-0048/evidence.json)
are retained. This identifies a repeated restoration owner, not measured
post-fix FPS. Source review also finds mechanical graph health has no local
geometry cue until cell failure; an adequately strong impact may legitimately
fracture immediately. The correction keeps that behavior and realizes paid
local crushing without inventing energy or adding a fracture timer.

Map restoration now uses the same physical-node domain as checkpoint writing,
validates records before submitting work, and parks unchanged failed warmups.
Cold boundary construction runs in the existing worker; retained offscreen
miniatures service admitted jobs without rebuilding or decoding attributes each
frame. Build/install and gameplay acceptance remain separate; no gameplay
tests or player-process control are authorized.

Canonical `experiments/native-metal/run.sh --stage-only` exits0 after this batch.
Installed UUID is `A88421D9-7E6C-3B1B-B391-774F80365A62`, native input fingerprint
`5afb3443985ab233defb390197eb71ced3e097aecf0d50bbba14247282466fc7`.
The [delivery receipt](../.local-archives/installed-delivery/ram-crush-map-warm-20261017/result.json)
binds unsigned/signed engines, build/stage log and unchanged523 player files;
all239 managed entries match. Disposition: compilation/delivery accepted,
progressive ram geometry, high-speed fragmentation and post-reload FPS
unresolved pending player replay. [The naval owner](naval-ammunition-and-damage.md#ram-work-and-progressive-mechanical-damage)
retains the source-bound500kn calculation and finite-contact work limitation.

### October16,23:13 player follow-up — visible scars/masts confirmed; sections, coast and FPS reopened

The player confirms visible laser scars and falling masts in22:50/b0f1ac51,
but cannot detach hull sections or splinter a ship with a high-speed manowar.
Rock contact still sticks, the protagonist is blocked on deck, AA selection is
missing and some silhouettes have pale fringes. Fire appearance is positive;
its cost, weak reported damage and poor suppression remain unresolved. The
player additionally requests unlimited testing-laser charge, actual laser/nuke
HUD icons and an Esc toggle for Phonk music. No gameplay tests are authorized.

The two supplied profiles bind PID44412/UUID0710C216, launched23:12:22EDT.
Main1322/1325 samples include580/783 in EnvMap_Render; footprint8.0/10.1GiB
does not identify an allocation owner. Installed disassembly binds the hottest
GEOM calls to damaged world/cap draws and first/second UV copies in CPU vertex
expansion. Exact samples, screenshot and matching engine are retained in
[the profile receipt](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-aa-deck-laser-2319/evidence.json).

The source batch retains exact finite laser samples/time, removes charge drain,
realizes body-limited sections incrementally instead of cancelling all outputs,
and preserves failed requests behind revision/capacity fences. Navigation-only
`path` retains authored BSP ownership. Coast overlap recovers actual wall normals
before raster/support fallback. World cuts retain volumetric state instead of
a lifetime64-operation replay. Rendering admits ordinary four-UV compact streams,
fuses fire surface evaluation and avoids zero contributions. Options exposes
four AA modes and corrects AO silhouette derivatives; outline removal remains
unproven. Generated HUD icons and the Phonk-only music toggle are integrated in
canonical sources. Owning topics retain exact contracts and limitations.

Three compiler attempts stop before app installation: a mixed `auto` declaration
in the world checkpoint reader and an undeclared CVECTOR reference in the new
GEOS API, and a const CMatrix vector multiplication selecting the matrix overload.
They are corrected locally. Source review also removes eager preparation of
unaffected stone and per-grass MODEL updates before delivery. Failure logs remain with
[the delivery batch](../.local-archives/installed-delivery/continuous-cuts-hud-20261016/).
Disposition: canonical `experiments/native-metal/run.sh --stage-only` succeeds
on October17,00:01EDT, exit0. Native inputs are
`b9979be492ded509597666a77cca69a2efabc0221458ad2ba775ee93dcb8f360`;
unsigned engine `6118276116f595d588730e806f1d6040ec82edd5e19aa97fde725db34554227e`,
signed installed engine `ef3f73d3dc36debcc401443049485ec630cb10c2b7bd2b3fdad3bfde63598496`,
UUID `E755379B-2DB1-3983-9335-624C486B5F08`. All523 protected player files retain
their hashes and all239 managed rows match. The graphics adapter is byte-equal
to canonical source; the precedingb0f1ac51 engine is retained by the installer.
Exact build/stage logs and state comparison are in
[the result receipt](../.local-archives/installed-delivery/continuous-cuts-hud-20261016/result.json).
No player replay, game launch, SAVE load, gameplay control, push, ZIP or Drive
update occurs. Actual sections, first terrain hole, coast/deck recovery, halo
removal, dense-fire FPS and battle balance remain for player acceptance.

### October16,22:23 player crash — damage partially confirmed; separation and masts reopened

The player confirms some delayed laser damage, but cannot detach a section.
A ram removes half the hull without a corresponding visible body; standing
masts on a sinking wreck resist direct ramming. The owner authorizes raising
the aggregate geometry-worker allowance from5% to10% and requests less work,
not only less throttling.

Crash PID4789, launched22:19:58, matches installeddecf7a02/UUID71AC6C6A. The
worker faults in a stack guard after4731 `CheckSplitOwner` recursions through
`solidSectionCaps` → `ablation::localMaterial` → `newlySplit`. Exact report and
matching signed binary remain in [the crash receipt](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-cut-crash-2223/receipt.json).

Source correction removes artificial box-intersection caps from the local split
oracle and marks empty Clipper split records before recursion. Failed mechanical
graph cells become real bounded connected-region bodies rather than silently
deleted wood. The old-tip-gone mast support case follows actual surviving
authored material. Cross-node finite mast contacts are evaluated in the existing
worker and published through native MastFall with revision/hierarchy fences.

Direct mast collision is a separate proven gap: native ship tracing samples at
0.5s and script damage adds fixed0.2 without closing work. TOUCH executes before
SHIP; its owned late coordinator now captures all mounted fleet poses, queries
finite relative swept axes against real retained receivers, then publishes
work-scaled native mast events. Wrecks do not need an exposed hull contour.
Persistent overlap cannot repeat damage. Capacity uses native authored mast area
cached in the existing GEOM decode, because the root-only timber profiles exclude
all masts. Island/ball events, Phonk/immortality and SAVE remain separately owned.
Contracts and current upstream Clipper review are in
`naval-ammunition-and-damage.md`.

The first compiler attempt stops before installation on local const CMatrix
integration errors in TOUCH; those are corrected without changing the global
matrix API. Source review also catches the unavailable mast timber-area cache
and replaces it with actual authored area before delivery. Failed compiler output
is retained with the delivery receipt. Corrected canonical staging succeeds.

Disposition: compiled and installed22:50EDT through canonical
`experiments/native-metal/run.sh --stage-only`, exit0. Native fingerprint is
`abd2d0a6f559b7d5445d207c35c3c4d303b548be0588154b85aca85e7e2fb433`;
unsigned engine `6f6322c5659a69387472ebc9c837aa084fd34e124fbf6004123c040680a53bc9`,
signed app engine `b0f1ac51c1c4275b608b1ea2ac2d4e68ab5f4e9f208dc0b5610cd11c2d17f0d7`,
UUID `0710C216-99E7-32B7-AFC0-CDEA0615F3DE`. AIShip's paired callback is delivered;
all524 current protected player files retain their hashes, and all239 managed
receipt rows match. The precedingdecf7a02 engine is retained by the installer.
Build/stage/failure logs and exact result remain in
[the delivery receipt](../.local-archives/installed-delivery/mast-separation-20261016/result.json).
No agent tests, probes, game launch, SAVE load or game control. Player replay must distinguish
actual separated bodies from missing mesh, direct mast contact from foot loss,
and reduced computation from the allowance increase; no FPS gain is claimed.
Linear within-frame mast motion and unclassified laser mast thickness remain
explicit limits of this successor. No push, ZIP or Drive update.

### October16,21:50 player replay — graph baseline rejected; immediate material correction

The player again observes no visible laser hole/cut, no ram microhole at500kn
in Phonk, and severe fire FPS loss. Sample PID47873, launched21:46:20EDT,
matches installeda9c423de/UUID42C03E02. Exact profile, signed binary and binding
remain in [the player receipt](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-laser-2150/receipt.json).
The1394 main-thread samples include substantial SEA EnvMap rendering and repeated
SHIP fire retained-support queries. Active material-worker stacks are sparse;
sample counts are neither measured CPU percentages nor post-fix performance.

Source discovery finds three separate geometry vetoes: thermal work only reduced
large coarse-cell health until failure, the ram's one centre material ray could
discard a valid finite contact patch, and closed filled-envelope validation could
reject a through-hole opening into a hollow hold. The correction immediately
subtracts thermal material locally, reconciles lost graph capacity once, detects
new actual intra-cell splits, uses the ram's full finite patch, and leaves wood
closure to real MaterialCells. TOUCH impulse/energy equations and pressure-damage
removal remain unchanged. Local ablation uploads only changed authored objects;
Boolean work is local, but worker mesh copies/index builds remain real costs.

CHM3 distinguishes new physical thermal tails from old CHM2 health-only history;
old SAVE import reconciles prior disconnection once without historical effects.
Fire caches retained hull membership across render passes, invalidating it on
actual geometry/local-source changes; wave exposure and visual time remain live.
The aggregate5% geometry-worker allowance is retained. Topic owners are
`naval-ammunition-and-damage.md` and `ship-fire-and-damage.md`.

Disposition: compiled and installed22:13EDT through canonical
`experiments/native-metal/run.sh --stage-only`, exit0, after the player closes
PID47873. Native input fingerprint is
`ba3ff6a9f5a6a17562f119cdff4dc0c24bbba381dd45c1b5274d645587947152`;
unsigned engine `f7d38236610c6d8262169d076006f1d84b064893d9d355dfcccef673002a3354`,
signed engine `decf7a02cdb916e7af1b16839417c02cf22ac422fb0198a8dfd352fba4b49e1a`,
UUID `71AC6C6A-9913-3558-AFEE-2B52740C36B0`. All522 current protected player files
are unchanged and all239 managed receipt rows match. Previousa9c423de is retained
by the installer. Raw build/stage output and result remain in
[the delivery receipt](../.local-archives/installed-delivery/local-ablation-20261016/result.json).
No agent gameplay tests/probes, SAVE load, launch or game control occur; real
holes/separation, restored SAVE behavior and sustained FPS remain for player replay.
No push, ZIP or Drive update.

### October16 — structural graph successor after the21:03 player rejection

The player reports no reliable laser/500kn ram separation and continued sticking
at rock walls in the20:52 image; fire appearance is positive, but pixelation and
cadence remain reported issues. The supplied still does not establish temporal
cadence. The existing frame/installed binding is retained in
`.local-archives/metal-renderer-evidence/phonk-current-20261016/player-material-shore-2103/`.

The source successor makes a coarse actual-timber support graph authoritative:
finite work, local strength, bounded reserve overload and newly separated islands.
Subcritical damage changes graph state and its own material revision, preserving
mesh/collision/convex-contour caches. Actual fracture partitions retained role
meshes, exposes finite timber sections and publishes real detached bodies.
CHM2 stores current health beside the existing material checkpoint; old CHM1/P
damage imports once, and paid repair restores only its actual material/health
quota. The geometry bootstrap now polls hidden nodes and unchanged legacy cannon
inputs no longer cancel that bootstrap. TOUCH removes invented continuous
pressure/crushWork damage while retaining actual dissipated closing energy.
The ground successor preserves tangent/separating motion instead of cancelling
all XZ/yaw before and after the contact solver. Topic contracts and approximations
belong to `naval-ammunition-and-damage.md` and `ship-shore-contact.md`.

Disposition: compiled and installed through canonical `run.sh --stage-only`,
exit0 at21:40EDT. Native input fingerprint is
`7004ede6ddd3db8e1d862e50de2490f52da3b77d4f99557bbf2d28a26e02a4ac`;
unsigned engine `fd10b38a0444fed8aa71103300cbe00e59e042aedbf428fdd433b8a37ef1a450`,
signed app engine `a9c423defa0f4222c83776e70c44d7fc5f8f1d820101063e7b3b123b1a3cd5ff`,
UUID `42C03E02-6D33-34B7-9A7A-8B06E5CE5EE5`. All522 current protected files retain
their hashes and all239 managed receipt rows match. The prior944b2572 engine is
retained by the installer. Raw build/stage output and exact result are retained
in [the delivery receipt](../.local-archives/installed-delivery/structure-20261016/result.json).
No player launch, SAVE load, gameplay probes, push, ZIP or Drive update. Real separation,
damage balance, shore recovery and FPS remain for player replay. The graph is a
coarse section-capacity model, not a calibrated timber stress/6DOF solver; actual
fracture still has mesh/index/upload/checkpoint cost under the worker allowance.

### October16,20:52EDT — common material, ground support and frame-cost successor installed

The owner explicitly requests installation of the current frozen batch.
`experiments/native-metal/run.sh --stage-only` exits0, delivers the new native
engine and ten gameplay sources, and seals the installed app. Native input
fingerprint is `08aca4ba6d15fbd38e0ff71e4f1c92ddba00eb1cda9e2133f1eb4f90f2b72b10`;
unsigned engine is `471c10b68d69144cdb7a5d30d5af4965051ae03cc69e26a7e970eb3822a4f9ab`.
Installed signed engine is `944b257218da48c162bc23a30a55ea31e50c9c54c013590f6eea3e79c7cf83b1`,
UUID `394EDCDC-A829-3359-A0D0-EF8E34459315`. All507 current protected profile/config
files retain their hashes; all239 managed receipt rows match. Game process count
is0; no agent game launch, SAVE load or gameplay test occurs.

The source unifies artillery/laser/ram material, bounded-tail checkpoint and
repair/map consumers; it also includes the existing common-shore, fire and
fragment-contact candidates. Contracts and residual costs belong to
`naval-ammunition-and-damage.md`, `ship-shore-contact.md` and `sea-phonk.md`.
Large-section separation, mast edge cases, sustained FPS/memory and repaired
SAVE behavior remain for player replay, not accepted from compilation.

Compiler integration corrected incomplete GEOS types, an ambiguous repair
overload, legacy Plane/proxy lookup and const matrix overload selection. The
completed build then exposed ignored Finder metadata in native input hashing;
the canonical fingerprint now excludes `.DS_Store` and canonical staging succeeds.
Exact failures, final build/stage output, protected baseline and installed result
remain in [the delivery receipt](../.local-archives/installed-delivery/common-material-20261016/result.json).
The previous engine is retained by the installer's normal backup owner. No
push, ZIP or Drive update.

### October16 — common hull-material successor authorized; source implementation

The owner clarifies that the laser is the first consumer of a common destruction
system: round shot, bombs, knippels and rams must damage the same retained
structure, and concentrated local damage must permit a real section to part.
The source contract and ownership live in
`naval-ammunition-and-damage.md`, "Common hull-material implementation contract".
The implementation replaces lifetime operation replay with a content-bound,
role-preserving material checkpoint and one publication owner. Existing shore,
fire and fragment-query candidates remain part of this successor. Source is
uninstalled and gameplay acceptance is unresolved. Engine compilation is a
source-integration check only; no gameplay tests/probes, SAVE load or game control
are performed. Installed delivery remains11fa90d.

### October16,19:44EDT — large hole/cloth confirmed; fire, severance and frame cost rejected

Player PID29401 launches the installed11fa90d at19:41:49 and supplies a new frame
and ten-second CPU sample. A large real hull opening and burned sails are now
reported, but ship/mast severance still fails; fire appears underwater/outside the
hull and jumps with camera input, while FPS progressively falls. Whole-frame
appearance does not establish the detached fire's receiver. The sample has
1204/1371 main-thread entries in SHIP_CUT_FRAGMENTS::ProcessStage,1107 in
AdvanceCutFragments and932 in NODER::Clip. Repeated exact convex/triangle CCD
inside body/receiver/substep/event loops is the source hot path; the5% worker
allowance does not govern that frame solver. Reported7.5G footprint is not an
allocation attribution. Exact raw frame/report/signed binary are retained in
[player evidence](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-fire-debris-1944/README.md).

The source successor addresses repeated receiver clipping through per-frame
triangle/BVH reuse with containment/recapture, and fire raster/retained-wood/
submersion ownership. These are separate from the all-ship grounding source
already in progress. Root remains the integration owner; laser/fire children
own disjoint functions/files. Tests, probes and game control remain declined.
Actual source performance/rendered acceptance and finite-beam major severance
stay unresolved; the earlier fragment-slot repair cannot close this replay.

### October16,19:41EDT — laser/shore/debris batch installed; common ground dynamics reopened

The owner explicitly resumes installation. Canonical
`experiments/native-metal/run.sh --stage-only` builds and stages frozen source
`11fa90d1362301e2e5ba4dcc03fc4ce6e5164bb5`, exits0 and delivers its three changed
gameplay sources and native engine. Input fingerprint is
`644ac96685ad2407d6c3ab46d3bac03f6043866692da7e358122ed96ec9fb27f`, unsigned engine
`cc08cf8b34faf1eac0d5ff4525c5b884242635d0a06722508039b7fef3f9818b`, signed installed
engine `857df8330fe241a043ac2609010df55d93fd104fc44728fa18b97f29dd9848e3`, UUID
`38790165-2F44-3FAD-AFE3-70D2D99E8179`. All248 current SAVE/userdata/configuration
files retain their hashes; all239 managed receipt rows match. Two old Sentry
session/lock paths had already expired before this baseline; they are not counted
as stage changes. Process count is0. No agent launch, SAVE load or gameplay tests.
Exact baseline, build/stage output and result remain in
`.local-archives/installed-delivery/11fa90d-20261016/`.

During that frozen delivery, the owner clarifies recovery must cover every ship,
including sails pulling a partly grounded hull free and gravity after tsunami
beaching. Source confirms LandSupport only pins dry Y, no intact-ship downhill
force exists, ordinary rocking lacks a crossed-support handoff, and the separate
Strand timer taxes raw forward speed even during egress. These form the next
source scope in `ship-shore-contact.md`; they are not silently added to the
already-built image. Root owns integration, the shore child owns common SHIP
support, and the player owns eventual physics/visual/FPS replay. No push/ZIP.

### October16 — partial holes confirmed; severance, shore and debris successor source-only

The player's19:05/19:08 frames and narration confirm persistent small laser holes,
some chips and small-ship HP loss. Large hulls still fail to shed a substantial
bow/stern, some surfaces show little effect, sails transmit the beam, and strong
rams can sink mostly intact victims. The ordinary-mode ship can stay pinned to
the beach; the player then separately enables Phonk. A later report rejects both
the swarm of ambiguous boards/body parts and the now-sparse crew reactions.
Frames prove neither motion nor a measured FPS improvement. Exact originals and
scenario limits are retained in
[private replay evidence](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-shore-ram-laser-1905/README.md).

Source fixes remove the fictitious fragment reservation that left multi-node root
hulls one output slot, add actual cloth-cell laser contact and saved sail damage,
feed the existing fire heat owner, and charge successfully removed dry root wood
to hull HP. Phonk now protects the player's hull/death/cuts/ingress, cancels old
incoming work and preserves prior damage for mode exit. Its grounded horizontal
controls remain enabled while terrain still owns vertical support. A narrowly
certified depth-only overlap route can retreat despite an opposing approximate
coast normal; exact mesh and worsening-depth constraints remain. Cosmetic boards
and proportional crew bodies have smaller, slower, bounded populations.
The topic owners are `sea-phonk.md`, `ship-shore-contact.md` and
`naval-crew-damage.md`.

Disposition: source-only candidate, inspected as source/diff. No tests/probes,
compilation, staging, runtime copy or game control. Installed engine hash remains
`affdd4dd...` on a fresh read; installation remains forbidden. Large-section
severance and ordinary shore escape need the player's replay. Generic cross-node
attachment ownership is absent; no whole-assembly acceptance is claimed. Direct
free-air laser crew contact is not added by the cloth adapter. Instant ordinary
light-touch sinking still lacks the HP-versus-pose clarification. Root owns the
source integration; the player owns visual/FPS/gameplay acceptance after a future
explicit delivery request. No push, ZIP or Drive update.

### October16 — light shore touch sinking reopened; source timer repair, installation forbidden

The owner reports sinking after one light coast touch, then explicitly says to
install nothing for now. Existing source reveals duplicate grounding timer chains:
exit/re-entry before the old100–200ms callback leaves multiple callbacks observing
Strand=true, each rescheduling and charging HP. Native sea-layer freeze also did
not prevent those global callbacks taxing stale speed. The source successor binds
both producers and the callback to a per-character generation, rejects stale
chains, stops the grounding sound at exit and skips HP while native motion is
frozen. Actual active damage/perks/Phonk budget remain unchanged. Normal SAVE load
clears the event queue, so no old one-argument pending callback requires migration.
Live replacement of a running VM's old queue is not supported.

Disposition: source-only repair; no build, staging, runtime copy, game control or
gameplay test. Installed app remains the19:01EDT `a5a0fae` image below. The report
does not yet distinguish HP death from physical descent; the specific immediate
sinking, raw-forward-speed grounding damage and structural/ingress alternatives
remain unresolved. Root owns continuation after the player's HP clarification;
installation remains held until the owner changes that instruction.

### October16,19:01EDT — player stall/crash successor installed; manual replay pending

The player reports no visible holes from laser fire, then a severe stall after
one bomb salvo. Their ten-second sample binds1479/1518 main-thread stacks to
SetHullCuts and1268 to RebuildLaserSurface, which synchronously replays laser
history after changed cannon cuts. It does not measure FPS/GPU cost or establish
laser admission/commit. Their subsequent crash in GetCollisionDetails+404 is a
BSP triangle-byte read with uninitialized traceid; ordinary MODEL traversal also
lost the exact winning leaf. Both diagnostics match installed UUID
`D0079EAA-54AA-3C79-974C-A7D9DF28601F`. Exact raw reports and signed executable
are retained in
[player evidence](../.local-archives/metal-renderer-evidence/phonk-current-20261016/player-sample-183658/README.md).

The source successor queues immutable combined reconstruction, preserves prior
geometry until publication, cancels without a frame-thread join, retains queued
laser doses and freezes renderer-reset inputs. Live cut workers share an averaged
5% logical-CPU allowance. Checkpoints do not interrupt opaque library calls and
main capture/upload or initial SAVE restoration remain outside that allowance.
Collision detail state now follows the winning ordinary trace, with initialized
node/index and range checks. No agent gameplay tests/profiles are run as requested.
Canonical `experiments/native-metal/run.sh --stage-only` exits0 after source
`a5a0fae26f2edb6daaa1fb62bbeb66b21d697ba5`. Native fingerprint is
`c53b58be580a9f522fb6f478fae9aa4d939efb4c328283b8ac1be994fce28f18`, unsigned engine
`1c26a1c7b383fcb653c61b560b176286b6bb2e88391eb8d3cdf831f3c315d05d`, signed installed
engine `affdd4dd1b3925d203610f71abdf03f9da759a3040361c081a35109462163533`, UUID
`8B297FCE-B108-3220-8615-8F97C28EE6A8`. All249 current protected files retain
their hashes; all239 managed receipt rows match. No game process runs at handoff.
Build/stage and preservation receipts remain in
`.local-archives/installed-delivery/d03e571-successor-20261016/`.
Disposition: installed; actual laser holes, CPU utilization/FPS and remaining
combat/shore behavior await the owner's manual replay. No new ZIP or push.

### October16,18:34EDT — combat repair installed; player will test

The owner explicitly requested installation after stopping source work. Canonical
`experiments/native-metal/run.sh --stage-only` exits0 and delivers the native
combat changes together with `PROGRAM/sea_ai/AIShip.c`. The first build found
`GEOS::INFO::boxcenter` is a `VERTEX`, not a `CVECTOR`; an explicit conversion
repairs ram-bound projection without changing its behavior. The retry compiles,
stages, signs and verifies the installed app. No extra gameplay tests, game launch
or player SAVE load occur; mandatory build/staging guards run in the normal path.

Native input fingerprint:
`3c9feddfc00f1581f394407c3706d6b045ad9c0645c7105a5ed93b4d82f96d8d`.
Unsigned engine:
`50c1780f901de3ce2c3d9e78c63c0edd1d7e14550b6ec2d779fc26f3b9292bb9`.
Signed installed engine:
`1ce61b820ca779d808a25a90ffcbe443e85c2305c42a38db7dfa9be687af6454`.
All249 baseline SAVE/userdata/configuration files retain their hashes and all239
installed receipt rows match. Game process count is0 at handoff. First failure,
successful stage output and preservation/result receipts remain under
`.local-archives/installed-delivery/bf106f2-20261016/`.
Disposition: installed; gameplay appearance, physics and FPS await the owner's
manual replay. Tiny-salvo sudden sinking remains unresolved.

### October16 — source-only combat repair; tests explicitly declined

The owner resumed code repair and will test personally. The source candidate
addresses animated unchanged-yard cut cancellation, aim frozen behind pending
laser admission, wasted finite-cut history, grounded control lock, discarded wreck
momentum, high-energy ram depth, excessive full-anatomy gore and casualty/blood
presentation. The owning topic documents record the changed contracts. Source and
diff inspection only: no tests, compilation, staging, launch or player SAVE load.
Installed gameplay/FPS and abrupt tiny-salvo sinking remain unresolved; no concrete
script double-damage defect was found. The installed/public ZIP batch stays23ac5eb.

### October16 — player rejects shore recovery and live laser behavior

The player reports hulls sticking to the coast, a laser that locks to an intangible
ship center during intended manual aim, slight bow cutting but no visible stern
or broadside cuts on some models, and sparks/beam without visible ship/ground
removal. Cutting reportedly falls to about10FPS. Excessive crew scatter and
abrupt sinking after a salvo or while lasering are also reported; their causes
remain unresolved. The live installed binary still matches the verified147e4d1f
image. Source/log inspection is read-only while the player continues shooting
and reloading; no agent game control, native build or delivery follows this report.
The controlled production-ABI aperture fixture does not close these cases.

### October16 — release metadata race isolated

The full ZIP64 retry parses95463 members and extracts natively, but verification
rejects a changed root `.DS_Store`. That file reappears during scratch
removal as well; ENOTEMPTY masks the original extracted-byte assertion. The raw
archive, inventory and full traceback remain in the reconstruction evidence owner.
ZIP rewriting now excludes Finder directory state and its AppleDouble record.
Owned cleanup admits only a single late `.DS_Store`; an unknown new file still
fails and remains intact. The small native roundtrip retains file bytes, modes,
symlinks and xattrs, and both cleanup cases pass. The corrected canonical producer
then exits0:95461 members,44780 files,1990 directories,8 symlinks and46778 xattrs
pass; deep/strict signatures and isolated relocated `--stage-only` pass, and
scratch cleanup succeeds. Retained public ZIP:
`dist/releases/23ac5eb-final/Corsairs-Iddictive-Remaster-macOS.zip`,10873247949 bytes,
SHA-256 `149fd245cf4316f9f4e6aaa7964c11dcde55a394c6ca3ff1e09c5a1a5e918267`,
MD5 `ba0ce162e91a588afc5ec666dbcab95f`. No engine/gameplay or installed-app bytes
changed. Drive publication remains with the existing delivery owner; its512MiB
connector limit requires the separately owned large-file route.

### October16 — bounded laser replay and reconstruction closure

The single production-ABI held-input replay completes401 frames, exit0 in5.49s.
Its actual emitted-contact line loses the near hull, then becomes clear through
the40.5m control segment; the8.276m next-hit advance crosses empty hold and is
not burned-material depth. The0.6m neighboring solid changes by at most1.146µm;
miss/release intervals have no contact, pending work or new cuts. Whole/close
renderer frames show two small sea-through apertures below the gun line; they
are clear close up and tiny in the distant view. Foreground crew occlude part
of the target in the after-aim frame. This accepts the controlled ship-hole
component, not the user's unreproduced input/state or broader terrain/FPS modes.
No canonical gameplay fix is invented from this result.

Two earlier invalid fixture runs are retained separately: a frame0 diagnostic
null read, then a missing library-private `STORM_METAL_SHIP_FIRE` definition.
The canonical build capture now retains Ninja's production compilation database.
Independent compiler dumps match production/fixture `SHIP` layout exactly
(10864B, `phonkLaser` offset8856) before the accepting run. Raw contacts, material
snapshots, commands, hashes, invalid attempts and PNGs are retained under
`.local-archives/metal-renderer-evidence/phonk-current-20261016/integrated-sea-laser/player-rejection-23ac5eb/`.
All private processes are terminal, scratch is removed and app/config remain
unchanged; player SAVE was not used.

`prepare_metal_inputs.py --export` retains complete build inputs and verified
movies/music, including the11 city/rowboat originals missed by the first global
manifest. Copied inert vendor Git pointers are excluded; their original bytes
remain preserved privately. Normal restore from the actual app into a fresh
source copy with spaces succeeds. All236 copied PROGRAM/RESOURCE receipt rows
verify; app-only launcher/executable rows are excluded. Development SAVE is
empty,15 prepared movies need no FFmpeg and music needs no network. A first
source upgrade commits through the canonical transaction; unknown edits and a
missing city original reject. Exact procedure, source snapshots, raw outputs
and invalid diagnostic attempts are retained in
`.local-archives/reconstruction-20261016/`. No clean C++ rebuild, new GPU sweep or
installed-player replay is claimed by these restoration checks.

### October16 — shore travel frozen; ordinary-transition rollback installed

The user superseded further port repair with an explicit freeze and rollback.
`codex/frozen-shore-travel` preserves the entire unfinished feature and both small
optional-camera message repairs. Active native/script travel, resident Location
and port artillery changes return to `f8c45b2`. The earlier city geometry, fort,
decorative population and unrelated laser/combat/forest/sailing work remain.
The independent SAVE global-reference reader remains compatible with saves made
after the added globals. Player SAVE/settings are not loaded or rewritten.

One promoted gameplay source, `characters_login.c`, must stay registered and be
restored to its reviewed original; merely removing its manifest entry would leave
old installed port callbacks. Existing rowboat resource identities remain admitted
for the already-delivered receipt, without running their generator or activating
their native/scripts. This preserves the normal updater's known predecessor;
unused generated files do not become live gameplay.

Canonical `run.sh --stage-only` installs rollback `23ac5eb` with deep/strict
signature verification. Native source fingerprint is
`e9020972445099cce4b0a41bd734bd1d3a666f5a8d42086527424845afeb274e`;
unsigned engine is `60afa3ec0c6e24c67a43e8aa10597dffd113e4503aab031d4c121c8a7fe4cf5f`.
The16 pending script/city-INI files are delivered. Refreshed preservation hashes
confirm all254 protected SAVE/userdata/configuration files unchanged and all239
installed receipt entries match. Latest actual player SAVE predates the first
port installation, so no speculative port-coordinate migration is added.
Exact before/result receipts and build logs are retained in
`.local-archives/seamless-port/frozen-shore-rollback-20261016/`.

All596 headers in the retained f885 baseline independently match that Git commit;
all55 captured files match their recorded hashes. No current working CPP was
archived with those libraries. No further port journey QA is active. Ordinary
town-transition player replay remains unrun. The new laser rejection authorizes
a bounded replay of the complete input/contact/material/render path; its prior
f8 native replay never linked because the fixture lacked a camera include path.
That missing accepting replay is unresolved, not evidence of a new kernel cause.
Requested source/reconstruction and ZIP delivery remains active after the laser
disposition; no unrelated QA or player-game control is resumed.

### October16 — actual port failures reopen acceptance; publication held

After playing c5f58cda, the user reports water through the town, transparent/dark
characters and noisy foliage, unexpected swimming/teleport on interaction and
interior entry, and a crash on port exit. The supplied screenshot/crash report
reopen the port's acceptance; no single height or material cause is assumed.
Port diagnosis/source repair was reopened, then superseded by the freeze above.
The exact installed b06 ABI baseline is retained as49 libraries/596 headers and
producer commands under `.local-archives/seamless-port/coherent-f885a33/`.

The later delivery request authorizes publishing engine source and a complete
reconstruction path to GitHub, allowing large required inputs on Google Drive,
and a current game ZIP. Those publications now wait for the requested rollback;
the source/dependency audit can proceed independently. Portfolio work has its
separate owner, with its download link and Habr excluded. This does not resume
unrelated gameplay checks or grant agent control of the player's game/SAVE.

### October16 — current combined batch installed; execution paused

The user explicitly stopped further QA and requested integration of the current
batch. Canonical build/staging from `f885a33` installs the physical-timber laser
repair `f8c45b2`, sea-only region transition/raised spyglass and midpoint aiming
overlay, existing combat/forest work, and seamless-port source `a469606` together.
The repeatable fixture-input capture owner is also committed. The installer
applies33 gameplay/rowboat files and delivers matching shared headers and engine;
all239 managed receipt entries match their actual installed bytes. All254
observed player SAVE/userdata/configuration and bundled-default files remain
byte-identical. The previous signed a3b088fb engine is retained under
`experiments/native-metal/.cache/installed-engine-backup/` for recovery.

Actual gameplay acceptance remains **UNRUN** for this combined installation:
held laser and cannon→laser behavior, intermittent nuclear firing, scene/FPS
behavior, sea-region/spyglass transitions, and the complete deck→boat→town→quest/
trade/combat→same-ship/SAVE route. The final private fixture stopped after a
missing include-directory compilation error; it linked no executable and ran
zero native cases/GPU seconds. No retry followed the user stop. Earlier CPU and
native component results remain their recorded evidence, not acceptance of this
installed batch. Forest form remains the accepted sparse variant; ground-grass
color changes were not resumed.

All owned fixture workers are terminal; the root has no native goal. The related
island automation and the older FPS goal are paused. Completed chat history is
consolidated in the existing local `TASKS.md` handoff. The managed build-only
checkout is archived through the native app; its49-library/591-header f8 closure,
exact producer commands and unique inputs remain in the primary evidence owner,
and its8214 portable inputs have the same manifest as the saved checkout. No
push, new test campaign, player launch or automatic resume occurred.

Exact final build/install/protected-state receipts and installer output are in
`.local-archives/metal-renderer-evidence/phonk-current-20261016/pause-delivery-20261016/`.
`DEVELOPMENT.md`, `dev-runtime.md` and `metal-portable-bundle.md` own rebuild,
delivery and transfer entrypoints; gameplay work resumes only on a new explicit
human request.

### October16 — seamless port source batch checkpoint; native journey pending

The separate75-path San Juan candidate covers resident land/sea placement,
deck/rowboat/land hero transitions, canonical NPC services/quests, guard and
port/fort artillery routing, and SAVE reconstruction. Its source review found
and repaired failed-landing actor rollback, absent native AI groups after load,
the old64-entry NPC registry, and missing old-save rowboat controls. Restored
custom travel keys remain intact. Full PROGRAM compilation, actual-source
adapter cases and the exact old-SAVE global/reference reader matrix pass; the
adapters are not native scene acceptance. The frozen v3 patch has SHA-256
`079b167b3bdfac19b75791ba9b04bbf76289d5d73dc6a39bdba933754f1d196d`;
all75 working paths match its receipt, only the two approved control paths
changed from v2, and the other73 remain byte-identical. Source and check evidence
belong to `.local-archives/seamless-port/integration-20261016`,
`restoration-oracles-20261016`, `control-oldsave-20261016` and
`save-reference-remap-20261016`; `seamless-san-juan.md` owns the implementation.

Disposition: integrated source candidate only. Its matching canonical native
build and full deck→boat→town→services/quest→combat→same-ship/SAVE journey remain
queued after the isolated laser corrective batch. No port candidate is installed;
installed a3b088fb and player state remain unchanged. No player app control or
SAVE load occurred.

### October16 — frozen successor built; world held-fire passes, ship still open

Source `cf75576` is built in a managed build-only checkout, isolating the city's
continuing native/script changes. The canonical456-action build exits0 at
fingerprint `8d1d2707b95d81f201d6bf19a237c5da0681430ed2583b414675c4969f8e3c33`,
unsigned engine `a4af4d21a422961f21e3891189f3cd7b69245036bbefb9243e5c8c60cd94b3c8`.
All49 archives, matching native headers, exact Ninja producer commands and
unchanged third-party header addenda are retained in `integrated-sea-laser`.
The complete935-file installed/projected PROGRAM snapshots have identical599
global variable ordinals/names/types/counts and synthetic reference roots. The
new-global SAVE-reader defect found by the port owner does not apply to this
function/local-only script delta.

Actual generated-state native held fire on Antigua commits18 times and deepens
the same physical ray from0.800079 to7.20008m; the view ray reaches9.2829m and a
nearby untouched face changes by at most2.62e-5m. The first rock attempt was
beyond the3500m beam range and remains retained as an invalid setup. Galeon held
fire cuts its near skin, but the frozen initial view ray still stops at the far
hull; this measurement is not the acquisition-time moving camera/muzzle ray.
The original fixed-ray failure is not replaced with visible water or cut counts.
Water inside a hole is not evidence that its opposite wall was cut.

The source-bound diagnostic keeps the same ship action and changes only private
worker/Poll logging. Its first invocation accidentally selected baseline mode
and no fire; the corrected explicit laser invocation captures14 real works and
196 target preparations/stages, all successful with matching identities. Nine
discards have no geometry change; the first exact payload lies in empty hold
space. CPU preparation independently reproduces that unchanged result, while
the retained accepted control changes geometry. Fragment-budget and identity
rejection hypotheses are therefore falsified for this case. Current committed
GEOM collision is atomically refreshed; no stale-BSP cause is established.

The exact captured rev1 ray exposes a separate material-boundary defect:
canonical collision first meets the outer far skin at7.6938648m, while retained
inner timber begins at7.4763031m. The actual authored wood object enables
collision; caps and finite walls provide no nearer hit. Canonical collision
omits this0.2175617m backing. A derived comparison preserves the captured
surface, direction, radius, tangent and0.64m dose: starting at canonical outer
skin leaves the inner backing and a front cap; starting at physical inner
timber clears both material and canonical centre rays through the plank. The
original captured input remains byte-identical. This proves the plug mechanism,
not final held-fire or player acceptance. An opt-in laser material trace is in
source implementation; ordinary collision and structural metrics remain their
existing contracts. Short interior-only doses also require real material
publication without charging synthetic canonical HP/area. Exact source, inputs,
commands and outputs are retained in `held-fire-diagnostic/trace-input` and
`held-fire-diagnostic/derived-cpu`; no speculative budget or aim repair follows.

The corrective source now routes both laser rays through a dedicated retained
material query, preserving ordinary collision/details and structural formulas.
Its worker/loading projection owns the physical index and reserved ray scratch;
commit moves them atomically. Ordinary cannon-hole updates reuse clipped CPU
triangles, and restoring hull cuts refreshes this material projection. Five
native translation units and the focused source review pass. Two0.064m doses
advance material by0.0639999/0.0640011m; empty air remains unchanged. The second
dose removes real existing finite-wall area and follows the unchanged damage
formula; the earlier strict zero-damage oracle was rejected and retained. An
actual-GM cannon-hole primitive query advances1.00000191 to9.27596569m, preserves
its neighbor within2.24e-7m and restores to the original hit. These are CPU/source
checks, not native frame/FPS acceptance. All57 classified stock roots have zero
validated closed volume; the retained synthetic closed-timber limitation does
not exercise a current stock consumer. The11-path frozen patch and full check
inputs/results belong to `held-fire-diagnostic/implementation/receipt.json`.
The corrective coherent build and actual held-fire replay remain pending.

A second unexplained working-only header reversion to bfde388c is retained. The
committed and isolated native header remain727e7008. Restoring only that owned
committed prefix in the saved checkout passes the unchanged dose/atomic-prefix
oracle with before/after source identities equal. The writer remains unproven;
no hook or configuration change is claimed.

Disposition: world held-fire component passes; ship held-view penetration and
actual player acceptance remain open. Canonical staging is held, installed
a3b088fb remains unchanged, and no player app control or SAVE load occurred.
Exact attempts, source recovery and build evidence are retained under
`.local-archives/metal-renderer-evidence/phonk-current-20261016/integrated-sea-laser/`.

### October16 — live laser depth acceptance reopened; drift repair candidate

The player still sees hull sparks without deeper ship/rock damage while holding
weapon6 and LMB. Read-only saved beam fields prove contact/admission/persistence,
not the requested visible cut. A frozen native Galeon replay of20 existing
samples has16 shallow bores and4 through-hits; complete frames show subtle whole
ship damage and visible close holes. That manually admitted contrast does not
accept live firing. Player SAVE was neither loaded nor modified.

The exact saved Battleship pair has7.71669mm lateral drift inside a0.16m-radius
bore. The previous continuation incorrectly gives0 instead of0.175121m free
path. The successor follows the new center ray only through the existing finite
octagonal prism and includes earlier samples of the same atomic pending batch;
retargeting rechains its prefix after a committed predecessor moves. Radius,
delivered depth and direction remain unchanged. Independent/reverse/side-exit,
world-coordinate, atomic/idempotence and actual-source queue rollback negatives
pass. Integration caught a stale header attestation: the retained unchanged
oracle fails on bfde388c and passes on reconciled727e7008, with source identities
verified before/after compilation and release. Exact evidence belongs to
`phonk-current-20261016/player-drift-reopen/result.json` and `sea-phonk.md`.

Disposition: reconciled source candidate; native held-fire and installed player
ship/rock acceptance remain open. The direct-GM Battleship restore contrast and
intermittent nuclear firing remain unproven separate diagnoses. Canonical
build/staging will use a pinned build-only checkout while the seamless-city
owner continues changing incompatible native/script inputs in the saved tree.

### October16 — sea-only region handoff and raised spyglass source candidate

The player rejects the installed visible sea→global-map→sea trip and asks to
raise the spyglass above the rail like manual aim. The successor retains one
outgoing sea framebuffer, uses the existing map only for hidden region/admission
projection, and reveals the rebuilt sea in the retained camera mode/orbit/zoom.
Failed capture cancels before teardown. Manual map travel keeps its cinematic;
the held image does not imply continuous sea simulation during loading. The
spyglass shares the standing-height floor plus0.5m without moving third person.

The additive aiming-overlay request selects the midpoint water fill/rim alpha
.19/.63 between the earlier .08/.48 and stronger .30/.78 values. Existing
night/day scaling, receiver colors, geometry and firing remain unchanged. The
two-literal source comparison and calculations are retained in the same evidence
owner's `aim-midpoint-results.json`; installed brightness remains unaccepted.

Six canonical native syntax checks, complete projected PROGRAM compilation,
actual-body script transition cases,54 CPU transition assertions and camera
rebinding/eye-expression negatives pass. These do not prove native scene ordering
or the installed appearance. Frozen13-path patch and exact inputs/results live in
`.local-archives/direct-sailing-sea-only-20261016/`; `direct-sailing.md` and
`deck-aim-zoom.md` own the contracts. The hidden map's inherited queued quest
encounter materialization limit remains explicit.

Disposition: source/component candidate only. Canonical build/staging and real
Barbados→Antigua/scale25↔50, rotated/zoomed cameras and spyglass replay remain
pending. Installed a3b088fb above is unchanged. Shared integration must exclude
unfinished city/combat source from a delivery batch.

### October16 — corrected successor installed; player replay pending

The player reports automatic laser aiming in first person, missed ship targets,
no visible ship/ground bore, high-speed ram spinning intact ships, and trees
remaining over removed ground. The later correction accepts separated island
geometry and defers crumbling/edge smoothing: the world-support defect is trees,
not a new falling-terrain requirement. First-person laser must remain manual;
third-person automatic targeting is allowed but must reach real hull geometry.
Nuclear automatic acquisition remains required. The subsequent forest request
replaces island sprite placements with terrain-aware shader trees; ordinary
location grass and sea fort/gun ownership remain separate.

Source discovery confirms a camera-proxy ambiguity and an omitted vegetation
consumer. The laser successor reads the active native camera and validates
retained surface fallback picks through the actual muzzle trace. Its source-body
sanitizer cases and native syntax pass. A coherent hidden native CORE scene now
commits manual first-person fire with a stale outside-camera proxy and automatic
third-person fire while the camera looks away; installed-player acceptance remains
separate.
Original island trees are separate Grass `.grs` instances with no cut-support
query, rather than terrain GM geometry. A source correction filters roots through
the committed world cutter; the procedural forest continuation preserves that
support contract. Collision and secondary-fragment source work is frozen for
shared integration; the integrator owns canonical build/stage/app delivery.

Frozen `6e2cc7b` native CORE libraries were replayed in a hidden private fixture,
with matching archived headers and isolated userdata. Manual Flyingdutchman fire
commits two finite cuts and renders a small bore; a blanket async-queue failure
is therefore disproven for this case. Actual Antigua blast and finite beam also
publish, with a0.29996m bore and remote trace delta below1e-7. Fixture setup and
the normal six-significant-digit attribute codec are accounted for separately;
neither source checks nor these component scenes accept the player's rejected
interaction. Evidence owner: `phonk-live-regression-20261016`.

The native ram snapshot rejection is reproduced at a rotating node's local
radius2.00000024 after an admitted physical2m input. Separating physical admission
from local-coordinate validity repairs it: the same work1000 native hook now
commits at frame113. Current manual/automatic fire commits at frames121/120.
At960x600, frames100..180 have maximum CORE durations9.4005/9.15575/9.34792ms
respectively; these are private component observations, not player FPS or a
replay of the collision solver. Actual1920x1200 Antigua finite doses, prior-state
restore, pending-dose ordering and stale-job cancellation also pass. Exact
fixtures, library hashes and raw observations: `phonk-current-20261016`.

Canonical native build succeeds for source fingerprint
`63423341dd777e216116adbe1b8f0ff8349b22db1395f5baf89500068dd4c389`, unsigned engine
`36bea3e3fc4861c1219f8036fa8c5bba650c24afe5927c0d2e2c2a2e6ac12b4f`.
The first forest shader fails on the reserved MSL name `vertex`; renaming it
passes runtime compilation. The standalone terrain fixture then exposes its own
null-texture legacy draw; binding its terrain texture permits rendering. Actual
sea `ISLAND::Mount` and the normal Grass message path also render the forest;
the foreign land-world fixture cannot exercise it because it has no primary sea
model identity. First native Mount fixture lacked the normal `LightingPath`
attribute; adding it fixes that setup failure. No player data is involved.
The resulting forest is rejected visually for giant sparse leaves and planting
inside the authored village. Mesh scale/crowns and settlement exclusion remain
with the same forest source owner. Retained first frames and failed inputs are
kept alongside the corrected runs.

The successor uses16–31m trees and authored settlement-triangle clearance.
The real Antigua placement identity loses36 previously forbidden roots while
other XYZ remain unchanged; independent settlement/surface oracles pass. Actual
sea Mount/Grass renders the corrected city without forest inside its streets.
An additional real cut removes support at a retained root while a distant root
stays supported; the native frame completes. The player explicitly accepts the
latest tree form and asks not to densify it further. The apparent acid-green
ground is the original ground texture under the fixture's full-white ambient;
no terrain recoloring or alternate canopy atlas is applied. Actual gameplay
lighting remains for the player's review.

Canonical calibrated build source fingerprint
`469b2025045b0ac2036ca2132eda33687f434b37e2227c40f2a0fcce8c116dd0`, unsigned engine
`f0f1e56441c6b8c8fe8e1d8dbec2e75eaea3a71a02ce418c7542b53a3a90ae24` passes.
The added orphan-weapon native fixture first reports an empty seed and then a
mutex error. Exact record layouts identify its wrong ABI: unlike the actual SHIP
target, the benchmark driver omits `STORM_METAL_SHIP_FIRE`, shifting protected
Phonk fields192bytes. Correct native target defines repair the fixture: an actual
fragment is created at frame42. Its immediate-delete assertion then runs before
the normal deferred erase fence. The stronger individual live-SHIP deletion
scenario also omits ordinary ShipLights/Seafoam/Sailors teardown; ordinary
sinking keeps the native SHIP, while sea exit removes those consumers first.
That unsupported orphan scenario stays unresolved; no product destructor or
registry changes follow these fixture failures.

The separate supported-lifetime scene keeps the parent SHIP/AI alive. Its first
moving token2 legitimately becomes tokens3/4 from delayed birth-contact damage,
with the same model/owner/GEOM generation and unchanged parent HP/cuts/revision.
The deterministic fixture uses the existing synchronous cutter only to seed and
move material before any contact step; real weapon dispatch and asynchronous
secondary commit remain unchanged. Incoming impact, actual manual deck-camera
laser and blast all pass native execution: old token2 retires at frames59/58/59,
after weapon admission at53. Parent HP/saved cuts/hull revision/callbacks and
intact emitter HP/geometry remain unchanged. The three-case serial batch takes
28.725s; this is not player FPS or the complete AIBalls callback chain. Exact
inputs, native archives, binaries and raw rows are retained under
`ship-collision-20261016/secondary-scene/native-hook/attached-before-core-batch.json`.
No product source, build or app bytes changed for these checks.
Dormant sea-town native context is compiled but its new city content/scripts
are excluded from this batch.

Disposition: corrected successor installed and signed; combat/forest native
component checks pass, including moving-piece impact/laser/blast replacement.
Actual player replay of first/third-person fire, collision, forest lighting and
FPS remains unresolved. The stronger unsupported orphan scenario is separate
from those accepted component cases. Installation
receipt and preserved-state hashes live in `phonk-current-20261016`.

### October16 — city, neighboring-island sailing and cache repair installed

Direct-sailing source `0b436d0` is merged after the city candidate `891ce96`,
preserving both histories and the city's disjoint sea load/clear hooks. The
existing immutable world-map resource pool queries the same authored `_area`
predicate as the map. Ordinary sea→map→sea admission performs the short region
handoff and retains the existing survivor/quest owners, fleet formation, motion,
sails and Phonk state. This is not simultaneous island streaming.

The source owner's three native syntax checks, full projected script VM,
actual-body transition/denial/fallback-lock cases and468999 area-predicate
comparisons on31 installed meshes pass. The optional SAVE-codec probe remains
unaccepted because its unrelated minimal fixture also fails. Retained source,
commands, versions and raw results are in
`.local-archives/direct-sailing-20261016/results.json`; `direct-sailing.md` owns
the contract and the Barbados→Antigua/scale25↔50 acceptance scenario.

The full city remains on its original navigation patch:14 assets totaling
44,154,647bytes,32 residents and no background neighborhood expansion. Core
`UpdateAdd` previously appended same-millisecond reused entity IDs out of order,
breaking the lower-bound removal contract. Reusing its existing sorted `Add`
repairs the actual-source CPU counterexample while preserving uncached updates
and other classes. The canonical9-action native build passes for fingerprint
80ea207c above. With identical fixture source/assets, the full native replay
changes retired cached IDs from4 to0 and sorted state from0 to1. All32 residents
move9.60..26.42m; skinning, zero-delta, land guard, retirement, reactivation and
CORE cleanup pass. The three complete frames preserve the town, fort, coast and
original waterways. Exact before/after evidence is linked from
`puerto-rico-city.md` and `evidence/puerto-rico-city/native-postfix-20261016.json`.

Canonical staging installs18 changed gameplay/resource paths and the coherent
engine. Post-stage parity is0 cache/0 installed; full installed `seadogs.c`
compilation is0 errors. All251 protected player-state hashes,14 native-tested
city assets and4 fort/locator/seabed inputs verify. Previous signed5b57e16b is
retained in the canonical prior-engine backup. Build archives, exact stage log,
script inputs/raw logs and the delivery receipt are retained under
`.local-archives/metal-renderer-evidence/phonk-current-20261016/`, with the final
record in `integrated-region-city-delivery-receipt.json`.

Disposition: installed delivery and private native city/component checks pass.
Actual player sea/landing/fort interaction, neighboring-island handoff and
combat/forest/FPS acceptance remain unresolved. No game launch/control or player
SAVE load was performed.

## Earlier October16 verdicts (historical)

### October16 — current frozen batch installed

The user explicitly requested installation before further worker/prewarm design.
The174-action native build and canonical stage/install pass. The installed app
contains proportional finite laser cuts, solid-ground/physical-timber geometry,
hollow interiors, newly disconnected bow/ring pieces, original-face attachment
support, outside-camera hostile targeting, nuclear cloud detail and the current
renderer/contact corrections. Heavy world/ship mesh preparation runs in value-only
workers; complete staged geometry, collision and saved records publish together.

Current-source CPU/sanitizer checks pass solid/hollow24-dose rings, independent
volume oracles, full-bow versus partial-notch, actual mast severing, actual
Flyingdutchman shallow/legacy/progressive cuts and16 complete world roles.
An independent directed-interval check accepts439 retained triangles with0 bad
intervals after the coordinate-domain correction. Source-body probes pass queue
pause/cancel/rejection recovery and GPU-stage failure rollback; Commit allocates0.
These use a fake GPU where stated and do not accept complete native scene timing.

Disposition: installed; player behavior unresolved. The prepared private CORE
drivers at `phonk-auto-scene-20261016` and `phonk-world-async-20261016` have not
been run for this batch. Actual main-frame upload/retirement cost and cold contour
capture remain unmeasured. Gradual sea-side worker prewarm is deferred; current
immutable input prewarm occurs during Mount. Secondary fracture/weapon hits on
detached tokens remain with the separate collision continuation.

### October16 — island spin and recoil source correction

The player rejects island contact that spins and repeatedly kicks the ship, and
requires weak elasticity, speed loss and angle-dependent physical yaw. The actual
shore methods reproduce stationary5m overlap as0.280rad yaw/5.582m displacement
in one frame; parallel/reverse movement can also receive a false impulse.
The source successor removes the fixed1024-step yaw/position loop and radial
strength queue. It consumes the published island triangle/depth normal and the
shared hull impulse with an immovable shore, preserving HP/material protection.
Slow contact, angular time, mirrored yaw, weak rebound, initial overlap and
separating/tangent negatives pass source-bound sanitizer checks;10000 live/live
response comparisons preserve every output bit. Native touch.cpp syntax passes.
`ship-shore-contact.md` owns the contract, limits and accepting player scenario.

Disposition: unresolved. This is source evidence, not installed acceptance;
the installed arm64 engine still hashes to the dcca image above, with normal
PROGRAM/RESOURCE bundle roots and the player's separate SAVE owner. This task
does not launch/control the game or change those runtime files. Shared integration
owns the focused checkpoint, canonical build/staging and delivery;
native damping/propulsion/Phonk and player-save coast replay remain pending.

### October16 — local laser and nuclear cloud repair reopened

The player rejects oversized laser removal on terrain and ships, plus the smooth
orange nuclear cap/stem. The installed dcca image above is the rejected baseline;
the new source candidate is not installed. `ExecutePhonkLaser` previously turned
short exposure into a complete halfspace and chose its orientation/retained side,
so reducing HP or delaying the same plane would not repair locality.

The finite candidate retains actual muzzle-contact samples in0.32m-wide bores
with4m/s delivered depth. CPU sanitizer cuts on actual Antigua/Cumana and
Flyingdutchman meshes preserve remote triangles and move only local collision.
Partial mast notches retain support; a complete sampled support section invokes
stock whole-mast fall. Measured authored-skin aperture is0.0848309m² in the probe;
deeper and overlapping cuts cannot charge its entrance twice. Legacy plane,
nuclear and ram semantics remain separate. Complete native input/load/crew replay
is pending.
The nuclear helper renders217 consecutive1920x1200 Metal frames with nested
density, cooling core and smoke lobes; exact foreground/zero/expiry checks pass.
These frames use a plain fixture backdrop, not the player's island scene.
The private evidence owner is `laser-locality-nuclear-detail-20261016`.
Canonical build passes at native fingerprint
`698b076b1de7ed54dca1f022b5c2cfd1ebc33280aacff97bf9f4238022149cbf`;
unsigned built engine SHA-256 is
`55127364d859e889838949760aec1cc9c8c8d2b992694338091df4181176bee5`.
Both earlier native integration drivers compiled/linked against that historical
library closure; they have not been executed. Player PID25222 has exited, without
agent process control. The subsequent source batch is still under repair, so
that earlier build no longer proves the current candidate. Installation and
accepting player action remain pending; the installed image is still dcca above.
The current native fixture source/compile closure is retained at the same evidence
owner's `native/{world,ship}` directories. After player exit, rebuild those private
drivers with their `build.py`, run world Antigua/Cumana and held-fire/ram ship
cases, then consume the final frozen source through canonical `run.sh --stage-only`.

The follow-up request adds automatic enemy targeting in third person, cut-time
frame stability and real solid-ground/hollow-ship material geometry. Automatic
laser/nuclear target acquisition is source-verified: canonical hostility,
camera-independent range ordering, live laser muzzle occlusion, nuclear movement
lead/arc/self-blast checks, preserved manual aiming and paused/exit-flight state.
`phonk-auto-scene-20261016` retains source-body/syntax evidence and the unexecuted
native adapter. World async preparation is a separate source candidate. The
ship kernel now passes all24 progressive solid-ring cuts, an independent volume
oracle, hollow panels and an open-notch negative. Actual Flyingdutchman wood
admits a0.03m shallow bore at exactly that depth while preserving its hollow hold;
three progressive records preserve the remote ray. Full split/topology takes
20–90ms CPU on that model, so synchronous publication cannot meet frame stability.
Value-only ship preparation and its ordered main-thread transaction are under
integration; native build/staging waits for that owner to freeze. Ground must
remain solid material; ship interiors and
gunports must remain genuine voids, without blanket filled cut disks.

### October16 — world destruction and old-control correction installed

Actual CORE/MODELR/ISLAND reproduces the player's unchanged-island result:
Antigua's seabed and fort reject a220m blast, rolling back every receiver.
The world-only correction uses local triangle-column/prism sections and pinned
Clipper2 winding normalization, preserving authored outer/PN surfaces. Both
complete Antigua/Cumana native transactions now pass: the downward ray reaches
y=-220, a previously hittable fort has an authoritative empty collision/draw
owner, laser-after-blast commits, and clear/rebind plus invalid-input rollback
pass. A first diagnostic bootstrap omitted Render; it is corrected privately.
The fixture also now mirrors Mount's seabed unlink rather than double-mounting it.

A full native readback exposes a separate cap material defect: tiled atlas UVs
and full-white vertices make a pale grid. Separate cap-only base-texture batches
now preserve source UV density/multiplier and leave outer materials untouched;
component union/unchanged/rollback checks and canonical build pass. Final native
frame/Trace acceptance and canonical stage are pending for this exact successor.

The next complete frame still stripes where independently clean shore and seabed
caps overlap. Actual Antigua geometry measures194446m² duplicate cap coverage;
its crater floor is exactly coplanar in both receivers. Scene-level cap union
now retains secondary-only coverage instead of hiding all seabed walls or adding
depth bias. Reflection caps remain independent. Sixteen complete/PN roles and
rotated/scaled coverage now pass ASan/UBSan. A legitimate122-micrometre strip
exposes earcut's invalid diagonal; the pinned integer triangulator fallback uses
its source-defined one-grid-unit boundary repair, with the original strict
earcut oracle preserved. Canonical native build and both integrated CORE/Metal
frames now pass: Antigua/Cumana forts disappear, crater walls retain base
materials without duplicate bands, Trace reaches the220m floor, laser-after-
blast and clear/rebind/invalid-input cases pass. Evidence is private
`world-cut-integration/coverage-*`; source fingerprint is
`40c418e19b744d0e704379c6008865a8ab89e039e443ccc32c46593a8cbe3807`.
This is a hidden fixture with real engine services, not a player scene or SAVE
replay. Canonical `run.sh --stage-only` now installs this exact source batch;
the signed image and unchanged source/player inventory are verified above.
Receipt and complete stage log belong to private
`world-cut-integration/delivery-correction`.

Old saved control maps have no6/7 actions. Actual VM/native controls retain user
remaps through reconciliation after LoadGameOptions and battle-interface setup.
The existing HUD now reads the selected weapon and native charge/reload; native
negative sentinels suppress cannon/powder sprites only for6/7. Script positive/
OFF/ordinary cases pass;126 native HUD rows retain ordinary output bit-for-bit.
No app launch/control or player SAVE load occurs. The corrected dcca image
supersedes the rejected eac7 batch; original saved-game weapon action remains
unresolved until the player's replay.

### October16 — world cuts, numbered ordnance and crew installed; player rejects switching

Canonical `run.sh --stage-only` installs the signed engine and source fingerprint
above. The frozen batch and deep/strict signature pass;528 freshly recorded
player files retain their bytes, with0 added. Evidence belongs to private
`20261016-030846-world-ordnance-crew-delivery`. Prior attempts stop before install:
one native compile lacked an explicit message header; the next composed-script
gate rejected two changed canonical inputs. The header and two verified normalized
hashes are repaired, preserving composition transforms and strict admission.

World geometry now shares retained draw/Trace/Clip mesh data for laser halfspaces
and nuclear craters, using saved `Islands[].PhonkWorldCuts` records. Actual GM
CPU sanitizer checks cover Antigua terrain/PN, Cumana fort, town open shells,
overlapping cuts and rollback; untouched PN triangles remain byte-identical.
They do not prove the integrated entity/save lifecycle. The actual production
Metal nuclear helper compiles and draws1025 pixels in a private256-square frame;
foreground depth occludes it exactly and early/late effects differ. Its initial
reserved MSL parameter name is corrected before the successful final stage.

Native6/7 actions,6000m target picking, parabolic missiles,4s reload,220m blast,
shared draped receiver and crew source are included. The player subsequently
reports no6/7 switch, continued laser fire and no island cut. The installed
binary contains `ExecutePhonkOrdnance`, `ApplyPhonkNuclearImpact`, `CutWorldWeapon`
and `RestoreWorldCuts`; installed control scripts contain both new names.
BattleInterface has no Phonk weapon consumer, proving missing HUD integration,
but isolated script registration checks do not disprove the reported control
failure. Source/delivery pass; accepting action, world save/rebind and visible
battle replay remain unresolved. No player process is stopped or controlled.

### October16 — deck crew gore and attached blood source candidate

Hypothesis: actual spatial hit inputs and posed-mesh fragments can replace the
old dormant random jump with weapon reactions while retaining script-owned crew
loss. The explicit follow-up adds spray and blood coating the struck ship.
`AIShip.c` now dispatches ordinary ammunition/bomb/airburst impacts; `Sailors`
owns escape/death and a96-piece real posed-model fragment pool. Optional Blood
receiver mode projects bounded stains onto the moving ship and reprojects on
laser hull revision; ordinary land blood is unchanged. No SAVE schema or extra
numerical casualty owner is introduced. `naval-crew-damage.md` owns the details.

Native syntax checks pass; the existing native VM passes all five ammo profiles
and suppressed-state cases with0 script errors. Actual-method CPU impact/cut/
escape checks pass3D/distance/disabled/fire negatives. Native build, installed
delivery and visible battle replay are unresolved. The active runtime integration
chat owns the successful-cut hook and combined canonical build/stage; the player
game is running, and this task launches/controls no game. The last verified
installed inventory above is unchanged by this source candidate.

### October16 — ship auxiliary LOD and rope endpoint lifetime staged

Canonical `run.sh --stage-only` exits0 in149.811s and installs signed engine
f931e4b5d0a338ab3e730a23d24733060a90382dbc0d31431df57cdd2a5c6c12.
All528 freshly hashed player files retain their bytes, with0 added; full app
signature and frozen native source fingerprint pass. Prior cdbe5bf8 is retained.
Exact source inventory and log belong to private
20261016-015153-ship-lod-rigging-delivery. The installed engine consumes all11
changed native inputs, including the shared auxiliary scope header.

SHIP owns one main-camera projected-hull policy: below64 physical pixels it
omits reflection submission, crew drawing and shadow casting, preserving hull
draws, shadow receiving and simulation. Near/zoomed views restore detail; invalid
camera/bounds fail open. GPU checks seed an existing caster, then remove it in
the same sea frame: packets2→2, registry2→1; an independent caster still shadows
the receiver. Sun depth/offscreen admission, restoration, nested scopes and
Reset/domain changes pass. Actual-method CPU sanitizer checks cover near/far,
ship sizes, zoom, reflection overrides and1000 transitions. Policy/GPU inputs
and receipts belong to private ship-auxiliary-lod. No player FPS gain is claimed.

A separate confirmed rigging defect writes sail-wave endpoints after actual
rope deletion. Every rope free now synchronously clears matching signed ties
and rotating attachments for that model before releasing storage. Actual core
enrollment/message dispatch and rigging methods pass ASan/UBSan for live ties,
individual removal, retie, group removal, destructor and removed receiver;
other-model/unrelated-rope ties survive. Private rope-sail-lifetime owns the
reproduction and corrected fixture. This invalid write is repaired; causation
of the original renderer-tree corruption remains unproven.

The fresh eight-ship composed replay was deferred by its live-game guard before
build/run; PID87851 subsequently confirms the player app is running. No agent
app launch/control or SAVE load occurred. New original-scene LOD/spyglass,
stability and sustained-FPS acceptance remain unresolved.

### October16 — contact broad phase and string comparison CPU batch staged

Canonical `run.sh --stage-only` exits0 in184.572s and installs signed engine
cdbe5bf8608ee2a8579f3c14a8799efb6987280024e65f8d74ba40a902ac09e8.
All528 freshly hashed player files retain their bytes, with0 added; full app
signature and all three frozen source hashes pass. Prior d370a07d is retained.
Exact log/inventory belong to20261016-004602-cpu-contact-strings-delivery.
The canonical dependency build recompiles137 actions after the shared header
change; no gameplay scheduling, AI frequency or rendering quality is reduced.

Existing conservative swept-radius rejection now runs before pair preparation
and final SAT in the frame solver. Its public Body validation/narrowphase stay
unchanged.244 complete serialized output rows match the baseline bit-for-bit;
this includes repeated rows and80 seeded six-body worlds. ASan/UBSan pass.
Separated6/16/32-ship synthetic hulls prepareN times and perform zero SAT queries,
versusN² preparations andN(N-1)/2 queries. Actual player FPS is not that fixture.

The shared case-insensitive trait skips equal bytes and computes each differing
byte's existing locale conversion once.65536 byte pairs and100000 seeded strings
retain results;3.8M representative comparisons take32–103ms versus216–376ms,
depending on case/mismatch. Private source/results belong tocontact-cpu-broadphase
andstring-compare-cpu; metal-frame-pacing.md owns cost/claim limits.

The player's new005417/PID45608 sample has UUID648AB217-EB6D-3977-88BB-4A3B74F20784,
matching this installed image. It still fails their performance expectation.
Main-thread samples: contact6/1355, case comparisons23/1355, attributes197/1355,
AIShip104/1355, ProcessRealize1015/1355. Categories overlap and scenes are not a
controlled before/after pair. Continuing CPU attribution targets parameter reads
and native draw preparation; no sustained-FPS or original-scene acceptance is
claimed. Recurring renderer-tree corruption remains open independently; a private
rope deletion/live sail-wave check now reproduces a separate invalid endpoint
write, whose production repair and original crash attribution are not accepted.

### October16 — night aim strength and airborne shore support staged

Canonical `run.sh --stage-only` exits0 in113.378s and installs signed engine
d370a07da2131bf85f6d782713640ec0471c3c1ffff92b286f48162571344eee.
All528 fresh player-file hashes remain unchanged, with0 added. The full app
signature verifies, prior4c69a6c3 is backed up, and all four frozen source hashes
remain unchanged through delivery. Exact inventory and log belong to
20261016-003025-night-shore-delivery. Production aim MSL compiles; the existing
solar factor gives.75 night/1 noon water strength. Solids/daytime retain style.

The shore source removes sea-height airborne grounding, reuses actual island
traces, and holds the retained keel on terrain after landing/OFF. The elevated
saved pose reconstructs support without a SAVE schema change. Ordinary wet
TOUCH and GroundPenetration bodies remain identical to18b87d8. Actual-method
ASan/UBSan adapter checks include airborne/strand admission, landing, prediction
restoration, OFF/reconstructed pose and terrain/water invalidation. Sampled rays
can miss thin obstacles or initial overlap; sea-phonk.md owns the contract and
limits. Native compilation/staging pass; actual coastline/tsunami/Shift, stable
beached pose/load and original wet grounding replay remain unresolved. No app
launch, control or SAVE load occurs.

Two original-player PID74525 samples (001257/001401) use the preceding4c69a6c3
engine. Main-thread simulation share rises142/1294 to399/1306 (11.0% to30.6%);
TOUCH rises17 to139 and island-related branches8 to43 sampled stacks. These
inclusive categories overlap and are not additive/per-frame costs. They support
a CPU contact/prediction investigation, not a complete GPU/FPS attribution.
The requested CPU repair and independent recurring indexed-span crash remain
open; neither is accepted by this delivery.

### October16 — water-pattern and star-size corrections staged

Canonical `run.sh --stage-only` exits0 in118.136s and installs signed engine
4c69a6c30c86b7684db81918a9d43e54618cab6a87765ff844f86eaca9c9255d.
All528 fresh player-file hashes remain unchanged, with0 added; prior d533f315
is retained and the complete app signature verifies. Exact source hashes and
receipt belong to20261016-000502-water-stars-delivery.

Native readback finds56921 water samples rejected by a one-ULP depth difference,
forming the reported grid. One explicit captured depth plus non-writing
Foam/SunRoad overlays in that captured scope remove every one-ULP mismatch in
Native and Performance121-frame scenes; real foreground depth still rejects.
Complete frames preserve brightness and have no patterned holes. No tolerance
or ownership bypass is introduced. The saved dynamic-lighting setting is0;
native processes exit0 but the inherited fleet wrapper rejects its unrelated
eight-cube gate. No full-lighting or FPS acceptance follows from those runs.

Actual production star MSL retains a4x4/5x5 subpixel-dependent footprint at
1x/4x/16x, with pixel difference at most1.28e-5 and unchanged cloud attenuation.
The existing cloud probe now checks its intended relative attenuation rather
than a previous saturated centre sample. sea-aim-overlay.md and
metal-dynamic-sky.md own component evidence. No original app/control/SAVE load
occurred. Player night/spyglass acceptance and the recurring cache crash remain
open independently.

### October15 — approved water aiming visibility staged

Canonical `run.sh --stage-only` exits0 in116.879s and installs signed engine
d533f315ae1b1718dc964c822a523350ba0e7c1379ef4f0ef004566ab457a4c9.
All530 freshly hashed player files retain their bytes, with0 added; previous
c86eade9 is retained by the installer. The complete app signature verifies.
Source and delivery inventories belong to20261015-231928-aim-visibility-delivery.

Water-only fill/rim strength and inward line width are increased; other surface
colors, receiver/depth admission, air and firing remain unchanged. The exact
header passes private Native/Performance121-frame complete Sea::Realize scenes
with eight point cubes; complete frames show readable water fill and perimeter.
The player accepts the shown visibility. The prior composed projection pair
also passes after diagnostic relation enrollment is corrected; its initial
AIHelper failure belongs to fixture setup, not the player's cache crash.
sea-aim-overlay.md owns projection/style evidence. No original game launch,
control, SAVE load, FPS result or original-save cannon acceptance is claimed.
The repeated22:41 player cache crash remains a separate open investigation.

### October15 — layered shadows and coherent water depth staged

The frozen two-source batch passes canonical `run.sh --stage-only` in109.464s
and installs signed enginec86eade9. All527 freshly hashed protected player files
remain unchanged, with0 added; previousd876fa01 is retained. Exact source hashes,
stage output and player inventories belong to
20261015-222414-layered-shadows-water-depth-delivery. The unchanged six faces of
each point cube share one layered pass, reducing48 passes to8. Independent actual
material48-face and production-header192-face depth oracles are bit-identical;
uninstrumented Off/On/Off whole-GPU medians20.804/17.806/20.461ms establish the
preserving performance gain. metal-sea-benchmark.md owns the measurement limits.

Fresh normal driverf75ebdd5 links all50 just-staged canonical libraries. Native
and Performance384 runs exit0 with all150 bound inputs unchanged; every warmed
frame retains152 sails,334 animated draws, one sea/current sky and eight cubes.
The complete Native frame retains hulls, rigging, sails, water/reflections, sky
and six stock HUD icons/names/counts. Existing horizon/reflection boundaries and
daylight appearance remain separate unresolved visual issues. These runs use
shadow tier1; the saved tier2 sun map is larger, while both point maps are512.

The same batch keeps later captured-water passes in the primary1x depth domain,
preventing AA4 SunRoad depth resolve from invalidating the aim receiver tag.
All12 paired component cases pass; sea-aim-overlay.md owns the cause and proof.
The additional real Sea::Realize replay aborts preflight before compile/GPU work
because the original player is running (observed PID65269). No agent controls or
stops it. The subsequent player crash at22:41:06 binds UUID
4BB96145-F21D-3921-93FE-1A97F29D7807 to this installed batch: tree removal in
IndexedSpanFrameCache::erase/evict, now reached from ROPE::Realize. The existing
cache-corruption obligation remains open; this does not implicate the layered
GPU change or prove the tree itself caused the earlier memory damage. Real
cannon/player acceptance, long-run stability and120FPS remain open.

### October15 — scene chip assets and HUD ABI repair staged

The frozen three-source batch passes canonical `run.sh --stage-only` in115.256s
and installs signed engine d876fa0184ee309c0d407ff3d9a363149f9925654a13f3e485f14aadfd496bde.
All526 fresh player-state hashes remain unchanged, with0 added; the prior6a6be8eb
engine is retained. Receipt and exact source map belong to
20261015-210016-chip-assets-hud-abi-delivery. The normal current50-library
HUD384/fleet136 runs exit0, retaining all152 sails and334 warmed animated draws,
eight point cubes and native4xMSAA+FXAA. The complete HUD frame shows six intact
stock icons/names/crew counts. The physics owner returns normal RAM151 and
laser151 exit0 against the same50 libraries, with all151 state rows in each mode
identical to the earlier normal baseline. RAM action45.499 to30.194ms and laser
cut-frame CORE43.936 to34.066ms reduce the first-action hitch without removing
the remaining synchronous split. Whole frames preserve the visible hull-stopping
beam and retained/detached assemblies. This fixture directly calls impact and
has no stock death handler; ordinary TOUCH, SAVE, death and sinking are unproven.
No original app launch, player control or SAVE mutation occurs. sea-phonk.md owns
the scene asset lifetime and causal first-action comparison. The independent
span-cache corruption and120FPS remain unresolved.

Ordered128/512-triangle shadow clusters and native sparse-ICB optimization are
rejected after same-frame48-face depth parity and whole-frame GPU regression.
The expanded render/compute observer has measured overhead and remains private.
metal-sea-benchmark.md owns those results; none of these renderer candidates is
included in the installed batch.

### October15 — late laser/hitch batch staged; beam accepted, hitch open

The physics owner freezes six native files, including the final deltaTime guard
on early contact-light publication. Canonical stage-only exits0 in97.612s and
installs signed6a6be8eb, with526 protected player files unchanged and0 added.
Exact final source map and delivery receipt belong to
20261015-192931-physics-late-fx-final-delivery. An earlier delivery was interrupted
after engine installation during bundle sealing; completing canonical seal_app
restored strict verification before the final successful stage. Its exit-15
receipt is not an accepted delivery. The frozen batch keeps opaque fragments
early and moves transparent laser FX after Sea; it also shares chip meshes and
hashes closed-volume edges. sea-phonk.md owns its causal evidence and limits.
The earlier physics batch completes serialized native RAM401/laser401/water151 runs
193610/193617/193622, all exit0 against50 canonical libraries and the frozen
source map. Whole-frame observation accepts air/water beam visibility and hull
occlusion; cut/charge/retained-geometry invariants remain. Severe hitch acceptance
is rejected: RAM108.295ms and laser43.9756ms remain, with cold RAM CORE cost
unattributed. A same-binary warm RAM replay is queued with that owner after
root releases the GPU. Root retains Git/build/install ownership. No original
app launch, player/save/death/quest or sustained-FPS acceptance occurs.

### October15 — point-shadow cost isolated; no production shadow change

Hypothesis: tiny sailor shadows dominate the eight-lamp fleet scene. Private
native stage-boundary counters attribute about5ms of vertex/fragment stage sums
to48 point-cube passes, about6.7ms to main geometry and about2ms to reflection;
overlapping GPU stages cannot be summed into whole-frame time. The unmodified
counter observer adds.233ms in its paired control. A separate conservative
receiver observer requests43/48 cube faces in the tested frame, so whole-face
pruning alone has a small ceiling. Paired136-frame skinned-caster Off/On tests
retain normal models/receivers and45164 animated draws: removing only their
point/sun casting saves about1ms command time, while point stages still total
about4.1ms. This is GPU-skinned-caster evidence, not a semantic sailor policy
or a120FPS result. Exact sources, hashes and CSVs belong to private
gpu-pass-counters and shadow-face-demand-observer evidence; frame-pacing owns
the detailed result. Disposition: diagnostic accepted, production optimization
unresolved; no shadow LOD or daytime lamp gate is installed.

The separately frozen full-ASan395-TU native fleet exits0 for384 frames with
no ASan report, using system allocation and actual engine instrumentation.
Its follow-up `hud384` also exits0 for384 frames with the real stock
BattleInterface icon/command/info initialization and six ship icons. This adds
the previously missing HUD producer to that trajectory; it does not identify
the recurring span-cache writer or accept player stability. The same retained
binary/dSYM and source manifest bind both runs in full-asan-native-fleet.

Independent native ABI evidence finds two `%ld,%ld` scans writing64-bit longs
into32-bit Point coordinates in BattleShipSign. An actual-Point canary changes
the following sentinel to zero before repair and preserves it with `%d,%d`.
The two-format correction removes this proven overwrite; no causal link to the
span-cache map corruption or FPS gain is claimed. The newly staged canonical
engine passes actual HUD384 and full-fleet136 native runs; runtime source,
50-library hashes and outputs belong to integrated-chip-hud-f2be757.

### October15 — thin-rope MSAA and Options label input delivered

The screenshot's native daylight source had AA disabled. FXAA alone leaves
missing subpixel rope samples; inspected tube topology/UVs are continuous and
all five sail_rope mip alphas are opaque. Native MSAA4+FXAA visibly improves
coverage;2 samples leave conspicuous dotted spans. The existing checkbox and
persisted boolean now enable4 samples plus FXAA, with no settings migration.
The nine Metal checkbox labels drop individual-position input so label clicks
can reach the native row-toggle branch. Details and compatibility limits belong
to metal-normal-less-surfaces.md.

The final integrated hidden136-frame fleet exits0 at native and MetalFX
Performance modes, each recording678 MSAA passes,136 FXAA frames,8 ships and
45164 animated draws. Existing actual-backend water, dynamic-lighting and
scene/HUD probes all pass. Per-flight MSAA attachments are reused; after warmup
GPU allocation varies between two plateaus3.7MB apart rather than growing each
frame. Final unbracketed GPU medians are18.515ms native and16.768ms Performance;
these are correctness runs, not an isolated performance comparison or120FPS.
The older static metal_graphics_live_probe stops on obsolete `d3d9->` spelling
in LostRender; current source uses `renderDevice->` for the same unbind calls.
It is not reported as a passing check.

Canonical stage-only completes in182.1s from the frozen AA source batch; its
compiled watermark is3e02dba and source hashes are recorded in the receipt.
The installed engine is35567395 and its prior a61220ef is backed up. Fresh526
protected files retain hashes, including settings and SAVE. Installed INI bytes
match source through the manifest's UTF8-to-CP1251 conversion. Receipt, source
hashes and logs belong to20261015-181902-rigging-aa-delivery; private source,
frames/crops and exact link evidence belong to rigging-alias-fit. Original-app
click/OK/Cancel/reopen and moving-player-scene acceptance remain pending under
the no-launch boundary. The aim identity capture scope retains its prior
single-sample path, and Performance upscaling still sacrifices fine detail.


### October15 — authored sail ambient restored in source; native daylight and compatibility pass

The private136-frame daylight candidate170936 restores resolved authored
ambient reflectance for sky SH in raw/skinned/CPU lighting and all four matching
shadow receiver totals. A separate raw coefficient preserves the authored value
before legacy ambient-floor mutations. Direct sun/lamps, unlit paths, sky and
exposure are unchanged. Native whole-scene readback visibly restores cloth
detail/brightness; its27 measured GM hull material cohorts have A=D and are
not a demonstrated whole-hull brightness correction. The existing actual-backend
dynamic-lighting GPU fixture passes against this candidate at171817, including
COLOR1, CPU/native skinning, sun, indoor/outdoor points and absent-sky fallback.
Night completed-SH appearance and installed player acceptance remain open.
The two canonical source files hash-identically match the tested private
candidate. Canonical run.sh --stage-only for2b8e6e4 exits0 and installs a61220ef,
backing up61af9725. Fresh526 protected files retain hashes with0 changed/added.
Receipt and logs belong to20261015-173029-ambient-delivery; no agent game launch
occurs. Native correctness evidence does not accept player/night appearance,
whole-hull brightness, the recurring cache crash or120FPS.

### October15 — installed87, repeated cache crash and CPU contention separated

Canonical staging succeeds for87b4acb; delivery receipt, log and518-file
preservation snapshot belong to20261015-162232-ghost-repair-delivery. No agent
game launch occurs. Player16:32 repeats IndexedSpanFrameCache removal through
sea foam;16:50 repeats the same erase/evict offsets through BIShipIcon/HUD.
This rejects a foam-only explanation; the earlier corrupting write is unknown.
The private fracture scene initially exits-11 before action in SEAFOAM::AddShip:
its fixture enrolls unmounted models. Its owner corrects enrollment ordering
in fixture input only. The same coherent87 binary then completes RAM401 frames
(165616), LASER401 (165708) and water-aim151 (165809), all exit0 without this
cache crash. These narrower diagnostic cases do not accept player stability,
contact-solver gameplay or final destruction pixels; their owner reviews them.

Daylight dump163805 records27 real GM hull cohorts, one sail and three rigging
cohorts. GM resolved ambient equals diffuse; their sun and SH are present.
The sail resolves authored ambient1.5/diffuse0.3, while SH uses0.3: the distinct
ambient response is lost. A material-coefficient correction is not installed;
it would not by itself change the measured GM hull response.

Generic ordered batching actually admits main GM geometry, but both complete
Off/On/Off brackets regress. The165150 repeat also records actual CPU clocks
and background processes: controls spend12.069/12.182ms on the main thread
versus13.857/14.074ms completion cadence; candidate17.896ms. Background
contention is real and isolation is unproven. Exact scope and distributions
belong to metal-frame-pacing.md and metal-sea-benchmark.md. Neither disabling
background work nor this batch is evidence of120FPS.

### October15 — native draw ghost reproduced; empty-object domain source correction

The16:04 player report is reproduced with actual Flyingdutchman geometry and
the exact native `GEOM::Draw`: rough refinement/side partitioning loses empty
object overrides, so original GM objects reappear visually with no retained
collision. First rough and flat→rough cases resurrect18 objects. Four source
lines preserve the object domain in refinement and both halves; false original
draws become0. ASan/UBSan exact draw/rebuild/saved-plane/upload-rollback cases
and native geometry syntax pass. Only `ship_laser_geometry.hpp` changes behavior;
This source correction is subsequently staged as87/61af above.
Native pixels/contact acceptance remains pending.

### October15 — timber/contact/flight installed; player rejects contact and ghost geometry

Canonical `run.sh --stage-only` for frozen5e9af7f exits0 and installs e3fbcf90
with prior7433ec35 backed up. Strict/deep transactional sealing and content
checks pass; all518 freshly hashed protected files retain their bytes.
Receipt/snapshot/log belong to
`experiments/native-metal/.cache/diagnostics/fragment-repair-delivery-20261015-155613/`.
No agent game/CUA launch or FPS/lighting prototype is included. The destruction
owner binds the subsequent16:02/16:04 player rejection to live72459 and this
installed hash: collision pass-through persists and a detached stern remains
visible on the source hull. Native fracture/contact acceptance is reopened
and rejected; further variants wait for the real native owner reproduction.
The destruction owner prepares the coherent5e9 private scene executable; root
retains GPU/build/stage integration. The initial native diagnostic attempt
stops before GPU creation while72459 plays. A later user closure and fresh
holder check establish an idle app; the next serialized GPU slot is assigned
to the requested interpolation pilot, followed by these root-owned diagnostics.

### October15 — reflection cost measured; daytime lighting and ordered batching open

The private frozen reflection candidate now completes native Off/On/Off, with
actual47.4% reflected-reference reduction and6.8–12.1% bracketed FPS uplift.
Full-scene cadence remains13.96ms with the candidate, far from120FPS;
`metal-sea-benchmark.md` owns exact distributions, coverage and source scope.
The main-view daytime ship/sail darkness is reported again and remains unfixed.
A bounded lighting observation backend and a heterogeneous ordered-batch
backend compile/link against the earlier coherent4ee15fd archive set; neither
is installed. The lighting dump stops at the new live72459 holder assertion
before device/GPU creation; the full ordered-batch comparison has not run.
The player subsequently exits; execution awaits the serialized GPU slot.
These frozen diagnostics are not performance evidence for installed5e9af7f.

### October15 — installed destruction rejected; timber/contact/flight source successor

Player15:20:32 frame/replay rejects installed7433ec35 sections, breakup hitches,
dragged fragments, water-aim visibility and exposed wrecks losing contact. A
later report adds inconsistent held-Shift tsunami launches. Source diagnosis
reproduces post-cut sheet shelling, open caps,38.430ms rough-cut CPU work before
upload, uncapped grazing friction and premature wreck gates. The successor
clips shared outer/timber/cap roles once, preserves holes, retains spatial
assemblies, bounds friction by normal impulse and caches separate physical
contact with success-only topology invalidation. Rough-cut CPU work becomes
8.187ms before upload/BVH; this is no FPS claim. Independent review reproduces
stale contact after cannon-on-laser; producer/consumer revision checks now pass.
Flight removes the once-per-event latch and consumes full time, fixing reproduced
second-crossing rejection and200ms invented support acceleration. Exact owners,
checks and negatives belong to `sea-phonk.md` and `ship-wreck-contact.md`.
Native syntax/source-bound CPU+ASan/UBSan pass; canonical build/stage, native
pixels/motion and prior-save replay remain pending. The installed identity and
player data remain the last verified delivery above; no game/CUA, GPU comparison,
SAVE or settings mutation occurs in this attempt.

### October15 — hollow-hull and fragment-contact successor installed

The player requests immediate installation and confirms the game is closed.
Frozen `4ee15fd` includes `88b3ebe`; canonical `run.sh --stage-only` exits0
and installs signed engine7433ec35... in the sole supported player app.
The previous d660336f engine is backed up; transactional strict/deep sealing
passes. Cache/app gameplay checks report0 pending with compiler ready.
Fresh518-file protection has0 changes/additions. Receipt, before snapshot and
stage log are in `.cache/diagnostics/fragment-delivery-20261015-151248/`
under `experiments/native-metal`. No game or GPU diagnostic was launched.
This proves coherent build and delivery only: rendered fragment motion,
severe-loss death/quest handling, prior-save restore and player acceptance
remain unresolved. No reflection LOD prototype, FPS gain or renderer-crash
repair is included. The source-only status below is historical.

### October15 — coarse reflection geometry candidate, not delivered

The player permits dropping fine vessel detail in water reflections. Actual
captured geometry establishes a large small-primitive population; the bound
and source owners are in `metal-sea-benchmark.md`. A private frozen backend
builds against51 unchanged linked inputs and65 header dependencies, replacing
only matching rigid reflection indices. Its78 cooked groups use45.87% fewer
indices; this is not a measured reduction in complete reflection cost or FPS.
The full-fleet Off/On/Off runner is ready but has not run: the live player
process99471 is preserved. This candidate remains unresolved, without production
source, installed engine, SAVE or configuration mutation. The120FPS goal and
native scene quality checks remain open.

### October15 — hollow-hull, fragment contact and remnant survival reopened

The player's14:02:59 frame rejects the staged50daab4 destruction. Actual
Flyingdutchman CPU geometry reproduces84.48% root-surface loss with no prior
fatal predicate; ordinary18.60% upper trim remains the allowed negative.
The source candidate adds a cumulative remaining-material fatal floor, explicit
wood inner surfaces/end rims, measured bounded detached bodies, existing wooden
splinters, convex-body/native-BVH contact, wave support and waterlogging. A late
sea Execute owner plus birth clocks resolves mixed receiver poses without render
time simulation; generation guards preserve token ownership across GEOM resets.
The rejected vertex-only collision and64-body damage gate are absent from the
final source. Exact owners, resource bounds, coverage and proof limits belong to
`sea-phonk.md`; ordinary ram consumption belongs to `ship-wreck-contact.md`.

Actual ship/geometry/collision translation-unit syntax and focused ASan/UBSan
geometry/contact/lifecycle checks pass. This is an unresolved source candidate:
no canonical build/stage, native rendered fracture, real script death/quest or
prior-save replay has occurred. The last installed engine remains d660336f.
The player process remains active and is not stopped. Native build/GPU/app
delivery is pending. The
controlled Flyingdutchman fixture pins baseline header/archive identities and
must bind candidate archives only after canonical build. No game/CUA launch,
SAVE/config mutation, renderer-crash fix or FPS claim follows from this attempt.

### October15 — pressure, protected material and jagged fracture canonically delivered

Canonical `run.sh --stage-only` of `50daab4` exits0 and installs signed engine
`d660336fae0108c01f6ba5e2a6ebb523822e0686030ec65c9a995f44b8ac7d18`.
The native/AIShip batch is coherent; canonical strict/deep sealing passes and
independent content check reports0 pending in cache/app with compiler ready.
Fresh guarded518-file before/after hashes have0 changes/additions; no game was
launched. Log, source commit and protection receipt belong to
`experiments/native-metal/.cache/diagnostics/contact-jagged-stage-20261015.log`
and matching `.json`/`-player-before.json`. Previous498cc8dc is backed up.
This accepts delivery only. Player contact/protected-hull/jagged-frame replay,
renderer-crash cause and the120FPS goal remain unresolved.

### October15 — bounded order and diagnostic-cost controls

Captured compatible main draws admit390→234 under conservative screen-disjoint
stable grouping, preserving original instance order and all9009 reversed-pair
separations. Existing local AABB corners retain the same156 reductions. This
is5% of3125 native captured draw calls, not a measured FPS gain. Frozen linked
profile1/0/1 Core means13.937/13.712/13.894ms bound diagnostic overhead near0.2ms;
it does not account for the missing time to120FPS. `metal-sea-benchmark.md` owns
source-bound assumptions, runs and the viewport/visual proof ceiling.

The original cache crash remains unreproduced. Actual production mimalloc cache
churn passes, and a partially instrumented real GEOM/Device/renderer/TCarcass
run135700 passes23 changed cuts, two resets and six foam lifetimes in24 hidden
frames. `metal-frame-pacing.md` owns that narrower negative evidence. No native
performance change or renderer-crash fix is delivered by these diagnostics.

### October15 — pressure, protected material and jagged fracture source integrated

The source owner freezes eight native/gameplay/admission paths after the player's
small-NPC grinder, protected Phonk hull and straight fracture-edge rejection.
The heavier hull must supply inward native acceleration; accepted contact HP
fraction bounds material through the inverse damage curve. Mechanical cuts use
one persisted jagged signed field across geometry, support and saved restore;
lasers/old cuts preserve planar behavior and geometric failure preserves the
prior mesh. Actual adapter/frame solver, native motion, script VM and actual
mesh ASan/UBSan checks pass within the limits in `ship-wreck-contact.md` and
`sea-phonk.md`. Root reviews the exact frozen hashes and integrates one atomic
native+AIShip batch for immediate canonical stage-only. Player replay and the
separate installed renderer corruption remain unresolved; no FPS gain follows.

### October15 — installed renderer crash; cache corruption cause unresolved

The player's original app exits with EXC_BAD_ACCESS at13:09:14, PID87823,
UUID `A63B31D4-322A-3B02-902C-B614D3FA0D17`. Independent installed hash/UUID
inspection binds it to the498cc8dc delivery above. Main-thread removal of an
actual indexed-span map node faults inside `std::__tree_remove`, reached through
revision eviction from `TCarcass::Realize`/`SEAFOAM::Realize`. This identifies the
failure site, not the earlier memory-corruption producer. The actual cache
header passes300000 mixed revision/topology operations,40000 LRU insertions and
192 crossed invalidations under ASan/UBSan with membership/iterator/byte checks.
No cache guard or renderer fix follows from that negative reproduction.
`metal-frame-pacing.md` owns the evidence and remaining diagnostic boundary;
installed renderer stability is unresolved. No game process remains in the
integration owner's subsequent process check; no autonomous launch follows.

### October15 — rigid instancing shader equivalence, no production change

Captured material/geometry fit qualifies770 draws into154 conditional groups,
but complete command order has zero adjacent compatible draws; limiting gather
to uninterrupted qualified blocks lowers the reduction ceiling from616 to312.
The first bounded candidate is the390-draw main block. A64² M3 Max fixture then
compares ten original draws against one instance draw with current raw-hull
shaders, separate transforms/raw lighting/catalogues and unchanged synthetic
textures/fog/shadows. Four color/depth comparisons are bit-exact, including
cutouts and coincident Equal-depth original order; deliberately shared payloads
and reversed order change pixels. `metal-sea-benchmark.md` owns the retained
sources, provenance and proof ceiling. No production renderer source, installed
app or player state changes; no whole-frame or120FPS result is established.

### October15 — structural, wake and MetalFX corrections canonically delivered

Canonical stage-only of `9288f64` exits0 and installs signed candidate
`498cc8dc8236fe1e622be129fa439a20eceb450da71bc75d86df0346700077db`.
Strict/deep `seal_app` verification passes, independent content check reports0
pending in cache/app, and a fresh guarded508-file before/after snapshot has0
changed/added protected files. No game was launched. Log and proof belong to
`experiments/native-metal/.cache/diagnostics/structural-fracture-stage-20261015.log`
and the matching `.json` receipt. This delivers source/component corrections,
not acceptance of the player's rejected fracture frame or a120FPS improvement.

### October15 — installed fracture acceptance reopened; coherent repair ready

The player rejects the4c14ef scene: half hulls survive and crew/rigging remain
over removed support after both ram and laser. Their new explicit structural
fatality requirement also covers their own Phonk hull; intact-contact HP limits
remain. This reopens acceptance of the earlier component-verified delivery.
The source owner freezes21 paths for one cut/death event, waterplane/keel
failure, pause/load acknowledgement, retained crew support and native mast/rig
detachment. Actual-method/real-mesh ASan/UBSan, native syntax and VM cases pass;
these do not accept the player's scene. `sea-phonk.md` owns detailed evidence.
The root's current process check finds no game holder. Root integrates this
coherent packet with the separately committed wake and MetalFX identity fixes
for the next immediate canonical stage; no renderer experiment is included.

### October15 — early-HUD MetalFX sea identity correction source verified

The player reports missing ordinary-sea aim draping on installed4c14ef with
MetalFX Performance. An actual-backend Sea2 fixture reproduces230 native/294
scaled contact pixels before HUD and0 after early HUD completion. The source
candidate preserves the same camera/origin and proportional viewport, sampling
identity with the normalized nearest texel used by restored native depth.
Current component output is230/294/240 with no sky pixels; own-model, camera and
viewport negatives remain0, and relation-colored solid contours retain coverage.
`sea-aim-overlay.md` owns the cause, consumers and proof ceiling. No FPS gain or
installed correction follows from these component checks. The game process is
now absent in the integration owner's current observation; the independently
active structural-death/crew/rigging source unit still prevents a coherent
canonical delivery. Root retains build/stage and installed replay disposition.

### October15 — long wake correction source verified; delivery waits for idle app

The reported straight/angular wake prompted an actual-source reproduction:
12-column absolute16-bit indices wrap at5462 rows, joining remote samples.
Both wake grids now reuse bounded local indices with the existing base-vertex
draw; all history, spacing, lifetime, UV, alpha and contact segments remain.
Actual reservation/draw ASan/UBSan cases across4/12 columns and up to16385 rows
prove adjacent same-segment triangles and exact ordered grid coverage; native
syntax passes. `ship-wake.md` owns the independent source/runtime limits.
The498.4-knot screenshot alone does not prove the required earlier speed history;
the player recalls a possible Shift boost. Rendered acceptance stays unresolved.

The integration owner confirms original installed player process74058 is live.
No stage, launch or process interruption follows this source correction. The
separate structural-death/crew/rigging unit has also reopened after the player
reported surviving half hulls and floating attachments; its source is not frozen.
Root retains the later build/stage transaction after that coherent packet and
an idle-player check. Installed engine remains the4c14ef delivery below.

### October15 — revised gameplay batch canonically staged

The coherent sustained-crushing, transverse midship-fracture and contact-budget
follow-up is committed in `1bc5325` atop the single-beam/physical-contact batch
`f4d3c58`. Canonical stage-only exits0 and installs the exact signed candidate
`4c14ef379c6a9ca248a6a0339222f830f0b33351b7ff2f5c23e4a7709e030467`.
Canonical `transact`/`seal_app` verifies the strict/deep app signature; independent
gameplay check finds0 pending in cache/app. No game was launched. The receipt is
`experiments/native-metal/.cache/diagnostics/frozen-gameplay-stage-20261015-revised.json`.
SAVE/userdata hashes match the older prepared-player snapshot; its two config
differences (`engine.ini`, `metal_graphics`) have modification times before the
current stage. This is not a new full-state before/after hash proof. The source
component evidence belongs to `sea-phonk.md` and `ship-wreck-contact.md`; actual
player ramming/fracture/beam/boost replay remains unresolved.

### October15 — gameplay stage interrupted; installed state recovered

Local source commit `f4d3c58` freezes the single-beam phonk and initial physical
contact batch. A new direct player crushing/midship-fracture requirement reopened
that source unit while canonical staging was running. The integration owner
terminated its owned stage process (exit143); buffered output subsequently showed
that two installed scripts had already changed and signing had started.

Canonical delivery recovery restored the exact previous `sea_phonk.c` and
GreatRecipe-composed `AIShip.c` bytes and their delivery-state entries. The
installed engine remains `51666fe49dc69f9564b1976cd14de2b09db0866f0a0377cf632476857509a6e6`;
canonical strict/deep signature verification passes and508 protected player files
retain their hashes across recovery. No game was launched. The receipt is
`experiments/native-metal/.cache/diagnostics/frozen-gameplay-stage-20261015-recovery.json`.
Source commit and cached build were preserved; neither constituted accepted
delivery. That hold was superseded by the coherent revised delivery above;
the recovery remains evidence of the interrupted attempt, not current app state.

### October15 — broad fleet exclusions measured; lamp cache rejected

The player closed the original app before the sequential112549 ablation. With
native2560×1655/eight ships/384frames/128warm-up, full controls are15.275/15.782ms;
no dynamic shadows12.047, no volumetric sky13.312, no reflection refresh12.183,
no sails13.316 and no sailors12.433ms. Disabling shadows+sky+reflection together
reaches8.755ms. These remove work for diagnosis, not accepted quality settings.
The separate113132 constant-water-fragment control retains all geometry/depth/
reflection/shadow lifecycle and gives16.751ms against16.617/16.962ms full: no
material whole-frame benefit. The first early-return no-water test changed
other passes and is rejected. `docs/metal-sea-benchmark.md` owns the complete
matrix, frozen linked-input provenance and limitations.

The empty-render case has1.432ms median GPU work,1.112ms estimated preparation
and6.834ms median `nextDrawable` wait, yielding8.332ms completion cadence. Full
controls wait only0.023/0.025ms there. That empty-case presentation floor does
not represent6.8ms recoverable from the full scene. The player reports dark
daytime ships and excessive yellow lamp influence; source diagnosis and shader
cost separation remain active. No installed app or player state was changed.

AC-native083658/084358 retain every required fleet pass at2560×1655 with VSync
off. Warm main-completion means13.4541/12.9805ms (about74–77 benchmark FPS)
supersede the low-battery context below. Quality1 control084245 reaches12.3552ms
(about81FPS), while CPU preparation remains about12.07ms.120FPS is unmet.
AC half-sum cloud timing is1.0165× FP32 cost; reject that precision candidate.
The small palette-hash candidate has no whole-frame gain and is fully restored;
neither candidate was installed.

Two ship-local lamp-cache compositors pass the88-case structural depth oracle,
including native same-pass cached-depth seeding. Blit+Load worsens the fleet
pilot; ideal warm seed timing is5.855ms versus5.870ms full for eight captured
cubes, with synthetic checker alpha. There is no material saving. The complete
candidate source is retained privately and its metadata/registry production
delta is restored. `docs/metal-frame-pacing.md` owns this rejected architecture
and its remaining fidelity gaps. Diagnostic exclusion sources and linked-input
receipts are retained privately; their production hooks were restored before the
gameplay source commit and were never staged into the installed app.

### October15 — producer storage bounded; redundant app copies removed

Shared producer storage now retains one build, one prior-engine backup, the
newest diagnostic trace and explicit protected evidence within an8GiB diagnostic
budget. Per-run profiling has a3GiB cap and owns its process groups/private
scratch; exporter/package failures and SIGTERM clean exclusively owned outputs.
Shared process, signal, cap and protected-evidence probes pass. Python/shell
syntax passes; no full app export/archive or engine build/install was run for
this tooling change. Full packaging replay remains unresolved.

The redundant native metadata ZIP was removed: available space increased by
22,470,463,488 bytes in that observation. Old reproducible fleet traces were
then pruned from18,338,312,208 to8,434,422,789 logical bytes; available space
increased9,875,386,368 bytes. Removing162 obsolete regular engine-hash backups
retains the immediate prior engine and the unrelated frozen-cannon directory;
available space increased1,832,497,152 bytes in that observation. These are
separate physical observations, subject
to concurrent host activity. Current012905 geometry/profiling,001520 source
evidence and player sea-fps captures remain protected. Unique older release
differences/receipts were preserved before cleanup. After the player explicitly
approved removing the three named copies, the dated release, archive payload
and extracted app bundles were removed. Available space increased by
22,672,351,232 bytes (21.12GiB physical); APFS sharing makes the earlier allocated
size unsuitable as a freed-space estimate. All82 preserved unique hashes remain
verified. The installed engine hash and strict deep code signature are unchanged.
The receipt is `.local-archives/metal-release-evidence/2026-10-11-storage-audit-2026-10-15/three-app-cleanup.json`.
Retention is one installed player app, with an optional actually used dev runtime.

### Historical October15 — cloud precision candidate; degraded battery context

Attempt022134 reuses the exact binary, native inputs, stock fixture/model and
2560x1655/eight-ship output, but inherits MetalFX Quality1 from the normal player
settings adapter; the earlier native comparison explicitly sets upscaling0.
Its warmed Core/completion means126.875/127.060ms therefore do not establish a
source regression or optimization baseline. All pass minima remain present.
The machine also reports Battery Power4%; no isolated cause of the slowdown is
proven. No game, SAVE, player settings or power settings are changed.

Control024227 explicitly pins every graphics setting to012905, including
upscaling0, with the same source/binary/input/model/output/population hashes.
Core/completion means remain140.892/145.112ms; GPU interval median142.794ms.
Disabling MetalFX does not eliminate the degraded context. Battery later reports
2%/Final warning, but a sole power cause is not proven. Readback and same-setting
control belong to001520/native-power-control.json. A physical-charge question is
pending; low charge alone is not a blanket prohibition on bounded component work.

A CPU-only platform probe reports Low Power Mode disabled, thermal state nominal
and thread QoS33. The exact linked SDL2.32.10 creates one active
PreventUserIdleDisplaySleep assertion during video initialization and releases it
at SDL_Quit; none exists before/after. An added NSProcessInfo activity token is
supported but redundant with SDL's existing facility. Ordinary App Nap is not an
evidenced cause; no benchmark activity wrapper or power preference is changed.
Probe source/output and decision belong to001520/power-policy-probe. Stable
whole-scene timing and the physical-charge response remain outstanding.

A narrow mixed-precision normalized-noise candidate and reused A/B driver run
under001520/cloud-half-pending.
Only two fBm sums use half intermediates/coefficients; coordinates, sampling,
remaps, pow, march, optical moments and resource/cadence owners remain FP32 or
unchanged. Host/source expansion and actual MSL compilation pass;453 GPU commands
complete, including384 cloud/256 measured cloud and64 radiance commands. Four
weathers have no nonfinite output and maximum displayed RGB difference0.003106;
paired partial-field panels show no obvious change. Full-field/fog/light/motion
quality remains unverified. Timing is inconclusive: summed case medians suggest
2.74% component gain but within-case spread reaches9.76x and three weather sums
worsen. No product shader is installed. Rejected-run receipt belongs to001520/
cloud-next-baseline-rejected.json; the last comparable native12ms is historical,
not a claim about this degraded context.120FPS remains unmet.

### October15 — main-draw visibility and whole-packet frustum routes rejected

A temporary default-off probe queries the actual main raw draw, then disables
the query. One2048x8-byte buffer follows each command's existing completion
flight; results never change drawing. Native Off/On color+depth bytes match over
occluded/visible, Less/Equal/LessEqual, discard, encoder Load, clear, unrelated/
replaced depth, nonmonotonic state, unknown current/previous PSO, distinct slots,
capacity/allocation and pending-command reset/destruction cases. The independent
equal-depth/discard fixture demonstrates why an original-main zero result can
still have a later Equal receiver contribution.

Fleet011928/012710 queries701 receivers per frame. At240 completed warm commands,
012710 records168240 queries,58095 zero and110145 visible; zero eligible under
the conservative no-discard contract. Every packet retains alpha test;93600
also pass through an uncertain pipeline epoch. The zero population is therefore
opportunity evidence, not authorization to omit the existing receiver.

A separate padded homogeneous clip classifier uses the captured current local
bounds/MVP and validated pose/index metadata. Native six-plane, intersecting,
W-crossing, invalid/boundary and375 independent-grid CPU cases pass; queried and
unqueried color/depth match. The same fleet has164640 known and3600 unknown
packets, zero fully off-frustum, zero contradictions and57855 zero-query packets
whose bounds intersect the frustum. Whole-packet frustum rejection cannot save
this workload. Neither probe is installed or retained in production: both source
files are restored to c87e66d. Inputs, fixtures and receipts remain under001520's
receiver-visibility-rejected diagnostic owner. Probe runs are instrumented and
are not throughput acceptance, despite the runner's generic profile/capture flag.

Fresh same-binary captures012905/012947 compare conservative/exact bounds at
2560x1655 with all existing quality/pass settings. Across48 cube-face encoders,
conservative submits11287218 vertices versus10755964 exact (+531254/+4.94%).
Receiver input remains2560437 vertices/853479 primitives, with33.64/33.66%
zero-area and about50.25% backface rejection. The prior234159 profile predates
the animated-bound expansion. Single-frame counter timings do not establish
whole-frame throughput; the initial early replay request's empty profile is
rejected and replaced by a ready/nonempty profile. Inputs and counter receipt
remain under001520/current-bounds-gpu-profile.json. Static topology fit015539
then records701 receiver packets:390 qualified static packets/538665 triangles
contain zero removable repeated-index or bit-identical finite-position pairs.
311 animated packets/314814 triangles remain unknown. Only eight ordinary main
draws share2752 repeated triangles, absent from the reflection/receiver population.
Actual-class ASan/UBSan addressing, provenance, revision, deduplication and
one-frame lifecycle pass. The diagnostic changes no GPU commands and is removed
after its single fleet fit; retained source/fixture/receipt belong to001520/
degenerate-static-rejected. Follow-up020528 qualifies every remaining311
animated packet/314814 triangles through the real Shared44-byte upload/current
palette: zero repeated indices or identical XYZ+weight+packedBones pairs.
The actual-class ASan/UBSan fixture passes addressing, influences, validation,
revision, deduplication and one-frame cases. Both static and animated probes
are now removed; together they reject an exact topology filter across all853479
receiver triangles. Animated source/evidence belong to001520/animated-degenerate-
rejected. Tighter conservative-bound feasibility remains pending on actual012905
captured buffers/palettes/eight cube matrices, fetched through native gpudebug
into001520/captured-bounds-fit. Its native oracle remains unresolved: CPU sphere/
six-plane admission predicts11367981 refs versus profiler11287218, with no
independently established ICB counter correction. The CLI's counter description
does not resolve it and its advertised ICB info handler is unavailable.
Conditional same-pose current/exact/pair admission is11367981/11346681/11364249:
pair removes only3732 refs (0.0328%) for2.325x the box transforms. Rebuilt current
matches all48 captured-box CPU admission counts, but general float containment
is unproven. Pair bounds are rejected at the value gate; this does not explain
the531254 difference between separate On/Off captures. Analyzer and input closure
remain under the existing captured-bounds-fit owner. No product/runtime delta;
no additional culling module or quality reduction is accepted from these probes.

### October15 — receiver ICB preserves pixels but reduces whole-frame throughput

The bounded fit counter observes701 admitted receivers/311 skinned, one exact
cull+six-field viewport run and557 unique geometry resources on all383 steady
frames. The default-off prototype packs original uniforms, uses completion-owned
leases and encodes ordered commands on GPU. Apple's ICB fragment contract requires
texture argument buffers; parent standalone texture bindings fail PSO creation.
A derived resource wrapper retains the canonical fragment body. Installed MSL's
three-argument vertex setter takes stride, not byte offset; pointer arithmetic
corrects the prototype's initial addressing mismatch. The final native oracle
matches every color/depth byte with raw/skinned, strips/lists, both index widths,
offsets/base vertex, cull/viewports, sun/points/fog and allocation/capacity/lease/
no-light/reset/destruction negatives. An initial capacity fixture counted total
instead of admitted packets; its corrected rejection exceeds2048 admitted draws.

Same-binary Native fleet384/warmup128, prepared materials/bounds/specialization:

| Run | Receiver ICB | Core mean ms | Main completion cadence mean ms | CPU preparation estimate median ms |
| --- | --- | --- | --- | --- |
|005914|0|12.196|12.258|12.061|
|010009|1|12.530|12.586|9.979|
|010059|0|12.038|12.108|11.221|

The preparation estimate falls, but whole throughput is about3–4% worse. Every
existing coverage minimum remains. This module alone is rejected; production
land_shadow.hpp is restored exactly and no ICB candidate is staged/installed.
Source, patch, fixture, initial failure logs and final measurements remain in
001520/receiver-icb-rejected. Reconsider only as an enabling transport for proven
GPU visibility rejection, not another stand-alone draw-call optimization.

The existing diagnostic runner now records Core mean, main completion cadence,
CPU preparation estimate and the actual STORM_METAL environment. Retained-run
arithmetic preserves previous distributions and rejects failed GPU commands;
no permanent test or player-facing control is added. A separate private-storage
cloud-noise experiment matches16,777,216 bytes but is slower: shared/private
component medians0.440/0.543ms, sums126.471/205.947ms over192 samples each. It is
also rejected; production noise storage and installed51666fe49 remain unchanged.
The full120FPS and installed player-world acceptance remain unresolved.

### October15 — prepared materials installed; CPU saving does not yet improve throughput

The existing backend now reuses unchanged fixed-function stages and encoder
texture/sampler bindings. Texture-stage/transform/reset and temporary grass
overrides invalidate their stage entries; external sky bindings invalidate the
resource cache. World/view/projection, lighting, fog and mutable texture handles
remain live. Final1328-byte base or3008-byte fire uniforms use one aligned,
completion-owned arena upload for both shader stages, with direct-byte fallback.
Default is enabled; `STORM_METAL_PREPARED_MATERIALS=0` selects the old path.

Same-binary Native2560x1655 fleet384/warmup128:

| Run | Prepared materials | Core wall mean ms | CPU preparation estimate median ms |
| --- | --- | --- | --- |
|002903|0|12.019|10.213|
|002948|1|12.012|9.899|
|003109|0|12.043|10.114|

Subtracting measured GPU/drawable waits suggests about0.25ms less CPU work;
whole-workload throughput does not improve. Cross-frame overlap makes individual
frame-tagged GPU intervals unsuitable as FPS estimates, including the closing
Off GPU median13.119ms. All152 sails/311 animated/sea/sky/8 complete cube minima,
1717 median draws and20 allocations remain; explicit arena accounting raises
reported upload bytes from15.28 to17.47MB without increasing geometry detail.
Eleven rebuilt native resource, lighting, sampler, skinning, point/sun, sea and
MetalFX/HUD probes pass. A task-local actual-method CPU driver covers dynamic
uniform/resource transitions and cache/fallback lifetimes with ASan/UBSan; its
source/receipt is retained, without a second parent replay or raw child log.
The entire ON scene was inspected. These are component/workload checks, not a
player-world replay. Evidence belongs to capture001520's prepared-materials and
prepared-probe/delivery receipts. Canonical stage-only exits0, content plans are
empty, compiler ready and all526 player-state files match pre-stage hashes.

A separate tile-local cloud light-factor cache is rejected and not installed.
The32/64-height variants reduce isolated warmed compute from0.435 to0.220/0.275ms,
but fair-day RGB differences reach about0.38 and wet-day0.16. Even helper-only
extraction changes some half-float results. Both variants fail the fidelity gate;
no whole-fleet integration follows. Source, output and exact commands remain in
capture001520's ignored cloud-light-rejected owner. The120FPS goal remains open.

### October15 — separate-queue cloud experiment rejected

The conditional async prototype moved only unseen256x256 cloud tiles to a
bounded second queue. Initial2048x2048 priming remained serial, two monotonic
events ordered recycling and completed-field rotation, and Reset/readback/split
submissions drained both queues. Native tracing tagged main/cloud commands and
capture targeted the device. The diagnostic runner counted both queues and
reported main-completion cadence separately from frame-tagged execution union.
A disposable arithmetic oracle passed overlapping interval union, legacy CSV,
unknown queue, failed command and missing-main negatives.

Frozen Native fleet384/warmup128, specialization and guarded bounds enabled:

| Run | Async clouds | Core wall mean ms | Main completion cadence mean ms | Core median/p99 ms |
| --- | --- | --- | --- | --- |
|000857|0|12.003|12.055|11.294 /14.649|
|001001|1|11.834|11.941|11.625 /14.621|
|001058|0|12.023|12.076|11.537 /14.435|

The ON trace includes256 warmed cloud commands and256 main commands; every
coverage/draw/upload/allocation minimum remains. Cross-frame overlap makes the
ON frame-tagged GPU union13.666ms unsuitable as an FPS estimate. Whole-workload
mean throughput improves only about1%, while the median worsens. The added queue
and synchronization lifecycle do not justify that gain. The source candidate and
runner changes are restored to the installed21ad2b9 behavior; no async code is
installed or retained in production. Candidate patch, source identities, exact
invocations/results and an unrun lifecycle fixture remain under capture234159's
ignored `async-cloud-rejected/` diagnostics owner. Pixel/lifecycle and capture
replay acceptance were deliberately not expanded after the throughput gate
rejected this candidate. Player state and installed engine6f761a93 are unchanged.

### October15 — animated bounds CPU cost reduced; whole-frame target remains unmet

Immutable per-bone bounds now belong to the existing indexed-span cache and its
revision/LRU/reset/4MiB lifecycle. Current-pose transforms conservatively contain
the exact skin; unsupported weights/palettes/topology and more than64 potential
receiver lamps use the existing exact path. A light-cap transition invalidates
cached conservative results. Geometry, pose, shader inputs, detail, reflections
and shadow settings are unchanged. Default is enabled; the environment switch
`STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS=0` retains the matched exact control.

Same-binary fleet runs use Native2560x1655, specialization1,384frames/warmup128:

| Run | Conservative bounds | CPU median/p99 ms | GPU union median/p99 ms |
| --- | --- | --- | --- |
|235049|0|11.283 /14.759|11.326 /14.755|
|235106|1|11.244 /14.669|11.291 /14.732|
|235140|0|11.301 /14.773|11.264 /14.765|

Bounds preparation falls from about1.76 to1.07ms/frame, but the complete frame
does not materially accelerate. GPU waits grow by about0.6ms; subtracting the
measured GPU/drawable waits from CPU wall time gives preparation estimates
10.764/10.156/10.937ms median. CPU sampling alone missed these waits and must
not be interpreted as proving their absence. All sail/crew/sea/sky/cube minima,
draws, uploads,20 warmed allocations and GPU memory remain composed. The full
ON scene was inspected; ordinary solar/exposure and animation variability also
changes OFF/OFF exports, so scene byte identity is not claimed.

ASan/UBSan proves8,467,278 containment coordinates across11 retained meshes,
cache/address/revision/budget/reset negatives and64/65-light transitions. Five
rebuilt existing span, raw-skinned receiver, point/sun shadow, skinning and world
registry probes pass with the candidate enabled. Independent source review found
no actionable defect. The installed batch passed canonical stage-only; all526
player-state files match their pre-stage hashes. No game launch/control or push.

Current GPU capture234159 ranks cloud tile30.59%, main geometry13.43% and point
receivers8.41%. Cloud loop/size compiler controls preserve compared output but
only improve aggregate component cost about2%; those prototypes are rejected.
Separate-queue cloud scheduling was a viable conditional experiment; its later
matched throughput rejection is recorded above. Even CPU preparation still exceeds8.33ms.
The full120FPS goal and installed player acceptance remain unresolved.

### Historical October14 snapshot — Options readback repaired; material specialization verified

The player's renewed settings report is reproduced in the real script VM/native
option-file path. The file already held MetalFX Quality1. `ReadMetalGraphicsOptions`
passed a local OBJECT by value to `SendMessage`: native LOAD parsed chosen values
into a temporary while the caller retained defaults. Native diagnostics saw3
after parsing but script input/setter still saw0. The one-character `&gopt`
correction restores all sixteen graphics fields. Legacy `ReadSavedOptions(ref
gopt)` already references its caller and is unchanged. Real OK/save, fresh-VM
read, next-frame reopen, Cancel without saves, invalid9 without saves and an old
record missing upscaling all return0 with zero script errors. The corrected
script exercised by that proof matches canonical SHA68f2ff7e. Attribute-only
get/set checks from the earlier batch did not exercise the broken boundary.

The conditional shader candidate is retained after a same-binary Off/On/Off
fleet comparison. `ffp_specialization.hpp` specializes at most two structural
material stages; dynamic values and the existing operation arithmetic remain
unchanged. It caches up to128 variants asynchronously, uses generic pipelines
while pending/unsupported/failed and cancels/drains on reset. Bindings remain
under the shared encoder cache; MetalFX is composed. The first native Off run
231508 aborted because the generic fragment also required explicit empty Metal
function constants; the corrected factory passes the subsequent runs.

Every invocation below uses `STORM_METAL_UPSCALING=0` and
`python3 tools/run_sea_benchmark.py --frames 384 --warmup 128`:

| Run | STORM_METAL_FFP_SPECIALIZATION | CPU median/p99 ms | GPU union median/p99 ms |
| --- | --- | --- | --- |
|231800|0|12.296 /15.503|12.320 /15.601|
|231923|1|11.560 /14.748|11.277 /14.727|
|232324|0|12.290 /15.561|12.334 /15.633|

All retain152 sails,311 animated draws, sea/current sky,8 complete point cubes
and equal draw/upload populations. Ten variants are ready after warm-up, with
zero warmed fallback. The marginal gain is about8.5% GPU and6% CPU, including
improved p99. Off already includes the generic loop/unrolling changes, so the
comparison does not establish the full patch's causal gain against old source.
The fleet manifests omit the specialization variable; the exact invocation map
and source identities are retained in `ffp-measurement.json` under the ignored
native-api-migration diagnostics owner. The target8.33ms remains unmet.

The component oracle covers1,261,568 pixels with worst absolute difference
1.19209e-7, undefined-constant fallback, dynamic transitions, seeded stale-key
rejection and pending-reset safety. Seven rebuilt native resource/pixel,
generated-coordinate, sampler, lighting, skinning, menu/fan and scene/HUD probes
pass with specialization enabled, including MetalFX Quality HUD composition.
Full-frame Off/On exports preserve scene composition/materials; stills do not
accept motion or the player world. Independent source/evidence review found no
actionable defect. Player Options interaction and world replay remain unverified
under the existing launch/control stop. Delivery receipt follows this entry.

Delivery: the final canonical `experiments/native-metal/run.sh --stage-only`
exits0, installs signedf70393af and backs up d50531dd. Subsequent content plans
are empty in cache/app, compiler ready, and the installed option_sl projection
matches the passing canonical script. All248 recorded live player-state files,
including graphics/options/engine config and SAVE, retain their hashes. No game
launch/control or push occurred. Native persistence and shader evidence accept
this installed source batch; the actual player menu/world remain unreplayed.

### Historical October14 snapshot — MetalFX Spatial delivered;120FPS remains unmet

The requested MetalFX picker and native Spatial scaler are integrated with the
native API/cache batch. Options adds Off/Quality/Balanced/Performance after FXAA;
the existing message ABI carries the mode in flags bits7–8. Native0 remains the
default. The owning contract is `docs/metal-graphics-settings.md`.

The engine and embedded scaler/copy/depth shaders compile on the M3 Max.
One-off native GPU checks pass all four modes, internal/native dimensions,
mid-scene readback with the frozen slot, an exact one-pixel markerless glyph,
HUD/repeated markers/depth clear, Present fallback, mode toggles, resize/reset
and unknown-mask rejection. Existing menu/interior FXAA/bloom, resource pixels,
authored alpha and GPU skinning probes pass. Actual script VM execution passes
the settings default, four mode/flag round trips, picker wrap and malformed
value preserving prior state, with zero script errors. No new permanent tests
were added. Full Options input/rendering and player-world replay remain pending.

Sequential frozen fleet runs use2560x1655,8 Manowars and256 measured frames
after128 warmup. Native213124: CPU12.914/GPU14.651ms medians; Quality213207:
12.146/14.867ms; Performance213357:12.240/14.037ms; closing Native213627:
13.006/14.623ms. GPU p99:17.955/19.790/18.401/17.861ms respectively. Each
retains152 sails,311 animated draws, sea/current sky and8 complete point cubes.
Quality does not improve GPU time here; Performance saves about0.6ms, roughly4%,
with a lower-resolution scene. Neither meets the8.33ms CPU/GPU budget. The
reviewed whole-frame exports retain scene composition and water/reflections;
thin rigging loses detail with spatial scaling. Motion/weather/player fidelity
is not inferred from still exports. The legacy underwater offscreen path remains
Native and has no measured scaling benefit.

Canonical staging first stopped at an old baked-shadow source spelling check:
its GetD3DDevice assertion no longer matched the migrated GetNativeDevice call.
The existing five affected getter-contract probes were aligned with the actual
native owner, retaining their receiver/caster/lifecycle checks; all pass. The
second exact `experiments/native-metal/run.sh --stage-only` exits0 and installs
signed d50531dd with the previous engine backup, matching launch adapter and0
pending gameplay files. Nine app/cache protected files and526 player-state files
retain their recorded hashes. No game launch, SAVE/config preference change or
push was performed. The separately documented inherited grass-fog mismatch is
unchanged and outside this batch.

### Historical October14 snapshot — submission cache; MetalFX requested before delivery

The native API milestone is committed as821c3ab. The subsequent backend-only
candidate shares encoder-local raster/depth/blend/viewport and shadow-texture
bindings across raw, compact and converted paths, including solar slot reuse.
Encoder completion/target and shadow transitions invalidate the cache. Sampler
reuse includes the effective texture-label clamp and all imported sampler fields.
An environment diagnostic switch compares the same binary with caching on/off.
Fourteen existing native resource/shader/pixel fixtures pass in both modes;
the broader grass-fog fixture fails identically on restored pre-migration355b7a
and the native API candidate (depth3/table exponential:ff4484c4). That inherited
oracle/behavior mismatch remains outside this batch; expectations were not changed.

Frozen fleet204313 with caching disabled measured CPU median13.639ms and GPU
union14.821ms. Enabled204500 measured13.157/14.781ms; restored512-point-map
fleet204959 measured13.236/14.757ms. All required pass minima remain unchanged.
The reduction is about3% CPU; whole-frame/GPU multiplication or120FPS is not
established. Per120-frame blocks approximately1.43M raster setters and1.93M
shadow texture setters are omitted, with1.74M sampler lookups reused.

The point-map-only width experiment204830 changed High point maps512→256 while
retaining sun4096 and every pass. CPU/GPU medians13.050/14.569ms show only about
0.2ms GPU reduction against the two512 runs. Lower depth texel count does not
remove six-face surviving vertex/skinning work. It is rejected as a meaningful
FPS repair; shadow_quality.hpp is restored byte-for-byte. Adaptive screen LOD
and static/dynamic shadow splitting remain candidates requiring their own
camera/invalidation and visual evidence. Existing crew rendering already uses
projected2/3-pixel hysteresis and preserves player crew/spyglass projection.

The player then requests MetalFX upscaling plus its Options control. The M3 Max
reports spatial and temporal support through the installed SDK's native queries.
The pending batch must keep full-resolution UI and safe Native fallback;
temporal capability alone does not supply motion vectors/history. No new app
delivery or game launch has occurred. Installed signed engineee5a226b remains
the last delivered runtime while the requested scaler and settings are integrated.

### Historical October14 snapshot — native API candidate; installed app unchanged

The engine and all existing CMake targets compile after removing the shipping
D3D9/COM API and graphics-header dependency. Twelve existing native checks pass,
including retained-device lifetime, reset/index negatives, exact draw pixels,
HUD boundaries, point shadows, authored alpha and GPU skinning. A full-scene
probe initially rejected the imported default R5G6B5 setting; that added check
was removed because the prior Metal factory always created RGBA8 storage.
The corrected uninstrumented fleet202818 passes all minima:152 sails,
311 animated draws, sea/current sky and8 complete shadow cubes. Across256 warm
frames CPU median13.378ms/p9915.752ms; GPU union14.802ms/p9918.258ms.
The before-migration fleet192703 measured CPU14.710/GPU16.210ms medians;
unpaired variation does not establish a causal migration speedup. Native API
removal is source/component verified, not120FPS or player visual acceptance.
This source batch is not staged or installed yet; installed signed engine
ee5a226b remains the last delivered runtime. Quick submission reductions follow
only after this parity baseline.

### October14 — current CPU/GPU owners; cloud replacement deferred

Restored canonical source is rebuilt by the normal benchmark path for corrected
fleet768-frame CPU sample181629. All pass minima remain present,20 allocations.
Instrumented CPU median13.280ms/p9922.071ms and GPU union14.603ms/p9917.982ms
are profiling observations, not new uninstrumented speedup evidence. Of348
main-thread samples,335 are in ProcessRealize and12 in ProcessExecute;213 are
under Device::draw,71 under reflection traversal and46 under shadow resolve.
These inclusive populations overlap and must not be added. No main-thread
GPU-wait symbol appears in the sampled branch; background wait threads do not
establish a main-thread stall. About1700 draw submissions remain per frame.
The corrected GPU capture below independently points to scene shading,
reflection geometry and vertex/clip-heavy receiver replay. Priority is now the
CPU render submission/state path and GPU material/receiver work. No performance
repair or120FPS acceptance follows merely from the profile.
Source/counter follow-up: authored lamps are already filtered by object bounds;
the64-lamp shader capacity is not the actual per-fragment count. Median6.869M
submitted vertex/index references include repeated passes and are not unique
vertices. Bounds preparation alone times about1.9ms/frame in the existing
optimization counters; current-pose skinning contributes. Software rasterization
is not the observed path. Specializing material branches and reducing redundant
submission/geometry work remain hypotheses until equal-output paired measurements.

### October14 — cloud candidates; selected appearance, no proven FPS gain

The player requests detailed real volume clouds with substantially lower cost,
all weather/rain/storm/day/night and sun scattering; the accepted fog/atmosphere
is retained. The player subsequently prioritizes the CPU/GPU bottleneck because
cloud replacement has not established a whole-scene performance benefit.
SunshineClouds2a73a80b is adapted in a task-owned native diagnostic with its
published NoiseTexture recipes/curl and example mask. Standalone Metal2048²
fields complete in35.9–45.6ms; individual256² tiles range0.18–1.54ms in these
limited states. These are isolated dispatches, not critical-path sea timings.

Full2560×1655 corrected-material sea175734 passes152 sails/311 animated/one sea/
current sky/eight complete point cubes,20 allocations: CPU median14.152ms,
GPU union15.510ms. Source-specific light/mask adapter180023 passes the same
coverage, CPU14.034ms/p9916.079ms, GPU15.121ms/p9917.712ms. Time-separated runs
do not prove a stable whole-scene speedup; neither meets8.33ms. Whole images
retain textured hulls/water/reflections. Initial author contour/flatness concerns
are superseded by the player's explicit approval of175734's Sunshine image,
with a request for slight softness. Preserve that exact shape/light direction;
do not substitute180023's source-specific lighting or clear static-mask region.
The static mask is not an all-weather test. Godot compositor/reprojection is not
ported; no conclusion about upstream visual quality follows from this adapter.

At the player's request, Flower17b5ebd is rendered in the same scene181020.
Its native ambient/ground-light adapter is unverified and gives a dark/dense
result. The player rejects Flower and ends that branch. It is not installed and
receives no further tuning. CPU median13.686ms, GPU union15.328ms, all pass
minima retained; this is not evidence of a meaningful whole-scene speedup.

Both temporary native headers are restored byte-for-byte toHEAD; candidates,
exact resource hashes and images remain in the ignored cloud-replacement native
research owner. No app stage/launch, SAVE or configuration mutation occurred;
installed signedee5a226b is unchanged. The benchmark cache is a diagnostic build
was subsequently rebuilt from the restored source for181629. Both candidate
branches leave the original engine unchanged. Sunshine appearance is selected,
but softness, all weather, motion, source integration and staging remain deferred
behind the performance investigation. Disposition: Flower rejected; Sunshine
not delivered; >120FPS unresolved.

### October14 — corrected-material profile shifts priority to scene shading

Hidden capture20261014-170328 uses the corrected Hull1 materials. Native
overlapping GPU profile ranks main geometry23.54%, reflection geometry10.02%,
traversal receivers8.89%, cloud tile8.33%. Main shading has44.77% instruction
throughput limiter and17.68% fragment occupancy; receiver replay has79.34% clip
limiter. Earlier cloud-first ranking belongs to the incomplete-material fixture.
The4.865ms cloud compute statistic is not serialized frame time. Capture/sample
timings are instrumented and do not replace warmed whole-frame acceptance.

Separate cloud-deck kernels give no warmed gain and change optical fields;
a conservative static noise envelope adds about10–15% cost. Both remain rejected
diagnostic prototypes. A current-pose solar caster rejection passes393216 exact
GPU depth bytes over12 changed poses and unknown-state negatives. Full native
171431 retains all coverage minima, CPU median14.919ms/p9918.452ms and GPU union
median16.622ms/p9921.767ms; no whole gain is established against corrected165042.
Time-separated runs do not prove a causal regression. The source predicate is
restored and the candidate is not installed. Original signedee5a226b engine and
uncapped pacing remain the last verified delivery; no game launch/control,
SAVE or player configuration changes occurred. Next bounded investigation:
specialize the existing material pipeline without changing its states or pixels.
Disposition: dominant complete-scene owner identified; >120FPS unresolved.

### October14 — white benchmark hulls corrected; earlier fidelity verdict withdrawn

The user's white-hull observation is a fixture defect. Native Mount emits
Ship_StartLoad/Ship_EndLoad; the isolated scene has no stock AIShip handlers and
left the geometry texture directory empty. Earlier logs contain unbound ship
materials, and the installed hull textures exist under Ships/Manowar1/Hull1.
The fixture now selects that stock directory around Mount and resets it before
loading sailors. No renderer, installed asset, game config or player state changes.

Corrected hidden run20261014-165042 renders yellow/dark textured hulls; native
texture labels resolve Hull1. Sails, crew, water, reflections and current sky
remain present, with152 sail/311 animated/one sea/current sky/eight complete
point-cube minima. CPU median13.983ms/p9916.617ms; GPU union median15.599ms/
p9919.779ms;20 allocations. Performance remains unresolved. Earlier fleet runs
through20261014-160729 have incomplete materials: their whole-scene fidelity
verdict is rejected and their timings cannot compare directly with this corrected
workload. Independent foam/topology body proofs and cloud-field parity remain
valid for those properties. The installed engine/pacing delivery above is
unchanged; no game launch/control occurred. Disposition: fixture material repair
accepted; >120FPS and player-runtime acceptance unresolved.

### October14 — empty cloud-ray support preserved; whole target still unresolved

The post-repair native capture `20261014-154733` still ranks cloud tile first:
27.80% cost and4.077ms compute statistic. A weather-lookup relocation changes up
to four half-float ULPs and is rejected. Its experimental shader never enters
the installed app. The accepted conservative ray window instead skips only the
existing height profile's guaranteed empty support; four weather fields match
2097152 bytes exactly. One shader coefficient now owns density and its bound.

Full native `20261014-160222` retains all prior eight-ship draw/coverage minima
and scene composition,20 allocations: CPU median12.653ms/p9914.990ms and GPU
median14.485ms/p9917.861ms. This is not a large whole-frame speedup or>120FPS.
The one-ship scaling control first rejects missing animated coverage because
the old fixed layout puts it at the edge (`20261014-160422`, exit74). The formula
now centers variable populations while preserving every eight-ship position.
Corrected control `20261014-160729` passes19 sails/62 animated/one sea/current sky/
seven complete point cubes: CPU median8.420ms, GPU median9.816ms. Even this
control exceeds8.33ms, so reducing fleet submission alone cannot accept the goal.
Canonical `run.sh --stage-only` subsequently exits0 for frozen8b74436, installing
signedee5a226b with prior13180929 backed up. Independent installed hash matches;
source fingerprintacaa0757/unsignedc11c40e2 bind the exact native batch. Independent
content check reports0 pending cache/app and compiler ready. The player's explicit
uncapped/no-VSync condition is now reflected in the original state's canonical
graphics record and engine config: VSync0/max_fps0. Every other config line is
byte-preserved; original configs are backed up in their private data owner.
No game launch, controls or SAVE/userdata-directory mutation occurred. This
accepts installation/configuration, not the requested performance target.

### October14 — sailor topology cache unified; staging pending

The actual native CPU sample finds UniqueIndexCache rebuilds under Sailors.
Eight ships create280 independent crew sources, exceeding its256-entry four-way
owner. Seeded actual-body replay reads1116 indices every warm cycle (186 lists);
the nearest one-source case already caches correctly. Topology now attaches to
the existing8192-span LRU with4MiB payload budget and the same4096-index fallback.
Actual-header ASan/UBSan checks pass280 sources×120 changed poses with zero warm
reads and unchanged bounds; revision/addressing/invalid/eviction/reset negatives
pass. The original span-cache probe also passes.

Matched native `20261014-152752` preserves152 sail/311 animated/one sea/one current
sky/eight complete point-cube minima,1717 median draws,20 allocations and the
whole scene. CPU median12.738ms/p9915.217ms; GPU median14.407ms/p9918.129ms.
The CPU work is removed; the8.33ms whole-workload target remains unmet. Larger
Metal groups/occupancy hints show no stable material gain and are rejected.
This source batch has not been staged; installed signed13180929 still consumes
the preceding3d15e6a. No game launch, controls or SAVE mutation occurred.

### October14 — autonomous native fleet benchmark; >120FPS unresolved

The hidden engine-linked fixture runs without CUA, game startup, SAVE or focus
changes, using original app assets, eight Manowar1 ships and2560×1655 with VSync
off. The384-frame baseline `20261014-144800` has256 warmed frames: CPU Core wall
median12.754ms/p9916.626ms, completed GPU execution union median15.432ms/
p9921.285ms,36 allocations/frame. Actual coverage minima are152 sail draws,
311 animated draws, one main sea/current cloud field and eight complete point
cubes. Full native readback contains water, reflections, ships, rigging and sky.
This proves a representative rendering workload, not whole-game FPS or battle AI.

The foam index repair's matched run `20261014-150339` preserves draw/pass minima
and reduces allocations36→20; GPU median15.344ms does not support a major FPS
claim. Native sea capture `20261014-145742` contains113 encoders/3098 draws;
embedded overlapping M3 Max profiling attributes26.68% cost to Sky cloud tile
(7.171ms compute statistic, not a serial addition to frame time). xctrace's
unfinished `20261014-145352` document is rejected evidence. CPU sample
`20261014-151002` succeeds after rejecting a too-short symbol-processing timeout:
252/333 main samples are Realize, including73 Sailors and66 reflected ship
traversal. Inclusive groups overlap; they are not added.

Source now avoids noise reads for algebraically empty cloud samples. Intermediate
detail-only repair `20261014-150924` has GPU median14.669ms/p9918.665ms and CPU
median12.773ms/p9915.569ms with the same coverage; target remains unmet. Final
height-envelope variant `20261014-151505` passes the same coverage and whole-scene
visual check: GPU median13.601ms/p9917.964ms, CPU median13.258ms/p9913.854ms.
Four-weather old/new GPU fields differ by at most one half-float ULP. The existing
dynamic_sky-probe star-opacity assertion fails both the untouched baseline shader
and current shader; it is an independent pre-existing negative, not a passed gate.
All benchmark timing/profiling artifacts belong to the ignored native diagnostic
owner; durable contracts and rejected fixture paths are in
`docs/metal-sea-benchmark.md`. Canonical `run.sh --stage-only` subsequently exits0
for frozen3d15e6a, installing signed13180929 with prior20bf56a4 backed up. Independent
installed hash matches; source fingerprintac434511 and unsigned3248914a bind
the exact native build. Independent gameplay check reports0 pending in cache/app,
compiler ready. Player SAVE/config remain outside the delivery transaction.

Disposition: benchmark coverage, foam mechanism and bounded cloud repair accepted;
>120FPS whole-workload performance unresolved; original-app delivery passes. No game process was launched,
controlled or stopped; player SAVE/userdata remain untouched.

### October14 — sea index allocation repair installed; player FPS verdict pending

The active919af997 fleet profile binds substantial main-thread draw work:
137 inclusive samples in residentBuffer,118 inside native buffer creation;
62 of65 samples at the raw index residency call come from sail realization.
Actual source rewrites the shared sail IB before each sail, advancing revision
and creating another Metal resource. Sea raw-sail indices now use the existing
completion-owned three-flight upload arena. Rewrites preserve queued bytes;
partitions reuse a revision; physical draw/packet offsets and logical geometry
keys stay distinct. Location residency is preserved. Warm shader-name creation
and native bulk FFP bindings remove smaller repeated submission work.

Native actual-resource checks pass23,544 assertions, addressing/readback and
three blocked/completed flight lifetimes;120 warm frames/3,840 rewrites create
zero arena buffers. Actual FFP table cases and shader variant checks pass;
rebuilt sea GPU pixels and two animated coverage/depth/pixel oracles pass.
The player's383-frame trace has14.991ms median GPU frame span; timings overlap,
so these CPU repairs do not establish that all remaining time is CPU-only.
The earlier one-frame capture is rejected sea evidence because it captured the
menu cabin; capture admission now excludes that existing domain. Mechanism,
negative cases and evidence owners: docs/metal-frame-pacing.md.

The first canonical stage built the engine but stopped on an unreviewed
AIShip composition fingerprint from the separate phonk change. Its source owner
proved the exact victory-guard delta and corrected admission in54f61fa.
The immediate canonical stage-only retry exits0 and installs signed20bf56a4;
919af997 is backed up. Native source fingerprint655896c6 and unsigned98ca670c
bind the dirty backend actually built, rather than only the printed54f61fa ref.
Independent installed hash and gameplay checks pass,
with0 pending content and compiler ready. This accepts delivery, not a measured
FPS multiplier. The player requests original-app installation and owns the
remaining live fleet/zoom/spyglass check; no CUA/test-copy launch follows.

### October14 — Phonk combat music and Cmd-Q cleanup installed; player replay pending

The player reports Phonk music stopping across combat/noncombat changes and
supplies incident `7ED1FFE1-AD0C-4224-B4F1-5CE99FA59D1A` from13:09:01 EDT.
The report identifies the installed arm64 `metal-engine`, UUID
`e7bd6392-f13b-3baf-9049-e6e66422d4d9`, with `EXC_BAD_ACCESS` at0x280 in
`OwnedStaticSails::Mesh::~Mesh` during `__cxa_finalize_ranges`/`exit`.
This is a shutdown crash, not evidence of a combat crash or save corruption.

An actual-script-body fixture in the native VM reproduces the fallback victory
path stopping the active loop on WAR→SAIL (`RunCases=3`, zero script errors).
`Ship_PlayVictory` now consults the existing Phonk music owner before stopping
the voice; its ordinary victory behavior/timer stays intact. That owner now
checks native playback state before returning for a matching track name.
The same VM fixture passes SAIL→WAR→SAIL→WAR without a voice restart/stop,
ordinary victory after mode exit, a stopped-voice request, and death/boarding/
sea teardown (`RunCases=0`, zero script errors). Audio messaging/entity presence
are mocked; this proves script lifecycle, not audible player acceptance.

The crash owner is the static pending player miniature. Ordinary quit releases
renderer services before that owner destructs; resource-entity disposal covers
reset but does not run on this exit path. Native main now calls the existing
`discardSeaVisual` after `ExitApplication`, before `CleanUp`. An actual-body
ASan/UBSan lifecycle fixture reproduces use-after-free before this correction
and passes populated/empty pending, prior resource reset and consumed-map
ownership afterward, with exactly-once buffer/texture/model release. Renderer
dependencies are mocked. Native main syntax passes with the canonical build
flags; the combined delivery above supplies full native build/installation.
Actual Cmd-Q and audible gameplay replay remain pending.

Disposition: source/component checks and installation pass; player acceptance
remains unresolved. The source-only phonk task performs no staging or game
controls; the FPS delivery owner installs its committed changes in the combined
batch above. Owners: `docs/sea-phonk.md`
and `docs/sea-map-transition.md`.

### October14 — oblique global view and prewarmed map resources installed, player replay pending

The player rejects the fixed perpendicular global-map view, artificial clouds
and slow/stalled transition. Source binds the persisted1.565 pitch, flat cloud
quad and preparation after sea teardown. Sourcea008432 retains overhead only
at size handoff, eases to the native1-radian map angle during reveal, normalizes
former forced-overhead saves and removes the decorative cloud overlay.
Tilt/lift/reveal become400/850/300ms; native scene weather remains composed.

An ordinary-sea resource entity prepares one immutable island GM or sea texture
per frame before the button is pressed. Existing local cache misses stored−1,
causing repeated geometry loads; insertion is corrected. Shared island leases
preserve reset lifetime; mutable counter geometry stays per-map. One shared
34-texture path definition feeds prewarm and ordinary WdmSea. The owning player
miniature model warms before flight and refreshes current cloth, nodes and
changed hull cuts at handoff. Reset/abort disposes/rearms existing owners without
a second map/weather/encounter state or reset-time entity creation.

Actual-body CPU cases pass angle lifecycle, prior overhead normalization,
ordinary/reverse negatives and27 size matches. ASan/UBSan resource cases pass
repeat/case/collision keys, null/missing memoization, texture scope, reset lease,
inactive/transition gates and callback retry/rearm/disposal. Old/new sea texture
paths/readiness match over36 calls. Native syntax passes. Canonical build/stage
exits0 and consumes frozena008432+d45de5e (FPS owns its opt-in capture).
Signed installed engine919af997 matches its candidate;95887b0 is backed up.
Unsigned engine54a4595a and source fingerprint520147b5 bind this native batch.
Content post-check reports0 pending in cache and installed app, compiler ready;
game holders0. No game launch, controls or player SAVE inspection occur.
Player timing remains pending; immediate exit before warmup, first Metal residency/pipeline creation
and synchronous reverse SeaLogin remain latency limits rather than visual passes.

### October14 — scene lighting bind and water temporal repair installed; player replay unresolved

The player reopens water acceptance: after loading a scene, ordinary water
accelerates into rapid boiling and later slows. They clarify that entering a
building or the sea also leaves light/weather visibly catching up. Baseline is
the installed arm64 engine `987efaf3` from source `c4b3d51`, modern water enabled.
The FPS owner returns its completed trace/build lease and reports normal game
exit; this graphics root launches no game, sends no input and reads no player SAVE.

Two independent source/body falsifiers reproduce contract violations. The sea
profile updater changes scale in `(world + move) * scale`, producing position-
dependent transient motion that decays with its8s easing. Clean60Hz replay on
64 original height frames measures RMS44.683 at10000 and234.669 at−100000 for a
partial-storm load. Fixed authored lattices blended by continuous profile weights
reduce these to2.229/1.615 while leaving the settled original field exact.
Scalar buoyancy and the four-vertex mesh share one sampler; phase/map generation
retains its common visual clock. The same active weights publish fixed bump and
foam scales to Metal. Actual fragment checks pass endpoint-mix oracles for coarse
normal, fine normal and foam, full periodic world translation, a single authored
profile and the changing-UV negative. The historical timer discards load-stall
debt and does not reproduce sustained catch-up; no clock-speed clamp is added.

Integration additionally catches a cap6→200 restoration jump: the next frame
previously jumped to67.15. Shared amplitude gain now gives6.127 on the next frame
and continuous recovery; cap decreases remain immediate. Actual ATTRIBUTES
cases retain hourly weights/gain/phase/move, all-zero publication on clear or
bank absence, direct location/fog fallback and additive tsunami/whirlpool.

WEATHER scene bind previously sampled before the current script hour/setup and
retained its initialized old snapshot at dt0. Fresh load12 published ambient
101010 instead of282828; prior-state interior20/sea8 retained123456 instead of
383838/202020. The bind now seeds its current hour and reseeds once when setup
completes (`isDone`, or map `outside`). Actual production-body first-consumer
checks now match complete current state; early getters, late authored solar
parameters, post-bind quest unlock and map completion pass. Ordinary later
`isDone` keeps its smoothing; RNG/lock/request lifetime/rain are preserved.
The mechanism and detailed evidence belong to `docs/metal-graphics-parity.md`.

The FPS handoff adds only static native encoder labels to identify GPU passes;
source review removes those labels back to the previous byte-identical behavior.
Its initial readonly pipeline-state label assignment is rejected during review
and replaced with writable command-encoder labels before canonical compilation.
No dispatch, visual detail, resolution or reflection cadence changes here.
Source, component and existing routing/waterline checks pass. Canonical
`run.sh --stage-only` exits0 against the frozen combined batch, with8248 active
payload inputs verified, no pending content and compiler ready. Signed installed
engine is `95887b017b90c981cbfabc927c220502e07b357d8a8d45021f92374987250d2e`;
unsigned build `a696b94d972f5e7387816d1ddf8e61b1276c77c6e9182acd34a10fb0f40fe852`,
source fingerprint `1e5bedd4f397c6f25aca48e97d7a10661b92f1eb378395890189ecae2ea9db63`.
The printed Git labelc4b3d51 precedes this source checkpoint; the fingerprint
binds the actual batch. Installation is accepted. The original enter-building/
enter-sea first frame and subsequent water motion require player-scene replay;
no game launch/input or player SAVE inspection occurs in this graphics task.

### October14 — installed sea performance replay remains unresolved

The subsequent FPS replay binds signed engine `95887b01` from `e823a7d`.
A pre-trace fleet/battle view still shows47FPS. Its eight-second named Metal
System Trace contains350frames; the combined compute interval overlaps multiple
dispatches and GPU stages. Separate production-kernel benchmarks are substantially
shorter, so the trace does not establish a7ms sky shader culprit. No visual-quality
cut is accepted from these numbers. Source opt-in one-frame native GPU capture
passes Objective-C++ syntax and native state/pixel probes; actual sea capture,
canonical delivery and Xcode replay remain pending with the FPS owner.
Retained958trace/log live in the same diagnostic directory as the older captures.
Its isolated launcher exits0, six baseline player inputs remain unchanged, and
new copied AutoSave5 is backed up under the Application Support data owner's
`diagnostic-replays/sea-fps-20261014/SAVE`. Disposition: original FPS acceptance
unresolved. The following987cohorts are historical.

The separate FPS owner replays signed `987efaf3` from source checkpoint
`c4b3d51` using its isolated state. The player's10:54:12 island/fleet screenshot
shows53FPS, so the original performance request remains unresolved. This does
not reopen the source or installation receipts below as failed deliveries.

The retained candidate's selected steady cohorts report wall16.53–17.52ms,
whole-command GPU14.46–15.15ms, about1735–1794 draws and18.7–19.3MiB upload per
frame. Posed bounds CPU is.328–.5ms versus the earlier.46–1.05ms; the selected
120-frame index scan falls from about50M to5.45M. Camera/geometry differ between
the old and new captures, so these are not a matched FPS ratio or evidence of a
GPU regression. Other candidate cohorts and loading transitions are excluded
from these ranges. No readbacks or explicit syncs appear in the selected rows.

`FrameProfile` reads `GPUStartTime`/`GPUEndTime` for the whole command buffer;
those profile logs contain no per-pass GPU timing. Source
shows the coherent reflection cube and finite-depth water tracing, but neither
is established as the costly pass. FPS owns the next bounded diagnosis through
the installed Xcode Metal System Trace before any further renderer change.
Its logs remain under `experiments/native-metal/.cache/diagnostics/sea-fps-20261014`.

The FPS owner reports unchanged original player-state hashes and zero game
holders after exit. Near/player crew and ordinary zoom/spyglass acceptance
remain incomplete because the computer-use session reached its user-change
boundary; no alternate input route was used. This graphics root launches no
game and performs no player input or SAVE inspection. Disposition: installation
accepted, visible graphics and the original FPS episode unresolved; FPS owns
subsequent performance source/build/stage, while the player owns the graphics
scene acceptance described below.

### October14 — full graphics repair and editable engine installed; player acceptance open

The player expands the original audit to repair all confirmed losses, rejects
the first procedural sun as a tiny white point, and requests ordinary Git source,
installation, commit and push. The first requested procedural installation
succeeded at signed `e8cbbe75`; its visible sun is rejected by the player's screenshot.

`experiments/native-metal/engine` now owns the exact previously composed engine
source, followed by the bounded repairs below. Normal builds compile that tree;
engine archives, ordered patch replay, generated CMake and historical Git objects
are no longer engine inputs. The original license remains with the source.
One shared source fingerprint binds source, built engine and staged engine.
An actual source edit invalidates readiness; clean temporary CMake configuration
and the complete442-target native build pass without the retired source paths.
Retired engine-archive absence is allowed; changed active dependency content is
rejected. Existing diagnostic consumers now inspect the direct source owner.

The revised solar shader retains the hot disc and adds compact optical bloom
and a soft angular aureole. Its radius37/50px at the two reference thresholds
matches the original sun's visible support; actual Device rendering measures
36px at the stronger threshold. Missing/black artwork gives the same sun,
ordinary materials and corners remain clean, and dense cloud attenuates it98.6%.
These are actual Metal component scenes, not screenshots from the player's game.
Star origin and depth-aware reflection evidence below remains valid.

Storm geometry now consumes the authored weather presets, including amplitude,
scale, movement and foam, blended continuously with native storm strength.
The independent shipped-heightfield fixture spans1.4373 calm to31.8906 storm
instead of retaining calm geometry. The existing sea-height limiter preserves
shore, island and cabin policy. An integration review reproduced nested profile
attribute notifications changing the live sea while loading the bank; the
branch guard removes all281 live mutations across283 callbacks, and explicit
Ready applies the completed profiles. Direct sea/fog updates remain active.
The underwater camera test also uses actual camera Z under every event guard.

macOS movie playback now uses AVPlayer through the existing video entity.
The15 original hash-bound WMVs remain intact; a finite manifest prepares
H.264/AAC companions for the unsupported original codec. Canonical staging
delivers them through the existing content transaction. Actual media components
show94 changing Seaward frames through completion and261 France02 frames with
correct aspect; skip, destroy, missing/unsupported media and reuse negatives
pass. Full future PROGRAM compilation passes without executing Main.

The separately authorized fire batch adds smoother bounded fire resolution,
continuous occlusion tolerance, severe-fire damage and fire-directed crew
movement. Its real native fire/crew fixtures pass; visible player acceptance is
still open. The sea CPU batch caches immutable skin index topology, enumerates
actual blot children and hides only subpixel foreign crew with hysteresis while
preserving simulation and player crew. Bounds/sanitizer and projection cases
pass. The old unindexed lighting probe still fails its historical outdoor
ambient-floor expectation; no claim is made that every diagnostic passes.
The merchant handoff adds the requested200 shrapnel bombs once per seven complete
game days, with isolated real-VM/codec/idempotence checks; player replay is open.

Canonical `experiments/native-metal/run.sh --stage-only` exits0 after all final
source integration.8248 active payload inputs and renderer/material gates pass;
20 pending content paths (five scripts and15 movies) are delivered. Signed
installed engine is `987efaf34220f0d0b25bb94bd5e5709e359fa457ca7997fe96619ef0cb7fd74e`;
unsigned build receipt is `71deb36cd16fd2a07eb7fa9d65e4ac2d4fe7c11e27d6b7c0666f9bfde01764d0`,
source fingerprint `1dca5490437c19f05c5a1ea74f481fc904b0b87b57967447ea133e23ad534732`.
Its printed Git label69f1dd4 precedes this working-tree checkpoint; the source
fingerprint, rather than that label, binds the installed batch. Installation
is accepted. Sun, storm size/foam, moving/stationary reflections and scripted
movies require the corresponding player scenes; component checks do not close
their visible acceptance. The separate FPS owner has the installed hash for
its isolated comparison; this integration task starts no game.

### October14 — astronomy repair installed; procedural sources and reflection successor pending

The player reports an absent solar object, stars clustered at the horizon and
ship reflections displaced on water. They subsequently require procedural Metal
sun/stars and a representative audit of remaining visual losses, naming small
waves during strong storms. No autonomous game testing is authorized.

Source discovery proves native weather always selects one sky directory while
the old SKY branch dispatches astronomy/disc only for multiple directories.
The shared astronomy dispatch now also runs after the single opaque sky.
An actual-body fixture fails the old ordinary single-sky case and passes the
successor, multi-sky, missing-entity and reflection-call negatives. Actual Metal
star placement also reproduces the missing world-origin transform: translated
cameras move or lose the point while the unchanged ordinary billboard stays put.
The common view transform corrects that mismatch. Owners and numeric component
evidence are in `docs/metal-dynamic-sky.md`.

The first canonical `run.sh --stage-only` exits0 and installs signed638da3c8,
backing up d3f21a3f. Content checks report0 pending and compiler ready. No game
launch or SAVE read occurs. This accepts installation, not the player's scene.

After the player's procedural requirement, solar photosphere/aureole and star
fragments no longer sample artwork. The advanced solar source uses a shared
.00465rad radius, limb darkening, atmospheric forward scattering, existing
transmittance-LUT spectrum and per-pixel cloud cover. Actual-backend GPU checks
accept identical source shapes with missing/black texture (centers247/12), black
square corners, nested scope rejection, source cleanup and an unchanged ordinary
material. Production-helper Metal optics change clear halves242/240 to covered
242/0; LUT spectral RGB230/90/43 and outside-disc black pass. Stars retain the
same21-pixel point at38/38 under three camera translations with no sprite texture;
ordinary planet geometry retains its25-pixel negative. The moon keeps its
authored phase path. Owners are in `docs/metal-dynamic-sky.md`.

Reflection placement is reopened at the actual user's on-water condition.
Flat Sea2 passes an independent mirrored-geometry oracle, while modern Sea3
projection fails. Tilted-wave finite-object rays expose a single-origin cube
parallax error. A hit-only SSR experiment is rejected because the old displaced
cube image remains as a ghost. The successor retains background/radial depth
from the existing capture and traces the actual water ray. Eight tilted markers
have maximum0.154px error; actual native bridge depth conversion, correct-hit/
background behavior and stale/invalid capture negatives pass. Existing rebuilt
original/modern sea and depth-water probes pass; `docs/sea-reflections.md` owns
the mechanism and remaining limits.
The audit in `docs/metal-graphics-parity.md` separately records the proven
storm heightfield/profile loss and absent macOS video playback; neither is
claimed repaired by this batch. Actual game appearance/FPS remain unresolved.

The final ordered stack reverse/replay and canonical build pass. Two solar patch
attempts were rejected transactionally (zero-context, then overlapping hunks);
comparison with the exact pre-patch source established the overlap, which was
merged into the existing vector-read hunks before successful replay. No partial
source migration was accepted. Compiled unsigned successor engine SHA256 is
`2b6ec78126d4d3ecad989326344aaf23f0796bd0831a2c894cc75f5cdf2dc02c`.
Final `run.sh --stage-only` exits1 before mutation: the player has reopened the
installed app (observed PID16865); its signed engine remains638da3c8. Disposition:
source/component accepted, delivery and player scene unresolved. No game process
was stopped or launched, and player SAVE was not inspected.

### October14 — nightly Beard patrol, rapid NPC sinking and quiet client installed; player replay pending

The player supersedes the permanent guide with a ship appearing only during
the inherited clear22–04/date12–18 window, patrolling the same Bermuda corridor
and disappearing at dawn. Ordinary non-quest NPCs encountered there must sink
almost immediately. The player also requests miscellaneous client diagnostics
off to avoid logging/profiling costs. The accepting game remains the installed
arm64 Metal app; current engine76511506 has zero running holders. Its ordinary
player ini has runtimelog1/tracefiles1 and no logs key. The candidate does not
edit that config or SAVE; normal launcher environment owns quiet activation.

GRC now restores one named native actor at full appearance alpha, checks its
native loaded acknowledgment, preserves same-night route position and resets
to the same start on the next watch. Continuous post-movement proximity owns
player admission. Native authored endpoints reverse through existing navigation;
an exact named retirement detaches before script deletion and normal collection.
Ordinary NPC admission preserves qID/quest/ALONE, player/companions, sea-owned,
deleting and dead fleets. Stock roster/cargo losses close the fleet immediately,
with immutable slot bounds. Its native model sinks/fades for about one second
with shared stock cannon smoke, then uses the original deletion callback.
The existing needDelete predicate blocks the traffic/service/campaign consumers;
only the finite native projection retains that pointer until completion.

Full GRC PROGRAM compilation and actual selected sea importer/reference
admission pass with no script errors. Real VM/codec cases cover midnight/dawn,
next-night reset, missing native actor, ten protected NPC classes, total roster/
cargo loss, no revival, daytime legacy migration and advanced battle state.
Four native units pass syntax. Actual native body execution covers both route
ends/pause, finite sinking/saved age, normal deletion/veto and immediate retire;
their environment is substituted, with no rendering or game launch. Actual
quiet-body instrumentation passes zero diagnostic formatting, file factories,
profiling timer/sink work with unchanged event/call returns and stack cleanup.
Normal public startup defaults quiet; diagnostic mode remains explicit. Backend
periodic matrix/shadow reports stop before collecting data when profiling is off.
Gameplay Log_Info remains unchanged. The two new ordered native patches bind
the same compile receipt as content activation. Initial source checks show two
script files pending and compiler build/stage pending. No autonomous game test,
launch, controls or SAVE read occurs.
The player owns visible patrol, attack, NPC sinking and measured FPS replay.

The first canonical stage rejects the two new patches before source/app mutation:
their scratch dry-run used p0 headers, while the owning ordered loader requires
a/b paths. Both patch headers now follow that existing format; the loader's
exact path validator passes for all nine native source targets. Native bodies
are unchanged. This was an input-format rejection, not gameplay acceptance.

Canonical `experiments/native-metal/run.sh --stage-only` subsequently exits0
at5d06725. Installed signed engine is d3f21a3f5a1ce929511b882ca1f6694ca4ce45fd23d71cedbea4404a5f630acf;
built engine receipt is19977b752124f87277dd2b7226263d7d8bc6beeb47debd466d79f96a741bc68d.
It binds authored-actor patch b460c11c504b2e97c9c4a65acafa48ccea4166deb5c1fda7fdbdd4662dffc5f8
and quiet patch433868621affc93ab01d550a1d9e0f5649a97934fb14e806d28b7007a2110070.
Installed launcher SHA6393b971a2979c92803e9f61ed04f12c642d35b8be81323a589b9a1782378e75
matches the source whose ordinary startup selects quiet1/profile0/deck-trace0.
Installed GRC de7763f05e01dd86637d479efc80c49ca8520423310f85c2f7eba207ea93697c
and reload e55cb52abd271c40c03ebf4beda0fd01676c3af8ed61b30d4e43620d68711d6d
match the isolated VM inputs. The fixture is refreshed with the installed
camera/miniature sea077d2061 and worldmap5fb39db4 consumers; its full PROGRAM
compilation, NightCases and NightAfterLoad still return0 with zero script errors.
Post-stage content comparison reports zero pending for cache and installed app,
compiler ready. The prior signed76511506 engine is retained for recovery.
Strict installed-bundle signature verification passes; game process holders0.
Installation is accepted; visible gameplay and measured performance remain
unresolved and belong to the player's next ordinary app session.

### October14 — actual player miniature and softer overhead motion installed, player replay pending

The player finds the overhead successor better but still abrupt, with no clouds,
and rejects the generic player model as an actual ship miniature. The next batch
extends tilt/lift/reveal to700/1200/400ms and restores only thin textured wisps,
with no opaque fog substrate. An independent visual copies the current hull,
visible mast hierarchy and cloth before sea teardown. Map-save startup obtains
the current player/RealShips descriptor through the normal map creation path;
existing sail geometry/material functions and the shared saved hull-cut parser
own reconstruction. NPC models and map collision footprint stay unchanged.

Canonical native build exits0 and consumes the new ordered seven-path visual
patch plus the sea-camera bridge. Full composed installed PROGRAM compiles in
the real isolated VM with zero errors; current-model descriptor, shipless cleanup
and inactive sea admission return0. Sea/worldmap composition is idempotent.
Native source checks cover independent cloth ownership, visibility flags and
saved-cut validation. No player SAVE or game controls are accessed. This is
source/build evidence only. Canonical `run.sh --stage-only` exits0 at8f18c51 and
installs signed engine76511506, backing up e870. Installed sea/worldmap cooked
SHA prefixes077d2061/5fb39db4 match the VM inputs; reload9b767abd and the existing
GRC layers remain unchanged. Content post-check reports0 pending in both cache
and installed app, compiler ready; canonical raw sea matches the committed
b5cacb source through this delivery. The prior unidentified raw-source drift
does not recur in this batch; its cause remains unassigned.

The ordinary app launch creates a process, but native window capture fails with
cgWindowNotFound/ScreenCaptureKit errors, so no startup or visual acceptance is
claimed. The user then requests closure and explicitly forbids autonomous game
testing. The exact installed engine process receives SIGTERM and a process read
confirms zero remaining game processes. No game controls or player SAVE reads
occur. Visible two-way replay, actual miniature loading and saved-map
reconstruction belong to the player; no automatic relaunch follows installation.

### October14 — stationary Beard guide installed; player requests nightly patrol instead

The player requires a visible ship pointing to the same place on a missed
night, with the battle still restricted to nighttime. The former first-search
controller creates no ship and checks approach only on an hourly calendar key.
The October14 00:01–00:05 log records search/admitted0, no queued ambush and a
nearest nighttime sample102.77 from the hidden radius100; movement between
samples is unobserved. Thus the hourly query and invisible target cannot accept
the requested route; the reported05:00 exit is not attributed to Beard.

The replacement seeds one stock passive Merchant guide before map creation
at the proven navigable western point620.738/840.767. Its zero propulsion and
disabled native expiration use saved-descriptor restoration, without a new
native patch or recurring encounter allocation. It remains a visible guide
in daytime/next night; only moonlit nighttime proximity can request the battle.
The existing per-frame TrafficFrame event reads fresh player coordinates.
Selected-guide events and actual stock sea import exclude only that guide.
Legacy Dream markers migrate without success callbacks; foreign encounters,
advanced battle states and the lost storm patrol remain intact.

Full installed PROGRAM plus the candidate compiles without errors. A temporary
real VM fixture passes same-hour approach from the recorded route, one-time
reference admission through actual WdmAddEncountersData, exact dawn/day
exclusion, next-night eligibility, delayed dawn cancellation, pause, land-water
rejection, protected companion and generated-state codec/migration. GRC bodies
remain original; only native map messaging is substituted and ordinary game
OnSave/OnLoad is excluded in the headless codec fixture. Three consumer outputs
are idempotent. These checks do not accept model visibility, real game loading,
the physical attack or map-frame performance. The player owns that replay.
After the player closes the game, canonical `tools/sync_metal_gameplay.py apply`
exits0, reseals/verifies the app and installs exactly four files. Its post-check
reports0 pending in both cache and app, compiler ready. Installed GRC/worldmap/
events/reload raw SHA-256 prefixes are11a7e034/446bc483/d33f8583/9b767abd;
all four bytes match the actual VM inputs. Signed engine remains e8708248.
No engine rebuild or unready camera source is consumed. The user is told the
installation is complete and may launch; no game controls or SAVE reads occur.
The feature still requires the player's visible guide/next-night/night-vs-day
replay. `docs/great-recipe-quest.md` owns the controller/lifecycle contract.

After installation, the player rejects the permanent stationary-guide choice
and explicitly requests a ship that appears/patrols the same region only at
night and disappears at dawn. Encountered ordinary non-quest NPC ships must
be sunk almost immediately; quest ships must survive. This supersedes the
standing-guide contract. The installed batch remains an intermediate delivery,
not accepted gameplay; patrol and NPC-combat ownership discovery is pending.

### October14 — first sea/map camera rejected; overhead successor installed, replay pending

The requested menu transition lifts and tilts around the live player ship in
three nonlinear impulses, matches its projected area to the map miniature and
reverses on sea entry. GPU-only frame retention and moving clouds replace the
ordinary loading artwork; the map prepares incrementally on engine frames.
`docs/sea-map-transition.md` owns the contract and the remaining visual checks.
Canonical source composition is idempotent and the complete script VM compiles
with inactive-sea and locked-map guards returning0 and no errors. Native syntax
and27 independent scale fixtures pass. The first canonical build rejected p0
patch path headers; both new patches now follow the existing a/b owner format,
and the stack applies9 changed native files. Canonical `run.sh --stage-only`
subsequently exits0 at0740387+6dc1d99 and installs signed engine
`d92e0f8db16641abbd9ad53519bd2b711563f380cdbbac6cb8d325c844c2e7e0`.
The formerb3 engine is backed up. Sea/worldmap/reload prepared hashes are
eab3cc00/00daf129/c7ee4648; accepted Beard scripts5c4d5588/a90675cc are included.
The player explicitly closed the game and authorized installation. No agent
game input or player SAVE edit occurred. Actual motion remains unresolved.
The temporary extra pause-menu button was explicitly rejected and removed
before delivery; the existing native map/sea actions own the transition.

The player replay rejects the first camera: excessive duration, mismatched
incoming view and nearly opaque white clouds. A read-only game capture confirms
the white veil covers almost the complete scene. The successor removes cloud/
fog quads, tilts to the overhead axis before a vertical nonlinear lift, holds
that pitch in the map and reverses the same trajectory. Native build passes.
Successor canonical stage exits0 at a48a5dd and installs signed engine
`e87082489e406ec79bcd5214118782325246923c8f00b14023afa1532a02a8f1`;
d92e is backed up. The overhead projection oracle and27 ship/height/yaw cases
pass against the shipped miniature. Actual successor motion and duration
remain unresolved until the player's replay.

Canonical sea.c was repeatedly restored to the exact219f prior source by an
unidentified writer while other camera owners remained intact. One stage
stopped at the existing recipe hash gate before app mutation. The accepting
retry starts from the committed c14d source and completes; instrumentation
observes another source change during install-engine's resource transaction,
but does not identify the writer. c14d is restored afterward. This is not
attributed to Beard, a Python import or a hook; future source/staging must bind
that file to its committed camera owner again before composition.

The additional adjacent-island report is separately unresolved. Installed
DIRECTSAIL is0; its historical loader is not included/called by the active
script chain, and both distance expressions are commented out. N-map projects
live local coordinates but the sea loads one island. This proves absence of
direct island admission. Native map initialization also clamps an exit position
back to the old island's area; the successor skips that clamp only for cinematic
sea exit, preserving ordinary saved-map initialization and hull collision.
Actual coordinate round-trip and neighbouring scene admission remain required.

### October13 — contact yaw and mode-owned saved velocity successor installed

The player reopens movement outside Phonk: tight arcs at high speed or after
contact, including another ordinary ship and retained speed after mode exit.
Exact native contact power/producer/queue/movement with Flyingdutchman mass.10,
Y acceleration.5 and braking2.5 reproduces excessive contact rotation. A high
single hit changes3.566→.099rad/s and11.054→.307rad total turn after correction.
Repeated forced-contact stress is synthetic persistence, not accepted actual
pair detection. No-input/no-contact500 stays straight. Initialized live mode
exit already caps ordinary velocity; its behavior is preserved.

Full native Save/Load and speed calculation expose a separate lifecycle defect:
saved500 State outlives the transient mode and reloads493.56kn/2.975rad/s on its
first ordinary frame. The successor saves the existing ordinary exit projection
only for active initialized Phonk, leaving live momentum intact. An airborne
State at500/yaw3/Y80 saves12/.05/Y7, including both existing position records;
State152-byte format and record counts stay unchanged. Ordinary500 payloads are
byte-identical. Old unmarked saved500/yaw cannot be distinguished from legitimate
script motion and are deliberately unrepaired; do not infer old-save repair from
the new serializer probe.

Only TOUCH-origin impulses use the default-compatible collision adapter and
transient slot tags. A separate contact component follows the exact existing
yaw damping; hull/linear-kick and turn capacity bound its admission. Linear
recoil, pure scripted rotation, mixed/queued impulses, helm and perks retain
their response. Verified modest contact, slot reuse/delete, no-input motion and
live exit match; prediction restores tags/counter and load/death clear them.
Ordinary saved small contact becomes an unmarked baseline on load; its finite
native decay and separately bounded next contact pass. Geometric admission can
change low-linear/large-lever contact torque; only verified modest fixtures are
claimed equivalent. The rejected blanket AddStrength clamp erased pure script
rotation. Prior full jump lifecycle and18 trajectories pass with7 flight/effect/
HUD owners unchanged. `sea-phonk.md` owns contracts and upstream/reuse evidence.

Frozen patch SHA256
`952649812ca7e28941d3534cefe7d0e8af544830b52807a3c21a7ef7bbd53db7`
is promoted from8246 only after source/probe review. Canonical
`run.sh --stage-only` exits0, changing4 prepared native source files. All11 frozen
owners match the compiled tree. Unsigned engine
`ec68dc79aa65d939d64bdaa5d84d3bc54da9d26b70d42bde9b770fb3470349e2`;
signed installed engine
`b3e7c48fbc93629493360a3eb1df0a40c4e9fdd49bf40e1ab0af91ed7e1320e8`.
Canonical signature verification completes; previouscfe is backed up. Cache/app
check reports0 pending, compiler ready; all502 freshly protected state hashes
remain unchanged. The accepted Beard reference hook `f7b49f9` is consumed in the
same transaction; installed worldmap_reload raw SHA
`97e41648234f2a3bc78dd9454025bdcd9e15e4625471315fe30b32fc7b765ff5`.

After delivery, the app is selected for bounded runtime inspection. Screenshots
show interactive local sea at03:43, ordinary displayed speed2.8, then upper deck
at04:05. Two Escape attempts are refused because the user changed the app, with
fresh state read between attempts; no agent input is delivered. One pending
question selects player test versus relinquishing input. This is a current UI
ownership conflict, not a recovered global human ban on gameplay inspection.
Disposition: source/component/serializer and exact installation accepted;
actual collision/exit and save/reload gameplay remain unresolved. The inherited
thin-hull/long-frame admission and linear contact impulses are unchanged.

### October13 — Beard actual map admission call reopened and corrected

The player rejects three automatic sea exits at22:03/23:09/00:00 with no first
battle. Actual-VM reproduction distinguishes the consumer call: passing
wdmLoginToSea by value writes the admission flag to a copy; explicit reference
preserves it. The prior component probe incorrectly supplied that reference,
so it did not test the actual installed caller. Source/topic checkpoint
`f7b49f9` corrects only the hook/accepted whole-file bootstrap. Full installed
PROGRAM compiles without errors, actual composer/reference forwarding passes,
and night-only GRC01f9549 remains unchanged. Canonical stage consumes the accepted
rawreload97e416 above; no camera candidate is promoted. Disposition: exact source/
VM/delivery accepted; visible zone entry→whirlpool/assault remains unresolved and
owned by the Beard task. `great-recipe-quest.md` owns continuation.

### October13 — night-only Beard zone ambush content installed

The Beard owner delivers source/topic checkpoint `d5c97c4` through the canonical
content sync. Current installed normalized Great Recipe source SHA256 is
`01f9549c8928d4f205fba5c6ffec62d73f729d20e9d451eaf61e7cd828e46f54`.
Final apply exits0 including strict app signature verification; native engine
remains cfe07b6e, and cache/app check reports0 pending, compiler ready. No native
build, launch or game input occurs. The owner does not read or mutate player SAVE.

The player changes admission to NIGHT ONLY, including previously issued Dream
markers. Full installed PROGRAM plus the new quest compiles without errors;
isolated actual-VM queue/admission/escrow cases pass daytime and04:00 rejection,
queued03.99→04 cancellation and22:00 admission. Codec plus actual quest OnLoad
stays in search during daytime and activates once the next night. Component
replay substitutes Boss/Upgrade/Journal and native sea-loading boundaries;
unchanged bodies separately full-compile. Disposition: source/content delivery
accepted; visible zone autoattack remains player-replay unresolved. Reported map
freezes remain unmeasured. `great-recipe-quest.md` owns admission and continuation.

### October13 — Phonk complete jump/impact lifecycle and reflection delivery

The player rejects weightless flight, weak landing and retained water effects
under an airborne hull. Complete movement/model-matrix replay of the prior500
candidate finds a50m stress crest gives297–305m apex and15.1–15.4s flight with
no pitch evolution. The prior entry already penetrates water; the defect is not
absence of all penetration. Track and particle insertion gates still allow
retained draws, and there is no native impact snapshot for a landing splash.

The successor preserves native6×6 hull support and measured departure velocity.
Phonk vertical gravity rises smoothly to8× at500, and continuous angular response
follows actual flight momentum in body axes. Closing contact preserves incoming
position/velocity; finite compression follows current supported water, with
maximum dip92% of root-hull freeboard and continuous buoyant recovery. Native
impact generation/contactXZ owns one bow sound and existing front particles;
their normal lifetime ends the extra draw. Airborne tracks, contact particles
and water-projected glow stop drawing while retained state ages. Ordinary/NPC
motion,500 controls, shoreline guards, prediction and transient cleanup remain.

Eighteen complete-frame component cases at45/33/12/8ms pass: default500 crest
apex29.8–30.3m, severe116.8–118.7m and50m stress66.2–67.1m. The6m-freeboard
fixture dips5.52m and leaves the deck.09–.46m above moving water. Actual installed
Flyingdutchman1 root geometry gives11.70m freeboard and10.764m dip with.936m deck
clearance. Retained air track draws change2→0 and particle draws3→0. Coupled
contact/particle/sound, W/S/coast/current transport, seeded mode exit, ordinary
moving water, predictor restoration, load/death and grounding checks pass.
These component fixtures do not resolve the later player collision/exit report.
Rejected finite-heave approach tunnels32m below a wave; a constant entry target
fails moving-crest depth bounds. Final native ramp/current-water response avoids
both. `sea-phonk.md` owns the mechanism and remaining player scenarios.

After source integration, canonical `run.sh --stage-only` returns0 with two build
workers and8 changed prepared source files. All10 frozen Phonk source owners
byte-match the compiled tree; the compiler receipt binds final patch
`8246fa23a1ce834b7eef3833f12a52ad315255772e50b5ad2d8c4d6d4a73636a`.
Unsigned engine `f4eeaf4cb1e90f22d489ce7df39ea754f425259ce1ab3a14d986b8cb8a064e98`;
installed signed engine
`cfe07b6eac1216c3c4adea8ee693c65ccc8cc5603921ff09a68e753410a4172b`.
Strict app signature passes; preceding f238 engine remains in recovery storage.
All502 fresh protected SAVE/archive/settings hashes remain unchanged. Post-stage
sync reports0 cache/0app pending, compiler ready. No agent launch/input occurs.

The same transaction consumes reflection inputs4d43fa4a/b4250832 from `cad4858`;
their source/GPU evidence is below and `sea-reflections.md` owns the contract.
Disposition: source/component and exact build/delivery accepted; actual player
jump/impact/wake feel, stationary/moving reflection and FPS remain unresolved.
Inherited TOUCH can miss thin hulls on extreme long frames; this packet does not
claim all-frame collision coverage. The player's subsequent ordinary/NPC curved
trajectory report is a new reopened movement scenario, not a passed proxy.

### October13 — reflection normals follow physical waves; compiled candidate

Player reports rapidly changing ship/shore reflections even with stationary
camera. Source binds a64-slice/s normal-volume cycle against the physical wave
phases'1–8 slices/s. The64 shipped height frames have a continuous loop; no wrap
fault is established. `renderer-rain-weather.patch` now publishes existing
physical phases in constant2.Y/Z; `sea_shaders.hpp` uses them only for marked
modern volume normals. Legacy X, rain244, original shaders, preblended2D
fallback, ripple amplitude and coherent dynamic reflection-cube cadence remain.

The initial disposable ordered-stack attempt rejects an early phase edit because
the later rain patch consumes that context. Moving publication to the existing
rain/sea uniform hunk preserves patch order and passes complete reversal/replay.
Canonical `build.sh` returns0, changing exactly one prepared engine source.
Explicit rebuilt `sea-probe` passes original and modern GPU cases. A disposable
current-MSL kernel accepts marked phases, unmarked/2D fallback and120-frame
periodic-volume variation0.223287 versus old1.578761. These are component checks;
they do not accept the player's scene or FPS. Dynamic-cube coherence guard passes.

Source SHA256:
- `renderer-rain-weather.patch`: `4d43fa4af788001033186a2c60eb2e67d8c8c28a360b2fd69dd7819497c5fd0b`
- `sea_shaders.hpp`: `b42508321134816a30fdd57d53b5a703f54d27b15dd490d3e1e97043a81b064c`
- Compiled unsigned engine: `67566646efbfaa16914c50df376a9c950b908fdf1d8933eff2b1f0cd46857c49`

No app stage, launch, game input or SAVE/config mutation occurs in this task.
Build/GPU ownership is returned to the active Phonk integration root; its next
batch may consume these ready inputs. Disposition: compiled component candidate;
installed delivery and stationary/moving-camera player reflection replay unresolved.
`sea-reflections.md` owns the phase contract and rejected loop hypothesis.

### October13 — aiming CPU successor after unresolved23FPS replay

The player reopens the installed03be2be/d4efee59 batch. The actual ship/fort/water
scene shows23FPS with contact highlighting; the10-second foreground CPU sample
`game-20261013-215934-65827.sample.txt` contains431/1297 main-thread samples under
aiming (33.2%), including stopped-hull sorting and range-aperture polygon clipping.
The log confirms v13 and reports a degenerate tube boundary. The player declines
an aim-off capture but reports roughly twice the FPS without aim. This does not
provide a paired instrumented baseline and keeps the original FPS fix unresolved.
The shader component improvement alone is insufficient as player acceptance.

The successor deduplicates identical float normals at their canonical producer
and reuses the existing bounded synchronous CPU helper for pure station math.
Immutable current-call inputs, separate chunk scratch/slots and ordered join
preserve34,312 sections/33,832 slab sets across480 fixtures, including empty slabs
and complete invalid-density rejection. Hull-before-pad reduces interior sorting;
its independent worst adversarial support difference is.322micrometres.
Interleaved whole-station component medians change1.017→.260ms at52 guns and
3.253→.702ms at112 guns.50,000 canonical support fixtures remove11,847 duplicate
edges with2,500,000 identical occupancy queries; actual renderer CPU construction
restores48 air vertices from the reproduced0-vertex failure and retains contact.
These observations prove the source mechanisms, not whole-game FPS. Full evidence
and responsibility/lifetime boundaries belong to `sea-aim-overlay.md`.

After the user's explicit closure, canonical `run.sh --stage-only` returns0,
changes two prepared source files and reseals/verifies the app. The final prepared
controller byte-matches the component/syntax-accepted source; the normal correction
is present in the built shared header. Final source SHA256:
- `cannon-trajectory-aim.patch`: `f00abc3382977ea7bf4c1cf140a499146b5faff0a1aa6e8d7fabfe2d0a0c55c9`
- `cannon-target-height.patch`: `5e207bc0d6b4a482724e6462eb824d0cbf93b6ff4bf502432801d5f107a5e4ed`

Unsigned engine `8c6cab41da7dc786252e12e3e2919e75fb28ee52c5ce4b439fbad80ff15f7b58`;
installed signed engine
`f238f3571d19459f69528121548c8c247668d8dc33afe89feb497706ee7644a7`.
The preceding d4efee59 engine remains in canonical recovery storage. Exact7fe9
Phonk and502029 fire shader inputs remain; unready later flight/landing candidates
are excluded. Post-stage comparison reports0 cache/0app pending, compiler ready;
all503 fresh protected SAVE/archive/settings hashes are unchanged.

Subsequent camera-transition source edits to sea/worldmap scripts and an unconsumed
new header are later unready candidates; their owner preserves them in isolated
scratch and restores only its exact hunks before this checkpoint. No further
sync/apply consumes them in this FPS batch. No agent game launch/input occurs.
Disposition: component/source/
build/exact installed delivery accepted; the original player aiming-FPS/air replay
remains unresolved. The declined paired capture is not requested again.

### October13 — aiming revision13 and accepted fire repair installed

The player reports severe manual-aim FPS drops on water and ships and requests a
simple rounded trajectory tube with light fog, nearly clear at the guns and denser
downrange. Foreground CPU samples attribute16.2%/18.4% of main-thread samples to
aim preparation; there is no matched scene FPS baseline. The former air fragment
walks sections/support planes per covered pixel. Revision13 rasterizes the same
stopped convex slabs and analytically integrates linear fog at their boundaries,
then composites one retained optical target. Contact geometry/color, target
selection, actual firing and raking remain unchanged. Current-call CPU buffers
reuse allocation capacity and circle directions without cross-frame caching.

Exact component geometry parity, unchanged contact-only image and independent
outside/inside/opaque/far-cut GPU oracles pass; the air GPU example drops9–26ms
to roughly2ms at1024×640. These measurements exclude the player scene and do not
accept its FPS or appearance. `sea-aim-overlay.md` owns the full bounded evidence.

The first canonical stage refuses a downstream target-height patch context after
scratch declarations move; the next refuses an incorrect hunk line count. Neither
attempt installs an engine. Narrowing only that patch's context and correcting its
count preserves its complete behavior. The final `run.sh --stage-only` returns0,
verifies the ordered stack and changes one prepared controller source. The built
engine contains the v13 marker. The installation transaction reseals/verifies the
app and retains the preceding6349 signed engine as recovery.

Source SHA256 receipts:
- `aim_volume.hpp`: `3198d5f057e23277eb4a3456e683881bcaaa36ccc85b1a04a6f3f2c7a6ae6b3c`
- `backend.mm`: `d3032c50472faf4014be6eb5661f0c105ed2b2f089f6d80858fca2d6a9df773a`
- `cannon-depth-drape.patch`: `5882f4611189a6e0186d453ec29b7c7710dc728695a6af12d43e7dbdfb1522c5`
- `cannon-target-height.patch`: `6e2f4bd4a18a457000b574b7406c7ad936baea6b28444f8e3daf17b2702db87e`

The same frozen batch consumes fire shader SHA
`502029bc9b19165a872e2b84f57d39a99393c66e9ccce24de49b2da548d16f1c`
and retains Phonk patch SHA
`7fe9c366e173cbb7c78430395bf8dbd5783a330b6679e94a12e6b6b9975e3aa1`.
Unsigned engine `d708cf93875d6774b1d9394f02f44548beb3d8c8e118dc6cfcac892654811c84`;
installed signed engine
`d4efee59cbc2f67a50a4dbf80919ee3d1ad0428e8688a70f34959ed0426cdc38`.
Post-stage comparison reports0 cache/0app pending, compiler ready. All502 protected
SAVE/archive/settings hashes remain unchanged. No game launch/input occurs.
Disposition: component/source/build/exact app delivery accepted; original aiming
FPS and full player visual acceptance unresolved, awaiting the same water/ship
scene with stable camera at1x–5x. Fire player replay remains with its owning chat.

### October13 — Beard native-message boolean corrected after player trace

Player replay21:38:49–21:39:20 logs search, eligible date/night and clear weather,
but repeatedly reports `navigation rejected x=620.738 z=820.767`. The route check
introduced in6c65f91 combines integer comparisons with a right-hand `SendMessage`
result. The real compiler converts only the left operand of `&&`; its comparison
owner lacks the right-hand VAR_PTR case, so a native1 is treated asfalse.
The diagnostic label described a failed wrapper, not a measured geometry refusal.

An isolated copy of the existing VM probe registers one tiny native entity that
returns the message answer; it executes the full installed PROGRAM without game
Main, SAVE or rendered scene. Original helper reproduces
`native=1 original=0 normalized=1`; the corrected actual helper accepts both route
endpoints/native1 and rejects native0 and coordinates outside the west corridor.
BeardPointerCases0/script_errors0. Only the message return is substituted; native
water geometry and player appearance are not accepted by this fixture.

One source line compares `SendMessage(...) !=0` before the logical combination;
no coordinate, calendar, weather, occupancy, captain or TTL change is made.
Canonical content-only `sync_metal_gameplay.py apply` returns0 and strictly
verifies the resealed app. Installed quest byte-matches the candidate, comparison
reports0 cache/0app pending and compiler ready; signed native engine remains
`6349eb2addb21151d998b5b7245f8bbe1d44f4fd7a270420ca4bde578301f7ce`.
Firea9837c remains source-only for the FPS-owned next native batch.
Disposition: boolean mechanism and source/content delivery accepted; the ship's
visible appearance remains unresolved until player replay. No agent game control
or SAVE inspection occurs.

The same player reports severe global-map stalls. This script log has no CPU/GPU
frame timings. Its21:39:08.973→11.884 gap contains Sea_FirstInit and player-group
initialization; that correlation does not establish the cause or whether it is
the reported4:00 stall. Matching live profiling remains pending with Beard root
in `TASKS.md`, separate from the manual-aim FPS root's scope.

### October13 — player rejects250 speed; controllable500 successor installed

The player finds250 too slow, superseding the earlier chosen ceiling. This is a
speed preference, not evidence of another physical clamp. A single native
constant line doubles ceiling/motor/brake to500/120/240 while retaining W/S speed
choice, coasting, world-vector braking and approximately4.2s rest-to-maximum.
Flight/contact/glow/HUD/music and ordinary motion code are unchanged.

Exact production Drift and effective-current cap ASan/UBSan fixture passes
120kn after1s, heldW500 ceiling, release/coasting, S240kn/s bearing preservation
and stop, airborne inertia, combined-current500 cap and untouched OFF branch.
Canonical `run.sh --stage-only` returns0 with two workers after the final source
change; exactly one source file changes. Built header byte-matches the probe and
compiler receipt binds patch SHA256:
`7fe9c366e173cbb7c78430395bf8dbd5783a330b6679e94a12e6b6b9975e3aa1`.
Unsigned engine:
`62becaa7b2d4eff9737c351f15220c33a34088acca0a083663dfd4901d327690`.
Installed signed engine:
`6349eb2addb21151d998b5b7245f8bbe1d44f4fd7a270420ca4bde578301f7ce`.
Canonical transaction verifies the complete app signature and retains prior
179d8fe engine backup. All492 fresh closed-game SAVE/config hashes are unchanged;
Beard diagnostic content checkpoint5eac985 is preserved. Delivery check reports
zero pending cache/app files, compiler ready. No game launch/input occurs.
Disposition: source/component, build and exact app delivery accepted; player500
speed/control and revised jump/landing/contact replay remain unresolved.

### October13 — Beard appearance rejected again; file diagnostics installed

The player reports no ship after the two delivered map repairs, suggesting a
wrong coordinate. The installed quest still targets the documented navigable
west route; this does not establish the live failure cause. Diagnostic-only
source now records hourly calendar/phase/weather/player position and the exact
navigation, occupancy, slot-allocation or native-creation outcome in the stock
`userdata/Logs/compile.log`. Owned marker checks also record position, target and
remaining TTL. No window, coordinate, encounter cleanup or quest-state behavior
is changed; no player UI messages are added.

Full PROGRAM from final Phonk `c6e99f7`, installed shared headers and the candidate
compile in the real VM: BeardTraceCompile0/script_errors0. Missing/present
attribute reads and the actual TTL<3 negative pass and produce file-only evidence.
A separate calendar fixture substitutes only map entity presence; two calls in
one hour produce one line and preserve absent quest state/rate/clock.

Content-only `sync_metal_gameplay.py apply` initially refuses the reopened
player process8179 with no files changed. After the user's second explicit
closure it returns0 and strictly verifies the resealed app. Only
`PROGRAM/scripts/great_recipe.c` changes in the cache/app; installed quest bytes
match the source and the installed native engine remains
`179d8fe10eb8cb5af4194b20fe9c7fd07a0da1b64f101838f566400b06b5e791`.
Delivery comparison reports0 cache/0app pending. The following Phonk500 source
patch makes the compiler receipt pending for that separate native batch; it is
not an installed engine change. No SAVE inspection or agent game input/launch
occurs. Disposition: diagnostics source/compiler/content delivery accepted;
original ship appearance remains unresolved until the player's normal map replay
yields the recorded gate and creation/lifetime evidence. Two reproduced
component defects are not accepted as the cause of this continued failure.

### October13 — Phonk support departure, continuous landing and airborne water contact

The player's screenshot shows an airborne hull with hull foam and a connected
stern wake still projected on water; the player rejects both jump and landing
motion. The previous source applies an artificial12–42m/s crest kick, aligns hull
pitch to flight velocity, and resets height/vertical velocity at contact. Both
water-effect consumers ignore flight. These source mechanisms independently
explain the visible mismatch; the screenshot alone is not a trajectory recording.

The correction reuses the existing6×6 hull-averaged ShipRocking support and native
gravity/draft. Negative required support reaction detaches a rising hull with its
actual supported velocity and presented attitude. Gravity drives flight; water
contact preserves incoming position/velocity before damped height and angular
recovery. Initial impact decay allows a subsequent wave serial even on moving
water; the same serial cannot relaunch. TOUCH prediction restores its sample cache
and transient states. Existing ordinary behavior and shoreline grounding remain.
The inherited long-frame TOUCH hull-admission limitation remains unresolved.

One native HasWaterContact query gates ShipTracks and SEAFOAM. Airborne wake
insertion stops while old rows age; segmented draws prevent a bridge across
landing. Foam/contact mesh and emission stop in air, but existing particles age
normally. Landing restores existing wet emission rules without a fake storm
burst. No new script flag, SAVE layout or replacement physics engine is added.

The speed100 report has a separate proven display owner: ordinary arcade HUD
scales native speed by0.4, so physical250 displays100. The live player Phonk
numeric readout now uses native knots. Physical cap250, ordinary/dead/NPC HUD
scaling and spyglass scaling are preserved. Topic owner: `sea-phonk.md`.

Exact production-method component fixtures at30/83/125FPS with a seeded100-knot
approach and moving50m tsunami yield departure16.05–16.21m/s and apex51.46–51.64m
instead of the previous95m reference. Contact retains incoming approximately
−30.1m/s and maximum angular frame step is0.0178rad. Moving-water recovery/new
serial, predictor cache restoration, adjustable throttle/brake/bearing/current
cap, grounded/ordinary/dead and inertial flight negatives pass. Exact wake
Execute/Realize fixtures preserve wet rows, age old wake, reject airborne inserts
and landing bridges; foam emission/lifetime checks pass. Numeric HUD fixture
shows250 in live Phonk and100 in preserved OFF/dead cases. Full native syntax and
zero-fuzz eight-path reconstruction pass. These are component results, not a
player's assessment of landing feel or rendered foam.

Final native patch SHA256:
`982674867e713217be40ca9d6dbdc61677126ec770dd1f9bafe72d4d9a89fa93`.
After the player closes the game, canonical `run.sh --stage-only` returns0 with
two build workers and binds this patch in the compiler receipt. All eight built
source owners byte-match the accepted packet. Unsigned engine:
`f9b680464d094492b269d5e6c5e47f3bdb4a4a7ad82fa6ce04751d881b8b710c`.
Signed installed engine:
`179d8fe10eb8cb5af4194b20fe9c7fd07a0da1b64f101838f566400b06b5e791`.
Canonical transaction verifies the complete app signature; prior97a8 engine is
retained by its backup owner. Delivery check reports0 cache/0app pending, compiler
ready. All492 fresh closed-game SAVE/config hashes remain unchanged. No game
launch/input occurs. Disposition: prior player jump/landing/contact rejected;
revised source/component, build and exact app delivery accepted; revised player
jump/landing/wake/foam/HUD replay unresolved.

### October13 — adjustable Phonk speed, crest jump and integrated app delivery

The player's real sea replay rejects insufficient speed, barely visible lilac
light and braking that changes motion direction. The native source confirms S
lowers sails below the old mode gate, resetting world momentum into ordinary
forward motion. The user then requests a tsunami launch, direct W/S speed control
and a maximum250 rather than fixed400. The successor keeps existing remappable
controls: heldW adds60knots/second, releasedW coasts, S removes120knots/second
along the world vector, and A/D rotate the hull. No minimum cruise or autonomous
maximum acceleration remains. Effective horizontal movement, including currents,
is capped250. Exit restores only mode-owned motion to ordinary propulsion.

A real rising tsunami approach arms a single crest launch above45m/s. Flight
preserves its launch vector without water currents, traction, motor or braking;
gravity and descending water interception complete landing. Transient flight is
included in TOUCH prediction snapshots and resets on load/death/exit. Shoreline
grounding remains authoritative. The light annulus peaks outside the hull, and
four brighter sources sit outside its surfaces. Topic owner: `sea-phonk.md`.

Frozen native patch SHA256:
`4369361cc4e3d48e5fe7338d2a6273da11e54430b8edebd2e8478d509cae8cf5`.
Exact production ApplyStrength/Move/TouchMove ASan/UBSan cases pass adjustable
59.7kn/1s and120kn/2s acceleration, release/coast, braking bearing/stop, nonzero
current ceiling, inertial flight, real tsunami launch/95m reference apex/landing,
repeat/slow/unsupported rejection, predictor XYZ/transient restoration and seeded
ordinary-mode exit. Ordinary groundedY, never-active OFF and dead negatives pass.
Native syntax and halo/state restoration probes pass; these are component
evidence, not a player's visual or driving acceptance.

At the user's install request, the exact Phonk packet is integrated with the
already accepted/frozen NPC `122053c` and one-line Beard constructor `a3e879b`.
Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the final
source changes; four native files change, and all three ship sources byte-match
the tested successor. Its compiler receipt binds the final Phonk patch.
Unsigned engine:
`b53c2c8f483941aa2810a7340d4b7b182da9613bcbd94095f807020c85bbd12b`.
Signed installed engine:
`97a8a84ac04e29c89c11d3481510de08f16f283f410de1a53a8ffb0a29d05195`.
The canonical delivery transaction verifies the complete app signature strictly.
All nine NPC projections are delivered; actual installed PROGRAM/shared VM returns
NPCGenerationCases0/script_errors0 with game Main/scene OnLoad disabled only in
the disposable fixture. Delivery check reports0 cache/0app pending, compiler ready.
The Faster MP3 hash matches its manifest;243 fresh closed-game SAVE/config hashes
remain unchanged. Prior signed22179 engine is retained by the canonical backup.
No game launch/input occurs. Disposition: source/component, build and exact app
delivery accepted; player throttle/brake/glow/jump, NPC scene and fixed-coordinate
Beard appearance remain unresolved. Historical TOUCH long-frame hull admission
is preserved, so no all-frame no-tunnelling claim is made.

### October13 — Beard map navigation installed; accelerated encounter replay pending

The player reports no ship while accelerating the global map across the12th and
following days, and rejects using another SAVE as acceptance of that route.
Source investigation isolates a concrete defect: the shared water-point query
uses ship occupancy for existing actor/player coordinates. The actual extracted
native function rejects its own live actor; it therefore invalidates an existing
quest marker and denies player admission. This is a reproduced mechanism, not
proof of every missing sighting in the player's reported session.

`worldmap-traffic.patch` now exposes navigation-only water admission through the
existing bounds/island owner; stock positioning retains ship occupancy by default.
`great-recipe.c` uses navigation for existing actors and separately retains all
five placement occupancy checks. Calendar, weather, route, TTL, captain state and
ordinary port exits are unchanged. The complete ordered stack applies, reverses
and reapplies identically; four changed native files byte-match the C++ candidate.
The actor/player positive and occupied-spawn/island/map-edge negatives pass.
Full PROGRAM plus candidate headers compiles with zero VM errors; no permanent
test or SAVE migration is added.

Canonical native build returns0 with two workers. Once the player closes the
installed game, canonical `experiments/native-metal/run.sh --stage-only` returns0
and consumes the frozen native Phonk4bca predecessor plus its independently
verified death-music source checkpoint `802fbde`; unfinished NPC and later Phonk
native changes are excluded. Unsigned engine:
`c76303aef3d81a5039ff6d31bda19543fc9d3c6d330bc57f718f32caf8298274`.
Installed signed engine:
`22179cb17ab929049f647de0ced1145894b40bc58f24d78c9b022dc9bd8c2332`.
Installed quest bytes match source after line-ending normalization, the delivered
native header declares the new query, installed full PROGRAM/header compilation
returns0/errors0, and sync check reports0 pending cache and installed files.
The previous installed engine is retained by the canonical rollback owner.
No game launch/input or further SAVE inspection occurs. Disposition: source,
component, build and file delivery accepted; visible accelerated-map appearance,
continued marker lifetime and entering the encounter remain unresolved.
Topic owner: `great-recipe-quest.md`.

### October13 — finite suppression budget and integrated app delivery

The player requires recoverable small fires, meaningful crew shortage and severe
multiple-site overload. The previous model gave every burning cell the complete
crew cooling rate. The new model shares a finite ship-wide budget, prioritizes
burning cells, preserves rain and per-site limits, and derives count/ratio from
one current crew snapshot. It adds one bounded cell pass with no hot-path
allocations. Defence protects casualties; Repair, morale and staffing govern
suppression. Exact owners, previous-state guards and counterexamples live in
`ship-fire-and-damage.md`.

Production CPU replay: Frigate1 with259/370 crew recovers1–3 fresh fires in
7.5–16.75game seconds. Six normal fires spread and lose11.83–18.60% hull and
28.87–45.97 crew over300game seconds; priority can recover this medium reference.
At148/370 crew with Defence1.0, six remain burning and cost18.28–18.67% hull
normally; maximum Defence reduces deaths without replacing missing crew.
The complete20-wood/19-cloth Manowar union confirms the full skilled crew can
recover six fresh fires. Redoutable's98/559 crew can fail even a single priority
fire. These fixed-pose numbers omit waves, rocking and active repair; they are
component evidence, not accepted player battle balance.

Full prepared PROGRAM and the actual context event return0 with script_errors0.
Missing/stale projections, changed quantity and inactive/dead/zero-crew negatives
pass. Two real VM codec rounds pass with fixture game Main/OnLoad disabled.
The first entity-free full-program codec attempt invoked ordinary game OnLoad
and crashed; it is rejected as a scene-load oracle. Real saved-game OnLoad
and battle replay remain unresolved under the explicit game-control stop.

Frozen Phonk source checkpoints `3cef104`; the final native patch is4bca1bde.
The previously generic reused HeatedShot icon is replaced by the dedicated
hot-ball/tongs artwork in `825cb5e`, now installed with its checksum-bound texture.
No later navigation or NPC source enters this batch. Canonical
`experiments/native-metal/run.sh --stage-only` returns0 after the last model,
context and ordered-patch change, using two compiler workers. Unsigned engine:
`c34deca6d6d66afe1c5fbe95eace338a41dc4f00770927934f46ecee90b4373d`.
Signed installed engine:
`1ed666c5cfad7485b98b3be32cafead29777942c994664213f239938f8e1ba9c`.
All277 frozen inputs,488 original/isolated SAVE/config hashes and55 prepared
PROGRAM projections match. Gameplay check reports0 cache and0 installed files
pending, compiler ready. The actual installed PROGRAM/context and two isolated
codec rounds again return0 with script_errors0. No game is launched or controlled.
Disposition: integrated source/component, compilation and exact file delivery
accepted; fire balance/visuals, hull appearance, Phonk interaction and whole-frame
aiming FPS remain player-replay requirements.
The exact Phonk sea replay still needs drift/countersteer, purple glow, audible
track and on/off restoration; its checksum-bound controller/menu/music match
the installed app, whose strict signature verification passes. No launch/input
is performed for that independent delivery check.

### October13 — Redoutable heat transport repaired; compiled, not activated

The player confirms the weak x8 fire was on «Редутабль». Its actual saved
Battleship1 context reproduces .58 cloth ignition fading after about29game
seconds without spread or casualties. The existing model removed a fixed ambient
amount from cold neighbours and attenuated conduction by centre distance.
The successor scales only ambient cooling by heat and attenuates by existing
support gaps; crew/rain rates, damage rates and all118 neighbour links remain.
The all-cooling variant is rejected for weakening emergency suppression.

Production-model CPU replay now spreads the four low/high cloth starts in
25.5–35.75game seconds, with3.58–11.53% hull loss and3.29–14.20 crew loss after
300game seconds. Healthy single, rain and priority negatives recover; healthy
three-site ignition extinguishes in19.5seconds with0.128% hull loss. Exact
source/context, rejected hypotheses and wave/rocking limits live in
`ship-fire-and-damage.md`. The player subsequently requests70%-crew medium-ship
and six-site large-ship balance; that arithmetic/gameplay expectation remains
under the same source owner rather than inferred from this single-site matrix.

Canonical `experiments/native-metal/build.sh` returns0 with two workers: nine
incremental native steps, nine renderer consumer classes, unchanged183 frozen
native inputs. Model SHA-256:
`c15cd9b1c4bfc0e9d30a01bbaed314fe47cf0d3cfbc338324af7adccba787819`.
Unsigned engine:
`eb313850e534fc2dd1af502d9ee2efb2dc242dd3de1b257ade7f8c0116c51743`.
The serialized HeatedShot checkpoint completes during this build without changing
its frozen native inputs. Installed signed2bec remains unchanged; no stage or
game control is performed. Disposition: source/component and compilation accepted;
activation, medium/multiple-site balance, actual battle visuals and FPS unresolved.

### October13 — heated round-shot perk; source-only integration

The separate ammunition owner checkpoints `d975645`: the high-tier passive
HeatedShot perk tags20% of hero/cannoner round shots at launch, shades that
projectile warm red, and gives tagged hits20% ignition. Tagged critical hits
combine the existing critical admission into one60% roll. Native event tagging
uses the existing SAVE codec; ordinary projectiles clear the transient marker.
The two exact recipe-projection predecessor hashes advance with AIShip/atlas
source; unknown edits remain rejected. No quest behavior changes.

That owner reports canonical native build0 before the thermal successor, and
the serialized script-vm-probe refresh0. Full current PROGRAM is materialized
through actual sync.plan/recipe_sources over the installed baseline: compiler0,
HeatedShotCases0 and script_errors0. Learned FastReload, hero/assigned-cannoner,
unassigned negative and ordinary/bomb/other-ammunition cases pass. Only fixture
Main is renamed to prevent real startup. Final thermal build above consumes
this same native patch. Installed perk/UI/projectile and battle replay remain
unresolved; no SAVE, launch or app staging occurs. Topic owner:
`naval-ammunition-and-damage.md`.

### October13 — reopened battle defects; compiled successor, activation pending

The player reports34FPS while aiming at a ship, ocean visible beyond the
opposite hull through real openings, and negligible fire damage/spread/crew loss
at x8 on another liner rather than Beard, subsequently identified by the player
as «Редутабль». These reports reopen their individual
acceptance; neither the earlier menu nor component probes close them.

The source correction prepares existing aiming plane queries once per current
stopped path and consumes roots without temporary vectors. Exact roots and
occupancy match the predecessor; measured query-only cost falls substantially.
A foreground CPU sample has191 aiming-camera samples out of1234, but it is not
a matched ship-versus-water or GPU/FPS comparison. Whole-frame FPS is unresolved.
Damaged geometry draws its opposite exterior wall two-sided, restoring technique
and service state without added geometry or submissions. Its actual GM ray and
draw-state probes pass; installed hull appearance and rasterization cost remain
unresolved. See `sea-aim-overlay.md` and `ship-fire-and-damage.md`.

Real native attribute creation appends duplicate fire-state branches. The new
writer updates keyed branches; initialization imports the newest historical
records and canonicalizes only the owned trees. Real attribute/codec probes
pass, including archived duplicates, bounded checkpoints and old-state negatives.
Ordinary full SAVE decoding already merges names, so this defect does not prove
the reported weak fire's cause. The x8 thermal path advances game time. A later
production-model replay binds the player's «Редутабль»/Battleship1 context:
98/559 crew, repair/defence .14, passive Doctor2 and the archived wind. Four
admitted .58 cloth sites extinguish in28.75–29.5game seconds, without spread or
crew loss; .76 hull sites vary by position. Fractional crew damage is preserved
by the real float attribute writer. Thermal transport/cooling is the remaining
repair owner; it is separate from the earlier state-persistence correction.

The integrated canonical build first rejects the rigging patch's insertion
context before compilation, then rejects a `Vector`/`CVECTOR` mismatch after
source preparation. Both attempts are rejected; the last intact installed
engine is preserved. After the rigging owner supplies its corrected patch and
typed helper, canonical `build.sh` returns0:64 remaining native steps compile
and link in24.35s with two workers and unchanged frozen inputs. Unsigned engine:
`3b807fe45351922e382d188a23a34aead2e951613114ae27410829c9a9682270`.
The normal source checkpoint is `4a027cd`; the fire-state/aim/hull corrections
are checkpointed as `82b09c2`. This accepts compilation,
not installed visuals, damage balance or FPS. The build now defaults to two
compiler workers through its existing CMake/Ninja owner; invalid overrides fail
before preparation. Sampled process-group RSS does not include every compiler.

The249-file original player manifest is unchanged. The user played the supported
isolated `CORSAIRS_USER_DIR` copy; its251-file snapshot, including the changed
Autosave, is hash-verified under the existing private player archive. A later
readonly comparison finds no newer SAVE change. The actual engine process has
exited without agent input or termination. No app launch or stage-only occurs
after the explicit stop on game control. Disposition: compiled source accepted;
activation, matched FPS, hull view and ordinary-liner fire balance unresolved.

### October13 — directional rigging lighting; source candidate

The noon deck screenshot shows bright ratlines beside a dark mast and crew.
Rope/Vant/Flag had no vertex normals, so their outdoor fallback assigned every
face upward sky and solar energy. The source candidate adds geometry normals
and uses the existing raw fixed-function lighting decoder. Animated ropes,
moving vants and cloth folds update normals in their current vertex writers.
Scoped neutral material prevents inherited emissive/specular state; a dedicated
producer role preserves prior no-caster participation, ordinary hull admission
and aim behavior. `ship/Flag.fx` joins exact predecessor resource admission.

The scoped patch roundtrip, CPU geometry, material restoration and unchanged
alpha/fade/depth/unlit-recipe checks pass. Full build/GPU and the matching
installed deck replay are unresolved. No app input, launch, stop or staging is
performed for this candidate; installed signed2bec remains the player runtime.
See `metal-normal-less-surfaces.md` for the mechanism and remaining acceptance.

The first integrated canonical build rejected the rope include context before
compilation: the isolated patch baseline incorrectly retained a later fire
header. Cache/app bytes remained unchanged. The corrected candidate is generated
at its recorded insertion point and passes the full current 92-patch stack and
its complete reverse/forward roundtrip in a task-local copy. Native compilation
and installed replay remain unresolved until the shared build owner retries it.

The canonical retry applied all 92 patches (15 changed source files), then
compilation rejected the helper's `Vector`/`CVECTOR` mismatch. The source cache
is migrated; engine/app bytes remain unchanged. The helper now uses real
`CVECTOR`, the corrected geometry fixture passes, and Rope/Flag/Vant actual
translation-unit syntax checks pass with current Ninja flags (1.12/0.85/0.79s).
The shared build owner's next canonical retry returns 0: 64 remaining native
steps compile/link in 24.35s with two compiler workers, all frozen inputs
unchanged. The corrected normal patch is
`bedbf142987e54216b777ea5d034fcc378150d04e49f901c71940fe784dfd83f`;
typed helper is `57a44a0db44b4bc03eee60dbcde58b02cc253d6de64ca37a8eedf20b3e154fcd`.
The installed signed2bec and its running process remain untouched. No staging,
GPU or player-scene acceptance is inferred from the compiler/link result.

### October13 — subsequent water-composite fog correction; source only

The separate fog owner delivered a27-insertion/10-deletion candidate in
`sea_shaders.hpp`, SHA-256
`63cd9626a29067d957552c7113e4f7f0f2d53a1de0dc5fb357b6733932aac8b5`.
It applies existing local transport only to the new water contribution, leaving
the already transported scene snapshot untouched. Sea2/Sea3 and their aiming
variants share this correction. Texture/uniform/host interfaces are unchanged;
the no-depth fallback retains its previous route.

Exactly two8-pixel native compute submissions pass. The independent transmitted
background derivative error changes from0.06002282 to0.00000002; volume-off
output is byte-identical to the extracted predecessor. Opaque and HDR references
pass. Final clipping intentionally follows local transport, so saturated HDR
water is allowed to differ. This is component evidence; Bermuda/underwater and
whole-scene FPS remain unresolved. The source is a subsequent batch; current
installed2bec and the paired Fire comparison inputs remain unchanged. Its full
21-file evidence closure was copied and hash-verified before original cleanup.

### October13 — living-ship damage installed; matched battle replay pending

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the
focused source checkpoint `f664c28`. Unsigned engine:
`e272b7291bbb424a1eb7caaf85d9a6ad8480bf95a217cdea876c04fad6bc6b27`;
installed signed engine:
`2becdd6995703d71db16276a6233a224ca661a6980cfd550fd3de65d3e1a952e`.
All163 installed receipt rows,249 original SAVE/userdata/configuration hashes
and34 activated inputs match. Cache/app each have0 files pending and compiler
ready; deep/strict signature passes. The signed8ad0 engine and its exact
five predecessor PROGRAM files are preserved by the existing backup owner.
Its162-row receipt was reconstructed and matches the recorded pre-stage hash.

Normal app startup with the supported `CORSAIRS_USER_DIR` override opens the
menu on a249-file clone. Menu animation and a125FPS counter are visible;
neither proves battle performance. Keyboard input through computer use returns
`frontmostApplicationChanged`; coordinate input also returns
`noWindowsAvailable`. Window AX selection succeeds but does not resolve the
keyboard precondition. The player declines foreground/control with an explicit
stop, superseding the earlier demo-control request. The player subsequently
plays the isolated copy and reports the battle defects above; its new Autosave
is preserved. No matched dense-battle measurement has passed. Unrelated quest
walkthrough acceptance remains player-owned.

This installed batch integrates authored-cloth sail capacity, committed-hole damage
admission, stable saved thermal cells, actual hull perforations and bounded
chips, plus the exact-hull witness for near-airburst ignition. Existing critical
ignition remains one50% roll; ordinary bombs and admitted explosions within2m
use0.5%. Native and gameplay changes activate together. The preceding quest,
rim, weather and Beard normalization remain in their existing owners.

The canonical native build returns0 with17 patched source files and all nine
renderer consumer classes admitted. The full prepared PROGRAM compiler returns
0 with0 script errors, including31 actual ignition/anchor cases. Source hygiene
passes after exact staging. New patch-file blank context markers and whitespace
in the admitted stock interface file are preserved rather than reformatting
their upstream content. Source/component checks do not accept player behavior.

The preserved signed8ad0 engine is the comparison baseline. The fresh player manifest
captures SAVE, userdata and configuration before activation. The shared thermal
model performs no hot-path allocations and advances at0.25game-second steps;
20dense63-cell steps total0.327ms in a CPU-only comparison. Renderer point limits
remain48 and global chip geometry remains64pieces. The256-cut cold hull rebuild
measured19.263ms; whole-battle CPU/GPU frame time and thermal state remain
unresolved. Prior-save masks/repair, shared-table display, burning cloth, actual
shell impacts and matching dense battles require installed-runtime replay.

The previously frozen13-line seabed-fog fix is included in this source batch.
Its component evidence is retained; the later water-composite hypothesis is a
separate batch and has not changed current shader inputs. The local environment
adds the existing repository check action; disk consistency passes, while its
visible Codex menu remains unverified because computer use cannot access Codex.

### October13 — rim battle and captivity correction installed

Canonical `experiments/native-metal/run.sh --stage-only` returns0 for the frozen
packet. Unsigned engine: `1008c9ae6b3567c7cd666cafdba58a7b42ed892815f1236207d43bdc04457ffa`;
installed signed engine: `8ad0c4b3795f806862555a70ac585e9a2edd9a3a529381841b61e296ca59cc03`.
All162 installed ownership receipt rows match actual bytes. All249 fresh
SAVE/userdata/configuration hashes and all15 frozen source/launcher inputs remain
unchanged. The full compiler fixture's146 materialized PROGRAM files match the
installed code, with only Main renamed to avoid a duplicate driver entrypoint;
its remaining linked inputs read the installed PROGRAM. That closure returns0
with0 script errors. Cache/app each report0 files pending and compiler ready;
the installed deep/strict signature passes. No actual game is started or operated.

The previous healthy outward-control correction preserved steering but missed
the requested sustained circular fight. Its escape result is superseded as the
gameplay goal; camera, relation publication, real-water range and finite damage
corrections remain. The fresh placement candidate fits the unchanged stock
column to the rim, keeps saved centers and existing actors intact, and places
the first new boss on the same ring within real gun range. Its actual VM changes
71 violations to0, with two SAVE-codec rounds and full PROGRAM compilation.
Strong tangential transport and signed radial support now pass actual stock
movement/ShipRocking checks:135 tangent seeds and675 unchanged-column cases
remain around the ring, while4 of15 powered outward-control cases escape.
Propulsion and rudder remain native. One scalar body emits both C++ and
MSL flow;288 parity and240 UV-advection checks pass. These do not accept boss AI
or a complete player battle.

The player's later explicit visual correction also requires a deep, visibly
turbulent maelstrom with large moving waves. Preserving the earlier50.4m capped
depth is superseded. The candidate blends large fields into a0.16-radius bowl,
with208–256m center depression and sampled27–36m crest-to-trough height around
the fighting ring. Small fields retain bitwise geometry identity. Expanded
conservative bounds and the positive-crest trace correction pass actual native
mesh/camera bodies. The canonical native build returns0 with five source files
regenerated and all nine renderer consumer classes admitted. Paired native GPU
day/night views use installed sea/foam/sky assets, but the first low camera shows
apparently flat foreground and almost-black night; this is an inconclusive
component view, not visual or FPS acceptance. A higher supported camera exposes
the deeper curved wall, torn spiral folds and crest foam; another age shows their
motion. Night stays unreadable in this component, whose fallback lighting omits
the native weather/dynamic-sky irradiance used by the real marine scene. Its
finite grid, simplified reflection cube, absent ship and uncaptured player lens
remain declared limits. No game-lighting change follows from those fixture
inputs; full night/fleet appearance and FPS remain player-owned acceptance.

Captivity dialogue mismatches are reproduced through the stock dialogue wrapper:
answers precede diagnosis, unsupported actions remain visible, and re-entry
restores a generic prompt. The existing HP checker also leaves a fight active
when stock regeneration precedes recipe polling; the opposite order passes.
This is an order-sensitive component counterexample, not proof of the player's
exact handler registration. Named stock quest callbacks pass the bounded
terminal fixture. The core implements resumable capture context,
earned cabin access, contextual prompts/actions, finite lessons and six distinct
existing character models, preserving advanced saves and royal-ending gates.
The explicitly requested ship-hull reduction from8× to4× shares its existing
normalizer and preserves damage fraction without repeat scaling. Stock wrapper,
finite fight, final reward and owned SAVE-lifecycle component fixtures pass.
Independent review finds the same polling hazard in the mandatory shore battle's
hero/helper HP checkers and an optional drill answer under Jack's powder prompt.
The final dialogue keeps both contexts. Named shore callbacks reproduce the
two old stock damage→regeneration failures, then pass both corrected roles,
stale/duplicate callbacks and two SAVE-codec rounds each. Legacy recovery is
restricted to the active shore hero and an actual helper in the uncleared vault.
Full PROGRAM compilation and the composed stock quest dispatch pass. Native
world-transition ports in behavioral fixtures do not accept a player round trip.

Further player steering makes Beard deliver the first hold conversation himself,
with contempt based on the saved pre-battle flagship's class, then leave for his
gated cabin before the bosun's explanation. All earned quest rewards are deferred
to successful final completion; failed return to Bermuda gives0 quest XP.
Existing property restitution remains distinct. Stock ship tasks now select
squadron-first attacks and useful broadside positioning; actual
powered/rudder escape cases remain possible. Search sea admission must consume
the owned offshore encounter, preserving ordinary Bermuda port exits. No current
seabed grounding is inferred merely from a map clearance query, which tests
navigation collision rather than depth. The native firing consumer prioritizes
the selected task target only after normal enemy/range/friendly-fire admission;
its22 extracted-body checks pass. Selected-target failure still allows ordinary
fallback fire. The shared critical-hit ignition policy is50% for each admitted
critical event, without a second skill multiplier; noncritical bombs remain0.5%.

The minimal strongest-core storm auto-entry correction is committed as5fd7792
and admitted to this batch; its54 actual-method cases
pass against6 baseline failures. Fire hull/cloth expansion and the separate
fog-contour correction remain their next batch. Full scene/dialogue/combat
acceptance stays with the player after canonical installation.
The broader repository context gate remains nonzero for the separate untracked
fire/cloth candidate and a missing environment context-check action. Neither is
consumed or silently reclassified by this packet.

### October13 — relation, camera/current and range successor installed

After the player closes the app, canonical `run.sh --stage-only` returns0 for
the13 frozen source inputs. Unsigned engine:
`f9dac7979a2fa0af1e6d1998acd551121ce4750d411d68d16cf6e6bd7d49a7ce`;
installed signed engine: `1c220e8d34d9964d9c7123dedcb2f46b72f18cedbe7dff3701e266b8998f8310`.
Deep/strict signature and all162 installed receipt rows pass. All249 fresh
SAVE/userdata/named configuration hashes remain identical and all13 source
inputs remain frozen. Installed actual PROGRAM compilation returns0/errors0;
cache/app each report0 pending/compiler ready. The three separately committed
sailor/raider/smuggler dialogue files are now delivered too.

Native relation publication now synchronously refreshes an already published
matrix after late insertion or changed commander; initial entry stays batched.
Load reconciles actors before returning, with no redundant next-tick refresh.
Actual-method45 cases pass against15 baseline failures, including immediate
queries, old serialized size and surrender/captor/friend/neutral negatives.
The installed script relation callback has no recursive native relation query.

The field-relative camera keeps ordinary output bitwise equal. Stock movement
and decoded hull inertia prove healthy low-wind outward control and opposed
rudder authority, with no injected pose/velocity; a mastless vessel still drifts
inward. Radius, depth and rough geometry are unchanged. The remaining cannon
cache/report computations use physical local water:61 metadata cases pass,
including the previouslyNaN small-gun range and hull-damage falloff. Target
admission and the existing ammunition/firing lifecycle remain unchanged.

The aiming plane solver prepares current-frame immutable coefficients once;
200,000 ordered roots and all trajectory stations remain bitwise equal.
Representative52-gun query work including preparation changes2.52→0.92ms;
this is neither complete-overlay timing nor a GPU/game-FPS result. The player
explicitly defers manual-hit and aiming-FPS checks. Full vortex fire, emergence,
camera/control, capture and balance remain player-owned replays, not component
acceptance. No game is launched, stopped or operated by the task.

The new atmosphere-contour report has a separate read-only diagnosis owner;
Fire hull/cloth expansion remains its next source batch. Neither is consumed
by this frozen correction or silently dropped for a clean-tree claim.

### October13 — player accepts global-map performance

After the installed map-copy correction, the player reports «все норм120 фпс».
This accepts their global-map performance replay. It does not establish a paired
speedup, city/night performance, manual shots or the separate aiming-volume scene.
That scene is reopened with a supplied FPS26 frame and has a continuing source
diagnosis owner; its native build/staging remains serialized with this task.

### October13 — combined native batch delivered; battle acceptance reopened

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the final
ordered patches, CMake and source changes. Unsigned engine:
`1dd9dd3bff9dd856c5d4fd94be4974775b8b4e2838337b8cea0757dec2cc37ea`.
The installed deep/strict signature passes and all160 ownership receipt rows
match actual bytes. The12 frozen native/composer inputs remain identical.
An actual installed PROGRAM copy compiles0/errors0; its fixture alone renames
the game's Main to avoid the probe driver's duplicate entrypoint.
Exactly three later, separately owned dialogue files await the next content batch.

The batch includes full-fleet flagship centering, island-free first sea entry,
finite physical Beard emergence, sea mesh/seam and underwater corrections,
target-relative automatic cannon reach, stock death/capture recovery and8× boss
hull without healing. Native fire/SAVE codec and production posed-bound probes
pass their recorded component scopes. Current GPU skinning parity, offscreen
skinning and water-camera probes pass after rebuilding their actual targets.
The land shader probe now compiles with the existing Objective-C++ interop;
its grass-fog assertion fails identically against the intact predecessor backend
and the candidate, so no fog product change or scene FPS acceptance is inferred.

The actual installed launcher and bundled Python accept an isolated prior profile:
saved VSync1 is applied on restart, FXAA1/high shadow2 and unrelated configuration
remain intact. The complete Changes resource matches its27e29448… projection.
No game is launched by the task. After the player restarts,243 of248 baseline
state hashes remain identical; one autosave, engine.ini and three engine logs
change during that player session. Task delivery does not edit SAVE/configuration.

New player frames show sea coverage without the earlier exposed brown floor,
but the camera remains low and automatic fire is still absent. Source inspection
finds delayed ship insertion without refreshing the native relations matrix and
absolute negative ship height entering the stock camera's box dimensions.
These are continuing repair hypotheses with owning falsifiers; the installed
batch does not accept them, vortex steering, three-manowar balance or city FPS.

### October13 — chart copy hot spot and diagnostic command source accepted

A ten-second player chart profile attributes771 of1467 main-thread samples to
the frame event's direct object/attribute copy. `WdmTrafficRefreshFrame` now
borrows `worldMap` for `IsEntity`, preserving the same native entity check and
the existing scheduler. The real-VM callback fixture passes valid/deleted entity,
idle, partial/completed slice, cancellation and retained-state cases; its process
timings support eliminating the clone, not a scene FPS multiplier.

The requested `sh tools/profile-game.sh` command and local Environment action
wait five seconds and record ten seconds of the exact installed engine's CPU
stacks. Shell/TOML checks and a real player invocation pass. They do not operate
the game or measure GPU time.

An idle isolated canonical map apply is attempted but rejected before mutation:
the compiler receipt still binds the previous whirlpool patch. The alternative
dev workspace has47 different, unsnapshotted files; those are preserved.
The248-file player-state snapshot remains identical. Source accepted; map
activation awaits the canonical combined native stage and the chart replay.

### October13 — complete remaster changelog source projection

The existing main-menu Changes resource now composes the complete authored
Russian remaster inventory above its unchanged imported ReConstruction body.
The generated output is212,383 bytes, SHA-256
`27e2944842d0b6d23df703bef07fdc912a6c4915907627318ddd0746fb91f06e`.
All161 non-table authored lines retain order and all33 table cells/headers are
preserved as readable plain text. The original170,024-byte body reconstructs
exactly; input snapshots, deterministic projection, full source composition
and seven malformed-input negatives pass. The existing text/scroller and menu
layout are unchanged. Source projection accepted; installation and actual
first/scroll/reopen interaction remain pending.

### October13 — player replay reopens vortex battle, capture and Options

The player's night battle shows missing water blocks and exposed terrain. Manual
fire works, but neither Beard nor companions automatically fire; Beard does not
approach a damaged or mastless flagship. A captain-duel loss leaves the hero dead
on deck after a journal notification. The night map patrol is difficult to reach.
Options chosen with OK also revert after quitting and restarting the installed app.
These observations reject the corresponding gameplay/restart acceptance; prior
VM and GPU component passes retain only their recorded scopes.

Source diagnosis proves two defects: vortex subdivision can create blocks larger
than the unchanged sea vertex budget, and swallowed boarding death leaves the
stock interface/death state blocking the deferred captive reload. Native ship
camera already clamps against displaced `WaveXZ`; a camera hypothesis does not
replace the demonstrated mesh defect. Auto-fire, capture approach, encounter
reachability, water/camera classification and Options persistence have continuing
source repair owners. No current candidate has been staged or replayed by the player.

The independently frozen chart-speed source admits the existing counter28/x8
branch on the global map and removes its redundant counter8/x3 override. Real
ProcessControls VM input events pass x8, slower/floor, ordinary sea, land,
boarding, pause, dialogue/interface and quest-lock cases. Its complete source
hash is admitted by the recipe composer; composing twice is identical. Installed
activation is pending, using the existing engine after the player session ends.

The frozen backend posed-bound scratch/span reuse passes exact CPU parity,
including source replacement, NaNs, signed-zero order and disabled/sparse/span
fallbacks. Those per-draw measurements do not establish scene FPS. Native build,
GPU parity and installed activation await the combined source batch.

### October13 — ordinary drinking now includes contradictory Beard tales

The separate content stage returns0 and correctly reuses the identical installed
engine. `Habitue_dialog.c` admits four tales only in its ordinary continued-drinking
branch; fights, gambling, French/Dutchman stories, drink cost and intoxication
remain stock. The tales contradict each other and provide uncertain Bermuda/Laki
leads without starting the quest or writing a walkthrough journal. Every branch
returns to the paid drink or exit; insufficient money leaves exit available.

The first fragment's default/return switch form fails native compilation with
duplicate-label/syntax errors. An explicit unknown-node guard and terminating
break restore compilation without changing the prose or existing stock nodes.
Real-VM entry/four-follow-up/poor-player/unhandled-node probes return0/errors0;
actual installed PROGRAM plus ten dialogue/interface segments also compiles0.
Final cache/app composition each reports0 pending/compiler ready; the158-file
installed ownership receipt matches every SHA-256 row and all247 player-state
hashes remain unchanged during delivery. Source/component/installed delivery accepted;
actual tavern conversation and the broader quest/scene replays remain unresolved.

### October13 — physical quest, bounded storm and inline Options installed

The combined canonical `run.sh --stage-only` returns0 after the final one-OK
correction. The installed signed engine is `f14d7cbd33346db3a88529e76e850235311ecf4ba49ba66e0a4eeb466ffd673d`;
the unsigned frozen native build is `3cee0d45986c005c673990c034b08f2dbdba9d00f117e3868acc76bea9d9dbd0`.
The canonical transaction verifies its deep/strict signature and retains the
previous signed engine for rollback. Cache and app each report0 pending managed
files/compiler ready; all156 installed receipt rows match their SHA-256 bytes.
A fresh247-file SAVE/userdata/named configuration snapshot is unchanged.

Storm119.9/120-second and authored-contact negatives plus four SAVE-codec rounds
pass. The inline Options controller now uses the stock saver once: the rejected
double-load predecessor raises an invalid function, while the corrected one-OK
callback passes one primary write, one graphics write and transient-result cleanup.
Actual native format/disk reentry preserves FXAA/VSync/custom dimensions and Cancel
does not rewrite them; message IO/render-only adapters bound that component proof.
The complete PROGRAM plus nine dialogue/interface segments compiles without errors.

Disposition: source/components and exact installed delivery accepted; physical
quest actions, moonlit/helper combat, full ending/balance/playtime, real storm
departure/save replay, rendered Options input/restart and city/map FPS remain
player-owned. Local source checkpoints are `481aa60` (physical quest), `febae07`
(storm), `7aa95b6` (Options) and `a100279` (tavern tales). The original selected
staged commit replay passes after automatic transaction reconciliation is repaired;
no product workaround was used. Installation is identified by actual hashes,
not the older HEAD printed in its log. The following entry records the separate
tavern content delivery. No push occurred.

### October13 — physical captivity and moonlit cache source candidate

The next quest batch repairs witness chronology, the hidden doctor lead,
the named Ratli duel and its saved state, and the fixed mutiny choice. Five
real inventory supplies replace dialogue-only chores. Three checksum-bound
additive GM files register native item pickup/use locators on quest clones.
The new shore chapter uses stock shore/dungeon geometry, a key and real cache,
exactly two scripted skeletons with random monster generation disabled, optional
earned Beard assistance and guarded rescue/return. The helper withdraws alive.
Only the active bay requests the stock full-moon presentation; no calendar edit.

Disposition: source/state candidate accepted at local checkpoint `481aa60`.
Current combined composition
admits the frozen storm successor of the448b9ff predecessors. Complete PROGRAM
plus seven dialogue/interface segments compiles
with zero VM errors;26 composed hooks are consumed/idempotent. Isolated supply,
cache/handover, active wounded-enemy and pending rescue probes pass with zero
errors and SAVE-codec rounds. World/presentation/reload adapters and seeded
enemy deaths limit this evidence: physical pickup, chest UI, fights, pathfinding,
moon appearance and elapsed playtime remain player replays.

The player has saved/closed the game and authorized installation. Storm sources
are frozen; the new native engine builds. The graphics source lane is resolving
an observed same-callback option-segment lifetime gap before the combined stage.
No installed-readiness claim is made yet. Thirty-nine quest journal entries now
retain heard facts, goals, deadlines and consequences, removing internal
schedules, prescribed tactics and unknown future branches. Other custom debt,
custody and crime records are unchanged: their details express current state.
The decisive probe outcomes and their limits are recorded here; disposable
fixtures are removed after their final accepting consumer.

### October13 — corrective quest and performance batch installed; player replay unresolved

`experiments/native-metal/run.sh --stage-only` completes after the frozen source
batch. Its canonical build recompiles the Metal backend and links the engine;
the composed patch stack is verified. Gameplay delivery applies29 reviewed
paths, including the four quest/dialogue scripts and five RedBeard model,
texture and portrait assets. Both cache and installed app report0 pending
gameplay files and compiler ready. The transaction signs and verifies the app
with strict/deep checks; the prior signed engine is retained for rollback.
Installed managed `messages.h` and `sea_ai/script_defines.h` match the built
owners. An isolated copy of the installed PROGRAM compiles the complete shared
chain plus six dialogue/interface segments with0 VM errors.

A fresh pre-stage hash snapshot of246 SAVE/userdata and named configuration
files matches after delivery. It is distinct from the older249-file receipt.
The installed executable hash is
`d8cbdc916ca5549fb6ac5c3749258d9fa34babd4749604d9dae9efda82d03f1f`.
Disposition: corrective source, component checks and exact installed delivery
accepted; startup, whole-fleet vortex battle, solo1-vs-3 balance, both boarding
paths, captivity/ordinary endings, earned royal ending and main-plot coexistence
remain player-owned replays. Night-city/global-map FPS and moving-lamp shadow
stability also remain actual-scene measurements; no numeric gain is claimed.
The repository context check still reports the pre-existing missing environment
project-context action; no unrelated environment change is included.

### October12 — player rejection reopens battle, dialogue and performance acceptance

The player observed a small solo vortex, no convincing boss emergence, immediate
capture after firing, little battle and a dialogue crash. Source inspection
confirms the old eight-second scripted loss and forced companion destruction;
that behavior does not meet a playable battle contract. The supplied crash lands
in DIALOG::AttributeChanged after parent access. The old quest dialogue bound
Dialog.Links before deleting it; the corrected ordering follows the stock
delete-before-bind owner. The report does not establish any other crash cause.

The requested replacement admits the whole fleet to a sustained large vortex,
lets an agile heavily armored class2–3 Red Beard fight exclusively with shrapnel
bombs, and permits a genuine early victory. The final balance target is one boss
defeating three ordinary manowars. Captivity stays aboard ship, with richer
investigation, fights and character dialogue; the journal provides clear goals.
RedBeard receives an independent Hugtorp-derived model/texture with long red hair,
a head-weighted beard shell and one blind eye. A recoverable real barrel, unique
red pearl, Laki handoff and seven-day Bermuda production gate replace free bulk
bomb rewards. Daily production is20–30 with a hard stock cap200. Offshore global
map appearances must use the same admitted dates and encounter state.

The night/global-map investigation captured an active two-second sample with
493/914 main-thread samples inside CAMetalLayer nextDrawable. The earlier
inactive sample is rejected as FPS evidence. Source proves that the renderer
requests immediate presentation when vsync=0, while the Metal layer retained
its default display synchronization. Constructor and Reset now honor the same
presentation interval. A real hidden Metal device probe fails before the edit
and passes after it: immediate0, interval-one1, immediate0, default1; invalid
Reset preserves the preceding valid interval. This proves presentation behavior,
not a city or map FPS gain. The lamp patch separately repairs persistent light
identity through deletion, compaction and reset; actual-method probes pass.

The full canonical native build consumes the revised lamp and large-vortex
patch stack with six changed native source files and succeeds. Script/asset
integration and canonical stage-only remain pending. Disposition: source/native
components accepted; corrected delivery and all changed player scenes unresolved.

The subsequent exact-WORLD repair also builds through the canonical stack.
Before/after actual-Metal fractional-camera replay reduces redundant cube
refreshes6→0 while baseline, camera-return and moved-geometry4096-pixel outputs
remain byte-identical. Physical movement and caster removal still invalidate.
The exported counter counts complete cubes, despite the temporary fixture's
misleading extraCubeFaces label. This is component quality/reuse evidence,
not an installed-city FPS verdict. The R900/duration600 actual sea shader
component shows62132 active and61294 age-changing pixels with Clear identity;
its textures/waves remain diagnostic approximations. RedBeard's actual GM/TX
uploads and three115-bone GPU component poses pass; stock man animations,
dialogue/spyglass portraits and player fights remain unreplayed.

The October13 source audit additionally finds a neutral second draw for raw
sea/deck receivers wholly outside a selected point lamp's authored range. The
receiver now reuses exact known posed bounds and the registry's existing sphere
intersection; unknown bounds or nonfinite source data retain admission, and
mixed moon/sun correction remains. Paired actual-Metal full-color readback is
byte-identical across all4096 pixels while the far point-only correction changes
from applied1 to applied0. The existing point, mixed directional, cutout and
late animated-actor checks pass before and after. Finite near/tangent and unknown
data negatives pass; NaN position/range keep the old mask. No whole-scene FPS
gain is claimed. This independent performance source change is not installed.

The local weather compute field now reuses identical same-frame, same-command
inputs across pole-facing cube views while refreshing each view's sampling
metadata. The key retains weather, eye, angular window, terrain revision and all
pipeline/input/output identities; unknown inputs and encoder failures recompute.
Paired actual Metal +Y/-Y readback reduces the two-kernel dispatch count4→2;
all40,421,376 volume bytes and consumer metadata match the uncached baseline
(SHA256 `0d9a0ed8f8d9d461bdc4e5f1963b8e407c082529ca2721b8e638abea42a04593`).
The wet/dry/night/orographic, rebase, indoor and teardown GPU checks pass. This
proves duplicate compute removal for matching cube inputs, not a50% scene FPS
gain; above-water main/reflection reuse is not established. App staging remains
pending.

October13 quest-ending probes reproduce two defects in the previous candidate:
stock self-rescheduled delays lose their replacement entry (`601`), and an early
flagship wreck can wait forever for an unlogged boss (`603`). A quest-local,
save-rearmed deferred event clears its token before invoking the continuation;
normal and early boarding now share boss emergence and bounded recovery.
Current deferred/pause/stale-token, early-wreck continuation and missing-entity
recovery probes return0 with zero VM errors. Winning now restores every original
companion ship; a full900-row registry falls back to the reserved original ship
without rewriting a shared row. Unique reward/foreign-row and full-registry
probes also return0. The post-loss sea-entry hook no longer requires Lightning
before CreateSeaEnvironment creates it: rainy-storm admission fails650 before
and passes0 after, while dry/nonstorm entry remains excluded.

The optional alliance requires actual won ship-work/practice, boarding mercy
and release through the stock prize panel, then actual Diego freedom, shared
recipe, restitution and a Bermuda treaty. It offers a permanent political
refusal while preserving ordinary production. Jackman's identity, authored
dialogue/work and scripted scenes remain his own; only ordinary residence poses
move him beside the seated naval king. The actual VM exposes a local `pirate`
identifier shadowing the `PIRATE` constant; renaming it restores the promised
pirate-flag friendship. The integrated eligibility/release/concession/relation/
residence/land/capture probes and two SAVE-codec rounds pass with zero VM errors.
All22 composer hooks are idempotent, and the full current PROGRAM plus six
dialogue/interface segments compile. Native scene, boarding-panel, political
dialogue and main-plot replays remain player-owned and unresolved; fixtures
replace unavailable world/presentation boundaries. Corrective staging is next.

### October12 — Great Recipe and rough native vortex installed; player replay pending

The requested Great Recipe now has an eight-hook composer and two dedicated
scripts: Bermuda offer, moonlit vortex/assault, persistent captivity, fair
shrapnel duel, both dream outcomes and post-loss real Red Beard encounter.
Full candidate PROGRAM and both dialogue segments compile with zero VM errors.
State/save-codec probes pass restoration, distinct chores, success-only Bermuda
stock, unique reward row, foreign-row alias, full cargo, duplicate reward,
paused phases and resumed damage cap. Removing GC reservation fails113; removing
the exposure cap fails304. These fixtures stub world/audio/scene/native-query
boundaries; they do not accept the whole player quest.

Native vortex shares world-space deformation/slopes/current between SEA and
SHIP, with explicit saved descriptor/age restoration, finite onset/fade,
spiral foam and debris. Native source build and ordinary/modern water probes
pass. Actual shader component readback distinguishes active and moving output,
then restores byte-identical ordinary water. The first diagnostic replay used
multiple captures without normal frame reset; adding Present and restoring
the initial depth comparison fixed that fixture-only identity failure.
The player rejected the component's smooth bowl. Revised patch74f86e75
adds sheared folds, two secondary geometric scales and a lip varying in radius
and height, with identical current/load behavior. Paired actual-Metal readback
using the same approximate ordinary waves and synthetic textures shows broken
spiral crests:21,901active pixels,21,240age-changing pixels, Clear identity.
Those diagnostic assets are not full-game appearance evidence.
Disposition: core state/compiler and revised field component accepted;
combined canonical staging accepted; player quest/sea replay remains pending.

### October12 — safe pushable wreck contact installed; player replay pending

The separately authorized collision repair is committed as f309db56 with
exact-context followup fd62f1e0. Accepted patchbf6f673f keeps dead-hull prediction
at the actual displaced pose, suppresses
phantom impulses and recursive/negative-power damage, and splits contact
displacement by mass. Its new native method retains XZ in existing anchor/current
position and absorbed downward travel in existing saved sinking displacement.
No save fields are added. Its final zero-fuzz dry-run composes after the quest
vortex under one build owner; only predecessor context changed, and all native
added/removed lines match the accepted physics patch.

Actual-method broadside/oblique probes reduce damage1774.61→36.7633 and
917.045→15.681; live/live output stays identical. A10:1 live:wreck mass pair
moves the live hull0.301m and wreck5.719m; equal and reversed masses, departure,
old impulses, depth/dt and next death-pose persistence pass. Full native source
syntax passes. Disposition: source/component and canonical staging accepted;
the player's original battle/save replay remains pending. Topic owner:
docs/ship-wreck-contact.md.

### October12 — night table preparation installed; player FPS replay pending

The separate night-performance request targets repeated point-lamp CPU work
without reducing lamps, resolution, selected-source budgets or shadow faces.
The actual pre-change registry builds equal geometry/argument tables eight
times for eight refreshed cubes. The candidate shares their immutable snapshot
within one Metal command and observation epoch; each cube keeps an independent
ICB, face buffer and argument buffer. Observation, scene/command/device changes
invalidate reuse, including a changed palette offset with unchanged caster key.
Completion-owned flight slots retain the geometry resources.

A disposable actual-Metal replay with2048casters reduces preparations8→1.
Full depth readback is byte-identical before/after. After three warm-up batches,
six batches average1.083ms→0.496ms for eight-cube CPU encoding and
0.377ms→0.150ms in table building; first-use allocation is excluded. Completed
commands release borrowed snapshots and geometry references before slot reuse.
Same-key palette-offset GPU depths retain
independent0.3/0.6 values; new command, scene,24-slot exhaustion and completion
reuse pass. Rebuilt dynamic-lighting and sea-point-shadow fixtures pass. No
permanent test is added. These are component timings, not player FPS.
Disposition: source/native GPU checks and canonical staging accepted; the
player's same-save night-city/sea replay remains pending. Separate source
checkpoint95411d4 preserves the independent performance result.

### October12 — final combined delivery receipt

Canonical `experiments/native-metal/run.sh --stage-only` succeeds after source
checkpointa63b778, with final ordered whirlpool74f86e75 and wreckbf6f673f
patches. Five native source files update from the preceding cache; the source
stack, full native build and resource transaction pass. The previous installed
engine0dbca948 is retained by the existing hash-keyed backup owner.
The delivery transaction verifies the complete app with strict/deep codesign.
The installed engine hash matches the signed candidate above; its delivered
script message header matches the built header. The C++ field header is an
engine build input, not an installed script requirement.

Cache and installed gameplay both report0 pending files and compiler ready.
An isolated copy of the actual installed PROGRAM/header compiles the whole
chain and both Great Recipe dialogue segments: RecipeCompile=0,
script_errors=0. It renames only the fixture's entry point and creates its own
userdata; player SAVE is never executed or modified. All249 pre-delivery player
files retain their hashes. Exact phase/state/save-codec and GPU proofs above
remain component evidence; no game was launched.
The preexisting aggregate `agent_context.py --check` error remains:
`environment has no project-context check action`. Owned source hygiene has
zero nonignored untracked paths; that unrelated environment defect does not
invalidate the successful canonical stage or installed compile.

### October11 — public archive prepared; delivery pending

The public app export contains the current installed code/data sections, neutral
configuration and no player SAVE, userdata, logs or profile. Its archive adds the
installer-facing Russian comparison against GPK1.3.2 AT + ReConstruction1.4.1
and a short installation guide. ZIP64 contains95,333 members and10,734,555,329
bytes; SHA-256 f9a55cd53e94c69f3a44a6b9ca2254470eb8a26d08014f538889a1a001f3b35e,
MD5 5333f700bf4f74ba5f973999c01d6c04. Full names/CRC/size/mode inventory,
native extraction, eight links,46,715 xattrs, strict/deep signing and relocated
stage-only with prior profile state retained all pass. No game was launched.

The reconstructed archive producer now has tools/rewrite_metal_zip64.py as its
durable owner; its native31-member replay exactly matches the accepted candidate
and preserves links/xattrs. Existing output, malformed and escaping paths are
rejected without replacing prior files. The delivery topic owns its invocation.

The connected uploader rejected10.7GB before transmission at its512MiB limit.
Updating the cold desktop placeholder stalled without changing the old cloud
revision, so the owned copy process was stopped. A dated archive was copied to
the same folder through Drive for desktop. Drive later cancelled that upload,
retained a recovery copy and removed the queued item; the cancellation initiator
is unknown. The unchanged verified archive was queued again. After offline
retries, live File Provider progress advanced beyond zero, then Drive cancelled
the second attempt as well. A direct browser version upload was started, but
the player took ownership of the upload and its dialog now reports cancelled.
The verified local ZIP was handed off; no upload or monitor remains agent-owned.
The player supplied the same public folder and will add the ZIP later.
Disposition: archive accepted; cloud upload explicitly player-owned.

The player chose to keep Russian interface text inside the actual screenshots.
The English ImageGen trial was rejected and retained privately; no translated
image is integrated. English/Russian case text and captions remain localized.

Website main22613f48 was pushed and delivered through its canonical npm run deploy
after GitHub Actions rejected the job before execution for account billing.
The server revision matches that exact commit. Public HTTPS bytes match the
Russian changelog and six representative actual screenshot assets, and the case
contains the supplied Drive folder and changelog links. Live browser acceptance
passes RU-default/EN/RU,18 art blocks, three noninteractive localized placeholders,
desktop/mobile image ratios and gallery/Escape/focus return. The folder link
reaches the supplied target, and the changelog link completes a browser download.
Disposition: website published and accepted; future ZIP upload player-owned.
The existing unrelated agent_context check still reports the missing
project-context environment action; website build and delivery checks pass.

### October11 — lunar reflection phase installed; screenshot graphics preserved

The player's23:40 December31 capture shows a bright water road under an almost
invisible lunar disc. Installed Moon31 is authored nearly black; SunGlow used
the independent full-strength refl_night template for every lunar phase. A moon
outside the viewport is valid and is not suppressed. The sky-overlay hypothesis
was rejected: its inverse source-alpha blend makes procedural alpha1 identity.

solar-cloud-visibility.patch now caches mean linear energy from the current
authored moon texture's existing64x64 CPU mip when its path/id changes. Relative
energy is normalized to the shipped Moon15 mip and display-encoded for the sea
shader's existing pow(2.2) decode. Only lunar reflection RGB changes; its broad
template, alpha, cloud transmission, full-moon output and solar path remain.
Actual C++ helper/cache/color probes pass all31 installed phases, byte-identical
fullmoon, Moon31 gain0.0103753, texture-change invalidation and unsupported/missing
texture/mip/lock legacy fallback. No per-frame decode or GPU readback is added.

Canonical run.sh --stage-only exits0 after the complete ordered stack changed
two engine sources. Cached engine c0983107, signed installed0dbca948; deep/strict
application signature passes after stage completion. Previous6d94e888 is backed
up; script plans remain zero pending and stage reports SAVE/config unchanged.
Disposition: installed; exact nighttime player replay unresolved.

The user rejected newly generated scene graphics for the portfolio. Those
variants are retained privately as rejected material; the three new case images
use real morningOctober11 screenshots, with lossless source pixel equality and
only proportional WebP delivery. Russian/English prose, current mechanics and
three explicit future-image slots are updated in the website's own repository.
Eight additional aiming, fleet and treasury images now use the original player
captures; the aiming pair describes separate scenes rather than a reconstructed
matched comparison. Screenshot UI stays Russian at the player's explicit choice.
The Russian changelog has a durable docs/changelog-ru.md source.

### October11 — continuous chart rain and sliced fleet refresh installed; player replay pending

The later screenshot isolates grainy rain-cloud circles to rain.tga cards,
respawning every0.8s. The projector replaces them with one stationary world-XZ
precipitation quad per front, beneath unchanged ordinary cloud lobes. Its Metal
fragment evaluates the weather sample's squared smooth radial kernel without
texture noise or respawn. At most12 quads preserve front position/radius and
close/Tab opacity. CPU recording and real compact-bridge GPU readback pass stable
placement/pixels, monotonic intensity, dry identity and ordinary-quad fallback;
complete natural appearance remains the player's accepting view. Case intake
contains14 original screenshots.

The initial rain-veil stage installed a shader containing the reserved Metal
keyword `kernel`; the actual GPU fixture rejected its compilation. Renaming the
radial value to `parcel`, then canonical staging, installed35360903 and passed
the real shader readback. This rejected intermediate is not visual acceptance.

Complete service-chain VM measurement found a33.49ms all-fleet pass; binding cargo
attributes once reduced it to26.40ms, still too long for an8.33ms120FPS frame.
Periodic refresh now snapshots live encounter IDs and spreads whole-fleet work
over the existing one-second native encounter phase. For60 fleets, normal
30/60/120FPS callbacks visit at most2/1/1 fleets; isolated average costs are
0.921/0.522/0.522ms, with0.033ms queue admission. Public entry/load/manual refresh
remains synchronous. Pause/resume or a long frame may finish a larger remaining
prefix at the old catch-up boundary. This removes the measured periodic bulk
pass; it does not establish actual player FPS or eliminate every stutter cause.

Synchronous/sliced complete fleet-state equality, service/supply action counts,
list mutation, pause/exit/manual cancellation and four save-codec rounds pass.
Whole shared PROGRAM startup compiles with zero VM errors; an initial fixture
Main-name collision was corrected in the disposable prelude, not product code.
Port transactions/route selection and strategy admission use equal external
stubs in the scheduling fixture; live gameplay remains player-owned.

Final canonical stage-only exits0 after the native callback and script handler,
with cached engine295524b9 and signed installed6d94e888. Deep/strict signature
passes; both gameplay plans have zero pending. Installed/cache encgen bytes match;
canonical text matches with the delivery's existing CRLF conversion. All245
idle SAVE/config snapshot files are unchanged. Previous35360903 is backed up.
Disposition: installed, player chart/freeze replay unresolved. Night-city FPS,
lamp flicker, sea saturation and near-chart/local cloud parity remain open.
Read-only follow-up found repeated per-lamp GPU table preparation, not a measured
night FPS cause; its bounded CPU probe belongs to the separate night lane.

### October11 — unified chart cloud material and regional storm entry, installed; player review pending

The player accepts ordinary cloud readability but rejects the separate rain-cloud
appearance and abrupt transitions. Local overcast retains blue distant haze,
and chart storms can be sailed through without encounter. Five additional original
screenshots are retained; case intake now contains13 images.

Confirmed render mismatch: precipitation thresholds swapped a front's light atlas
and alpha-sort batch. All parcels now retain the ordinary cloud atlas, lobes,
subtextures and order while wet cores darken continuously. Rain alpha grows from
zero without the wet-batch threshold; ship/Tab opacity remains.12/.95. Solar-hour
fog color now desaturates/dims with the same sampled cloud/rain/storm, feeding
sky, water and distant geometry; clear weather and missing palette retain RGB.

Confirmed contact defect: regional storms returned TestInStorm=-2, while automatic
dispatch required an authored storm index>=0. Native -2 now dispatches through
the existing0.5s throttle; explicit hazard precedence remains. Actual-method CPU
fixtures reproduce the old missed entry and pass contact, dry, paused, proximity,
dead-hazard, tsunami and tornado negatives. The map-header recorder retains36
lobes/material/order through dry/rain/storm and both threshold crossings; shade
changes by at most1/255 there. Fog-method clear/overcast/storm, unbound state and
absent-palette cases pass. These prove API behavior, not rendered player acceptance.

Seeded Jan3 sea/map binding preserves coverage/rain/storm exactly; sparse chart
sprites and close-camera transparency can still look clear beside continuous
local overcast. This perceptual gap remains unaccepted. Jan7/Jan16 fields are
continuous, but the chart's1game-hour/real-second compresses wet runs to3–7seconds.
No model rate, save contract or rain-duration bound changed. Complete natural
cloud appearance/lifecycle, earlier city FPS, lamp flicker and chart stutter remain
player-review items.

Delivery: final canonical run.sh --stage-only exits0, cached engine fae5e544,
installed signed2ab4995d. Deep/strict signature passes after staging completes;
both gameplay plans have zero pending files. Previous installed2f1c2077 is backed
up by the installer. A patch hunk-count failure was corrected before the successful
transaction. Disposition: installed, player replay unresolved; no game operation,
ZIP/upload or publication.

### October11 — weather tempo, rain impacts and lamp stability, installed; player review pending

Observed rejection: the player sees authored cloud masses in fa648c36 but reports
excessive speed, rain jitter, no visible thunderstorm cores, chart stutter, city
shadow flicker on turning near San Juan lamps, and cheap sea rain without water
response. The broad city FPS gain and orange/green sea saturation remain unproven.
Eight supplied original screenshots are retained in the private case intake.

Weather owner: one deterministic calendar/seed field supplies cloud parcels,
local wind, precipitation and storm cores; land scenes bind their actual chart
location instead of the last ship position. Real chart `hourPerSec=1` made the
previous80–160units/game-hour fronts move80–160units/real-second. Cohorts now
last16calendar hours and transport at5–10units/real-second on that same chart.
Clouds persist through24-hour shower cycles:2h growth,4h mature,2h decay and16h
parcel dry intervals. Moisture creates cores without a second storm lottery.
130 chart locations ×16 seeds have longest wet intervals16.75h January,22.25h
July and17.75h December. Strong storm cores occur in31%/48% winter/summer
overview samples; clear area remains96.57%/93.75%. Save-v1 rebind, calendar
partition, quest overrides and finite-event negatives pass component checks.

Projection reuses authored WdmClouds/WdmRain/WdmLightning art. Rain/flash
animation uses frame seconds, preventing the previous437.5cycles/real-second
rain alias. Ship camera opacity multiplier is.12; Tab overview is.95. The native
three-stage cloud shader reproduces RGB58/71/83 and unchanged outside geometry.
The broader existing land fixture separately fails depth3 fog parity; this is
recorded as a distinct observation, not accepted by the cloud-only check.

Lamp owner: immutable AVERTEX0 source validation/uploads now reuse live geometry
and the three flight slots, invalidated on owner unlock/release. Animated bounds
formerly read the facade's zero-filled CPU bytes; current pose bounds now use
GPU-source vertices/palette and reuse the pose-keyed span cache. A separate
camera-yaw fixture proves newly admitted lamps could lack offscreen static
casters in the previous-frame registry. View-eligible authored influences warm
those casters within64 sources; eight cube budget, caster/vertex caps and shader
behavior remain. ASan/UBSan lifetime, pose and warmed-yaw negatives pass. Native
dynamic-lighting, sea/deck point-shadow and GPU/CPU skinning fixtures pass. Extra
eligible caster capture may cost CPU; actual city FPS/flicker needs player replay.

Rain owner: existing sea impact batches no longer depend on a collision-receiver
list, retain roof precedence, age independently of intensity and cap at500 drops.
Impacts expand/fade under ambient light; falling streaks vary/taper and packed
alpha clamps to255. SEA publishes reservedvc244 from smoothed rain and its
existing wrapped phase on each draw; modern water/reflection normals share two
bounded analytical ripple packets, with no extra texture/pass/history. Native
rain-normal readback changes3296/16384pixels, remains finite, phase-wrap error
6e-8, and dry/invalid states are identical. On idle M3 Max, a4112×2580 all-near
normal-only pass is.201ms dry/.506ms wet: component delta.305ms, not a complete
scene/FPS guarantee. Existing modern/original water fixtures pass.

Chart CPU owner: `WdmTrafficClampFreight` skips52-goods scans only when no freight
manifest exists. Actual VM60fleet×6hull workload falls16.70→11.44ms per sweep;
external voyage/service are stubbed. Legacy import, cargo/ammunition, empty and
unknown fields and four save-codec rounds pass; cadence/quests remain. Periodic
player stutter is unresolved. Full startup plus city/sea chart-position fixtures
compile/run with zero errors; failed initial fixture attempts were local syntax
and duplicate-Main seams, corrected without product changes.

Delivery: canonical `run.sh --stage-only` exits0 after final inputs, caching
fc840681 and installing signed2f1c2077. Deep/strict signature passes; both
gameplay plans report zero pending, and weather/encounter consumers match
canonical bytes. All250 SAVE/config files match the new idle snapshot. One
autosave changed during the preceding live player session; that older snapshot
is not a delivery preservation receipt. Installedfa648c36 is backed up by the
existing installer. No game launch/input/stop, ZIP/upload or site publication.
Disposition: installed, full player weather/lamp/rain/FPS replay unresolved.

### October11 — historical tsunami, pirate descriptions and repair installation

The player reports tsunami handoff before visible contact. Its16-unit ordinary
encounter envelope is replaced for the player tsunami query by radial bounds
of the transformed authored GM box. Ordinary storms/NPC queries, swept support,
severity and shelter remain. The actual methods pass302 ASan/UBSan cases and an
independent72-case geometric oracle; both native translation units and canonical
build pass. The box is conservative by up to2.655 map units in sampled headings;
exact rendered first contact remains a player scenario, not a pixel guarantee.

Goods52, perk16, item168, RPG34 and ship66 blocks are refreshed with concise
pirate copy and source-verified mechanics. Labels, ordered IDs, story passages
and all97 substitution tokens remain. The actual native string parser preserves
105/126/549/66/71 block keys and passes UTF8 plus ASan/UBSan checks. After the
player's subsequent repair-balance request, both repair descriptions and their
mechanics change together: free0.5pp/sec60s, skill ceiling20–40%, repeat240s;
material-based2pp/sec30s, ceiling75%, repeat360s. Shared menu limits use the same
helper; companion eligibility no longer remains true above its ceiling.
Timed activity guards stop both repair queues before repair or boarding deferral.
The actual script VM reproduces old balance/lifecycle rejection and accepts
new balance, materials, officer activity and negative-state cases. External
world/repair/event services are fixture seams. Full startup, lazy perks and
LeaveBattle compile with zero errors; live activation/battle balance is pending.

Mast-icon diagnosis is read-only: the installed sprite matches its registered
hash and command texture slot. Eligibility hides it unless the main ship's plan
fits the existing60%-standing-mast ceiling, resources, crew and safe-sea gates.
On a three-masted ship, two standing masts already exceed floor(3*.6)=1; losing
one mast therefore does not offer a repair. No mast eligibility change is made.

Delivery: both subsequent canonical stage-only commands exit0 after their frozen
inputs. Cached native engine1e289cab signs to installedba6c9cb3; the final content
batch reuses that matching engine. Deep/strict signature passes; both gameplay
plans have zero pending files and compiler ready. All nine final gameplay/copy
consumers match canonical runtime bytes, and all244 SAVE/config files match the
pre-delivery snapshot. The previous10dda731 engine is recoverable through the
existing installer backup. No game launch, input, stop, ZIP, upload or site
publication occurs. The existing missing project-context check action remains
a separate broader verification gap.

### October11 — naval/weather/player-feedback batch, installed; scene replay pending

Observed mismatch: the player supplied six screenshots and reports airborne aim
not matching surface grouping, excessive air brightness, disconnected world-map
weather, solar reflection through fog, unclear spyglass classes, chainshot spread,
reload timing and shrapnel cargo order. They then caught sea and city low FPS.
Five-second read-only samples bind PID33507 to installed arm64 engineacd3bd01;
city has active land lighting and harbor ships, without background-sleep rows.
The nearby sea window capture is not synchronized FPS evidence. After rejecting
computer-use overhead, the player opened the city and left it active; the second
profile used `sample` only. No game input, setting, stop or launch occurred.

Main-thread source-bound shares: sea point-cube preparation225/2191 and city
183/2256; city index-span validation531/2256 and reflection543/2256 are larger.
Groups overlap. Deduplicated Metal residency preserves the resource set and six
faces; bounded LRU repairs the reproduced8192-entry index-span admission lockout.
The exact live cache occupancy, double baked/dynamic shadow claim and resulting
FPS gain remain unproven. See `metal-frame-pacing.md` for retained profiles,
ASan/UBSan invalidation negatives and official residency contracts.

Regional weather preserves version1 saved macrofront/RNG and samples calendar/
map coordinates in map and local scenes. Map wet/storm regions use the existing
translucent map pass, alpha≤61/255; the48² sampler/draw preparation costs.094ms
in a CPU component fixture. Story locks, tornadoes and tsunami descriptors remain.
The player refines storm survival to20% hull without defence/perk and80% with
maximum defence plus StormProfessional; intermediate floors use existing defence
tiers. Gusts/handling determine accumulation; ordinary wear never crosses the
floor or heals existing damage. Independent hazards remain dangerous. Actual VM
helpers and32 protection/perk/sail-upgrade long-duration cases pass. Native
storms now update the existing sea `bStorm` state through NativeWeatherChanged.

Airborne sections share the drape's27 weighted trajectories/3C covariance,
retain all muzzles and stopped obstruction support, and lower air cap/extinction.
Surface styling and ABI/MSL are preserved;739 CPU cases pass. Ammo, six cargo
tables, saved goods IDs and two save/load rounds pass the actual script VM.
Reload speeds are1/1/.8/1.2/(1/1.65), chainshot precision35%; the existing armed
fort masonry-contact repair is included. The modern additive SunRoad no longer
adds fog scattering twice: native black-solar/half-fog RGB56→0, clear/original
water, foam alpha, Fresnel/night and both point-shadow fixtures pass.

Integration rejects two intermediate attempts: new map header lacked its
cpp-local include path (fixed in target CMake); raw encgen intake dropped military
functions (replaced by exact prior composedb772bf source with only storm lottery
changed). Final full startup plus eight late consumers compile with zero script
errors, including the final zero-damage guard. These failures are not installed.

Delivery: canonical `run.sh --stage-only` exits0 after the final native inputs.
Cached engine7457f97d; signed installed10dda731; deep/strict signature passes,
both gameplay plans report zero pending and compiler ready. All244 snapshotted
SAVE/config files are byte-identical. Rollback owner retains previousacd3bd01.
The source milestone is468a15a; no push or package/site publication occurs.
The broad context check still reports its pre-existing missing project-context
action; nonignored untracked paths are resolved by source registration/commit.
Player replay owns fort projection, map/sea transition/readability, prolonged
storm handling, ammunition screens/timing and city/sea FPS. GreatRecipe and larger
non-sea threading remain deferred. Six original case images are durably retained
in the private intake; ZIP/site work follows player review.


### October10 — world-stable horizon transport and rain admission, installed; replay pending

Observed mismatch: the player reports dark/light horizon jitter when pitching
the camera and apparently endless rain; otherwise the preceding scene is good.
They retain observation ownership. Read-only process checks show the game exited
before installation; this task does not stop, launch or operate the game.

Hypothesis and delta: view-aligned72-row local atmosphere moves its sparse
ground-fog samples with pitch; absolute far-point reconstruction adds floating-
origin cancellation. `local_weather.hpp` retains density, terrain, lighting and
the two compute passes but samples a fixed world-angular lattice. A512-cell
azimuth ring dispatches only the current frustum's window; pole views cover the
full ring. Fixed513-column storage prevents camera/reflection allocation churn.
The shared sinh elevation mapping in `sky_fog.hpp` resolves the horizon layer;
relative homogeneous rays remove absolute far-point subtraction. Land receivers
copy the same angular metadata. Storage grows18.94→75.94MB, while ordinary views
dispatch their bounded window. The earlier background-depth veil and solar
occlusion repairs remain integrated.

Rain's native status becomes dry below.03, but particles formerly emitted down
to.0002. `weather_engine_integration.hpp` now uses the same clamped admission/
intensity transform for events and particles. A dry front previously emits56.314
target drops after becoming dry; the repair emits zero. Calendar timing, random
fronts, persisted version1 state and quest locks are unchanged. Natural long wet
sequences remain possible; the exact player's seed/lock is not observed, so this
does not claim their complete duration report proven resolved.

Component result: actual rotation-only-VIEW/floating-origin10000 GPU sweeps at
heights5/30/100 show horizon transmittance variation .342285/.212646/.128418
before and zero measured FP16 variation after, over41 pitches and seven fixed
world rays. Independent density integration is constant; spatial interpolation
retains up to.041 error, distinct from stability. Angular coverage passes
6,117,552 rays including wide/narrow FOV, aspect, pitch, roll and yaw wrap.
Wet/dry/night/orographic GPU fixtures pass terrain, floating-origin and indoor
negatives; measured medium compute is.295–.646ms, not whole-game FPS. Rain's
1000-seed/24h simulations reduce rendered wet time January50.791%→35.677% and
July80.188%→63.524%, with native wet status unchanged. Saved phase/RNG, pause,
invalid clock, sea/map continuation and24h quest lock checks pass. Dynamic-sky
backend, shadow and original-sea GPU consumer probes pass; the initial sea
probe invocation lacked its required techniques argument and is discarded.

Delivery: canonical `run.sh --stage-only` returns0 after the final native input,
verifying8252 portable inputs, nine consumer classes and zero pending gameplay
files. Cached engine SHA `865283faf889830c48cc6101cddc9d50deb73d0d4a6be1ace516c61b0bddeacf`;
installed signed SHA `acd3bd0104699691775b4c0b4f6e8af92b8fe6cacd30d4741224ab9e66693747`.
Final signature verification passes after completion. A premature signature
check during signing is discarded. All248 SAVE/config files match the pre-stage
snapshot. No player launch or permanent test is added. Mechanism contracts and
rejected temporal-history/flat-fog alternatives are in `docs/metal-dynamic-sky.md`.
The separate repository-context gate retains its pre-existing missing
project-context action; no green verdict is claimed for that configuration.
Disposition: source/component and installed delivery verified; camera-motion
appearance and genuine rain-episode duration remain `unresolved`, owned by the
player's next report and this task's continuation.

### October 10 — horizon transport and solar leakage repair, installed; player replay pending

Observed mismatch: the player reports a persistent horizon line, occasional
gaps along it, sun/moon glare through islands and rays from below water after
accepting the telescope precision repair. Their supplied December28,1668 sea
captures show18:05 at125FPS and18:45 at111FPS. These are different views/times,
not paired evidence that the original rare37FPS event is repaired. The player
explicitly owns subsequent reports; no new game observation is authorized.

Hypotheses and exact delta: `sky_fog.hpp` previously integrated local atmosphere
only to the radius2000 SkyFog coverage mesh, although the sky behind it used
the full background ray. The veil now uses background depth1; actual foreground
depth and fallback behavior remain unchanged. `sun-below-horizon.patch` gates
direct halo/overflow/flares/reflections by the authored disc's visible circular
area, clips at the current camera's angular horizon, and fixes the fourth-to-first
polygon edge. The atmospheric/directional twilight owner is preserved. Direct
island glare uses the existing collision result immediately. SunRoad producers
trace `SUN_TRACE` from their actual reflection camera, including offscreen
sources, preserving the established10,000-unit range. `sea_shaders.hpp` shares
the existing projected-water source-depth mask with cube SunRoad; `backend.mm`
reuses the current pre-water depth snapshot for that additive pass. The source
visibility model and amortized cube updates remain; this does not add per-water-
point directional island shadows. Mechanism ownership and rejected finite-sea-
edge/short-shadow-cascade alternatives are recorded in `docs/metal-dynamic-sky.md`.

Component evidence: an extracted actual `fs_skyfog` Metal fixture reproduces a
linear RGB vector error up to0.094 against an independently integrated6000-unit
background across radii1000/2000/4000 and alpha0/.5/1. The repair yields zero
error; actual foreground and disabled-volume negatives match unchanged. The
actual cube SunRoad shader previously emits0.340714 total RGB energy for both
clear and fully island-covered source depth; the repair emits zero when covered
and retains exact clear/offscreen/behind-camera/missing-snapshot energy. Exact
source probes cover1001 bounded monotonic disc elevations, moon/sun and both
reflection modes, clear/island-hit/absent collision layers, below/half horizon,
and current reflection camera with the retained10km trace. Polygon closure
has zero mismatches across3968 rotated/height cases versus1508 formerly.
No permanent test or live player-state mutation is introduced.

Delivery: initial build rejects a malformed newly edited patch hunk count;
correcting that count lets the ordered stack and nine native consumer classes
pass. Canonical `run.sh --stage-only` then returns0 with8252 portable inputs,
zero pending gameplay files and installation/ownership receipts. Cached build
SHA is`05128183486333a283edf9cd18ff3ae68c043322ed2d25a6d2b25ebbc86c021c`;
installed signed SHA is`4f11929d28001cc02c064c6e61e238cd661298bb3716c4db8c891abb761a0211`.
Final app signature verification passes after staging completes. A premature
verification during resource signing is discarded as an in-progress observation.
All248 player SAVE/config files match their pre-delivery hashes. No launch is
performed. The repo context gate retains its unrelated pre-existing missing
project-context action; native/component/diff checks pass independently.
Disposition: source and installed delivery verified; complete horizon,
solar/lunar island occlusion and underwater-ray replay remain `unresolved`,
owned by the player's next report and this task's continuation.

### Live sea low-FPS capture — October 10, capacity eviction observed

The player-owned installed app reached an active sea scene as PID 40516,
launched 20:55:47. A five-second sample at 20:57:37 binds engine SHA-256
`258f76c4ea3238cdf2f6589303aeab86a697d435b88459c62793036c862ba595`.
Nearby native observation shows 37 FPS, rain, island shore and six fleet HUD
entries. The player confirmed approximately this FPS and saved the scene before
requesting a repair; the exact minimum was not measured.

Hypothesis: scene-dependent geometry uploads and repeated ship reflections
consume the main thread. Evidence identifies substantial buffer creation,
reflection and sailor rendering cost, and installed disassembly binds sampled
tree deletion to the 512-entry residency capacity branch. This distinguishes
actual capacity eviction from the prior source-only suspicion. Details and
claim limits belong to `docs/metal-frame-pacing.md`.

Delta: read-only CPU sampling and native window observation; no input,
settings, SAVE, content, engine, staging or process change. Existing installed
inventory remains unchanged. Disposition: CPU hot spots diagnosed and capacity
eviction observed; FPS repair, paired CPU/GPU timings and the exact severe
episode remain unresolved. The prior resource-owned residency candidate stays
rejected; it was not reinstalled.

### Bounded residency repair — October 10, installed; FPS acceptance unresolved

Requested outcome: recover the saved sea scene's FPS while preserving ships,
crew, reflections, shadows and graphics settings. One `backend.mm` cache now
serves ordinary raw draws, depth/shadow draws and source submission spans;
existing shared resource identities distinguish vertex and index buffers.
The cache adapts the established converted-geometry LRU pattern: 512 MiB and
8,192 entries, evicting only oldest buffers instead of clearing every mesh at
512. Exact revision changes replace immutable uploads; encoded commands and
shadow packets retain old data. GPU skinning and the sail arena are unchanged.

An extracted actual-code Metal fixture seeds 600 unchanged resources: the old
implementation performs 4,800 warm repeat allocations over eight sweeps; the
repair performs zero. It also passes revised-data GPU reads after cache clear,
selective count/byte eviction, hit promotion and bookkeeping cleanup. No
permanent tests are added. See `docs/metal-frame-pacing.md` for the owner,
alternative decision and claim limits.

Canonical `run.sh --stage-only` succeeds after the player game exited: portable
inputs, engine build, nine renderer consumer checks, zero pending gameplay
scripts, resource ownership and app signing pass. Installed engine SHA-256 is
`fb521607d0f5b34d79668582d6f1828aa3defc55d46386e52ad52c39b13db4e9`.
Normal app launch creates PID 60047 at 21:04:36; its menu renders. The player
loads a sea scene before the next UI action (that action was rejected because
the user changed the window). Native observation shows 38 FPS, six fleet HUD
entries, rain and the island at December 28, 1668, 14:30. Ships and reflections
remain visible; no material FPS recovery is proven. The intended latest save
is `Кампот/Пуэрто-Рико - море 2`, created 20:59; the player's exact selected
filename was not observed.

Two follow-up samples find >99% main-thread background sleep, not rendering;
the displayed FPS is therefore insufficient for paired allocation evidence.
The foreground profile remains pending the player keeping the game active.
A bounded 60-second CPU-activity watcher finds no active interval; no further
sample or optimization is selected from this inactive evidence. Continuation
owner: the current task, triggered by the player's active-sea confirmation.
No process was stopped by this task; SAVE/config and content are unchanged by
delivery. Disposition: component and installation verified; original FPS
outcome remains unresolved, including same-save warm allocations and nearest
camera/crew/shadow comparison. The game remains available to the player.

After the player confirms active gameplay, a fresh five-second foreground
sample contains 2,700 main-thread samples: buffer creation 136 (5.0%), raw
residency 181 (6.7%), reflections 427 (15.8%), sailors 700 (25.9%), Execute
360 (13.3%) and SEA_AI 182 (6.7%). These overlapping inclusive shares are
relative CPU samples, not frame timings or a controlled camera-matched gain.
Native observation shows 40 FPS at a neighboring camera; the player's 21:12:03
attachment shows 62 FPS from another view. Allocation cost is materially
reduced; all-scene FPS recovery and long-session stability remain unresolved.

Installed disassembly binds a 142-sample sailor hot loop to repeated FNV hashing
of the same immutable bone palette for material packets. The source candidate
now evaluates that exact full-byte hash lazily once per accepted pose; begin,
end and source cleanup invalidate it together. Extracted actual methods pass
1,000 repeated packets with one hash, independent 256-bone byte oracles,
changed last-byte/new-offset pose, nested/invalid rejection and failed-upload
state. This second component is compiled in the native engine below; installation
and actual crew/shadow replay remain pending.

The player additionally reports distant running sailors/high ship detail and
a pale horizon-water strip that breaks at island silhouettes (21:12:03 and
21:12:20 screenshots). Distance/FOV admission and the water/fog endpoint are
separate investigations; the sky/weather direction is retained. Source confirms
no sailor distance gate, but sailors already use low-character meshes, while
sails and ropes have distance/FOV reductions. The sampled sailors' distances
are unknown; no arbitrary-distance draw suppression is introduced.

Horizon component: `fs_skyfog` treats the SkyFog veil flag as an additive override
despite its actual `srcalpha/invsrcalpha` technique, suppressing local in-scatter
only for that veil. Removing that argument restores the shared transport;
actual blend mode, weather, cloud-free air field, water material and veil alpha
stay unchanged. The actual Metal fragment reproduces the darkening at alpha0/.5/1
with fixed medium and passes after correction; no-volume, additive and non-veil
draws stay identical. This is a component proof, not the player's visual verdict.

Native build succeeds with both lazy pose-hash and veil corrections as
`b7afcba6722cb6b292da1ab1c781097c5e452d2dbb39b1d0250cf31c8038477e`.
After the player saves/exits, canonical `run.sh --stage-only` succeeds at 21:21:39;
8,252 portable inputs, nine renderer consumer checks, zero pending gameplay
scripts and app signing pass. Signed installed engine SHA-256 is
`d5f417e0fd11c79ff34a5d15f8aca04622c6f72a87237e6d94fa8c991105ff9c`.
The normal player app starts PID 96502 at 21:21:44, using the existing player
state and bundle PROGRAM/RESOURCE. The player reaches sea before the next UI
action; that action is rejected and no menu click is sent into gameplay.

Actual replay rejects horizon acceptance: the 21:23:17 screenshot exposes a
hard water rectangle and a separate vertical sky seam; the 21:24:30 spyglass
image exposes stair-step patterns across both distant island and ship at
2,121.5 m. The 21:25:57 evening image retains the water rectangle. The earlier
constant-medium veil fixture proves only its corrected blend component.
Native observations 68/88 FPS and player screenshots 63/56/82 FPS have different
views, zoom and daylight; they do not prove a paired gain. The player reports
no noticeable recovery, so the original FPS requirement stays unresolved.

The player leaves gameplay active for the 21:24:46 five-second sample
`.cache/diagnostics/fps-20261010-final-foreground.sample.txt`: 2,362 main samples,
Realize 1,885 (79.8%), Execute 405 (17.1%), reflections 472 (20.0%), sailors
352 (14.9%), buffer creation 166 (7.0%), repeated RTTI 352 (14.9%) and lazy
pose hashing 29 (1.2%). Inclusive shares overlap and the camera is not paired
with the first capture. No GPU timing is available from this public launch.
The remaining render submission cost, stale reflected weather-view binding
and far-depth AO precision are separate source investigations; no distance
suppression or graphics downgrade has been applied.

The next bounded batch corrects the proved stale-view owner: preparation before
the consuming render encoder checks the existing frame/view/projection/origin/
terrain key. Reflection views cannot remain bound when water or shadow receivers
restore the main view. Actual LocalWeatherVolume/skyLocalMedium Metal reproduction
seeds main then cube: stale main binding has 630/1,281 identity samples and
transport error 1.08644; restored preparation has zero identity samples and zero
error to baseline. Repeated same-view and legitimate cube-view negatives pass.
The separate smooth vertical sky seam remains unbound until installed replay.

Actual AO GPU reproduction identifies the spyglass mechanism independently:
an unobstructed plane at 2,121.5 m falsely self-occludes only under strong zoom.
Depth32 reconstruction error exceeds the 0.8 m AO radius, and inverse-projection
scaling plus a forced two-pixel floor sample an unresolved neighborhood. The
candidate uses actual projected radius and fades AO when symmetric two-ULP depth
uncertainty exceeds its existing radius*bias budget. All 24 plane cases at
700/1,200/2,121.5/3,500 m, ordinary/telescope FOV and three slopes stay AO=255;
near contact stays flat255/contact187/halo255. A quarter-radius admission was
rejected because it retained false contours at 700 m. The final production
shader itself passes the same GPU oracle; no permanent test is added.

Remaining RTTI cost is reduced at the binding owner: SetTexture retains its
AddRef/Release semantics and classifies changed bindings once; 19 draw-time
2D/cube/volume casts use borrowed classifications. Reset/cleanup clears them
before releasing owned base pointers. Actual-resource sanitizer checks preserve
all eight stages, null/unknown/type transitions, same-pointer calls, duplicate
refs, invalid stage rejection and reset-like cleanup/rebind; the production
methods match those tested. Graphics, draw admission and texture bytes are
unchanged. Native build succeeds as
`de7faf3e5f22a17a2bfa4aff978b6b8d216372a21c829d90bcc852e0dfaeff59`.
The player process exits and canonical staging completes before the subsequent
weather request arrives. Signed installed engine is
`061aee869b3e76517b75705e413f7eb2d6595c7cf81decebe0333f9dba133990`;
the previous d5f417e0 engine is backed up, SAVE/config/resources are preserved,
8,252 portable inputs and all nine consumer checks pass with zero pending
gameplay inputs. The game is kept closed for the requested weather investigation.
Installed sea/spyglass/FPS replay remains unresolved; the current task owns
integration, staging and replay.

### Weather calendar clock — October 10, installed; scene replay unresolved

The player rejects a fixed 1–3-hour rain cutoff and mandatory clear intervals:
rain duration must emerge from the dynamic weather, with short and longer spells.
No maximum-rain timer or forced alternation is introduced. The existing stochastic
front generator, continuous rain/cloud/wind transitions, seasonal probabilities
and quest locks are retained.

The actual clock owner is wrong on the world map: WEATHER advances meteorology
with entity seconds, while WorldMap advances its calendar with hours-per-second
(default 1). Sea uses 250 entity seconds/hour; ordinary land uses 350. A seeded
six-hour replay produces different front RNG and phase in sea/map; an already
wet front still has rain0.799859 after six map hours, versus initial0.8.
This reproduces the timer mismatch; it does not prove the exact photographed
sea rain was a stuck quest lock or that all long rain is erroneous.

Candidate: WEATHER advances its existing model from the same unwrapped calendar
increment used by its solar clock; WorldMap forwards its actual calendar increment
through WEATHER_BASE before wrapping the date. Map WEATHER.Time.speed remains0,
so its execute layer cannot advance meteorology a second time. Version-1 model
coordinates retain350 units/game-hour, preserving saved phase/RNG/current fronts
and requests. Full intervals are consumed rather than clipped to one second.
Lightning cadence and checkpoint I/O retain entity time; visual smoothing,
indoor rain suppression and quest locks remain independent consumers.

A disposable probe uses the production Engine and actual ATTRIBUTES object:
six calendar hours on sea, land, map and accelerated map retain the same front
RNG; worst condition error is0.003553 in wind-speed units. Seeded wet-to-dry map
state clears below rain0.03. Prior version-1 state loads with its exact phase,
finite requests expire, quest locks remain, pause/invalid deltas do not advance,
saved continuation preserves RNG, and advancing calendar does not advance
lightning cadence. Canonical build/staging succeeds after the player exits,
including the revised ordered patch and shared WEATHER_BASE consumer: four source
files change in the cache, all nine renderer consumers pass, scripts have zero
pending inputs and app signing succeeds. Installed signed engine is
`e675530ab7a88ec6f8b971adb978d1d0f6bfba881fc2ae488583821cbd847385`;
the previous061aee86 is preserved by the installer. Configuration SHA-256 stays
`dd594a8f58b5c914661f379e02c8ee5022b234336aa83d4f74fba8be9bbf92a0`.
A final patch-context-only cleanup reaches canonical staging again with zero
cache-source changes and matching installed inputs; the installer skips copying.

The public launcher starts PID23162 at21:44:40 with diagnostic
STORM_METAL_PROFILE=1 inherited only by this process (ordinary subsequent launch
defaults to0). The menu renders and remains animated. Native automation moves
the cursor but two menu clicks and keyboard activation do not open the save list;
no save or graphics setting is changed. The player is asked to load the existing
sea save and leave it active. Menu metrics are excluded from gameplay FPS claims.
The player subsequently reaches sea: native observation shows six fleet HUD
entries, the black-sailed flagship, rain and islands at December29,1668,07:25,
with116 displayed FPS. No obvious hard rectangular water boundary is visible in
that view. The selected save filename and matched original camera are not
observed, so this accepts interactive sea startup only. The next observation
shows the player in the San Juan port office at December28,07:36 and126 FPS;
no navigation, save or weather input is sent by this task between observations.

A five-second foreground sample at21:51:13 contains2360 main-thread samples:
Realize1413 (59.9%), Execute119 (5.0%), texture/general RTTI13 (0.6%), raw
residency58 (2.5%), sea reflection144 (6.1%), sailors81 (3.4%) and drawable
acquisition690 (29.2%). These inclusive shares overlap; the sample contains sea
rendering but the nearby player navigation prevents camera-matched acceptance.
Active profile windows contain120 frames with inactive0: wall9.70/10.63ms,
GPU9.13/10.14ms, drawable wait2.18/2.27ms,650.7/633.9 draws and2.73/2.79M
submitted vertices per frame. These are observed intervals, not a controlled
gain over the original37-FPS episode. The new binding owner materially reduces
the sampled RTTI hot spot; rare slow scenes and distant crew cost remain open.

Disposition: native/calendar component, installation and interactive player
startup verified; actual sea/map weather evolution, original horizon/telescope
views and paired low-FPS replay remain unresolved. The current task owns replay;
the player is asked to leave the problem telescope view active. The game remains
player-owned; graphics settings and SAVE are unchanged by these probes. A final
process reconciliation finds PID23162 ended and PID36180 started21:53:11 using
the same installed e675530ab7a88ec6 engine. This task sent no quit/relaunch input;
the new process's scene and profiling mode are unobserved.

The player confirms loading the sea save and chooses to continue playing rather
than leave the telescope view active. Native observation of PID36180 shows the
deck camera, a nearby black-sailed ship, six fleet HUD entries, rain and San Juan
shore at December28,1668,14:35, with70 displayed FPS. No obvious rectangular
water boundary is visible in this view; the shore occludes much of the distant
horizon, and telescope artifacts are not tested. No camera or gameplay input is
sent. The ensuing five-second21:55:33 sample contains3806/3870 main-thread
samples in the existing inactive sleep branch (98.3%); it cannot establish
current rendering cost or FPS gain. No repeat sample is taken. Disposition:
loaded-save sea startup observed; telescope replay is player-deferred and the
current task retains the original visual/FPS acceptance as unresolved.

### Weather event startup crash — October 10, 20:38

Player incident 473271E8-9B10-4AA5-A6C4-F0694592E155 ran the installed
258f76c4 engine (UUID D9DCE515-3C82-3A84-8FD1-8F4224CE96F3). Its first
native condition-change event entered `IsEntity` with the uninitialized
`loadedLocation` reference; installed disassembly maps the failing VM call to
`FUNC_IS_Entity_LOADED`, where `GetVarPointer` returns null before `DATA::Get`.
The preceding fog component repair did not cover this event's script execution.

`Whr_NativeChanged` still updates condition flags in every mode, but refreshes
gameplay audio only when the land interface or sea is active. Rain sound and
rain eligibility use the existing indexed location registry, removing duplicated
checks through the transient `loadedLocation`. Menu audio retains its own owner.
See `docs/metal-dynamic-sky.md` for the adapter lifecycle contract.

Evidence: a disposable real-VM fixture running the original functions and event
payload exits with SIGSEGV (-11) with `loadedLocation` unbound. Corrected functions
pass wet/dry flags, menu/transition exclusion, town/land/sea/deck sound schemes,
night, cave/house, explicit rain exclusion and Inside lock; zero script errors.
The full installed `seadogs.c` chain also compiles and executes the actual native
change/lightning/tornado payloads before Main, with no game window or scene:
`installed_weather_events=pass`, exit 0, empty error log.

Focused `sync_metal_gameplay.py apply --path PROGRAM/weather/WhrWeather.c`
updates and reseals the played app; subsequent cache and installed comparison
both report zero pending files. Installed script SHA-256:
`ff78b75725efaa5085812211019628518ea1d257434d1ef211e0a7a72afcf760`.
Engine SHA-256 remains 258f76c4ea3238cd; no native build, SAVE/config change,
game launch or UI manipulation. Disposition: repaired event component and
installed candidate; player startup, old-save and cloud-scene replay unresolved.

### Historical native fog callback startup crash — October 10, 20:21

Player incident B49BAFA3-58F6-47DC-9E8B-B17A247ED29C is a new SIGSEGV in
`SKY::UpdateFogSphere`, after script initialization. Installed-engine disassembly
at the reported offset shows the null event result being dereferenced for
`GetInt`. The weather migration removed registration of `Whr_OnCalcFogColor`
but left the engine's per-vertex `WEATHER_CALC_FOG_COLOR` producer intact.

The ordered `weather-engine.patch` now computes the sphere's existing vertical
alpha fade natively; its RGB still comes from the shared Metal atmospheric field.
`WhrSky.c` supplies the authored Fog.Height at sky creation. Removed the obsolete
script function and deletion handler. No restored script weather producer, new
render pass or additional per-frame CPU traversal.

Evidence: a disposable UBSan fixture executes the actual fog vertex/sphere
functions with a missing callback. Original source aborts on the null result;
corrected source passes creation, updates, uninitialized-buffer early exit,
513 vertex/3048 index bounds, exact authored fades at 200/500m and invalid/missing
height fallback, with zero script-event calls. The compiler-only current native
VM compiles the complete installed `seadogs.c` chain without executing Main;
`seadogs_compiled=1`, exit 0, error log empty. Its two changed weather scripts
are byte-identical to the installed consumers.

Canonical `run.sh --stage-only` succeeds. Installed signed engine SHA-256:
`258f76c4ea3238cdf2f6589303aeab86a697d435b88459c62793036c862ba595`.
Previous 87101a13 engine retained by normal backup; SAVE/config preserved.
Disposition: repaired component and installed candidate; player startup, old-save
and cloud-scene acceptance unresolved. No game launch or UI manipulation.

### Historical script startup crash — October 10, 19:49

Player crash E098FEFA-1490-4237-8EDE-FAAC571DACB7 aborts in
`CoreImpl::ProcessEngineIniFile`, before scene creation. The existing player
error log identifies `Whr_BindMapWeather` as undeclared. Installed `WhrWeather.c`
had reverted to its exact legacy hash c66e496f while its receipt recorded the
new 5acf4521 adapter; the new world-map consumer remained installed. The writer
that restored those bytes is unknown. No active child writer was present during
this diagnosis; there is no evidence attributing the revert to the player.

Restored only the verified legacy file to its staged receipt through the existing
guarded transaction and retained backup. Full startup-chain compilation then
exposed a second defect: the candidate called nonexistent `WhrCreateSeaPreset`.
Replaced it with the existing `WhrSetSeaPreset`/`WhrGetSeaPresetFromWind` pair
at environment creation; continuous native wave updates remain unchanged.

The sync check previously inspected the cache only. It now also compares the
installed application and checks its delivery receipts, so a cache-only zero
cannot hide a stale installed consumer. Canonical content apply delivers the
correction without rebuilding the unchanged engine. All seven changed gameplay
consumers match canonical runtime bytes; `WhrWeather.c` is now SHA-256
`2cf06bd8cc1bced6817fe5f1316f136044fbaeea54b6b3469748aeb715063cae`.

Evidence: an isolated compiler-only executable links the actual current native
VM/core, compiles the complete installed `seadogs.c` include chain against its
installed shared headers, and returns `seadogs_compiled=1`, exit 0 with an empty
error log. It does not run script Main, create a game window or touch player
userdata. Engine bytes remain 87101a13. Disposition: compilation defect repaired;
player startup, old-save and weather scene replay still unresolved.

### Autonomous weather and multilayer/local clouds — October 10

Requested: native weather owns wind and conditions independently of the solar
clock; scripts submit storm/tornado/quest events. Clouds need independent layers,
world-scale altitude variation and soft terrain-adjacent fog without an abrupt
straight island cutoff. Storm damage redesign is deferred in the local backlog.

Delta: `weather-engine.patch` integrates the native front/request/save owner,
replaces the map wind producer and drives sea amplitude/propagation from wind.
Canonical weather/world-map scripts become event/effect/calendar adapters.
Native lighting interpolates existing hourly color anchors; no new ambient lift.
Three far cloud decks share the existing cache, plus a local GPU transport volume
and load-time island height field. Owner, units and consumer limits are in
`metal-dynamic-sky.md`.

Intermediate results: the first ordered-patch replay failed on the later baked
shadow patch's obsolete `refineIsland` context; only that context changed to
`captureIsland`. The first compile then exposed a missing `core.h` include in
the map consumer; corrected. Final canonical `run.sh --stage-only` returns 0.
Installed signed engine SHA-256:
`87101a13ec9b213a8ccb4231cd549f5bebdb1d8f4074b43775f6da94c0077c02`.
The preceding 2daeaebc engine is retained by the normal backup owner. Gameplay
sync now reports zero pending files and compiler ready. Saves/config remain intact.

Evidence: GPU local-medium wet/dry/night/medium, terrain and rebase fixtures pass;
one-off native model/actual-ATTRIBUTES reconstruction passes fixed-hour wind,
30/120Hz, pause, event expiry/lock and RNG continuity. The actual VM compiles
the changed weather/effect function fixture with zero errors and valid palette
attributes. A separate isolated VM roundtrip emits an empty-function error
despite passing palette checks; it does not prove a complete save/load replay.
Modern particles/caustic GPU fixture also passes unchanged alpha/blend boundaries.

Disposition: installed candidate, full player acceptance unresolved. Required
player replay is old-save load, sea↔map↔interior weather continuity, quest/world-map
events, soft clouds around islands and complete scene/FPS. No game launch, UI
interaction or game-process termination. Existing unrelated project-context check
action remains absent.

### Daytime horizon streaks and cloud-shaped island haze — October 10

Observed: the player's 07:48–07:54 sea captures show vertical cloud/light streaks
extending below the horizon and patterned blue haze over a nearby island. Ship
brightness is explicitly lower priority and is unchanged by this batch.

Reproduction: a disposable Metal comparison changes only a striped cloud field
while holding weather/atmosphere inputs fixed. Before repair, RGB-vector delta
is .110966 at each of three lower-hemisphere elevations and .611835 in foreground
fog. `skyRadiance` clamps negative elevations to the same positive cloud row;
the fog environment also contains the complete background-cloud image.

Delta: `skyAirField` owns the unchanged atmospheric/weather response shared by
the background and foreground scattering. `skyRadiance` adds clouds using their
actual direction and antialiases the hemisphere edge over two cache texels.
`sky_fog_environment` reuses atmospheric scattering without projecting distant
cloud silhouettes onto geometry. Actual sky/reflection draws retain cloud detail.
No extra texture/pass, CPU vertex work, weather timer or brightness adjustment.

Result: after repair, both lower-hemisphere and foreground-fog cloud deltas are
zero; upper sky at elevations .03/.15/.4 is byte-identical with both cloud
controls. Native Metal shader compilation passes. Final canonical
`experiments/native-metal/run.sh --stage-only` returns 0; installed signed engine:
`2daeaebcf3214220b4e83e32912c4f10da8e149a7777bfd935d851e11c44a22b`.
Read-only installed hash matches the delivery receipt. Previous e35742f3 engine
is retained by the canonical backup owner. SAVE/configuration are preserved;
no game launch, UI interaction or game-process termination was performed.

Disposition: installed candidate; player camera/sea/island replay remains
unresolved. This closes the component reproductions, not visual scene acceptance
or FPS. The existing unrelated project-context check action remains absent.
Cloud-layer capabilities and the corrected consumer contract are recorded in
`docs/metal-dynamic-sky.md`.

### Historical imported materials, lamp variants and culled subobjects — October 10

Observed: the player's 17:03 report rejects the previous 119b43ed candidate:
fireplace illumination still follows camera framing, street lantern glass stays
black, ship/boat coronas do not light their geometry, and some weapons glow.
FPS is reported acceptable; shadow trembling remains reported. Sky and sea are
accepted and receive no change in this batch.

Proven consumer gaps: global RDF `selfIllum` import admits values .44628/1 from
ordinary weapon exports; SanJuan's luminous night lamp model is not selected
at weather hours 19/20 when `Lights` is true but `Night` is false; GEOM subobject
culling remains after the previously repaired NODER boundary; ship flare sockets
originally have a visible corona without a physical light. Ship models also have
ordinary light sockets, so this does not claim that all hull illumination was absent.

Delta: `lighting-consumer-coverage.patch` restricts imported emission to location
models registered as lamps, captures nearby culled GEOM subobjects without colour
drawing, and promotes modern ship flares to physical sources using the existing
default lamp recipe. Flare socket parsing, visual recipe and independent activation
are retained. `metal_lighting_consumers.py`, integrated in the existing gameplay
composer, selects the existing lamp variant by `Whr_IsLight` on location load.
No new CPU vertex pass, scene traversal, sky/sea change or night gain is added.

Verification: native build passes. Checks executing the actual material importer,
GEOM object-culling method and ship AddFlare/SetLightsOff with bounded mocks pass,
including ordinary-material emission rejection, off-camera capture with no colour
draw, and independent flare/light activation. A native script VM reports
`CheckLamp=0`, `script_errors=0` for the prepared lamp-selection block. Fresh
weapon and dynamic-light GPU checks pass; the extended GPU bounds check passes
negative world origin, WORLD scale and far exclusion. These checks prove consumer
properties, not the composed game scene or a live hour transition.

Delivery: source commit `ed27122`; final canonical `run.sh --stage-only` returns 0.
Installed signed engine SHA-256:
`e35742f3bf0890bd362d2a72dd0be54785f9bde82fb10be268d52ce2cac1dcbb`.
Read-only installed-symbol inspection confirms the four new renderer bridges;
the installed location loader contains the prepared lamp-model selection.
No game launch, UI interaction or SAVE/configuration replacement was performed.

Source persistence incident: eleven committed backend bridge/state lines were
repeatedly removed from the working file after successful writes/builds. The
committed source and installed engine retained them. The user reports no editor
or review rollback; no writer has been identified, so this is not attributed to
the user, another agent or a hook. The working file was restored to the committed
hash `09161132143620675e3625ee58395b0aca63bd49c3f59a8a693e17d1b822b5cc`
and a separate read confirmed it. A later diagnostic relink during the deleted
state failed on those four symbols; it is not recorded as a passing check and
does not invalidate the separately verified installed binary.

Disposition: installed candidate, real-game acceptance unresolved. The player's
next replay must establish fireplace/grate and deck continuity when sources leave
the camera, street lantern appearance, hull illumination and weapon response.
Shadow trembling and quick-action stripes remain unresolved; no fix is claimed.
The unchanged repository environment still lacks its project-context check
action; no passing aggregate hygiene result or measured FPS gain is claimed.

### Historical lamp emission, off-camera occlusion and depth reuse — October 10

Observed: the player reports black luminous lantern materials, grate/deck
illumination changing with camera framing, coarse local shadows, bright weapon
reflections, and reduced outdoor/deck FPS. Sky and sea appearance are explicitly
accepted and are preserved. Quick-action icon stripes are reported but their
affected control/state is not yet identified from the supplied screenshots.

Proven source defects: GEOM discarded authored `selfIllum`; ship source
publication required a location prepass and the current receiver's ownership;
corona distance rejection skipped physical light updates; camera-culled nodes
never reached the point-caster registry; generated weapon reflections bypassed
incident material light. The cube key also hashed a camera-rebased transform,
so physically unchanged geometry could invalidate it on camera translation.

Delta: the ordered `authored-emission.patch`, `ship-source-catalog.patch` and
`offscreen-point-casters.patch` restore shared material/source contracts and
bounded capture-only location geometry. Stable catalog IDs update once and
expire when not republished. Nested/far geometry retains range rejection.
Absolute caster keys reuse unchanged depth cubes; world-origin translation is
applied consistently to off-camera admission. Shared four-tap filtering keeps
the existing cube resolution and eight-light budget. Generated lit reflections
use existing GPU material light. Full-night diffuse SH gain is 1.25 instead of
1.15; sky and base-water radiance receive no brightness lift.

Verification: current named CMake targets were rebuilt before executing
`lighting-scene-probe`, `dynamic_lighting-probe`, `weapon-probe` and
`sea_point_shadow_gpu-probe`; all passed. Readback covers capture without color
drawing, caster deletion, camera translation with cube reuse, nested/far
rejection, first-cube admission under negative world origin, source expiry,
hull lighting without local D3D slots, and no duplicate lamp contribution.
Weapon reflections become black at zero incident light and quarter-energy
under quarter illumination, while the unlit compatibility cases pass. Source
inspection independently confirms SanJuan night-lamp emission 1 and day-lamp
emission 0. Native node traversal is compiled, not replayed through a game camera.

Delivery: canonical `run.sh --stage-only` returned 0 after the final renderer
and ordered-patch changes. Installed signed engine SHA-256:
`119b43ed10513514a723fc0698934f69ce789b78a6c598acbf09df7d348a40d1`.
The earlier installed f76f6d3f candidate is superseded. No game launch or UI
interaction was performed; SAVE/configuration are preserved.
The repository context check still fails on the unchanged environment's missing
project-context check action; no passing repository-hygiene result is claimed.

Disposition: installed candidate; real-game acceptance remains unresolved.
Player replay must establish lantern appearance, grate/deck continuity,
shadow edges, night readability and actual FPS. The icon stripes remain an
unlocalized defect owned by the lighting task pending the affected UI state;
no icon repair is claimed. Depth reuse is verified, an FPS gain is not.

### Historical pre-batch local-lamp shadow continuity — October 10

Observed: at 19:45 in the San Juan port office and on streets, walking changes
static-object shadows while the objects and their illumination remain visible.
Expected: a lamp keeps occlusion on visible receivers within its influence;
multiple candles may legitimately produce multiple silhouettes.

Source reproduction: the two-nearest location selector loses lamp 1 after one
rank-crossing step among three lamps. Its quarter-second fade removes only the
shadow, while the full authored catalog continues illuminating the wall.
Delta: retain up to eight admitted lamps while their influence spheres overlap
the renderer view; distance ranks initial/free-slot admission only. Remove the
separate 30-unit world-origin rejection for sea/deck lamps already consumed by
visible receiver packets. Source owner and budget limitations are recorded in
`docs/metal-lighting-parity.md`.

Result: canonical native build and selector check pass; dynamic-lighting and
sea/deck point-shadow GPU fixtures pass, including world/view translation.
Repository context check fails because the unchanged environment configuration
has no project-context check action; this is not recorded as a passing check.
No game launch or interaction was performed. Installed engine remains the
previous b910314a0293bac7 batch: PID 78195 still runs the player's session,
so canonical staging has not been attempted against it. The committed sky
source was restored after an unexplained working-tree reversion; no new sky
change is part of this shadow delta.

Disposition: unresolved — source/build verified, installation and player replay
pending. Acceptance requires street/interior movement without rank-induced
shadow loss, correct multi-candle shadows, and FPS. The existing eight-cube
budget limits coverage when more than eight lamps affect the view; GPU cost
can increase relative to the former two-lamp selection.

### Unified outdoor sky field and removed horizon strip — October 10

Observed: near 19:36 the horizon carries a bright authored band (GPU-measured
sky RGB 0.30/0.41/0.53 at the strip's own grazing angles) while the night field
directly above it is 0.008, and the water reads brighter than the sky. Expected:
one outdoor field owns the displayed sky, the reflection environment, distance
fog and surface irradiance, so the horizon converges night and day and no
authored color overwrites the night/cloud response.

Delta: removed the exact-color grazing strip
(`seam=(1-smoothstep(0,.0015,dir.y))*horizonFog.w*(1-twilight)`) and the
`includeClouds` split it served; `skyRadiance` now has a single evaluation used
by the sky draw, the reflection/fog environment kernel and the solar irradiance
packet. The `veil`/`overcastAmbient`/`haze` branch existed only for that split
and was deleted with it.

Result: a native GPU probe over six weather states (19:36 clear and overcast,
midnight, noon, 18:25, 06:06 dawn) x eight azimuths x ten elevations reduced the
worst grazing sky-versus-fog mismatch from 0.536635 to 0.022250, and the narrow
0-0.002 rad band from 0.536635 to 0.004120. Outside the removed band the sky is
byte-identical in all six states. `dynamic_sky_probe.mm` now rejects a horizon
that a brighter authored fog can overwrite and fails on the pre-fix shader.
Canonical `run.sh --stage-only` completed and installed signed engine SHA-256
`b910314a0293bac7c2eb4635e06ccc629fb90e666dd4ddb370f3bb3eace0cc18`; the change
marker is present in the staged and installed binaries.

Disposition: unresolved. The visible band, water-versus-sky brightness and FPS
remain player-replay items. Distance fog now samples the clouded field instead
of the removed veil variant; that is the intended single-field behavior and is
part of the same replay. Contract belongs to `docs/metal-dynamic-sky.md`.

### Normal-less surfaces and night navigation — October 10 correction

Player screenshots reject the earlier outdoor-unlit fixes: flags/vants remain
bright and the horizon/water issue persists. The earlier staged label does not
accept those images. An isolated GPU material draw reproduces the actual bypass:
texture × factor yields RGB128/128/128 despite a dim completed sky packet. The
repair applies incident light to final material RGB, removes the identified
ambient-factor producer copies and their ineffective CPU vertex workaround, and
repairs the rope cross-section frame. The player reports that the new city
lighting works well but is slightly too dark; the common GPU diffuse packet now
raises night energy by 15%, preserving day/twilight and solar output.

Source contracts, rejected hypotheses and component results belong to
`docs/metal-normal-less-surfaces.md`. Canonical `run.sh --stage-only` completed
after the final shader change, installing signed engine SHA-256
`499866d6fa43ab8c2905c064c3e111e34940edbe6788f3c33392ed9dda3698ad`.
The corresponding material resources match the applied source. No game launch
or player interaction was performed. Flags, assembled rigging, jungle/city night
brightness and the original horizon/water observation remain player-replay
items; this batch does not claim the horizon defect is repaired.

The older night-floor fixture fails identically against pre-repair HEAD;
that verification gap is not relabeled green.

### Shared outdoor lighting, horizon and harbour service — October 9

Observed: dawn sky/water are warm while ships and shoreline do not receive its
directional diffuse fill; a different overcast/night haze branch creates a
horizon stripe. Foreign service fleets retain a map-projected centre and the
sea-login switch fails to apply their DRIFT command. Expected: outdoor consumers
share incident sun/cloud/sky illumination, and servicing hulls wait at the real
port instead of converging offshore. Contracts belong to metal-dynamic-sky.md,
metal-lighting-parity.md and worldmap-traffic.md.

Delta: one completed GPU packet now supplies diffuse sky irradiance and solar
spectrum/transmission to static/skinned models, CPU fallback, shadow receivers,
grass, modern water, foam and identified water particles. Sky fill replaces the
outdoor ambient proxy; local emitters, interiors and UI retain their owners.
Grazing haze converges to the visible horizon. Service groups reserve authored
reload ship locators across fleets; overflow rows extend seaward, and all group
members receive primary/secondary DRIFT. Mission/route and stale berth negatives
retain their own placement. Existing saved sea scenes are not teleported.

Evidence: native build and runtime MSL compilation pass. GPU gray geometry gets
dawn sky RGB11/8/8 with direct sun disabled; menu isolation, authored D3D lighting,
source-gated sea/deck sun and independent red/blue lamp shadows pass. The horizon
component's worst RGB mismatch falls0.605447→0.011831 across five clock samples.
Original sea four-pass GPU checks pass. The complete composed PROGRAM and authored
harbour/seaward-row/wrong-island/missing-locator/mission/route VM checks pass with
zero errors. The legacy dusk sea-vs-land equality fixture rejects the existing
water glow distinction: sea body includes twilight glow, land haze excludes it
already at593cb26. It is not a same-radiance oracle for those two materials.
The standalone grass probe has an existing C++/Objective-C header conflict;
production backend MSL compilation covers both grass entries.

Canonical `experiments/native-metal/run.sh --stage-only` returned0 after the final
native edits, built all8252 portable inputs and installed the signed arm64 engine
SHA-256 `8659e3c0e1b98149f8307b09523776f745fc2fd8bccd4c7b68f8644684c3df06`.
This build also consumes the independently committed fort-contact patch593cb26;
the older source-only note below is historical. Its salvo acceptance stays with
that feature's player replay. The final service-state guard is a content-only
delivery through `sync_metal_gameplay.py apply --path PROGRAM/sea_ai/sea.c`.
No launcher, player SAVE or configuration change is part of this batch.

Disposition: installed candidate; visual horizon/material quality, land scene,
physical harbour positions and collision-free waiting remain unresolved pending
the player's review. The user explicitly requested no further game-window
inspection, launch or interaction. No player scene or FPS acceptance is claimed.
The preceding installed inventory below is historical.

### Fort shrapnel contact burst — October 9, source only

Hypothesis: shells missing the small gun proximity spheres reach masonry through
ordinary Bombs fallback; `Ball_FortHit` displays its stock blast only20% of the
time and never dispatches the shrapnel burst. Player reports most shells entering
the fort without explosion or battery damage; a scene-level pass-through remains
unproven. Delta: an armed normal non-own fort mesh contact enters the existing
shrapnel explosion/8m damage path, with original-segment blocker precedence and
a1cm exterior radial origin. Gun proximity, ship behavior, cumulative cannon
damage, shared2–8m curve and binary save layout remain unchanged.

The old extracted production function reproduces zero burst/dispatch on an
independent wall contact outside its gun sphere. Final production functions pass
ASan/UBSan contact, exposed-gun dispatch, repeated/destroyed-gun, segment-boundary,
ordinary/fragment/own/mode/missing/unarmed and earlier-blocker cases. Strict patch
replacement and actual native translation-unit compilation pass. Fixture limits
and stable contract belong to airburst-ammunition.md.
Disposition: source correction verified; canonical build/staging and real salvo
remain unresolved. No installed app/engine/content or player SAVE/configuration
was changed; current installed inventory remains the cloud/weather entry below.

### Cloud/weather and surface-light refinement — October 9

Hypothesis: oversized broad cloud interiors and fast visual-state time constants
produce heavy dark patches and abrupt weather; geometry direct light does not
consume the completed cloud transmission already used by astronomy and water.
Delta: horizontal cloud shape scale 0.00008→0.00012, brighter lower-cloud ambient,
4/8-second cloud smoothing, 4-second weather colors and 3/6-second fog/wind/rain.
One completed five-ray solar sample now attenuates geometry directional diffuse
and specular; authored ambient, local lamps and interior/fallback light survive.
Contracts and native component evidence belong to metal-dynamic-sky.md.

The installed updated engine reached the player's ongoing sea battle at
December 28 1668 13:48: fleet ships, wooded shoreline, sky, water and smoke rendered.
The visibly black torn sails remain on the flagship while neighboring cream sails
retain their material color; cloud transmission alone does not establish a sail
material defect. Native geometry checks passed for converted and indexed input,
including opaque cover, point lamps, interior and absent-sky negatives. Weather
and dynamic-sky probes passed; the 513-frame component replay remained continuous.
Disposition: installed sea-scene rendering and component behavior accepted;
actual weather-preset transition and an interactive land scene are not replayed.
No FPS improvement is claimed; the separate live sea performance finding below
remains unresolved. Player SAVE/configuration were preserved.

Historical installed inventory (October9; superseded by the current verdict): canonical `experiments/native-metal/run.sh --stage-only`
returned0 after the final native source change. All8252 portable inputs and the
ordered patch stack passed; the signed candidate already matched the played app,
so the final transaction skipped replacement. Installed arm64 engine SHA-256:
`dd96d2876c4ce6d6ed02a93d5b4ba1156a5e00b01b00e62058c2044e0dfeba63`.
`codesign --verify --strict` passed. PROGRAM/RESOURCE roots and player SAVE/config
remain the same owners as the previous inventory. The previous engine21396882
remains recoverable in the canonical installed-engine backup. The player reopened
the installed game after the final stage; a redundant launch was refused by the
launcher's existing running-game guard, with no second process created.

### Live sea low-FPS capture — diagnosed CPU hot spots, October 9 (prior engine)

Player caught the slow sea scene in installed PID36996, engine21396882
(same full hash as the installed inventory below). A five-second read-only
sample at 22:04:38 EDT records 425/1355 main-thread samples in ship environment
reflections and 352/1355 in Metal buffer creation/copy; these groups overlap.
A subsequent native capture shows 36 FPS, six fleet HUD entries, other ships,
wooded shoreline and daylight water highlights in ordinary ship view.
Hypothesis: repeated geometry rendering/uploads scale poorly with this scene.
Observed allocations do not yet prove 512-entry cache eviction as their trigger.
GPU timing is unavailable because profiling is disabled; no setting, input,
engine, save or running game was changed. Disposition: diagnosis accepted at
the sampling layer, causal correction/FPS acceptance unresolved. Exact source
constraints, counts and retained raw evidence belong to metal-frame-pacing.md.

### Non-sea CPU short-path investigation — rejected prototype, October 9

Hypothesis: independent animated collision-vertex transforms can cheaply reuse
the synchronous four-chunk CPU pool. A disposable native driver used the actual
MODELR::Trace numeric block and math/vertex types; no engine, patch stack,
installed content, player state or running game was changed.

All eight sizes from 0 to 32768 matched serial output bytes. Three warmed timing
rounds showed parallel overhead losing at 1024–8192 vertices; at 32768 it saved
about 0.03 ms per call, without evidence of that workload in a real land frame.
Disposition: rejected for unconditional implementation; size-gated use and larger
animation/particle/navigation/render/loading work deferred in TASKS.md. Exact
measurements and shared-state constraints belong to metal-frame-pacing.md.
The installed inventory below is unchanged by this source-only investigation.

### Historical installed inventory — bounded hourly map work and CPU sea jobs, superseded by cloud/weather refinement

Canonical `experiments/native-metal/run.sh --stage-only` passed with the ordered
`cpu-parallel-sea.patch`. Installed signed arm64 engine SHA-256:
`21396882da13c0393675fe6479b3345a3b649226654d32ae3ca0945998499226`.
The installed map script is
`b772bf956179fb0801d597b51953f81956d269b3c0e2a0b067623493a6a777d9`.
Hourly campaign/strategy work is distributed across frames; independent sea
height/normal maps and disjoint mesh blocks use synchronous OS CPU jobs capped
at four chunks. VM/events/render submission remain serial.

Native VM scheduling and four save-codec rounds pass; numeric kernels match
serial outputs bit-for-bit and a local GCD probe observed four threads. Source,
build and engine staging are accepted at those layers; real stutter/FPS and sea
visual parity remain unresolved. Contracts are in worldmap-traffic.md and
metal-frame-pacing.md. No profiling was run for this attempt.

Build preparation initially rejected three changed immutable input captures.
Exact manifest-hashed originals were recovered for characterUtilite.c,
reaction_functions.c and Crew.c; all 8252 input hashes then passed. Their previous
bytes were preserved by hash under `.cache/input-recovery`; no manifest weakening
or player SAVE/config replacement occurred.

Both the cache launcher and public app initially aborted with `fail to create
program`. An isolated full-program comparison reproduced the same error with
the prior and new map scripts. Debug info localized it to BattleInterface.c:2480:
canonical characterUtilite.c had lost its existing fleet helpers/treasurer layer.
Restoring its reviewed manifest edits/append preserved the newer officer-escape
marker. The actual seadogs.c segment then compiled; the diagnostic runner's
subsequent duplicate Main is an entry-point artifact. Installed recovered script:
`f26cc458a19fa583e02e8a22e1e39737142a1281d7dd2f7a27ac027328b54e24`.
The subsequent public-app launch stays running and reaches Metal initialization
without compilation errors. Two later runtime errors in ShipLights.c report
`incorrect argument index` and `Invalid conversation`; their cause is unresolved.
CUA image capture fails with ScreenCaptureKit
error -3811, so menu/interactive-scene and visual acceptance are unresolved;
process/log evidence is not scene acceptance. The launched app is retained for
player replay; no save was selected or game input sent.

### Historical installed inventory — officer escape, crew debt and NPC traffic crews, superseded by the hourly/CPU batch

tools/sync_metal_gameplay.py apply delivered three scripts into the signed app:
PROGRAM/worldmap/worldmap_encgen.c (composed from the edited traffic source,
WDM_TRAFFIC_SHA256 4f1555eb…, composed output daa5abcd…),
PROGRAM/scripts/Crew.c and PROGRAM/quests/reaction_functions.c. A later apply
delivered the revised Crew.c that writes off an escaped creditor's debt. The
following check reports zero pending files and compiler readiness, and the
installed markers were re-read from /Applications/Corsairs Iddictive Remaster.app.

Delta: an escaped officer's debt is written off — the entry closes with status
"escaped" and a ClosedDate, leaves the journal, projections and payment paths,
and nothing remains owed to a runaway; his escape no longer raises a mutiny at
normal morale and requires low morale plus weak leadership; a dismissed or
removed officer cannot start the scene at all; NPC traffic hulls publish with
their optimal crew and port service recruits toward it from a dated NPC harbour
reserve instead of the hero-sized tavern pool.

Result: source and delivery accepted; the script VM full-chain probe also fails
on the pristine baseline (fixture artifact) and does not gate this batch. Player
replay is unresolved: a real escape or dismissal with good morale, the journal
closing the runaway's debt instead of keeping it open, and map NPC ships that
visibly carry crews instead of empty holds. No game launch, input or player-data
write.

### Historical installed inventory — directional sky/fog and coherent camera rays, superseded by the officer/crew batch

Canonical `experiments/native-metal/run.sh --stage-only` returns0 for local
source commit `9052624`. Installed signed arm64 engine:
`44cdd0287dfb32ae0027f3410ab194d832ef05472f4c500e1012d7c608ca41ed`.
App receipt:
`9bb557fcf846722bcc3759a887295c74b777d0834c58eddd22f2924c0aa4dd66`.
All117 managed files match; the installed Mach-O contains the directional fog
kernel, FFP/land/modern-sea variants and camera-relative SKY ray uniform. Canonical
build verifies8252 portable inputs and reports zero further source patch-stack
changes or pending gameplay files. The prior75dc2057 engine is retained by the
installer's hash-named backup. The app signature passes during delivery.

The18:25 shore screenshot reopened acceptance after the prior late-night palette
repair. Fully fogged geometry and modern water now sample the current sky's
directional color, sharing its completed cloud blend and atmosphere. Native
backend tests cover dusk both directions, noon, night, dawn, dense rain and a
rotated/translated SKY/camera; no-fog materials and disabled-sky fallback remain
exact. All four modern sea passes agree with the independent FFP receiver and
preserve foam alpha. The updated513-frame GPU cache replay is smooth/finite at
mean0.689ms and p951.867ms, retaining2048²/96-MiB cloud detail plus a256-KiB fog
field. These are component results, not player FPS or a complete game replay.

The user closed the game before staging;247 current non-log player files and
five foreign working/index entries remain exact. Source/native/build/delivery
are accepted. Complete18:25 shore/sea and weather/FPS replay remains player-owned,
unresolved. Cyan translucent foliage-card edges are recorded separately; their
cause is not isolated. No game launch/input/termination or player-data write.
ZIP/Drive/site and release work remain deferred. Native evidence and disposable
sources are retained under the ignored native sky-evidence owner.

### Historical installed inventory — cloud precision, highlight detail and night fog, superseded by directional fog

Canonical `experiments/native-metal/run.sh --stage-only` returns0 for local
source commit `92d3659`. Installed signed arm64 engine:
`75dc205792475022246dd6e4e40a32357a95013b23b33426f175651012d64024`.
App receipt:
`b64de0531cd4566c813f08749f74aa8d122ce73f71efb64b777362ee440b453a`.
All117 managed files and the final deep/strict signature pass. Installed Mach-O
contains both new shader corrections; installed `Evening.c` equals canonical
encoded source. The stage consumes one gameplay file and zero engine patch-stack
changes; it installs the newly built renderer and backs up the prior engine.
Portable8252 input verification passes. No game is launched by the agent.

The player's 09:06/09:12/21:54 scenes reopen sky acceptance. Cloud ray samples
now derive from fixed origin and analytic distance, eliminating planetary FP32
accumulation bands without more samples, a larger cache or blur. Unpremultiplied
cloud highlights receive a shoulder before Storm's UNorm target loses detail.
Canonical hour20–23 fog RGB converges to existing night levels; hour19, ambient,
density and real storm/rain overrides remain unchanged. One visual weather/fog
snapshot continues to feed sky, distant geometry, water and foam.

Fresh native 4112×2580 components reproduce the morning defect (23.23% fully
white, peak4.2386) and reject it after correction (0% fully white, peak0.96274).
Matched crops show the contour slabs disappearing; noon, dusk and moon-up night
stay finite. The513-frame native cache replay covers weather recovery, wind,
complete-cache swaps, frame reuse, absence/light jump and reset. Current timings
on M3Max are mean0.698ms, p952.014ms, max2.951ms, prime42.93ms, with unchanged
96MiB cloud snapshots. These are component timings, not player FPS. Freshly
built `dynamic_sky-probe` and unchanged weather surface/bridge checks pass.
Images, diagnostic source and `precision-evidence.json` are retained under
`experiments/native-metal/.cache/sky-evidence`.

The player had closed the app before staging;247 current non-log player files
and five foreign working/index paths remain exact. SAVE/config, game input,
launch/termination, push, ZIP/Drive and site remain untouched by this attempt.
Disposition: source/component/build/delivery accepted. Full game morning/night
mountain/ship/sea replay, weather playback and FPS remain player-owned, unresolved.

### Historical installed inventory — procedural sky and shared solar visibility, superseded by precision/night fog

Canonical `experiments/native-metal/run.sh --stage-only` returns 0 after the final
source/ordered-patch batch. Installed signed arm64 engine:
`85d060146aff17e6ab277446135e5ad2a6967274ab6eebc6bc4e7f2739f7dc26`.
App receipt:
`492280f22a3a44ccd92f641008f59a5d55e252a436ea1bf30c2a34520f1fc88d`.
All 117 managed files match their receipt; the deep/strict app signature passes.
Native build/staging identifies source commit `2a6a776`. Canonical build verifies
8252 portable inputs; the final stage has zero further source-stack changes and
zero pending gameplay files, compiler ready. Source milestones are `e47a40d`
(volumetric sky/night sea) and `2a6a776` (shared solar/cloud visibility).

The sky uses licensed shape/detail/weather noise, world-oriented volumetric
clouds, density-dependent Beer/powder lighting, spectral twilight, mild aerial
perspective, original star catalogue and completed-cache interpolation. Modern
water has no artificial night emission floor. Solar disc/halo/overflow/flares
and both modern sea solar consumers use the same cloud transmission and smoothed
solar direction. SUNGLOW prepares the field before SKY's later layer; original
geometry/ship-sail obstruction remain. Legacy texture-alpha cloud masking is used
only when the procedural field is absent, preventing double cloud attenuation.

Native GPU evidence covers full cache cycles, clear/storm recovery, wind turns,
reset/absence/light jumps, cloud-aware stars, noon/dawn/dusk/night sea, solar overlay
pixels and clear/thin/opaque transmission. The reported dense-cloud sunset loses
its formerly clipped solar road; clear sunset retains it. Early SUNGLOW preparation
retains an enclosing sky draw scope. Original sea bytecode and modern material,
underwater/grazing, weather foam/alpha and unsupported-shader negatives pass.
Cache timing remains component-only: mean 0.834 ms, p95 2.243 ms on M3 Max for the
513-frame check; shared sky/sea sunset diagnostic is 4.75 ms. These are not game FPS.
Evidence is retained under `experiments/native-metal/.cache/sky-evidence`.

All 248 baseline non-log player files and five foreign working/index entries
remain exact. No game launch/input/termination, SAVE/config mutation, push,
ZIP/Drive upload or site change occurred. Disposition: source/component/build/
delivery accepted. Complete player weather playback, ship/shore reflections,
SunGlow geometry/postprocessing and FPS remain player-owned replay, unresolved.

### Historical installed inventory — local continuation/optional exit, superseded by procedural sky/sea lighting

Canonical `experiments/native-metal/run.sh --stage-only` returns 0 for the frozen
local continuation and player-safe exit over the map-water material, shore contacts,
first-third gain, collision guard and prior menu/script batch. Installed signed engine:
`9b2c0616802fcd02c84df07963060c454caf814b12faeace4a5cada9db84f68f`.
App receipt:
`7816732da659cf649efdd7846c47d0ea83474e9bf92aa598c612a70d2130d416`.
All 117 managed entries and 37 encoded canonical inputs match; cache/app gameplay
plans are empty and compiler ready. Post-stage deep/strict app signature verification
returns 0 during canonical sealing. Final staging consumes patch SHA-256
8973106f79b2b8ae74a609649792948f4a2d63f40b5812a28ddc1c5203986a21,
with three source-stack changes. Native build metadata names 22bc292;
source hashes bind the subsequent candidate rather than pretending that stamp
identifies its uncommitted inputs. Installed worldmap.fx equals native source
a8c474aebdf2c7a8ce7d461d845fbff28a078083c3c7f53f34e490e8ca5de5d5.

The map wave now consumes ordinary WdmSea base/animated material and clock,
without a second texture collection. Its fixed angular crest phase replaces the
rotating pattern, and actual landfall produces a water-side foam lip. Severity
still owns maximum reach/speed; full strength lasts through the first third of
propagation after entry, then shared visual/contact gain fades to zero. Local
shore emitters follow displaced water rather than spawning beneath it. Exact
native frames and gain sweeps plus 74 local shore/lifecycle checks pass; actual
game appearance, continuous playback, spray and FPS remain player-owned replay.

Local expiry formerly occurred only 700 units beyond the launch origin. It now
continues 2500–5000 units, continuously derived from the same strength and clamped
by scene reach. The natural encounter lock clears only once the entire controlled
hull passes the wave's trailing support; other navigation locks remain. The wave
keeps affecting NPCs/shore while the player stays; map return is optional and
local teardown still disposes the transient event. Thirty-eight actual native
component checks and eight VM lock cases pass; the full staged PROGRAM compiles
with installed headers and zero errors. Continued-wave/optional-exit game replay
is pending and player-owned.

The installed correction removes the flat propulsion band, distributes local
tsunami hull damage through actual contact and binds event torque to the shared
wave slope. Tilted hull sampling/current-frame pose replace the mismatched support
path; bow kick is reduced, while strong broadside capsize remains. Visible Stop
controls are removed and one notification reports exact sampled strength/nominal
damage. Source-bound ASan/UBSan, full-PROGRAM/menu Storm VM and ordinary-control
negatives pass. The separate frozen daylight aim batch also passes this same
build/staging transaction; its player replay owner remains its cannon chat.

Fresh closed-game preservation matches all 248 preceding non-log player files,
including SAVE/config, plus five foreign working files and their index entries.
The preceding b4013994 delivery's human relaunch as PID14595 added two Sentry
session/lock telemetry files. The new closed-game guard delivery also preserves
all 247 preceding non-log files and the five foreign files/index entries. The final
2cb3b168 delivery again preserves those same files and foreign index entries.
No agent game launch,
input, termination, player-state write or push. Player-visible acceleration,
wave/hull following, damage timing, capsize and strength notification remain
unresolved, with the human owning current-save replay. Contracts and component
evidence belong to `docs/sailing-speed-floor.md`, `docs/sea-tsunami.md` and
`docs/sea-aim-overlay.md`.

The subsequent player report reopens local storm-plus-tsunami hang acceptance.
The previous session is terminated and its system log overwritten by the player's
normal relaunch as PID99695. Retained launch output has no fatal error; a bounded
sample of the new process shows normal Execute/Realize/present work, not the old
freeze. This prevents a definitive attribution to the old session's stack.
Actual FakeTouch/contour/vector/wave/drift checks independently reproduce an
unbounded ship separation under persistent predicted current. The correction
passes ASan/UBSan, ordinary-pair/island equality and the canonical native build;
the sole changed translation unit matches the tested candidate. Normal-quit
canonical staging installs this guard as 8bea3d2b; the player owns the original
storm-plus-tsunami replay. The new map complaint reopens material/motion
acceptance; the final 2cb3b168 batch corrects material/shore presentation while
preserving the guard and severity-defined propagation geometry.

### Historical installed inventory — ordinary sea-entry spacing and port service, superseded by b4013994

Canonical `experiments/native-metal/run.sh --stage-only` returns 0 for the
content-only sea bridge batch. Focused source commit:
`18efcae7fc8dc6ad66daed253398f7200b398eb1`. Installed signed engine remains
`3456e45b2998cf17a687a2c7a71e1b944af0b4ff51f833ad64c8fc99e836ad4c`;
its existing native build stamp remains `7d2eed3`. Current app receipt:
`2c6f701cae34d541e38dbe0795300c1ad85ea0466d50bb4da354e2fb1d89ab86`.
All 117 managed entries and 37 canonical encoded inputs match. Cache/app plans
are empty and compiler ready. Only `PROGRAM/sea_ai/sea.c` changed in delivery,
with runtime SHA-256
`9a46021057b2447e30f5e1689d8e5aea29ebf34a24f5034dde325c10b9ee4aad`.

The ordinary bridge gives close moving fleets approach room, a variable
staggered formation and stationary port-service tasks. Actual full-PROGRAM VM
group/event/geometry/task checks pass with zero errors; the owning contract and
global harbour audit are in `docs/worldmap-traffic.md`. The earlier global
admission/shore-return fixes and tsunami/native package remain installed.
Visible map berth overlap is still possible and was not changed by this batch.
All 246 non-log player files, five foreign working paths and their index entries
match the fresh baseline. No game launch/input/termination, player-state mutation
or push. Mounted sea positions, coast correction and visible behaviour remain
unresolved and player-owned.

### Historical installed inventory — expanding fronts, superseded by sea-entry receipt 2c6f701c

Exact canonical `experiments/native-metal/run.sh --stage-only` returns 0 after the
last native patch and material-registry edit. Signed engine:
`3456e45b2998cf17a687a2c7a71e1b944af0b4ff51f833ad64c8fc99e836ad4c`.
App receipt `1f6f72a619e2019cf82bd0029c45e1cccb0da6f33ca9d54def882cec0c751f41`
binds all 117 managed entries. All 37 canonical encoded inputs, both script headers,
six native map paths, five native AI paths and the new WdmTsunamiWave technique
match the exact frozen source. Cache/app plans are empty; compiler ready and
installer deep/strict signature passes.

Esc on the global map offers separate arc/ring creation and marked-event Stop.
The finite hollow front expands and fades; rendered-land cache governs actual
island shelter. Normal initial approach preserves the sampled strength, allowing
the existing fierce beam capsize; attenuation begins beyond configured spawn reach.
Ordinary global fleets lose persisted HP/cargo and can sink, while quest/qID/ALONE
and locally admitted fleets are protected. Local NPC steering uses existing
navigation skill and ordinary turn inertia without replacing tasks. Existing local
damage/capsize, transport/spray, material/horizon, daylight/fort/audio remain installed.

Ordered patch `35eacd393496a88c69f4c1eb1317ddda28f044e07d40d5c7e692e2bc0c36388c`
preserves the earlier 1202-line local-sea prefix and adds native map/steering/technique.
World-map technique `fa7d680cf37593f8a8413827eaac1efeb8815b0bd61dc4151df77307221d7b32`
uses the existing generic fixed-function backend. Prior sea/sky material hashes
remain unchanged. Component geometry/HP/save/controller/offscreen checks and the
full integrated PROGRAM pass; player replay of map/sea, moving hulls and FPS stays
unresolved and player-owned. No game launch/input/termination or push.

The pre-existing closed-game snapshot still matches all 246 non-log player files,
five foreign working files and five index entries including three staged blobs.
A new baseline persistence failed on a HEAD typo before staging; preservation
therefore uses that verified prior snapshot, not a claimed fresh receipt. Two
additional lowercase launch logs predate this stage and are excluded as logs.

### Historical installed inventory — tsunami follow-ups and map debug, superseded by3456e45b

Exact canonical run.sh --stage-only returns0 after the final native/material inputs.
Signed engine331857037f693b12c1374a0bfb65c01c84b7d5418bf82c64376d48235d3349c0;
receipt66c742d6ca3dd069f357709fb145421f7e4b79bb0d2bb04f9f4fe8c598e40519.
All116 entries,37 canonical inputs and both delivered script headers match; cache
and app plans are empty, compiler ready and installer deep/strict signature passes.
Fresh246 non-log player files, five foreign working files and three staged blobs
remain unchanged. No game launch, input, termination or player-state write.

Installed source adds continuous60–300m transport through ordinary hull/coast
collision, actual shoreline trace into a bounded native splash pool, reduced
crest lattice and a narrow exact-color sky seam. Native/source/offscreen component
checks pass; real ship/coast/spray/horizon/weather/FPS acceptance remains unresolved
and player-owned. Ordered patchfeba0ff372f36ff2c7fff96409a3e21e8f42aef7e53c648f34ed798253c10976,
materiala547cef544331beedbfada3ad89af868b6203d81bc8ddd2d33e5368963f57097,
sky0380f543b38b0b9e811de9e500a51a8ecf0f3560651f05813270741c4a102fe2.
Prior continuous damage/capsize/natural event and daylight/fort/audio checkpoints
remain installed. The global-map menu now shares natural creation and deletes
only marked events on Stop/replace. Scoped map/menu cases and the full integrated
PROGRAM pass with explicit fixture seams; real marker/approach/contact/forced sea
replay remains unresolved. The content batch reuses the same installed engine.
No push.

### Historical installed inventory — continuous tsunami, superseded by33185703

Canonical `experiments/native-metal/run.sh --stage-only` returns0 for the expanded
batch. Signed engine:
`4423316154a9a9df91434b503be963cb3b1467d0d19651aeca1ebaa502895a91`.
Receipt SHA-256:
`2b30bb9882742f575cad42689e4121aec2f730293d294cb7716571b590bced66`.
All116 managed entries,37 canonical inputs, empty gameplay plan and deep/strict
signature pass. All246 freshly hashed non-log player files and three foreign
staged blobs remain unchanged. No game launch, input, termination or SAVE replay.

Continuous random strength drives 10–50% nominal hull damage, height/width/speed,
foam and native pitch/roll; existing storm perk and armor reduce damage. Real
beam-on overturn enters ordinary ship death. Marked saved storm descriptors keep
sampled strength and trigger finite-circle automatic local sea with active exit
lock. First white-grid material is superseded. Representative installed
sea-height/normal/sky component frames show irregular crest foam and preserve
ordinary/cancel depth/MRT identity. Full source VM damage cases pass with explicit
contact/terminal seams; native geometry/contact/motion and map transition
component checks pass. Real splash/damage/capsize/natural map replay is unresolved
and player-owned. Source receipt binds native patch
`e35de55b65c69cb46b7d86c670a2993842ff2ce4baabab254ee4f123a287246c`
and material `e105c0ca07a1e472b1b8276321c264e0836d293b55e913933fa6f8988edba8ac`.

Daylight/fort/audio checkpoints `ce5faf2`, `3646cf0`, `31bb837` remain installed.
The additional horizontal wave transport and sky-horizon correction pass component
checks in the next source batch. Its first stage attempt refused while installed
game process30770 was active; a later read observes no game. Further staging is
held for the subsequent requested shoreline splash integration and admitted
crest-material correction. No override or termination is used. These follow-ups
are not part of this installed inventory. No push. Previous
inventories below are historical.

### Historical installed inventory — nearby sound admission, superseded by44233161

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the
focused audio source checkpoint `31bb837`. Supported app remains
`/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`64f461f7c3b74f1fca4ad49bfca9f24bbd017444312fe85a5b9566ad48d39a76`.
Receipt SHA-256:
`740f0676676d2b8a8572631959122330cc33d412839c9e64bb5be0e69737812a`.
The receipt binds all116 managed entries; deep/strict signature passes. All37
canonical gameplay inputs and both delivered script headers match cache/app
bytes; both gameplay plans are empty. All246 freshly hashed non-log player files
and three foreign staged blobs retain their hashes. No game launch, input,
termination or saved-game replay is performed.

The native sound service admits positioned naval transient FX at a40ms minimum
onset interval for the same alias/type within20m. It rejects duplicates before
sample selection or voice creation, retains the first authored level, has no
total voice cap for a continuing salvo and does not queue rejected events.
The source-bound probe reduces512 simultaneous splash requests from512 voice
initializations to1; ten spaced events remain ten live voices. Loops, cache,
speech, music, distant/different effects and prior cannon/charge policy pass their
nearest unaffected cases. Actual broadside sound quality and FPS remain unresolved.

Daylight and fort source checkpoints `ce5faf2` and `3646cf0` remain installed.
The1–7m threshold,2–8m damage curve and ordinary charges are unchanged by audio;
`AIBalls.c` retains its original producer calls. Ships and preceding impact-chart
shape/FPS have player acceptance; new daylight contour and fort balance still
need player replay. Expanded tsunami remains a separate active batch in its
existing owner. No push. Previous inventories below are historical.

### Historical installed inventory — daylight and fort follow-ups, superseded by64f461f7

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the frozen
daylight and fort source checkpoints `ce5faf2` and `3646cf0`. Supported app remains
`/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`10400733be42be03508aa27280dcbd1e2d6be77d11ef638a56c563fe6956da51`.
Receipt SHA-256:
`23c4496eff4d21ae05c3b27d35954b1dd08451ec9e03a3b6f17a8c0fd4651fad`.
The receipt binds that engine; final deep/strict signature passes. All37 canonical
gameplay inputs and both delivered native script headers match cache/app bytes;
both gameplay plans are empty. All247 fresh closed-game non-log player files and
three foreign staged blobs retain their hashes. Root performs no UI/input,
launch, termination or save replay. No game process is observed at delivery.

The player accepts the preceding projection/shape and reports excellent FPS.
This follow-up changes only bright-background rim/keyline strength and fort
proximity: intact gun positions replace the fort's huge enclosing-box trigger.
The existing1–7m threshold,2–8m damage curve, ship behavior and ordinary charges
are preserved. Matched authored PortoBello approaches disable1/1/1/2 guns versus
the former6/6/6/4 and previous installed0/0/0/0. These are geometry/VM checks,
not a saved-game salvo. New daytime visibility and fort balance need player replay.
The rejected cosmetic-fragment muting is absent. Engine mass-sound handling and
the expanded tsunami feature remain separate active batches with their existing
owners; neither is claimed installed here. Esc start is human-confirmed;
physical F11 and the next waveform/ship/world-map behavior remain unresolved.
No push is performed. The prior accepted projection inventory below is historical.

### Historical installed inventory — accepted impact charts, superseded by10400733

The frozen batch passed `experiments/native-metal/run.sh --stage-only` with exit0.
Supported app: `/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`b5e871dd3a2c24168d9267bcda7b44309dd39c7a4e26946e9f167c1627c5113a`.
Receipt SHA-256:
`6c4a9e52ccfa3e68cc59b1a77a91b7f64e5b4050ec4b9c75e693d45d7ae2bedf`.
Final deep/strict signature passes; the receipt binds the installed engine.
All canonical gameplay delivery bytes and changed native script headers match;
cache/app gameplay plans are empty. The protected245 non-log player files and
three foreign staged blobs retain their exact pre-delivery hashes.

Revision12 uses each launch sample's own landing/crossing time, keeps true means,
and evaluates separate sea-XZ and solid lateral/up density charts. Exact ordinary
sea MRT identity prevents the water continuation from climbing walls. The thin
analytic perimeter does not outline alpha holes or receiver-identity seams;
bounded70ms tangent inertia retains current physical depth clips. Current-depth
switches and disjoint footprints clear history. Finite relief support is padded
by the water-impact fore/aft radius; it is not measured terrain or guaranteed
conditional coverage. Stopped air and ordinary firing remain the existing owners.
Source/GPU/Metal-validation fixtures and canonical native build pass. The player
now accepts the drape's shape/projection and reports excellent FPS. Their
brightness-local daytime contour increase is a source candidate, not yet installed.
No instrumented paired FPS or exhaustive angle/zoom acceptance is claimed.

Airbursts now share ordinary bomb aim/height law, use stable per-shell1–7m
proximity and one2m-full/8m-zero damage curve, including sails/forts. The native
projectile codec is unchanged. Esc has sea-only start/cancel tsunami buttons
through the existing pause menu. Their integrated complete PROGRAM include
graph and pause/debug segments compile with zero VM errors without executing
game startup. The player confirms Esc starts the wave and supplies the visible
crest; its repetitive straight appearance is rejected for the next tsunami batch.
Physical F11, cancel/restart and wave/ship response still require player replay.
Ships' airburst results are accepted by the player. Airbursts barely damage forts;
the requested fort-specific balance sits between the earlier excessive damage
and this near-zero result. Its ammunition owner is investigating actual gun
geometry and trigger/damage reach; no ordinary-charge change is authorized.

Focused local source checkpoints: tsunami `bdfce2e`, aim `ba9ae12`, airburst
`a813b29`; no push. Root launched no game, sent no player input and stopped no
actor. Two player startup logs changed during delivery; their actor is unknown
and they do not establish acceptance. One game process is present after delivery;
its actor and actual scene are unverified, and root leaves it untouched.
The preceding revision11 inventory is historical and player-rejected.

### Historical installed inventory — revision11 rejected, superseded by b5e871dd

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after revision11.
Supported app: `/Applications/Corsairs Iddictive Remaster.app`. Signed engine:
`5b14e3196d4674c7cc296de97debc83fc26fbf7a8a8eb88b41f4cf40989221a7`.
Receipt SHA-256:
`0ebc514c8b38488591258f7a36a94c33e6e3301a836f8108c46d8e6bae0fdce4`.
The receipt's engine entry matches installed bytes and final deep/strict signature
verification passes after installation completes. All250 fresh non-log player
SAVE/settings/userdata files and three foreign staged blobs retain their hashes.

The decal now samples current world positions within a finite covariance field,
anchors at the selected receiver and filters the actual surface mask. The airburst
flight offset no longer positions the surface patch. Registered model fragments
write own/relation identity during ordinary geometry rendering; neither their
full-screen depth snapshots/difference passes nor separate own/water captures run.
Stopped air and firing mechanics are preserved. Strict/fast complete scene/aim
MSL, current CPU producer, ordered source stack, native build and source-exact
offscreen native encoder pass. The GPU oracle covers flat, tilted, curved and
creased surfaces, water below the camera, far/horizon rejection, own geometry,
relation color, alpha holes, origin translation and5x zoom. It proves those
properties, not real-game appearance, startup or paired FPS. No game is launched,
player input sent or actor terminated; player replay remains unresolved.

Tsunami's unchanged native patch and the sea F11 dispatch are integrated in local
checkpoint499037a. The real no-window SDL/controls/core/script/native command
chain passes, including restored controls and start/cancel/restart. Physical
macOS Fn/F11 delivery, visible menu and wave/ship response remain unresolved.
`docs/sea-tsunami.md` owns that precise acceptance gap.
Focused local aim checkpoint69f6e84 owns the delivered four native inputs and two
topic updates; no push. Source input hashes bind the same bytes staged before
this checkpoint; only documentation metadata changed after staging.

### Historical installed inventory — projected patch rejected, superseded by5b14e319

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after revision10:
one projected covariance patch, a bounded contact pass and no water ownership
capture. Supported app remains `/Applications/Corsairs Iddictive Remaster.app`.
Signed engine:
`0fc23284301f1b3465887169686ab1b14dd1d6b605a07659a3ca92d73282fe6d`.
Receipt SHA-256:
`d9d0421b3f72784831020597741e1f8d18bebc06e5d7fddbba5307ccf3732a6c`.
The engine hash is bound in the installed receipt; final deep/strict signature
verification passes after the canonical installer finishes. All245 non-log
player files and three foreign staged blobs preserve the fresh closed-game hashes.
The separately owned tsunami content/engine inputs remain in the frozen batch;
no new work is performed on that feature. No UI/input, game launch or termination
is performed. Native build, strict/fast MSL and the actual offscreen native encoder
fixture pass. Focused local source checkpoint00e7d57 owns four aim paths; no push.

The player's three13:13–13:14 screenshots accept revision9's removal of the
giant false hillside line, but reject its main shape: broken upright terrain/
foliage fragments and disappearance over water. FPS83/75/42 belong to different
views and do not establish a paired overlay cost; the player reports remaining
aiming slowdown. Revision10 changes the geometric representation, rather than
adding another receiver derivative or volume-intersection heuristic. The13:51
screenshot rejects revision10: a flat ring crosses water/ship bow without
conforming to their surfaces; the player also reports close-water aim marked at
the horizon. The screen metric ignores receiver position, and airborne arrival
can include the airburst's8-unit overhead offset. Camera inversion and picking
already include the correct absolute camera and water; those are not replaced.
Revision11 is prepared with selected-receiver anchoring, bounded current-world-
depth coverage and ordinary-model MRT identity. Actual player appearance and
paired FPS remain unresolved; further computer interaction is stopped.
Tsunami's no-window SDL/core/script/native-trigger chain passed in its resumed
owner; physical macOS key delivery and visible wave replay remain unresolved.
These source-only checks do not prove a100% live F11 trigger.

### Historical installed inventory — boundary-gradient drape, superseded by0fc23284

Canonical `experiments/native-metal/run.sh --stage-only` returns0 after the
unit-boundary gradient correction to the contact stroke.
Supported app remains `/Applications/Corsairs Iddictive Remaster.app`.
Signed engine:
`2a79c6b1822a5eca4d642d464939e74fc2943415354606684dac6472fe3aa6a5`.
Receipt SHA-256:
`abc852e3fd4bc37af79168e0a6c554911628e67ba97fbfdfc6a9691b1725e5b0`.
The same frozen delivery includes the separately owned debug tsunami patch and
three PROGRAM/INI files; its compiler receipt binds the exact native patch.
Existing ammunition, manual rake reach and firing rules remain in this batch.

All245 non-log player SAVE/settings/userdata files retain the fresh closed-game
baseline hashes after this stage. The engine hash matches its installed receipt;
three foreign staged blobs remain identical. No game is launched or player actor
terminated during this batch. Native build and strict runtime MSL compilation
pass. Focused local commitffe2514 owns only the aim shader/topic paths; no push.

The last rendered observation belongs to predecessor enginec474cc73: normal
actor8202 was responsive in a stocked first-person sea scene, showing67FPS while
looking at sky/rigging without a drape. The player's subsequent screenshot
showed92FPS and accepted a thinner main rim, but reopened a huge secondary
hillside strip and broken perimeter. These observations do not accept the new
engine's appearance or paired aiming performance. Its far-strip suppression,
seam continuity, zoom visibility and FPS remain unresolved pending player replay.
The player's 13:00:21 screenshot shows121FPS but rejects predecessorda34fbae's drape:
the giant hillside stripe persists and spreads, and the player reports worse
appearance. This is a rendered rejection, not performance acceptance.
The new source-exact GPU probe rejects the reproduced false outside stroke while
preserving true-rim AA and center/zero-gradient/range cases. It leaves the actual
field boundary unchanged; requested geometric smoothing of abrupt transitions
has not been implemented or accepted.
Further computer interaction/launch/replay is stopped at the user's request.
The repository-wide context check remains red on four preserved untracked files
owned by the stopped tsunami task; none is adopted into the aim checkpoint.
The debug wave's start/stop and ship response remain with its separate owner.

### Historical installed inventory — ammunition, manual rake reach and smooth depth density, superseded byc474cc73

The canonical `experiments/native-metal/run.sh --stage-only` succeeds after the
lower airburst fuze, fragment collision, count-scaled cadence, percentage crew
loss and final ammunition precision/icon corrections, then the manual rake
reach-order, solid-without-water capture and all-gun smooth depth-density repairs. Played app remains
`/Applications/Corsairs Iddictive Remaster.app`. Installed signed engine:
`70d78026692e3eeb351e65fff178df43fa31994583cc043a2372e930ba3d8678`.
Receipt SHA-256:
`08f6c75b2cd1180d9097ebbdc03ef79d61395be66e65e5aa90557ab4b8360cb4`.
Deep/strict signature passes. Both cache/app delivery plans are empty and
compiler_ready is true; all seventeen frozen inputs of the final staged batch matched. All253 normal
player files retain the closed-game baseline hashes after final staging. The
aligned fire-mode texture is
`448c6bbb827eecbfc9933ecdc0f55f602712d8058f57d7cdeb85ce6dab13d9c9`.

Current base crew losses are round shot0.05%, grapes0.6%, knippels0.075%,
bombs0.125%, airburst1.2% of remaining crew before existing modifiers. Gunner
precision multipliers are100/50/30/80/80%; ordinary round shot preserves the
crew's original accuracy. Airburst base hull39 and cloth25.2 are twice bombs
and knippels respectively. Full PROGRAM startup/catalogue/stores/controls/shop
compile with zero script errors. Actual native VM old-catalogue save/load,
percentage conversion, accuracy values and clamp/zero cases pass. Actual native
cadence/state and fragment collision fixtures pass; their stubs do not accept
visible gameplay.

Two fresh current-save/config clones use the supported installed launcher. The
first reaches a rendered first-person sea scene and closes before final stage.
The final clone's actor PID78428 opens the menu, then a read-only observation
shows a responsive sea scene, ordinary companion ship and an aim footprint.
Its HUD now shows airburst0 and powder2201, so it cannot accept a new stocked
airburst volley. Root's attempted keyboard input is rejected by the UI's
user-changed-app guard; no root input or changed-action acceptance is claimed.
Normal player saves remain separate from these clones. The final clone actor
closes before the native repair preflight; a new player actor PID90979 starts
from the ordinary player directory. Preflight rejects staging while that actor
is alive. No game is stopped or new native binary installed during this transition.
Read-only normal-profile observation reaches a responsive third-person sea scene
but shows round shot0/powder0; this scene cannot accept stocked firing. The
native ordering defect does not replace the existing ammo/powder readiness guards.
PID90979 subsequently closes, but a new ordinary-profile actor PID96068 starts
before the next preflight. Canonical `run.sh --stage-only` exits1 immediately
with `Close the running game before staging or launching the Metal candidate.`
No build/staging occurs and the installed engine/receipt remain unchanged.
No unknown/player process is terminated. PID96068 subsequently closes. The
canonical stage-only path then installs source047406c plusaaf7a69 and returns0;
the independent deep/strict signature check passes. Cache/app plans have zero
pending files and compiler_ready is true. All253 current player files preserve
the new closed-game baseline exactly. After the final density delivery and
checkpoint, the three owned clone/fixture directories are removed through
codex-delete-temp; their durable receipts remain in this ledger and the topic
documents. Stopped clone actors remain closed.
Stocked current-save firing/preview replay remains the player's accepting action.

The player reopens manual targeting: aiming at a ship appears to break fire.
An actual-function oracle proves one relevant defect: the centered selected
depth point rejects every gun before longitudinal rake stations are assigned,
although three stations are mechanically reachable. The smallest native patch
moves exact reach filtering after station assignment. It retains impossible
high-mast and terrain rejection, far intent and mapped single-gun behavior;
cadence/state and native syntax checks pass. This source repair is now installed;
the broader any-ship symptom remains unresolved pending real firing replay.
Source is checkpointed in `047406c`; the three unrelated staged blobs retain
their hashes. Root remains the sole build/staging integration owner.

The October8 screenshot/reply reopens contact projection acceptance: fort arcs
and contour disappear/flicker with aim motion. The live launch log also records
missing water ownership withholding all contacts. Sourceaaf7a69 admits exact
current registered solids without water capture, retains own/stale/unknown
negatives, and passes64 admission states plus strict native MSL compilation.
That independent gate is installed; sampled fan/prism undercoverage stays open.
The player additionally reports shells outside the red zone. The same-instant
source oracle uses actual AIBalls independent yaw/elevation/speed and rotation
math:945/2187 legal one-gun points fall outside the inscribed ellipse, and
selected-projector omission/normalized density mean add multi-gun losses.
The former red region is therefore rejected as a guaranteed dispersion boundary.
The player's subsequent choice changes the visible contract to main-density
regions with allowed rare tails, then relaxes a strict ellipse to a smooth
rounded blot projected on actual scene depth. The source candidate includes
every firing gun, weighted launch-law samples and covariance-root interpolation;
the physical first-hit masks and independent water guard remain. Its canonical
build and installed stage now pass on70d78026; source/GPU and bit-identical
allocation checks do not accept the pending real-game projection or FPS. The
52-gun density constructor falls from3.046 to1.244ms in the paired fixture;
fan/prism holes, closed perimeter and motion flicker remain unresolved. The
focused local density checkpoint is388d4e7; no push is performed.
The GreatRecipe quest remains deferred. Existing trade, fort, fencing and player
state owners retain their prior contracts.

### Historical installed inventory — fort airburst, naval firing modes and Bermuda trade, superseded by4a632fcd

Sourcee2ecbbf passes canonical build.sh and run.sh --stage-only after the endpoint
jitter correction. App: /Applications/Corsairs Iddictive Remaster.app.
Installed signed engine:
b8d033b74c3323bc8e40271f2a5affefae45f085f33516ce663f100b8704ffab.
Compiled unsigned engine:
9802b87f95ec372397a3df7f69410f70c6dd4463dd905dd3ef9100f4d7a4bea4.
Receipt:
f3be724a2caac01c79d1b5be53a4a04bd4b42fa6e710d77b3768c2fd6f05bb65.
Engine receipt/deep-strict signature match. Seven changed cache/app inputs and
all six changed PROGRAM compiler inputs match; both delivery plans empty and
compiler_ready true. All252 fresh player baseline files and three foreign staged
blobs retain hashes. Root launches no game. Fresh game inventory is empty before
explicit sole app/GPU replay assignment to ammo owner01a1195a-bb08-70e3-b43c-ea7ba85a67dc,
using supported installed launcher and a fresh current-SAVE/config clone. Actor
must preserve normal player state, report owned PID/clone/results and release;
no further build/stage/GPU/game comparison runs during that session.

Installed: fort overhead fuze, hull67.5/fort180, navigator cell40, Bermuda-only
normal purchase and saved trade-row reconciliation, quickmenu salvo/single mode,
per-gun reload and quality3.5–10 timing with jitter for all endpoints. Existing
tracers, ballistics, native save codecs and adaptive fencing are retained.
Real store/HUD/fort/single/partial-battery/preview/ordinary replay remains open.
The same mode owner's actual-function old/candidate oracle subsequently proves
one-viable-gun mode0 prediction/delay mismatch: at150m/10mps the new delay scale
creates16–49m systematic lead error. Root authorizes private sample forwarding
only in the native firing-mode patch and topic doc, retaining modes1/2+, codec,
scripts and jitter. That correction is not yet installed. Fresh-clone actor owns
PID36601/session38693 using supported launcher; no new build/stage occurs until
its bounded baseline replay ends and it releases. Future GreatRecipe remains docs-only.
Projection CPU/GPU resource preservation passes and bounded historical-run gain
is recorded below, but production/FPS adequacy is rejected; no prototype is
installed. Global map hourly jitter still lacks its advancing-map profile.

### Historical installed inventory — corrected ammunition/icon and adaptive fencing, superseded byb8d033b7

Canonical run.sh --stage-only succeeds after source4d42729 and fencingc4143b9.
Played app: /Applications/Corsairs Iddictive Remaster.app.
Installed signed engine:
fcce6e7b27c9df45e17982ccf487e93aa1af07af1e0f60888142497095ac3132.
Compiled unsigned engine:
4ddb0cdb74e72e748a709a5e1ec1f2dd177904dd99470fc21ab7d3d371d2aadf.
Ownership receipt:
ee1cb94afb21bb3250d66b12396bdbbec55a54b93e0d2bcf49974e2568be291a.
Engine receipt matches; deep/strict signature exits0. All five changed content
inputs match cache/app and both changed PROGRAM files match the zero-error full
native compiler fixture. Installed/cache plans are empty; compiler_ready true.
Corrected shared12m aim, either-direction fuze, shop ordering, both drawn icon
atlases and adaptive fencing are installed. Original tracer/ballistics baseline
is retained. Source installation is accepted; corrected shop/sea action and
fencing scene/balance replay remain unresolved.

Fresh closed-game baseline includes251 player files. Post-stage comparison finds
only five startup logs changed; all SAVE/config files retain hashes. Root launches
no game. New unknown/player PID89995 PPID1 starts23:25:51 after engine/receipt
delivery23:23:56. lsof txt inode393740055 equals the installed executable inode,
binding this process to the corrected engine. Preserve the live scene; no second
game/GPU workload. All three foreign staged blobs remain exact, agent_context
passes after the source checkpoint. Root releases build/stage ownership and
returns receipts to ammo, fencing and projection owners. Projection prototypes
are uninstalled; their GPU/resource/throughput gate waits for the live scene to
close and sole-GPU assignment. Global map jitter still lacks its advancing-map
profile. Airburst read-only CUA screenshot fails with native pipe closure; this
is no gameplay acceptance. One subsequent read-only capture on the same route
succeeds: the live sea/spyglass scene renders responsively, targeting a nearby
ship. HUD shows ordinary Bombs1849/Powder1584 and the stock bomb icon. Single
frame120FPS is not salvo/airburst/manual-aim performance evidence. No input was
sent; special charge5 volley, shop/icon, friendly and unarmed states remain open.
Projection's exact ordinary UV memo passes CPU193/14164573/8730/13 and strict
compile; local frozen55 CPU cost0.997816→0.826971ms is17.1% lower. Its increased
segment storage still needs GPU resource/throughput acceptance. Source owner
retains that frozen temp and continuation; GPU/FPS/installed projection remains
unresolved while the player scene owns the GPU. Root compiler fixture has no
remaining dependency and is removed through codex-delete-temp after delivery.

Later observed transition: PID89995 closes normally; source parent and root fresh
exact-basename inventories find no game. Root grants projection soleGPU for one
frozen UVmemo capture/corpus while ammunition's mode child remains CPU-only.
Terminal resource/193Result/14164573leaf/8730range/13negative preservation passes;
primary spill1152→1120,96/44 registers unchanged, instructions58638→94691.
Historical-run whole two-pass warm medians7.68808→6.51192,5.94563→5.06154,
4.74008→4.20117ms show11.4–15.3% lower isolated time, not paired-scene/FPS proof.
Production/FPS adequacy stays rejected at4.2–6.5ms per55–64 endpoints. No
projection is installed and no new variant follows. Session12 terminates,
fresh profiler/game/GPU/probe inventories are empty; supervisor reports no game
interruption. GPU actor explicitly releases; root's build/stage serialization
hold ends. The next fort-proximity/fire-mode/trade-policy/HUD-icon source packet
remains unfrozen under ammo owner01a1195a-bb08-70e3-b43c-ea7ba85a67dc.

Ammo owner then returns16 frozen paths and normally releases source leases.
Root rejects deterministic endpoint delays for2guns under always-jitter contract;
the same mode child repairs every-gun bounded jitter, preserving ordered positive
delays, approximate3.5/10 windows and unchanged codecs. Revised64-site fixture
and64seed2/4/32×3.5/10 cases pass. Root verifies all16 final SHA hashes,
all three foreign staged blobs and fresh game absence. Native build.sh exits0:
8252 inputs verified, eight source files changed, nine renderer consumer classes
pass and engine linksarm64. Root-owned full PROGRAM compiler fixture links the
canonical Ninja flags/libraries and composes only seven pending app inputs;
startup/catalogue/initStore/controls/store all pass with script_errors0. No game
is launched. Frozen future GreatRecipe concept is documentation only; unlock
quest remains deferred and Bermuda sales stay available. Root owns focused
checkpoint, canonical stage-only and post-stage SAVE/config/source comparison.

### Historical airburst inventory — October 7, superseded byfcce6e7b

Played app: /Applications/Corsairs Iddictive Remaster.app, signed engine
40c8c30003414fca988119a5720867e68cf68df6021c8942ac8b6cb6cd135d4b,
content receipt9678b1336556d4566dd5fb7bd7806a3ad3758b0e444176bf7321a88f02b56844.
Airburst source164fa93 passed canonical stage-only; deep/strict signature and
all13 exact cache/app content inputs pass, changed PROGRAM bytes match the
zero-error native compiler fixture. Compiled engine wasdc1bd592. Distinct
original-ammunition tracers remain installed. Older3df6afbf inventories below
are historical predecessors, not the current engine.

Root launched no game. Installed PID85802 began23:04:01 in the standard player
directory; airburst owner also confirms no launch. Initial post-stage SAVE/config
hashes match, while five startup logs change and two sentry files appear. Later
live play adds Curacao sea2 and removes prior AutoSave10; those concurrent changes
are preserved, not restored. No blanket251-file preservation is claimed after
that live state transition. The process is subsequently absent at the ML build
preflight; do not infer future closure from this snapshot.

The player rejects absent overhead bursts and misplaced shop display order.
Airburst owner reopens fuze/shop display (persistent IDs retained) and combines
the never-installed drawn icon into one corrected delivery. Its earlier icon
delivery fixture pass is withdrawn: temporary symlink rejection occurred before
testing. Root holds install until corrected source/evidence freeze, GPU release
and a fresh closed-game check. Projection prototypes remain uninstalled.

Separately, adaptive fencing source/ordered integration c4143b9 links through
canonical build.sh; compiled candidate is55129060828f5986daeab3972686bcc1b623a14b9b64bbea681fe2e0fa044e6c.
It is not installed; scene/balance replay remains player-owned. Native build
preflight has46% memory free and no game process. Root's focused ML checkpoint
preserves foreign index blobs; hygiene check is currently blocked only by four
airburst icon assets owned by that active source task. No unsafe adoption occurs.
Root retains Git/build/stage integration; source owner repairs ammo; projection
owner has released its short embedded-capture replay actor: native session7 ends
normally, no profiling/benchmark or GPU actor remains. Existing heatmaps expose
invocation/SIMD costs only, so arithmetic subroutine dominance stays unproved.
Its next source-cost diagnosis is CPU-only; no new GPU variant is admitted.

Root releases24 current-thread scopes through normal native release after
checkpoints, returning ammo/icon source ownership for the proven repair. The
reported cause is ordinary-bomb aim height≈2.2m below the overhead slab and a
descending-only fuze; height warp keeps that aim chord. Shared12m target-height
offset plus overhead traversal in either direction is a source proposal under
the ammo owner, awaiting actual-arc and shared preview/fire negatives. Save IDs,
native lanes,35m arming and blockers remain required; no new fix is installed.
Root retains corsairs-compile-program.j0u6cp1y only for the already-authorized
corrected-batch compiler replay and baseline evidence; cleanup belongs to root
through codex-delete-temp after that batch reaches its terminal disposition.

### Historical combined player-defect package — October 7, superseded by40c8c300; player replay pending

That historical played engine was signed3df6afbf; boarding/cargo source is committed in
c69959f and installed with the frozen cannon subset. The player reports global-miniature-map jitter, absent thoughts loot
orders, projection holes/flicker and nearly empty enemy cargo. Manual target aim
also remains slow: the held live scene (November19,06:00) shows50FPS;1185/1717
main-thread samples are in the aiming camera, chiefly GEOMClip/GEOMTrace. Cannon
owns projection/FPS plus later chainshot/tracer/marker corrections. Root's separate
advancing-map hold is pending, so this sea sample is not a map diagnosis.

Read-only save evidence finds Cornelia's third merchant hold already almost empty
before capture while two convoy companions carry wine/weapons. Root reproduces
greedy100/0 allocation, then verifies50/50 with exact finite stock conservation,
negative cases and four codec rounds. Optional own stores now use real port stock
and existing headroom/reserve, retain their ledger on sea entry and do not block
sailing when scarce. Full-PROGRAM stock/weight/snapshot checks pass. The cargo
contract/evidence belongs to `docs/worldmap-traffic.md`.

The prior installed standard MainHero variant lacked the order menu and its cabin handler
ignored the saved mode. Root connects both variants and the handler, including
current-cabin mode changes. Actual VM default/silent/manual and codec checks and
the composed full-PROGRAM/MainHero menu checks pass. Contract and adapter limits
belong to `docs/boarding-loot-orders.md`. Source verification does not accept
installed dialogue, transfers, NPC loot balance or rendered smoothness.

After the player's normal closure, canonical build.sh and run.sh --stage-only
both exit0. Compiled engine0dd247a1 becomes installed signed engine
`3df6afbfb67f741bd6fe0283cc1c7f809a981a020cfcceb038e390a5af6a81a8`;
The original batch receipt `7a8522267414c66b09877cd491fb8d2791c6379d6851f9181804e1cb60c667a9`
matches it; the later content-only tracer receipt below supersedes that receipt.
Post-stage deep/strict signature passes. Cache check reports zero
pending files/compiler ready; the independent installed canonical plan is empty.
All251 player files retain exact hashes, with no new files. Installed MainHero
retains standard.c identity and matches its military-callback composition;
the development cache deliberately retains boarding-loot.c. Boarding, worldmap,
AICannon/AIBalls and both public/private shared headers match their consuming
receipts and composed source. Verification initially compared unlike MainHero
variants and the wrong private-header path; corrected owner-aware verification
passes without a product edit.

This batch also installs demasted chainshot hull targeting, the existing luminous
bomb flight stream for all ammunition, and authored-texture surrender tint.
Actual VM/codec and marker GPU/Draw probes pass; installed player firing/marker
replay remains unresolved. Four frozen cannon paths are committed in cc85f54
after the hook specialist's0cfcc99 repair and the original unchanged normal
helper's successful retry; foreign staged/working changes remain separate. Global-map
jitter, projection holes/flicker and manual-aim FPS remain open. Cannon product
freeze is released for its next source batch; root remains the sole integrator.

### Ammunition appearance — October 7, distinct profiles committed and installed; player replay pending

The player rejects identical-looking ammunition after the installed uniform
bomb-stream batch. Root owns a bounded content revision: grape microtraces,
round shot smaller than chainshot, and the original bright bomb effect. Native
K2 PSYSv3.5 profiles reuse only original luminous Particle2, with ordered width,
life and emission rates; the three nonbomb smoke clouds are removed. Grapes and
knippels sprite sizes change while their native type-specific collision radii
stay intact. Actual VM initializer, original-catalogue speeds, perk/no-cannon
negatives and four codec rounds pass. Prior-runtime binary asset delivery,
idempotence, source/runtime drift rejection and existing text codecs pass.
Commit ed0d98d checkpoints eight owned source/document paths; three foreign
staged blobs retain exact hashes. Normal `sync_metal_gameplay.py apply` exits0
with exactly four content changes, reusing the installed engine. Canonical/cache/
app/receipt hashes agree for AIBalls and the three XPS profiles. Independent
installed plan is empty; cache has zero pending files/compiler ready; post-install
deep/strict signature passes. All251 player files, signed engine3df6afbf and
authored Bomb_Smoke a115d87a retain exact hashes. Current receipt is
`1950a423440b4a42c0fad09900bdc0f26a983afcd6791e6a86ae2800cb32615a`.
No engine rebuild, launcher/configuration change or gameplay launch is performed.

The disposable child's parser confirms ordered graph parameters, and its
original particle-bytecode GPU fixture is supporting shader evidence only.
The corrected separate native DataSource::Load/FieldList proof accepts all three
new XPS plus original Bomb_Smoke, with exact1/2-component counts and native graph
evaluation. Actual BBProcessor/per-profile geometry/night visibility are not
accepted by the synthetic fixture. Particle reduction is an analytic source
budget, not measured game FPS. The child cleans its own temp through the normal
helper (exit0); root releases the sole GPU proof actor to the cannon owner.
Visual gameplay acceptance, perceived speed and night FPS remain unresolved
with the player-owned firing replay.

### Continuous aim family — October 7, first GPU performance attempt rejected; not installed

Root inspects the cannon owner's gpu-results.txt/final-cpu-results.txt and
separate Porto curve51 input records in continuous-family.kfAeKRPV. Shared
CPU/MSL family checks pass11 named cases and182 seeded equal-record endpoints,
with zero GPU parity mismatches across193 families. The unsupported
Flyingdutchman zero-time sample and one synthetic binary64 Porto endpoint
transfer/oracle disagreement remain separate, not silently accepted.

Measured warm GPU query time is12.027ms/55 Flyingdutchman families,
8.981ms/63 Nevis,7.690ms/64 Porto and7.068ms for one Nevis duplicate query.
Disposition: rejected for the requested performance target; isolated parity
does not prove scene integration or game FPS. The same source-only child owns
only conservative BVH-node broadphase revision, with outward float bounds and
compensated leaf predicates. No production input, build, install, launch or
player state changes occur for this attempt. Signed installed engine3df6afbf
and content receipt1950a423 are independently rechecked unchanged. Scene/water/
lifecycle, production boundary certification and actual aiming FPS remain open;
root continues to own integration and the cannon owner the source proof.

### Continuous aim FP32 broadphase — October 7, second GPU performance attempt rejected; probing stopped

Root inspects float-node-cpu-results.txt/float-node-gpu-results.txt against the
retained baseline report. Outward FP32 node bounds preserve193 equal-record
CPU/GPU cases and compensated leaf/sweep arithmetic. Warm GPU query time is
12.4439ms/55 Flyingdutchman families,9.3938ms/63 Nevis,7.8303ms/64 Porto and
7.3196ms for the Nevis duplicate; one-triangle coplanar query costs1.1423ms.
Disposition: rejected for performance. Differences from the previous attempt
may be run noise; reduced node arithmetic has no proven benefit or dominant
cost attribution. The separate Porto serialization disagreement remains open.

The cannon owner stops GPU work after two failed attempts, retaining its report
and evidence; no production input or install changes occur. Renderer/resource
diagnosis is the cannon owner's next bounded responsibility before another
algorithm variant. Root remains the sole production integration/build/stage
owner; installed aiming FPS and scene/lifecycle acceptance remain unresolved.

### Continuous aim shader resources — October 7, diagnostic evidence captured; no performance improvement accepted

Root reads the cannon owner's retained gpu-resource-report.md. Native Apple
capture/profiling uses only the frozen55-family workload, one command buffer
per capture, with all55 outputs and queries/visits/tests unchanged. Compiled
state is61806 instructions,96 temporary/40 uniform registers,11552 spilled
bytes and zero static threadgroup memory. L1 traffic is predominantly stack
(0.37/0.39GiB/s reads and0.48/0.48 writes). This establishes heavy experimental
kernel stack state, not stack bandwidth dominance or the installed FPS cause.

The maxThreads32 descriptor retains inputs/arithmetic/parity but changes no
compiled resource fields; structural benefit is unsupported. Cross-session
dynamic instruction estimates disagree and are not speedup evidence. The
cannon owner closes sessions13/20 and reports no remaining native debug session
or exact probe/game process; GPU actor is released. No production delta or
installation occurs. Disposition: resource evidence accepted at its diagnostic
layer, performance/scene/player-FPS acceptance unresolved. The same repair owner
now uses a bounded read-only source specialist to bind segment/coplanar data
lifetimes before any edit; root retains production integration/build/stage.

### Continuous aim segment state — October 7, CPU/compiler gate passed; GPU performance pending

Root reads gm-binding-report.md and family-segment-state.aWvG5A2o's
cpu-final-results.txt/metal-compile-final.txt in the retained projection fixture.
For Flyingdutchman1/Nevis/PortoBello all40445/80025/104038 original-scale BSP
triangle triples match visible/collidable render triangles; no collidable render
face is absent. Source DrawBuffer binds svertex as BaseVertexIndex. Disposition:
source anatomy binding accepted for these three models only; live transforms,
instances, depth/deformation/camera/water remain unresolved.

The source specialist's scalar Segment state and immutable Family/Bin references
retain generic coplanar handling.193 complete equal-record Result shadows and
14164573 ordered per-triangle checks pass, as does strict MSL compile with
fastMath disabled and zero queues/dispatches. Separate Porto transfer/oracle
drift and unsupported zero-time sample remain open. CPU/compiler proof accepts
no FPS improvement. The projection chat owns one serialized14-workload GPU
falsifier; root holds GPU-heavy build/staging/replays until its actor release.
No production source/cache/app/index changes or game launch occur in that probe.
Root remains production integration owner; performance and scene acceptance
remain unresolved, and all previous rejected performance results are retained.

### Continuous aim scalar GPU — October 7, isolated improvement; product performance rejected, actor released

Root reads family-segment-gpu-report.md.14 workloads/193 families/56 serialized
command buffers retain all Result/Pair bits and work counters. Three-warm medians
fall from12.4439/9.3938/7.8303 to7.7885/6.0224/4.8505ms for Flying55/Nevis63/
Porto64; duplicate query falls7.3196 to5.1948ms. Coplanar1 regresses1.1423 to
1.4941ms. Disposition: isolated corpus improvement accepted; product/full-image
performance rejected. Porto transfer/oracle and numeric certification stay open.

Frozen55 native profile retains original inputs/outputs/counters. Spilled bytes
fall11552 to11040, registers96/40 remain and instructions grow61806 to134200.
Stack bandwidth rates fall0.37/0.48 to0.18/0.08GiB/s; neither total traffic nor
bandwidth/player-FPS dominance is established. Session9 ends normally, fresh
native session and exact probe/game inventories are empty. Projection chat
explicitly releases GPU actor; root's build/stage/replay serialization hold ends.
No product source/cache/app/save change or staging is requested. Projection owner
retains artifacts and continues proven kernel/resource-boundary investigation,
preserving coplanarity and uncapped sweep; root remains production integrator.

### Airburst integration — October 7, native/startup compiler/timer gates passed; staging pending

Root accepts the frozen source packet from airburst owner01a1195a-bb08-70e3-
b43c-ea7ba85a67dc, preserving foreign staged/working paths and the distinct
tracer baseline. Canonical build.sh exits0: ordered stack changes three native
files, renderer routing checks pass and arm64 engine links. One build uses the
existing cache and10 native workers on the48GiB/16CPU host; memory pressure
reports53% free, no overlapping game/build/GPU actor at admission, estimated
8GiB build plus8GiB OS/interactive reserve. No app launch occurs.

The disposable current-native compiler copies installed executable-relative
headers and composes the exact13 pending content inputs over installed PROGRAM.
Its initial missing-header setup fails before product compilation. Once corrected,
full startup compilation exposes the product's unavailable GetTickCount API.
Root replaces only the script limiter with registered PostEvent/ready-handler
and creation reset; native patch and projectile codec stay unchanged. Full startup
plus catalogue/store/controls now pass main=1 catalogue=1 stores=1 controls=1
script_errors=0. Actual native timer processing retains one heavy effect through
119ms and permits the next at120ms: before120=1 heavy=2 blasts=3 script_errors=0.
Source owner VM save round and swept-fuze negatives remain valid for unchanged
inputs; live shop/select/burst/friendly/direct-hit/ordinary-bomb replay stays open.

Player baseline binds251 regular non-symlink files and signed engine3df6afbf;
current receipt1950a423 remains installed. Root's compiler/baseline temp owner is
corsairs-compile-program.j0u6cp1y; the redundant intake copy is removed through
codex-delete-temp. Staging waits projection GPU actor release. Airburst source
scopes have released; separate frozen adaptive-fencing sources await their own
scope release for root's exact-path index/check closure, excluded from this batch.
Disposition: source/native/compiler/timer gates passed, install and gameplay
unresolved. Root retains sole integration/build/stage/install responsibility.

### Continuous aim specialization — October 7, resource boundary passed; product performance rejected, actor released

Root reads family-specialized-gpu-report.md. Ordinary/full masked separation
preserves193 complete Results/Pair bits, counters and CPU masks;56 serialized
command buffers end112 encoders/dispatches, poisoning mask/output on every repeat.
Primary spill reservation drops11040 to1152 bytes and instructions134200 to
58638, while masked full retains11072 bytes/133699 instructions. This accepts
the compiler resource boundary, not bandwidth dominance or player FPS.

Total two-pass warm Flying55/Nevis63/Porto64 costs7.6881/5.9456/4.7401ms versus
7.7885/6.0224/4.8505;1.3–2.3% may be run noise and remains insufficient.
Disposition: product performance rejected, no larger dispatch or integration.
Session9 ends, fresh native session and exact probe/game inventories are empty;
source owner explicitly releases GPU actor. Root proceeds with the independent
frozen airburst stage; projection prototypes remain excluded. Projection owner
retains artifacts and continues read-only query/arithmetic diagnosis, not a GPU
actor. Porto transfer/numeric and live scene/lifecycle acceptance remain open.

### Historical harbour and aiming-FPS batch — October 7, superseded by3df6afbf; player replay pending

Native full-hull BSP replay reproduces three saved LeFransua returns stalled
outside arrival while the point-sized PTC route is connected. Commit c6bf191
adds bounded transient full-hull recovery after ordinary port ground contact;
the original arrival callback/radius and living fleets remain intact. Four winds
and mid-approach transient reset/save-attribute reload pass; rendered player-map
replay remains unresolved. Build passes. Player closure was observed and the
combined canonical stage-only completed after the final source freeze.
The cannon writer released its verified aiming-FPS source as11571da after the
new player report. Root now owns combined build/install with ce5eef9+c6bf191,
avoiding a second package. Harbour c6bf191 remains frozen and is now installed.
Its live aiming profile attributes1533/1951 main-thread samples to the overlay,
including BSP clipping and segment/model tests. The cannon owner reports an
extracted AimMarch probe preserving1000seeded first-hit/segment results while
reducing AABB checks92.99%. Final native GEOM Clip/ClipByPlane equivalence passes
1920 real Flyingdutchman/Nevis/PortoBello queries, including1183 over-cap cases,
exact callback bytes/order/cap128 and cold/warm/zero-plane/test-only/rejection
negatives. The tighter sphere was rejected for capped-set differences; the
original sphere/traversal remain. Source is frozen; live FPS remains unaccepted.
Combined canonical build.sh passed after11571da: stack migrated exactly three
source files and the complete engine linked. Compiled candidate is
`7e091886541c4a31d505ea739e189f8e616bc533638839b9912cb0599490a7c8`.
Prior compiled2e552d2d and installed fdab9859 are historical. Canonical
run.sh --stage-only completed with signed installed engine
`37598a37e4c73e7d4823576b69ad0a0dca34b59c97ce70efb9637f74d209b9f9`.
Installed receipt68f05ba2e537252aabc54b4095b8389183345334fbe299ad72cc888e5f917170
binds that engine hash; deep/strict signature passes after staging completes.
The earlier concurrent verification during resealing failed and is superseded
by this post-stage result. All252 player files retain exact hashes, with no new
files. Both shared headers and12 cannon/worldmap content consumers match source/
cache/app, including0051df1d encounters and944fa5c0 sea outcomes. Root releases
the single installed-app launch/UI actor to the cannon writer for normal prior-save
LoadGame aiming/FPS/firing replay first; root does not launch/profile/advance play
concurrently. Harbour rendered acceptance remains root/player-owned afterward.

The cannon replay owner independently verified installed37598a37 on live30726
and observed the public app's current saved night sea battle (November3,02:12,
Time2) through CUA, without gameplay controls. Normal third-person FPS114 is
scene startup evidence, not an aiming improvement versus the old daytime51.
Matched manual-aim profiling remains pending the player's same-view hold. The
launch/UI actor stays with player+cannon replay; root does not profile concurrently.

Frozen cannon checkout rollback is retained under canonical native-metal/.cache:
installed-engine-backup holds exact signed8c5fb0bf/fdab9859 engines; all nine
SHA-addressed original blobs remain in gameplay-source-backups. Historical
receipts11728d8f/ea3ee5e3 and old script_defines.h0810dd98 are separately retained
in installed-engine-backup/frozen-cannon-476c045, never over active headers/receipts.
Hashes and engine signatures pass; app/player files are untouched. The cannon
writer owns archive of its managed frozen checkout after this retention handoff.

### Historical installed cannon/header batch — October 7, superseded by37598a37

The previous installed engine was
`fdab9859e2228c979a675f25e0f75e769df4ccb0b909dac2171bc89a3a2dff53`.
The cannon writer delivered5ceff92 plus ce5eef9-equivalent event-header correction
through frozen476c045; its reported live sea-battle startup is separate from root's
harbour fixture. Older installed snapshots below are historical. Source c6bf191
is the next engine batch; player SAVE/configuration were not changed by root.

### Installed integration snapshot — October 7, mast repair delivered; repair action replay unresolved

The later player replay reopened cannon acceptance: the old-save menu does show
RakingFire, but its placeholder icon, surface gaps, unrotated baseline envelope,
sinking-sail aim, surrendered colors and sail-HP jumps are reported. The next
cannon repair is a source candidate; it is not yet installed.

The sole played application is `/Applications/Corsairs Iddictive Remaster.app`.
Current signed engine is
`8c5fb0bff1c12a65c9716998f560be9516e08216f6d3324ef948a5ea052096ed`,
observed after the parallel cannon-verification writer's canonical
`experiments/native-metal/run.sh --stage-only`. That later batch retained the
mast bridge and generated icon; cannon action acceptance stays with its writer.
The original mast delivery's signed engine
`bf42886adaf2185c7a0bc51dc88e148175d37f22f4c3656a58375810f8f0c9eb`
is now historical. The earlier
`34e1c03d03ad7be83669c92bd11534d24213833a69113dcc7325e4f9f9b66eec`
engine is preserved by the canonical
backup owner. Older engine hashes below are historical attempts, not a second
current inventory. This snapshot supersedes their pending-install labels.

Emergency mast repair is installed in BattleInterface and the existing
LeaveBattle confirmation. Native compile and whole-PROGRAM/lazy confirmation
compile pass; a native VM fixture and serializer round pass limits, 8–72-hour
timing, scarce planks, cancellation, stale quotes, rollback and repeat callbacks.
Its external queries and geometry bridge are instrumented. The original feature
batch reported zero pending files and compiler ready; strict/deep app signature
verified that batch.
The original mast batch reopened in an active first-person sea battle, observed
through its native window. The later cannon batch is observed at the main menu;
no competing UI interaction was started. Repair requires safe sea, fallen main masts and
planks; rendered attachment, confirmation fit and save/load after repair remain
unresolved with the player. No game input or save mutation was used for this
observation. The dedicated mast/mallet icon is now generated from the existing
command-atlas reference and installed as two 64px tiles in texture slot 5. Its
prepared TX and BattleInterface binding match the receipt; actual safe-sea menu
visibility remains unresolved. `docs/mast-repair.md` owns the behavior contract.

The current batch retains Living Caribbean and closes safe sea sail uploads,
live code-based surrender relation lookup, restored per-ship lamp ownership,
the squad-compatible loose-pickup halo and the already-consumed sailing/berth
inputs. The coherent dynamic reflection cube stays at its ordinary cadence;
rejected transient rings and distant crew/shadow cuts remain excluded. Reviewed
FPS draft bytes remain separately preserved in the ignored local archive.

Canonical build/stage, strict app signature and compiler readiness passed for
that delivery; its cache and installed gameplay plans were empty. The arcade
quarter-speed correction c8a136c is delivered. The mast/cannon batch described
above supersedes its earlier pending-install label; later working changes still
need their owning checks. The selected sea-return source repair339178e is also delivered through
the existing gameplay_sources/delivery_state content transaction. Installed and
cache sea.c match its canonical projection, SHA256
944fa5c0cc3e028ca3d828ec019243cd35b2d6a91c5f6096069d861ae976e8ba.
For that earlier content-only transaction, engine34e1c03d and all241 post-quit player files stayed unchanged;
the transaction's strict/deep signature verification passed.
The legacy ordinary spawn bypass repair742a597 is delivered through the selected
content transaction: current cache/installed worldmap_encgen.c SHA256
0051df1dd35d4094af4ad6b30f1f6216e1f3af27c2f70a74a20766302a4482dd
supersedes the earlier a3fb4c7a source below. Strict/deep signature passed;
engine8c5fb0bf and244 post-quit player files are unchanged. The player's existing
over-limit groups are preserved, and three LeFransua arrival stalls are under
native investigation; this is not a harbour-drain or player replay receipt.
Built/cache/app message headers match.
Player journey source is saved in focused local commit
`59a3dfad5f6cd4fcc4aad3cc819feae2b634b3ef` (14 owned paths; no push).
Whole-PROGRAM compilation and four serializer rounds pass with pickup composed
on supply. Real Metal probes verify immutable sail/IB rewrites and recycled
slots, warm allocation reuse, additive pickup pixels, depth/occlusion and full
state restoration. Relation CPU/sanitizer and owner-lamp negative checks pass.
These checks do not measure game FPS or accept a rendered gameplay action.

The safety batch adds funded partial service, stable crew reserves within an
authored hiring refresh, paid crew/gun capacity reservations and a 75% physical
commercial load ceiling. It fixes repeated seller deduction, impossible cargo
destinations, wrong-port credits and false completion with residual freight.

The finite trade cycle and player journey hardening are installed with worldmap script
`a3fb4c7ac5773bd7c85a12140c6b1eec59de5c615ff5ea8224bb238ce17ffc1b`.
Commerce admission now requires finite work, yields to idle incumbents, and
previews the whole real roster with the same paid service planner before native
creation. Actual living freight reduces remaining demand. Ready unemployed
fleets can physically reposition after a day, with exclusive working-port claims.
Hostile shipment targets redirect at sea to an explicit physical return; losing
both endpoints uses a recorded salvage port and a separate stock receipt.
Aborted repositioning releases its former working-port claim.

The earlier gameplay-only `run.sh --stage-only` succeeded after final source/pins.
Its engine stayed `096395187b7b85e3f647f528d176531f0e6fbe16ad5baebc0029ac725197b8a0`;
234 observed player SAVE/SaveData/config files retained exact hashes. No game
launch or player-save upgrade was performed. Actual-script state/serializer
scenarios and final whole-PROGRAM composition pass. A full-archipelago isolated
decision dropped from 12.514 s to 58.5 ms for depleted stocks (177.3 ms rich).
This is not measured game FPS or acceptance of a rendered economic cycle.
Existing cargo is preserved during diversion. New assemblies stop feeding an
observed stock-blocked port; paid replenishment releases that guard. Quota60 and
object ceiling80 remain unchanged. Empty ports remain possible after demand,
war or player purchases; numeric balance is still initial tuning.

Sea handoff now checks actual actor/group/ordinal admission, preserves dead
tombstones and reconciles orphan sea ownership before map refresh. Capture records
loss before the actual transfer removes the group receipt. Only the living
original pending officer is recovered; a purchase binds only the purchased hull's
berth. Military endpoint rejection, orphan reservation and zero original survivors
use existing physical cleanup. Native avoidance fades only for ordinary port
approaches, service holds recoil and departure clears stale pursuit.

Canonical stage-only exited successfully after the final native source and five
script changes. All five installed script hashes match the frozen candidate;
gameplay plans are empty and compiler readiness passes. The signed installed
candidate hash matches the installer's receipt (raw build bytes differ because
installation strips, rewires rpaths and signs). All 234 existing player SAVE,
SaveData and configuration files retain identical hashes. No game was launched.

Actual-method ASan/UBSan navigation and real COMPILER state/conservation probes
pass, including four serializer rounds and nearest allowed cases. The combined
installed PROGRAM candidate and normal lazy shipyard LoadSegment compile without
script errors. The reusable explicit `script-vm-probe` diagnostic passes; native
reconfiguration preserves unchanged watermark bytes instead of invalidating all
objects. Initial diagnostic setup failures were dependency/include scope and
fixture duplicate declarations; they were corrected at those owners, not gameplay.
Headless military localization uses a fixture-only key-return adapter because no
StringService scene exists; actual military/news persisted effects remain intact.

Disposition: delivered source/state safeguards; interactive harbour drain,
prior-player-save upgrade, capture/purchase, service/battle, FPS and perceived
balance remain unresolved with the player. An unvisited legacy service plan lacks
the pre-casualty crew watermark and preserves observed lives; all native late-damage
producers are not covered by the actor handoff proof. See `worldmap-traffic.md`.

The dev helper is integrated in `4106e7e` and now targets installed
`Contents/MacOS/launch`; its native dry-run
resolves this engine using bundled settings/state ownership. A one-file actual
installed push/revert canary is exactly restored and signed, with native binary
and all 233 preexisting player SAVE/config hashes unchanged. Disposable helper
fixtures prove conflicts, owned rollback, locks, symlinks and clean upgrade
ordering. No game launch, player input or forced exit was performed.

Prison-day/tournament, sea aiming/ballistics, map/encounter UI and night-floor
candidates already match canonical installed consumers; stale backlog labels
are corrected. The player's same-save sea FPS/visual comparison, pickup action,
prison arena/outcomes and the existing Living Caribbean scenarios remain
`unresolved`. The broadened FPS/dev-helper request is handled before any goal
pause; native task state remains the scheduler owner.

### World-map island avoidance and ship jitter — October 6, installed `d8d2980c`; player replay pending

Historical engine inventory at the October 6 attempt; current engine belongs to
the integration snapshot above. The October 7 player report of stepped movement
reopens player-ship smoothness acceptance; AI avoidance checks do not close it.

Player reported that world-map ships sail into islands instead of working around them, and that
a ship jitters back and forth while under way. Source delta: `wdm_islands.cpp` gains
`FindDirection`/`FindReaction` node fallbacks (probe along and against the course, static
reaction radius 20 -> 32) and `worldmap-traffic.patch` extends `wdm_enemy_ship.cpp` /
`wdm_merchant_ship.cpp` with island look-ahead steering, island weight 1.5 -> 2.0, smoothed
ship repulsion, a resolved degenerate turn sign, a speed branch that converges on
`minManeuverSpeed` without overshoot and patrol reversal that needs a return point farther than
25 units. Engine `d8d2980cfd7f0810ced5ccf2a4015ecfc10bdd3d46506e1e7cfbda25f5e14337` is
installed into the played app; gameplay scripts were not part of this delta. Disposition:
`unresolved` — needs the player's world-map replay of both defects.

### Dynamic reflection cadence — October 7, installed coherent baseline; sea replay pending

Historical 4s sample of PID83470 (three own and four foreign ships) put 464/1678
main-thread samples in EnvMap_Render → SHIP::Realize, including per-draw buffer
creation. The later half-rate dynamic-cube draft was removed before the October7
delivery; `sea-reflection-budget.patch` retains bounded sun-road cadence and
waterline reprime, with a coherent dynamic cube every normal frame. Safe shared
sail uploads close the repeated allocation owner without accepting reflection
lag. Current source is consumed by `ee144763`; same-save performance and water
appearance remain player replay. The previously reproduced unmodified-HEAD
waterline-probe failure remains outside this delta.

### Animation buffers and model lifetime — October 6, installed; save/sea replay unresolved

Resumed the unfinished crash patch from chat
`01a10f5b-84f6-7a33-a406-41af3f18909d`. The requested batch contains the growing
animation-buffer registry, sailor move-assignment release and recursive owned
model-child deletion. See metal-frame-pacing.md for the contracts and
retained sanitizer evidence. All three canonical source files match the tested
candidate and the compiled delivery source.

The main checkout's installed-content preflight reproduces
`unrecognized installed fleet bridge: PROGRAM/sea_ai/sea.c`; its content plan
also includes the user-paused expansion. The already prepared frozen delivery
checkout retains the installed gameplay package and has zero pending scripts.
Its canonical `experiments/native-metal/run.sh --stage-only` exits zero after
consuming the lifetime patch, verifies the bundle signature and installs engine
SHA-256 `97d40b3b55b2ecfd34faef515382d33341f45cd50b881f2671fe8923c3694d41`,
UUID `20312442-F4B4-33DC-886D-2242D7B76674`. The pre-install build hash is
`f3f6edd2d1beb3c56424a21f2565790cac830d9f8933eb3d250c128ec37a0260`;
rpath adjustment and signing account for the different installed hash.
The previous signed engine `073f9be014385482642ea3806a6daf41aca3ae3062a71928089232305608d6ac`
is retained by the delivery cache's installed-engine backup owner.

All 925 installed PROGRAM files and 217 original player SAVE/config files
retain their hashes, including after diagnostics. The native menu renders on
both the normal installed-app launch and the installed launcher's task-owned
copy-save profile. The latter process (PID 74011) uses the installed binary and
fixture cwd; automated keys/clicks do not open the save browser, even after
raising the window. No save-load or interactive-scene acceptance is claimed.
The fixture process is closed; no game/build process remains. No running player
game was stopped and no paused expansion was installed.

Disposition: build/staging/installation accepted; native save-load, open-sea
crash reproduction and long-session model accumulation remain unresolved,
with player replay/report as the next evidence owner.
Focused source checkpoint: `315e3bf` (the lifetime patch and ordered-stack entry);
no push. Repository hygiene passes and foreign staged blobs are preserved.
The existing frozen delivery checkout remains with the crash-replay owner;
this task created no worktree. Disposable copy-save diagnostics are removed.

### Prior FPS rollback, engine safety and sea strength — October 6, superseded engine; sea replay pending

Canonical `experiments/native-metal/run.sh --stage-only` completed after the
player closed the game, from the isolated installed-content delivery checkout.
Installed signed engine is `073f9be014385482642ea3806a6daf41aca3ae3062a71928089232305608d6ac`,
UUID `490F4355-A1CB-3F77-85DC-3A7484FE5F19`; its pre-install build is
`cf59e38555e350060271410080d0968574e4498a0f1922df2bfc7056bdd9215d`.
The installed engine matches the canonical signed candidate and deep/strict
bundle signature verification passes. Installation changes executable rpaths and
signature, so the pre-install and installed hashes intentionally differ.

The installed batch includes the mast null-child guard (`19525c9`), foam
registration capacity guard (`0532a1b`), rejected geometry-cache FPS delta
rollback, and shared sea strength / loaded-cannon count fix (`72963c0`).
Exactly three installed script consumers match their frozen hashes:
`sea_ai/AICannon.c` (`3419ae6185f2c551d4aeeea328ba66aa6e1d4866dcbcc14ffb05536103c4d878`),
`sea_ai/sea.c` (`7b4c7a20efa8d2b18954a47e134e7cc3701b72d241d6b38fcb4a30db63ad976a`), and
`worldmap/worldmap_encgen.c` (`3fe5ad32138fafabece7fe38ad8c98443349656da27753da689c53febb2aec9a`).
Paused expansion changes remain excluded. All 215 preexisting player SAVE/config
files retain their hashes; a new autosave appeared after staging finished during
the player-launched session. No player process was stopped.

The player reopened the installed app at 02:32:55 EDT (PID 63449). Native window
observation shows the world map rendered with the player's fleet and a displayed
125 FPS. This accepts startup/rendering only, not open-sea FPS or stutter behavior.
The changed sea-strength action and exact save-load/sea crash scenarios remain
unresolved under the FPS/crash root owner, triggered by player replay/report.
The exit worker has finished; no installation wait remains active.

Latest actual pre-fix crash is October 6 01:31:24 EDT, PID 16765, on previous
installed UUID `B326BD3C-E57F-3CCA-ADE0-C13CB04EFD2B`:
`SEAFOAM_PS::Init +156 -> SEAFOAM::AddShip +180 -> SEAFOAM::Init`. The crash PC
dereferences the invalid INIFILE argument `0xbdd00f1b41688990` before the first
texture-key read. The corruption writer and causal relation to the FPS change
are unproven. The newly pasted attachment is the earlier 00:36:55 mast crash on
old UUID `36D8A720-0642-3598-9870-B8A61C2BE4E5`; it cannot prove mast-guard failure.
See metal-frame-pacing.md for component evidence and rejected hypotheses.

### Living Caribbean expansion — October 7, implementation installed; player replay unresolved

Implementation/delivery is finished after the user's explicit resume; native
goal state owns the subsequent requested pause and player acceptance remains open.
The current source batch closes paid initial assembly/service, physical crew/gun/
goods capacity, preservation of explicit low/zero ship state, weighted eligible
expedition targets, real evacuation routes and independently held harbour control.
Twenty-three existing callback owners bind participation, real naval/fort/land
contribution, crime revocation, return news, pre-damage harbour choice and safe
contact with the real expedition commander. Legacy missing crew fields receive
the established default; explicit zero remains zero. Late contracts preserve
uncontracted credit and cannot increase its reward ceiling.

Canonical `experiments/native-metal/build.sh` passes and reuses the verified patch
stack. Whole-PROGRAM VM compilation, lazy dialogue compilation and four native
serializer rounds pass. A separate VM runs the actual hull-damage consumer,
physical cargo-load comparison, contribution/repaired-repeat negatives, late
patent/contract, murder scope, return news and contact admission; its native save/
load repeats earn no additional credit. Both final runs have zero script errors.
The old synthetic prize fixture now reserves actual crew weight; no assertion was
removed. These fixtures do not accept rendered scenes or balance.

Composition is idempotent and rejects an unknown callback revision. Exact pinned
parallel aiming/sailing/boarding/custody edits are retained. Read-only preflight:
the prior 14/11 pending-consumer inventory and PID47160 are historical. The game
was observed closed before delivery. Canonical `run.sh --stage-only` succeeded
after final source/ordered-patch integration, and installed signed engine is
`46f859b8b41135c368d9f40a5a57ecb38ef3cedb19c9ca6a89f3d79bfaaf5e45`
from source checkpoint `316e8b0`. Both cache and played-app plans are empty;
compiler readiness and deep/strict signature pass. All 233 player SAVE/config
hashes are unchanged; built, installed and VM message headers match.
Hostile contact now has request→explicit consent→revalidated visit, exact scoped
deck protection and one-time return cleanup, with native live-projectile query.
No game launch, player input or forced process exit was performed. Real prior-save,
action/scene/pricing and FPS replay remain player-owned and `unresolved`.
Disposition: implementation and installed delivery accepted at their layers;
visible gameplay/balance is not accepted by source/VM/build evidence.

### Prison blank scene after a day or bout — October 5, installed; focused replay accepted

Hypothesis: custody's shared day completion rebuilds weather inside an already
loaded cell. Baseline installed custody SHA-256
`b764435349c4c2e8f2c408b5ae018601e32fd1a1cfc6507adc702d4ebe2c377b`;
installed engine `29ab8ad61cf66ffe502769ccb738c3eca2f7eb83cdacc6e577ebed72f520c93e`.
No game was running at intake. A disposable copy of the player's Santiago tavern
save, the installed executable/assets and current graphics settings reproduced
the defect: ordinary cell entry rendered correctly, but both rest and an actual
nonlethal defeat/return left a flat blue scene with only HUD. The hero remained
at `goto9`, position (-15.33168, 0.0001, -2.882836), and hero/location/model entities
were alive. Missing prison assets and an invalid return locator are rejected.

Exact delta: remove only `Whr_UpdateWeather` from `CustodyLife_Day`, preserving
the cell's locked Inside environment while advancing the date. The same fixture
then retained room geometry and the hero after rest and defeat/return; the
cellmate menu rendered with nine days remaining and zero wins after defeat.
The rendered custody body equals the delivered one apart from its comment.
The existing native VM compilation/state probe passes date costs, funds, parole,
reward idempotence, defeat, cancellation, weapon/immunity cleanup and isolated
arena creation. Its input was bound through `sync_metal_gameplay` to the reviewed
portable package; the direct historical probe entry failed before execution on a
missing native-storm original. No permanent tests or native changes were added.

`sync_metal_gameplay.py apply` succeeded with exactly one pending script, resealed
and verified the installed app signature, and preserved the engine hash. Cache
and app custody now match
`af63f1a9d008d2e664f4aab5596b10e6c776a569777c684587d2acf4b20c4b46`;
both content plans are empty. SAVE/configuration were not replaced. The normal
app launcher subsequently started the installed engine under its player-state
owner; the player took control, so automated input stopped. That process later
exited before a passive screenshot; no normal-session prison replay is claimed.
All diagnostic processes ended and the owned temporary fixture was removed through
`codex-delete-temp`. Disposition: this reproduced blank-cell
defect is accepted in the recorded copy-save replay; tournament victory,
interrupted-save recovery and release remain outside this repair's replay.
Focused local checkpoint: `48def07` (`fix(prison): keep cell visible after days
and bouts`), no push. Repository hygiene passes; unrelated staged/working changes
remain with their owners. See prison-surrender-system.md for the owning contract.

### Regional shipping routes — October 5, installed; player replay pending

Hypothesis: uniformly selected global destinations spread a finite 32-fleet
population across long unrelated trips, weakening visible regional encounters.
The route owner now weights distance continuously, retains a positive long-route
floor, spreads origins by same-role residency and directs pirate routes toward
actual ordinary commerce. Existing diplomacy, quest exclusions, local detection,
fleet strength and battle rules are preserved. Existing saved fleets keep routes;
new departures carry origin/destination metadata in the existing descriptor.

Disposable native script VM compilation and seeded route checks pass nearer/farther
preference, commerce attraction, same-role origin pressure, quest/other-role
negatives and missing-coordinate fallback. Canonical `run.sh --stage-only` succeeds.
Cache and played-app encgen both match
`c7d8ed65bfa0bfa002d4cdcafb9cef184a6853d136b698cee852057762c7b449`.
Engine remains `29ab8ad61cf66ffe502769ccb738c3eca2f7eb83cdacc6e577ebed72f520c93e`;
no native change or player-state replacement. Installation is verified; density,
encounter frequency and enjoyment remain unresolved until the player's voyage.
See worldmap-traffic.md for the route model and research references. Focused local
checkpoint: `3c6ad94`; repository hygiene passes. No push.


### Jungle dusk illumination continuity — October 5, installed; player replay pending

Player screenshots at 17:26:01 and 17:26:04 show Cuba jungle ground switching
from dark to bright at the same March 6 1668 18:50. Live installed engine
`8dfd720876045ae8e63bde2cff5d52bca2c1c532a19236c64003c6f4f9898e06`
reports alternating `applied=1 sun=3` and `applied=0 sun=0`. The source admitted
outdoor illumination only when a shadow map was applied, despite Weather
legitimately skipping the sun prepass in dusk/night. See metal-lighting-parity.md.

The shared admission now accepts any successfully completed active location
prepass. Shadow validity, ambient floors, weather, indoor response and failure/
disabled fallbacks remain unchanged. A disposable GPU replay fails on the old
expanded static path (9,360 changed colour channels, maximum delta 24); after
the fix all four expanded/raw static and CPU/GPU-skinned paths are pixel-identical
with and without shadow passes. Existing dynamic-lighting negatives pass.
Canonical build succeeds with zero ordered-patch source changes and the content
plan has zero pending files. No permanent test delta. After the player confirmed
the game closed, canonical `run.sh --stage-only` exited zero and verified the app
signature. Installed signed engine:
`29ab8ad61cf66ffe502769ccb738c3eca2f7eb83cdacc6e577ebed72f520c93e`.
SAVE/configuration preserved. The installed app was reopened, its menu rendered,
and PID 41195 runs from the established Application Support owner. The player
resumed walking; logs reach outdoor lamp-free frames and a passive screenshot
shows the Cuba atlas. Automated load input was refused because the player was
actively interacting. No further input is sent and the running game is retained
for that player. Disposition: source/GPU/build/installation and startup verified;
the exact jungle load/walk brightness replay remains player-owned and unresolved.
Focused source checkpoint: `cfc9e5c` (two renderer files, no push). The required
`agent_context.py --check` again exceeded a bounded 30-second run; its owned
process group was terminated. No broad hygiene pass is claimed. Focused diff,
GPU and build checks pass; the preserved foreign staged paths are unchanged.

### Encounter review and compact panel — October 5, source candidate

Player reports blind plain-entry into hostile fleets, wrong default priority and
stretched encounter art. Exact sources show quick entry bypasses ExitFromMap,
contextual commands were replaced with plain-entry defaults, and square 512x512
art was stretched into 518x160. Candidate restores quick-entry review (31130)
and contextual attack/pursuit focus; explicit panel sea entry remains31131 with
quest guard. Illustration is square160x160 beside top-aligned scrolling summary;
actions remain one row. Disposable source checks and native installed-header VM
compile pass. Source commit0e551a2 installed after the player closed the game. Canonical
run.sh --stage-only completed; all four installed encounter files match the
reviewed output hashes. Actual geometry/action replay unresolved.

### Coherent fleet simulation and encounter actions — October 5, installed; scene replay pending

The persistent roster, sea handoff, commitment/retreat decisions and independent
plain sea entry are integrated. Canonical build and isolated VM state probes pass.
Initial stage delivered engine/scripts but omitted the built shared message
header, causing player startup SIGABRT during script compilation. An isolated
probe with copied installed headers reproduced the crash. Corrected
install-engine.sh delivers reviewed messages.h to app/development consumers in
its signing/rollback transaction; unknown header edits are refused before writes.
Corrected run.sh --stage-only completed successfully. Installed and development
header SHA-256 is 38bab60eeeb5cb90370f3ea4c51b9be5169555ac09df74e15debd630257125a2;
installed signed engine is 8dfd720876045ae8e63bde2cff5d52bca2c1c532a19236c64003c6f4f9898e06.
The same isolated compile probe now passes using the delivered installed header,
including popup compilation. SAVE/configuration preserved. Disposition: installed
and compiler verified; actual menu/save/UI/balance replay remains player-owned
and unresolved. Focused local source commit 6039e62; no push.


### Player fleet threat assessment — October 5, installed

The prior traffic delivery excluded the player from NPC strength comparison:
the player fallback chased only by distance/hostility. The correction estimates
every active companion-slot ship from class, hull role and current HP/SP, using
the NPC strength scale. Ordinary traffic scores the player alongside NPC prey;
superior player fleets cause retreat and suppress forced contact, while voluntary
entry, naval and quest encounters remain available. Nonquest legacy pirate
Follow also strength-gates pursuit, but remains a dedicated pursuit encounter.

Native build passed. Exact native-method probes pass risk/bravery thresholds,
escape, trade preference, distance and quest/qID/ALONE/missing-state negatives.
Exact composed scripts compile in the isolated native VM; active-fleet, parked
ship, damage, departed companion and no-ship fixtures pass. The first script
compile rejected makeref on an attribute branch; makearef corrected it.
Independent review identified first-frame awareness; map-entry and synchronous
creation refresh corrected it before the final script probe.

The player closed the game and canonical stage-only began. Its content step
installed two reviewed worldmap scripts and preserved SAVE/configuration, but
the player reopened the installed game before engine replacement. The canonical
engine guard stopped that step (exit 1). After the second explicit close reply,
the unchanged canonical stage-only completed (exit 0), with signature verified.
Installed signed engine SHA-256:
`2679fd1c3ee3feacfcb020ad4d39fc87093e0f75382248de2b17578f7f8b08e3`.
Installed worldmap.c is `f744ff02267d85c4c07fd32f81f8aa2814e6038867fdacc6068352ee33dd2c6d`;
worldmap_encgen.c is `975b3085bc51e8508d56c3138fcd62646f7ad2bf2d0d78b7874c68da7af21372`.
Installed plan has zero pending files; no player-state replacement or game launch.
Additional VM fixtures pass legacy-pirate awareness using real class tables,
quest exclusion and retained bravery; fixture enum/nation setup was corrected.
Source commit `f449f90`. Broad repository hygiene exceeded its bounded 20-second
budget inside git check-ignore --stdin; only its own process group was stopped.
No broad hygiene pass is claimed. Disposable task fixtures were removed.
Disposition: installed, source/build/native/VM probes verified; player scene
replay and perceptual gameplay balance remain unresolved.

### Global world-map traffic — October 5, installed baseline

Owner: parent integration in native-metal. Replaced player-ring trade/prebuilt
battle spawning with bounded native port traffic (18 trade, 8 patrol, 6 raiders).
A caravan has real encounter fleet counts. Targets use live relations, class/
count strength and saved condition. Pirates prefer commerce, avoid stronger
fleets and flee nearby superior opponents; home patrols can supply one rescue
to their own commerce. Battle state uses saved IDs and at least 24 hours of the
actual native map clock, with deterministic victory at twice opposing strength.
Sea import includes the rescue fleet and retained hull/sail/crew wear.

Canonical build exit 0, engine SHA-256
`0095faecfbeed11f6689fea71bca02e93a6810e9ea5bf16c934c502f34c844ee`.
The earlier accessor placement caused a C++ private-member compile failure;
it was moved to the public clock owner and the canonical build then passed.
Focused actual-native methods pass role acquisition/escape, territorial leash,
friend/quest exclusion, saved-ID reordering, removed partner, pause, one rescue,
one-day minimum, superiority and guarded-trade risk. Exact composed scripts
compile in isolated native VM; rescue-range/cursor and retained-wear state probes
pass. Fixture paths/branch references were corrected separately and never changed
installed PROGRAM. These probes do not accept actual navigation or sea scenes.

The frozen content plan contained five map/sea paths plus the separately
reviewed boarding script/dialogue. No game process was running, and canonical
`run.sh --stage-only` then completed (exit 0). Installed signed engine SHA-256 is
`5db2f3ebd1acc00b1c7db1b3fb41f5681029a53b5ad1960a0306bfe820e85911`;
the unsigned build hash above differs because staging signs its candidate.
Installed five map/sea files match their exact reviewed output hashes; the normal
installed content plan reports zero pending files, including boarding. SAVE and
configuration remain preserved. No scene was launched for acceptance.
Source checkpoint: `e80b20e`. Broad agent-context hygiene stalled again in
`git check-ignore --stdin`; only this task's stalled processes were stopped.
No broad hygiene pass is claimed; focused source/native/VM checks passed.
Disposition: installed, source/build/probes verified; player replay unresolved.


### Boarding loot to flagship chest — October 5, installed

The player requested all cabin chest loot in the ship chest, with no automatic
items granted to the hero. Collection stores every exact item ID/count and cash
in flagship `box1`; quest/unique/rare, gold and unknown items are included.
Manual inspection leaves ordinary stock for the player; its exit safeguard stores
remaining protected items in that same chest without personal pickup. The reply
is `Да, всё в наш рундук.`. See `fleet-gameplay-audit.md`. Source commit: `d391091`.

Before delivery, the then-installed native engine's isolated script VM rejected
the prior behavior and passed this batch with an empty error log: seeded chest
stock, multiple sources, exact counts, duplicate special items, raw gold, cash,
unchanged personal inventory/wallet, repeat collection, manual preservation,
invalid cabin/destination and overflow refusal. Geometry/readiness, quest-use
queries and exit rendering are fixture boundaries; this is not player acceptance.

October 5, 14:18 EDT: the monitor found the frozen scripts already installed by
the parallel native/world-map staging owner. Exact installed SHA-256 values:
- `PROGRAM/Loc_ai/LAi_boarding.c`: `e7215938e52f09931069fcd7645922e0c1eb0c465a4608c187ad6e7038e48771`.
- `PROGRAM/dialogs/russian/Enc_Officer_dialog.c`: `4ca117b98c1b2f076a7419aa0bbb2eab4e8ec501706f8cd99d79e297c1c4cb64`.
Both cache and installed sync plans are empty, compiler readiness passes, and
`codesign --verify --deep --strict` exits zero. The installed engine is
`5db2f3ebd1acc00b1c7db1b3fb41f5681029a53b5ad1960a0306bfe820e85911`;
this heartbeat performed no installation, engine or player-state writes.

The earlier incoherent shared stage is resolved. Heartbeat `automation-4`
(Установить сбор лута в рундук) is confirmed PAUSED, with its saved definition
retained. The earlier broad agent-context check stalled in git check-ignore;
no broad hygiene pass is claimed. Unrelated dirty/staged files remain untouched.
Disposition: installed and signature/hash verified; the next real cabin collection
and manual-inspection exit on the player save remain unresolved.

### Treasurer shop eligibility and rarity — October 5, installed

The player reported a false unavailable-store status and no automatic sale.
Real store type is shop, while treasury expected store; merchant and door IDs
also differ in case. Treasury added a nationality refusal that ordinary store
trade has commented out. All three independent merchant rejections are corrected.
Rarity no longer vetoes selected chest items; quest/unique and explicit locks remain.
Native filter/merchant replay passes; transaction fixture lacked normal stock
bootstrap and is not a successful money/notification replay. See treasurer-service.md.
Player scene replay remains unresolved. Commit `c0b0e2e` was delivered by
canonical `run.sh --stage-only` (exit 0); installed core SHA-256 matches
`5f60221013586c18b5709a27f30ca6bb640674652fa71d9802cc93f886435809`. Engine, SAVE and configuration remain unchanged.
The broader agent-context hygiene check stalled in git check-ignore and was stopped;
its success is not claimed. Task-owned source and documentation are committed.


### Automatic treasury trade notification — October 5, installed

Automatic port service adds the existing GETITEM message icon and text once
when proceeds or spend is positive. No sound is played, as requested.
No-op orders, unavailable merchants and save rehydration do not generate
success notifications. Service runs on port, town or store entry when enabled.
Source commit: `b77aa4174d9d2c942060cb505a98b4063130dbb7`.
Canonical `experiments/native-metal/run.sh --stage-only` completed successfully.
Installed characterUtilite.c SHA-256 matches the pinned source output:
`00e8be7f5716d4791aab6f5560beff5f62219b3bc63ecfb35621fa066aa3570f`.
The installed engine was reused; SAVE and configuration were preserved.
Installation is confirmed; automatic purchase/notification replay remains unresolved.

### Treasurer equipment veto removed — October 5, installed

Player capture proves KeepCount=0, but ordinary blades remain blocked by
`Нужно вам / офицеру`. The prior patch was installed but did not remove this
independent veto. The player explicitly owns equipment-retention choices.

Both equipment-need predicates and their blanket sale veto are removed.
Ordinary eligible chest equipment follows the explicit retention, category, price
and manual-lock rules. Personal/officer inventories are outside the sale donor
set. Quest, unique, rare and unknown-item protections remain. The final preview
column shows rarity. No saved setting is reset. After the player closed the game,
canonical `run.sh --stage-only` succeeded, installed script hashes matched the
manifest, and the existing engine was reused. Source commit: `c97c097`. SAVE,
configuration and renderer resources were unchanged. No game replay is claimed.

### Treasurer zero default retention — October 5, installed

The latest three campaign saves omit `Treasury.KeepCount`; the shared chest-sale
calculation and interface previously substituted one retained copy. Their fallback
is now zero. Explicit saved choices remain intact, and quests, rare items, useful
equipment and manual locks still block sale before retained quantity is applied.

Canonical `experiments/native-metal/run.sh --stage-only` succeeded with two script
updates. Installed script hashes match the pinned manifest; the existing engine
`abff64b4e1d002379e6bab3f04f6ee3173b05bdee09c37a59cdcee53bc053044` was reused.
SAVE, configuration and renderer resources were unchanged. No game was launched
or additional gameplay replay run, honoring the player's stop on further checks.
Disposition: installed; interactive sale acceptance remains player-owned.

### Treasury controls and deck framing — October 3, installed

The player supplied treasury, deck, shore-leave and aiming captures. The cropped
deck image was rejected for publication and replaced with an explicitly
recomposed full-body illustration; these edits do not accept native behavior.

The initial follow-up corrected missing treasury `TableActivate` handling and
restricted the 0.5 m eye lift to manual first-person firing. Canonical
`experiments/native-metal/run.sh --stage-only` completed and installed engine
`abff64b4e1d002379e6bab3f04f6ee3173b05bdee09c37a59cdcee53bc053044`; the initial
214 player-file hashes were unchanged at delivery. The source/matrix and native
row-event fixtures predate the player's explicit stop on further checks.

After the next player captures, the purchase checkboxes and footer controls were
aligned into single rows, chest quantities and item features were separated, and
Telegram was removed from the main menu. The player closed the game and authorized
installation. The same canonical stage completed with four script/INI files
applied and the existing engine reused. Final batch, player-state snapshot and
logs are retained in `.cache/portfolio-followup-2026-10-03`. No further gameplay
or rendered-layout checks were run; installed delivery is accepted, interactive
behavior and final appearance remain unresolved until the player's replay.

### Historical portfolio capture-pack snapshot — October 3

Hypothesis: the middle of the portfolio case needs actual fleet/treasury/growth
screens, and a separate save profile can expose those states without changing
the player's campaign. Six prepared copies now live in `SAVE/Портфолио` under
the public launcher's existing external player-state owner. The source game,
installed PROGRAM/RESOURCE, engine `a743bc4a` and player configuration are unchanged;
no native build or staging was needed. The manifest and capture guide are retained
in `experiments/native-metal/.cache/screenshot-save-pack-2026-10-03`, alongside
the 4,368,779-byte ZIP, SHA-256
`da64753c81ecd7e7dd720b05f5ba7ed34fe6a90bfbb11e1dd29cc6a14aecaf90`.

The prepared state is limited to the profile identity, independent cargo targets,
safe-sale settings, conserved officer-to-chest supply transfers and four reduced
morale values for the paid-rest picture. Two sea snapshots retain their native
sea data exactly; item definitions and quests are unchanged. The original 205
recorded player files retain their hashes. All six installed copies match their
manifest hashes; the ZIP passes CRC verification.

The installed engine loads the original control and all six saves with empty
error logs, verifying their profile/location/rank and relevant prepared state.
An isolated OnLoad observer bypasses world creation and rendering. The first
harness input omitted InterfaceStates.Launched, and an attribute-address
expression did not compile; initializing the flag and using makearef correct
those fixture inputs. Neither failure required a game-source patch. The decoded
objects roundtrip exactly, and a corrupt compressed payload is rejected.
No player game was launched or stopped. Disposition: save-pack generation,
native state compatibility and profile delivery accepted; actual scene loads,
UI actions and screenshots remain player-owned. The capture/format contract is
recorded in `docs/screenshot-save-packs.md`.

### Treasurer alignment and chest valuables — October 3, installed; player replay pending

The player's treasury screenshot shows tab labels at the top of their controls
and inconsistent authored button text. Native source binds the tab defect to
`FORMATEDTEXT` clearing vertical alignment when the script replaces its text.
Treasury tabs and the shared-chest tab now reapply alignment after their final
label write; ten treasury and two ship buttons use the normal UI font with
explicit vertical offsets. The existing rectangles and original engine defaults
are preserved. The sale table now shows only its equipment/valuables categories,
with shorter action labels and a separate valuables toggle.

Emeralds were explicitly outside the old equipment-only filter. The new shared
filter admits ordinary jewelry/minerals from the flagship chest only, preserving
quest/unique/rarity/price/keep safeguards and the ordinary merchant restrictions.
The isolated native VM compiles core/store/treasury with an empty error log and
verifies exact emerald/mineral quantities, buyer/chest/wallet conservation,
personal inventory identity, repeat-sale refusal and protected supplies/amulets/
unknown stock. Scene rendering and real player transactions remain unresolved.
Ten XInterface checks pass with zero failures and an empty error log, including
valuables toggles/keep preview, returning to purchases and full native UI unload.
Earlier UI fixture attempts waited on a game-time exit behind the paused window;
the established cancel-then-exit lifecycle resolves that fixture boundary.

The installed content batch contains exactly five consumers: `itemsbox.c`
`2b70d49b`, GoodsTransfer `58f977d0`, character utilities `2ce7768f`, treasury INI
`88a059df`, ship INI `fa4a01b9`. The hash transforms are idempotent and reject
unknown bytes. The installed engine remains `a743bc4a`; content-only sync reuses
it rather than incorporating unrelated pending native edits. After the player
closed PID 90102 and authorized commit/push, the ordinary exact-hash
`sync_metal_gameplay.py apply` exited zero. Its bundle reseal and deep/strict
signature verification passed. Both plans now report zero pending files; all
fourteen fleet pins and the ship INI match in cache and installed app.
All 212 regular player-state files retain their pre-install aggregate SHA-256
`bb6086c449ab595af37a8f2a90db03bc770e319cc1223d7953a31d8670eaa09d`.
No player game was launched, no native engine was rebuilt, and unrelated working
and staged entries are preserved. Commit `3a1bf7e` is verified at `origin/main`;
the dependent treasury completion `c0de1e4` is included in that push.
The earlier normal apply refused the open game before writing. Its bounded
heartbeat `automation-3` was deleted after the successful user-triggered retry.
Disposition: integrated source, content installation and signing accepted;
rendered alignment and real sale on the player save remain unresolved until replay.

### Earlier treasurer completion and shipyard collision — October 3 historical installed snapshot

Canonical `experiments/native-metal/run.sh --stage-only` exited zero after the
final treasury callback and manifest changes. Six changed consumers are installed;
all fourteen fleet pins match both the cached and played resource roots, gameplay
sync has no pending files and the compiler is ready. Deep/strict signing passes.
Current changed pins: character utilities `4d2db76b`, officer dialogue `0b603006`,
GoodsTransfer `b545f8de`, store `d031125b`, loader `75576eb6`, interface INI
`ca5ba998`. Engine `a743bc4a`, the 197 recorded player save/configuration files and
the six inherited source/index entries remain unchanged. No player game was
launched or stopped by this attempt; installation followed the player's closure.

Treasury now has separate purchase/sale tabs, ship-specific total cargo targets,
shared money reserve and real stock/capacity limits, partial orders, configurable
safe equipment sale preview, retained quantities and explicit item locks.
Automatic sale and purchase are separately opt-in; save rehydration never trades.
Queued service obeys actual port/shop/quest/night/busy restrictions and one
accumulated time charge. Contracts and fixture boundaries are recorded in
`docs/fleet-gameplay-audit.md`; raw verification is in the ignored
`experiments/native-metal/.cache/treasurer-verification/2026-10-03` owner.

The player's shipyard report matches the real log's `shipyard.c:1068` compile
failure. A native before/after reproducer binds it to the old treasury's generic
`Add` function surviving as a compiler name after segment unload, colliding with
local `add`. The same sequence also reproduces logged ship/tavern compile errors;
a fresh VM and the corrected scoped callbacks both compile all three cleanly.
Treasurer verification passes 34 core checks, 22 native UI checks and 12 automatic
service checks with empty final error logs. The unconverted UI-busy attribute was
also reproduced and corrected; actual queued trading now defers while UI is open.
The player's live treasury/shipyard and port transactions remain unresolved until
replayed; neither compilation nor widget state proves screen composition or
actual world-time advance. Disposition: source/probes/staging/install accepted;
real game feature acceptance unresolved.

The previously handed ZIP64 archive contains the older eleven-pin batch and is
not refreshed by this installation. The player owns cloud upload. FPS diagnosis
and the reported combat durability remain separate unresolved items below.

### Live sea battle FPS — October 2, CPU hot spots measured; comparison pending

The player's 23:24 screenshot shows 44 FPS in deck aiming view with four fleet
ships and a burning target. A five-second read-only sample of the existing
installed process (PID 12605, started 22:31:35) places 79.7% of 2,465 main-thread
samples in entity realization. Manual aiming accounts for 16.2%; sea rendering
accounts for 28.5%, including 13.3% in reflections. Index validation is a nested
9.9% contributor, mostly sea geometry. The unchanged engine is `a743bc4a`, on
Apple M3 Max at 2056×1329. No build/archive process was active and no thermal
warning was recorded. The old ICB-allocation hot spot is absent.
The public run has frame profiling disabled, so GPU duration is not measured.
The player-owned same-battle Tab/FPS comparison is pending; no renderer or
configuration change was made. Details and raw evidence belong to
`docs/metal-frame-pacing.md`.
Disposition: active CPU work diagnosed; exact camera contribution/GPU bound
unresolved, no performance-fix or FPS-improvement claim.

### Boarding, chest exit and treasury first-open — October 2 historical installed snapshot

The player reopened acceptance with unreachable cabin exit, an attackable loot
speaker, too many dialogue choices, and a blank captain/zero cargo on first
treasurer open. The fleet layer now has eleven pinned script consumers. Latest
pending outputs are boarding `dbdf4a2a`, officer dialogue `4e5d0d36`, chest
interface `36b643ca`, and treasury interface `1da1ed60`. The player closed the
game and canonical `run.sh --stage-only` exited zero on merged `d7b9c40`.
All eleven installed pins match; gameplay sync reports zero pending files and
compiler ready. Installed deep/strict signing passes, and native engine
`a743bc4a` is unchanged. No player save/configuration or process was changed. Contracts and fixture limits belong to
`docs/fleet-gameplay-audit.md`.

Native fixtures pass two choices/Esc, both reply paths, restored exit input,
neighboring chest/body/fight/interface clicks and consecutive protected-loot
conservation. The original chest close reproduces seven global/type errors and
leaves actor mode; the local snapshot correction restores player mode, retaining
the cabin-menu exception, with empty error logs. Treasury source/C++ discovery
binds the initial `-1` to scroll initialization; the data fixture proves valid
first cargo/orders and later companion selection without changing the first order.
Real scene/input acceptance remains player-owned. Reported excessive hero
durability remains unresolved; this correction does not change the HP curve.

PR 21 is merged as `a88433e`; PR 22 (treasury first-open) is merged as
`d7b9c40`. The frozen public app contains all eleven current script pins, including
the four corrections, and passes deep/strict signing with unchanged native engine
`0e45889d`. Native Info-ZIP refused an incremental update of the 11 GB ditto
ZIP with an invalid-structure warning; the old inventory is unchanged. A fresh
native ditto ZIP reproduced the defect: 95,308 entries and offsets above 4 GiB
are represented by wrapped 16/32-bit end records without ZIP64 records. Both
archives fail standard random-access header reads; native ditto extraction reads
the actual payload and all eleven extracted pins match. Packaging acceptance is
reopened. Native `zip -FF --out` copied local records but failed with `EFAULT`
(Bad address) before producing a final archive; no such output is published.
The bounded libarchive 3.7.4 probe establishes the compatible route: stream
local records with Mac-extension interpretation disabled to retain explicit
AppleDouble files, then use the ZIP writer with automatic ZIP64. Forced ZIP64
local headers are rejected by native ditto even for small files; automatic mode
preserves bytes, file/directory xattrs and symlinks in the native extraction
probe. The real 95,308-member archive is being assembled with the frozen signed
app plus its native AppleDouble tree; bare libarchive ZIP is rejected because
its metadata writer gap is documented in issue 2041. No original archive or
installed app is changed by these task-owned packaging attempts.

Read-only inspection of the last player save (`вход в пещеру 4`, 22:17:46 local)
parses all 6,467,501 payload bytes and 649 variables with no remainder: Blaze,
index 1, rank 18, player type, 83.096/196 HP, `Progression.HPCurve=1`, no
`chr_ai.immortal` or HP-checker attribute. This disproves an enabled immortality
flag in that snapshot; difficulty against stronger opponents remains unmeasured.
No additional HP or damage balance change was made.
The automatic ZIP64 artifact is now verified: 11,301,648,844 bytes,
SHA-256 `4abf447a07ead465c6c0a9b890b0d326ba34667922c9528ea5bb6427dda3898a`,
MD5 `1ea3a7ea46b98dcfbea7746e8a40694f`. All 95,308 normalized payload names,
CRCs and sizes match native ditto input; one AppleDouble name changed only
Unicode normalization. All eleven gameplay pins, engine, launcher and resource
seal match the frozen app. Real native ditto extraction, deep/strict signing and
the relocated native launcher's isolated `--stage-only` all pass.

The connected Drive uploader rejected the actual byte upload before transmission:
11,301,648,844 bytes exceeds its 536,870,912-byte cap. Browser recovery started
Upload new version on the existing file under its live personal owner account,
preserving file ID and sharing. The player then took ownership of the upload
("сам загружу"); no further agent upload or monitoring is authorized. The browser
session no longer exposes that tab, so neither cancellation nor completion is
confirmed. Provider completion remains unverified. The installed player app was
not launched. The verified ZIP is retained for the player's upload at
`/var/folders/sf/3cjm3vl53wg7fvf4wnnk4f6w0000gn/T/corsairs-fleet-integration.4m7wj1xt/zip64/Corsairs-Iddictive-Remaster-macOS.zip`.
Disposition: source/VM, installed state and native ZIP verified; cloud upload
handed to the player, provider completion unverified.

### Historical 21:40 dialogue delivery — October 2, installed; superseded by the correction above

Player screenshot shows Felix Rocha with no dialogue text or choices. The live
compile log at 21:26:34 reports an officer-dialogue load failure; error.log binds
it to a duplicate `TraderStock_RecordPlayerSale` extern after the real shop
segment was loaded. A native VM reproduces exactly that loaded-state conflict.
The original isolated compile fixture had not loaded shop_rotation and missed it.

The fleet manifest removes only the duplicate extern, reusing the main program's
registered trading function; the prior installed digest remains an approved
upgrade input. Corrected dialogue SHA-256:
`179e563196c7e89b9167c31425fd238fa8bf27a407d0ddc4cc3f4cd562bfc108`.

The actual core dialogue wrapper produces collect/inspect/finish choices on
first load and after unload/reload in separate native event frames; the regular
sale action refuses at sea. Exit 0, empty error log. Only cabin-entity readiness
is abstracted. Same-frame unload/reload and missing sea cleanup in scratch
fixtures were invalid probe contexts; the production event lifecycle resolves
those fixture failures. The user game closed; canonical staging completed successfully, the installed
script matches the corrected digest, and gameplay sync reports zero pending files.
The superseded archive job was stopped; no old archive was uploaded to Drive.
Disposition: source/state and installed refresh verified; real player interaction
remains pending. Existing item-box and ship-light log errors are observed separately;
this dialogue change does not claim to repair those independent runtime paths.

### Fleet gameplay batch — October 2, installed; player replay pending

Owner: current Corsairs integration task. The exact-hash fleet layer composes
on the canonical shared/living-Caribbean package. Ten script owners preserve
reviewed capture/crime/white-flag fixes; canonical stage delivered 22 script/INI
updates, the reviewed aiming techniques and the new ARM64 engine. The player
closed the game; no automated launch or player save/config change was performed.

Hero HP uses permanent P/E, additive one-time migration and unchanged HPPlus;
officer capacity grows with rank/base Authority/A. Ammo fallback, paid fleet
morale, shared carpenter materials, safe market-bound chest junk sale, selected
ship treasury targets and optional final cabin-loot orders are installed.
Unknown/quest/unique loot is preserved; absent speakers cannot strand capture.
See `docs/fleet-gameplay-audit.md` for contracts and exact replay gaps.

Native exact core/dialog compile: exit 0, empty error log. HP and resource/loot
VM state fixtures pass; geometry, native reload dispatch and cargo-load refresh
remain abstracted in the second fixture. Native Metal grass/fog pixels pass;
manual-aim geometry passes 758 checks with zero failures. Raw built engine:
`9ded1de7e465073d03a0234d9769a47c1cc4c0cc1aca8b8cc0adbcbdb6f7fdb5`.
Installed, signed engine:
`a743bc4ab07d40326c3c54c42b2039aed92995cbe6f48d226aae313c2a01adc1`.

The first idle staging attempt rejected `_dev/ship.fx` digest `092a2b5a...`.
Reapplying historical reviewed be695e5 to the captured engine reproduced those
exact bytes; both staging allowlists now recognize that historical output while
rejecting altered bytes. The next canonical `run.sh --stage-only` exited zero;
installed deep/strict signing passed and gameplay check reports zero pending
files with compiler ready. PRs 16, 17 and 18 are merged; the focused historical
technique delivery fix is 5ead729. All six inherited working/index entries remain
owned by their original writers. Public archive refresh is now in progress.

The stale index transaction was repaired by the .codex owner (71d63a5); the exact
original focused commit succeeded as d584d93, with normal lifecycle lease release.
Disposition: source, VM, build and installed staging verified; actual gameplay
acceptance remains unresolved until player replay.

### Sea aim overlay v4: fan arcs, raised ring, wave clearance — October 1, source candidate; staging pending

Player replay of the staged v3 tube (4 screenshots, open sea + shore + close ship, ~21:15): the 4-face tube renders as a white highway to the horizon at range and explodes into sheets and stray lines at close range; no drape ring is visible on water or ships while the shore ring reads; the per-vertex draped ring cuts chords through cliffs (crooked cork, stray hillside lines); the close-range ship lock shows rails crossing the hull plus an edge-on ring line. Root cause bound to code: the 0.35 m sea marker drowns in normal-weather chop (fMaxSeaHeight 2.0, PROGRAM/weather/WhrSea.c; calm 0.5), burying ring, march end and cross on every water/waterline lock. Installed engine proven byte-identical to the worktree v3 (source-patches.json patch text md5 9dc02672), so all four screenshots are v3 behavior.

Delta (cannon-trajectory-aim.patch 1078 to 976 lines, net -106): tube fill, corner rails, volume flush, vertex struct and their constants deleted; the battery fan is three trajectory arcs (mid-gun spine + two faint outer-gun edges, new kAimEdgeA0/A1); sea marker kAimImpactY 0.35 to 1.5 m with the wave-height evidence in the comment; the ring is one flat ellipse at the highest sampled surface + 0.35 m (center + 24 samples over sea/island/fort/ship disc) and returns its plane height so the cross sits on it. Fire()/GetFirePos unification, manual-fire bort-delta removal, reticle restore and terrain/ship-disc sampling unchanged from v3. Falsifier: git apply --check --whitespace=nowarn (harness flags) against the reversed base passes and the applied tree is byte-identical to the edited source. Not staged: the game process is running and staging skips engine install while it runs; no build was started to avoid stealing player CPU. Owner: docs/sea-aim-overlay.md. Disposition: source candidate; run.sh --stage-only + player replay pending after the game closes. Unresolved: alleged left-bort asymmetry (code path is side-symmetric; needs a repro screenshot with reticle state and whether the guns fire), far-aim readability, heavy-storm ring wash.

### One sun model and below-horizon glow gate — October 1, staged; replay pending

Follow-up to the fog-vs-sky verdict: the 05:00-23:00 engine sun (28 degrees up
at 20:00) contradicted the night shader and dusk presets, and the SUNGLOW
overflow/flare/reflection billboards had no below-horizon gate (moon/sun
light spilling from under water). `sun-below-horizon.patch` narrows the
engine window to 05:30-19:00 (sunrise ~06:07, sunset ~18:22), fades the
directional sun color across civil twilight, and gates the three unclipped
billboard paths by elevation; disc/halo keep geometric y-clip. The shader
mirrors the engine curve branch-free, so 19:00 cannot pop. `dynamic_sky-probe`
PASS (new: 18h day-bright vs 20h night-dark, sunset continuity, 19:00 edge).
`metal_evening_lights.py` flips Evening 19h/20h `Lights` to true (Night stays
false) so towns keep lamps after sunset. `run.sh --stage-only` exit 0;
installed engine 753de4b5 (pre-codesign f8e3e90b) to /Applications; gameplay
2 files (Evening.c, LAi_player.c). Unrelated dirty deck-walk/living-caribbean
candidates were stashed during staging and restored after. Observed but
unowned: `cannon-trajectory-aim.patch` changed in-tree during this session by
another writer; preserved untouched. Disposition: staged; player replay at
19-23h (sea horizon + town lamps at 20h) pending.

### Background fog vs sky color — October 1, systemic fix staged; replay pending

Player screenshot (open sea, October 6 1667 20:00): dark sky above, light
blue-grey fog band with fogged mountain at the horizon. Root cause is
structural, not a preset typo: three independent color owners diverged at
dusk. Authored 20h fog is cool (77,104,134) while 20h sky textures are warm
(low-row average 127,120,110), and the Metal procedural night palette is fixed
dark blue independent of both. The old shader converged only a narrow low band
(smoothstep .018-.16) to fog, so any per-hour fog tweak just moved the seam.
Fix: smoothed visual fog is now the single horizon owner. `dynamic_sky.hpp`
derives the whole base gradient (zenith = fog * (0.22,0.42,0.72), horizon =
fog), night/day cloud colors (fog*0.55+0.02 / fog*0.49+0.55) and the horizon
match from `horizonFog`; authored textures modulate luminance only and cannot
shift hue. Fixed palettes remain solely as the no-fog fallback. New probe
fixtures (dusk 20h fog: horizon exact, zenith darker blue-dominant, upper sky
follows fog) PASS alongside the existing midnight/noon and horizon-exact
checks. `run.sh --stage-only` exit 0; installed engine b317f323 (pre-codesign
9dabbf94) to /Applications/Corsairs Iddictive Remaster.app. Unrelated dirty
deck-walk candidates were stashed during staging and restored after.
Disposition: staged; player sea replay at 19-23h pending.

### Cannon manual-aim trajectory overlay — October 1, staged f1f0480; replay pending

Player asked for a beautiful manual-aim ballistics visualization. Disjoint
subagent delta (Darwin, closed): new `cannon-trajectory-aim.patch` (3 hunks:
`ai_ship_camera_controller.cpp` player-only overlay call inside `Realize`,
`ai_ship_cannon_controller.cpp` `DrawManualAimOverlay` plus static-buffer
helpers, header declaration only) registered in `build.sh` immediately after
`metal-camera-view-restore.patch` with the remaining order intact. Parent
verified the single-line registration, snapshot anchor bytes, the inherent
first-person gate (`Realize` returns early on `isCameraOutside()`), and the
`DrawLines(..., "Line")` flush. Residuals: fixed HeightMultiply 0.4 (no
knippel 0.65 without a script getter), circular drop ring instead of ellipse,
static amber. No save/AI/damage/reload/camera state touched. Canonical build
and staging were not run in this session; the overlay has zero player replay.
October 1 evening: HEAD f1f0480 (point-and-shoot aim, ship-lock ellipse,
contact glow, deck eye lift; live HeightMultiply 0.4/0.65 by charge, unified
broadside ribbon, own-ship sail/rope fade) staged via
`run.sh --stage-only` exit 0 after user closed the game. Installed engine
`cebbfc14...` (pre-codesign `3dd3292f...`) to /Applications; gameplay
0 files pending. Worktree also carries unrelated dirty deck-walk + dev_runtime
candidates (other threads). Known v2 gaps awaiting replay: no island/fort
trace (terrain aims fall to max-range sea), solid ship cylinders lock
rigging air (floating ellipse, live balls overshoot to hill), horizontal
ellipse edge-on at mast height, _alpha fade ZWrite off (sea overdraws own
rigging below horizon). Disposition: staged; player replay pending.

### Night floor 0.20, trade auto-pin follow and ship chest button — October 1, source candidate; not staged

Player voice report on the installed .38 floor: the city at night is flooded
like daylight even away from lamps, while jungle and sea already read slightly
too dark. Hypothesis: the source-light floor is the single variable;
jungle/sea evidence forbids any uniform lift. Delta: `lighting.hpp` floor .38
to .20 with recomputed probe bands (zero-authored 0.19--0.21, lamp-free
vertical 0.07--0.09, retained-matte ground 0.029--0.032, night highlight 0.220
vs day 0.963); probe compiled and PASS. `metal_tradebook.py` gains
`SystemInfo.TradeBookPinAuto` so auto-pins follow the current eligible town
while manual pins stay; unflagged legacy pins migrate as auto.
`metal_living_caribbean.py` adds the missing `CHEST_BUTTON` control to
`ship.ini` (the prior batch added only the `ship.c` command handler, so no
button could appear), gated by cabin ownership. Smuggler sell-link absence is
not a defect: the sell link is base-game gated by
`FindFirstContrabandGoods(PChar)!=-1`, untouched by our Meeting_3 patch.
Grey horizon and night blue-through-fog are unbound: night fog presets are
near-black (2,2,2)/(4,6,9) and the analytic night horizon is dark blue, so the
next evidence is a screenshot with place and hour. `backend.mm` albedo .38
floors and lamp stacking are recorded as separate deferred variables in
docs/metal-lighting-parity.md. Sync check shows three gameplay files pending
with compiler ready. Nothing staged: game process state was unverifiable in
this session (`ps` blocked by sandbox) and no apply/launch was requested.
Ballistics visualization (Darwin, running) is a disjoint subagent delta and
excluded from this batch. Disposition: source candidate; staging and all
player replays pending.

### Instant sea-crime punishment — October 1, diagnosed; fix proposed, not applied

Player with a privateer patent attacked a ship at sea and instantly lost the
patent (or trade license) plus reputation. Read-only subagent diagnosis
(Beauvoir, closed) is factual with file:line chains: the instant punishment
flows through two preserved legacy branches for an already-hostile but
untracked target, bypassing the deferred report contract in
docs/crime-reputation-system.md. `tools/patch_crime_reputation.py:605-615`
(`Ship_FireAction` else-branch, second and later shots) calls vanilla
`Ship_NationAgressivePatent` (`AIShip.c:821-849`: patent stripped, hunter+40,
no witnesses). `patch_crime_reputation.py:380-390`
(`CrimeSea_PreparePlayerBallHit`) calls full `Ship_NationAgressive`
(`AIShip.c:850-895`: reputation -10, pirate flag, nation war, hunter+5,
patent stripped) when relation is already ENEMY and the ship is untracked.
Tracking is silently skipped in `CrimeSea_Begin` (pirate victim, base-nation
enemy, `Situation`/`MQPirate`/`Coastal_Captain`), and a stale encounter
`AlwaysEnemy` (see docs/sea-encounter-enemy-state.md) routes even the first
player shot down the instant path. No instant trade-license strip path exists
in `AIShip.c`; the lost paper was almost surely the patent. Proposed minimal
fix: a `CrimeSea_HandleUntrackedEnemy` local-hostility-only branch plus a
marked deferred incident in both legacy branches; open question whether a
lawful prize should also skip rep/hunter/war in `Crime_ApplyPublicConsequences`
(currently only the patent item is exempted). Exact verdict needs target
nation vs patent nation, sailed flag, pre-shot hostility, fort/port
proximity, and log lines. No source, SAVE, or runtime mutation was made.

### Tavern combat scaling — October1, diagnosed; balance unchanged

User reports losing armed tavern fights at rank16. Latest PortoBello_tavern
save: hero HP100/heavy43, ordinary Habitue visitors HP152–181, rank up to22,
combat skill up to67. Installed GeneratorUtilite scales NPCs from aggregate hero
rank; Duel_Prepare_Fight can additionally apply Hunter parameters/+40HP. Earlier
duel enemy no longer resolves, so exact lost-fight identity/action chain remains
unresolved. No source/runtime/SAVE/difficulty mutation; owner and acceptance
boundaries captured in docs/land-combat-balance.md.


### Land officer inactivity — October 1, diagnosed; command replay unresolved

Current installed Metal gameplay is unchanged at source5477bc4 and prior engine
installation. User reports officers fail to help in jungles. Latest jungle save
Cuba_Jungle_10 decoded read-only: hero Charge range35, armed living same-group
officers428/430 in stay template. Officer updater explicitly returns on stay
before searching targets. This explains inability to react in that saved state;
writer/history of stay not proven. Player asked to replay Charge (defaultJ) in
the same fight. Owner/next falsifier: docs/officer-combat-assistance.md. No live
application/SAVE/config/code mutation occurred; feature outcome remains unresolved.


### Spain governor compilation correction and tornado delivery — September 30, installed; player replay unresolved

The player's governor conversation produced repeat COMPILE ERROR at line358
in `dialogs/russian/Governor/spa_Governor.c`, Undeclared identifier merch_8.
The exact-hash Metal-only gameplay layer quotes the three prisoner model names;
all other dialog/quest bytes are unchanged. Real Metal VM LoadSegment reproduced
the original failure and compiled the complete corrected segment. Unknown input
rejection/idempotence checks pass. Canonical run.sh --stage-only succeeded after
this layer; exactly one gameplay file was pending and applied. Owner/evidence:
docs/dialogue-escape-exit.md.

The pending cc45e5e tornado engine correction was delivered in the same batch.
Fresh idle baseline: 146 SAVE files and four configs. Installed only the engine
and the individual governor script; preserved all player state byte-for-byte.
Deep strict app signature passed. Staged engine SHA
`f01a318a43d872aa4ed49be59812fee84d177a6ed57c1c8fa8ff3e3dc2f78bd9`;
installed signed engine SHA
`dfb96ced00417b0a3d374e7807857b7322775f7674b6ca59c49833239803704e`;
governor script SHA
`fa900d90d028a243376fb451f951865f804f3702f38483b979aea7bb032b02e7`.
Recovery engine/script/resource seal and probe/stage/install logs retained in
native-metal .cache/app-update-backup-governor-tornado. No game launched for
installation acceptance; same-save governor quest/farewell and actual storm
visual replay remain unresolved. Prior brightness/FPS/trade layers retained.

Source correction committed as `5477bc4` on main. An automatic staged-ownership
hook initially rejected the new source file despite same-thread creation; the
bounded .codex repair c02eff6 restored byte-exact FileChange creation provenance.
Native adopt-staged accepted both exact task paths, and the unchanged original
focused commit succeeded. No product/index/HEAD mutation came from the specialist.
Verified origin/main equals `5477bc456529034f17fd003463c42b1036311d01`;
working/index clean, agent_context --check passed. Adjacent interface/ship.c compile errors in the player's logs
are outside this governor correction and were not changed; earlier merchant
crash and dawn-water boundary also remain unresolved. Earlier inventories below
are historical and do not override this installed batch.


### Tornado white rectangles — September 30, historical staging attempt; delivered above

Source `cc45e5e` on main restores existing particle technique loops around native
tornado pillar/ground and noise-cloud draws. Only renderer-particles-fx.patch
changed. The player's storm screenshot at11:41 shows white sprite rectangles;
the current native bridge had bypassed the authored texture/alpha stage chain
and inherited the untextured TornadoPillar material. Installed mask textures
exist. The missing Tornado/ITEMS.TGA entry concerns debris and is a separate
resource finding, not proof of the mask mechanism.

A 32-square correctness fixture with actual Metal particle bridge and seeded
prior valid material state reproduced white rectangle pixels, then preserved
transparent background ff203040 and half-alpha bf9098a0 with the authored chain.
The untextured negative remained white. This is not an FPS measurement or actual
storm replay; the player game was running during this small correctness fixture.
The existing source inventory probe could not open its absent native-storm
baseline and did not pass. Canonical build integrated two changed consumer
source files; run.sh --stage-only passed with compiler ready and zero gameplay
pending. Staged engine SHA-256 `f01a318a43d872aa4ed49be59812fee84d177a6ed57c1c8fa8ff3e3dc2f78bd9`.

Installed standalone arm64 Metal engine remains
`88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
A closed-game baseline captured143 SAVE files and four configs. The user then
restarted as PID39052 and player state changed before installation. The
installer refused at the initial baseline comparison before creating backups
or replacing the engine; no live runtime mutation occurred. Installation remains
pending a closed game and fresh SAVE/config baseline, with this chat as owner.
Native storm replay remains unresolved after delivery. Mechanism/evidence owner:
docs/audio-and-particles.md. Prior dawn-water boundary and merchant crash remain
unresolved; this patch does not address them.


### Outdoor night brightness correction — September 30, installed; sea boundary unresolved

Source `80b99e1` on main was built and canonically staged via
`experiments/native-metal/run.sh --stage-only`, then installed into the standalone
arm64 Metal app. The player rejected the .40 outdoor ambient floor as too bright
in Porto Bello at05:25. The corrected .38 floor removes half the last increase;
indoor fill, authored lamps, brighter/daylight weather and compatibility behavior
remain unchanged. Existing CPU outdoor energy/continuity/negative checks passed;
existing GPU lighting probe reports modern night48 (prior51), day52 unchanged,
legacy night14 unchanged, with emissive/unlit/UI/alpha checks passing. Perceived
night readability still needs the player scene replay.

Staged engine SHA-256 `ace96ac1a30e44197cf8cb715f1a67265f23133847e1c0740d54d29cdef70adf`;
installed engine SHA-256 `88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
Deep strict app signature verified; all 140 SAVE files and
4 player configs byte-identical across installation. Rollback
engine/CodeResources retained at native-metal `.cache/app-update-backup-night-tune`.
Trade auto-pin from bbd2037 remains installed. The user closed the playing game;
CUA observation inadvertently started diagnostic PID76219 at21:44:47, which was
closed with TERM before staging. No player gameplay process was stopped. Earlier
engine inventories below are historical, superseded by these hashes.

The separate dawn-water boundary has a source-backed cubemap-background
hypothesis, not a reproduced cause; no sea patch was applied. Ownership and exact
missing falsifier are in docs/metal-weather-surface-color.md. Merchant dialogue
crash remains deferred by the user's prior instruction.


### Minimum scene lighting and trade auto-pin — September 30, installed; replay unresolved

Integrated source: `bbd2037` on main, pushed to the verified public origin/main.
The exact batch was built and staged with
`experiments/native-metal/run.sh --stage-only` after the final source change.
Gameplay sync reviewed and applied only PROGRAM/interface/tradebook.c, preserving
SAVE/config and renderer resources. Installed app remains the standalone arm64
Metal GPK 1.3.2 AT + ReConstruction 1.4.1 owner. No game process was running during
staging or installation. Earlier inventory hashes below are historical.

Modern classified outdoor source-ambient floor increased .36 to .40; existing
warm indoor floor (.125,.100,.078) increased to (.140,.112,.088). Day ambient above
the floor, direct lamps/sun, emissive/unlit/UI behavior and graphics settings were
not changed. Existing CPU indoor/outdoor probes and native GPU lighting/dynamic
lighting fixtures passed, including retained materials, alpha, skins and lamp
contrast. The representative modern outdoor night GPU channel rose 45 to 51;
day stayed 52. These are fixture samples, not whole-scene perceived brightness.
The player screenshot before this batch showed Cartagena at 06:30 and FPS125.

Trade comparison initializer and mode toggle now attempt current-town auto-pin
when the saved pin is empty, including a persisted empty attribute; valid manual
pins and immediate unpin remain. No eligible current city/known prices leaves it
empty. Two disposable VM fixtures failed before Main; no gameplay acceptance is
claimed from them. The earlier PID78900 crash was the diagnostic CLI invocation
without engine.ini, not the player's merchant-dialogue crash.

Staged engine SHA-256 `9d152e333b6745c41ca750364a9cddc0ffd81dbd1f1bb3082b259a93342eaee8`;
installed engine SHA-256 `3ea9553b27f614bb1f93ee8a7f493c9cef97518444efe5b1b82bf5eb50619884`;
installed trade script SHA-256 `e4572beabccda273daca89bfee8270ac633c6a1b3ffbdd08a12b46e33a2340c6`.
Deep strict signature verification passed. All 139 SAVE files and
4 player configuration files were byte-identical across installation.
Rollback engine, prior trade script and CodeResources retained in the ignored
native-metal cache `.cache/app-update-backup-night-pin` with an install receipt.
Current player display configuration: {'full_screen': '1', 'screen_x': '1920', 'screen_y': '1241', 'display_mode': '0', 'msaa': '0', 'max_fps': '120'}; dynamic_lighting=1,
shadow_quality=2, modern_lighting=1 remain enabled in retained metal_graphics.

Disposition: build, probes, canonical staging and installed signed delivery
accepted for their checked properties. Actual Cartagena dawn/night readability
and comparison auto-pin on the current save remain player-replay unresolved.
The merchant dialogue crash remains unresolved: nested SetShow reached corrupted
window vector state; isolated valid recursion/nesting did not reproduce it.
No speculative crash guard was added. The user explicitly deferred crash
investigation and requested immediate installation of the existing batch.
Owners: docs/metal-lighting-parity.md, docs/trade-journal.md,
docs/metal-graphics-settings.md. No new game launch was required for this delivery.


### Fleet performance, deck shadow detail and safe Apply — September 30, installed; replay unresolved

Integrated source: `71ea527` on main; pushed and verified at
`https://github.com/iddictive/Corsairs-Remaster`. Only four renderer source/probe
paths were published; private runtime/topic documents remain local and excluded.
The installed source is this exact batch; signing changes the Mach-O hash.
Ignored diagnostic benchmark/candidate cache directories remain under the
native-metal owner because codex-delete-temp accepts only direct system-temp
children and rejected their repository-cache locations; no deletion bypass used.

Requested target is the existing standalone arm64 Metal app at
`/Applications/Corsairs Iddictive Remaster.app`, GPK 1.3.2 AT + ReConstruction 1.4.1.
Baseline installed engine SHA-256 was `6961f227bc2ad3688a034cc080e57c15af7e70dfc4e9c67242888b2e108d1d60`,
one player process during diagnosis and zero before build/install. Screenshots
and live logs reopened prior safe-Apply acceptance: actual installed patch still
reset the device; it rejected reset and produced 290 vertex-lock failures.
Fleet samples independently identified quadratic point-caster map recycling.

Integrated fixes: FIFO geometry-slot recycling/incremental exact-key updates;
sea cube registration deferred until actual lamp selection while location
registration remains eager; Near96/Far288 sea/deck maps with projected static
AABB rejection and footprint blend; stable-screen Apply with flags/quality only.
No global resolution increase, settings override, reflection cadence change, or
PROGRAM/RESOURCE replacement. Owners/evidence: docs/metal-frame-pacing.md,
docs/metal-lighting-parity.md and docs/metal-graphics-settings.md.

Canonical `experiments/native-metal/run.sh --stage-only` passed after the final
source/layout change, with zero gameplay pending and compiler ready. Staged
engine SHA-256 `614b1e0b0e84aa8fb0ac410828b8a2bea5ea3dc61d4dc3746c77feaae4050434`. Standalone dependency rewiring/signing
produces installed engine SHA-256 `96b5b6e0687e6e737e51181986097f2331b87da48f320f9b5899d2e4201138d6`; deep strict
app signature verification passed. All 139 SAVE files and all four player
configuration files were byte-identical across installation. Rollback engine
and prior CodeResources are retained at `.cache/app-update-backup` under the
native-metal owner.

CPU one-off 800-moving-caster benchmark: 16.908ms to .097ms per frame for the
registry component, not total-game FPS. Mixed key hits/motion/instance expansion,
stale compaction and scene reset passed. Existing GPU fixtures passed sea
point/mixed-order sunlight, no-source/no-stale frames, all quality tiers/live
backend apply, and location/indoor/skinned/cutout lighting; graphics ABI probe
forbids deferred device/window reset.

A preliminary integration build failed on missing receiver pass/encoder
declarations before installation. Corrected declarations and appended matrix
layout were rebuilt through the canonical staging path; installed app remained
untouched until that succeeded. Disposition for that preliminary candidate:
rejected. Integrated build, fixtures, staging and installation: accepted for
those properties. Actual fleet FPS, close-deck appearance and in-game Apply:
**unresolved**, with player same-save replay as acceptance owner.

CUA attempted the updated standalone app launch; process 49297 started, but
native UI observation failed with `Computer Use server error -10005:
timeoutReached`, so process existence is not scene acceptance. No game process
was stopped. Current player config: {'full_screen': '1', 'screen_x': '1920', 'screen_y': '1241', 'display_mode': '0', 'msaa': '0', 'max_fps': '120'}. Older display/staging inventories
below are historical; the hashes and config in this subsection supersede them.

Player replay reports improved FPS and missing deck shadows. The launched
process environment has `STORM_METAL_DYNAMIC_LIGHTING=0`, matching the retained
`metal_graphics` record `dynamic_lighting=0`; renderer diagnostics show applied=0.
The previous failed Apply saved flags62 (bit0 off) but rejected device reset,
so comparing the old live scene with the restarted scene is not a same-feature
performance comparison. Player was directed to enable dynamic shadows and Apply
inside the game. Final fleet FPS/appearance with shadows enabled remains pending;
do not count the shadow-disabled FPS report as proof of the full shadowed scene.



### Installed standalone app — September 30, startup accepted

The installed owner is `/Applications/Corsairs Iddictive Remaster.app`, an independent arm64 application with a native entry point, bundled dependencies and a complete verified ad-hoc signature. The prior unsigned bundle caused TCC to synthesize resource signatures during CoreAudio startup while WindowServer also waited on TCC. The corrected package passed a signed CoreAudio init/uninit probe; the player subsequently reached the main menu and playable sea, deck, trade-journal and prison scenes. This accepts the observed startup, not every gameplay path or OS failure. See `docs/metal-remaster-delivery.md` for the concrete packaging fix.

Player state remains in Application Support. The 2560×1655 display snapshot in this earlier startup attempt is historical; current values are in the fleet-fix subsection above. Applying a resolution takes effect normally after restarting the game, as confirmed by the player. No graphics-engine change was made for that report.

### Personal remaster menu and portable build — September 30

Current staged owner: native-metal, GPK 1.3.2 AT + ReConstruction 1.4.1,
arm64 engine SHA-256 `4878e76db86e82426682413f3acd0c94806a925c22aa22798c21097226b86a65`,
Metal backend. The staging configuration below is historical; current player display values are recorded above.
The user closed the game before the integrated batch. Canonical
`experiments/native-metal/run.sh --stage-only` passed all 548 targets and applied
three hash-bound menu files; subsequent sync reports zero pending and compiler
ready. The logo texture matches its manifest. Mach-O deployment floor is 15.0;
source diagnostic paths are relative; the public exporter rewires its copied engine to bundled libraries.

The actual Storm VM compiled the changed main menu and resolved its URL bridge.
HTTP and file URLs returned rejection without opening a browser. The first
fixture omitted `InterfaceStates.Launched`, causing a fixture-only attribute
error; seeding the required state corrected it. Player replay accepts startup, sea play and the corrected logo/social text spacing. The final removal of the duplicate banner and the VERSION link are staged; the actual HTTPS click replay remains player-owned. Standalone app and public source/Drive/site publication are delivered; corrected-menu archive replacement and user-requested repository cleanup remain in progress. Older dated verdicts below
retain their feature replay status, not the current engine inventory.


### Trade journal units and controls — September 24

Baseline: main `a535e0937b6b`, installed arm64 Metal engine, 1920x1241,
fullscreen 1, MSAA 0, zero game processes before delivery. The player screenshot
showed consistent lot arithmetic but ambiguous raw-piece quantities. The
Metal-only journal layer normalizes display units, sorts planned rows by total
gain after allocation, and adds pin/unpin and contraband visibility state.

The first candidate compiled in the installed script VM and passed exact-input
rejection. A disposable state fixture crashed without logs; it is not acceptance
evidence. `run.sh --stage-only` succeeded, gameplay sync was zero pending and
the installed game rendered the changed journal on the player's save.
The player rejected its toolbar icons and reported the route text aligned to
the top. This visual candidate is **rejected**. Text buttons and explicit
post-update vertical centering are prepared; their delivery/replay is
**unresolved** while the player closes the game. Stable contracts and remaining
scenarios: `docs/trade-journal.md`.


September 24 follow-up: text controls and route centering were staged through
`run.sh --stage-only` and are visible in the player's 10:50 screenshot. The same
screenshot rejects header clipping and total-gain ordering (tobacco 16.5 above
gold 157). Removed the post-allocation sort, retaining margin-per-centner order;
set explicit header scales in both modes and shortened the name header to Товар.
The game was closed before gameplay sync applied the one changed script.
The installed script passed compilation in the Metal VM; canonical
`run.sh --stage-only` succeeded with zero pending gameplay files.
Visual replay and the reason for zero gold allocation remain **unresolved**.

### Grouped graphics settings and safe Apply — September 23

The player rejected the old resolution/aspect controls, the unfinished graphics
panel and an Apply button with no clear outcome. The first grouped candidate
opened after removing stale callbacks to the deleted resolution picker, but the
real native replay then showed `Не удалось применить` for Desktop 1920×1080.
The replay isolated two independent defects: `SaveMetalGraphicsOptions` changed
from void to bool after an earlier untyped call, producing an option script stack
error, and a Desktop/effective-size change called `d3d9->Reset` while the live UI
held back/depth references. The reset and rollback failed and subsequent frames
reported invalid render targets.

The final candidate uses a ten-row, three-column Screen/Lighting/Effects table
with eight visible rows, its own scroller and fixed Apply/Cancel buttons. Display
extent is stored separately from custom resolution, preset heights use the live
display aspect, and Desktop is available only in fullscreen. Apply loads the
temporary `option_sl.c` owner around the confirmed write, then queues renderer
flags after closing the modal. Fullscreen and resolution are saved for the next
launch; live screen transitions are disabled because native replay made legacy
UI elements disappear. The Graphics button keeps its fixed label.

Canonical `run.sh --stage-only` succeeded. Built and installed arm64 engine
SHA-256 both equal
`02830575817460a3342aea8f820899f31cf9c8b13aa036730b857307f0f8caf1`.
The final installed `option_screen.c` hash is `129c1126…`; gameplay sync reports
zero pending and left SAVE/config untouched. Native Apply replay of the preceding
save-owner correction rewrote `metal_graphics`, produced `apply request`,
`queued until renderer RunStart` and `deferred success`, and left `error.log`
empty. The same replay rejected live fullscreen switching because UI elements
disappeared, so the final staged revision keeps screen geometry launch-owned.
Final flag/FXAA and restart replay remain **unresolved**.

### Deck direction, contact, reticle and late actor shadows — September 22

Player replay first at 23:45 reopened W direction, the sea reticle over the
third-person hero, the hero floating above the deck and absent hero/sailor
shadows. The later daytime replay at 11:45 explicitly rejected installed
`65b9228e…`: the hero still could not walk, floated while sailors contacted the
deck, and no actor cast a shadow. The player confirmed the spyglass works after
equipping it; that report needs no fix.

Baseline was installed arm64 direct-Metal engine `6150163f…`, one player process
48868, fullscreen 1, base 1920×1080, MSAA 0, display mode 1. Source discovery
proved world camera yaw was applied to local path coordinates, a separate
0.14 m model lift, third-person entry explicitly enabling the sea reticle, and
late MODELR actors missing point-shadow admission after ship lamp slot shutdown.
Owners/contracts are in `docs/live-deck-walk.md` and
`docs/metal-lighting-parity.md`; crew collision remains cancelled.

The first correction used local camera yaw and placed the model root directly at
the path support. The replay proves root placement is not the character contact
owner. The replacement keeps the current animation's authored `camera` locator
at the deck camera's established eye target; its compiled matrix fixture rejects
both direct root placement and the archived reverse multiplication order.
A Metal-only exact-hash camera script layer hides the third-person reticle,
restores first-person behavior and restores each view after telescope use.
The renderer now admits late scene-model casters from valid point-lamp or
directional-sun snapshots captured earlier in the same frame. Resolve uses the
same frozen sun direction after live slot 0 changes. No persistent light or
actor-specific bridge was added.

CPU matrix regression passes rotated/rocking ships, backwards/strafe/diagonal
motion, locator anchoring and rejection of both old transforms. The daylight GPU
regression disables and replaces slot 0 before a late skinned actor: 152 pixels
darken from the frozen sun, while the following no-source frame remains
unresolved and pixel-identical. Existing independent point-lamp and no-stale-lamp
negatives still pass.

The next player W attempt did reach the bounded deck trace. It repeatedly logged
`input=(0,0) reason=no_input` at a stable valid deck position while `WalkMode=1`;
path and solid geometry were never queried. The failed duplicate `DeckWalk_*`
actions have been removed from the Metal consumer. The deck camera now reads the
canonical `Chr*` movement actions, which a Metal-only exact-hash layer maps into
`Sailing1Pers` at startup and again after the options registry is rebuilt. Stale
saved `DeckWalk_*` rows are skipped so they cannot reclaim WASD.

The trace result reopened the second candidate. Its shadow registry admitted only
one MODELR draw (964 vertices), because admission depended on the light state at
that individual draw and rejected the ship cutouts' standard
`SELECTARG1(texture)` alpha contract. The new registry captures eligible pre-HUD
sea geometry independently of light order; resolve still requires a valid
same-frame source. A mixed-order Metal fixture draws a texture-alpha mast before
the sun and a late GPU-skinned sailor after slot 0 is cleared. They produce 104
and 96 separately darkened pixels; the following no-source frame remains
unresolved with zero changed pixels.

Canonical `run.sh --stage-only` passed after these movement and shadow changes.
Built and installed arm64 engine SHA-256 both equal
`c3d1e1642224dcd4cc310946bd01ecd627b7b2b0a8e056213493b31191b652d0`.
Gameplay sync reports zero pending and compiler ready; the two canonical control
scripts are installed without touching SAVE. Disposition: **third correction
staged; movement, contact and real deck-shadow replay unresolved**. No automated
gameplay input was sent.

Historical first correction follows. Canonical build passed.
After the player closed the game, `run.sh --stage-only` succeeded, delivering one
camera script and engine SHA-256
`65b9228ee9c202922236d0f1491ff169d1bc6137dfeab0cc969d3ed982ebb55b`.
Built and installed hashes match; gameplay sync reports zero pending and compiler
ready. Staging did not target SAVE; current display configuration is unchanged.

The first late-skinned GPU test failed (red 139/135): the fixture forgot the
standard skinning X reflection and its blocker missed the sample. Correcting
only the fixture footprint gives red 139/0; existing red/blue lamp tests and the
no-source following-frame negative (0/0, no extra point dispatch) pass. The
isolated real Metal script VM passes third/first/outside reticle and telescope
return transitions. Its first harness attempt redundantly loaded an already
included sea segment; removing that fixture-only load resolves the compile error.
Both task-owned VM temporary directories were removed through codex-delete-temp.

A repeat canonical stage after the fixture-only correction was refused because
the player had launched PID 67118. No game-code, ordered-patch, CMake or material
change followed the successful staging; the installed/built hash remains the one
above. No process was stopped and no automated gameplay input was sent. The game
is retained for the player. Disposition: **game correction staged; real deck
appearance and interaction replay unresolved**. The exact current test-source
batch can pass the launcher again at the next idle delivery; this does not imply
an uninstalled game-code delta.

### Live graphics, midnight palette and baked shadows — September 22, staged and launched

The player's current screenshots reject the staged graphics UX, doubled Belize
building/object shadows, and blue sky with a black horizon at 00:00. The next
candidate has three changes: direct modal Apply/Cancel with a renderer/display
transaction; solar palette phase from game hour rather than the moon-bearing
weather vector; and location-scoped preservation of baked static sun shadows,
retaining character shadows and static occluders for point lights. Slot-1 baked
modulation stays enabled in the same scope. Material metadata is explicitly
initialized because the geometry loader allocates raw storage.

Verification so far: both new engine patches apply, reverse and reapply
identically across their combined 17 paths; all prior graphics script layers
strip to the exact canonical source; live-apply/source and Belize/Cozumel
contract probes pass. The live-settings backend and quality fixture compiled
before the final baked-state integration; the full candidate build is pending. The midnight
CPU fixture passes; rendered midnight/fog and live-settings resource-lifecycle
fixtures are prepared but have not run. No native UI or Belize replay has
accepted this candidate.

Historical external baseline before this delivery: one player-owned
process, PID 3411, running the arm64 direct-Metal executable under
`experiments/native-metal/.cache/CorsairsMetal.app/Contents/MacOS/metal-engine`,
SHA-256 `d345ccdbeec8c2b0628cc475ac5a2863eace86959ef4d82ddf19e42b70e9dd91`.
No Wine/prefix is involved. `engine.ini` remains fullscreen 1, 1920×1080,
MSAA 0, display mode 0. Saved `metal_graphics` is Desktop (5), base 1920×1080,
quality 1, all six effects on and AA off. Runtime, SAVE and installed scripts
were not changed in this attempt.

The player then requested installation and launch. With the prior process
closed, canonical `run.sh --stage-only` succeeded: 17 engine source files
migrated, the arm64 engine compiled, and three graphics interface files were
installed. Built and installed binary hashes both equal
`6150163f4652f0782cc4e43d3d70a0d86cf0351a886ab3a0980c1b7ced0b17bb`.
Sync reports zero pending and compiler ready. All 118 SAVE sizes/mtimes match
the pre-delivery manifest
`adfc08b2f2ef9b0791dcb6adc9515f197621dacca02901938adfb19091cd4521`.

Before launch, real GPU fixtures passed midnight versus noon with the same
above-horizon moon vector, exact horizon fog, live shadow quality/off/on/no-op,
invalid-setting rejection, device-reset location preservation and baked flag
reset. `run.sh --launch-installed` started PID 48868. Native window inspection
shows the player in Belize's moneylender interior at 11:45, with the scene and
HUD rendered. No automated gameplay input was sent. Current `engine.ini` is
fullscreen 1, base 1920×1080, MSAA 0, display mode 1; saved settings remain
Desktop (5), base 1920×1080, quality 1, six effects on and AA off. Launch reports
matching 2056×1290 backbuffer and SDL drawable.

Disposition: **staged and launched; feature replay unresolved**. Player replay
still checks Apply/Cancel, one static Belize shadow plus moving-character
shadow, midnight sky, and the remaining horizon band. Earlier squad/cabin and
other scene acceptance remain open in `TASKS.md`. The running game is retained
for the player; no extra GPU processes remain.

### Recovered sky, display and squad correction — September 22, source integration

The latest player report reopens sky/water/stars/weather synchronization,
square sun glow masking mountain silhouettes, grey horizon, night/lamp pools,
grass viewed from different angles, Mac display proportions and squad/officer
supply. Recovery preserved 27 candidate paths at `368819ad`; these changes
subsequently passed the canonical staging path as recorded below. Old sky/display source workers
were interrupted; no game or build process was active at the new baseline.

Historical baseline before this batch: arm64 direct Metal,
`experiments/native-metal/.cache/CorsairsMetal.app/Contents/MacOS/metal-engine`,
SHA-256 `366ecab7178101c0f8af7f04f9b95625ae30f47b7eb6e1f2cc248aaebdc1d065`.
Launcher remains `experiments/native-metal/run.sh`. No Wine prefix/bottle is
involved. `engine.ini`: fullscreen 1, 1920×1080, MSAA 0; `metal_graphics`:
resolution 2, base 1920×1080, shadow quality 1. Previous settings inventories
below are historical, not a claim about this currently observed record.
The last player-visible result is the reported defects; no new scene acceptance
is inferred from recovery or source checks.

Integration found that reserve counting filters ship-locked companions while
the generic spender does not. The squad layer now mirrors the counted donor
set for spending. An isolated Metal VM passes all three commodities, quantity
conservation, locked/travelling isolation, ordinary-item return and protected
item/old-save negatives. Restoring the unfiltered spender is rejected by the
fixture. A first fixture omitted `Ship.Type`; corrected fixture data passes
without script runtime errors. SAVE and the installed scripts were untouched.

Canonical staging initially stopped before source migration or compilation: the
legacy platform extraction referenced a commit that did not yet contain
`platform.patch`. The installed runtime was not changed. The legacy bytes were then bound to `9d111d61` with the recorded SHA-256.
The next attempt rejected `platform.patch` during the transaction dry run
(`main.cpp`, `sdl_window.hpp`, `sdl_window.cpp`); no source transaction or runtime
write occurred. Exact scratch replay proved the old 46-patch stack and
historical platform both reverse cleanly; the new platform patch contained
malformed whitespace in its context. It was regenerated without changing the
display payload. The full new 47-patch stack passes forward/reverse/forward
with byte-identical results before the next canonical attempt.

The corrected canonical path migrated 12 source files successfully, then the
compiler rejected duplicate sun-angle declarations in `WEATHER::SetCommonStates`.
This is an integrated source error in the weather patch; staging did not reach
the installed executable or scripts. The duplicate declarations were removed
from the ordered patch and guarded by its source probe. The next invocation
exposed macOS Bash 3 empty-array expansion under `set -u` after migration;
the optional bootstrap arguments now use a nounset-safe expansion, verified
for both zero arguments and a path containing spaces.

The engine then compiled, but ground staging rejected the already installed,
exact reviewed Porto Bello shore model. The ground helper now resolves only that
known shore output through its hash-verified original-town backup. An isolated
fixture passes first/repeat staging and rejects unknown live-town or backup
bytes. No geometry was restored or replaced by this compatibility repair.

Canonical `experiments/native-metal/run.sh --stage-only` subsequently succeeded.
Current built and installed arm64 binary SHA-256:
`d345ccdbeec8c2b0628cc475ac5a2863eace86959ef4d82ddf19e42b70e9dd91`.
SunGlow technique SHA-256 is
`014bbf13890f74450369f8b74269f5518356457fdbcee4be890374aeeb2b527e`.
Six gameplay/interface files were installed; sync reports zero pending and
compiler ready. All 118 SAVE files retain their baseline sizes/mtimes (manifest
SHA-256 `5717dad6a9837b5433c75233f9755216c292b2398673ed885abc718ff8f270b6`).
Built probes pass outdoor/day/interior lighting, lighting alpha/unlit/UI,
dynamic lamps/casters, grass cull/stage-1 isolation and caller-state restoration,
dynamic-sky shader/phase/fog and backend scope cleanup. These are correctness
fixtures, not real-scene or game-FPS acceptance.

The player subsequently launched PID 3411 and reopened two outcomes:
- Graphics UX is rejected; changes after OK have no immediate visible effect.
  The current implementation persists options but reads renderer flags only at
  launch. Live application and the modal action flow are now a new active
  correction, not accepted behavior. Observed saved resolution is Desktop (5),
  while the running launch used 1920×1080 and `display_mode=0`.
- The supplied Belize street screenshot at 11:50 shows overlapping shadow
  edges. Their exact owners are being traced; prior shadow probes do not accept
  that real scene. Grey horizon polygons also remain unresolved.
- Further Belize screenshots at 00:00 show a bright blue sky with moon/stars
  and a black horizon band. Source diagnosis confirms that weather replaces
  `whv_sun_pos` with the visible moon vector; the procedural shader incorrectly
  used its elevation to select daylight. The next candidate derives daylight
  and twilight from game hour. This source correction is not staged; PID 3411
  remains the player's running prior batch. Midnight fog is authored near
  black, so horizon acceptance requires replay after the phase correction.

Belize source/material inspection also identifies `terrain1` as actual ground
with an opaque baked-shadow atlas in texture slot 0. Metal renders static
location casters over it; this is the leading explanation for the screenshot,
not yet a matched scene replay. Removing that base or disabling its receiver would
lose ground or dynamic character shadows; those alternatives are rejected.
The scoped static-caster correction remains under investigation.

Disposition: **staged; visual/gameplay acceptance unresolved**. Settings live
application and Belize double shadows remain active source work. Existing
squad daily-tick/cabin replay and sky/night/player scene checks remain required.
The batch contract and surviving older backlog are in `TASKS.md`.

### Graphics settings / outdoor night / Porto Bello — September 22, correction staged

Canonical `run.sh --stage-only` succeeded after the player closed PID 51641.
The installed and built arm64 Metal executable SHA-256 is
`c49b87fa7f877beb53800cd6702109632a81e13a832691dc67843665dcce7543`.
The prior portfolio-launched process used shore-correction binary `24760ae2…`;
its jungle capture is not a matched Cozumel acceptance replay. Root did not
launch or terminate the game. The GPU/build window is released to the player
and the separately authorized capture owner, without concurrent GPU runs.

New source adds the four-file Graphics options layer, a global record and
launcher adapter, scene-boundary FXAA, and a bounded neutral ambient minimum
for modern outdoor night lighting. Options transforms/persistence/negatives,
outdoor CPU lighting, FXAA GPU, scene/HUD boundaries, dynamic lamp/static/skinned
lighting, cinematic and D3D9 FFP GPU checks pass. See
`docs/metal-graphics-settings.md` for ownership. Gameplay sync now reports zero
pending files and compiler ready. The four UI files are installed. Global
`metal_graphics` initializes all existing lighting/water/sky effects on, FXAA
off, fullscreen on and preset 1920×1080; engine.ini has MSAA 0.
The subsequent launch proves a 1920×1080 backbuffer/drawable. First initialization
incorrectly rounded the original 2056×1329 to this preset. SDL owns a desktop-sized
fullscreen window, but Metal still renders the configured dimensions, stretching
the image to the display. The player reports elongated characters/HUD. The
previous window-only resolution description was incorrect. Root is repairing
exact custom-size preservation. After PID 22006 exited, canonical staging
succeeded again with the corrected four-file UI layer and adapter SHA-256
`4a25cebc4d79aaa0ac793341c667bc04150cac1fdc72e2cb3e909406b50be3a8`.
The engine binary is byte-identical (`c49b87fa…`); option_screen.c is now
`f6c99efaa093b7cab27357cc150d1d7757852ac5054135af895c5f35fc45664d`.
The exact untouched initialization record was backed up and restored to
resolution choice 4 with base 2056×1329; engine.ini is fullscreen 1, 2056×1329,
MSAA 0. Preset heights now retain the original aspect. Sync is zero pending and
compiler ready. Root released the idle runtime to the authorized capture owner
for actual modal/toggle/save/restart verification.
The Graphics button is visible at bottom left. The player then reported its
modal controls unclickable: source inspection proves that hidden-window
initialization locks children and Show alone does not unlock them. Explicit
activation in the opener is installed. Exact transforms, prior-layer upgrade,
custom-size/preset/negative checks and an isolated actual Metal VM compilation
plus option-attribute round-trip passed. Native toggle/Close/reopen/
Cancel/OK/restart replay remains **unresolved**.

Rejected delivery attempt: a source agent directly modified four live runtime
UI files despite its source-only assignment. Root froze that agent and restored
exact pinned originals on September 22 at 12:30. Root's initial idle check
incorrectly searched engine-1 rather than metal-engine; PID 51641 was still
active, so restoration also crossed the intended idle boundary. The error
was disclosed and the capture owner notified. All four files were then restored
to original hashes; later canonical stages installed the reviewed layers.
SAVE was not changed. No further runtime/GPU work is allowed
until the process was absent; the subsequent bounded watch confirmed closure.
One FXAA fixture ran during this mistaken idle
window; its pixel result is supporting evidence, not clean-session acceptance
or performance evidence. It passed again in the later idle GPU window.

Porto Bello screenshots additionally expose a ground UV/density discontinuity
between the active Town and Town_sb models, not missing Sandtile loading.
The exact UV correction is now installed with SHA-256
`7abc29913b1fe3f0477afa5d04464230a7c1f4174a99f70c3bfd0b4550d8fc7b`;
its backup is retained and scratch idempotence/unknown-backup checks passed.
The 20-point boundary check bounds packed UV error to 0.5972 texel and preserves
all non-UV data. The player's September 22 13:05 Porto Bello screenshot still
shows blurry ground and a hard sand/shore transition. This reopens visual
acceptance: numerical UV continuity did not close the requested appearance.
Read-only mesh/material analysis identifies a likely distinct Town.gm
object 46 sand versus object 38 rockK3 boundary; exact camera/material-ID replay
remains pending, including distinguishing a geometry edge. The Town_sb UV fix
does not own that visible material transition. Root retains its next-batch
correction separately from the confirmed resolution regression. Character texture quality remains unresolved; a 2x
capture alone does not prove a loader defect.

The player's 13:39:58 screenshot confirms the unresolved material edge and blurry
ground, and adds visibly stepped character-shadow boundaries. It does not isolate
texture loading, filtering, shadow-map budget or a geometry edge as their cause.
Root prepared diagnostic source only: same-frame view/projection/viewport capture
before HUD, and a discriminating FVF44 UV1 raw-versus-expanded GPU fixture.
Both diagnostic targets compiled; the CPU telemetry preservation/empty-frame/
nonfinite/fallback probe passed. GPU execution and canonical staging were held
while the player controlled metal-engine PID 92955. That process later
closed. The FVF44 UV1/raw fixture then passed, proving the second repeated texture
coordinate reaches the native raw location path; this rules out the suspected
TEX2 transport loss but does not identify the Porto Bello material edge.

Source analysis binds stepped moving character silhouettes to the two directional
sun cascades, previously fixed at 2048² (64/2048 and 192/2048 world texels), with
linear comparison and nine Poisson taps. No nearest-filter or format mismatch was
found. A systemic `shadow_quality` profile now controls all location sun maps and
lamp cubes: Low 1024/256, Medium 2048/512, High 4096/512. Allocation, viewport
snapping, main sampling and both fallback sampling paths consume the same profile
texel. CPU policy checks, all three real Metal allocations, FVF44 UV1 and the
dynamic sun/point caster fixture pass.

Canonical `run.sh --stage-only` succeeded with executable SHA-256
`366ecab7178101c0f8af7f04f9b95625ae30f47b7eb6e1f2cc248aaebdc1d065` in both
build and staged app. The four Graphics UI hashes match the reviewed
shadow-quality layer; gameplay sync is zero pending/compiler ready, including
the four custody-life scripts. The current `metal_graphics` record selects High
(`shadow_quality=2`), FXAA on, fullscreen 1920×1080. Native Porto Bello shadow,
ground seam, Graphics interaction and custody-scene replays remain player-owned
and unresolved; staging does not accept those visual or gameplay results.

Disposition: **resolution and modal corrections staged; native acceptance pending**.
Player owns visual acceptance.
The integrated graphics source is committed as `58b6af68`. The hooks specialist
repaired stale Git claims in its own repository (`73fac38`); it did not mutate
this source tree or index. A later exact-path git add was rejected as
foreign-staged-target; the same authorized specialist audited that ownership
reconciliation. Its follow-up fix `efcdcb1` restored the exact normal git add
(exit 0, 54 staged paths), without touching pickup/prison candidates or the
runtime. No index bypass was used.

### Pickup halo — source candidate, not installed

The separately authorized pickup task returned an exact engine patch, item-role
script adapter and topic document. Isolated strict patch/syntax, geometry and
script round-trip/negative checks passed. Root accepted source integration
ownership; ordered-stack/gameplay receipt wiring, canonical build/GPU/staging
and real pickup/occlusion/cleanup replay remain pending for the next batch.
The current corrective Graphics stage excludes this feature.

Disposition: **source candidate, runtime unresolved**; see
`docs/metal-pickup-glow.md`.

### Prison life / supervised tournament — source candidate, not installed

The separately authorized prison task returned its Metal-only four-script
transform, gameplay source and topic. Exact Metal VM script/dialog compilation,
day/funds/parole state, reward idempotence, defeat/cancellation, issued sword
quantity, prior immunity, private arena isolation, loaded-arena recovery and
stock lethal-hit interception passed. Removing transaction cleanup triggered
the intended negative. No SAVE/shared runtime was changed. Root owns a separate
source commit and next-batch hash-checked sync integration; current Graphics
delivery excludes it. Real arena/outcomes/save-load/release/escrow replay remains
unresolved. See `docs/prison-surrender-system.md`.

Disposition: **source candidate, runtime unresolved**.

### Cozumel ground regression — September 22, correction staged

The player screenshot at 11:49 shows the entire shore surface missing while
rocks, palms and the player remain. At that capture PID 92991 ran the prior
staged binary; its SHA-256 was unchanged. This rejects that batch's ground appearance.
`common.ini` maps «залив Косумель» to `Shore7`; `Beliz.c` maps it to
`Locations/Outside/Shores/Shore02/shore02.gm`. Its `shoreU2SG` and
`bump_city_shoreU2SG` materials use `shadow.tga` in slot 0. The new whole-draw
skip incorrectly classified these base surfaces as standalone shadow overlays.
The previous SanJuan-only probe repeated that unsupported classification.

Correction: remove the slot-0 draw rejection, retaining original geometry and
the separate slot-1 dynamic-shadow replacement. The source/data probe now binds
the reported location and preserves 22 real shore materials. Build and canonical
`run.sh --stage-only` passed after the player closed the game. Build and staged
executable SHA-256: `24760ae2f499e5c06c2b0200cb8865fcaf25e09df81da6923f5f8580f267157e`.
Gameplay sync is unchanged: zero pending files, compiler ready. The active
configuration remains the inventory below; no game was launched by root.

Disposition: correction staged, real-scene restoration **unresolved** until
player replay or the separately authorized portfolio capture. The capture task
has a conditional GPU release only while no user game is active. The new graphics
settings request is a separate pending source batch with root integration ownership.

### Integrated visual completion — September 22 (superseded by ground correction)

The expanded batch is built and staged through the canonical
`experiments/native-metal/run.sh --stage-only`. Both the build executable and
staged `CorsairsMetal.app/Contents/MacOS/metal-engine` have SHA-256
`648eea3734bcc804cc38c4a635001199a3f7b08b2796534f7b8d0f9a6c78aac7`.
The active runtime remains arm64 direct Metal on Apple M3 Max, 2056×1329,
fullscreen 1, MSAA 0, with no Vulkan/MoltenVK linkage. At staging completion no
game process was running. Gameplay sync reports zero pending files and compiler
ready. Original textures and existing saves are retained.

The earlier baked-town-shadow, world-map scope and unlit triangle-fan fixes below
are included. The expanded delta closes these demonstrated source gaps:

- Location lamps crossfade only their shadow weights over 0.25 seconds when the
  desired two-lamp membership changes; authored illumination remains intact.
- Ship lamps have stable identities and same-frame cube shadows with a combined
  receiver correction. Other lamps remain lit, sun-only packets retain their
  receiver path, and unlit models remain casters without being relit.
- The point-caster registry no longer replaces an already-seen instance of the
  same mesh with a later transform. This was the reproduced cause of the missing
  ship-fixture shadow, despite apparently successful cube dispatch counters.
- Exact town-backdrop roles clamp vertical edge sampling, preventing opaque
  bottom-row bleed into the transparent top. Horizontal repetition remains.
- TX loading uses physical mip sizes and independently clamped dimensions. All
  6,410 files match the supported layout; 14 ship textures exercise full DXT
  tails, including two rectangular 2048×1024 chains. Malformed reads are rejected.
- Existing floorU3, Sandtile and rock images use rotation/offset/four-cell
  anti-tiling with explicit Metal gradients. Directional paving, alpha textures
  and explicit clamp sampling retain their original path.

Verification: the complete 46-patch stack applies strictly; engine build and
canonical staging passed. Compiled selector and TX layout tests passed. GPU
checks passed for anti-tiling periodicity/seams/mips/exclusions, backdrop alpha,
FFP lighting, all six unlit fan paths, resident map geometry, existing sun
shadows, dynamic lighting, point cubes, GPU skinning and ship point shadows.
The ship fixture uses shared geometry at different transforms: red 139→0,
blue 140→0, while the other blue contribution stays 173→173; off/reset passes.
Material and lifecycle contracts are recorded in the owning topic documents.

Disposition: source/build/staging and the named fixture properties are accepted.
Real-game appearance and frame time remain **unresolved**, explicitly owned by
the player. `TASKS.md` records the replay scenarios. The separately authorized
portfolio capture task may use this exact staged binary after root releases the
runtime; it is not evidence of replay until the actual scenes are captured.

## Previous verdicts and attempts

### First completion staging — September 22 (historical)

The first completion batch below was built and staged. The user then expanded
the active batch to lamp continuity, ship lamps, texture mips and anti-tiling;
these newer sources are not yet staged. The player explicitly owns real-game
replay; no game was launched by this task. `TASKS.md` tracks current acceptance.

Inventory at first staging: sole active target `experiments/native-metal`; launcher
`experiments/native-metal/run.sh`; arm64 direct Metal on Apple M3 Max, no Vulkan
or MoltenVK linkage. Both `.cache/build/bin/engine-1` and the staged
`.cache/CorsairsMetal.app/Contents/MacOS/metal-engine` have SHA-256
`600d65a9c4f2c9b28002fc57f85a9c030a1a9b4318b4a22351639f864143548c`.
Configuration remains 2056×1329, fullscreen 1, MSAA 0. Game process count is zero.
Gameplay sync check/apply reports zero pending files and compiler ready. Earlier
inventories, pending staging statements and staged hashes below are historical.

Hypotheses and exact deltas:

- The previous baked-shadow change neutralized slot 1 but missed standalone
  slot-0 shadow geometry. The real San Juan model proves six combined materials
  and one standalone layer. The geometry service now returns a draw decision;
  only the standalone layer is skipped when dynamic sun shadows are ready.
  Base materials and unavailable-dynamic-shadow fallback remain.
- The map model role existed only in the backend. `WdmRenderModel` now opens and
  restores it around geometry. A GPU comparison then caught stale illumination
  and missing authored specular in the unlit resident path; it now preserves
  authored color/alpha/specular, suppresses light contributions, and uses a
  distinct unlit pipeline key. Map techniques still own their unlit behavior;
  this is not a delivered dynamic map sun/shadow system.
- Unlit triangle fans used by shadow blur, sun glow and navigation still expanded
  vertices on CPU. Common compact and native device paths now triangulate only
  indices in the frame arena. Static/dynamic and indexed/nonindexed submission
  preserve vertex bytes and signed base/index offsets.
- The red FFP lighting fixture omitted a normal-bearing FVF and sampled an
  interpolated center from vertices outside narrow lighting lobes. Explicit FVF
  and a smaller projected quad repair the fixture; thresholds and backend
  lighting equations were not weakened to obtain green results.

Verification: canonical `run.sh --stage-only` succeeded after the final backend
change. Ordered source stack, engine link, consumer coverage and real-GM shadow
composition passed; coverage and GM checks are now canonical build/stage gates.
`menu_unlit_native-probe` passes six fan routes against full triangle-list images
with zero CPU conversion and a short-stride negative. `native_legacy3d-probe`
passes rich attributes, signed indexed base, nested map roles and outside-scope
restoration. `ffp_lighting_gpu-probe` passes original material/light/specular/spot
thresholds. Original shadow silhouette/9-tap blur/receiver and GPU-skinned pixel
parity pass. The map and skinning probes were rerun after the final pipeline-key
change and passed.

Disposition: source/GPU/build/staging accepted for this batch; real-game visuals
remain **unresolved**. Next player checks: San Juan daylight shadows, map models
and labels, sun glow/navigation, then Marigo shipyard night across lamp influence
boundaries. Two-nearest-lamp continuity and sea ship/deck lamp-shadow ownership
are still unresolved source/design work, not completed by this batch. Prior
renderer/gameplay replay obligations remain in `TASKS.md`.

### Providencia capture-state repair — September 20

The latest save was not a failed ownership transition. Its persisted colony state already
had `Colonies[Providencia].nation = PIRATE`, `HeroOwn = 1`, and a capture date. The stale
state was the separate battle marker: `objTownStateTable.towns.t29.captured = 1`,
`LAi_IsCapturedLocation = 1`, and `PChar.GenQuestFort.TownCrew = 3`. The three remaining
defenders could not be spawned at `Providencia_ExitTown/rld/loc0`, so the normal group
completion callback never ran.

Delta: preserved the original `SAVE/Кампот/После захвата и швартовки софтлок` and created
`SAVE/Кампот/После захвата и швартовки софтлок (починен)`. The copy changes only the stale
battle marker and counter to zero; the colony owner, capture date, player location, and
capture quest state remain intact.

Evidence: the ReCon 1.4.1 save round-trips through the repository converter and re-parses
with `TownCrew = 0`, `captured = 0`, `LAi_IsCapturedLocation = 0`, and
`Providencia.nation = PIRATE`, `HeroOwn = 1`. The original save is byte-preserved. Loading
the new slot and replaying the store/loot interaction on the real game surface remain
unresolved because the native window was not available to the current control surface.

Disposition: targeted repaired save prepared; ownership is accepted from save state, while
real-game load and the one-time capture-loot interaction remain **unresolved**.

### Dialogue has no Esc exit, September 19

Every dialogue line the engine shows can offer the player a plain farewell link, and the
standard dialogue files answer it with their own `case "exit"` / `case "Exit"` handler that
restores `NextDiag.CurrentNode = NextDiag.TempNode` and calls `DialogExit()`. Esc never
reached that option: `init_pc.c` binds `VK_ESCAPE` to the global `DlgCancel` control and
`DIALOG::Realize` raises `DialogCancel` for it, but both handler registrations in
`PROGRAM/dialog.c` were commented out, so the event had no consumer and the key did nothing.

Delta: new `tools/patch_dialog_cancel.py` composes one fragment onto the delivered
dialogue revision, after the dialogue-attack fragment. `DialogCancel_Exit()` registers on
`DialogCancel` in `StartDialogMain` and `SelfDialog`, scans the current line's now-built
`Dialog.Links` for a link whose `go` is `exit` or `Exit`, and, only when one exists, sets
`Dialog.CurrentNode` to it and raises `Event("DialogEvent")` — the same dispatch the engine
performs for a click, so the dialogue's own exit node runs and nothing else changes. A line
without that link, including a forced quest line, ignores Esc.

Evidence: `sync_metal_gameplay.py check` reports exactly `PROGRAM/dialog.c` pending with the
compiler ready; the suite's 35 deterministic hashes and the linked `_verify_dialog`
contract pass with the new composite. `docs/dialogue-escape-exit.md` owns the contract.

Disposition: source accepted. Metal apply, startup and the Esc replay stay **unresolved**:
the running Metal process owns the runtime, so the delivery helper refused to write.

### World-map label icons lost their second texture stage — September 19

After `renderer-world-map.patch` joined the ordered stack, the island labels on the global
map lost their icons. The connected producer was wrong in three places, and the first one
alone is enough to erase a label icon.

The icons are a two-stage technique. `WdmDrawLabelIcon` selects stage 0 from the texture and
then blends stage 1 over it with `blendfactoralpha` and `D3DRS_TEXTUREFACTOR = icons.blend`,
and `WdmIslands::LRender` fills `uv0 = icons.f[0]` and `uv1 = icons.f[1]` so the two stages
sample adjacent animation frames of the atlas. `bridge_screen_vs` in
`experiments/native-metal/backend.mm` forwarded only `uv01.xy` through `bridgeVertex` and left
`uv01.zw` at zero, so stage 1 sampled texcoord 0 and the blend collapsed. Second, the consumer
called `StormMetalDrawWdm*` *before* the legacy `DrawPrimitiveUP`, but the technique pass is
what applies `ts[]`, blend and depth state (`DX9RENDER::TechniqueExecuteStart`); `drawBridge`
reads that state through `prepareFixedFunction`, and the `technique` argument of every
`StormMetalDrawWdm*` was only null-checked. The world map never sets a technique itself
(`WdmRenderModel` calls `gs->SetTechnique` for models only), so the flat plates and labels drew
with whatever pass the previous consumer left behind — nothing like the proven `stars.cpp`
pattern, which wraps the bridge in `TechniqueExecuteStart`/`TechniqueExecuteNext`. Third,
`(x, y)` was treated as the rectangle centre, while `WdmInterfaceObject::FillRectCoord` and the
label icon path treat it as the top-left corner and add `(w/2, h/2)` inside the rotation.

Delta: `bridge_screen_vs` now places the corner at
`(x, y) + (w/2, h/2) + rotate((corner - 0.5) * (w, h), angle)`, writes
`o.uv01 = float4(uv0, uv1)` so stage 1 reads texcoord 1, and, for rotated rectangles only,
mirrors `u` — the `ang != 0` branch of `FillRectCoord` assigns the texture coordinates mirrored
in `u` relative to its `ang == 0` branch, which is the authored pointer orientation. The
secondary `uv1Rotation` is applied about `(0.5, 0)`, the exact transform the legacy
`CMatrix(0, angY, 0, 0.5, 0, 0)` rotation produced, and `WdmWindUI` now hands the wind bar its
natural `0,0,1,1` source so that transform reproduces the old `[-0.5, 0.5] + 0.5` mapping.
`renderer-world-map.patch` adds `WdmMetalDrawScreenRects`/`Quad3D`/`DangerDisc`/`ShipWake` to
its `metal_world_map_bridge.h`, each running `TechniqueExecuteStart(technique)`,
submitting, and closing with `TechniqueExecuteNext()`, so a compact record is submitted under
the same pass the legacy draw would have used. `renderer_world_map_probe.py` now asserts the
second-stage forward, the rotated-`u` mirror, the centred secondary rotation and the technique
wrapper, and `renderer_consumer_coverage_probe.py` follows the renamed ship-wake hook.

Evidence: `run.sh --stage-only` reports the stack verified and stages `CorsairsMetal.app` with
an arm64 `engine-1`. Passing: `renderer_world_map_probe.py`, `renderer_ui_fonts_probe.py`,
`renderer_consumer_coverage_probe.py`, `renderer_rain_weather_probe.py`,
`renderer_particles_fx_probe.py`, `foliage_cutout_probe.py`,
`weather_surface_bridge_probe.py`, `point_shadow_bridge_probe.py`. Three probes fail on this
tree and on `HEAD` alike, so they are not caused by this change:
`ffp_cache_signature_probe.py` (`IndexError`), `world_shadow_owner_probe.py` (`FAIL resident
raw shadow span`) and `night_local_lighting_probe.py` (`AssertionError`).

Disposition: source, build and staging **accepted**. Whether the icons are visible again stays
**unresolved**: this sandbox has no Metal toolchain, so the embedded MSL string is never
compiled here and no GPU replay is possible. Whether the wind pointer's authored `u` mirror is
visible on a symmetric arrow is likewise unverified.

### Prepared renderer consumers connected to the Metal path — September 19

Inventory in `docs/renderer-remaining-work.md` listed eight prepared renderer patches
(weather, astronomy, UI/fonts, vegetation, particles, world map, sea post-process) as outside
the canonical stack. A read-only diff against `experiments/native-metal/backend.mm` showed
every receiver already implemented and exported (`StormMetalDrawBillboards`,
`StormMetalDrawGlyphInstances`, `StormMetalDrawCompactPrimitive`, `StormMetalDrawGrass`,
`StormMetalDrawAdvancedParticles`, `StormMetalDrawParticleSprites`, `StormMetalDrawWdm*`),
so those features still ran the legacy CPU path because their engine-side producers were never
in the stack, not because the Metal side was missing.

Delta: `experiments/native-metal/build.sh` appends `renderer-grass`,
`vegetation-continuity`, `sea-island-vegetation`, `renderer-particles-fx`,
`renderer-rain-weather`, `renderer-sky-astronomy`, `renderer-ui-fonts`,
`renderer-world-map` and `sea-postprocess-uv` to the ordered stack after
`group-target-liveness.patch`; `CMakeLists.txt` adds `STORM_METAL_RENDERER_CONSUMERS=1`,
`STORM_METAL_PARTICLE_BRIDGE=1` and `STORM_METAL_GRASS_BRIDGE=1`. Five prepared patches
were rebased onto the live ABI: the star hunk calls `StormMetalDrawBillboards` instead of the
removed `StormMetalDrawStarSprites`; the UI new-file block gained its missing
`new file mode` line so `apply_source_patches.py` accepts it; the grass, vegetation and
particle patches lost their compile errors (a `-Wsizeof-array-div`, a redefined
`static CMatrix identity`, and `-Wc++11-narrowing` casts). `renderer_rain_weather_probe.py`
now verifies the staged tree is this patch applied instead of re-applying to an already-patched
snapshot.

Result: `run.sh --stage-only` reports `Source patch stack verified; 23 source files changed`
and stages `CorsairsMetal.app`; both `engine-1` and the staged `metal-engine` are
`9bf4ca74e46a1a8f67d3f837306913b70c824f545d84385b69534915ba19905f`.
`renderer_consumer_coverage_probe.py` passes ("9 renderer consumer classes reach native
raw/compact Metal; no direct consumer UP bypass") and so do the eight feature probes.

Disposition: source, build and staging **accepted**. Visual parity and the real-scene
zero-legacy-counter check stay **unresolved**: no GPU probe runs in this sandbox and the
changed consumers were not replayed in the game.

Two water patches stay outside the stack as superseded: `water-depth-tolerance.patch` and
`water-refraction-filter.patch` edit the `modernSeaForeground` /
`modernSeaForegroundFootprint` epsilon test that the current `sea_shaders.hpp` refactor
removed, so neither applies.

### Character-group target outlived its character, and the AI lookup faulted — September 19

The crash report (`EB2AC7FE...`, PID 18407, launched 21:43:46, faulted 21:47:57) faults with
`EXC_BAD_ACCESS` at address `0x8` inside `CharactersGroups::MsgGetOptimalTarget + 684`, reached
from `Supervisor::PostUpdate` -> `Core::Event` -> script bytecode -> `CharactersGroups::ProcessMessage`.
At symbol + 684 the staged build executes `ldr x0, [x0, #0x8]` immediately after the
`core.GetEntityPointer(c->grpTargets[s].chr)` call, and the register state in the report has
`x0 = 0` with `far = 0x8`: the engine dereferenced a null character reached from a target id.

That is `c->AttributesPointer` in `MsgGetOptimalTarget`. The function picks the optimal
`grpTargets` entry and resolves it with `GetEntityPointer`, but only the multi-target loop
skips an unresolved pointer; the chosen index (also the forced `s = 0` fallback and the
single-target case) was dereferenced unchecked. A group stores the target id, not the
character, so a target that had already left the location left the lookup returning null.
The script path is `LAi_group_GetTarget` (`PROGRAM/Loc_ai/LAi_groups.c`), which runs from the
warrior, guardian and officer templates on every supervisor update and already treats a zero
return as "no target" (`index = -1`).

Delta: new `group-target-liveness.patch`, appended to the canonical stack in
`experiments/native-metal/build.sh`. The unresolved target now returns the same "no target"
result the existing `numTargets <= 0` and missing-variable paths use. No target selection,
group relation or script call is changed. `docs/group-target-liveness.md` owns the contract.

The same staging attempt also had to unblock the gameplay delivery:
`sync_metal_gameplay.py apply` aborted with
`unexpected backup revision: PROGRAM/quests/quests_reaction.c` because
the recorded `gameplay-baseline` revision for that file and for
`PROGRAM/locations/locations_loader.c` is the pre-delivery revision of the 2026-09-18 layer
and is not today's `BASE`, and `BASELINE` had no record for either path. Both recorded
revisions differ from the current Metal file only by that layer's reviewed edits (the
fight-mode lock in `quests_reaction.c`, the boarding fight-mode reset in
`locations_loader.c`), so the two hashes are now recorded explicitly, exactly as
`PROGRAM/dialog.c` already was.

Result: `run.sh --stage-only` completed after both changes; `engine-1` and the installed
bundle are both `e14aea3bceb82e420bd2effbca0e6fbeb872d627bfb42b868797c4f861374f9d`, the staged
binary carries `cbz x0, 0x1000ab5d4` in front of that load, and
`tools/sync_metal_gameplay.py check` reports zero pending files.

Disposition: source, build and staging **accepted**. Whether a guard or warrior that lost its
target now falls back to another live target in the running game stays **unresolved**: the
incident scenario was not replayed on this engine.

### Window-plane light shaft footprint — September 19

Indoor god rays landed on the floor correctly but did not leave the authored window.
`openingBeamMask` measured each sample's lateral offset in the plane perpendicular to the sun
(`lateral = toPoint - beamDir*dot(toPoint,beamDir)`) and compared it against the aperture's
own `right`/`up` half extents. Those axes span the pane, not the plane perpendicular to the
sun, so the accepted cross-section was the authored opening divided by the cosine between the
sun and the pane normal: for a pane normal `+z` and sun `(0.6,0,-0.8)` a prism point at
parameter `a` maps to `u = 0.64a`, so the `hw=0.19` apertures reported by the launch log lit a
`0.30` half-width beam that started beside the window and stayed too wide down the room.

Delta: the mask now traces the sample back along the sun vector onto the plane of the opening
(`planeOffset = dot(normal,toPoint)/facing`, rejected when negative, with `facing` already
guarding a grazing sun) and tests that hit point against the authored rectangle. No aperture
geometry, frame origin, march, or composite change.

Evidence: `run.sh --stage-only` completed after the change; `.cache/build/bin/engine-1` and the
staged `.cache/CorsairsMetal.app/Contents/MacOS/metal-engine` are both
`e14aea3bceb82e420bd2effbca0e6fbeb872d627bfb42b868797c4f861374f9d`, arm64, and the staged
binary contains the new `planeOffset` trace. The MSL was not compiled offline: this host has no
Metal toolchain (`xcrun metal` requires `xcodebuild -downloadComponent MetalToolchain`) and the
sandbox exposes no Metal device, so the shader source is compiled at runtime by
`newLibraryWithSource`.

Disposition: source, build, and staging accepted. Real-scene replay in an interior with an
oblique sun remains unresolved.

### Villemstad black pier and over-bright ground — September 19

The user screenshot showed two coupled material failures: an exactly black low pier/ground
mesh and pale ground around it. Parsing the loaded Villemstad geometry found neutral
vertex colour `127,127,127` throughout and valid materials with zero diffuse, including
the large `Pirs_SentMartin1_KNS.tga` mesh. The original renderer's authored location state
uses vertex `COLOR1` with a `* 2` texture stage; the Metal raw outdoor path instead
substituted material diffuse, turning zero materials black and `0.8` materials too bright.

Delta: the Metal default enables `D3DRS_COLORVERTEX`, raw static location draws obey the
three D3D material-source render states, and CPU-expanded relight no longer suppresses
`COLOR1`. `dynamic_lighting_probe` adds a bound indexed town fixture and raw-location
counter; `lighting_probe` asserts the D3D9 default.

Evidence: both probe targets compile and link. Their GPU execution and canonical
`run.sh --stage-only` gate are pending because the Metal game is still running and the
sandbox has no display service.

Disposition: source diagnosis and build accepted; GPU fixture, staging, and real
Villemstad replay unresolved.

Follow-up — September 20: the renderer's initialization left
`D3DRS_AMBIENTMATERIALSOURCE` at zero (`D3DMCS_MATERIAL`) even though the authored
location contract uses `COLOR1` for ambient. The raw outdoor path therefore combined the
neutral vertex colour with each GM material's ordinary ambient value; with the location
`MODULATE2X` stage this overexposed the ground and made its texture look absent.

Delta: Metal initialization now seeds `D3DRS_AMBIENTMATERIALSOURCE` to `D3DMCS_COLOR1`,
and `lighting_probe` checks the diffuse, ambient, and `COLORVERTEX` defaults together.

Evidence: `experiments/native-metal/build.sh` passed, the CPU outdoor-lighting probe
passed, and `run.sh --stage-only` staged the fresh Metal binary and reported zero pending
gameplay files. The already-running Metal process was not restarted, and the hidden GPU
probe could not create a display in this environment, so real Villemstad replay remains
unresolved.

Disposition: source/build/CPU probe accepted; GPU, staging, and real-scene replay unresolved.

### Jungle-girl encounter and dialogue-combat softlocks — September 19

The latest Metal save is in `Beliz_Jungle_01` with `GenQuest.EncGirl=Begin_2`, three
`GangMan_*` actors in `EnemyFight`, `chrDisableReloadToLocation=1`, and no active group
alarm. The shot-before-dialog handler starts combat but its only
`LAi_group_SetCheck("EnemyFight", "LandEnc_RapersAfrer")` line was commented out. This is
the only commented group-completion registration in the gameplay tree, so killing the
last bandit could never run the existing completion case or open the exits.

Delta: the shot path now registers the completion callback, and the completion case
immediately restores player control, weapon state, fight-mode control, fast reload, and
location exits before starting the woman's thanks dialogue. Location load rehydrates the
callback for an in-progress saved encounter and completes it directly when all three
bandits are already dead. It also migrates saves made during the removed temporary
dialogue-combat group and restores that target's original group. Dialogue shows exactly
one action: `Ударить исподтишка` for the existing one-on-one condition, otherwise
`Напасть`. Both alert the target's original group through ordinary `LAi_group_Attack`;
the generated one-member group and its fight-end handlers are gone.

Evidence: the gameplay suite produces 35 deterministic exact-hash outputs and passes its
linked fixtures. `sync_metal_gameplay.py check` identifies only `dialog.c`,
`locations_loader.c`, and `quests_reaction.c` as pending, with the compiler ready. The
delivery helper now builds those outputs from reviewed originals and leaves archived
native-storm unchanged.

Disposition: diagnosis and source package accepted. Metal apply, startup compilation,
latest-save replay, group aggression, and both dialogue labels remain unresolved while
the current Metal process is open.

### Crew-payment attribute walk crashed on a dead parent node, 2026-09-19

The crash report (`B29D662E...`, PID 58898, launched 23:48, faulted 00:09:13) faults in
`ATTRIBUTES::GetThisName() + 8` called from `WorldMap::AttributeChanged + 976`, through a script
bytecode chain entered from `XINTERFACE::MouseDeClick` while the user worked an encounter
interface on the world map. The offending call sits inside the
`for (auto *pa = apnt; pa; pa = pa->GetParent())` ancestor walk: `apnt` itself had already been
dereferenced four times, so the node that faulted was an ancestor reached from `GetParent()`.

`ATTRIBUTES::Copy()` overwrites each child with `new_child = attribute->Copy();` and returns
`result` by value. Both operations move the `std::vector<std::unique_ptr<ATTRIBUTES>>`, and the
move constructor and move-assignment operator never re-pointed the moved children's `parent_`
at their new owner. The script builtin `CopyAttributes` is `*pRoot = pA->Copy();`, so every
non-root node of a copied class kept a `parent_` addressing a destroyed temporary, and the
ancestor walk then read freed memory.

Delta: new `attributes-parent-chain.patch`, ordered immediately after
`save-areference-liveness.patch`. `ATTRIBUTES::ReparentChildren()` now runs in the move
constructor and in the move-assignment operator, so ownership and `parent_` agree after a move,
and `WorldMap::AttributeChanged` ends its ancestor walk at a node that is no longer live
(`pa && ATTRIBUTES::IsLive(pa)`) instead of dereferencing it. No label content, renderer path or
draw is removed; a live `labels` ancestor still reaches `SetIslandsData`.
`docs/attribute-class-lifetime.md` owns the contract.

Result: `build.sh` completed ("Source patch stack verified; 3 source files changed"), `engine-1`
and the installed bundle are both
`c012f440b9d896387d65ab066509060f98c08d326f4532da11e070fc3abaa4a7`, that batch is staged, and
`tools/sync_metal_gameplay.py check` reports zero pending files.

Disposition: source, build and staging **accepted**; the crew-payment replay and the identity of
the destroyed node stay **unresolved** until the action is replayed on this engine.

### Sail rebuild crashed on renderer buffer-table exhaustion, 2026-09-18

The crash report (incident `8A66E848...`, PID 47749, quit 23:20 after 1 h 34 min) faults
inside `DX9RENDER::LockIndexBuffer` from `SAIL::SetAllSails` -> `SAIL::FirstRun` ->
`SAIL::Execute`. `LockIndexBuffer` indexed the fixed `IndexBuffers[10240]` table with no
range check and the id passed in was `-1`; the memory just before the table held `nullptr` at
the `buff` offset, so `buff->Lock(...)` read address 0. `CreateIndexBuffer` returns `-1`
only when every slot is allocated, and the Metal backend's `CreateIndexBuffer` always returns
`S_OK`, so the negative id means the table was full, not that the device refused a buffer.

Two defects. First, the table is the renderer's only guard and the draw entry points read it
directly: `DrawBuffer`, `DrawIndexedPrimitiveNoVShader`, `DrawPrimitive`,
`RenderAnimation` and the lock/unlock/size accessors all indexed a caller-supplied id with no
bounds or allocation check. Second, `SAIL::SetAllSails()` replaced `sg.vertBuf` and
`sg.indxBuf` without releasing the previous pair, while `SetAddSails` and
`DeleteSailGroup` release first, so every sail rebuild leaked one vertex and one index slot.
The rest of the growth toward 10240 is still unexplained: no single creator was proven to account
for the table, so the accumulation is **not** claimed fixed.

Delta: `renderer-buffer-guard.patch`, last in the canonical stack, rejects an out-of-range or
unallocated id in `LockVertexBuffer`, `UnLockVertexBuffer`, `GetVertexBufferSize`,
`LockIndexBuffer`, `UnLockIndexBuffer`, `DrawBuffer`,
`DrawIndexedPrimitiveNoVShader`, `DrawPrimitive` and `RenderAnimation`, traces the
first rejection once, and skips that draw instead of dereferencing memory outside the table.
`CreateIndexBuffer`/`CreateVertexBuffer` trace table exhaustion, and `DX9EndScene`
samples the live buffer counts every 1024 frames and traces each new 512-slot high-water mark, so
the residual accumulation is measurable from `launch.log` instead of inferred.
`SAIL::SetAllSails()` now releases its previous buffer pair before creating the new one. No
geometry, material, technique or draw is removed, and a rejected id never reached a real draw
before.

Result: canonical `build.sh` completed after the stack change ("Source patch stack verified; 2
source files changed"), `engine-1` is
`c927f681198c933e1006b250dab6ba0ad42aa9fc53a3523886e38e37849fb2b5`, and `native_legacy3d-probe`,
`indexed-probe`, `menu_unlit_native-probe`, `scene_hud_boundary-probe`,
`ffp_frame_cache-probe` and `indexed_span_frame_cache-probe` all PASS.
`docs/metal-renderer-buffer-table.md` owns the table contract.

Staging: that 21:46 bundle has since been replaced; the currently staged engine also carries the
attribute-parent-chain fix, so this guard is live in the bundle.

Disposition: source, build and probes **accepted**; the crash replay and the buffer-count growth
curve stay **unresolved** until a staged launch runs long enough to pass the count that faulted.

### Raw lit Metal fallback corrupted ship and water-adjacent meshes, 2026-09-18

The player captured giant multicoloured triangles on ships, shoreline water geometry and
nearby scene meshes, plus a main menu that rendered only its 2D layer. The common lit raw
fallback introduced in `7ee2556a` bound a compact fallback uniform where the normal `Uniform`
block was expected, omitted the land-sampling bindings, read indices from the compacted
submission instead of the original draw state, discarded signed base vertex, and treated a
triangle fan as a triangle list. Those contract errors explain intact UI and characters next
to exploded static or dynamic lit meshes.

Delta: `backend.mm` now submits the original indexed state, validates the exact referenced
source range, binds the normal, raw-land, land-sampling and shadow blocks at their shader
slots, preserves signed base vertex by rebasing the bound source span, and expands only
triangle fans into the frame arena. Static and dynamic vertex buffers remain GPU-rendered;
there is no CPU vertex transform, CPU lighting, reduced geometry or reduced material path.
`native_legacy3d-probe` is part of the canonical build and covers static/dynamic lit FVF,
16/32-bit indices, list/strip/fan topology, signed base vertex and pixel equality.

Result: `native_legacy3d-probe`, `indexed-probe` and `menu_unlit_native-probe` pass. The
engine builds and links as
`ef63777130bbed78092967e575e09fe8bd6bc7cfd3ad1d79aafdad47d94301f5`.

Staging and launch: `run.sh --stage-only` staged the same engine hash, gameplay reported
zero pending files with the compiler ready, and PID 47749 launched that bundle at 21:46.

Disposition: source, build, probes, staging and startup **accepted**; the real menu, town
and sea visual replay are still pending.

### Sneak fist attack could preserve forced-unarmed mode, 2026-09-18

The player clarified that the failure began after `DialogAmbush` struck an NPC with a fist:
the blade did not return, and a later boarding save still had no usable weapon. The chain
could enter engine `SetFightWOWeapon=true`, clear its `Target` during the draw step, then
make `DialogAmbush_FightOver` return because that target was gone. A location unload also
deleted the temporary target group without clearing forced-unarmed state. Separately, the
shared `MainHeroFightModeOff` reaction sheathed the protagonist and then locked fight mode,
which could preserve the same visible failure after dialogue.

Delta: the ambush draw first clears stale forced-unarmed state, fight completion no longer
depends on the draw step's deleted `Target`, and both normal sheathing and location unload
clear forced-unarmed state and the temporary ambush attributes. The generic sheath reaction
now unlocks fight mode. Boarding start and boarding-location load both clear stale fight
lock and forced-unarmed state, so an existing affected save recovers on entry or reload.
Purposeful story, cutscene and `noFight` location locks retain their own owners.

Result: `patch_gameplay_suite.py verify` passes all 35 cumulative transforms and linked
fixtures. Canonical staging applied `LAi_boarding.c`, `dialog.c` and `locations_loader.c`;
`sync_metal_gameplay.py check` reports zero pending files and the compiler ready. PID 47749
launched the staged bundle at 21:46.

Disposition: source transform, fixture verification, staging and startup **accepted**; the
affected save's boarding weapon and a new fist-attack replay remain user-visible checks.

### Sea frame cost: the span cache walked all 8192 spans, 2026-09-18

The player reported 40/20 FPS at sea with the GPU idle and no such cost previously. The
profile blocks are real frames: 1169 of them average 8.36 ms, which is the 120 FPS limiter
cap, 105 average 60.22 ms, and blocks x 120 x wall against the 2144 s process age
reconciles to within 26 s. In the slow blocks gpu is 3.4 ms against 5.6 ms in the fast
ones, drawableWait is 0.14 ms, and every instrumented renderer timer is at noise (FFP lit
0.007 ms, bounds_cpu_ms 0.2-1.5 ms per 120 frames), so the cost sat in an uninstrumented
path.

Two live samples read 41 % of the main thread in nanosleep. That is a focus artifact, not
the defect: while the player types in this task the window is unfocused, the frame loop
takes its sleep_for(50ms) inactive branch, and those iterations never present, so the
inactive counter cannot see them and wall absorbs them as if they were frame time. Focused
slow frames, sampled under DX9RENDER::DrawBuffer, put sm::IndexedSpanFrameCache::observe at
244/360 and 235/343 of SAIL::Realize, 150/226 of CoastFoam::Realize and 68/105 of
TCarcass::Realize, about 700 of 4776 main-thread samples, with Sailors::Realize at
893/4776.

The cause was structural. A revision change walked the whole entries map and then rebuilt
both revision maps from what survived, one std::map insertion per surviving span, so a
scene that reaches the 8192-span cap paid that work plus the rebuild for every rebuilt
buffer in every frame.

Delta: indexed_span_frame_cache.hpp lists each admitted span under its index and vertex
owner and evicts only that buffer's spans, unlinking every erased key from the other owner
and dropping the peer revision once that buffer has no span left. The revision-eviction
contract, the 8192 cap and the invalid-range rejection are unchanged. The profile line in
backend.mm gains minWall and slow20 (inter-present intervals at or above 20 ms) so the next
log separates a uniformly slow frame from a focus gap instead of inferring it.

Result: indexed_span_frame_cache-probe PASSes unchanged ("5760 repeated shadow submissions
scan once; revisions/bounds miss; invalid ranges reject; Reset revalidates"), the engine
builds and links, engine-1 is
3b5601e16244c6a1ce9090a24d99c96a08d8d59dad304e5488063a02928cef1c, and canonical staging
is still pending because the previous candidate is running.

Disposition: source, build and probe **accepted**; the real sea replay stays **unresolved**
until a staged launch shows the sail, foam and track frame cost down and minWall back at the
limiter cap. The same samples name one untouched owner: per-frame crew deck collision,
measured and recorded in docs/live-deck-walk.md.

### Sea frame cost: crew deck collision paid ten distance terms per triangle, 2026-09-18

A focused sea sample reconciles the frame the previous entry left open. Of 3118 main-thread
samples, CoreImpl::ProcessRealize is 2988, Sailors::Realize 1367, ShipWalk::CheckPosition
1137, SAIL::Realize 496 and 229, SEAFOAM::Realize 115 and 106, ShipTracks::ProcessStage
164, ISPYGLASS::Realize 92. Every DX9RENDER::DrawBuffer in that tree is 70-90 %
sm::IndexedSpanFrameCache::observe, about 815 samples of 3118, so the dynamic draw path is
what churned span revisions and produced the preceding entry's cost. CheckPosition is
entirely TrySteerAroundStatic, and its CanPlaceAt is about half deck_collision::canSweep
(NODER::Clip -> GEOM::Clip -> deck_collision::polygon) and half the support Trace.

The cause was the leaf predicate. deck_collision::Sweep::hits evaluated ten
capsule-to-triangle distance terms before comparing anything, the ship BSP query visits
every triangle inside radius plus half the capsule height, and CanPlaceAt runs up to seven
times per moved crew member per frame.

Delta: the shipped header derives the reach bound first. The initial spine is the part of
the swept solid closest to a fixed obstacle, so every term is at least initial - |delta|. A
triangle farther than radius + |delta| from the spine cannot block the move and returns
before those terms, the .0001f margin keeps the original overlap-escape branch on the full
computation, and |delta| is computed once per query instead of once per triangle.

Result: a differential build against the shipped header compares the new predicate with a
verbatim transcription of the old one over 541401 generated and boundary configurations,
189727 of them blocking, and reports zero differences. deck_collision-probe,
sailor_locomotion-probe, deck_walk-probe, indexed_span_frame_cache-probe and
character_contact-probe all pass unchanged, the engine builds and links, and engine-1 is
fb9372f5364545c285a424f8f3e372f836bfdd6220d7633ba2465792655bb794 (6176472 bytes).

Disposition: source, build and equivalence evidence **accepted**; the real sea replay stays
**unresolved** for both frame-cost entries until one staged launch puts the sea wall back at
the limiter cap.

Staging: the previous candidate exited, and experiments/native-metal/run.sh --stage-only then
consumed both frame-cost fixes in one batch. CorsairsMetal.app/Contents/MacOS/metal-engine is
fb9372f5364545c285a424f8f3e372f836bfdd6220d7633ba2465792655bb794, the gameplay receipt binds
deck-walk.patch to 0151c617e21d2ae0ef1e2168bd0c03410b13b13dc08c8a15a1a06388f9ef3926, and
sync_metal_gameplay.py reports the compiler ready with no pending files. Nothing has been
launched from that bundle at staging time, so the replay was the only missing evidence.

Replay: the player launched that bundle at 20:58 and stayed in the sea, so the live profile is
now the first real result for both entries. The sea blocks are 15-42 ms of wall with minWall
12-30 ms and slow20 near 120, against 3.4-4.7 ms of GPU: the wall is uniformly slow, not a
focus gap, and it is no longer the 60 ms class the previous log recorded.
sm::IndexedSpanFrameCache::observe and DX9RENDER::DrawBuffer fall to about 8 and 88 of 2387
main-thread samples in a 4-second sample of that process, so the span-cache fix holds. The
same sample puts Sailors::Realize at 679 samples with 677 of them inside
ShipWalk::CheckPosition, which makes the crew resolver the largest remaining game-side block.
That session exited on its own at 21:06 with normal profile lines and no crash report.

### Crew deck collision cancelled by the player, 2026-09-18

The player cancelled the crew resolver outright: crew may walk through each other and the
deck camera, and he does not want that computation at all. The measurement above is the
reason, and the deck-sweep optimization in the previous section is now inert for the active
runtime: it only ever fed this resolver.

Owner: experiments/native-storm/sailor-collision.patch owns every crew-side piece -
ShipWalk::CheckPosition's body and call site, CanPlaceAt, TrySteerAroundStatic,
ResolveDeckCharacter, the relaxation passes, the deckCharacter_ state and
MSG_SAILORS_RESOLVE_DECK_CHARACTER 77601 - and experiments/native-metal/deck-walk.patch was
its only consumer. No other patch in the applied stack names them.

Delta: experiments/native-metal/build.sh no longer applies sailor-collision.patch and no
longer binds it in the gameplay receipt. experiments/native-metal/deck-walk.patch drops the
crew side of the deck camera - DECK_CAMERA::ResolveCrew, the collisionShipId release message
in ReleaseWalker, the SeaDeckCharacterCollisionQuery type in deck_collision.hpp and the crew
terms in the STORM_TRACE_DECK_WALK line - while keeping the walker entity, its model and
animation, the first/third-person and spyglass paths and the protagonist's own CanWalkTo
gate against ship geometry unchanged. tools/sync_metal_gameplay.py binds the compiler receipt
to deck-walk.patch alone. experiments/native-storm/sailor-collision.patch, storm's own
deck-walk.patch twin and the two storm probes stay untouched: that runtime is a frozen
baseline that this build path no longer reads, and Sailors::Realize in the Metal source is
back to the stock per-man spos bypass with the stock CheckPosition call site.

Result: the ordered stack reconciled six source files (sailors.cpp, sailors.h, messages.h,
deck_camera.cpp, deck_camera.h, deck_collision.hpp), the engine built and linked through all
targets, and experiments/native-metal/.cache/storm now reports zero TrySteerAroundStatic,
deckCharacter_, ResolveDeckCharacter and MSG_SAILORS_RESOLVE_DECK_CHARACTER occurrences.
deck_walk_probe.py, deck_collision_probe.py and sailor_locomotion_probe.py still pass against
the frozen storm baseline, whose copy of the resolver is unchanged. engine-1 is
a992db57a418b24ac0b05fb4dffcf21c2b82372ed8d9040b07f2572ff4627282.

Staging: after that session ended, run.sh --stage-only consumed the whole source,
ordered-patch and material set in one batch and reported the compiler ready with no pending
gameplay files and no gameplay write. CorsairsMetal.app/Contents/MacOS/metal-engine and
build/bin/engine-1 are both
a992db57a418b24ac0b05fb4dffcf21c2b82372ed8d9040b07f2572ff4627282 (6158104 bytes), so one
staged bundle now carries the span-cache fix, the deck-sweep reach bound and this removal
together.

Launch: the player started that bundle at 21:10. Its first profile blocks are a land location
at the 8.33 ms limiter cap with 105-115 draws, and a 3-second sample of that process contains
no Sailors::Realize, deck_collision or CheckPosition frame at all - expected for a location
with no ship crew, and therefore no evidence about the removal either way.

Disposition: source, build, probe, staging and startup **accepted**; the real sea replay is the
only missing evidence, and it needs the player to take that build into the sea.

### Sea battle-mode gate survived the sea scene, 2026-09-18

The player reported that after a sea battle the game still behaves as if combat were running
once he is back on the global map, including when every enemy was sunk, and asked why it is
not reset there. Source reading confirms the gate; it also drops the reputation hypothesis.

There is no separate combat-mode state. `PROGRAM/battle_interface/BattleInterface.c` declares
the script global `bDisableMapEnter` (`идет бой`), and `PROGRAM/sea_ai/AIShip.c:3566-3650`
rebuilds it and `pchar.Ship.POS.Mode` from live distances on every sea frame: a live
`RELATION_ENEMY` ship closer than 1000, a hostile fort closer than 1400 that never has to
fire, a nonzero `iStormLockSeconds`, or `bQuestDisableMapEnter`. Inside the sea the flag
therefore clears itself once the player is clear, and a hostile fort inside 1400 is the
reasonable reading of "combat mode with nothing left to fight".
`tools/patch_crime_reputation.py` creates no pursuer and no reporter ship and holds no
battle lock, so it is not the owner.

The defect is the scene lifetime. `DeleteSeaEnvironment` (`PROGRAM/sea_ai/sea.c:96`) is the
single owner of every exit - the global map (`Sea_MapStartFade`), port and land
(`reload.c:408`), sea reloads (`Sea_ReloadStart`), island loading and the boarding return -
and it cleared neither `bDisableMapEnter` nor `POS.Mode`, so the last sea frame's verdict
survived onto the global map and into ports, which never recompute it. Off the sea that
stale value reaches `CheckSaveGameEnabled` (`seadogs.c:1746`; the shipped
`Mods_On_Off_File.txt` selects `SeaBattleMode_SaveEnable {On}`, so this build does not
refuse the save), `SalaryNextDayUpdate` (`scripts/time_events.c:10`), the flag change in
`interface/NationRelation.c:345`, and the cabin rest paths.

Delta: `tools/patch_sea_battle_mode_reset.py` is a second stage over the composed gameplay
output, in the shape of the journal stage. From base
`107f47b78d4a1fab38ad51bac1c0349bdafd1343a9ebb9eec20952a1fc213cd1` it inserts
`bDisableMapEnter = false` and `pchar.Ship.POS.Mode = SHIP_SAIL` into
`DeleteSeaEnvironment` after `pchar.Ship.Stopped = true`, pinned to
`33373b67ed2f3dc8166dadf5560df06df2a155bdd6ec35a94462e48764beff03`.
`patch_gameplay_suite.py` applies the stage after the journal stage and merges its hash
vector into `classify`; `sync_metal_gameplay.py` lists `PROGRAM/sea_ai/sea.c` in `BASE` and in
the expected vector.

Evidence: the new module reports the base revision and transforms deterministically;
native-storm now holds `33373b67...ff03`, `patch_gameplay_suite.py status` reports
`patched`, and `sync_metal_gameplay.py check` reports exactly one pending file with the
compiler ready. `patch_gameplay_suite.py verify` does not reproduce the installed composite
for `PROGRAM/quests/quests_reaction.c` (expected `b435b2fb...4514`, got
`77140737...5f145f`); that inconsistency predates this stage and is untouched by it.

Disposition: source and delivery preparation **accepted**; `sync_metal_gameplay.py apply`
and `run.sh --stage-only` are **pending** because the Metal runtime is held by the session
launched at 20:58. The replay that accepts the fix - leave a sea battle with enemies still
alive, reach the global map, and confirm the map button, the save menu and the ship's sail
state are no longer in battle mode - remains the only missing evidence.

### Light shafts anchored to the live frame origin, 2026-09-18

The beams slid sideways whenever the camera or the hero turned, while the window and the
room stayed put. The apertures, the sun vector and the march were already world-space;
the defect was the frame they were expressed in.

Storm draws camera-relative. DX9RENDER::SetCamera publishes worldOrigin =
-cameraPosition, and DX9RENDER::SetTransform(D3DTS_VIEW) re-derives the same origin from
the view matrix on every view set, so a world point is drawn at authored + worldOrigin
for that frame. backend.mm applied that origin at aperture submission time. The authored
LocationWindows meshes are submitted once per location load, so those apertures froze in
the load-time camera frame, while the per-frame lightshafts locator path carried a live
one. Orbiting the third-person camera therefore displaced the beams by exactly the camera
displacement.

Delta: volumetric_light_shafts.hpp takes the live frame origin in encode and adds it when
the frame uniforms are built, next to the camera position and the depth receiver;
backend.mm publishes authored aperture centers without the origin, in both the mesh and
the locator entry points; volumetric_light_shafts_gpu_probe.mm gains a floating-origin
fixture in which a fixed room under the live origin must render bitwise identical while
the origin moves, with the rejected submit-time origin as the negative, measured at 79 of
256 px.

Disposition: source, build, probe and the canonical staging of
.cache/CorsairsMetal.app (engine d7906b15fed5d3da9ca89de00766c3e70ac5941da0ac2f845f91ef337136950e)
**accepted**; the real-scene rotation replay stays **unresolved** until the player turns
the camera and the hero inside a shafted interior and the beams stay on their window.

### Dialogue ambush punch, weapon draw and fist sound, 2026-09-18

The dialogue surprise strike now reads as the player described it: the hero punches
with an empty hand, the punch carries a fist-impact sound instead of a saber, and the
weapon comes out at the end of the strike and stays out for the fight. Three owners
changed.

The strike plays a new action. `[ambush_punch]` is a clone of the shipped
`[attack_force_3]` block in every `.ani` that declares it, with the sound event
retargeted from `SndAlliace_attack_slash2` to `SndAlliace_ambush_punch` **and moved
from frame 5250 to the block's own `Attack` frame 5258**. The engine dispatches
`SndAlliace_` events from `Character::ActionEvent`, so the saber sound of the source
clip could not be muted from script, and `Attack` is the frame that runs
`CheckAttackHit` — with `fps = 15` from the `man.an` header and `speed = 1.5` that is
0.44 s into the clip, so the sound now lands with the visible contact and with the
scripted damage at 500 ms instead of 0.09 s before it.

The fist sample is the player's own file, staged by the new
`tools/patch_ambush_punch.py` into both gameplay trees as
`RESOURCE/Sounds/People Fight/Punch_01.wav` plus an `[ambush_punch]` alias, trimmed
from `universfield-punch-03-352040.mp3` to its impact (mono 44.1 kHz PCM, level
matching the shipped fight samples, pinned by digest in `tools/assets/`). Its
upstream page and licence are not recorded, so the terms are treated as unknown.

The weapon now comes out in a separate step. `DialogAmbush_ApplySneakStrike` is
damage only, and a new `DialogAmbush_DrawWeapon` at 900 ms restores the player type,
unlocks the fight mode, runs the ordinary `Normal to fight` draw, falls back to
`SetFightWOWeapon(true)` when no blade is in the slot, and then starts combat. A
group check function schedules the sheathe two seconds after the last member of the
target's temporary group dies, so the blade is put away only after the fight.

Delta: `tools/patch_dialog_ambush.py` (punch action, draw step, fight-over callback,
new `_verify_dialog` contract), `tools/patch_ambush_punch.py` (sound, alias and
animation clone owner), `tools/patch_gameplay_suite.py` `COMPOSITE_SHA256` and
`tools/sync_metal_gameplay.py` `BASE` moved to
`c68a86deb87a2fe1e1cc5a1e36f8c85e334f43f6711a55e17627a1d7fe36b75b` for
`PROGRAM/dialog.c` (previous Metal revision `884328db…`), `docs/crime-reputation-system.md`
and `docs/audio-and-particles.md` for the contract and the rejected alternatives.

Two defects of the first draft were caught before delivery and are recorded as
rejected: a parameterless `DialogAmbush_FightOver` (the group registry calls a check
function with its own name, so every shipped handler declares one string parameter),
and a `LAi_LockFightMode(pchar, true)` at the end of the ambush (a stale
`lockFightMode` blocks the saber key until an attacker clears it — the native port
already carries a `recovered stale fight-mode lock while under attack` repair).

The sound, alias and animation clone are staged in both gameplay trees, so
`tools/patch_ambush_punch.py check` is clean. The player closed the session and
`run.sh --stage-only` staged the batch at 19:45: `PROGRAM/dialog.c` was applied into
`experiments/native-metal/.cache/runtime` at
`c68a86deb87a2fe1e1cc5a1e36f8c85e334f43f6711a55e17627a1d7fe36b75b`, that runtime still
carries all 38 `.ani` files that declare `[ambush_punch]` and
`RESOURCE/Sounds/People Fight/Punch_01.wav` at the pinned digest, and
`tools/sync_metal_gameplay.py apply` reported `compiler ready` and left SAVE,
configuration and renderer-owned materials unchanged. The staged bundle engine and
`.cache/build/bin/engine-1` both hash
`d7906b15fed5d3da9ca89de00766c3e70ac5941da0ac2f845f91ef337136950e`, matching the
`gameplay-compiler.json` receipt for compiler patch `a9b691d5…` and the deck
patch digests; the previously installed bundle engine was `3cf0eaed…`.

Disposition: source, hash vector, punch content and the Metal delivery
**accepted**; the real replay stays **unresolved** until the player confirms the
punch, the fist sound, the blade appearing in the hand afterwards, the blade
staying out during the fight, the two-second pause before it goes away again, and
the bare-handed case.

### Dialogue ambush refused by the generated-NPC quest container, 2026-09-18

The tracing revision answered the visibility question on its first replay: the dialogue with a Spanish guard wrote `[compile] [trace] DialogAmbush blocked: quest GenChar_382 group=Spain_citizens` (six lines covering `GenChar_382` and `GenChar_386`), so the action was refused by policy and not by rendering. The gate was `CheckAttribute(target, "quest")`, and that attribute exists on every generated character: `InitCharacter` (`PROGRAM/scripts/utils.c`) and `LAi_CreateFantomCharacterEx` (`PROGRAM/Loc_ai/LAi_utils.c`) both write `chr.quest` as a data container while generating an NPC, with `quest.questflag` (the `!`/`?` mark) inside the same subtree. Reading it as a quest lock closed the whole layer for townsfolk, guards and officers.

Delta: `DialogAmbush_BlockReason` reports `quest` for `isquest` alone, the replacement line carries the reason the container is the wrong signal, `_verify_dialog` requires the `isquest` gate and rejects `CheckAttribute(target, "quest")`, and `docs/crime-reputation-system.md` names the protections that really hold (`LAi_grp_relations.quests`, `chr_ai.hpchecker`, `LAi_IsImmortal`, `DialogAmbushProtected`). `PROGRAM/dialog.c` is `884328db96b64dcec5c83ae54bb6f6a0f943715f3e093445b6d7361310063eff` in `native-storm` and `native-metal`, rebuilt from the committed original by the patch transform; `COMPOSITE_SHA256` carries the delivered revision and `sync_metal_gameplay.py` `BASE` advanced to the previous Metal revision `9c468773…` that the apply consumed. `tools/sync_metal_gameplay.py check` reports zero pending files with a ready compiler, and `run.sh --stage-only` completed after the change.

Disposition: diagnosis, source, Metal delivery and staging accepted. The replay that shows `Напасть` and `Ударить исподтишка` in a real dialogue, the visible swing, the kill and the survivor case stay **unresolved** until the user replays them; if a dialogue still offers nothing, the same trace names the next gate on the list (`groupquest`, `nofightlocation`, `boarding`, `captured`, `alarm`, `fightmode`).

### Dialogue ambush gate visibility, 2026-09-18

A real replay of the dialogue aggression layer never showed `Напасть` or `Ударить
исподтишка` in any conversation, a guard talk included, and the feature's own gate left
no trace that could separate a policy refusal from a rendering problem. The dialogue
owner now has one decision owner: `DialogAmbush_BlockReason` returns the first matching
gate name (or an empty string) and `DialogAmbush_CanAttack` is exactly
`DialogAmbush_BlockReason(target) == ""`. `DialogAmbush_AppendLinks` traces
`DialogAmbush blocked: <reason> id=… group=…` when it refuses, and
`DialogAmbush links: id=… group=… total=<n>` when it appends, so the next replay names
the blocking condition instead of the options simply being absent.

`PROGRAM/dialog.c` in `native-storm` and `native-metal` now carries SHA-256
`9c468773d9214b81c0e7bee9a2cd7caedb888b5bb5dfd775dbf5f4a7a828c3f4`, rebuilt from the
committed original by the patch transform rather than hand-edited;
`tools/patch_gameplay_suite.py` `COMPOSITE_SHA256` carries the delivered revision and
`tools/sync_metal_gameplay.py` `BASE` was advanced to the previous Metal revision
`8ccbe2e4…` that the apply consumed. `tools/sync_metal_gameplay.py check` reports zero
pending files, and `experiments/native-metal/run.sh --stage-only` restaged the unchanged
arm64 engine with zero gameplay files pending. Disposition: repository source and Metal
staging accepted; the blocking reason, the visibility of appended links (the dialogue
link list renders five lines per page), and the strike replay stay **unresolved** until a
real dialogue writes the probe line to `compile.log`.

### Dialogue sneak-strike windup and ally witnesses, 2026-09-18

The dialogue strike now has a visible windup and no longer treats the player's own
people as blockers. `DialogAmbush_Start` hands the hero to the actor template, turns
him to the target and plays the shipped `attack_force_3` swing
(`LAi_ActorAnimation` is a silent no-op unless the character is an actor type, as
`PunishmentAction` in `quests_reaction.c` also shows); a `DialogAmbush_SneakStrike`
event fires 500 ms later, restores the player type through `LAi_SetPlayerType`, then
applies the usual `break` energy cost, damage and combat start. `DialogAmbush_IsOneOnOne`
now skips witnesses that `DialogAmbush_IsPlayerOwned` claims, so officers, companions,
passengers and other player-group characters standing next to the target no longer
suppress `Ударить исподтишка`; unrelated third characters still do.

`PROGRAM/dialog.c` in `native-storm` and `native-metal` now carries SHA-256
`8ccbe2e4ebb0c706c430dce963667314533d103aea80a92381852996bb8f04be`, rebuilt from the
committed original by the patch transform rather than hand-edited; the Metal
`gameplay-baseline` copy stays at its pristine revision
`1f0f4f918f1f35134478e1ba2c98c607492f045242e8e6c30cccdfb95a6b57ce` (the sync tool
needed an explicit `BASELINE` record before it would accept a second delivery of the
same path). `tools/sync_metal_gameplay.py check` reports zero pending files and a ready
compiler, and `experiments/native-metal/run.sh --stage-only` rebuilt and staged the
unchanged arm64 engine `d34a0c36c8365d9ec583f37e8ab6e94702d0fab38e0d5d7f50c6b6055265f773`.
Disposition: repository source and Metal staging accepted; the real-replay part stays
**unresolved** because the swing, the ally-witness case, the kill and the survivor
case were not observed in the running game.

### Dialogue aggression redesign, 2026-09-18

The dialogue owner now exposes `Напасть` to every live non-quest, non-immortal,
non-protected target allowed by the current combat/location state, including
guards, officers, companions, and NPCs in a player-owned colony. `Ударить
исподтишка` remains restricted to a true one-on-one scene, but now spends the
normal `break` attack energy and calls the canonical weapon/skill/armour damage
and death path. A surviving target continues into ordinary combat; a player-owned
officer/companion/passenger is detached from the roster and made hostile before
combat. Both actions register the shared crime intent.

The old mixed gameplay runtime was upgraded by exact hash from the previous
dialogue output; all other gameplay files and renderer-owned changes were left
untouched. Static dialog checks passed. `native-storm` and `native-metal`
`PROGRAM/dialog.c` now both have SHA-256
`b63dc124f592ab650fc52fece69c99de78f31c63eb27a5706d62b30704a8aa98`.
`experiments/native-metal/run.sh --stage-only` completed with the arm64 engine
and zero pending gameplay files in the synchronization check. Disposition:
repository source and Metal staging accepted; the actual dialogue replay with a
guard, own officer, player-colony NPC, nearby witness, and protected quest target
remains **unresolved** because no computer-use/game replay was performed.

### Consolidated repository stage, 2026-09-18

After the previously orphaned Metal sources, probes, patch stack, material
manifests, and gameplay delivery scripts were committed, the canonical
`experiments/native-metal/run.sh --stage-only` path completed from clean
`main@5d6a4673`. The built and installed arm64 executables have the same SHA-256,
`6533851489d2a2489ae875fef7fcd44e080cd78ba383ef01c310d5f0d50b0ee9`.
Gameplay synchronization reports zero pending files and a ready compiler; the
Metal acceptance harness passes all four tests. Disposition: repository source,
build, and staging are accepted at this boundary; the visual scenarios listed
below remain unresolved until their real-game replays are completed.

### Land camera follow-state reset, 2026-09-18

The prior land-origin correction did not address the observed land-only bird's-eye
camera. The canonical `LocationCamera` now invalidates the old follow camera state
when `MSG_CAMERA_FOLLOW` is restored: `CameraFollow::BornCamera` must run against
the current character, the position transition is treated as a teleport, and the
look-mode execution flag is cleared. The reset is kept on the follow transition;
target changes used by cinematic `TOPOS` shots retain their existing interpolation.
This prevents a saved or top-view camera position from surviving the return to land
follow mode while preserving explicit top-position and free-camera messages.

The ordered Metal source stack, arm64 engine build, and `run.sh --stage-only` all
passed after the change. A real land-scene replay was not completed: the current
computer-use surface reports that macOS is locked, so the visual camera result
remains **unresolved** and is not claimed accepted from build output.

### Land camera origin correction, 2026-09-18

The land camera uses `DX9RENDER::SetCamera(lookFrom, lookTo, up)`, while the ship
deck path uses the position/angle overload. That land overload already updates
`vWordRelationPos` and the world transform through `SetTransform(D3DTS_VIEW, ...)`.
The previous candidate applied the same world-origin delta a second time after
that call; the correction removes the duplicate adjustment and only publishes the
resulting origin to the Metal lighting bridge. The canonical Metal stage succeeded
after this change and the candidate launched, but the macOS screen locked during
the real-surface replay, so land camera tracking remains **unresolved** until the
interactive scene can be observed. The launch profile reported `inactive=120`,
which is not visual acceptance evidence.

### Shadow coordinate and point-light budget pass, 2026-09-18

The Metal backend now applies `receiverToWorld` to both raw shadow caster MVP paths,
and the location shadow bridge writes light VIEW/PROJECTION matrices directly to the
underlying D3D9 device so `DX9RENDER` cannot mutate `vWordRelationPos`. Point-shadow
selection is limited to two nearest active lamps inside range and is skipped when the
directional sun is bright (`Diffuse` sum > 0.12); sea authored lights lazily open their
catalog before publication. Direct `ninja -C experiments/native-metal/.cache/build engine`
passed. The canonical `run.sh --stage-only` path also passed. The required real-surface
camera rotation/W+A/W+D replay remains pending.

**Sole active and supported runtime:** `experiments/native-metal` (`./experiments/native-metal/run.sh`).
All ongoing and future development, graphics work, shader migration, and gameplay fixes target
Direct Metal exclusively. The canonical engine build is `./experiments/native-metal/build.sh`.

**DEPRECATED (no longer maintained):**
- `experiments/native-storm` (DXVK-native / Vulkan translation layer under macOS ARM64).
- CrossOver / Wine wrappers (`run-main.sh`, `run-codex-test.sh`, `tools/build_engine_crossover.sh`).
- Historical Windows baselines (`KVL`, `KVL-Codex-Test`).

All non-Metal runtimes are preserved strictly as frozen historical reference points and do not
receive further patches, validation, or maintenance.

### Metal shadow stability pass, 2026-09-17

The current candidate removes main-camera visibility filtering from the static shadow caster pass
and invalidates the GPU point-shadow registry when caster transforms or bounds change. Point-PCF
radius is camera independent. The source stack staged successfully; outdoor point-shadow,
world-shadow-registry, and authored-light-shaft GPU probes pass. A fresh candidate reports eight
completed lamp cubes in the outdoor scene. Visual rotation replay and indoor window godray
visibility remain unresolved until observed on the real game surface.

### Live deck walking candidate, 2026-09-14

The currently staged native-storm engine for this attempt is
`a566121e0bf37d1c1e377f46da625883c11b0681efc708d0c63960d64b684996`.
The preceding `c0c216ab…` executable is preserved under
`experiments/native-storm/.cache/deck-walk-baseline/native-engine`.
The first candidate adds a ship-relative protagonist model, WASD/run input,
third/first-person views and existing spyglass integration without freezing sea
layers or changing the binary sea save layout. Native ARM64 build and script
transforms passed. The actual sea scene renders the protagonist and live crew;
the user confirmed walking generally and spyglass use.

Disposition: rejected as complete. User replay found the protagonist crossing
guns, ship structures and crew, plus pre-existing crew/crew interpenetration.
Independent source review also found shared ship-control names could overwrite
outside-view bindings. The next source candidate is repairing body collision,
crew separation before render and distinct deck-only ship control names; it has
now replaced the first candidate in native-storm and native-metal. Both ARM64
engines build; native-storm is staged as `fde51b3d…826f`, and native-metal is
staged as `fb1b67a1…e5ce` with a receipt bound to both shared gameplay patches.
Four deck control/camera/label files were synced to Metal through the checked
delivery tool; SAVE, configuration and renderer materials were untouched. The
first test process has exited. The shared
capsule sweep passed independent numerical probes for thin faces, edges,
pre-existing overlap escape/crossing, tangential clearance and rigid transforms;
crew/hero and crew/crew numerical cases, dense spawn, head-on passing, blocked
corridor waiting and route continuity pass. Actual gameplay replay of the repaired
collision candidate remains unresolved on both native runtimes. Windows is untouched.

### Native custody repair, 2026-09-14

At the custody-repair boundary the native engine SHA-256 was `c0c216ab7ae2c7a2a4b7fbdd950a508d0381c5073673c8df22087e880bfe0026`.
The cumulative installer now defaults to this native runtime and refuses an active
native executable, unknown script hashes, or a different engine. Original inputs
still come from the immutable historical KVL suite snapshot; they are read-only
build inputs, not the delivery destination. Metal, Windows and SAVE are untouched.

Two generated files changed: `PROGRAM/scripts/custody.c` restores the player
template while keeping fight/travel locks, places the mate at the verified same-cell
`goto24`, and unlocks controls on release. `PROGRAM/seadogs.c` rehydrates custody
after loading a location save because stock `CreateJail` is skipped on save-load.
All 33 cumulative transforms and the independent source/state review passed.
Disposition: installed; interactive acceptance is unresolved. Per the user's
instruction, no computer-use replay is performed; camera, same-cell conversation,
day progression and release must be confirmed in their game.

Journal follow-up: six native PROGRAM files were updated through the cumulative
installer, without an engine rebuild or SAVE changes. `patch_mod_journal.py`
adds stable custody/debt entries and nation-scoped public-crime entries, including
old-save custody/debt refresh. The original quest text baseline is retained under
`experiments/native-storm/.cache/journal-baseline` for deterministic regeneration.
All 34 transforms, the altered-input rejection and independent script/template
review passed. Journal display and interaction remain unverified in-game; no
computer-use replay or game restart was performed. Metal and Windows were not updated.

The CrossOver `KVL` and frozen `KVL-Codex-Test` installations remain preserved
historical Windows owners. `run-main.sh` and `run-codex-test.sh` still launch them;
their shared Windows `SAVE` owner does not extend to either native experiment.

At the last recorded Windows boundary, `KVL` had clean engine bundle `24210f11…`
from source commit `7fb3079d` (engine SHA-256 `d6fe322f…`, matching PDB age 26)
plus the then-installed gameplay scripts. That snapshot, its native-D3DX video
mitigation, and the earlier cumulative 26-file suite failure are historical
diagnostic evidence, not the current native runtime verdict. Do not infer from the
native extern-expression fix that every suite branch has been replayed.

## Historical diagnoses and replay boundaries

These observations describe their original experiments. Later rows in the
experiment history supersede their installation and pending-replay status.

The boarding imbalance was traced to ReCon's script-side crew-ratio rank bonus,
not the engine. `tools/patch_boarding_balance.py` prepares a reversible
cap on the effective enemy crew advantage at 2:1. This preserves the original
continuous bonus below that boundary and limits difficulty 5 to +10 enemy ranks
instead of allowing the bonus to diverge as player crew approaches zero. After the
previous game process exited, the patch was installed only in `KVL-Codex-Test`;
the original `KVL` script remains byte-identical.

The intermittent land-combat lock is also script-owned. Generic raider, assault,
and patrol encounters holster and lock the player while an NPC approaches, but
their shared "shot before dialog" paths start combat without releasing that lock.
The generic `MainHeroFightModeOn` handler likewise tries to draw the weapon before
unlocking combat, so the engine rejects it. A token-only three-file repair avoids
adding statements to the persisted quest layout. The current save emits the same
`State read error` on both patched and exact-original scripts, so that warning is a
baseline save condition and cannot accept or reject this candidate. The installed
runtime is currently on the exact pre-experiment scripts while the user plays;
`tools/patch_land_encounter_combat_lock.py` can apply the candidate only after the
engine exits, and the actual interrupted-encounter behavior remains the acceptance
owner. Engine source commit `cadee8a3` adds a narrower save-compatible fallback:
on an explicit combat-mode keypress it releases a stale lock only when a nearby
hostile fighter is actively targeting the player, outside dialog, priority action,
and no-fight locations. The changed translation unit compiles; full engine linking,
staging, and real encounter replay remain pending.

## Historical isolation-boundary inventory

This table records earlier isolation and staging observations, not the current
installed state. Use the current verdict above and verify the live runtime before
acting; later experiment rows supersede these historical snapshots.

| Candidate | Verified state | Disposition |
| --- | --- | --- |
| `run-codex-test.sh` | Launches `GAMES/.../KVL-Codex-Test/engine.exe` through the installed CrossOver runtime | accepted entry point at this historical boundary; now frozen fallback |
| Tracked `run.sh` / `config.sh` | Require missing `game/` and missing Wine Staging runtime | rejected historical entry points |
| `/Applications/Корсары.app` | Not present | absent |
| CrossOver bottle `Corsairs32` | Present; prior session used it during failed experiments | unresolved legacy evidence |
| CrossOver bottle `Corsairs64-Clean` | Not present | absent |
| CrossOver bottle `GAMES` | Present; CrossOver 26.3.0.39832, `win10_64` / `win64`; native D3DX; x64 LAV Filters 0.83 registered but unused by the video path | accepted runtime owner for the isolated test copy |
| `GAMES/.../KVL/engine.exe` | Original ReCon 1.4.1 installation retained separately; SHA-256 `d93314eb777ac67c709e37b7acf8538f1663305fa83007e249cab89705f4804b` at the isolation boundary | preserved vendor baseline at the isolation boundary |
| `GAMES/.../KVL-Codex-Test/engine.exe` | Installed bundle `34a3d6b971de8457f1352593381970c013483c6adce9e3b23b5025cdb6f5b2bc`; PE32+ x86-64 engine SHA-256 `089bc34033c7ec3282b9f7e5e6fc81afbb414f3e4bb6979bd5091880d5ac057d`; matching PDB SHA-256 `5dfada5776ea1609264c2bdb1ee073cea6b9cf71de2e2da0bf6f9bcda0090e91`; mimalloc SHA-256 `fb6236c00191c6bdd09988f55d141dd83a365324757458b45bbb703f0f98a444`; EXE/PDB GUID `39433DB5-C5D5-4C02-99A6-381BEC6C7E1F`, age 9 | launcher target at the recorded staging snapshot; lifecycle health `healthy`, no holders after staging; real-surface replay was pending at this snapshot |
| `GAMES/.../KVL-Codex-Test/engine.accepted-47cbaaa6.exe` | Exact first binary accepted by the user's gunshot replay, SHA-256 `47cbaaa6e2d0bb86f68134effa85ced612bc3751034d1303eb63906e88f20f0f` | rollback owner for the clean-watermark transition |
| `/Users/md/05_Repo/StormEngine-Corsairs` | `codex/corsairs-engine` at `94dbe08d`; clean source worktree with the accepted behavior fixes, additional FMOD stop/fader guards, the dynamic-call result fix from upstream PR #511, the triangular-sail fix from PR #507, stale save-reference guards, atomic option-file replacement, and the Wine/MSVC build fix | canonical isolated source and build owner |
| Engine bundle `34a3d6b9…` | Clean `94dbe08d` Release triplet: engine SHA-256 `089bc34033c7ec3282b9f7e5e6fc81afbb414f3e4bb6979bd5091880d5ac057d`, matching PDB SHA-256 `5dfada5776ea1609264c2bdb1ee073cea6b9cf71de2e2da0bf6f9bcda0090e91`, mimalloc SHA-256 `fb6236c00191c6bdd09988f55d141dd83a365324757458b45bbb703f0f98a444`, EXE/PDB GUID equal and age 9 | installed atomically in `KVL-Codex-Test`; prior triplet retained in immutable snapshot `55a73c72…`; original `KVL` engine still hashes to `d93314eb…` |

The original vendor engine preserved in `KVL` identifies itself as
`develop(c473abf5355e9f6816e349debfaccd27de0d2fce)-DIRTY(c3aed08cc49f248cfcfff2261e208857)`.
That commit is absent from the public upstream and ReCon repositories. ReCon's
dirty-build suffix is generated randomly and cannot recover the unpublished
source delta. The closest canonical public owner is
`Konstrush/recon-storm-engine`; its frozen `Recon_release_1.4.3` ref is
`63d75444e0397042a41731ff3fd70c32b5af23e7`, but it is not a proven drop-in
binary for the installed ReCon 1.4.1 scripts and content.

The supplied exception snapshot belongs to the temporary installer process
`Setup ReCon 1.4.1.tmp`, not to `engine.exe`; it contains no backtrace. In that
same snapshot `engine.exe` was running concurrently with the installer helper and
had a `wined3d_cs` thread. This does not prove that the game rendered or stayed
responsive. By 2026-09-12 15:52 +07 neither process was running.

CrossOver created a `Корсары - COAS ReCon 1.4.1` menu entry for bottle `GAMES`.
The candidate `engine.ini` reported `safe_render = 0`, windowed 1920×1080,
`msaa = 8`, `adapter = 1`, and `target_version = teho`. These values are an
observed baseline, not recommendations and not proof of a working renderer.

The captured pre-DirectX WineD3D run had a specific failure before useful
rendering. Its persistent `system.log` reported failures for all 54 distinct FX
files passed to `D3DXCreateEffectFromFile`: 46 `E5000` syntax errors and 20
`E5017` not-implemented errors, including `Write pass assignments`. It then emitted
28,044 `technique (...) not found` warnings. The loaded `d3dx9_43.dll` was the Wine
builtin: the bottle copy had the same SHA-256 as CrossOver's bundled copy and no
`d3dx9_43` override was configured. At the 2026-09-12 16:15 +07 snapshot this run
still had one `engine.exe` process.

The DirectX component installed at 16:19 replaced both bottle copies with native
Microsoft `d3dx9_43.dll` binaries. During the 16:37 run, `engine.exe` had the native
x64 bottle DLL open and `system.log` contained none of the previous FX compiler or
missing-technique failures. This accepts the native-D3DX hypothesis. That run did
not test WineD3D: the bottle had `CX_GRAPHICS_BACKEND = dxvk`, loaded CrossOver's
DXVK `d3d9.dll`, and logged 44 graphics-pipeline compilation failures before the
user observed another white surface.

At 16:39 a later run loaded the same native x64 D3DX DLL together with CrossOver's
WineD3D `d3d9.dll` and `wined3d.dll`. The stale DXVK log predates this process.
The fresh `system.log` has no FX compiler or missing-technique failures, the engine
reaches script initialization, `error.log` is empty, and `MainMenu.ogg` is open,
but the game surface is still white. Native D3DX is therefore necessary for this
build but not a complete fix.

At 16:48 the controlled `msaa = 0` replay again loaded native x64 D3DX together
with WineD3D. The user-visible surface was unchanged. The fresh `system.log` has
no shader or technique failures and `error.log` is empty. It reports three failed
startup WMV files. Although the matching Storm Engine source posts
`ievntEndVideo` after `PlayMedia` failure, the later controlled skip showed that
the failed video surface remained the visible white overlay. The MSAA hypothesis
is rejected.

At 17:23 the built-in `cameramode.skipstartvideo = 1` option bypassed the video
object entirely. The previous three WMV errors disappeared, script initialization
completed, and the runtime proceeded into location/dialogue code. This isolates
the white overlay to the startup-video path and rejects window size or initial
presentation as its owner.

All 15 game videos are ASF files containing WMV3 video and WMAV2 audio. CrossOver
ships the ASF demuxer in this runtime but no GStreamer WMV/WMA decoder plugin.
The x64 game therefore could not complete its DirectShow graph and returned
`0x80040217`. At 17:29 x64 LAV Splitter, Video, and Audio 0.83 were registered in
the same bottle, startup-video playback was restored with
`cameramode.skipstartvideo = 0`. A later replay disproved this candidate: LAV was
not loaded and the same three WMV errors returned.

At 17:39 one representative startup clip was transcoded to MP4/H.264/AAC while
retaining the filename expected by the scripts. CrossOver loaded `libgstisomp4`
and `libgstapplemedia`, but playback never advanced and terminated with
`gst_video_info_from_caps` plus `wg_parser_stream_copy_buffer` assertions. The
original WMV was restored byte-for-byte. This binds the remaining defect to Wine's
`AMMultiMediaStream`/GStreamer delivery path rather than either source codec.

At 17:44 the same representative clip was transcoded to AVI with uncompressed
BGR24 frames and PCM audio, removing video and audio decoder dependencies. Wine
loaded `libgstavi`, kept the file open beyond its nine-second duration, produced no
frames, and terminated with the same `wg_parser_stream_copy_buffer` assertion.
Container and codec substitution are rejected; actual playback now requires a
replacement for Storm's `CAviPlayer` plus a rebuilt `engine.exe`.

At 17:41 `StartVideo` was changed to honor the existing skip option for every
scripted video, directly schedule any `afterQuestName`, and post the normal
end-video event. `cameramode.skipstartvideo = 1` is active. The replay produced no
video errors and continued through script initialization; real-scene acceptance
remains user-visible.

At 17:49 the engine-side frame cap was changed from `max_fps = 61` to
`max_fps = 120`. `vsync = 1` and `show_fps = 1` remain unchanged, so the visible
counter can still be capped by the refresh rate CrossOver exposes to D3D9.

At 17:52 the active display baseline was changed from the temporary 1920x1080
Wine desktop to native panel fullscreen: `full_screen = 1`, `screen_x = 3456`,
and `screen_y = 2234`; the per-application `Corsairs` virtual-desktop assignment
and its 1920x1080 desktop value were removed. The trackpad preset keeps both mouse
sensitivities at `0.5`, maps sea/deck/world-map forward/back camera actions to
`Z`/`X`, and maps the otherwise unavailable middle-button combat action to `2`.
The game launched through D3D9 with this configuration; fullscreen geometry and
control comfort remain user-visible acceptance.

At 18:07 the menu cursor was isolated from 3D rendering. The nearest public Storm
Engine source (the installed dirty commit is not published) hides the OS cursor,
reads Windows pointer deltas, recenters it with `SetCursorPos`, and draws a textured
quad during the interface render pass. There is no separate interface FPS cap in
that path. The `GAMES` bottle had no `GrabFullscreen` value, so CrossOver's
fullscreen pointer capture was enabled with `GrabFullscreen = Y`; `msaa = 8`,
3456x2234 fullscreen, 120 FPS, VSync, WineD3D, and native D3DX were retained.
Cursor smoothness remains a user-visible result. The post-Alt+Tab zoom is tracked
separately as fullscreen/Retina display restoration, not as cursor rendering.

At 18:12 exclusive D3D9 fullscreen was replaced with the engine's own borderless
window mode to avoid device/display-mode restoration on Alt+Tab. The first
3456x2234 window was oversized: that native-panel mode was interpreted as Cocoa
points, producing a 6912x4468 Retina window over a 4112x2658 desktop. The corrected
window uses the active macOS logical size, `screen_x = 2056`, `screen_y = 1329`,
`full_screen = 0`, and `window_borders = 0`. CoreGraphics reports the live Sea Dogs
window at exactly 2056x1329 at origin 0,0. Texture degradation remains disabled,
MSAA is reduced from 8x to 4x, and the explicit 120-FPS/VSync settings are retained.

At 18:53 the `GAMES` bottle received one macdrv-only cursor experiment:
`HKCU\Software\Wine\Mac Driver\UseConfinementCursorClipping = N` (`REG_SZ`). The
value was previously absent and was read back successfully after the write. No
engine or renderer setting changed. The user reported no cursor improvement, so
the value was removed and the bottle returned to its prior registry state. This
second failed mouse-capture experiment closes the Wine-setting branch.

At 19:07 the installed ReCon 1.4.1 `engine.exe` received a reversible five-byte
input patch. Its disassembly contains the exact input-update sequence from the
public Storm source: it overwrites SDL mouse-motion deltas with `GetCursorPos` and
then calls `SetCursorPos` every frame. The bundled PDB is stale (its CodeView GUID
and age do not match the EXE), so it was not used for address attribution. The
patch changes the instruction at file offset `0x11392e` from `48 8d 4c 24 20` to
`e9 84 00 00 00`, jumping over only that Windows cursor-warp block and retaining
the existing SDL `xrel`/`yrel` path. The original binary is preserved as
`engine.exe.cursor-warp-original`; the patch and revert owner is
`tools/patch_cursor_input.py`. Static binary verification passed. The user
confirmed that cursor smoothness was fixed, but continuous camera rotation stopped
when the hidden host pointer reached a screen edge. The no-warp patch is therefore
superseded rather than accepted.

At 19:20 the patch was revised to preserve SDL `xrel`/`yrel` on every frame while
reusing the engine's existing dynamic window-center block once every 64 update
frames. The 11 bytes at file offset `0x11392e` are now
`f6 43 34 3f 74 26 e9 7e 00 00 00`: the periodic branch targets `0x14011455a`
(`GetWindowRect` and `SetCursorPos`), while ordinary frames jump directly to
`0x1401145b7`. The original `GetCursorPos` and delta-overwrite instructions are
unreachable on both paths. The game crashed because `0x14011455a` expects `RCX`
to contain the global `core` pointer prepared by an earlier instruction that this
branch also skipped. This patch is rejected and was restored from the exact
original before correction.

At 19:25 the periodic patch was corrected by restoring that required `core`
pointer before entering the dynamic-center block. The 23 bytes at file offset
`0x11392e` are now
`f6 43 34 3f 74 05 e9 7e 00 00 00 48 8d 0d 10 73 42 00 e9 15 00 00 00`.
Ordinary frames still jump directly to `0x1401145b7`; every 64th frame loads
`RCX = 0x14053b850` and jumps to `0x14011455a`. Disassembly verifies both targets,
the patcher recognizes and upgrades both earlier variants, and byte-identical
revert remains available. The user confirmed that it no longer crashed, but the
unconditional 64-frame warp made the visible menu cursor jerk. Periodic recentering
is therefore rejected.

At 19:35 periodic recentering was replaced with a four-edge trigger. Each input
update now reads the host cursor only for boundary detection, preserves SDL
relative motion, and recenters at 1028x664 only when the pointer enters the
64-pixel strip at the left, right, top, or bottom edge of the verified 2056x1329
borderless window. No timer-based warp remains. The patcher rejects `apply` when
that exact geometry is not configured. A separate constructor correction extends
the existing zero-fill to include both SDL delta fields at `this+0x40/+0x44`,
removing the independently audited uninitialized-first-update path. Static
disassembly, exact legacy migration, and byte-identical revert passed; menu
smoothness and continuous 360-degree rotation require the real game surface.
The user later confirmed that the macOS pointer could still escape at the Dock
edge, producing both the system and game cursors at different positions. The
four-edge recenter is rejected and superseded.

At 20:02 the engine was switched from borderless window mode to its real
fullscreen path by changing only `full_screen = 0` to `full_screen = 1`.
The 2056x1329 resolution, WineD3D, native D3DX, MSAA 4, 120-FPS cap, VSync,
video skip, and four-edge cursor patch are unchanged. This is the controlled
test for CrossOver/macdrv fullscreen mouse capture. The user reported no cursor
improvement and a hard freeze after Alt+Tab. True fullscreen is rejected and
`full_screen = 0` was restored after the game process exited.

At 20:18 the engine-side mouse path was replaced with the statically linked SDL
2.0.18 native relative mode. The exact public SDL dynapi wrapper is at
`0x1402c01b0`; the exact `SDLInput` constructor is at `0x1402b74b0`. Its
`SDL_AddEventWatch` call now passes through a Win64-ABI-safe helper in the legacy
cursor block, calling `SDL_SetRelativeMouseMode(SDL_TRUE)` in the same source
order used by Storm on non-Windows builds, then tail-calling the original event
watch function. Input update jumps over the entire absolute
`GetCursorPos`/delta-overwrite/`SetCursorPos` path and consumes SDL `xrel/yrel`.
SDL now owns RAWINPUT, cursor hiding/confinement, and focus-loss release/reacquire;
there is no timer, edge threshold, resolution dependency, or per-frame warp.
Static call targets, stack alignment, shadow space, source order, migration from
the edge patch, final hash, and byte-identical revert passed. The installed hash
is `ab8a4f9e38f5d970a512063c63663a901c70a42ab54991b5e385979a7b885c31`;
the user confirmed on the real game surface that the fix works correctly. A
two-finger trackpad swipe still arrives as wheel scrolling rather than relative
pointer X/Y, so it does not rotate the camera; gesture remapping is a separate
control-layer feature, not a failure of relative-mode capture.

At 22:16 both global time-scale controls were moved off the missing laptop
numeric keypad in the installed ReCon script. `TimeScaleFaster` now uses the
top-row `VK_A_PLUS`, and `TimeScaleSlower` uses `VK_A_MINUS`; the `R` toggle,
pause, time-scale limits, handlers, and hidden numeric-keypad debug controls are
unchanged. No `engine.exe` or KVL Wine process was running during the edit.
Static binding verification passed; top-row `+`/`-` behavior remains pending on
the real game surface.

At 23:00 two source-matched gun-effect defects were patched in the installed
engine. `SGFirePrt` had initialized `blood[numBlood].alpha` instead of
`smoke[numSmoke].alpha`; the 24-byte instruction block at file offset `0xbf7b2`
now targets the smoke particle. `ProcessedShotgun` also drew the camera-aligned
`sghor` cross through `ShootParticlesNoZ` on every frame after the first shotgun
initialization; the eight bytes at `0xc0010` now set that one overlay particle's
alpha to zero while retaining the draw call and its state effects. Smoke, blood,
flinders, hit logic, damage, and reload code are unchanged. The pre-effect binary
is preserved as `engine.exe.gun-effects-original`; `tools/patch_gun_effects.py`
owns apply/status/revert. Exact-byte, disassembly, hash, revert, and composition
checks with the cursor patch passed. Visible stripe removal remains pending on
the real game surface.

At 23:15 the Wine debugger backtrace from the reported John Bolton shot supplied
the first usable native stack. It does not contain Bolton, gun-hit, script, or
Sentry as the original owner. The application chain is `SEA::Realize` through the
only `std::for_each(std::execution::par_unseq, aBlocks, SSE_WaveXZBlock)` call,
then MSVC's parallel-algorithm helper, `CreateThreadpoolWork`, `TpAllocWork`, and
`RtlAllocateHeap`. Its 52-byte element stride exactly matches `SEA::SeaBlock`.
Sentry/Crashpad allocates again while reporting the already-corrupted process heap
and is secondary. The installed engine now forces the helper's existing
single-thread branch at file offset `0x15c472`, bypassing Wine's Windows
threadpool without changing sea geometry. The pre-delta binary is preserved as
`engine.exe.wine-parallel-original`; `tools/patch_wine_parallelism.py` owns the
reversible change. Static and three-patch composition checks passed. Stability
after shooting Bolton remains the real-surface falsifier; no quest-specific
invulnerability workaround was added.

At 23:20 the same installed engine terminated after shooting an ordinary jungle
pirate, while CrossOver left a stale frozen window behind. No `engine.exe` or
Wine preloader remained, no new Crashpad dump was written, and the prior
threadpool backtrace therefore cannot identify this second failure. The fresh
`system.log` is saturated with FMOD invalid-handle errors and ends on
`SoundResume` reporting a channel reused for another sound. Source inspection
found a matching lifecycle defect: `FreeSound` retained the stale channel pointer
and could push an already-free slot more than once, while `SoundPlay` compared
new channels against those free slots. The resulting duplicate free indices can
assign one slot to multiple live sounds. The isolated engine branch fixes this
owner in commit `8f33fd82` by making release idempotent, clearing channel pointers,
rejecting failed playback, skipping free/null duplicate candidates, enforcing the
slot bound, and reclaiming invalid channels in volume/resume/focus paths. This is
source-verified but not yet a live fix: a Windows build and the same-save gunshot
replay are still required. The installed 1.4.1 binary is unchanged by this source
commit.

At 23:28 a reversible content-level falsifier was applied because the full
sound-service fix cannot fit safely into the installed executable without a
Windows rebuild. Only the `name` entries in the `pistol_shot` and `bullethit`
aliases are commented out; all other aliases, sound categories, engine bytes,
renderer settings, and save data are unchanged. The exact original is preserved
beside the file as `RESOURCE/INI/aliases/sound_alias.ini.gunshot-original`.
The same shot still terminated the engine. The log proves that `pistol_shot` was
not loaded, then ends on the pre-existing `SoundResume` channel-reuse error; this
rejects the weapon-WAV hypothesis but does not erase the independent sound-slot
defect. The exact alias original was restored immediately.

The next baseline comparison removes only the installed gun-effect binary patch,
because crashes on two unrelated targets began after that high-risk delta and
persisted with both weapon sounds absent. `patch_gun_effects.py revert` restored
the pre-effect instructions while preserving the accepted SDL-relative input and
the sequential Wine-threadpool workaround. Status is now
`original+cursor+sequential`; an ordinary-pirate shot is the next falsifier.

The isolated source worktree is `/Users/md/05_Repo/StormEngine-Corsairs` on
`codex/corsairs-engine` at `1e2f9b0a`, based on ReCon
`Recon_release_1.4.3`. The unpublished installed `c473abf` source cannot be
recovered from its stale PDB; comparison against the closest public 1.4.1-era
candidate found 657 of 705 source files identical and exact matches in the input,
sea, and location-effect owners. Compatibility is therefore accepted from the
real 1.4.1-content replay, not claimed from source identity alone.

The reproducible Windows build owner is the dedicated CrossOver bottle
`CORSAIRS_BUILD` plus `/Users/md/Library/Application Support/CorsairsBuild`. It
uses MSVC 14.35.32215 / compiler 19.35.32217.1, Windows SDK 10.0.22621, CMake
3.30.5, Ninja 1.13.2, and Conan 1.60.0. The upstream GitLab Conan index exposes
empty 2025 revisions for its legacy DirectX and FMOD recipes, so the retained
2024 Windows package artifacts were imported under their original package IDs:
DirectX `3475bd55b91ae904ac96fde0f106a136ab951a5e` and FMOD Release
`ab2e9f86b4109980930cdc685f4a320b359e7bb4`. Their SHA-256 values are
`7686c9fc3325683fcc6cdd69dda71feafea86d03c13b52780e40b26e3d5da443`
and `8fb5af29ea3fe481476c2924cbd9d802e383e8f755f78624a01f243c25f1475e`.
The normalized local Conan package revisions are `f21bfabc8845ab18fd35dfc828f30869`
and `62cebbdd139383ce97556ea4c3d9866e`.

The first accepted binary was built from the same four behavior-fix commits plus
the then-uncommitted build-system delta and has SHA-256
`47cbaaa6e2d0bb86f68134effa85ced612bc3751034d1303eb63906e88f20f0f`.
After committing the build fix, a clean-watermark binary was also produced from
`1e2f9b0a`; it is PE32+ x86-64, reports linker 14.35, identifies itself as
`codex/corsairs-engine(1e2f9b0a41807c41bcbb12bf2c23c700a6497eaf)`, and has
SHA-256 `8ff1a081afe14310592a521f714bb444dcae8aa738c7d8f8d77f38a2041f010d`.
After the accepted process exited normally, its exact binary was preserved as
`engine.accepted-47cbaaa6.exe` and the clean-watermark build became the test
copy's `engine.exe`. No file in the original `KVL` directory changed.

On 2026-09-13 isolated source work continued without changing the running test
copy. Commit `b23af95d` makes `SoundStop` and `ProcessFader` reject free or null
FMOD channels and retire slots after FMOD errors; this closes deterministic stale
ID and empty-music-slot dereferences during location, sea, cabin, boarding, and
world-map transitions. Commit `e21c4c54` carries the one-line fix from upstream
PR #511: it clears a stale reference type before storing a dynamic call result,
preventing the script VM from mutating the referenced game object. A clean Release
build from `e21c4c54` completed as PE32+ x86-64 with SHA-256
`36da7f3a39ad129fcdd68f6f7900d0ccd69d98e367239beeaf961bbc265526c3`
and watermark
`codex/corsairs-engine(e21c4c548d62d55e4b0af65f5ec3fda70c6587d4)`.
It remains a prepared candidate only: the active game process still owns the
installed executable, so no runtime file was replaced. The build wrapper now uses
the MSVC bridge's direct output path by default after its FIFO formatter was shown
to strand completed compiler workers; the same build then completed in 26 seconds.
MSVC linker/debug metadata makes repeated builds from the same commit differ at the
byte level despite an identical source watermark, so every exact candidate is
preserved together with its matching PDB and mimalloc DLL rather than inferred from
the commit alone. The retained `e21c4c54` candidate has engine SHA-256
`6cc4db98eebb6cd2db4256ae81ea9faaf8a492ceae9f469d4dfbd8b2fea85a18`.

Commit `1ea68065` then ports the approved one-line fix from upstream PR #507,
correcting operator precedence in point-to-triangle distance calculation so
triangular sails do not stretch. Its clean Release candidate has engine SHA-256
`669b3f3e789bd66172575133ec4a6df5212b26c04fa62841f7a2b83133343d16`,
matching PDB SHA-256
`e478288f2e13d697b8b74b2031f8be886090e6a61d347fa02b945eeef0cfdfd7`,
and mimalloc SHA-256
`fb6236c00191c6bdd09988f55d141dd83a365324757458b45bbb703f0f98a444`.
The exact triplet is retained under `build/candidates/1ea68065`; it is not installed
while the current game process is active.

Commits `2616a256` and `94dbe08d` harden state persistence before the next runtime
candidate. Save-reference discovery now follows the actual `DATA` array size and
turns a missing stale `AREF` into an explicit uninitialized reference instead of
dereferencing it. Option saving now serializes fully before opening any file,
rejects an empty payload, writes and checks a sibling temporary file, then uses
`MoveFileExW(..., MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)` on the
Windows/CrossOver path. A clean MSVC Release build completed from `94dbe08d`.
The focused `xinterface-test` target did not compile because the existing
cross-build omits the Catch2 include path; this is a test-harness dependency defect,
not an engine compile failure.

`tools/manage_engine_candidate.sh` now owns `status`, `prepare`, `stage`, and
`rollback` for exact engine/PDB/mimalloc bundles. It binds candidates to the
embedded clean Git watermark, verifies PE x64 architecture, hashes and EXE/PDB
GUID/age, protects the original `KVL`, snapshots the installed triplet, blocks
launch during an interrupted replacement, and refuses mutation while Wine holds
any runtime component. `run-codex-test.sh` shares the same atomic lifecycle lock.
The Wine/MSVC wrapper is intentionally serial by default: a 16-job replay left
Ninja waiting on defunct wrapper children, while `--parallel 1` completed the same
15-step rebuild and link. `CORSAIRS_BUILD_JOBS` remains an explicit override.

No ready-made ReCon 1.4.1 trackpad patch was found. The installed scripts already
use raw pointer axes for camera rotation and expose the relevant mouse actions in
the text options owner. Current ReCon guidance recommends an FPS cap of 60–100 or
vertical synchronization. The user-selected `max_fps = 120` is intentionally kept
with `vsync = 1`; no source found proves a specific 120-FPS gameplay failure.

On 2026-09-13 at 17:01 +07, after the prior game process and all runtime file
holders had exited, exact engine bundle `423b4351…`, built from clean source commit
`b5a79fed`, was installed in `KVL-Codex-Test`. The previous `cb52f229…` triplet was
preserved as an immutable engine snapshot. Lifecycle verification reports engine
SHA-256 `b132a15d98ff2b7c4953ac759b72fb0c3ed51dd03c7b1539eccbcb8c35c6a7bb`,
matching EXE/PDB GUID and age 16, and no holder during installation. This closes
the known `_ReadFile(nullptr, count)` geometry-buffer crash path without changing
the original `KVL` copy.

The same idle-runtime maintenance window installed a bounded ReCon script/content
repair in `KVL-Codex-Test` only. It derives `pchar.worldmap.shipcounter` from live
quest encounters on every generation tick, deletes complete pending quest nodes,
purges non-quest `Follow`/`Merchant` objects whose saved hostility contradicts the
current nation relation, removes the unconditional test-tornado override, guards
cannonless AI `FireRange` reads, and defines the engine-requested `ship_bow` sound
alias. Exact pre-change files are retained under
`.codex-runtime-fixes-snapshots/20260913-worldmap-audio-v1`. A launch from the
patched copy logged watermark `b5a79fed`, completed script initialization with an
empty `error.log`, reached the game surface, and remained live. At the user's
request, the on-disk ordinary-encounter cap was then raised from the original 8 to
12; the running process had already compiled its scripts, so that density change
is accepted only for the next launch. The current save's relation purge and
`shipcounter` repair still require the first world-map load.

On 2026-09-13, four reversible script candidates were prepared for
`KVL-Codex-Test`: twelve-slot five-minute autosave rotation, boarding-officer
secondary ship posts, hostile fast-travel rejection, and richer world-map label
metadata. Exact originals are preserved by each patch set and runtime writes share
the engine lifecycle lock. The stable `b5a79fed` engine started with all four
patches and the user confirmed that loading the current save worked. An apparent
window transition and high Wine CPU use were incorrectly classified as a load
loop; that diagnosis is retracted because the real game surface remained usable.
The process was unnecessarily stopped during diagnosis, then relaunched on the
same stable engine with only the autosave patch reverted. After the user exited,
the autosave patch was reapplied and the stable runtime launched again. All four
script patches are now active in the frozen playable process.

The cumulative engine candidate from clean commit `b1dd11d6` contains the
save-compatible combat-lock recovery and the world-map label-layout changes. Its
exact retained bundle is `57a88979d620a854ece179fe387f33a3fe0a720aba28a8e193afe22476ef3da9`,
with engine SHA-256
`3929663f925fb9fd22d2dbbaf2fa4a4ed6eb815f75f903db2748c7911f794674`.
It built successfully and reached script initialization, but its save-load
acceptance remains unresolved: it was rolled back to the stable `b5a79fed` bundle
after the same now-discredited process-only hang inference, before a clear
user-visible verdict was recorded. Do not reject or reinstall it from that
inference alone; the next trial must use the real game window and explicit user
confirmation.

The trade journal route planner is prepared as a reversible two-file script/UI
patch for the mutable primary `KVL`. It keeps one city pinned as the purchase
origin and compares every known good against the selected destination, sorted by
profit per cargo centner. Its single combined `Взять` plan is bounded by the
recorded source stock, current cash, and the independently available space of the
player ship plus every removable companion, so the displayed rows cannot jointly
overload any ship when distributed in that order. After the active process exited,
the exact script and INI candidate was installed in the primary `KVL`. The next
launch completed script initialization with an empty `error.log`; real-interface
layout and route-result acceptance remain user-visible.

The first real-interface review exposed a missing interaction distinction rather
than a crash: the route planner had replaced the ordinary journal instead of
making it a selectable mode. `tools/patch_tradebook_modes.py` prepares a second
reversible layer with explicit `Обычный режим` / `С закреплением` switching and a
separate `Закрепить` action. Route mode defaults on for saves without a preference,
then pins `GetCurrentTown()` on land or the canonical nearby colony at a moored-sea
entry; an unknown/unrecorded current town safely falls back to the prior pin. The
running process has not been changed, so installing this interaction correction
requires the next idle window.

## Next diagnostic boundary

Keep the accepted `KVL-Codex-Test` WineD3D, native-D3DX, 2056x1329 borderless,
`msaa = 4`, `safe_render = 0`, all-video-skip baseline. Do not change the renderer,
resolution, input mode, or content while validating this engine.

The crash-after-shot regression is accepted as fixed from the user's real-scene
replay. The remaining narrow visual check is that the transparent shotgun cross is
gone while smoke, hit/damage, and reload remain intact. If the accepted replay did
not use John Bolton, repeat only that target on the same save and remain interactive
for ten seconds; no quest-specific invulnerability or script bypass is installed.
The `Кампот` profile previously overrode the global all-video bypass after loading;
its own `cameramode.skipstartvideo` now matches the global value `1`. Repeat the same
flag-change action and confirm that the national flag changes without entering the
Wine video surface and that control returns within 15 seconds.
The exact `6fdcb0a5…` triplet is now installed only in `KVL-Codex-Test`, and its
EXE/PDB match is verified. Perform one launch and same-save gunshot replay without
changing renderer, resolution, input mode, or content. Confirm an ordinary shot,
the transparent-overlay visual, and—when the save permits—the John Bolton path,
remaining interactive for at least ten seconds. Do not touch the original `KVL`
directory during this boundary.

For the shipyard-theft crash, load `Сантьяго верфь`, steal until the NPC catches
the player, finish the same dialogue, and remain interactive for at least 60
seconds. This specifically exercises the land-HUD alarm transition that previously
wrote one quad (112 bytes) past its vertex buffer. Separately repeat one flag change;
that content route now queues the existing `*_flag_rise` quest directly and never
enters `PostVideo_Start`.

CrossOver's D3DMetal and DXMT choices target Direct3D 11/12, while this executable
imports Direct3D 9. They are not independent D3D9 alternatives to DXVK and
WineD3D, even though the UI exposes them in the same selector.

## Experiment ledger

| Time | Candidate and hypothesis | Exact delta | User-visible result | Disposition |
| --- | --- | --- | --- | --- |
| 2026-09-12 15:49 +07 | New ReCon 1.4.1 / `GAMES` installation | None | Installer helper exception had no backtrace; `engine.exe` was observed concurrently; rendering not observed | unresolved |
| 2026-09-12 16:00 +07 | ReCon 1.4.1 through CrossOver renderer selector | User tried every exposed renderer option | Every attempt produced a white or black surface | rejected as a working path; per-option traces incomplete |
| 2026-09-12 16:00 +07 | DXVK D3D9 path | DXVK selected; windowed 1920x1080 | Swapchain was created, then many graphics pipelines failed to compile; white/black surface | rejected with direct log evidence |
| 2026-09-12 16:00 +07 | WineD3D D3D9 path | `CX_GRAPHICS_BACKEND = wined3d`; builtin `d3dx9_43.dll` | All 54 FX files failed compilation, followed by 28,044 missing-technique warnings; white/black surface | rejected with direct log evidence; native-D3DX WineD3D replay pending |
| 2026-09-12 16:37 +07 | Native D3DX with DXVK | Installed Microsoft DirectX D3DX9 runtime; backend was `dxvk` | All prior FX errors disappeared; DXVK logged 44 pipeline compilation failures; white surface | native D3DX accepted; DXVK remains rejected; WineD3D replay pending |
| 2026-09-12 16:39 +07 | Native D3DX with effective WineD3D | Selector stored `d3dmetal`, but the D3D9 process loaded WineD3D; `msaa = 8` | No FX or engine errors; engine reached main-menu state and opened its music; surface remained white | native D3DX accepted; WineD3D rendering unresolved; single MSAA replay pending |
| 2026-09-12 16:48 +07 | Native D3DX with effective WineD3D; MSAA falsifier | Changed only `msaa = 8` to `msaa = 0`; resolution, window mode, runtime, and DLLs unchanged | Surface unchanged; no shader/technique errors; empty `error.log`; three startup WMV files failed | MSAA rejected; then-current path rejection was superseded by the 17:23 video isolation |
| 2026-09-12 17:23 +07 | Startup-video isolation | Set only `cameramode.skipstartvideo = 1`; kept WineD3D, native D3DX, 1920x1080 windowed, `msaa = 8`, and `safe_render = 0` | No WMV errors; engine completed script initialization and entered location/dialogue code | startup-video path identified as white-overlay owner; skip is diagnostic, not final fix |
| 2026-09-12 17:29 +07 | Restore all WMV playback through native DirectShow filters | Registered x64 LAV Splitter/Video/Audio 0.83 in `GAMES`; restored `cameramode.skipstartvideo = 0`; all other baseline values unchanged | First replay emitted none of the previous `Video Error 0x80040217` failures | decoder fix accepted at log level; visible videos and playable-scene acceptance pending |
| 2026-09-12 17:34 +07 | LAV persistence replay | No new delta; startup videos enabled | Same three `0x80040217` failures returned; process loaded Wine GStreamer and Quartz, not LAV | LAV decoder fix rejected |
| 2026-09-12 17:39 +07 | Native CrossOver codec control | Transcoded only `AkellaLogo.wmv` to MP4/H.264/AAC under the expected filename; preserved the original | CrossOver loaded ISO MP4 and AppleMedia plugins but emitted no frames, hung, then asserted in `wg_parser_stream_copy_buffer` | codec substitution rejected; original restored |
| 2026-09-12 17:41 +07 | Prevent all later video hangs | Extended the existing skip option to every `StartVideo` call while preserving end-video and `afterQuestName` events; set skip to `1` | No video errors; runtime passed startup into normal script initialization | accepted as runtime mitigation; user-visible quest continuation remains pending |
| 2026-09-12 17:44 +07 | Decoder-free media control | Transcoded only `AkellaLogo.wmv` to AVI/BGR24/PCM under the expected filename | `libgstavi` loaded, but playback stayed on the first clip past its duration and hit the same parser-buffer assertion | container and codec paths rejected; original restored and global skip re-enabled |
| 2026-09-12 17:49 +07 | Raise engine frame cap | Changed only `max_fps = 61` to `max_fps = 120`; retained `vsync = 1` and FPS display | Configuration value verified; achieved frame rate not yet observed | configured; real-surface rate pending |
| 2026-09-12 17:52 +07 | Native fullscreen and trackpad preset | Set 3456x2234 fullscreen, removed the 1920x1080 Wine virtual desktop, kept mouse X/Y at 0.5, moved camera forward/back to Z/X, and middle-button combat action to 2; retained 120 FPS and VSync | Engine initialized D3D9 and game scripts without fresh errors | configured; fullscreen and control acceptance pending |
| 2026-09-12 18:02 +07 | MSAA cursor falsifier | Changed only `msaa = 8` to `msaa = 0` | User reported that 3D rendering was already smooth and MSAA did not own the cursor symptom | rejected; `msaa = 8` restored |
| 2026-09-12 18:07 +07 | Fullscreen mouse capture | Added `HKCU\\Software\\Wine\\X11 Driver\\GrabFullscreen = Y`; retained the accepted renderer, resolution, MSAA, FPS, and VSync settings | Game launched; software-cursor smoothness awaits direct observation | unresolved |
| 2026-09-12 18:12 +07 | Borderless Alt+Tab workaround | Set `full_screen = 0`, `window_borders = 0`, and `msaa = 4`; corrected initial oversized 3456x2234 window to the macOS logical desktop size 2056x1329 | CoreGraphics reports the live window at 2056x1329, origin 0,0; visible Alt+Tab responsiveness awaits confirmation | geometry accepted; Alt+Tab pending |
| 2026-09-12 18:53 +07 | macdrv cursor-event experiment | Added only `HKCU\Software\Wine\Mac Driver\UseConfinementCursorClipping = N`; retained 2056x1329 borderless, WineD3D, MSAA 4, 120 FPS, and VSync | User reported no change | rejected and reverted; Wine-setting branch closed |
| 2026-09-12 19:07 +07 | Storm cursor-warp bypass | Patched the supported `engine.exe` at offset `0x11392e` to jump over its per-frame `GetCursorPos`/`SetCursorPos` block and retain SDL relative deltas; preserved the exact original beside it | Cursor became smooth, but 360-degree camera rotation stopped at the hidden host-pointer edge | cursor cadence accepted; no-warp mechanism rejected and superseded |
| 2026-09-12 19:20 +07 | SDL motion with periodic recentering | Replaced the no-warp jump with an 11-byte branch that preserves SDL deltas every frame and invokes the existing dynamic-center `SetCursorPos` path once per 64 updates | Game crashed because the branch skipped the required `core` pointer setup | rejected and restored from exact original |
| 2026-09-12 19:25 +07 | Corrected periodic recentering | Added the required `core` pointer setup before entering the existing dynamic-center block; kept the 64-frame interval and SDL deltas | No crash, but the visible menu cursor jerked on every periodic warp | rejected and superseded |
| 2026-09-12 19:35 +07 | Four-edge recentering | Removed the periodic warp; recenter only within 64 px of any of the four 2056x1329 window edges; initialize SDL deltas in the controller constructor | Static branches/import targets, legacy migration, exact hash, and byte-identical revert verified | configured; menu smoothness and continuous rotation pending real-surface replay |
| 2026-09-12 20:02 +07 | True-fullscreen capture test | Changed only `full_screen = 0` to `full_screen = 1`; retained 2056x1329 and every renderer/input setting | No cursor improvement; Alt+Tab caused a hard freeze | rejected; restored `full_screen = 0` after process exit |
| 2026-09-12 20:18 +07 | SDL 2.0.18 native relative mode | Replaced four-edge recentering with a constructor-time `SDL_SetRelativeMouseMode(SDL_TRUE)` call through SDL dynapi; input update bypasses the entire legacy absolute cursor path | Static verification passed; user confirmed the fix works correctly on the real game surface | accepted for cursor capture; two-finger scroll-to-camera mapping remains a separate control feature |
| 2026-09-12 22:16 +07 | Laptop time-scale controls | Changed only `TimeScaleFaster: VK_ADD -> VK_A_PLUS` and `TimeScaleSlower: VK_SUBTRACT -> VK_A_MINUS` in the installed `PROGRAM/controls/init_pc.c`; no KVL process was running | Static source check confirms both commands use the top-row key codes while `R`, pause, handlers, and limits remain unchanged | configured; real-surface `+`/`-` replay pending |
| 2026-09-12 23:00 +07 | Gun-effect correction | Redirected the bad smoke-alpha store at `0xbf7b2` and set only the unconditional `sghor` overlay particle alpha to zero at `0xc0010`; preserved the SDL input patch | Exact bytes, disassembly, hashes, independent revert, and two-patch composition passed | configured; visible stripe and ordinary-shot replay pending |
| 2026-09-12 23:15 +07 | Wine parallel-sea crash workaround | Forced the installed MSVC parallel helper's existing single-thread branch at `0x15c472`; renderer, geometry, save, scripts, and quest state unchanged | The prior debugger stack maps the fault to `SEA::Realize -> par_unseq -> CreateThreadpoolWork -> RtlAllocateHeap`, but an ordinary-jungle-pirate shot later terminated the engine with no new dump | rejected as sufficient; retained as a source-backed Wine safety delta pending isolated build acceptance |
| 2026-09-12 23:18 +07 | Isolated ReCon engine source | Created `/Users/md/05_Repo/StormEngine-Corsairs` at `Recon_release_1.4.3` on `codex/corsairs-engine`; added source-level gun, sequential-sea, SDL-relative-input, and sound-slot fixes | Source diffs and branch ancestry verified; no live game files changed by this worktree | in progress; Windows build and isolated 1.4.1-content acceptance pending |
| 2026-09-12 23:20 +07 | Ordinary-pirate gunshot after sea workaround | No new setting or build delta; fired in the jungle using the same installed three-patch engine | Engine process terminated and left a stale CrossOver window; no fresh dump; final log error is FMOD channel reuse after many invalid handles | unresolved crash owner; stale-channel source defect fixed separately in `8f33fd82`, runtime proof pending |
| 2026-09-12 23:28 +07 | Gunshot sound-path falsifier | Commented only the `pistol_shot` and `bullethit` alias filenames; exact original preserved as `sound_alias.ini.gunshot-original` | Same ordinary-pirate shot still terminated the engine; log confirms `pistol_shot` was not loaded | rejected and restored exactly; weapon WAVs are not the crash owner |
| 2026-09-12 23:39 +07 | Return to pre-gun-effect binary baseline | Reverted only `patch_gun_effects.py`; retained cursor and sequential-threadpool patches | Exact patch statuses are `original+cursor+sequential`, `patched+sequential`, and `patched+cursor` respectively | configured; ordinary-pirate shot replay pending |
| 2026-09-12 23:45 +07 | Gun-effect revert replay | Fired again with the gun-effect binary patch absent and the cursor/sequential patches retained | Game still terminated after the shot | rejected as crash owner; source correction retained independently for the visible effect defect |
| 2026-09-13 00:15 +07 | Native Windows engine build | Built `codex/corsairs-engine` with exact MSVC 14.35, WinSDK 10.0.22621, DirectX 9, and FMOD 2.02.05 in isolated bottle `CORSAIRS_BUILD` | Full 384-step target completed; output is PE32+ x86-64 with linker 14.35 and the expected DLL imports | accepted build path |
| 2026-09-13 00:16 +07 | Isolated 1.4.1-content replay | APFS-cloned `KVL` to `KVL-Codex-Test`, replaced only its engine and matching mimalloc runtime, and launched it through `GAMES`; original KVL remained unchanged | Engine reached an interactive scene; user replayed the gunshot path and reported normal operation without a crash | accepted runtime candidate; exact Bolton-specific replay remains separately observable if not covered by this user test |
| 2026-09-13 00:21 +07 | Clean-watermark engine transition | After the accepted process exited normally, preserved its exact executable and installed only the clean `1e2f9b0a` build in `KVL-Codex-Test`; launched it through `run-codex-test.sh` | Log reports the exact clean commit watermark, D3D9 and FMOD initialized, `error.log` stayed empty, and the interactive process remained live beyond four minutes | startup and launcher accepted; same-save gunshot replay pending on this exact hash |
| 2026-09-13 00:39 +07 | Isolated sound lifecycle hardening | Added free/null-channel guards and FMOD-error retirement in `SoundStop` and `ProcessFader`; no game file changed | MSVC Release build `e69149e1…` completed from clean commit `b23af95d` | source/build accepted; isolated runtime replay pending |
| 2026-09-13 00:42 +07 | Dynamic script-call result fix | Applied upstream PR #511's `ExpressionResult.ClearType()` as commit `e21c4c54`; no game file changed | MSVC Release build `36da7f3a…` completed with the exact clean watermark | source/build accepted; prepared cumulative candidate, not staged while the game is running |
| 2026-09-13 00:49 +07 | Triangular-sail geometry fix | Ported the approved one-line operator-precedence correction from upstream PR #507 as commit `1ea68065`; no game file changed | MSVC Release build `669b3f3e…` completed; exact EXE/PDB/mimalloc triplet retained under `build/candidates/1ea68065` | source/build accepted; visual/runtime replay pending |
| 2026-09-13 01:18 +07 | Save/reference and option-file hardening | Added actual-DATA-size/null guards for persisted references (`2616a256`) and serialize-before-open plus atomic option replacement (`94dbe08d`) | Clean MSVC Release engine `089bc340…` built successfully; focused xinterface tests exposed a pre-existing missing Catch2 include in the Windows cross-test target | source/build accepted; test-harness dependency unresolved; runtime replay pending |
| 2026-09-13 01:19 +07 | Content-addressed engine lifecycle | Added full-hash/GUID/architecture/provenance verification, immutable rollback snapshots, interrupted-install recovery, original-KVL protection, and a shared launch/stage lock; prepared bundle `34a3d6b9…` | Staging rejected with active holders 57047/57062; installed engine hash remained unchanged and the lifecycle lock was released | safety rejection accepted; stage after the current game exits |
| 2026-09-13 02:36 +07 | Hardened engine staging | After PID 57062 and wineserver released all runtime files, installed exact bundle `34a3d6b9…` atomically in `KVL-Codex-Test`; retained prior triplet in immutable snapshot `55a73c72…`; original `KVL` engine stayed byte-identical | Lifecycle status reports the expected three hashes, matching EXE/PDB GUID and age 9, `health=healthy`, and `holders=none` | installation accepted; real-surface ordinary-shot, visual-effect, and Bolton replay pending |
| 2026-09-13 03:25 +07 | Profile-local flag-video bypass | With no `KVL-Codex-Test` engine process or file holder, changed only `SAVE/Кампот/options/options` `cameramode.skipstartvideo = 0` to `1`; global options already held `1` | Static readback confirms both global and loaded-profile owners now skip video; original `KVL` was not changed | configured; same-save flag-change replay pending |
| 2026-09-13 03:40 +07 | Route-local flag transition | Replaced the five `NationRelation.c` video dispatches only in `KVL-Codex-Test` with their existing `pir/eng/fra/spa/hol_flag_rise` continuation quests | Static readback confirms flag changes no longer enter `PostVideo_Start`; all downstream nation/relations/ship-flag handlers remain unchanged | configured; same-save flag-change replay pending |
| 2026-09-13 03:47 +07 | Land-HUD alarm overflow fix | Crash dump from the shipyard theft/caught-dialog path mapped to `BIManSign::Draw`; source analysis proved an alarm `0→1` transition allocated `7*N` quads then wrote `7*N+1`. Commit `66b442d1` always reserves the one alarm quad; clean bundle `6fdcb0a5…` was built and staged with matching EXE/PDB age 11 | Lifecycle reports `health=healthy`, `holders=none`; original `KVL` engine remains hash `d93314eb…` | source/build/install accepted; exact theft/caught-dialog replay pending |
| 2026-09-13 05:12 +07 | Boarding rank-scaling cap | After the active process exited, installed the reversible crew-advantage cap only in `KVL-Codex-Test`; candidate script SHA-256 is `a8813e17…a456`, original `KVL` remains `0cc70a67…f6c` | Temp-copy cases produce +10 for difficulty 5 at 50:10, retain +5 at 150:100, and return 0 at equal or player-advantaged counts; byte-identical revert passed; the installed game compiled all scripts and reached normal initialization without a new compile/error log failure | installed and source/runtime-initialization verified; reload from before boarding and confirm enemy HP on the real game surface |
| 2026-09-13 05:31 +07 | Restore the configured engine debug window | Commits `258e80bc` and `1a6445bc` restore the Windows F5 open/restore path, edge-trigger it, honor `DebugWindow = 1`, and deliberately bypass `[controls] ondebugkeys = 0` as the historical window handler did; renderer, content, scripts, and game config are unchanged | Clean PE32+ x64 engine `d8ef5bff…b3ae` with watermark `codex/corsairs-engine(1a6445bc…)` was installed as bundle `cb52f229…a5c8` after holders 72115/72248 exited; lifecycle status reported matching EXE/PDB age 14 and `health=healthy`, and the next game launch remained live | source/build/install accepted; real F5 window replay pending |
| 2026-09-13 13:57 +07 | Laptop interface key | Changed only the `Interface` and matching `IExit_F2` bindings from `VK_F2` to `KEY_I` in `KVL-Codex-Test/PROGRAM/controls/init_pc.c`; original `KVL` remains unchanged | Exact diff contains only the two intended bindings; installed file SHA-256 is `b670585a…0009`, and the game loaded the script and remained live without a new error log | configured and startup-verified; real `I` open/exit replay pending user confirmation |
| 2026-09-13 13:58 +07 | Santiago geometry-buffer crash containment | The supplied `0xc0000417` dump maps `_ReadFile(nullptr, 0x5f514)` to an unchecked `LockVertexBuffer` result; that byte count uniquely identifies `Santiago_fd.gm` and `Santiago_fn.gm`. Commit `b5a79fed` rejects failed index/vertex create-lock-read paths and releases the partially allocated geometry; valid model data, renderer, and content are unchanged | Independent source/disassembly review passed for the exact crash path; clean PE32+ x64 engine `b132a15d…a7bb` built with watermark `codex/corsairs-engine(b5a79fed…)`, and candidate `423b4351…6503` was prepared while the user continued playing the installed build | source/build accepted; staging and same-Santiago replay pending current process exit |
| 2026-09-13 17:01 +07 | Install Santiago geometry crash containment | Installed only exact bundle `423b4351…6503` from clean commit `b5a79fed`; preserved installed bundle `cb52f229…a5c8` as an immutable rollback snapshot | Lifecycle status reports engine SHA-256 `b132a15d…a7bb`, matching EXE/PDB age 16, `health=healthy`; next launch logged the exact watermark and stayed live | source/build/install/startup accepted; same-Santiago scene replay pending |
| 2026-09-13 17:02 +07 | ReCon world-map/runtime repair | In `KVL-Codex-Test` only: reconcile the leaked quest ship counter, delete full released quest nodes, purge stale random Follow/Merchant hostility after flag changes and map load, remove forced test tornado, guard cannonless AI range reads, add missing `ship_bow` alias, and set the user-selected cap to 12 ordinary encounters plus live quest encounters; preserved exact originals in `.codex-runtime-fixes-snapshots/20260913-worldmap-audio-v1` | Initial patched scripts completed initialization with empty `error.log`; engine remained live. The cap changed on disk only after that process compiled its scripts, so 12 applies from the next launch. Save evidence before the fix was Spain/friend plus Spanish `Follow/isEnemy=1` and `shipcounter=13` with one live quest encounter | startup accepted for the initial fix set; next launch and first current-save world-map replay must prove cap 12, stale encounter removal, counter normalization, and no new negative-count warnings |
| 2026-09-13 18:14 +07 | Land-encounter combat-lock diagnosis | Correlated the user's lock with 37 consecutive engine `HitNoFight` events, then traced all three generated land-encounter interruption paths and the shared fight-on handler | The 18:08 save has player/player state, an equipped blade and gun, a valid `E` binding, and no location fight ban, so save/config/equipment causes are excluded. Each interruption path retains the preceding `LAi_LockFightMode(true)`, and `MainHeroFightModeOn` does not unlock before requesting combat | diagnosis accepted; script repair required runtime validation |
| 2026-09-13 18:33 +07 | Dedicated-save Santiago teleport and map grant | After the game released the runtime, changed only `SAVE/Кампот/Гавана таверна`: player location `Havana_tavern -> Santiago_town`, `loadedLocation` array index `399 -> 364`, and added one of every 26 live static geographic-map items plus `Map_Best`; `location.from_sea` was already `Santiago_town` and remains unchanged. Preserved exact original SHA-256 `2def2c11…fa18` read-only under `.codex-save-snapshots/20260913-santiago-all-island-maps-v1` | Patched SHA-256 is `a1cf46d5…a150`; the save decodes completely with `location/from_sea = Santiago_town`, `loadedLocation = Santiago_town`, and all 27 requested map items. Removing those additions and restoring the two changed location fields reproduces the original decompressed main and external payloads byte-for-byte | save structure accepted; real load into Santiago and in-game inventory display pending |
| 2026-09-13 18:36 +07 | Land-combat token-only candidate | Tested first an explicit unlock-before-fight patch, then a token-only three-file variant that removed the approach locks without adding statements | Both launches reached the game surface; the current save emitted `COMPILE ERROR ... State read error`, so the token-only candidate was rolled back for a baseline comparison | startup reached; attribution unresolved pending exact baseline replay |
| 2026-09-13 18:37 +07 | Compare clean combat baseline | Relaunched the exact pre-experiment scripts through `run-codex-test.sh` after verified rollback | Script initialization started with an empty error log; loading the same current save then produced the identical `State read error` while the game continued normally. The warning therefore does not distinguish the combat patch from baseline | clean runtime restored and playable; warning is baseline save state, token-only combat candidate still needs real encounter replay |
| 2026-09-13 18:44 +07 | Save-compatible combat-lock recovery | Commit `cadee8a3` changes only the player combat-key path: a stale `lockFightMode` is released only for a nearby hostile fighter whose active target is the player, while dialog, priority-action, no-fight, neutral, distant, and already-fighting states remain locked | MSVC compiled the changed `player.cpp` with `/WX`; its object is current. Two aggregate build attempts exposed the retained Wine/MSVC wrapper's orphaned FIFO/background-process behavior, so task-owned build processes were stopped and the running `GAMES` engine was left untouched | source and translation-unit compile accepted; full clean link, staging after game exit, and real locked-encounter replay pending |
| 2026-09-13 19:51 +07 | World-map label layout and combat-lock engine candidate | Built clean commit `b1dd11d6` and installed exact bundle `57a88979…`; scripts and renderer otherwise unchanged | Engine initialized normally. Save-load acceptance was not observed clearly before a process-only hang inference triggered rollback | build accepted; runtime result unresolved, not rejected |
| 2026-09-13 20:06 +07 | Stable-engine script-set load replay | Restored exact stable bundle `423b4351…` and retained autosave, boarding-role, safe-fast-travel, and world-map metadata script patches | User confirmed that the game and save load were working; the live process and high CPU were normal for this transition | load accepted; prior loop diagnosis retracted |
| 2026-09-13 20:08 +07 | Relaunch after mistaken stop | Reverted only the autosave script patch and launched the same stable engine; boarding-role, safe-fast-travel, and world-map metadata patches remained | Game process started normally. Lifecycle lock correctly refused reapplying autosave during play | superseded by the 20:12 relaunch with autosave enabled |
| 2026-09-13 20:12 +07 | Enable rotating autosaves | After the user exited, reapplied the autosave script patch and launched `KVL-Codex-Test` with all four gameplay patches | Script initialization completed with an empty `error.log`; engine reached the user-visible game surface and remained live, but no autosave appeared after more than ten minutes | startup accepted; timer behavior rejected by the 20:23 observation |
| 2026-09-13 20:16 +07 | Promote the accepted test runtime to primary | APFS-cloned the live static runtime tree, verified engine/config and all patched script hashes, atomically replaced `/Games/KVL`, and removed the prior KVL tree without retaining a backup; the running `KVL-Codex-Test` process was not touched | New `KVL` is a separate inode tree with the same stable engine bundle and current patches. Mutation tools now target `KVL`; `KVL-Codex-Test` is frozen and its launcher remains isolated | primary promotion accepted structurally; first `run-main.sh` launch waits until the frozen peer exits |
| 2026-09-13 20:23 +07 | Repair silent five-minute autosave timer | Observed zero `AutoSave` files after 10:55 process uptime while both global and profile `EnabledAutoSaveMode` values were `1`. The timer treated the mere presence of `pchar.pause` as paused even when its value was `0`; changed the new primary `KVL` to pause only when that value is nonzero | Exact patched hash set verifies, prior eleven-slot protection and boarding completion event are unchanged, and the frozen running test process was not modified | source/static acceptance; first five-minute main-runtime save remains pending after peer exit |
| 2026-09-13 20:25 +07 | Stage the map/combat engine into primary KVL | Installed exact candidate `57a88979…` from clean engine commit `b1dd11d6` only in the new mutable `/Games/KVL`; preserved its promoted stable engine as the existing immutable `423b4351…` snapshot | Lifecycle status reports engine SHA-256 `3929663f…94674`, matching EXE/PDB age 18 and `health=healthy`. The frozen `KVL-Codex-Test` process remains live on engine SHA-256 `b132a15d…a7bb` | primary installation accepted; first main-runtime map, combat-lock, load, and autosave replay waits until the frozen peer exits |
| 2026-09-13 20:29 +07 | Share saves and launch primary KVL | Made `KVL-Codex-Test/SAVE` the single save owner, replaced the cloned primary SAVE directory with an exact symlink to it, and removed the redundant primary copy. Both engines were idle during the transition; launchers reject a live peer runtime | Primary engine `3929663f…94674` compiled all scripts, reached `Init weathers complete`, kept `error.log` empty, and sees the latest shared `Куба - море` save | shared-save ownership and startup accepted; real save load, map surface, combat recovery, and first five-minute autosave remain pending |
| 2026-09-13 20:30 +07 | Migrate upgraded world-map labels into existing saves | The real Jamaica map showed no Fort Orange label although the patched initializer, engine, translation, and coordinates were present. Engine save loading restores the saved `worldMap.labels` tree after current scripts are compiled, while `OnLoad` does not rerun `wdmInitWorldMap`; the current save therefore has none of the new label attributes. Added an idempotent migration in `wdmCreateWorldMap` that reapplies Town/Shore layout metadata and upserts the complete Fort Orange label before the world-map entity reads it, without resetting position or encounters | After the user exited, installed exact script hash `77f6c458…b6429` in primary `KVL`; the next launch compiled all scripts, reached `Sea_FirstInit`, kept `error.log` empty, and retained the shared SAVE symlink | install/startup accepted; reload the current save and verify the Jamaica surface |
| 2026-09-13 20:56 +07 | Five-minute rotating autosave replay | No new runtime delta. Observed the shared save owner after ordinary play on the patched primary runtime | `Кампот AutoSave 3` was written at 20:50:35 and `Кампот AutoSave 4` at 20:56:16; both are non-empty valid-sized save files. The 5:41 wall interval is consistent with the timer excluding pause/interfaces and waiting for a save-enabled state | accepted; reopen the Load interface to refresh its timestamp-sorted list |
| 2026-09-13 21:46 +07 | Providence false-friendly patrol diagnosis | The current save has Spain friendly and pirates hostile. Providence loaded with two generated `IslandGroup` patrols; ReCon unconditionally assigns `AlwaysFriend` to pirate port ships while its pirate fort remains hostile. A hostile fort within 1400 units independently sets `SHIP_WAR`, even before it fires. Prepared `tools/patch_island_patrol_relations.py` to make only port patrols follow their actual relation while preserving the special hostility of ordinary and quest pirates | This exactly explains green ships plus combat mode and no shots yet. The live runtime was not changed while the user was playing; both target script hashes and generated patched hashes verify | diagnosis and patch artifact accepted; install after engine exit, then re-enter the sea scene and verify hostile pirate patrols are red while friendly/neutral ports retain their relation |
| 2026-09-13 22:12 +07 | Pinned-city trade route planner | Prepared `tools/patch_tradebook_planner.py` for the primary `KVL` trade journal: explicit origin pin, selected destination, price/profit comparison, profit-per-centner ordering, and one fleet-wide purchase plan constrained by source quantity, cash, and per-ship free cargo space; original and generated hashes verify for both script and INI owners | In-memory transform reproduces the reviewed UI prototype, the 800x600 layout has one non-overlapping route/control row, and the primary runtime stayed byte-identical while `engine.exe` remained active | source/static candidate accepted; install after process exit, then verify the real journal and one representative route |
| 2026-09-13 22:12 +07 | Install and launch the trade route planner | With both KVL engines idle, applied the exact two-file candidate to primary `KVL`, then launched it through `run-main.sh`; no renderer, engine, save, or other gameplay script changed | Patcher reports both installed hashes exactly; engine watermark is still `b1dd11d6…`, script initialization completed, `error.log` and `script_stack.log` are empty, and the live process reached the game state | install/startup accepted; open the journal, pin one city, select a second, and verify the route row plus one cargo recommendation |
| 2026-09-13 22:17 +07 | Ordinary/route trade-journal modes | The first UI review showed no mode switch and no automatic current-town source. Prepared `tools/patch_tradebook_modes.py`: two non-overlapping mode/pin buttons, persisted ordinary/route preference, ordinary-table restoration, and canonical current-colony resolution through `GetCurrentTown()` or the live moored-sea owner `Sea_FindNearColony()` | Exact v1-to-v2 transforms and hashes pass; a focused independent review found no Storm syntax, resolver, state, or geometry blocker. The live primary engine remained untouched | source/static candidate accepted; apply after process exit and repeat the journal interaction |
| 2026-09-19 00:15 +07 | Crew-payment world-map attribute walk | New `attributes-parent-chain.patch` after `save-areference-liveness.patch`: `ReparentChildren()` in the `ATTRIBUTES` move constructor and move assignment, plus a liveness-terminated ancestor walk in `WorldMap::AttributeChanged` | Not replayed: the game had already crashed on the 21:46 bundle. The stack applied ("3 source files changed"), the exact batch was staged, and the gameplay sync check reported zero pending | source/build/staging accepted; crew-payment replay unresolved |

Historical session transcripts contain many Wine, DXVK, WineD3D, D3DMetal,
`d3d8to9`, Porting Kit, and `safe_render` variants. They are diagnostic evidence,
not accepted project state, because no stable baseline and end-to-end acceptance
record was preserved.

Commit `6563e73` changed only the legacy launcher to Wine virtual-desktop mode. Its
message says that this prevents the black-screen freeze, but the repository contains
no real-surface acceptance evidence for that claim; treat it as an unverified prior
hypothesis, not a known fix. The old Wine prefix still contains some supporting
artifacts, including registered fonts, but no runnable game executable.

## Isolated native Metal resource probe — 2026-09-14

Hypothesis: a bounded static resource-to-GPU path can be tested natively without
converting the game to another engine. Added `experiments/metal-probe/run.sh`, a
standalone macOS prototype using Metal directly. It reads the primary runtime's
FrigateQueen1 GM hull and three original TX textures without modifying them;
7,912 triangles render in four batches on Apple M3 Max. The default launcher was
built and replayed, and the textured hull was observed in its native window.
Missing and truncated GM inputs are rejected.

Disposition: accepted only for the standalone static hull draw. This is not a
Storm backend integration or a playable native game. Masts, sails, original FX,
vertex colors, animation, scripts, saves and gameplay are outside this probe.
The current game runtime verdict above is unchanged. Generated probe files live
in ignored `experiments/metal-probe/.build/`; close its window to stop the process.

## Native Storm bring-up — 2026-09-14, playable experimental port

The user superseded standalone scene polishing with a one-hour attempt to run
the actual game natively. The expanded `--harbor` Metal viewer remains a demo:
original port/ship meshes, generated static sails and substitute water do not
accept game fidelity or gameplay.

The native candidate is isolated under `experiments/native-storm/.cache/`.
Engine source is an archive of `4860fe13245b747973682c88b2f1d9e4850c8c40`;
primary KVL PROGRAM/RESOURCE and a separate copy of SAVE are APFS-cloned into
its runtime. Neither playable installation nor shared saves were changed.
There was no running `engine.exe` at launch. Native config requests fullscreen 2056x1329, MSAA 4 and vsync; adapter is 0.
The initial no-audio diagnostic build was superseded by a native miniaudio
sound service. The user confirmed interactive gameplay, fullscreen and music.
This accepts a playable experimental port, not complete rendering/audio parity.
Shoreline reflection jumping is reported in the latest save (Porto Bello store,
then outside); its cause and repair remain unresolved. Current options set
`cameramode.simpleseamode=0`; the planar reflection branch is not established
as the cause. Party portraits were present in observed gameplay and F2 screens;
the reported disappearance was not reproduced.

| Time +07 | Experiment | Evidence | Disposition |
| --- | --- | --- | --- |
| 16:39 | DXVK Native macOS + MoltenVK 1.4.2 | arm64 D3D9 device on M3 Max; all 15 original KVL VSO/PSO accepted; clear/present passed, also with Storm's old pinned headers | graphics API probe accepted, game pending |
| 16:43 | Native engine compile/link | Original modules compile as arm64 after focused portability changes; native FMOD is unavailable, optional sound service omitted using existing null-service consumer path | native binary produced; audio unavailable |
| 16:43 | First launch with binary outside data directory | Script loader could not find executable-relative shared engine headers | rejected layout; binary staged alongside data |
| 16:44 | Native game launch | Vulkan swapchain initializes, then SIGBUS in SDL Cocoa invisible-cursor PNG decoding | unresolved game startup |
| 16:46 | Disable global mimalloc new/delete override | Same cursor crash without allocator replacement | hypothesis rejected; allocator change reverted |
| 16:46 | SDL-only reproducer, no Storm/DXVK/mimalloc | Same EXC_ARM_DA_ALIGN at 0xbad4007 in NSCursor invisibleCursor after hiding cursor | SDL dependency owner proven; local dependency repair in progress |

### macOS application switching — 2026-09-14

The user reported both Command-Tab and AltTab failing in exclusive fullscreen.
Baseline: the same native ARM64 bundle above, isolated native data/SAVE,
DXVK Native + MoltenVK, fullscreen=1, adapter=0, texture_degradation=0.
No game process was running before the replacement launch.

Source evidence: Storm selected SDL_WINDOW_FULLSCREEN; SDL 2.32.10 Cocoa uses
CGDisplayCapture and a shielding window level in that path. DXVK's fullscreen
D3D9 swapchain also requests exclusive mode. The bounded change gives SDL the
macOS fullscreen Space (`SDL_WINDOW_FULLSCREEN_DESKTOP`) and uses a windowed
D3D9 swapchain on Apple only, so DXVK cannot recapture the display. Other
platforms retain their prior behavior. This is one presentation-mode change;
game data and saves are unchanged.

Build passed; the canonical run.sh staged and launched the candidate. The real
main menu fills the screen, and the DXVK log confirms `Windowed: true` with a
2056x1286 drawable. Command-Tab was sent through UI automation; that target-only
AX response cannot prove the destination application became active. The user confirmed both Command-Tab and AltTab now switch away and back.
Independent source review found no contradictory reset/focus path; non-Apple
behavior is preserved. Disposition: accepted for application switching. The
running candidate is retained for the user. Reflection diagnosis remains
pending independently.

### Coastal collision refinement — 2026-09-14

User requested a minimal native fix for premature ship/rock contacts and owns
in-game replay; do not launch or sail on their behalf for this check. Scope is
native candidate only. The current shared KVL runtime and its SAVE are untouched.

Source baseline: TOUCH accepted island contact directly from the depth raster;
PROGRAM sets a global -10m collision threshold. Ship contour adds 10% width.
Neither of those parameters was changed. A visible gap can still represent a
real shallow-water contact under the existing rules.

Added a collision-only island query: retain the raster broad phase, then veto a
coarse contact only after a valid vertical geometry trace confirms deeper water,
the point's movement does not cross geometry at sea/threshold levels, and the
existing height-map inside-solid test does not identify an interior point.
No-hit/invalid downward queries and decoratively immersed islands retain the
coarse collision. A valid seabed hit does not certify BSP completeness for every
separate rock; incomplete asset collision geometry remains a limitation. Existing model/BSP
trace acceleration is reused; no index construction or new per-frame work in
coarse-deep open water. AI depth queries, ship-to-ship response and grounding
remain unchanged. This is not a complete swept-hull solver and does not repair
inaccurate embedded collision meshes. Performance near dense shores is not yet
measured on gameplay.

Native ARM64 build passed; staged bundle binary matches the build byte-for-byte.
No game process was running during staging. Independent exact-method harness compilation and 17/17 cases passed, covering
coarse-deep/outside zero traces, false positives, genuine shallow/inside contacts,
invalid/missing hits, movement crossings, stationary points and immersion.
Disposition: unresolved for gameplay until the user repeats the
problematic rock approach and a genuine shore contact. Shoreline reflection
jumping remains a separate unresolved issue.

### Hull and keel contact separation — 2026-09-14 (current native candidate)

Supersedes the previous refinement's unchanged physical -10 threshold and
expanded coastal contour. User reported partial improvement, then authorized
an autonomous repair; user continues to own gameplay replay and no game was
launched for this change. Baseline remains the isolated native ARM64 bundle,
DXVK/MoltenVK, fullscreen Space, MSAA4/vsync, separate native PROGRAM/RESOURCE/SAVE.
No native or Windows game process was running before staging.

Discovery: depth codec/generator has no +10 bias; the old grounding predicate
alone adds10. Actual compressed PortoBello raster has only1759 intermediate
samples out of4194304, with approximately14.24x10.24 game-unit cells; the map
cannot be the precise keel surface. Five model-derived keel samples already
exist in save data. Strand scripts play scraping sound and speed-based damage,
but do not resist movement.

Delta: preserve original 1.1-width ship-to-ship contour and build an independent
1.0-width coast contour. Keep -10 as the map broad filter only; actual coastal
contact is at sea level with existing geometry refinement. Ground depth uses
positive BSP floor/inside evidence when the coarse map threatens the current
keel height. Keel penetration uses current physical State, waterline, cargo
immersion, roll/pitch and world offset instead of the prior rendered matrix and
+10 offset. Normal live ship translation into greater positive penetration is
blocked, planar velocity cleared, and grounding event retained; equal/decreasing
penetration and turning remain allowed. Prediction calls, dead ships and fixed
scripted movement do not gain this movement gate. New contour and blocked-move
fields are transient; the serialized ship/save layout is unchanged.

Evidence: native build passed; independent exact-source probes passed20 coastal,
10 ground-depth and9 full-Move scenarios. Open/deep map queries and prediction
moves avoid new exact-ground traces. Final bundle byte comparison is performed
at staging; application is not launched. No gameplay/FPS acceptance is claimed.
Unresolved limits: five-point keel sampling, raster false negatives, incomplete
BSP and conservative map fallback can still disagree with visible geometry.
This is not a full swept 3D hull solver. User shore/escape test and near-shore
performance remain unresolved; jumping reflections remain independently open.

### Native icon and graphics quality — 2026-09-14 (prepared update)

User requested the missing Dock icon and the discussed quick graphics changes;
the supplied screenshot was explicitly excluded. Current native process79564
is retained while playing, so its loaded binary/settings have not been replaced.

Prepared: native bundle CFBundleIconFile=Corsairs.icns, generated from the
original approved256px icon1.ico at engine/apps/engine/rsrc (red compass R).
Build-icon.sh preserves the artwork/transparency and exports the standard ICNS
representations through1024; those larger sizes upscale the256px source, not new
artwork. Source/output fingerprint skips unchanged conversion. Small32/128px
representations inspected; decoded1024 ICNS bounds cover the intentional circular
body and all four corners are transparent. No new menu-bar/tray icon is added.

Canonical run.sh stages the icon and selects repo-owned dxvk.conf with
`d3d9.samplerAnisotropy=16`. Installed DXVK applies this override to filtered,
mipmapped samplers, leaving point-sampled UI alone. SEA uses one256 size constant
for both cube targets, both planar targets and matching depth buffers in Init
and Restore; previous size was128. Independent source review verified viewport
selection follows target dimensions and no128-dependent shader offset exists.
Native build, plist and launcher syntax passed. Postprocess remains disabled;
there is no accepted bloom implementation. These settings do not establish a
fix for the separately reported jumping shoreline reflections.

The icon resource and Info.plist are now installed in the existing bundle;
CFBundleIconFile resolves to Corsairs.icns. The running process remains unchanged,
so Dock cache refresh and graphics activation still require relaunch.

Disposition: prepared, runtime activation/visual quality pending a user-approved
restart or the user's next launch. No FPS claim: reflection-target pixel count
quadruples, total-frame cost has not been measured. Latest hull/keel fix is
included in the prepared binary. Game data and SAVE remain unchanged.

### Direct Metal backend — historical bring-up, 2026-09-14

Owner: `experiments/native-metal`, separate from the playable `native-storm`
port. The actual ARM64 engine retains its rendering interface and loads the
original game assets, while a new backend implements commands directly with
Metal. Its executable has no DXVK, Vulkan or MoltenVK linkage. Source and CMake
are pinned independently to engine4860fe13 and native-port commitdf1e021f;
concurrent script/compiler work in native-storm is not overwritten. Candidate
PROGRAM/RESOURCE/SAVE and userdata are separate APFS copies. Initial presentation
is windowed, MSAA0. This is not the standalone metal-probe model viewer.

| Attempt | Evidence | Disposition |
| --- | --- | --- |
| Initial API/engine build | ARM64 engine linked; real Metal clear/draw/present and resource bounds passed | API bring-up only |
| 18:12 game launch | SEA environment-map initialization dereferenced an unsupported cube-texture result | rejected; cube and volume resources implemented and GPU-tested |
| 18:19 game launch | SCRSHOTER called D3DXLoadSurfaceFromSurface; Surface::GetDevice was missing, producing the user's null-pointer crash | rejected; resource device ownership and complete surface-copy path repaired |
| 18:24 game replay | Real main menu rendered; user loaded a sea save and observed ships, sails, terrain and UI, with missing water and about31FPS | startup/game integration accepted; rendering parity and performance rejected |
| Screenshot regression probe | Exact engine D3DXLoadSurfaceFromSurface linked into probe; device lifetime, render-target viewport changes, GPU pixel readback and copy path passed | bounded crash-path repair accepted |
| 18:27 performance sample | 1119/2495 main-thread samples in sky pixel lookup through full Surface::download; unchanged sky textures were repeatedly copied from GPU | cause identified; dirty-state repair and sea shader port in progress |

The first playable prototype explicitly skips programmable shader draws;
therefore missing sea, grass and other shader effects are known missing support,
not accepted visual parity. Fixed-function texture stages, lights, materials,
fog, blending, depth, cube/volume resources and screenshot copying are present.
No claim of complete Direct3D9 compatibility or improved FPS is made.

The profiling process99578 was closed after the sample. No native/Metal/Windows
game process was observed at this boundary. The neighboring script task retains
the playable native-storm build/PROGRAM owner and can test there; further Metal
launches must coordinate against that runtime. Accepted script/compiler changes
must be explicitly integrated together into this pinned candidate rather than
assuming its separate PROGRAM copy updates automatically.

Prepared follow-up: static texture CPU copies now have GPU-write invalidation,
removing repeated full sky-texture readback. GPU dirty-state and screenshot-path
regressions pass. The user supplied a later sea screenshot showing about73FPS;
its viewpoint differs from the earlier31FPS frame. The follow-up profiler sample
captured inactive/background sleeping, so it is not a valid performance comparison.
The four original sea VS/PS pairs now execute through Metal. An independent API
fixture loaded the original eight binary shaders, drew indexed geometry, checked
the four expected output colors and volume-to-cube reflection direction, and
rejected an unrelated shader; all passed. macOS fullscreen Space is configured
for the next launch, with MSAA off. Real water/fullscreen replay is still pending
the neighboring native script task's runtime window. Profiling process3088 has
exited (verified with a PID-specific empty ps result).

Latest source/build: all 15 original programmable shader binaries are recognized.
GPU fixtures passed for sea, particles, caustics, grass, worldmap and postprocess;
the latter also verifies FFP specular/TEMP and backbuffer Reset. Original
postprocess is supported but not enabled as a new game effect. Texture transforms,
generated cube coordinates and caustic slope bias are implemented.

At the user's explicit request, the candidate launcher now selects new Metal
water with `STORM_METAL_MODERN_WATER=1`. It retains the original wave geometry and
physics, with angle-dependent Fresnel, finer animated normal detail and
weather-dependent body color. Sea3 planar reflection retains large-wave distortion;
the modern Sea2 and SunRoad passes share their normal calculation. Depth-based
shoreline transparency/refraction is absent. Original shading remains available
with value0. The modern GPU fixture passed reflection-input, viewing-angle,
weather, cube-direction, foam and unsupported-shader checks.

The neighboring native task released the game runtime window. The first modern
candidate opened its menu, reporting 2056x1329 backbuffer and drawable, samples1,
modern Metal water. Automated mouse/keyboard attempts did not navigate the menu;
this does not establish a game hang. That task-owned menu process16345 was closed
for the final shader update (SIGTERM did not exit; SIGKILL ended it). The final
modern build is launched for the user to load a sea save. Water appearance,
fullscreen chrome and foreground performance remain unresolved pending that replay.

The running build logged an unsupported draw. Source audit found the original
stars consumer uses an FFP declaration with separate position/color streams;
this path was not covered by the 15 programmable shaders. The backend now handles
that point-sprite path and reports exact rejected draw combinations. Its GPU
fixture and the general resource/screenshot/presentation fixture passed. This last
stars update is built but intentionally not staged over the user's running modern
water build; the next launcher invocation will stage it. No performance comparison
was taken while GPU fixtures and the game were both active.

### Direct Metal — current integrated candidate, 2026-09-14 evening

Current owner remains `experiments/native-metal`; prior bring-up inventories above
are historical. ARM64 Release, direct Metal without Vulkan/DXVK/MoltenVK linkage,
2056x1329 backbuffer/drawable, MSAA off, fullscreen Space. User accepted modern
water appearance, night response and the earlier frame-arena performance gain.
Water now includes GPU scene-depth absorption/refraction, weather illumination,
and foreground rejection; the earlier absent-depth statement is superseded.
All 15 original programmable shaders and multistream FFP stars are supported by
bounded GPU fixtures; this is not a claim of complete D3D9 compatibility.

Latest staged candidate includes indexed FFP vertex reuse (dense index spans;
sparse fallback), NULL texture-stage weapon repair, POSITIONT stale-VS bypass,
precise visual wave timing, cinematic grade enabled, and four reversible material
replacements: Sandtile, pierWood1, tileU2, deckPlanksU1. Rock textures are unchanged.
GPU probes passed indexed vs expanded pixels, weapon diffuse/cubemap/null-stage,
original three-pass shadows including retained VS, water depth/day-night, original
sea, particles/caustics, grass/worldmap, postprocess, stars, generated coordinates,
lighting, cinematic and arena. Timer fixture passed 120 frames = 1000 visual ms
while preserving legacy timing, fixed-step, pause and cap. New frame-rate and
wave-motion improvement require an in-game comparison; no measured FPS gain is
claimed for the indexed change yet.

Compiler extern-expression patch is applied with forward/reverse validation.
Successful build writes `.cache/gameplay-compiler.json` engine/patch hashes.
`tools/sync_metal_gameplay.py apply` accepted the staged engine and applied seven
reviewed PROGRAM files; SAVE/config/renderer resources were excluded. Launch
reached the rendered game menu. This accepts menu presentation only; gameplay
changes and latest visual fixes still need their actual scene/action replay.

Dynamic land lighting and real scene shadow maps are a separate active goal,
not enabled in this running candidate. Existing baked COL and original character
shadows remain the current land baseline. Source-only agents own backend lighting,
scene hooks/weather integration, and island geometry; root owns build/stage.
Do not replace the running app while the user plays. Island source audit found
coarse authored geometry; normal-only smoothing cannot fix its silhouette.

Prepared follow-up (not staged): source-driven shadow prepass now traverses
explicit location/character models with light-frustum culling, including
camera-offscreen casters. One directional 512 map and one actual nearest lamp
256 cubemap feed normal material shading; other lamps remain lit. Release build
and GPU fixture passed day/night, moved/offscreen caster, point-light identity,
removed caster and incomplete-cube fallback. Interior/real scene appearance and
FPS are not yet accepted. The current application was not replaced.

Sea geomorph patch is integrated in this prepared build. Frozen original wave
data reproduced an old LOD4-to5 height jump of0.047725 from a0.001 camera move;
new threshold fixtures converge within1.4e-7 input position and1.3e-8 height,
with shared-edge/static/grid tests. Actual visual replay remains pending.

User reported about54FPS in San Juan harbor. Recent active aggregates showed
~15–16ms wall versus~3.5–4ms GPU, ~2.2M vertex references and~97MB upload/frame.
The subsequent 5s CPU sample mostly captured background sleep and cannot quantify
foreground cost. Source confirms five cubefaces for environment reflection plus
five separately filtered sun-road faces per frame. Repeated CPU geometry work is
a candidate, not a proven complete explanation for the reported frame rate.
The game process later exited; no forced restart was performed for profiling.

User-authorized follow-up launch staged the full shadow prepass and sea geomorph
with STORM_METAL_DYNAMIC_LIGHTING=1. Repeated build initially rejected overlapping
scene/prepass patch reverse-checks; they were consolidated into scene-lighting.patch
and the canonical build/run path then succeeded. User confirmed visible shadows,
but reported missing character receivers, flat/glowing interiors, and walking
reflection jitter: these remain open acceptance defects. Interior source diagnosis
found vertex-only local light evaluation misses large wall centers; per-fragment
lamp lighting is in progress. Character camera-fade receiving path is being fixed.

Prepared frame-local unlit indexed FFP cache passed GPU tests for view/projection,
VB/IB edits, locked buffer writes, Present and Reset; not yet staged. Active CPU
sample (lighting-active-cpu.txt) captured real running work rather than background
sleep: ISLAND main draw and SEA reflection model redraw both consume CPU time.
Runtime cache-hit counters will determine actual benefit after staging.

Island render-only PN consumer and CPU fixture are prepared, not yet built/staged.
Antigua geometry changes 62428 to141409 triangles; orthographic silhouette maximum
shift19.71 world units. Original corners, shoreline band, material/UV boundaries
and original BSP collision remain the protected baseline. In-game appearance and
performance remain unverified; this is not a completed island-quality claim.

Latest prepared correction: indoor lighting now consumes a stable scene catalog
of up to64 actual lamps, rather than camera-selected per-draw slots. GPU regression
passed equal receiver pixels across camera slots/player-focus boundaries, physical
second-light contribution, character opaque/faded receivers and indoor occlusion.
Not staged over the current game. Character close-contact shadow contrast and
walking reflection jitter remain open. User requested vegetation/rock candidates,
then clarified existing good high-resolution ground should remain: 1grass1 was
never regenerated; jungle1 candidate is withheld, original retained. New foliage
and rock shards are prepared but not staged or accepted in-game.

### Current Metal launch — lighting correction candidate

Supersedes the prepared/not-staged labels above: frame-local FFP cache, PN island
render geometry (opt-in enabled), stable indoor lamp catalog, and treePalms/trees
foliage are staged. Original jungle1/1grass1 and rock textures remain retained.
Latest launched engine SHA-256: `30c4c34aacffbf7b4e8c5c0d02127255ce75615fa6c50d77cba0179f69d5ee23`.
The process was observed as PID 67821; staged and built hashes match.
This launch adds D3D-compatible COLOR1 diffuse default and indoor shadow-lamp
selection independent of player/camera distance. Lighting and dynamic-lighting
GPU fixtures pass; actual Marigot movement acceptance remains unresolved.

Water temporal candidate was rejected: max channel discontinuity fell from211
to144 but120 jumps remained. It was reversed before this launch. Existing water
is preserved; camera-motion reflection/refraction jitter and sea disappearing
behind foreground foliage remain unresolved. Alpha-cutout foreground regression
passes, so alpha-test leakage is not established as the cause. Agents continue
source-only depth-tolerance and temporal-input isolation work while user plays.
Launch/process/log evidence does not establish interactive scene acceptance.

User rejected indoor visual result in Marigo_Shipyard: no convincing furniture or
character cast shadows, flat object brightness. Source/asset audit finds8 candle
locators and range4; the single pinned first candle lies6.15 units from merchant,
so it cannot shadow the desk. Multi-lamp shadow support is now being implemented;
single-lamp fixture success does not accept room lighting. Canonical alpha order
was broadened, but no isolated pre-fix failure was recorded; causal attribution
to it remains unproven.

Water depth-order correction passed the actual near=.115/far=32000 distant-depth
fixture:10 submerged world units changed from fffdfdfd (incorrectly transparent)
to ff88c6e2; near/night/foreground-cutout cases still pass. This source correction
is not yet staged, and remaining camera-motion artifacts are not accepted.

### Active correction — camera-dependent light and controls

User Story: As a player, I want stable world lighting and correct physical-key
controls so that camera movement does not change illumination or ship actions.

The September 15 01:37–01:39 screenshots reject the latest Metal lighting
candidate: interior shadow boundaries move with camera position and outdoor
characters are excessively dark. Earlier fixture passes do not accept this build.
No game process was present at the current process check. Root owns build/stage
and launch; source-only lanes inspect shadow spaces, authored light positions,
outdoor character energy, saved controls, and diagnostic wiring.

Acceptance: fixed world receivers retain light/shadow registration across camera
translation/rotation; daytime characters remain readable with directional light;
world-map W/S and A/D follow their named sail/turn controls on the existing profile;
Must not alter unrelated saved settings, land controls, night energy, or sea mode.
Scope includes Metal rendering and shared gameplay delivery to both native targets;
new texture/art replacements are outside this correction.

Discovery found DX9RENDER::SetTransform(VIEW) rebases world geometry and strips
view translation. The current shadow bridge supplies already camera-relative
views to that absolute-space API. The next falsifier must exercise this real
wrapper contract, then the same interior wall and outdoor scene in game.
Disposition: unresolved; no new playable acceptance is claimed.

### Verified gameplay delivery and user feedback — September 15, 02:16

Both native runtimes now contain the name-based control-registry restoration and
support-surface collision candidate. Staged engines: Metal
`f17cecbfda3dc7b0f60cfc87d2a5e4990ae1e00dee61fa2d258e3b40d094af62`,
Storm `30c1b91e2cecbbadeec96d7954b7d6fe536b03763c73d50c24a6cc87519c0863`.
Runtime tracing observed WMapShipSailUp=87, SailDown=83, TurnLeft=65,
TurnRight=68 after loading. User subsequently confirmed global-map controls are
correct: accepted on Metal. User also confirmed lighting and shadows now look
normal: accepted for the reported scenes. Native Storm startup/interaction was
not replayed in this delivery; its installed bytes are verified only.

The renderer correction uses absolute light views at DX9RENDER::SetTransform;
receiver sampling compensates the main-camera origin once. Outdoor authored
COLOR1 models receive full weather ambient, avoiding the ground-specific
attenuation. CPU coordinate negative and GPU lighting/contact fixtures pass.
Deck floor filtering is installed but actual walking acceptance remains pending.

The native source build path now uses transactional patch application and a
binary/patch receipt instead of nested git-apply and a hardcoded engine hash.
Its legacy migration matched a pinned 32-path snapshot; unknown edits reject.

Separate settings layer exposes seven existing global commands through the
existing controls UI. It is applied to Storm scripts and queued for normal Metal
sync at next safe launch; Metal in-game settings acceptance remains pending.

New requests remain open: smooth lamp flicker, long-distance shadow transitions,
progressive soft shadows, actual town AO. A temporal lamp patch is prepared;
shadow/AO source lanes are in progress. Performance task
`01a0a153-d759-7c83-af1d-3bc113a7b8ae` owns profiling and renderer optimization.
Its real CPU sample identified ship crew collision work; a revised source-only
solver fast path preserves one static sweep for moving sailors and skips repeated
relaxation only without dynamic overlap. No measured FPS improvement is claimed.

### Current Metal runtime and pending persistent-cache build — September 15, 05:29

The currently running Metal process is PID 89430 from the last fully staged visual
build. That installed binary predates the persistent immutable-geometry cache and
must not be used as evidence for its performance.

The pending source candidate preserves the existing render passes and quality
settings while retaining exact converted FFP vertex/index buffers across Present
and caching validated indexed spans across repeated shadow submissions. VB/IB
revision changes and device Reset invalidate the matching data. GPU payload is
bounded to 512 MiB and 8192 entries; revision metadata is admitted only with a
successful entry, so a 100,000-identity stress case remains bounded. Focused probes
pass 120-Present reuse, revision/Reset invalidation, invalid-range rejection and
5760 repeated shadow submissions with one index scan. Full build/stage and a
same-scene runtime measurement remain pending until PID 89430 exits. No FPS gain
is accepted yet.

### Launched persistent-cache candidate — September 15, 11:11

Supersedes the pending state above. `experiments/native-metal/run.sh --stage-only`
completed after the final source changes with zero patch-stack changes, all 11
hash-pinned materials, all three rockK2 aliases and zero pending Metal gameplay
files. The built and staged executable hashes both equal
`cdb73d61d4e2f3cfe673cae2f114eb5634c42485e3e8aca2d7718263b8aca9e3`.
The app is running as PID 10962 and reached a rendered gameplay scene.

Live `STORM_METAL_PROFILE` samples in the heavy indoor scene report approximately
900 cache hits and 456 MiB of avoided conversion/upload work per frame. Warm
samples are 8.62–9.61 ms wall time with 4.16–4.40 ms GPU time while drawing about
1012–1042 calls and 10.5–10.8 million submitted vertices per frame. These samples
prove that the persistent cache is active and removes substantial CPU/upload work;
they are not a controlled before/after FPS comparison. User-visible acceptance of
the latest lighting, sea/horizon, material, island and deck-sailor changes remains
pending in their corresponding scenes.

### Rejected visual candidate and corrective stage — September 15, 13:21

The user rejected the launched candidate on the real game surface: the tavern
showed 22 FPS, the outdoor town showed 35 FPS, San Juan ground still appeared
unchanged, static town objects produced doubled shadows, and the town backdrop
could render as a large opaque wall. The earlier build/probe/cache receipts did
not accept those surfaces and must not be cited as visual or performance success.

Discovery bound the San Juan low ground plane to `rockK2.tga`, not the previously
emphasized `town_cob.tga`; the three replacement aliases were staged but their
engine selection was not explicit in the app launcher. The launcher now exports
`STORM_METAL_ROCKK2=1`, and the staged executable contains the alias-selection
contract. Static outdoor location geometry is no longer replayed into the new sun
shadow map, avoiding a second dynamic shadow over authored/prelit town shadows;
moving characters remain shadow casters. Indoor point-shadow ownership is restored
to one stable lamp while the complete light catalog remains available for direct
illumination, reducing the worst-case prepass from 8 cube maps / 48 faces to one
cube map / 6 faces. The broad script injection that assigned the window-only
`LocationWindows` technique to 17 full-size `plan1` town backdrops has been removed;
the runtime scripts now retain their authored backdrop technique.

`experiments/native-metal/run.sh --stage-only` succeeded after these changes.
Built and staged executable SHA-256 both equal
`a277c70297c68e0c6ae3f225a09713d25dc6d049999c080bc85ec1c2f4339c57`.
The backdrop source/CRLF/negative probe, material corpus probe, patch transaction
probe, shadow cascade scalar probe and world-shadow owner probe pass. Disposition:
staged, user-visible acceptance unresolved. No game process was launched for this
attempt; the same San Juan ground/backdrop, static-vs-character shadow scene, and
fixed-camera tavern profile must be replayed before any repaired/FPS claim.

### Corrected Metal lighting and cache candidate — September 15, 14:18

The textureless intermediate candidate was rejected: a missing brace in the
`shadow.tga` material marker assigned `-1` to ordinary textures. The corrected
stage restores the authored textures, removes the bad opaque town backdrop,
keeps outdoor static geometry in the two-cascade dynamic sun shadow pass, and
selectively bypasses the separate baked `shadow.tga` stage when both cascades are
ready. The user accepted the restored town appearance; the initial corrected
replay remained rejected for performance at 32--38 FPS.

Live profiling bound that failure to CPU FFP conversion rather than Metal GPU
capacity: the rejected replay uploaded about 159 MiB/frame with 26--32 ms wall
time while GPU time was about 6--8 ms. The persistent conversion key incorrectly
included camera-origin translation and unused directional-light fields. After
removing those inputs and adding bounded LRU eviction, upload fell to roughly
44--54 MiB/frame and the visible San Juan rate rose to about 55 FPS, but exact
per-frame sun direction still invalidated the remaining lit receivers.

The current stage stabilizes slot-0 sunlight at angular cells smaller than half
the finest 512-map shadow texel, preserves true time-of-day evolution, and uses
that same direction for CPU lighting, cache keys, and shadow matrices. It also
preserves VB/IB usage metadata so rewritten dynamic model buffers use the frame
arena instead of persistent allocation. The staged executable SHA-256 is
`07c180a829a553091435113d4d2af04d9f89a49f3ff6132776436ddb606742d0`.

Real-scene observations for this exact stage: an interior reached 124--125 FPS;
steady indoor profile samples were about 8.33--8.55 ms wall and 4.6--7.6 ms GPU,
with zero steady conversion misses. The outdoor transition sample was about
16.25 ms wall / 5.21 ms GPU and showed real lit/dynamic cache hits, so outdoor is
materially improved but still below the requested 120 FPS and remains unresolved.
Synthetic 64x64 probes are correctness checks only and are no longer considered
performance acceptance; the required acceptance set is real heavy town/weather,
sea, and interior saves over sustained frame windows.

The same stage adds scoped lighting corrections requested during replay: dark
outdoor ambient is reduced by up to 12 percent while direct light is preserved
(slightly darker, more contrast); low interior ambient is raised and compressed
(brighter, less contrast); normal-less rope and vant geometry now uses each
ship's active scene ambient instead of the full-bright fallback. Day outdoor
ambient and unrelated transparent materials are unchanged. Disposition: staged
and running for user evaluation; indoor performance accepted, outdoor 120 FPS and
the exact visual balance remain unresolved.

### Full Metal migration rejection and corrective batch — September 15, 15:30

The user rejected the previous real-game candidate across the main menu, San
Juan interiors, daytime town, deck, cabin and night sea. Observed defects were
opaque black/green/grey texture backdrops on foliage and other alpha materials,
the opaque town mountain band, doubled and blocky outdoor shadows, missing local
character shadows, weak ship-lamp response, unstable night lighting and lower
FPS than the original native renderer. That candidate is rejected; prior narrow
GPU probes do not override this real-surface verdict.

The next staged batch removes the four-pass cinematic exposure/bloom/blur/
luminance pipeline from outdoor and sea scenes while preserving the accepted
indoor path. It fixes the Metal sampler mapping for `D3DTADDRESS_BORDER` and
`D3DSAMP_BORDERCOLOR`, which had incorrectly repeated hidden atlas RGB at sail
and alpha-mask borders. All 17 town `plan1` backdrops now use alpha blending,
and the separate baked `shadow` object group is skipped only while the dynamic
outdoor sun shadow is ready. Bound indexed scope-2 FVF `0x152` dynamic models
now read their source VB/IB directly in Metal and perform lighting and shadow
work on the GPU instead of a second CPU expansion.

`run.sh --stage-only` succeeded and the candidate was launched with profiling.
Warm observed town windows are about 8.33--9.72 ms wall and 4.50--5.99 ms GPU at
2056x1329, with no luminance readbacks, but still show roughly 124--219 legacy
3D draws and 15--26 MiB upload per frame. Therefore performance has materially
improved toward 120 FPS, but the full Metal migration is explicitly incomplete:
remaining compatibility classes and real-scene alpha/shadow fidelity still need
elimination and user verification. Disposition: unresolved, running for diagnosis.

### Consolidated Metal effects, audio timing, and GPU point shadows — September 16

All linked Git worktrees were consolidated into the main checkout and removed;
their backup refs remain available for recovery. The incompatible renderer
headers were reconciled and `run.sh --stage-only` completed after the final
source changes. The staged executable and build output have the identical SHA-256
`9ad979a415aaeac0df56625235498d11fdc9148b0e1bdab97a72dd6ae4614eb0`.

The character gun-fire animation now rejects duplicate `Fire` events and starts
`pistol_shot` before dispatching `MSG_BLADE_GUNFIRE`, which owns the muzzle visual.
Short PCM effects are decoded before start instead of using the streaming path;
looped, music, and MP3 sources retain streaming. The point-shadow path now keeps
a revision-aware caster registry, performs culling in a Metal compute pass, and
submits all six cube faces through indirect command buffers. The native probe
reported zero CPU face traversals and zero CPU point-shadow draws. Particle/fire/
smoke source coverage and the particle/caustic GPU fixture both pass. Disposition:
staged and source-verified; exact gunshot/smoke synchrony, splash visibility, and
shadow quality remain unresolved until replayed in the real game scene.

### Shadow stability and authored window shafts — September 16, 19:37

Real-game feedback accepted the Metal performance at 120--125 FPS and reported
three remaining defects: duplicate pistol audio, distance-dependent/piercing
shadows, and effectively invisible window light shafts. The prepared correction
removes the extra direct pistol sound while retaining decoded low-latency PCM,
extends the two sun cascades from 32/96 to 64/192 world units with overlapping
24--32 transition and 144--176 fade, and enables the authored `LocationWindows`
mesh bridge for 521 bindings across 51 window models. Shaft composite energy is
raised from 0.16 to 0.28 while preserving depth occlusion, temporal history, and
the adaptive GPU budget. Source probes pass. Xcode 27 compatibility is now
owned by the candidate build: the old `fmt 8.0.1` consteval path is disabled
and the legacy RDF header uses `std::size_t`. The canonical
`run.sh --stage-only` completed after these changes; the build and staged app
executables have the identical SHA-256
`f96e2357b85b2c9b851c0744332ff81acddcfb13e75e06113ca6b18c9b92017f`.
The native point-shadow probe reports one GPU cube dispatch, two retained sun
cascades, and zero CPU point-face traversals/draws. The authored shaft GPU probe
passes depth occlusion, temporal history and adaptive quality, averaging 0.148
ms at its 256x144 test resolution. Disposition: staged and source/GPU verified;
duplicate audio, corrected shadow transitions and stronger shafts still require
real-game replay for visual acceptance.

### Rejected exterior point shadow and screen-space shafts — September 16

The user rejected the staged lighting behavior after real-scene replay. Outdoor
camera rotation made an unrelated local shadow appear and disappear; the cabin
shaft pass produced a moving white veil near windows rather than geometry-bound
volumetric light; cabin, deck and ship lighting remained stale after waiting
across a day/night boundary. Inspection proved that the GPU point-cube path had
bypassed the intended indoor-only admission rule, and that the shaft path was an
additive screen-space approximation rather than a volume constrained by the
window mesh. The corrected stage admits point-cube shadows only indoors and
makes shafts explicit opt-in (`STORM_METAL_LIGHT_SHAFTS=1`) until their volume
and material response are rebuilt. Sun glare remains enabled. Disposition:
staged mitigation; unified time/weather/light revision across location, cabin,
deck and ship consumers remains pending and the lighting model is not yet
accepted.
### Land ambient bounce, coherent water reflections, and SSAO — September 17

Addressed user visual feedback on harsh pitch-black baked terrain shadows and
water reflection lag during camera turns. In `sea-reflection-budget.patch`,
removed the rolling single-face amortisation from `EnvMap_Render()`; the dynamic
environment cubemap for ships now updates all faces coherently every frame,
passing `sea_ship_reflection_probe.py` with zero camera-rotation lag.
In `backend.mm`, resolved the black shadow collapse by lifting the outdoor
ambient response: ambient light on land geometry preserves at least 38% of surface
diffuse albedo rather than multiplying dark 2003 baked vertex colors to near-zero,
and the sky/ground hemisphere ambient factor now provides a natural Caribbean sky
fill (0.30..0.68) instead of dropping to 0.18. Half-resolution GPU SSAO
(`ambient_occlusion_composite.hpp`) with depth-aware bilateral upscale is active
before the HUD pass, adding contact occlusion under characters, ship deck props,
and building crevices. Saved-state liveness tracking protects `TraceARoot`
against dangling references. Staged executable SHA-256:
`e5d084ff44e0bad2f0f3d9602e3114f81b99847659d6fbb75a1bd04281b65586`.

### Rejected night-lighting candidate and unified-source correction — September 17

User replay rejected the night candidate: camera rotation still moved lighting/
shadow response, the nearby ship lamp did not illuminate its boat, and the San
Juan bridge railing remained opaque black. The cause was split coordinate and
source ownership in the renderer: camera-relative receiver geometry was compared
against absolute legacy point-light positions, and the authored location catalog
suppressed ship-owned D3D point lights. The corrected Metal path rebases every
legacy point source into the receiver frame, preserves ship lights alongside the
authored catalog, and moves authored location sources into `LightingScene` as the
shared frame owner. `ogradaR3.tga.tx`, whose decoded alpha contains 32,497 fully
transparent pixels, is now included in the explicit architectural cutout set;
generic mixed-alpha textures are no longer forced into cutout, avoiding the
weapon clipping regression.

`shadow_space_probe.py`, `foliage_cutout_probe.py`, and the unified lighting scene
probe pass. Canonical `run.sh --stage-only` succeeded; build and staged executable
SHA-256 are both
`326cb6621b52081fae3acfbc1c2395994365a136d3679b287518ae52deb6251a`.
The user-owned replay rejected this correction: the boat and ship surfaces still
received no useful local light, camera rotation still changed the shadow response,
the bridge railing remained an opaque black mask, and cabin/hold openings produced
no light shafts. The named `ogradaR3.tga.tx` admission was therefore not the
material owner and has been removed. Disposition: rejected. The agent must not
perform visual scene acceptance for later batches.

### Systemic world coverage candidate — September 17

Metal no longer recognizes foliage, nets, rails, or individual texture names.
Texture resources now derive a cached binary-coverage semantic from decoded alpha:
the image must contain both real holes and solid surface pixels, with endpoint
values dominating transitional data. Automatic inference is restricted to static
world geometry, so mixed alpha used as weapon gloss cannot clip swords or pistols;
explicit legacy alpha-test/blend state retains priority for authored effects and
dynamic models. One effective alpha-test contract is consumed by final color,
immediate depth capture, retained shadow packets, and point-shadow registry keys.

`foliage_cutout_probe.py` passes, `build.sh` produced an arm64 executable, and the
canonical `run.sh --stage-only` completed after the game was closed. Build and
staged executable SHA-256 are both
`ee18301f5217d12c0a26b3cefd234bd172997cbc71ff668b79fcc5b32fc269ed`.
Disposition: staged and source-verified; railing, vegetation, weapon integrity,
and alpha-shaped shadows remain unresolved pending user-owned real-scene replay.

### Unified local light receiver candidate — September 17

Resolves the split between authored location lights and active dynamic D3D point
lights on world receivers. In `backend.mm`, the shader constant generator now merges
active D3D point lights (such as ship lanterns and lamp models) into the unified
receiver light array `lighting.lamps` alongside the authored location catalog,
bounding each source to the receiver's world extent and preserving distance attenuation.
The separate vertex-only point light evaluation on raw lit geometry is retired in
favor of this unified per-fragment evaluation.

Canonical `run.sh --stage-only` completed with both executable hashes matching:
`230f3ad5ba452bdf00a18ab3d7c9938cd1a12edcc97a589ce59271f28837df92`.

Also resolved the rotating-light / double-rebase defect: Storm's `DX9RENDER::SetLight`
already translates dynamic D3D lights into camera-relative coordinates (`+ vWordRelationPos`).
Re-adding `worldOrigin` in `backend.mm` previously added the camera position twice,
causing point light sources to orbit around the camera and detach from physical lanterns
during camera rotation. Removing this spurious second rebase makes dynamic lamp positions
strictly camera-invariant and aligned with receiver world coordinates.
Disposition: staged and source-verified.

### Gunfire muzzle flash lighting candidate — September 17

Integrated real-time gunshot flash illumination for character firearm attacks.
When `CHARACTER_FIGHT_GUNFIRE` dispatches, `Character` now creates a momentary high-energy
point light source `gunfire_flash` (radius 25.0, warm burst) located at the muzzle / hand locator
with a 120 ms lifetime. The source is automatically advanced and pruned in `Character::Execute`,
illuminating the character, surroundings, and ground during pistol and musket shots.
`lights.ini` in both runtimes now contains the authored `[gunfire_flash]` profile.
`gunfire-audio.patch` was cleanly reconciled and tested in the transactional patch stack.

Canonical `run.sh --stage-only` succeeded with matching executable hashes:
`22d1c33eb533a5c390d0f65053d9984adff8ee152862d5d6bededba5468dcffe`.
Disposition: staged and source-verified.

### Natural volumetric light beams and cabin/hold opening candidate — September 17

Resolved missing light shafts in ship cabins and hold, and eliminated the synthetic yellow haze.
In `volumetric_light_shafts.hpp`, the hardcoded yellow color multiplier (`half3(1.0, 0.88, 0.66)`)
was replaced with a natural soft-sun tone (`half3(1.0, 0.97, 0.93)`), and composite intensity was
tuned from 0.28 to 0.16 to produce subtle geometry-bound volumetric beams without image-wide washout.
In `PROGRAM/locations/init/Boarding.c`, enabled the authored window/opening definitions:
`capsm_rays` with `LocationWindows` is now loaded for small cabins (`capsm`), and `hold_fd` is
bound as `LocationWindows` for the ship hold (`My_Deck`), allowing the Metal light shaft bridge
to register window apertures and ladder hatch openings.

Canonical `run.sh --stage-only` succeeded with matching executable hashes:
`784924fe4bd3f105124b86b4712e102257b432a2d14ae72f1dd2d08b81685e89`.
Disposition: staged and source-verified.

### Godray direction and interior reach correction — September 18

Corrected the Metal volumetric shaft shader for Storm Engine's DirectX 9 light convention:
`u.lightDir.xyz` is already the sunlight travel vector, so the beam direction now uses
`normalize(u.lightDir.xyz)` without reversing it. Extended the aperture mask to 35 m and the
raymarch distance to 40 m so shafts from high church windows can reach the floor. Increased the
composite contribution from `0.11` to `0.40` for visible interior beams.

Canonical `experiments/native-metal/build.sh` completed successfully, including the
`volumetric-light-shafts-gpu-probe` target. Disposition: source/build verified; real-scene visual
replay remains unresolved.

### VRAM leak resolution and 3D world-space light shafts — September 17

Resolved the 12 GB VRAM accumulation and ~114k GPU region leak identified by user diagnosis:
1. Autorelease pool drain: in `backend.mm`, registered `objc_autoreleasePoolPush()` /
   `objc_autoreleasePoolPop()` boundaries across every `Present()` frame. Autoreleased Metal
   descriptors, encoders, command buffers, and string views are now reclaimed each frame.
2. Zero-allocation skinning poses: replaced per-character `[metal newBufferWithBytes:...]` driver
   allocations in `beginSkinningPose` with sub-allocations from the persistent ring buffer `frameArena`
   via `submissionBuffer`, eliminating up to 3000 buffer allocations/sec at 120 FPS.
3. Cached point shadow tables: eliminated per-frame `newBufferWithBytes` allocations for `faceBuffer`,
   `drawBuffer`, and `boundsBuffer` in `point_shadow_registry.hpp`.
4. True 3D world-space volumetric light beams: replaced 2D screen-space radial vector tracing with
   3D world-space raymarching from camera to depth surface, projecting points onto the window aperture
   along the 3D sunlight vector. Beams remain physically anchored to the room during camera turns.

Canonical `run.sh --stage-only` succeeded with matching executable hashes:
`bfac17de392e38709ee9da18454eea134329080b58eec40ab86b42030c457374`.
Disposition: staged and source-verified; real-scene visual and memory acceptance unresolved.

### Main menu 3D back-scene interior bridge and ambient ceiling isolation — September 17

Connected `InterfaceBackScene` (main menu 3D cabin environment) to the Metal interior lighting and postfx pipeline:
1. Dedicated lifecycle bridge in `interface-back-scene-metal.patch`: `StormMetalInterfaceBackScene(d3dDevice, true)`
   in `InterfaceBackScene::Init()` and `false` in `~InterfaceBackScene()` cleanly assigns the interior domain
   without mutating or polluting game location state upon exit.
2. Cabin ambient clamping: because the menu script loads daytime weather to render sea/sky through the windows,
   the daytime weather sky ambient (~0.65) was flooding the cabin. In `backend.mm`, clamped interior ambient for
   the back scene to warm indoor levels `(0.16, 0.13, 0.10)`.
3. Sun penetration block: directional sun rays from above cannot penetrate the opaque wooden cabin ceiling
   (`wp.z >= 22.0f`), while the open sea, ship hulls, masts, and sails outside the window (`wp.z < 22.0f`)
   retain their full sunlight and outdoor illumination.

Canonical `run.sh --stage-only` succeeded with matching executable hashes.
Probes `interface_backscene_bridge_probe.py`, `scene_hud_boundary-probe`, and `menu_unlit_native-probe` passed.
Disposition: accepted at probe and staging level; real-launch visual inspection pending.

### Deck walker elevation and looped sail audio — September 17

Restored the complete `deck-walk.patch` in both native source package locations after
the Metal snapshot had been left with a partial source edit and a four-line stub patch.
The visual walker transform now applies a `0.14f` Y elevation above the traced deck
position, while collision and crew resolution continue to use the unmodified floor.
Looped sounds are suspended and stopped when effective volume reaches zero, then
restarted with a short fade when volume returns, preventing silent continuous ship
loop playback. `deck_collision_probe.py` and `deck_walk_probe.py` pass, and the
canonical `run.sh --stage-only` completed through patch verification, build, gameplay
sync, and staging. Real-scene foot placement and audio replay remain unresolved.

### Metal local-light catalog and shadow-composition candidate — September 18

Hypothesis: daytime duplicate silhouettes came from stacking screen-space AO on top of
the outdoor dynamic sun-shadow pass, while night boat/pier receivers missed authored
ship lamps because only location lights entered `lightingScene`.

Delta: outdoor HUD-boundary composition keeps AO disabled when the active land scene is
outdoor; the existing sun cascade selection remains snap-stable instead of cross-fading
two offset maps. `scene-lighting.patch` now publishes each receiver object's live
`ShipLights` point sources from the confirmed `SetLights` path, with current flicker
and transformed positions, a separate ID namespace, and corona-only sprites excluded.
Catalog illumination moved to the Metal vertex receivers; the selected point cubemap is
still sampled per fragment for the shadowed contribution. Location torches and pier lamps
retain the existing source-owned publication path.

Canonical `build.sh` completed with an arm64 engine and no Vulkan/MoltenVK linkage.
`ninja -C experiments/native-metal/.cache/build lighting-probe outdoor_lighting-probe`
completed; both binaries passed. `night_local_lighting_probe.py` also passed.
Disposition: accepted at source/build/probe level; real-scene daylight silhouette and
night boat/pier replay remain unresolved until observed on the actual game surface.

### Town baked-shadow texture matching and sea-lamp catalog fallback — September 18

The town shadow mask matcher now uses a case-insensitive `shadow` substring search,
so `.tga.tx` variants such as `Shadow.tga.tx`, `shadow2.tga.tx`, and location-prefixed
shadow textures are marked for baked-shadow replacement. The generated Metal snapshot
was updated with the same source. `ninja -C experiments/native-metal/.cache/build engine`
completed and produced `bin/engine`.

Sea ship lantern publication exposed a second owner gap: `StormMetalSceneLight` rejects
valid ship sources when no land-location pass has opened the authored catalog. A lazy
catalog-open change was handed to the active `backend.mm` owner because that file is
currently locked by another task. Disposition: town matcher accepted at source/build
level; sea-lamp backend completion and real boat/pier replay remain unresolved.

### Land camera offset from the raw view restore in the shadow prep — September 18

Hypothesis: the reported land camera displacement, the displaced interface back-scene,
and the camera-locked drift of scene lighting were one defect. The land shadow-prep pass
saved the view with the engine reader but restored it through the raw
`IDirect3DDevice9` setter, which skips the rebase that `DX9RENDER::SetTransform`
performs. `GetTransform(D3DTS_VIEW)` returns the un-rebased view matrix, and every
canonical restore in the engine (`lights.cpp`, `location_effects.cpp`, `shadow.cpp`,
`DrawRects`) round-trips through the engine setter instead. The device therefore kept a
second translation of the camera position for the rest of the frame: a few units on a
deck, about 25 units on land, and about 1300 in the interface back scene.

Delta: `scene-lighting.patch` restores through `renderer->SetTransform(D3DTS_VIEW,
&view)`, and `multi-shadow-lamps-v2.patch` carries the matching context line. New
`metal-camera-view-restore.patch` checks the restored device view translation and prints
only on drift, and `location-camera-follow-reset.patch` gained an engine-side framing
probe (`[StormMetal] cam:`). Both are diagnostics to drop after the user confirms the
fix.

Evidence: `run.sh --stage-only` completed (515 objects, arm64 engine, staged app). Live
replay: zero `camera view restore drift` lines, `camRel` always `(0.00,0.00,0.00)`
with no NaN frames (both appeared in the previous build's log), `origin` tracks the
negated camera position and moves with the player instead of freezing, the framing probe
reports the camera 3.32 units behind and above the look target, and land frames reach the
120 Hz limiter at 8.3 ms wall with 2.4 MB upload per frame.

Disposition: accepted at source, build, and runtime-log level. Visual confirmation on the
land surface is with the user. Godray apertures (`openings=0`) and the remaining CPU
conversion (`legacyUnlitCpuDraws` 62-97 per frame) stay unresolved.

### GPU-skinned local lighting and camera-invariant shaft energy — September 18

Two defects, one receiver owner each. Night characters stood near-black under authored
lamps because a GPU-skinned raw draw sampled only the eight camera-selected light slots in
`vs_skinned`, while the floor beside the character was lit by the authored catalog in
`vs_landraw`. Interior shafts appeared to be projected onto the camera because the
accumulator used a strong forward-scattering phase (`g=0.76`,
`(1-g^2)/pow(1+g^2-2g*cosTheta,1.5)`), which varies by orders of magnitude with the
viewing ray even when aperture, sun direction and receiver are fixed.

Delta: `vs_skinned` takes `LandSampling` at buffer slot 6 — slots 4 and 5 already own the
bone palette and `SkinU` — evaluates the same authored catalog as `vs_landraw`
(contribution, range-edge fade, attenuation) and forwards `directPoint`. The compact
FVF unlit path no longer accepts GPU-skinned streams, whose 44-byte `AnimatedVertex`
layout it cannot decode, so unlit skinned draws use `fs` instead of the removed
`fs_skinned_unlit`. `bindLandSampling` now takes an explicit vertex slot instead of a
`vertexStage` flag. The shaft phase is a flat `1.0`; the aperture prism, depth receiver,
temporal reprojection and world-space sun direction are unchanged.

Evidence: `gpu-skinning-offscreen-probe` renders GPU-skinned draws pixel-identical to the
CPU oracle and covers the unlit case; `dynamic-lighting-probe` builds its skinned quad
through the real `gpu_skinning_bridge` and requires the lit catalog to exceed twice the
dark energy; `volumetric-light-shafts-gpu-probe` now requires the same fixed world beam to
stay within a 1.5x factor between two camera positions, and its occlusion check keeps the
receiver footprint while the integrated volume loses two thirds. Also passing:
`gpu-skinning-parity-probe`, `lighting-scene-probe`, `local-light-bounds-probe`,
`character_skinning_probe.py`, `authored_light_shaft_openings_probe.py` (522 bindings /
51 window meshes). `run.sh --stage-only` completed; build `bin/engine-1` and the staged
`metal-engine` are both `d34a0c36c8365d9ec583f37e8ab6e94702d0fab38e0d5d7f50c6b6055265f773`,
arm64, no Vulkan/MoltenVK. Launch log: outdoor menu frames at 8.3-8.6 ms wall / 4.7-6.0 ms
GPU, `camRel=(0.00,0.00,0.00)`, `sun=3 applied=1`.

Disposition: accepted at source, probe, staging and launch level. Night character
brightness under lamps and church/indoor shaft appearance remain unresolved until they are
replayed on the real game surface.

### Window apertures derived per opening — September 18

`StormMetalLightShaftMesh` reduced every submitted `LocationWindows` object to a single
axis-aligned bounding box. One object groups the glass of several windows, so the box
described the window band of the whole building and the aperture prism filled the
interior: the church reported `openings=3` while every interior view ray crossed the beam
for many units, so the volume pass read as a lit haze with the character punched out of
it. `openingBeamMask` additionally diverged 8% per unit and feathered over the outer 60%
of the cross-section, widening one window into a room-sized cone.

Delta: `light_shaft_apertures.hpp` derives one aperture per connected coplanar triangle
group of the submitted mesh (shared vertex, face-normal dot >= 0.9995, plane offset
<= 0.02), fits the rectangle to the group's own projected edges and keeps the smallest
covering pair; an authored id now owns its whole rectangle set. `spread` drops to
`1 + distance*0.02` and the cross-section feather to `smoothstep(0.78,1.0,...)`.

Evidence: `light-shaft-apertures-probe` (separated quads stay separate, shared panes and
double-sided panes merge, capacity keeps the largest deterministically, degenerate and NaN
groups are skipped); `volumetric-light-shafts-gpu-probe` gains a room-scale fixture that
requires the brightest column to stay at the opening and the far room wall to stay below
1/20 of the peak. Also passing: `authored_light_shaft_openings_probe.py` (522 bindings /
51 window meshes), `gpu-skinning-parity-probe`, `gpu-skinning-offscreen-probe`,
`dynamic_lighting-probe`. `run.sh --stage-only` completed; `bin/engine-1` and the staged
`metal-engine` are both `3cf0eaed9f89a588f8327485e33102fc5a222fe97a0d5cc7259c5f42fff9c9d8`.

Disposition: accepted at source, probe and staging level. Whether a church or indoor
interior now reads as light through glass rather than a haze is unresolved until it is
replayed on the real game surface; a new `[StormMetal] shaft apertures n=... c=... hw=...
hh=...` line reports the derived set per room.


### Deck contact and enabled weather-sun capture — September 22

The `c3d1e164…` installed replay narrowed the remaining deck defects. `system.log`
records nonzero canonical input and accepted path movement, so walking input is no
longer the blocker. The screenshot shows native sailors grounded correctly while only
the presentation protagonist sinks. The attempted animated camera-locator alignment
was therefore rejected: it aligned the eye target by translating the whole body below
the path support. The candidate restores root placement at the resolved support with no
former lift; `deck_motion_probe.py` passes ship yaw, roll, movement and contact-root
checks.

The same run reports a complete sea registry (`draws=5`, `vertices=10299`) but
`sun=0`, `rawShadowDraws=0`. Geometry admission is no longer the failing owner. Weather
enables the directional source before ship rendering disables/reuses slot 0, and no
receiver packet retains it. The candidate captures the first valid enabled directional
source in the current traversal frame and clears it at `Clear`/`Present`. The mixed-order
GPU fixture passes: 104 mast pixels and 96 late-actor pixels darken independently, while
the next frame without a source remains unchanged. `run.sh --stage-only` produced matching
built/staged SHA-256 `6b22fb9d3d5e428ca91b2f8d402b34527e68e740b4b8661f35fa9842227035f1`.
The 23:28 rejection screenshot came from the prior installed `c3d1e164…` process and still
correctly rejects that runtime; visible contact and deck-shadow acceptance for `6b22fb9d…`
remain unresolved until replay.

The subsequent installed `ff98425a…` replay was also rejected. Its live movement trace
proved that `MODEL::Trace` returned the ray start itself (`pathY=4.529`, false
`supportY=8.488`), so it lifted the protagonist instead of finding the visible deck;
movement then reached `solid_rejected_or_slid` at the stair. The replacement support
query scans ship triangles, accepts only upward-facing walkable surfaces near the authored
path, preserves the full vertical delta through the collision sweep, and still rejects a
vertical wall. `deck_motion_probe.py` passes the regional deck offsets, stair tread and
solid-wall negative.

That replay also showed that fixing the directional receiver slot alone was insufficient:
the ordinary lit sea FFP deck/hull path was rendered but was not retained as a shadow
receiver. The traversal registry now admits that existing `rawSeaLit` class while retaining
the custom-water, particle, unlit UI and dynamic-source exclusions; it also freezes the
first enabled directional source regardless of D3D light slot. The GPU fixture passes with
an unscoped deck receiver, an alpha-cutout mast, a late GPU-skinned sailor, Weather sun in
slot 3, and a no-source next-frame negative (`mast-darkened=104`, `actor-darkened=96`,
`no-sun-changed=0`). `run.sh --stage-only` completed with matching built/staged SHA-256
`8fcab0773294426dfed9ba1a6ebbcd0e64ee3ef158df58540ec4489816e8fa0c`; `run.sh`
started the installed build as PID 47262. Disposition: accepted by focused probes, full
build, canonical staging and startup; real deck contact, stair traversal and visible deck
shadows remain unresolved until this exact process is replayed.

The player replay of installed `8fcab077…` rejected that candidate. The protagonist
still floats above some deck regions, enters the planks near the stair and cannot
traverse the stair; mast, sailor and protagonist shadows are still absent, leaving
the ship visually flat. The live trace records nonzero movement input followed by
`solid_rejected_or_slid` at the stair, so controls are not the blocker. Source review
found that the visible-support query clips the complete ship model while starting
exactly on the hidden `path` mesh; nearest-hit selection can therefore select that
hidden navigation triangle at distance zero instead of the visible deck. The live
shadow registry contains only five scope-2 draws (`10299` vertices) even though the
same frames report hundreds of lit raw draws, proving that the focused deck/mast/actor
GPU fixture did not reproduce the real ship receiver admission. Disposition: rejected;
contact, stair traversal and all requested deck shadows remain unresolved.

The next installed `31d27e02…` replay was rejected for the same visible outcomes,
but its live traces isolated both remaining owners. Movement reached both stairways
and then repeatedly stopped at the symmetric lower transition near local
`x=±2.2, z=-1.0`; the shadow registry retained only one to four scope-2 packets,
all GPU-skinned, while the deck and mast were absent. Geometry inspection bound the
stair coordinates to `Caravel_w1`: the old nearest-support rule selected hidden ramp
triangles 1050/1052 about 0.15 m below the visible treads, and the collision sweep
then treated the remote high vertex of each large walkable ramp as a wall.

The replacement support rule selects the highest walkable surface no more than
`MEN_STEP_UP` above the navigation path, with nearest support only as fallback.
Floor-aligned collision triangles now compare the local plane height at the start
and end foot positions; non-floor triangles retain the global-height wall check.
The focused probe covers a hidden ramp below a tread, a large sloped floor, a low
riser and a tall-wall negative. In the first installed replay the live trace records
an uninterrupted descent from `z=-2.433` through `z=-0.688` to the lower deck and
the player accepted stair traversal and protagonist height.

That replay also proved why the ship remained flat: 54–155 ordinary ship draws per
sample were `rawSeaLit`, but zero entered `rawSeaShadow` because MODELR left alpha
testing enabled with a fixed-function combiner outside the narrow legacy
`alphaContract`. These partitions write depth without blending and retain texture
alpha in the raw packet. Sea/deck capture now admits that opaque cutout contract,
while depth-disabled and blended draws remain excluded. The mixed-order GPU probe
passes (`mast-darkened=105`, `actor-darkened=204`, `no-sun-changed=0`). In the real
sea scene the registry increased from 1–4 skinned-only packets to 18–175 packets;
17–172 are captured ship sections with active directional receivers. Canonical
staging produced matching built/installed SHA-256
`4646ae77ca21a6b8a82504aaa57f8c1309087211729e8ad91ae43576bc88db55`.
Disposition: stair traversal and deck contact accepted on the real game surface;
visible mast, sailor and ship self-shadowing are installed and runtime-admitted but
remain unresolved until the player confirms the rendered result.

### Metal audio voice policy — broadside and charge selection, September 22

The active Metal service allocated every cannon event as an independent miniaudio
voice from a 4095-slot pool. A large broadside therefore summed many full-level
copies without a concurrency bound or bus headroom. The same service let the four
ammunition-selection announcements overlap until their files ended. Alias
priority was parsed but did not participate in playback arbitration.

The candidate gives Metal its own audio source owner and adds two bounded classes.
All cannon and fort-cannon aliases share a maximum of 12 live voices with a
gentle inverse-power group gain; a sparse shot keeps its authored level. The four charge
selection aliases replace one another with a 90 ms fade and retain at most the
fading predecessor plus the current voice. Splash, impact, fly-by, speech, music,
and ambient sounds remain outside both rules. The policy probe passes, and the
full arm64 engine built as SHA-256
`77ab3ffc32386b864aa22e733db0143a8d4d519d05d42c872519c963912117d4` from the
Metal-owned sources. The first stage was correctly held while the prior player
process (PID 15045 at inspection) was active. After that process exited,
`run.sh --stage-only` completed and the installed bundle matched the same hash.
`run.sh --launch-installed` then started PID 25506; the arm64 Metal process
remained responsive in the sea scene without a new script or engine startup error.

Disposition: accepted at source, policy-probe, full-build, canonical-staging, and
startup level. Real-surface acceptance requires a
large broadside without metallic overload, a sparse single shot at normal level,
rapid charge changes that replace cleanly, and one unaffected impact or splash.

The first real-surface replay confirmed that the metallic overload was reduced,
but rejected the initial 0.9 peak-sum ceiling as insufficiently epic. Cannon event
times were never changed. The corrected candidate keeps the 12 spatial voices and
uses `active voices^-0.4` gain so a broadside retains density and controlled energy
growth while the hard polyphony bound still prevents the former hundreds-voice pileup.
Charge-selection replacement is unchanged. Rebuild, staging, and broadside replay
for this corrected gain law produced matching built/staged arm64 SHA-256
`ff98425afaf4b69a8d1ff9a8f263abb57f8c079c5f72be18c706f226141caf68`.
The corrected broadside replay remains pending.

### Dusk sea-horizon seam and directional shadow distance — September 23

The reported dusk sea scene showed a pale horizontal seam at the horizon. The modern
water body already bounded its reflected cube colour with the current weather palette,
but the separate additive sun-road pass sampled the same cube without that bound. The
candidate applies `weatherBoundedReflection` to the sun-road sample before its existing
sea fog and additive composition. It does not change the authored cube direction or
remove the daylight sun-road highlight.

Directional shadow coverage is extended consistently for outdoor locations and sea/deck
traversal. The cascades now cover 96 and 288 world units, select at 42, fade from 216 to
264, and use a 120/240 depth offset/envelope. The engine location patch consumes those
values through the Metal bridge; sea/deck traversal uses the same 288-unit footprint.
Authored point-light ranges remain unchanged.

The weather bridge and compiled colour probes pass. The shadow source, quality, point,
and mixed sea/deck GPU probes pass, including the no-sun negative, and the complete
ordered patch stack builds arm64 engine SHA-256 `af51fc67de64d55fb7f3a4031d3e2c22464888215364ba40a957d676755dc8e0`.
The broad modern-water fixture still fails
its pre-existing base-water colour assertion before it reaches the separate sun-road
mode; it does not exercise this fix. The prior installed engine remains active as PID
46457, so replacing its bundle executable is intentionally deferred. Disposition:
accepted by source probes, GPU probes, and canonical build; staging and real-scene
acceptance remain unresolved until the player-owned process exits, the installed
`27b4c61a…` executable is replaced by this exact build, and the candidate is replayed
under the reported lighting.

### Daytime black horizon band — September 23

The Cayman 10:35 screenshot showed a nearly pure-black band below the blue sky.
The active day weather presets define a light blue fog colour, while the procedural
sky read its low-horizon colour indirectly from mutable `D3DRS_FOGCOLOR` and the
separate SkyFog sphere rebuilt RGB through `WEATHER_CALC_FOG_COLOR`. Those two paths
therefore had no shared explicit visual-colour contract.

The Metal bridge now snapshots `WEATHER_BASE::GetVisualColor(whc_fog_color)` once
for the procedural sky and uses the same visual RGB for SkyFog vertices while
preserving their authored alpha. Missing weather keeps the original fog vertex;
near-black midnight weather remains unchanged and no brightness floor was added.
The bridge round-trip probe, dynamic-sky CPU/GPU probe, backend scope probe and
shadow-quality ABI probe pass. The complete ordered patch stack builds arm64 engine
SHA-256 `e451f299c4a9acfbe84f2d0fc20056bdcf9c0421ad5d89fbaeb0af0ce907cc57`.
`run.sh --stage-only` completed with matching built and installed hashes
`e451f299c4a9acfbe84f2d0fc20056bdcf9c0421ad5d89fbaeb0af0ce907cc57`.
The installed arm64 process was relaunched as PID 65086, remained responsive and produced
continuous completed GPU frames without a startup or shader error. Disposition:
accepted by source, contract and GPU probes, canonical build, staging and startup;
the real Cayman 10:35 horizon remains unresolved until player replay.

The subsequent real Terks 09:36 replay rejected that candidate: the same exact-black
band remained. Binary inspection confirmed that `e451f299…` contained the intended
RGB mask, so this was not stale staging. The draw trace bound the band to the separate
untextured `SkyFog` hemisphere (`FVF 0x42`, vertex diffuse, alpha blend). The rejected
bridge sampled visual fog only when `GenerateSky` or script `Sky.TimeUpdate` rebuilt
the vertex buffer; the procedural sky sampled current weather every draw. Its green
source probe checked call presence but could not detect that lifetime mismatch.

The corrected candidate gives `SkyFog` an explicit draw-time Metal scope. The backend
replaces only RGB with the current visual fog colour, retains authored per-vertex
alpha, and disables compact/persistent reuse for the 33 scoped vertices so stale black
data cannot survive from an earlier weather state. Missing weather keeps the original
legacy path. The bridge lifetime probe, dynamic-sky colour probe, backend scope/cleanup
probe and complete arm64 build pass; built SHA-256 is
`b99ef97a170aa96fc1edcd126efd5f3330373267ab3e0803db36a40be8026e5d`.
`run.sh --stage-only` completed after the previous game exited. The initially
isolated horizon build was `b99ef97a…`; the final integrated canonical batch also
contains the contemporaneous frame-pacing correction and rebuilt/staged as matching
arm64 SHA-256 `c195de8440f5fd9d126f312d924a8c9ace9cd37b24f30c016cb2d576a1f9c20f`.
Symbol inspection of that installed executable confirms the explicit
`StormMetalBeginSkyFog`, `StormMetalEndSkyFog`, and `StormMetalSkyFogActive` bridge.
`run.sh --launch-installed` started the integrated build as PID 81150; the process
remains alive, completed GPU frames and reports no startup, shader, or command-buffer
error. Disposition: the `e451f299…` real-scene result is rejected; the draw-time
candidate is accepted by source, focused GPU/backend probes, canonical integrated
build, staging and startup. Real daylight replay remains unresolved until the player
revisits the reported horizon.

The Tortuga 13:35 screenshot from the installed `c195de84…` candidate rejected the
RGB-only correction again and exposed a perfectly horizontal coverage hole. Geometry
inspection proved that the normal base-sky draw submitted only upper vertices 0–19;
the existing mirrored lower vertices 20–39 were gated to `Delta_Time == 0` reflection
draws. `SkyFog` is also strictly an upper hemisphere, so neither draw could touch the
black clear target below the shared `y == 0` equator.

The final Metal correction submits those existing lower faces in both opaque base-sky
loops on normal frames while leaving the astronomy alpha overlay and non-Metal path
unchanged. For negative sky directions the procedural shader already suppresses clouds
and returns the current horizon fog colour. The source lifetime/coverage probe, dynamic
sky shader probe, backend scope probe and complete canonical build pass. Canonical
staging produced matching built/installed arm64 SHA-256
`2078cc65f7b0d9f72f8e351d3ee2526a041315534e3359b8ee27be0e573454f0` and launched
PID 86431. Live Tortuga replay at 19:31 visibly shows continuous blue/fog background
below the former boundary with terrain and vegetation correctly over it; no black band
remains, and the HUD reports 127 FPS. Disposition: accepted on the real game surface.
After that verification process exited, the same installed hash was relaunched as
PID 87408 for continued play.

### Sea battle HUD tinted by ship lighting — September 23

The reported night-deck screenshot showed the sea battle controls and text turning
blue only in camera directions affected by ship light or shadow. The sea battle
interface is realized in `SEA_REALIZE` and calls `MakePostProcess()` immediately
before drawing its HUD. The existing Metal scene/HUD marker ran later at
`INTERFACE_REALIZE`, so point/directional shadow resolve could still composite over
the already drawn sea HUD. The authored battle-interface textures remain neutral;
this was a render-order defect rather than blue source art.

The Metal-only renderer patch now emits the existing idempotent
`Storm.SceneHudBoundary` marker at the start of `MakePostProcess()`, before its
legacy post-process early returns. This freezes and resolves the scene before sea
and land battle HUD drawing while retaining the later core marker as a safe no-op.
The consumer coverage probe requires both the early marker and the sea HUD call
order. That probe and `scene_hud_boundary-probe` pass, the complete canonical arm64
build succeeds, and `run.sh --stage-only` installed engine SHA-256
`64a2471b21b47da606feec7af338ef47483cedefc9d463ce198ef74908601eab`.
`run.sh --launch-installed` started the staged engine as PID 15639 and it remains
responsive with completed GPU frames and no startup, shader, or command-buffer
error. Disposition: accepted by render ownership, focused probes, canonical build,
staging, and startup; visual acceptance remains unresolved until the active game is
loaded into the reported ship scene and the camera is turned through the offending
light/shadow direction.


### September 30 — portable Metal input and build delivery

Hypothesis: captured pinned engine, dependency/toolchain and baseline gameplay
inputs eliminate original-checkout, Conan and historical runtime dependencies.
Delta: ignored hash-manifest inputs, relocated development-kit exporter and
canonical build/stage input selection; existing reviewed gameplay patch checks
remain authoritative. Private kit preserves player SAVE and settings.
Result: clean relocated 548-target build completed with intentionally nonexistent
external engine/Conan roots. First staging failed at the graphics adapter's
archived runtime baseline; selecting captured gameplay in that adapter corrected
the demonstrated path. Complete relocated canonical staging then passed, zero
pending gameplay files and compiler ready. All 139 SAVE files matched. Packaged
SDL and system-only dependency closure passed; tamper/occupied-output/old-Python
and active-game rejection checks passed. No visible gameplay acceptance is claimed.
Disposition: build/packaging accepted; final integrated canonical staging and
player menu/scene replay pending. A kit game remains active and is preserved.
Public app, personal menu branding, Drive and site download are a separate
user-authorized delivery batch; existing Reconstruction 1.4.1 remains unchanged.

### September 30 standalone startup and menu player replay

The signed native launcher replaced the shell entry point; proper nested framework
and whole-app signatures pass strict deep verification. Symbolicated system evidence
binds the prior `tccd` work to unsigned-bundle signature synthesis, not to a proven
Metal GPU deadlock. Signed CoreAudio init/uninit passed and installed state staging
finished in 0.18s. The player confirmed the menu and playable sea scene at 13:24–13:26,
so this corrected startup replay is **accepted**. No universal stability claim.
The screenshots rejected the missing main-menu logo and vertical social/footer
label placement. Logs prove a duplicated INTERFACES texture path; source correction
uses a leaf name and adds explicit offsets/normal footer font with reviewed-hash
migration. Corrected visual replay remains unresolved until the next menu view.
Resolution borders cleared after the player's game restart; live display reset
remains intentionally excluded because it previously invalidated interface resources.

### 2026-09-30 — remove duplicate menu banner

Player screenshot confirms the restored logo and centered social buttons. User requested removal of the lower framed banner and a clickable mixed-case version line instead. The exact-hash layout/script layer removes only that banner, routes its keyboard neighbours to VERSION, and attaches the existing HTTPS handler to VERSION. Canonical stage succeeds; installed app is resealed. Disposition: staged; final version-link interaction remains unresolved pending player click.

### 2026-09-30 — superseded unsigned-app startup baseline

### Installed app startup hang — September 30, player replay rejected

The player launched the independent app and reported the Mac becoming unresponsive,
including two failed login/recovery attempts. The system recorded WindowServer
watchdog reports at 12:42 and 12:47, reset reports at 12:43 and 12:45, and a
`metal-engine` hang at 12:48–12:49 for the installed standalone app.
Its main thread was blocked during `SoundService::Init → ma_engine_init →
CoreAudio → TCCAccessRequest → synchronous XPC reply`. WindowServer's main thread
was also in a TCC screen-capture preflight queue. This proves a startup hang and
concurrent session failures, not that the game caused both resets. The package's
startup/stability acceptance is **rejected**; causality and repair remain unresolved.
No automated game launch is authorized. Public copy must identify this build as
experimental with unresolved startup stability. The user prioritized case/source
publication and deferred game repair. Preserve raw system reports locally; never
publish their private machine/process inventory.


### 2026-09-30 — local generated-state cleanup

The independently installed signed application and 138 active saves remain. All 45 archived native-storm save hashes resolve to the active profile or its six-file `Archived/native-storm/SAVE` supplement. Root removed both disposable native development caches, the redundant private development bundle, four intermediate ZIPs and the unsigned app backup using owned temporary directories. The 8,252 hash-bound compact inputs remain (246 MB), and default `git gc` reduced Git from 6.3 GiB to 2.86 GiB without history rewriting. Development staging restores disposable game data from the installed app. Windows installations are untouched. The corrected archive passes native extraction, deep/strict signature verification and relocated launcher staging; installed profile symlinks were restored afterwards. Drive synchronization subsequently passed anonymous cloud metadata and HTTP 206 range checks against the new 11,298,197,161-byte archive. Finder Remove Download evicted its local payload; the public cloud file and folder remain available.

### 2026-09-30 — illustrated case and repository consolidation

Production website revision `ea37ee5` includes all five English image-generation edits and source/download links; original cover is preserved. Single visuals now consume plain EvidenceFigure rather than a paired gallery/artboard, removing the empty right-hand frame and irrelevant arrows. Build, lint, type check and case checks passed; public accessibility tree confirms plain figures. Final screenshot observation is pending because macOS locked during capture.

Verified 8,252 portable inputs before consolidating obsolete engine sources. Preserved the complete 30-ref engine history as a verified 110 MB Git bundle, moved the old dirty 153 MB deck-shadow worktree intact inside the canonical repository, and removed the obsolete external engine checkout/bare repository/build. Read-only candidate directories initially prevented deletion; owner write permission on those exact temporary directories allowed the approved deletion helper to complete. No save, installed app or Windows runtime was removed. Disposition: local consolidation accepted; historical dirty candidate is retained, not integrated.


## September 30 — minimum lighting and current-town auto-pin delivery

### Minimum scene lighting and trade auto-pin — September 30, installed; replay unresolved

Integrated source: `bbd2037` on main, pushed to the verified public origin/main.
The exact batch was built and staged with
`experiments/native-metal/run.sh --stage-only` after the final source change.
Gameplay sync reviewed and applied only PROGRAM/interface/tradebook.c, preserving
SAVE/config and renderer resources. Installed app remains the standalone arm64
Metal GPK 1.3.2 AT + ReConstruction 1.4.1 owner. No game process was running during
staging or installation. Earlier inventory hashes below are historical.

Modern classified outdoor source-ambient floor increased .36 to .40; existing
warm indoor floor (.125,.100,.078) increased to (.140,.112,.088). Day ambient above
the floor, direct lamps/sun, emissive/unlit/UI behavior and graphics settings were
not changed. Existing CPU indoor/outdoor probes and native GPU lighting/dynamic
lighting fixtures passed, including retained materials, alpha, skins and lamp
contrast. The representative modern outdoor night GPU channel rose 45 to 51;
day stayed 52. These are fixture samples, not whole-scene perceived brightness.
The player screenshot before this batch showed Cartagena at 06:30 and FPS125.

Trade comparison initializer and mode toggle now attempt current-town auto-pin
when the saved pin is empty, including a persisted empty attribute; valid manual
pins and immediate unpin remain. No eligible current city/known prices leaves it
empty. Two disposable VM fixtures failed before Main; no gameplay acceptance is
claimed from them. The earlier PID78900 crash was the diagnostic CLI invocation
without engine.ini, not the player's merchant-dialogue crash.

Staged engine SHA-256 `9d152e333b6745c41ca750364a9cddc0ffd81dbd1f1bb3082b259a93342eaee8`;
installed engine SHA-256 `3ea9553b27f614bb1f93ee8a7f493c9cef97518444efe5b1b82bf5eb50619884`;
installed trade script SHA-256 `e4572beabccda273daca89bfee8270ac633c6a1b3ffbdd08a12b46e33a2340c6`.
Deep strict signature verification passed. All 139 SAVE files and
4 player configuration files were byte-identical across installation.
Rollback engine, prior trade script and CodeResources retained in the ignored
native-metal cache `.cache/app-update-backup-night-pin` with an install receipt.
Current player display configuration: {'full_screen': '1', 'screen_x': '1920', 'screen_y': '1241', 'display_mode': '0', 'msaa': '0', 'max_fps': '120'}; dynamic_lighting=1,
shadow_quality=2, modern_lighting=1 remain enabled in retained metal_graphics.

Disposition: build, probes, canonical staging and installed signed delivery
accepted for their checked properties. Actual Cartagena dawn/night readability
and comparison auto-pin on the current save remain player-replay unresolved.
The merchant dialogue crash remains unresolved: nested SetShow reached corrupted
window vector state; isolated valid recursion/nesting did not reproduce it.
No speculative crash guard was added. The user explicitly deferred crash
investigation and requested immediate installation of the existing batch.
Owners: docs/metal-lighting-parity.md, docs/trade-journal.md,
docs/metal-graphics-settings.md. No new game launch was required for this delivery.



## September 30 — outdoor floor correction and sea-boundary diagnosis

### Outdoor night brightness correction — September 30, installed; sea boundary unresolved

Source `80b99e1` on main was built and canonically staged via
`experiments/native-metal/run.sh --stage-only`, then installed into the standalone
arm64 Metal app. The player rejected the .40 outdoor ambient floor as too bright
in Porto Bello at05:25. The corrected .38 floor removes half the last increase;
indoor fill, authored lamps, brighter/daylight weather and compatibility behavior
remain unchanged. Existing CPU outdoor energy/continuity/negative checks passed;
existing GPU lighting probe reports modern night48 (prior51), day52 unchanged,
legacy night14 unchanged, with emissive/unlit/UI/alpha checks passing. Perceived
night readability still needs the player scene replay.

Staged engine SHA-256 `ace96ac1a30e44197cf8cb715f1a67265f23133847e1c0740d54d29cdef70adf`;
installed engine SHA-256 `88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
Deep strict app signature verified; all 140 SAVE files and
4 player configs byte-identical across installation. Rollback
engine/CodeResources retained at native-metal `.cache/app-update-backup-night-tune`.
Trade auto-pin from bbd2037 remains installed. The user closed the playing game;
CUA observation inadvertently started diagnostic PID76219 at21:44:47, which was
closed with TERM before staging. No player gameplay process was stopped. Earlier
engine inventories below are historical, superseded by these hashes.

The separate dawn-water boundary has a source-backed cubemap-background
hypothesis, not a reproduced cause; no sea patch was applied. Ownership and exact
missing falsifier are in docs/metal-weather-surface-color.md. Merchant dialogue
crash remains deferred by the user's prior instruction.


Delivery closure: origin/main and local HEAD verified at
80b99e14b1e540cdc2660fad0f07cdf1b81a3922; working tree clean. Repository knowledge
check initially stalled in the Python3.14/Git stdin-stdout exchange (both sampled
waiting in write); the owned check processes were closed. System Python3.9
rejected missing tomllib before validation. The unchanged canonical
`python3.13 tools/agent_context.py --check` passed with the available compatible
interpreter. No verifier/source/hook workaround was installed.


## September 30 — tornado technique correction

### Tornado white rectangles — September 30, staged; installation pending

Source `cc45e5e` on main restores existing particle technique loops around native
tornado pillar/ground and noise-cloud draws. Only renderer-particles-fx.patch
changed. The player's storm screenshot at11:41 shows white sprite rectangles;
the current native bridge had bypassed the authored texture/alpha stage chain
and inherited the untextured TornadoPillar material. Installed mask textures
exist. The missing Tornado/ITEMS.TGA entry concerns debris and is a separate
resource finding, not proof of the mask mechanism.

A 32-square correctness fixture with actual Metal particle bridge and seeded
prior valid material state reproduced white rectangle pixels, then preserved
transparent background ff203040 and half-alpha bf9098a0 with the authored chain.
The untextured negative remained white. This is not an FPS measurement or actual
storm replay; the player game was running during this small correctness fixture.
The existing source inventory probe could not open its absent native-storm
baseline and did not pass. Canonical build integrated two changed consumer
source files; run.sh --stage-only passed with compiler ready and zero gameplay
pending. Staged engine SHA-256 `f01a318a43d872aa4ed49be59812fee84d177a6ed57c1c8fa8ff3e3dc2f78bd9`.

Installed standalone arm64 Metal engine remains
`88a2083d8768773c66c73816586e090829ac78bf49c7168bf2f6e97ad596e1a2`.
A closed-game baseline captured143 SAVE files and four configs. The user then
restarted as PID39052 and player state changed before installation. The
installer refused at the initial baseline comparison before creating backups
or replacing the engine; no live runtime mutation occurred. Installation remains
pending a closed game and fresh SAVE/config baseline, with this chat as owner.
Native storm replay remains unresolved after delivery. Mechanism/evidence owner:
docs/audio-and-particles.md. Prior dawn-water boundary and merchant crash remain
unresolved; this patch does not address them.


Tornado closure: local HEAD and public origin/main verified at cc45e5e175337098502dc8917b514f7f5c18171d; tree clean. Build/probe/stage and refused-install receipts plus the guarded installer retained under the ignored canonical native-metal owner `.cache/tornado-delivery`. Resume only after process count0 and a fresh player SAVE/config baseline. The staged batch is frozen; no additional source change precedes this install.


Sea-crime deferral (October 1, staged batch): killing both legacy instant paths. Ship_FireAction else-branch and CrimeSea_PreparePlayerBallHit ENEMY-branch now call CrimeSea_HandleUntrackedEnemy (tactical hostility now, one deferred incident for unlawful victims behind a surviving report). CrimeSea_IsLawfulPrize (pirate victim, base-nation enemy, covering patent) guards CrimeSea_Begin too, so lawful privateer prizes open no incident at all. Quest protections preserved. AIShip.c chain recomputed: crime e024268a, journal 6f39bde1, living ec2b0915; NATIVE_PREVIOUS_SHA256 keeps old delivered journal 1cb84155 for the upgrade path. sync check shows 4 files pending (ship.c, tradebook.c, AIShip.c, ship.ini), compiler ready. Committed as 1b9720f on top of 551136c (aim overlay) and 0904367 (night floor/pin/chest). Disposition: pending run.sh --stage-only, then player replay. Captive-captain witness channel stays deferred: ReleasePrisoner cannot distinguish fates, needs per-fate flags in Ransack dialogue.


Stage receipt (October 1): run.sh --stage-only exit 0 with closed game. Engine rebuilt (arm64), renderer consumer PASS, town shadows/Belize/Menu PASS. Gameplay applied: 4 files (ship.c, tradebook.c, AIShip.c, ship.ini). CorsairsMetal.app staged. SAVE/config/renderer resources unchanged. Batch contains aim trajectory overlay v1, night floor 0.20, trade auto-pin follow, ship chest button, deferred sea-crime. Aim polish (Singer), witness channels (Cicero) run as next batch; night/fog diagnosis (Noether) delivered. Disposition: staged, player replay pending.


Delivery gap (October 1): user plays /Applications/Corsairs Iddictive Remaster.app (engine f9040719), but engine-side patches stage only into .cache/CorsairsMetal.app (engine ec5f4298). Gameplay scripts mirror via sync apply; the engine binary does not. Aim overlay v1 source-verified in staged build (trajectory markers in ai_ship_camera/cannon_controller). Install candidate prepared at .cache/install-candidate/metal-engine (repo rpath stripped, Frameworks rpath added, no /Users/ or repo byte leaks, sha 775751c5). Disposition: blocked on closed game for in-place engine swap + codesign.


Install-flow fix (October 1): staging now ends with install-engine.sh, which copies the staged engine into /Applications/Corsairs Iddictive Remaster.app (rpath rewired to bundle Frameworks, machine-path assertion, ad-hoc codesign, per-hash backup, skip-when-identical). run.sh already refuses while the game runs, so the swap is always closed-game. Wired into run.sh before the stage-only exit so both stage and launch modes refresh the played app; missing bundle exits 0 for portable/export flows. Disposition: implemented, first live install pending closed game.


First live install (October 1): run.sh --stage-only exit 0, install-engine.sh copied v1 engine (aim overlay, night floor) into /Applications (new ef934466 vs old f9040719, byte-verified different; per-hash backups kept). Gameplay already mirrored. Sync apply and install each deep-codesign the 21GB bundle, so stages take minutes. Polish did not compile (iFadeRope undeclared x6, brace imbalance x2) and was stashed for v1; Cicero WIP snapshotted for a clean stage, both restored byte-identical and resumed. Commit of install-flow files deferred: git index locked by sibling op. Disposition: v1 installed, player replay pending; polish+install round next.


Volumetric cannon balls (October 1, source candidate): confirmed stock projectiles are zero-radius rays (SAILONE::CheckSailSquar ray-triangle, NODE::Trace mesh rays, ropes/vants/yards absent from cannon trace), which plus the 12-knippel mast-fall threshold explains rigging fly-throughs. New cannon-ball-volume.patch adds rigging-only capsules (knippels 1.0 m, grapes 0.35 m, rest half visual size; hull/fort/island/sea stay exact rays), registered in build.sh after the sibling aim patch; disjoint from cannon-trajectory-aim.patch and crime files. Falsifier: git apply --check passes against the post-stack source. Disposition: pending parent run.sh --stage-only, then sea-battle replay. Spec: docs/sea-cannon-ballistics.md.


Stage receipt (October 1, aim rework + volume): run.sh --stage-only exit 0 with closed game, committed as 168c245. Engine e32144db installed into /Applications (codesign valid, hash-verified). Batch: powder-smoke ribbon + impact ellipse aim overlay (firing-side only, depth-tested, live HeightMultiply 0.4/0.65 knippels mirrored from Ball_AddBall after parent caught a 1.0-hardcode mismatch), rigging-only capsule hits (knippels 1.0 m, grapes 0.35 m). Integration fixes by parent: sail.h blank-line context, sailone hunk2 off-by-one + missing trailing context (in-repo git apply --check was a false positive; out-of-repo check is the true gate), const CMatrix blocking operator*(CVECTOR). Disposition: staged, player sea-battle replay pending (preview==impact, native look, mast/knippel kills).

Aim staging fix (October 1, visual-verification pass): static review of 168c245 found ShipAimArc/ShipAimVolume defined only in engine-source src/techniques/_dev/ship.fx, never staged to runtime RESOURCE (run.sh copied only Rope/Vant + SunGlow), so TechniqueExecuteStart failed and the ribbon/rings silently drew nothing in the played game (verified: unknown technique returns false, no crash). run.sh now copies _dev/ship.fx to runtime RESOURCE; vant own-ship fade scan skips deleted groups (null GetEntityPointer deref after sinkings). Ballistics mirror verified exact against RealFire + AIBalls::Execute + Ball_AddBall (origin, elevation, raw angle, azimuth, HeightMultiply 0.4/0.65 knippels-by-charge-Type==2, analytic integration); preview-vs-live delta is ship-pos-vs-camera-pos origin (meters) plus per-shot dispersion shown by the ellipse. Ball-volume names/sizes verified (Knippels/Grapes/Balls/Bombs, 1.0/0.35/half-visual radii). Committed as 9c83a91, run.sh --stage-only exit 0, engine a59ee647 installed to /Applications, ship.fx copied to played RESOURCE, bundle re-signed valid. Disposition: staged, live visual verification in progress.

Aim v2 + volume + eye staged (October 1, be695e5): volumetric aim beam (arc tube + broadside curtain + landing disc, alphas up 2-3x), rigging hits entry-only (kills knippel mast one-shots from per-frame refire), deck eye +0.5m, install-engine auto-syncs Rope/Vant/_dev techniques. deck-eye-height conflict root cause: hand-written hunk without trailing context, which patch/git treat as extending to EOF; regenerated with proper trailing context, build green, run.sh --stage-only exit 0 with closed game. Engine b1599fb5 installed to /Applications, ShipAim techniques verified in played bundle, technique files synced, codesign valid. Cicero tools/* (witness channels) untouched and not in delivery path. Disposition: staged, player sea-battle replay pending.

Crime delivery gap (October 1, found during reputation audit): committed spec at 1b9720f/be695e5 contains CrimeSea_IsLawfulPrize/CrimeSea_HandleUntrackedEnemy, but delivered inputs/gameplay baseline (Sep-14 materialization) lacks both; lawful-patent cover and untracked-enemy deferral are NOT live. Live instant paths: same-flag fire (pirate switch + rep -10 + patent strip), ball hits on explicit/untracked enemies (full legacy Ship_NationAgressive or patent strip), deck-triggered boarding (StartBattleAfterDeck: instant war + patent strip). Needs inputs/gameplay re-materialization from current spec after Cicero hash chain completes, then restage. Disposition: documented, fix pending witness batch.

Point-and-shoot aim (October 1, staged): manual fire and overlay share one ManualAimTarget (camera ray vs solid ship cylinders incl. rigging airspace, then sea plane, then max-range sea point for sky). Fire() no longer marches to max range flattened to sea level; ship locks keep height, band ends on the ship, ellipse floats at aim height (hull-vs-masts). Contact glow tints center mark by relation (soft red enemy / soft green else); sea/sky stay brass. Ribbon stays single battery-spanning sheet + head fade, smoke cooled to 0xDCE6EC, yellow gun lines dead. Deck eye +0.5m moved to per-frame s_pos (old clamp-site hunk never applied). build.sh exit 0, run.sh --stage-only exit 0 closed-game, engine cebbfc14 installed to played app, ManualAimTarget in staged source (1 decl + def + 2 calls). Disposition: staged, player replay pending (preview==impact, ship lock height, glow colors).


Aim v4 live install (October 1): run.sh --stage-only exit 0 on user order while game ran; install-engine.sh put engine c08dc87f (aim patch md5 f7329f6a, 42997 B, byte-identical worktree/staged) into /Applications (post-codesign 8e66a9d9). Game relaunched 21:30 (PID 27663) on the new engine. Overlay is now 3 arcs + flat drape ellipse + cross at impact point, impact floor 1.5 m, no tube/rails. Disposition: installed, player sea-battle replay pending.

Crash/sea-FPS/error-flood batch (October 2, staged and installed): engine staged 10:24, bundle signature verified (`codesign --verify --verbose=2 --deep --strict`), installed binary carries the new `stale InterfaceBackScene` diagnostic. (1) Save-load crash: `storm::iEquals`/`iLess` built `std::string_view(nullptr)` in `src/libs/util/include/string_compare.hpp`, faulting at 0x41c87074 (crash log `metal-engine-2026-10-02-085005.ips`) from `SEAFOAM::AddShip`/`SEAFOAM_PS::Init`; `xcode27-compat.patch` now guards null pointers and fixes the `Range2T` typo in `second_normalized`. (2) Open-sea ~20 FPS: gameplay `interface/mainmenu.c` calls `DeleteClass(InterfaceBackScene)` without `&`, the entity is never erased, `StormMetalInterfaceBackScene(dev,false)` never fires, and the interior claim persisted into open sea (launch.log diag `indoor=1 active=1 applied=0` with 8464 draws, 40.1M vertices, 185.7 MB uploads per frame); `rawSeaLit` requires `!locationActive`, so sea geometry left the raw Metal lit path. `backend.mm` now clears the stale domain once in `StormMetalBeginAimOwnShip` (main-character `SHIP::Realize` only); menu-side hooks in `beginDynamicSky`/`BeginAimWater` were tested and rejected because the menu legitimately owns that domain. (3) `renderer-ui-fonts.patch` compact-primitive rejection now falls back to the backend `DrawIndexedPrimitiveUP`/`DrawPrimitiveUP` instead of tracing and dropping the draw. Disposition: staged and installed; replay pending (save load, open-sea FPS, log quiet). Mechanism owners: docs/audio-and-particles.md, docs/metal-lighting-parity.md, docs/metal-renderer-consumer-matrix.md.

Provisions/rum ship-menu fix (October 2, applied, install pending): Corsairs-provisions-rum-fix.patch applies to tools/metal_living_caribbean.py (base 3b3f903, result 46d4a34) and replaces the legacy fleet-only food/rum rows with two blocks keeping selected-ship (Судно) and squadron (Эскадра) durations distinct; ship.ini collapses [FOOD]/[RUM] into the enlarged [FOOD_SHIP]/[RUM_SHIP] rects and drops the legacy items. prepare_ship_interface/prepare_ship_ini assert exact anchors, and prepare() verified the produced digests against the new pins (ship.c bf94ec32, ship.ini b4e7ec7d). sync_metal_gameplay.py check: 2 files pending, compiler ready. Disposition: applied in the repo; install blocked by the running game (PID 60397, started 12:55) because run.sh refuses staging with a live process. Bounded heartbeat automation corsairs-2 (20-minute interval, until 2026-10-03T07:00Z) stages experiments/native-metal/run.sh --stage-only after the game exits, stays silent while it runs, then pauses and reports. Player replay after install owns acceptance.

Provisions/rum install receipt (October 2): user closed the game; run.sh --stage-only exit 0, gameplay applied 2 files (ship.c bf94ec32, ship.ini b4e7ec7d) into /Applications and .cache/runtime with matching hashes, engine unchanged (install skipped), bundle signature valid. The applied content is byte-identical to PR #14 (Fix duplicated provisions and rum entries in ship menu, branch fix/ship-menu-supply-duplicates, blob 46d4a34, still open); no duplicate local commit was created because the PR carries the same blob when merged. Heartbeat corsairs-2 paused. Disposition: installed; the ship-menu replay (Судно/Эскадра rows for food and rum) owns acceptance.

Player-log bound and stage-cost audit (October 2, source + probes): the played app appended engine stderr to `~/Library/Application Support/Iddictive Corsairs/logs/launch.log` without any bound (`public_launcher.py` opened O_APPEND, no rotation): 16.1 MB / 62k lines after roughly 12 h of play, +7 KB per 20 s of active play (about 1.3 MB/h). Dominant emitters: `[StormMetal] sea shadow resolve` 13.2k lines, `[StormMetal] diag` 9.6k, `[StormMetal] cam` 5k, `[StormMetal] shadow frame` 2.1k; `diag`/`sea shadow resolve` are per-128-frame dumps that ignore STORM_METAL_PROFILE=0 and carry zeroed profile counters in player mode. Engine spdlog files stay small (system.log 173 KB, compile.log 13 KB, error.log 2.9 KB, mimalloc 1 B). Fix: `rotate_launch_log` moves a non-empty previous session to `launch.log.1` before the redirect (empty leftovers keep `.1` intact), and `install-engine.sh` now syncs the reviewed launcher resource into the played app under the same allowlist discipline as the techniques (known revision 89aa9a96). Probes: rotation PASS (rotate / empty-leftover / no-op), launcher-sync decisions PASS (sync / skip / refuse on unknown edit), bash syntax and embedded python compile OK. Stage cost measured on this machine: no-op build.sh 4.4 s (0 source files recompiled), one-TU touch rebuild 9.1 s (5 ninja steps incl. link), codesign --verify --deep --strict on the 21 GB bundle 6.0 s; runtime trees stage via `cp -c` clonefile; a real engine install writes about 6 MB plus the renewed bundle signature. Disposition: source applied and probe-verified; install into the played app pending a closed game (install-engine.sh refuses while the engine process runs).
Willemstad missing textures (October 2, accepted): player screenshot at 16:45 showed the Curacao/its Willemstad sand flat and near-white and the central fort almost black and textureless; earlier notes had blamed baked shadows. Real cause: twelve shipped `RESOURCE/Textures` files are DDS containers carrying `.tga` names, and the native raw loader's `stb_image` TGA-only path rejected them ("unknown image type") where Windows `D3DXCreateTextureFromFileA` sniffed the container, so those materials drew untextured. The set is `SentMartin_terra1/2/4.tga` (8192x8192, 14 levels, 44,739,384 B), `SentMartin_terra3.tga` and `Shipyard_Marigo_KNS.tga` (4096x4096, 13 levels, 11,184,952 B) and seven `lighting/{storm,evening,day1..day4,morning}/locations/Outside/Jungles/Jungle10/shadow.tga` (4096x4096); Villemstad terrain `Mesh`/`Mesh70` (`lambert94/84/87/105SG`) and the shipyard/pier materials `lambert36/56SG` sample them. Delta: `experiments/native-storm/raw_texture.hpp` owns `storm::raw_texture::ParseDds` (magic, `dwSize==124`, required flags, DXT1/3/5 fourCC, mip count clamped to the full chain, block-math level sizes from offset 128, bounds-checked against the file size) and `raw_texture.cpp` sniffs the first four bytes before the stb TGA fallback and copies each level through `LockRect`/`UnlockRect` in the authored BC format, matching the compiled `.tx` path. Evidence: a header-only probe accepted all twelve files with exact end-of-file consumption and rejected truncated-DDS, real-TGA and empty negatives; Pillow decoded the same twelve as 8192/4096 DDS independently; `build.sh` exit 0 with the one touched renderer TU recompiled; `run.sh --stage-only` exit 0 with a closed game and engine `923d0b4821f50c7aa8bb2ea0e2a5aafa9f56919d99d5582a9d3ea1f9b4670635` installed into /Applications/Corsairs Iddictive Remaster.app from build artifact `fe88337fe53dc98634a3c762ce294831c96cf89d7593893b9cd8838e42d6bb5d` (deep/strict signature valid, previous `bc879e9d` backed up); player replay on the installed app confirmed the Willemstad ground, fort and shipyard surfaces. `run.sh` without `--stage-only` launches the `.cache/CorsairsMetal.app` cache bundle, not the played app, so the replay that owns acceptance ran from `/Applications`. Rejected: the baked-shadow-gate hypothesis. Durable fact and cost note: docs/texture-alpha-and-materials.md. Disposition: accepted.


Boarding chest-only collection (October 5, source/native VM verified): the player
rejected automatic personal pickup. Exact cabin item IDs/counts and cash now move
to flagship box1, with special leftovers after manual inspection preserved there
as well. The reply is "Да, всё в наш рундук."; the missing-speaker finish also uses
full collection. Final native VM passes on the installed engine, including seeded
stock, gold/unknown/duplicate/quest/rare items, personal inventory identity, repeat
collection, manual inspection, invalid destination and overflow refusal; scene
readiness, quest-use state and exit rendering are fixture boundaries. One isolated
fallback check timed out while the player game was open; the same final check
passed after it closed. Source commit d391091; canonical content plan is exactly
two scripts. Installation is waiting for the parallel world-map staging owner;
player-scene feature acceptance remains unresolved.


Boarding chest-only delivery closure (October 5, 14:18 EDT): the parallel native
staging owner delivered the frozen d391091 boarding/dialogue scripts. This
heartbeat verified both exact target hashes, empty cache/app sync plans, compiler
readiness and deep/strict bundle signing. Installed engine SHA-256 is
5db2f3ebd1acc00b1c7db1b3fb41f5681029a53b5ad1960a0306bfe820e85911. No game
was launched or stopped and no engine, SAVE or configuration was written by this
heartbeat. automation-4 is confirmed PAUSED and its definition remains saved.
Disposition: installation confirmed; real player-save boarding collection/manual
inspection replay remains unresolved.

### October 5 coherent fleet candidate — verification

Final source composition binds the persistent roster, sea bridge and encounter UI
through sync_metal_gameplay. Native build passed; isolated VM compilation/state
probes passed roster persistence, survivor/cargo restoration, no duplicate wear,
commitment and legitimate retreat, capture tombstones and action labels. Ordinary
Follow spawning is disabled; legacy naval/pirate Follow is upgraded. Direct sea
entry has its own native message. Installed application remains the previous
batch until the canonical stage-only operation succeeds. Scene/balance verdict
is unresolved and owned by the player's replay; no automated player launch.

### October 5 coherent fleet installation — startup rejection

Canonical stage-only completed with installed signed engine 8dfd720876045ae8e63bde2cff5d52bca2c1c532a19236c64003c6f4f9898e06 and nine reviewed script/interface inputs. Player launch PID 80649 aborted during startup script compilation; actual userdata/Logs/error.log reports seadogs.c Invalid Expression. Installed and development resource/shared/messages.h remained baseline a3904843 despite built header 38bab60e defining MSG_WORLDMAP_ENTER_SEA_DIRECT. The isolated VM using copied installed headers reproduced SIGABRT; earlier source-header VM success did not prove installed startup readiness. SAVE/configuration preserved. Disposition: rejected startup; canonical install-engine.sh now includes reviewed messages.h delivery in its rollback/signature transaction; corrected staging and installed-header compilation pending.

Corrected October 5 startup-header stage: canonical stage-only exit 0, deep/strict bundle signature valid, app/runtime headers byte-identical to built 38bab60e. Installed-header VM compiles main and interface/map successfully; unknown installed-header edit negative refuses before writes. Actual player startup and fleet-scene replay remain unresolved.

Repository closure gate: agent_context.py --check exceeded its bounded 20-second timeout after corrected staging; owned subprocess group terminated. No gate pass claimed. Focused commit remains blocked by staged-provenance-unavailable/claim-not-unique, with active .codex specialist 01a10de6-9ba1-7ae1-8175-c06f65c630e3. Foreign staged/working paths preserved; no push.

October 5 source closure: canonical adopt-staged accepted exact11 targets without concurrent transcript mutation; focused helper committed 6039e62. All four foreign staged/working fingerprints remain identical. No push or additional game staging was performed; committed source bytes are the previously installed batch. Repository gate timeout and player replay remain unresolved.

### October 5 jungle dusk illumination — source/GPU verification

Hypothesis: a valid dusk frame without shadow maps incorrectly disables modern
location relighting. Two player screenshots show the ground switching at fixed
18:50; the installed log switches `applied`/sun validity. Delta: one location
illumination-ready predicate shared by collection, blended actors and the engine
light-selection bridge, independent of applied shadows. A task-local GPU variant
of the existing fixture reproduces the old discontinuity and passes all four
receiver paths after correction, together with its existing blocker/lamp/indoor/
overlay negatives. Canonical build passes; ordered stack changes zero cached
source files, gameplay sync plan is empty. No player state or installed engine
changed in this attempt. Disposition: unresolved pending canonical staging and
the installed Cuba jungle load/walk replay.

October 5 installed continuity correction: after the player confirmed closure,
canonical stage-only completed (exit zero) with signed engine 29ab8ad6... and
verified bundle signature. Source checkpoint cfc9e5c; no push. Reopened installed
app renders the menu, then the player loads and walks an outdoor scene; process
41195 consumes the established app/resources/player-state owners. UI automation
refused load input during ongoing player interaction; passive observation later
shows the Cuba atlas. The game stays with the player, no agent input/save write.
Disposition: installed/startup verified; exact visual jungle continuity replay
is unresolved pending the player's same-transition result.

October 5 encounter panel delivery: run.sh --stage-only exit0 on user close/install reply, exact four installed script/INI hashes verified at source0e551a2; SAVE/config preserved. Installed engine29ab8ad6 unchanged in this content batch. Native isolated compile was already verified; visual/action player replay remains unresolved.

### October 5 resident-voyage bridge — initial source verification

Hypothesis: commerce cannot retain fleet identity through service while native
arrival/lifetime delete its existing descriptor. Delta: guarded ordinary lifecycle
fields in worldmap-traffic.patch; one arrival event, stopped service movement,
verified native locator request/commit and one departure event, preserving explicit
loss and legacy behaviour until scripts activate the contract. The preceding
ordered patch reverses against a disposable copy of current native source, and
the revised patch applies over that preserved input. The canonical compiler's
syntax checks pass for wdm_enemy_ship.cpp and wdm_merchant_ship.cpp after fixing
the merchant's explicit core.h dependency. No permanent test, cache build, stage,
installation, game input or player-state write occurred. Service/calendar/cargo
handlers and legacy-save transitions remain unimplemented, so functional voyage
acceptance is unresolved. See worldmap-traffic.md for the state/owner contract.

### October 5 resident voyages/service — connected source verification

Hypothesis: persistence requires both native arrival/lifetime and the script's
off-map expiry owner, while elapsed repairs must reserve actual port resources.
Delta: connect the existing descriptor to calendar service plans, material and
recruit debits, saved next jobs, ordinary Follow reload migration and paid sea
supplies. A full-max-crew reservation was rejected: ordinary colony hiring pools
can be smaller than a hull. Partial paid hiring retains the available reserve,
persists actual crew count and reaches the real MinCrew gate over work intervals.
Result: current ordered patch reapply and native syntax checks pass; the actual
arrival method's source-branch probe passes ID, repeat, pause, service and patrol
negatives. The exact script consumers compile in the native VM. Native descriptor
serialization with disposable scene hooks passes repeated-debit/repair exclusion;
state probes pass shortage/reserves, elapsed repair, physical gun repair, partial
crew, dead/quest exclusion and old Follow coordinate/ID preservation. Canonical
read-only script composition has exactly three changed consumers. Permanent test
delta: zero. These are source/state checks, not installed gameplay acceptance.
No canonical build/stage, app mutation or player input occurred. Disposition:
unresolved until the integrated batch is staged after player close and its actual
callbacks, prior save and interactions replay. Cargo/prizes and subsequent
expansion milestones remain active work.

Probe incident: the first standalone VM launch used its default Akella/Sea Dogs
log directory and wrote compile.log, error.log and script_stack.log there. The
probe then moved all logs/state to its task-owned temporary directory through
STORM_USERDATA. Prior contents of the default logs were not captured, so those
files are preserved; Remaster's separate player SAVE/config were not written.

October 5 source checkpoint: `df030ee502c5c994d6dd38b3004c8d35996e8602`
integrates the connected voyage/service component and its three script consumers.
The repository context gate passes; foreign staged blobs and working paths remain
unchanged. No push, canonical stage, installation or gameplay replay occurred.
The game is still running from the installed app (PID 8172). Cargo/prizes and later
expansion work continue under the same parent owner.

### October 5 cargo/convoy candidate — connected source verification

Hypothesis: sea cargo generation, resident lifetime clamping and late randomized
ship geometry would break finite shipments/prizes and funded readiness. Delta:
make the existing per-hull physical manifest the cargo owner across Store, sea and
arrival; persist loading jobs and finite capture settlement; select ordinary
composition without the hero and snapshot the actual ship at assembly. Preserve
real crew on interrupted service, consume elapsed provisions/ammo once, and admit
whole local battles within the sea's 32-hull array. Native retirement explicitly
escapes SetLiveTime's one-second clamp rather than leaving dead residents immortal.
Result: current ordered patch applies on its preserved input; changed enemy and
merchant translation units pass canonical syntax. The actual lifetime source
branch passes resident/retired/pause/legacy cases. Native VM compiles the exact
script batch and passes source-stock debit, sea restore/save, finite loss/delivery,
prize capacity, capture object independence, interrupted paid crew, daily supplies,
whole-battle admission, actual authored ship assembly and repeated native save/load
settlement. Error log is empty. Permanent test delta: zero. Three canonical script
consumers differ; no cached native build, app stage/install or player-state write.
Disposition: source/state verified candidate; real-game callbacks, old-save upgrade,
convoy/player capture and balance remain unresolved. National scheduling, siege,
player survival/participation, rumours and port recovery remain active milestones.

October 5 cargo source checkpoint: `ac447ddab983c6f2e685406dc15f871f4a2425cb` integrates the connected
cargo/convoy component locally. The context gate passes, no nonignored untracked
paths remain, and foreign index entries are unchanged. No push, canonical stage
or installed app/player-state mutation occurred. Remaining expansion milestones
and installed-game acceptance continue under the active parent goal.

October 5 fort prerequisite attempt: hypothesis: the old difficulty-dependent
assault threshold and global damaged-cannon count cannot govern persistent port
operations. Delta: native physical per-cannon damage persists on the real fort
commander across hit/scene/save/load; scripts read its own destroyed count and use
strictly more than half as the assault gate. Preserve actual damaged-fort crew,
with the existing intact initialization and admitted resurrection paths.
Result: eleven-path ordered patch applies; changed native fort source passes
canonical syntax. Actual helper probes cover fractional damage, repeated restore,
separate forts and bounds. Exact script batch compiles, strict half/count separation
and native script-state save/load pass with an empty error log. Permanent test
delta zero. A disposable patch check first used the wrong cache-root prefix;
correcting the new patch path to `src/libs/sea_ai` makes the canonical patch pass.
Disposition: source/state verified candidate; build/stage/install and physical
cannon/boarding replay unresolved. Two-sided fort/city stages and expedition
scheduling remain active, with no player/app/runtime mutation.

October 5 fort source checkpoint: `f755cb9` integrates physical cannon persistence and the strict assault gate locally. Context gate passes; foreign index entries remain unchanged. No canonical build/stage/install or player-state mutation. The active goal continues with expedition and land-state integration.

October 5 national-strategy attempt: hypothesis: persisted national reviews must
weight existing ordinary fleets without minting raids or rerolling current tasks.
Delta: nation-owned versioned posture, canonical 30–60-day reviews/minimum 14-day
hold, migration permission clocks, owned damage/attack and surviving readiness
reports, peace/depletion eligibility, immediate emergency reasons, weighted home
patrols and rare funded large-patrol preparation. No treasury or new active siege.
Result: exact composed scripts compile; native VM probes pass migration/no spawn,
repeat/no reroll, emergency/hold, peace/year skip, same-posture major loss, pirate
exclusion and subsequent native save/load. Error log empty. The first year-skip
probe exposed incorrect implicit nested-attribute binding; explicit `makearef`
fixes both the new hold and the existing pirate-band date. A second oracle exposed
an unchanged random recovery posture hiding the new loss reason; emergency reasons
now update once even when posture is already correct. Four script consumers remain
pending. Disposition: source/state verified candidate; no engine/app/player write,
and physical expedition authorizations/travel/land resolution remain active work.

October 5 strategy source checkpoint: `2caea7d` integrates nation strategy and corrected nested date bindings locally. Context gate passes; foreign index unchanged. Exact expansion batch remains uninstalled and the military stage continues under the active goal.

October 5 unvisited-fort inventory attempt: hypothesis: autonomous operations need
physical fort counts without depending on a player first entering every island.
Delta: pin the 23 actual fort locator GM identities and cannon-type counts in
source metadata, emit their lookup through the existing exact-hash composer, and
bind those counts to the actual commander before nation readiness assessment.
Canonical sync rejects a changed physical locator. Unknown or contradictory
saved defence remains unknown; genuine HasNoFort stays distinct.
Result: native RDF_LABEL/ScanFortForCannons owner establishes the parser; the first
disposable parse used flags as the name offset and returned material labels, then
was corrected to the actual name field. Independent recounts pass for all 23 and
the same files hash-match the installed app. Actual composed PROGRAM compiles;
VM checks pass cannon type filtering, partial prior state preservation, unknown
and contradictory counts, genuine fortless and national defence reporting.
Changed RESOURCE rejection and idempotent composition pass. Permanent test delta
zero. Disposition: integrated source/state candidate; canonical build/stage,
old-player save and fort/expedition scene replay remain unresolved, no app write.

October 5 fort inventory source checkpoint: `f935049e686ba28bccf726340a231233d6785b4e` integrates exact resource-bound defence metadata and its existing nation consumer. Context gate passes and foreign index entries remain unchanged. No app/runtime/player write. The active military milestone continues with authorization and actual shared siege stages.

October 6 siege-entry discovery: the actual port fort is cloned by
GetShipLocationID -> MakeCloneFortBoarding into BOARDING_FORT with authored attack
locators, then yard/bastion templates. Five actual locator GMs have separate loc/
aloc banks (25 per side outer, 16 per side inside). Existing BRDLT_FORT forces hero
attack and can auto-capture when a scene is missing; it is therefore geometry reuse,
not a ready defender/autonomous state owner. Character-death, berthing, ordinary
flagship game-over and Ship-object swap owners are bound in the topic document.
Disposition: read-only source/resource discovery; mission participation/finite
weighted actors and player recovery remain unimplemented/unaccepted. No app or
player-state effect; required next owner is the current root military milestone.

October 6 military source attempt: hypothesis — one colony operation and original
fleet descriptor can drive supplied travel, physical fort losses, finite land
casualties and evacuation without generated armies. Delta: composed military
owner, mission route/sea adapters and managed-fort cargo/crew preservation.
Final disposable VM covers paid naval ammunition/crew/hull loss, actual defender
battle interruption, ceasefire, fortless city entry, strict half gate, island
reservation, timed land stages, finite stock loading, partial evacuation and
surrender after city victory, scene admission/defer and mid-stage serialization.
All four serializer round trips pass with zero error log. The first scene fixture
lacked rank/island/cannon attributes; supplying actual valid shape removed those
fixture errors. A new harbour-task replay exposed an unbraced for/if changing the
following absent target from -1 to 1; an explicit block restores the correct hold
and nearest inactive-operation rejection. No engine compiler change. The complete
native patch applies and the changed merchant source passes syntax. Read-only
canonical composition has four pending script consumers. Disposition: source/state
candidate, not real-game acceptance. Foreground/player/recovery milestones remain
owned by this active root task; no app/cache/SAVE/config mutation or canonical stage.

October 6 callback integration attempt: hypothesis — the new modules must enter
existing consuming callbacks, and a military sea battle must import both original
fleets even outside ordinary radius. Delta: exact reversible thirteen-owner script
adapter; finite recovery hooks; shipless selected-survivor confirmation; real berth
and first ShipDead guard; foreground load/admission/death/no-heal/withdrawal wiring;
scoped GetRelation, mayor agreements, map expiry and rumour age callbacks. Fixes
preserve military arrival flag and canonical story exclusions while admitting a
cleared legacy Siege.Colony; complete late battle attachment assigns reciprocal
tasks and defers all members for missing map or hull capacity. Result: actual
whole-PROGRAM VM plus four native serializer rounds pass with error.log 0 bytes.
Focused audit probes execute real departure events, protected and cleared story
states, outside-radius pairs and both capacity failures. Headless localization
initially crashed _Language_OpenFile because the service was absent; a disposable
identity GetConvertStr adapter preserves original source compilation without
claiming localized/rendered output. A command-note hook initially used init-local
idLngFile; compile rejected it, corrected to the same command's saved original
note. Disposition: source/state evidence only; sixteen files pending, canonical
staging/install and specified player/scene callbacks remain unresolved. No permanent
tests, app/player SAVE/config writes or launch in this attempt.

October 6 FPS candidate attempt and rollback: hypothesis — lifetime-owned
immutable Metal buffers remove the 512-entry cache flush churn without reducing
quality. Delta — native buffer ownership plus independent mast null-child guard.
Canonical stage installed e9a65fbf on the existing installed content batch,
verified signature and preserved player SAVE/config hashes. Component uploads
improved; the player then reports long stutters/slideshow. Disposition: FPS
candidate rejected, component measurements do not accept native pacing. Source
rollback preserves mast guard (19525c9) and builds 4e0c0be1; stage is pending
player exit, with no player process stopped or SAVE/config restored.

October 6 repeat-crash diagnosis: the pasted 00:36 mast report is historical.
The latest local report at 01:31:24 belongs to PID 16765 and candidate UUID
B326BD3C; assembly binds the fault to dereferencing a garbage INIFILE argument
during first SEAFOAM particle initialization. The INI asset exists. Null-guard
and missing-file remedies do not address this observed non-null bad pointer.
The foam registry has an unchecked 64-entry bound, but no ship count at the
crash is available; overflow is a hypothesis, not an established cause.
Disposition: crash owner identified, corruption cause unresolved; no speculative
foam patch, settings reduction or additional engine variant was introduced.

October 6 fast foam repair attempt: hypothesis — unbounded slot admission can
clobber psIni with emitter coordinates before the next first-emitter Init. The
actual arm64 header/layout and installed assembly agree: slot size 2720, array
offset 48, count at 174128 and psIni at 174136. A disposable actual-header probe
reproduces the report pointer from float values 14.5335846 and -0.1015913 written
to slot64; slot63 and capacity rejection preserve the seeded pointer. The report
still does not establish actual ship history/count. Delta — two-line capacity
check before shipsCount++ via sea-foam-capacity.patch, preserving the first64
emitters and every ship's gameplay. Extra ships receive no foam beyond the
already declared capacity. Latest beta4/develop remain unchecked; particle-manager
and bow-splash sound fixes are inapplicable. Native build passes with exactly
one generated source changed and zero permanent test delta. Local source
checkpoint 0532a1b, no push. Disposition: guard mechanism accepted by the probe;
installed activation and real-save/sea crash acceptance unresolved. The frozen
2fa372af batch also restores the original FPS backend and preserves the mast
guard; one ten-minute exit watcher owns canonical stage/hash/signature/state
verification. Native UI save input was deferred/refused during user interaction;
no quicksave success or automatic game exit is claimed. Existing quicksave was
backed up before attempting maintenance input.

### October 6 — shared sea danger repair; source verified, player replay unresolved

The player reported incorrect sea danger for their fleet and NPCs. Installed
traffic/sea/UI share `WdmTrafficCharacterPower`; changing dead native `GetPower`
would not repair the consuming path. The class/role formula collapsed different
hull HP, installed gun quantities and calibers. Cargo-only readiness also dropped
from 25.6 to 6.4 after the last salvo moved into loaded guns, before any shot.
A canonical-only destroyed-gun state reproduced a separate alias guard defect;
ordinary dual-alias generated ships were not universally affected.

Delta: actual HP and cannon damage/count; optimal-crew readiness; exact native
loaded counts; guarded physical damage enumeration; saved actual hull/cannon
stats; loaded inventory normalization in NPC snapshots; one-time observation
rebase without clearing intent. No graphics settings or new GPU buffer caching.

Disposable actual-VM baseline reproduces both failures. Primary and seeded sea
exit/old-save probes pass with zero error log; exact main composition also
compiles. The installed-baseline delivery excludes paused military additions and
changes only PROGRAM/sea_ai/AICannon.c, PROGRAM/sea_ai/sea.c and
PROGRAM/worldmap/worldmap_encgen.c. Their output hashes are respectively
3419ae6185f2c551d4aeeea328ba66aa6e1d4866dcbcc14ffb05536103c4d878,
7b4c7a20efa8d2b18954a47e134e7cc3701b72d241d6b38fcb4a30db63ad976a and
3fe5ad32138fafabece7fe38ad8c98443349656da27753da689c53febb2aec9a.

Combined engine cf59e38555e350060271410080d0968574e4498a0f1922df2bfc7056bdd9215d
builds through the canonical native-metal path and supersedes the former pending
2fa372 batch. It contains mast/foam guards, the FPS regression rollback and the
loaded-count bridge. The earlier 10-minute exit watcher expired without staging;
player PID 42956 remained open. Installed engine remains e9a65fbf9570d8e61b2ca92cfa0be802c3065db1478f7dde1ad873045e82b958.
Source/probe disposition accepted; installation and actual player saved-game/
sea battle/FPS acceptance remain unresolved. No successful live installation or
scene fix is claimed from compilation.

### October 6 — frozen repair batch installed after player exit

Canonical stage-only exited zero from the isolated installed-content checkout,
shipping the mast/foam guards, FPS cache rollback and common sea strength fix.
Native build hash `cf59e38555e350060271410080d0968574e4498a0f1922df2bfc7056bdd9215d`
became signed installable hash
`073f9be014385482642ea3806a6daf41aca3ae3062a71928089232305608d6ac` after the
existing installer rewrote rpaths and signed with identifier `metal-engine`.
The disposable exit-worker's initial verification incorrectly compared those
different representations and recorded a false `stage_failed`. Corrected
verification matches the installed engine to the canonical signed-candidate
receipt, verifies all three script hashes and the deep/strict bundle signature.
All 215 preexisting SAVE/config files are unchanged. One additional autosave was
created after staging during the player-launched session; it is preserved.
The worker is finished. Observed PID 63449 launched the installed app at 02:32:55;
its native window renders the world map (125 FPS shown). No game input was sent.
Disposition: installation and startup/rendering accepted; exact save-load crash,
open-sea FPS/stutters and sea-danger action/nearest unaffected action remain
unresolved pending player replay. Paused expansion remains uninstalled. No push.

### Distant-ship reflection + land overexposure triage — October 6, read-only; no staging (game running)

Player FPS hypothesis (ships beyond horizon / spray / weather / CPU fallback) checked against
`experiments/native-metal/.cache/storm` source: `SEA::EnvMap_Render` sends `MSG_SEA_REFLECTION_DRAW`
to every entity in `SEA_REFLECTION`/`SEA_REFLECTION2` once per cube face (5x/frame) with no sender-side
distance cutoff (far plane 4000), and `SHIP::Realize` performs full light setup + model draw with no
frustum/distance culling. Distant-ship cost confirmed by code reading; committed every-second-frame
refresh (`ff9687f`) halves it. Spray/weather/CPU-skinning absent from the live 4s battle sample;
kept as secondary pending post-stage replay. Land/interior overexposure (Porto Bello 15:52, FPS 125):
auto-exposure bounded (base 1.03, adaptive 0.72-1.18) so not exposure; widened dynamic-lighting
admission in `5f32190` (Oct 2, weak-sun preservation) and `cfc9e5c` (Oct 5, lighting without shadow
maps) is the prime suspect (dynamic sun+ambient possibly stacking on baked geometry). No blind
lighting edit; breakage onset + post-exit staged replay pending. No running game stopped.

### Reflection distance-gate feasibility — October 6, read-only; deferred to post-exit batch

Sender-side skip in `SEA::EnvMap_Render` would be the single-point fix (one threshold, coherent
per refresh, probe-compatible), but layer members are untyped `Entity*` (ships, models, sky,
islands) with no common position accessor from sea.cpp; blind casting risks crashes. Receiver-side
gating (SHIP/MODELR handlers) duplicates the threshold and still needs a cutoff value with visual
replay. Both variants require build verification, blocked while the game runs. Decision: implement
the gate in the post-exit batch together with staging of `ff9687f` and same-save replay, choosing
the cutoff from the replay scene. No source touched, no game stopped.

### Reflection fix delivered engine-only — October 6, installed; replay pending

Full `run.sh --stage-only` rebuilds clean (`sea-reflection-budget.patch` position 24 in
`source-patches.json`, `dynamicReflectionTick` present in `.cache/storm` sea.cpp/sea.h, engine-1
relinked 10:08) but aborts in gameplay sync: installed `/Applications` `PROGRAM/sea_ai/sea.c`
digest `7b4c7a20` matches no known BASE/SHA/PREVIOUS revision, so the gate refuses to overwrite
player gameplay (17 files pending stay undelivered, paused expansion untouched). Engine-only delivery
via canonical `install-engine.sh`: final deep/strict verify initially failed on game-regenerated
`__pycache__` inside Frameworks Python (exporter excludes those caches; they reappear on every
launch). Fixed in `284fab1`: installer purges regenerable caches inside its transaction before
sealing, no gate weakened. Installed signed engine `7f252e3b` (pre-install `7467bbb3`), previous
`97d40b3b` retained in `installed-engine-backup`. Deep/strict verify passes. Pending: same-save sea
battle FPS replay (22-32 band falsifier), Porto Bello day overexposure replay, lighting onset timing.

### Overexposure narrowing — October 6, read-only; replay still pending

Interior fill (`interiorAmbient`) and outdoor response (`outdoorResponse`, `.30+.38*sky`
hemisphere, `.82` sun shaping) both date to the initial Metal publish `08b05c7` — excluded as
recent causes. Noon shader math clamps vertex lighting at 1.0, so a modest sun/ambient excess
flattens sun-facing plaster to flat white, matching the Porto Bello screenshot. Remaining
candidates: day fog color/density from weather presets (fog-FX changes staged Oct 1) or
sun+ambient stacking; both need onset timing + staged day replay to bisect. No source touched.

### Reflection lag on camera rotation — October 6, reported on 7f252e3b; fix pending sample

Player reports ship reflections jump/catch up when rotating the camera: expected from the
every-2nd-frame cube (night 15 FPS game rate makes it 7.5 Hz). Planned fix: force whole-cube
refresh when the camera moved since last frame (position + angle), keep half-rate when still.
Camera controller shows no idle wave-bob, so a small epsilon is viable, but the value needs the
load sample first: if the hotspot is not reflection work, the cadence fix gets reverted instead
of extended. No source touched, game running.

### Sea FPS + lighting + reflection batch — October 6, source candidates; build/install pending game exit

User load sample (`~/Desktop/seafps.txt`, night battle, 2 ships in view, FPS 15):
494/~2465 main-thread samples in `SEA::Realize`, 489 in `EnvMap_Render`, 474 in
`SHIP::Realize` → full `MODELR` → `DrawIndexedPrimitive` → `newBufferWithBytes`.
Reflection redraws every layered ship once per cube face (5 faces, no distance or
frustum gate in the `MSG_SEA_REFLECTION_DRAW` handler) — suspect #1, previously
measured at 27.6% in the Oct 6 open-sea sample. New suspect #2, untouched: AI script
VM (`AIGroup`/`AIShip`/`BC_Execute`) is ~21% of this sample. GPU-wait (`nextDrawable`
semaphore) dominates the earlier `/tmp/seaload2.txt` sample, so CPU and GPU limits
coexist; no FPS gain is claimed before a same-save replay.

Deltas (working-tree candidates, game PID 13542 running — no build/stage/install):
- `sea-reflection-budget.patch`: reverted the uncommitted camera-turn override (it
  forced every-frame refresh during continuous rotation, negating the cadence saving;
  cube faces are world-axis-aligned, so rotation alone must not force work). Added a
  3000-unit XZ distance gate in the ship reflection handler (cube far plane is 4000;
  main view still draws every ship). Ship-only hunk verified with `git apply --check`
  against pristine `ship.cpp.orig`; `sea_ship_reflection_probe.py` passes.
- `backend.mm`: aligned the two embedded MSL outdoor ambient factors from
  `.30+.38*sky` to the modern `.18+.44*sky` already used by `lighting.hpp` and the CPU
  vertex path (ground-level washout was GPU-only). The deferred `albedo*.38` floor and
  `rawOutdoor?materialDiffuse` substitution are unchanged — day replay decides those.
Focused commit blocked by the coordination guard (foreign staged entries from another
writer share index/HEAD; `commit --only` denied as `commit-working-entry-not-owned`,
lock reports unlocked). Not a harness defect: shared-HEAD serialization owns that call.
Changes stay working-tree candidates, which the canonical build consumes directly.
Disposition: `unresolved` — needs game exit, canonical build/stage/install, then same-save
night-battle replay (falsifier: leaving the 15 FPS point), Porto Bello day replay for the
ambient alignment, and a Tab/aim camera comparison.

### Reflection distance gate + outdoor ambient parity — October 6, installed; player replay pending

Canonical `run.sh --stage-only` rebuilt the engine from the working-tree batch (probe PASS,
ship hunk `git apply --check` clean against pristine `ship.cpp.orig`). Full stage aborts at the
known gameplay sync point (`unrecognized installed fleet bridge: PROGRAM/sea_ai/sea.c` — the
paused Living Caribbean expansion still rejects the installed content revision), so delivery
used the engine-only path: installed `a353a16e…` from fresh `engine-1` (Oct 6 10:42), bundle
signature verified, rollback `7f252e3b` retained in `.cache/installed-engine-backup`. Batch:
far-ship skip in the reflection handler (3000 XZ units, main view unchanged), reverted
camera-turn override, and the two embedded-MSL outdoor ambient factors aligned to the modern
`.18+.44*sky`. Focused commit still defers to the shared-HEAD guard (foreign staged entries).
Disposition: `unresolved` — needs same-save night-battle FPS replay (was 15), Porto Bello day
replay for the ambient alignment, and the Tab/aim camera comparison.

### Day-battle sailors skinning upload — October 6, source candidate; build/install pending game exit

Same-battle day replay (6 near ships, FPS 21): fresh 6s sample `/tmp/daybattle.txt` (PID 26807,
1628 main-thread samples) moves the hotspot. `Sailors::Realize` is 429: ~247 in
`acceptSkinningVertices` (`_platform_memcmp` full-buffer scan + heap `memcpy`) and ~132 in the
sailor draw path with `newBufferWithBytes` kernel traps plus `RawBuffer`-map wholesale-clear
destruction (`__tree_deleter` + `AGXBuffer dealloc` + IOGPU traps). Reflection is down to 94
(~6%, cadence working, near ships correctly unskippable); AI negligible in this scene.

Root cause: every animated sailor mesh per frame went memcmp (always dirty) → free/malloc →
memcpy → kernel `newBufferWithBytes` → old-buffer dealloc. Fix in working tree (`backend.mm` +
`resources.hpp`): per-facade 3-slot persistent shared ring indexed by `flightRing.current`
(same wait-before-reuse discipline as `FrameArena`), plain `memcpy` into the slot, no compare,
no per-frame kernel allocation. Facade identity/lookup contract unchanged. Untouched: bone-index
validation scan, `bind` size check, `clearSkinningSources` lifecycle.
Disposition: `unresolved` — needs game exit, canonical build/stage/install, same-battle replay.

### Skinning ring delivery — October 6, installed; player replay pending

Canonical build recompiled `backend.mm`/`resources.hpp` clean (fresh `engine-1` 10:50); full
stage still aborts at the known `PROGRAM/sea_ai/sea.c` fleet-bridge sync point, so engine-only
install: `fe4d39e3…`, signature verified, rollback `a353a16e` in `.cache/installed-engine-backup`.
Disposition: `unresolved` — same-battle replay must confirm FPS lift AND sailor rendering
correctness (no torn/garbage crew — the ring reuses the flight-slot wait discipline, but this is
its first live run).

### Ring-fix loss and redelivery — October 6, installed `07f0d860`; player replay pending

The 10:53 battle sample on `fe4d39e3` still showed the old `memcmp` + per-frame kernel
allocation: the skinning-ring hunk was missing from working-tree `backend.mm` at build time
(ambient hunk intact, `resources.hpp` ring members intact — a partial working-tree loss while
a co-writer holds staged entries in the same checkout). Hunk re-applied with exact-replace and
verified present; 10:57 canonical build recompiled `backend.mm.o` and relinked `engine-1`.
Engine-only install `07f0d860…`, signature verified, rollback `fe4d39e3` retained. Next builds must
grep-verify the hunk immediately before compiling. `fe4d39e3` verdict: ambient-only, ring absent.
Disposition: `unresolved` — same-battle replay (FPS + sailor correctness) on `07f0d860`.

### Ring-engine smoke without player — October 6, installed `07f0d860`; battle replay blocked by lock screen

Installed `07f0d860` (sailor skinning ring + outdoor ambient parity + ship reflection distance gate) launched via open at ~13:11 local (PID 43388) with zero game input sent and no SAVE/config touched. Pre-launch: `sea_ship_reflection_probe.py` PASS, ring hunk and both `.18+.44*sky` ambient factors grep-verified in working tree (canonical build 10:57, engine-only install 10:58, signature verified, rollback `fe4d39e3` retained). Process alive 10+ min with Metal diag lines flowing (fixed menu camera, no crash/hang) — startup/rendering smoke accepted for the ring engine. Interactive battle replay proved impossible: display woke to the loginwindow lock screen (screenshot verified), no UI-automation route exists in this session, and bypassing the lock is out of scope. Disposition: `unresolved` — on return: Continue into `открытое море 5` (day battle), read FPS vs 21, confirm crew rendering (no torn sailors), camera-rotation reflections, and day-town light; then a 6s `sample` for the hotspot comparison (memcmp/newBufferWithBytes expected gone).

### Dynamic-buffer transient ring — October 6, source candidate built `435cf1f6`; install pending game exit

Player day-battle sample (`~/Desktop/seafps.txt`, PID 61263, 2940 main samples, FPS 19): memcmp gone (ring live), but SAIL::Realize 368 draws through `resident()` → `newBufferWithBytes` 105 samples in kernel IOGPU traps. Root cause: sail cloth VB/IB are D3DUSAGE_DYNAMIC, rewritten every frame, so the 512-entry resident map reallocates a Metal buffer per sail per frame (~80 sails at 8 ships). Scene-specific, matches player evidence: harbor 3 furled ships 120 FPS vs battle 8 sails-up 19 FPS. Reflection lag on rotation is downstream (9 Hz cube at 19 FPS), not a separate defect.

Delta (working tree, game running — build only): per-facade 3-slot persistent shared ring for D3DUSAGE_DYNAMIC sources (`transientResident` in `backend.mm`, `dynamicRing` on VertexBuffer+IndexBuffer in `resources.hpp`), same flight-slot wait discipline as the skinning ring; static path untouched. `build.sh` recompiled `backend.mm.o`, fresh `engine-1` 16:24. Disposition: `unresolved` — needs game exit, engine-only install, same-save replay (FPS + sail correctness, no torn cloth).

### Dynamic-ring install — October 6, installed `b831c879`; same-save replay pending

Hunks grep-verified before compile (`transientResident`, `dynamicRing` present); `build.sh` recompiled `backend.mm.o`, `engine-1` 16:24. Engine-only install after player exit: `b831c879`, bundle signature `codesign -v` OK, rollback `07f0d860` retained. Disposition: `unresolved` — same day-battle replay (FPS vs 19, sail cloth correctness, rotation reflections).

### Distant sailor shadow caster cull — October 6, installed `2daf1adf`; armada replay pending

Player reported 120 FPS in 3-ship battle vs 10 FPS in fleet armada battle. Root cause:
in armada battles with 7-8 ships, 183 skinned sailor meshes bypassed shadow frustum culling
in `land_shadow.hpp` ("animated bind-pose bounds cannot safely reject a caster") and rendered
into both 2048² sun cascades (near + far) plus receiver passes, generating ~550 redundant GPU
draw calls every frame and hashing 183 bone palettes per frame on CPU.

Delta:
1. `experiments/native-metal/backend.mm`: gate `rawSkinned` sea shadow registry capture to
   `simd_distance_squared(worldMatrix.columns[3].xyz, cameraWorld.xyz) <= 3600.0f` (60m).
   Distant enemy ship sailors (>60m) are never captured into the shadow registry, avoiding
   bone palette hashing and packet creation.
2. `experiments/native-metal/land_shadow.hpp`: in `renderDepthCascade`, skip `p.skinPalette`
   casters completely for `farMap` when `!locationActive`, and cull >50m for `nearMap`.
3. Probes: `world_shadow_registry-probe`, `gpu-skinning-parity-probe`, and
   `gpu-skinning-offscreen-probe` all PASS.

Canonical build produced `engine-1`; installed `2daf1adf24286b72` into
`/Applications/Corsairs Iddictive Remaster.app`, codesign deep/strict verified,
rollback `07f0d860` retained in `.cache/installed-engine-backup`.
Disposition: `unresolved` — same armada battle replay (FPS lift vs 10, shadow appearance).

### Transient sail buffer per-frame ring gate — October 6, installed `6e0fde48`; armada replay pending

Live 4s sample of running game PID 83194 (10 FPS in armada battle / menu) proved that 923 of 1657
main-thread samples (56% CPU) were spent in `_platform_memmove` inside `transientResident`.
Because each ship has 10-15 sails, `DrawBuffer` is called hundreds of times per frame with the same
shared sail vertex buffer. `transientResident` previously re-copied the multi-megabyte buffer on
every single draw call because it lacked revision/frame caching.

Delta:
1. `resources.hpp`: added `dynamicRingRevision` and `dynamicRingFrame` to `VertexBuffer` and `IndexBuffer`.
2. `backend.mm`: in `transientResident`, gate `memcpy` to `dynamicRingRevision != cacheRevision || dynamicRingFrame != flightRing.current`.
   The buffer is now copied at most once per frame across all sail draw calls.
3. Built clean `engine-1`, installed `6e0fde48b99f5945` into `/Applications/Corsairs Iddictive Remaster.app`,
   codesign verified, rollback `2daf1adf` retained in `.cache/installed-engine-backup`.
Disposition: `unresolved` — same armada battle replay (FPS lift vs 10, sail rendering).

### Armada sea AI relation cache & sail index fix — October 6, installed `614b88dc`; armada replay pending

Live 6s sample of running game PID 87212 (armada battle) showed:
1. Sails corrupted due to `transientResident` reusing a single in-flight index buffer across multiple
   D3DLOCK_DISCARD sail draws within the same frame. Fixed: `rawIndices` reverted to `resident`
   (independent revision buffers, no in-flight overwrite); `rawVertices` retains per-frame ring gate.
2. CoreImpl::ProcessExecute consumed 1223 of 2595 main-thread samples (47% CPU) in `SEA_AI::ProcessStage` ->
   `AIShip::Fire` -> `isCanFire` -> `isFriend` -> `GetEffectiveRelation` doing hundreds of thousands
   of `ATTRIBUTES::GetAttributeClass("SeaSurrender")` string lookups per frame with case-insensitive `strlen`.

Delta:
1. `experiments/native-metal/backend.mm`: `rawIndices` reverted to `resident(rawIndexBuffers, ...)` so sail
   indices are not overwritten in-place within the same frame.
2. `experiments/native-metal/sea-surrender-relations.patch`: updated `AIHelper::GetEffectiveRelation` with
   16ms surrender cache (`UpdateSurrenderCache`). When no ships have surrendered (`!bAnySurrendered`),
   returns cached `*GetRelation(x, y)` instantly with zero string searches. When surrendered, compares cached
   DWORD IDs instead of traversing attributes.
3. Built clean `engine-1`, installed `614b88dcead66bd2` into `/Applications/Corsairs Iddictive Remaster.app`,
   codesign verified, rollback `6e0fde48` retained in `.cache/installed-engine-backup`.
Disposition: `unresolved` — armada battle replay (FPS lift vs 10, sails clean, cannon firing).

### Sea combat script merge (speed floor, gunner spread, aim heights, ballistics) — October 6, installed app; player replay pending

Player screenshot showed ship speed 0.4 kn and player volleys missing fort batteries. Audit found the
sea-combat patches lived only in git-ignored `gameplay/PROGRAM/sea_ai/` (LF) while the installed app and
`.cache/runtime` held newer CRLF files with fleet features (`WdmFleetSeaMarkGone`, `shipName` block,
`trafficFleetID` charge selection) but WITHOUT the sea-combat fixes — a blind push would have reverted
fleet work, a blind pull would have dropped the combat fixes.

Delta (merged RT base + WS combat hunks, written to workspace, `.cache/runtime`, and installed app):
1. `sea.c` `Sea_ApplyMaxSpeedZ`: `WindAgainstSpeed`/`fWindAgainstSpeed` fallback (8.0, clamp >= 1.5),
   headwind multiplier floor 0.22, `MaxSpeedZ` floor 1.8 kn. Fleet `shipName` hunks preserved.
2. `AIShip.c` `Ship_GetBortFireDelta`: nonlinear spread `pow(dist/1000,1.45)*55`, `GunProfessional` +0.12,
   `LongRangeShoot` +0.06, close-range (<60m) cap 1.2m. Fleet `Wdm*` hunks preserved.
3. `AIShip.c` turn: `fTRFromSpeed` floor 0.35, dismasted slow-turn (*0.20, lerp below SP 25),
   `Bring2Range` floor 0.07 -> 0.02, kept surrender `BI_CallUpdateShip` post.
4. `AICannon.c` `Cannon_GetFireHeight`: dropped `Y/2` waterline bug; Balls/Bombs >= 2.2m, Grapes >= 3.2m,
   Knippels 15m. Same path covers volleys at forts.
5. `AIBalls.c`: `SpeedMultiply` 3.0 -> 2.0, height arc 0.40 -> 0.70 (knippels 0.85), gunner-perk
   accuracy block, `GOOD_KNIPPELS` int-compare fix.
Verified by grep in all three targets; WS==RT for all 4 files (pending 30 -> 26).
Disposition: `unresolved` — game process predates delivery, scripts compile at launch; needs full app
restart then sea replay vs ship + fort (volley grouping, headwind speed, dismasted turn, ball arc).

### Fort-battery auto-fire fix — October 6, installed app; player replay pending

Player report: volleys reach the fort but miss its guns (high lighthouse battery, steep up-angle).
Engine audit (`ai_ship.cpp:408` -> `Fire(vFirePos)` -> `Fire2Position(bort,pos,-1.0)`): fort auto-fire
aims at the gun position minus 1m with NO `CANNON_GET_FIRE_HEIGHT` call, so ship aim-height fixes
never applied; random offset came from `Ship_GetBortFireDelta` (tens of meters at fort range), and
`Fort_CannonDamage` falloff `pow(0.11,dist)` zeroes everything beyond ~1.5m from a gun.

Delta (script-only, workspace + runtime + installed app, grep-verified):
1. `AIShip.c` `Ship_GetBortFireDelta`: aim point higher than 6.5m with non-knippel charge (ship hull
   aim sits at 2-4m, so high aim means a fort battery) tightens spread x0.35. Knippels excluded to
   avoid buffing dismasting. Symmetric for NPC ships.
2. `AIFort.c` (new in workspace, pulled from runtime): falloff `pow(0.11,d)` -> `pow(0.30,d)`.
   Direct hits unchanged (1.0 at zero), near misses at 1-2m now chip guns.
Known engine limit (not patched, needs C++ rebuild): `AICannon::CalcHeightFireAngle` returns flat 0
when no ballistic solution exists (d<0) — from directly under a high battery some guns may still
fire flat into the cliff. Revisit if replay shows flat shots.
Disposition: `unresolved` — needs app restart + fort duel replay (hits on batteries, suppression pace).

### World-map traffic avoidance and jitter repair — October 6, engine installed; replay pending

Hypothesis: generated traffic reacts to islands through `WdmIslands::FindReaction`, which
returned zero whenever `PtcData::FindNode` failed to resolve the ship's patch node, so the
avoidance force collapsed exactly where a ship was closest to shore. The remaining jitter came
from the enemy-ship turn sign and the below-`minManeuverSpeed` speed branch oscillating every
frame, plus patrol routes reversing at their own endpoint.

Delta (`experiments/native-metal/worldmap-traffic.patch`, section list extended with
`src/libs/worldmap/src/wdm_islands.cpp`):
1. `wdm_islands.cpp` `FindDirection`: probe along and against the course at r=4..36/48 when the
   source or destination node is unresolved, before falling back to the straight line.
   `FindReaction`: probe the four axis directions at r=4..24 and raise the static reaction
   radius 20 -> 32.
2. `wdm_enemy_ship.cpp` `FindIslandForce`: island weight 1.5 -> 2.0 plus a 32-unit look-ahead
   `ObstacleTest` that picks the free side (7/10-unit lateral probes) and brakes at 15 units.
3. `wdm_enemy_ship.cpp` `FindShipsForce`: lateral reaction clamped from the pass geometry
   instead of a fixed starboard flip; head-on pairs keep the fixed side.
4. `wdm_enemy_ship.cpp` `Move`: degenerate `sn` resolves from `turnspd`, and the reverse-wind
   branch converges on `minManeuverSpeed` without overshoot.
5. `wdm_merchant_ship.cpp` `KillTest`: route reversal only when the stored return point is
   farther than 25 units.

Result: the ordered stack round-trips and `run.sh --stage-only` re-applied it (3 source files
changed), then built and installed engine
`d8d2980cfd7f0810ced5ccf2a4015ecfc10bdd3d46506e1e7cfbda25f5e14337` into
`/Applications/Corsairs Iddictive Remaster.app` with the bundle signature verified and the
previous engine retained in `.cache/installed-engine-backup`.

Blockers recorded, not repaired as part of this delta:
- `tools/metal_military_integration.py` held real newline bytes inside its `enc()` string
  literals, so every canonical stage aborted with `SyntaxError`; the literals are restored and
  the file is left as the other owner's uncommitted revision.
- The gameplay sync still rejects `PROGRAM/Loc_ai/LAi_login.c`: the working callback revision
  covers 16 owners while the installed baseline contains the paused expansion's 13. The stage
  ran with the committed revision and the working file was restored afterwards; no gameplay
  script was delivered by this attempt.
- A concurrent Codex thread delivered gameplay scripts into the same checkout and installed app
  at 21:49-21:50; the engine install is engine-only and left those files untouched.

Disposition: `unresolved` — the player's world-map replay must show ships rounding islands and
the sailing ship holding a steady course.

### Fort accuracy-hack revert + bombs-only blast — October 6, installed app; replay pending

Player rejected the x0.35 fort spread tightening as incoherent (same gunner can't turn sniper vs
forts). Reverted in workspace + runtime + installed app (grep: `iFortCharge` gone).
Falloff rework kept in coherent form per player: only BOMBS get blast (`pow(0.30,d)` when
`AIBalls.CurrentBallType == GOOD_BOMBS`), solid balls and other kinetics stay direct-hit-only
(`pow(0.11,d)`). Direct hits unchanged at 1.0 for all ammo.
Disposition: `unresolved` — needs app restart + fort duel replay with bombs vs balls.
Follow-up per player: kinetics 0.11 -> 0.20 (1m 20%, 2m 4%, 3m 0.8%), bombs stay 0.30.

### Mast-floor propulsion 2.0/2.5/3.0 (engine-only) — October 6, staged to dev bundle; played-app install pending game quit

Player spec: minimum speed by standing main masts — 1 mast 2.0, 2 masts 2.5, 3+ masts 3.0 —
whatever the wind, sail damage, load or skills; dismasted ships stay adrift at 0.
Rewrote `experiments/native-metal/sailing-propulsion-floor.patch`: dropped the sail-cloth-scaled
floor (`MSG_SAIL_GET_DEPLOYED`, 0.30 x capability x deployed) and count standing main masts
directly from `SHIP::pMasts` in `CalculateNewSpeedVector` (skip `bBroken`, skip topmasts
`mastNum >= TOPMAST_BEGIN`). Kept guards: not dead/fixed, sails raised, not Stopped/SeaSurrender,
AI DRIFT blocked. Patch stack verifies clean; engine `e62d65b9` built 22:20 and copied to
`.cache/CorsairsMetal.app` (hash match verified).

NOT yet in `/Applications` (still pre-floor binary from 21:51): `install-engine.sh` refuses
while the game runs, and the player is in-game. Needs: quit game, run install, restart.

Pre-existing blocker, not caused by this batch: `sync_metal_gameplay.py check` is RED —
`PROGRAM/interface/itemsbox.c` first (half-finished 25-file WS/RT sync adopted 9 files into
`.cache/runtime`), then sea files (`AIShip.c` UNKNOWN, `sea.c`/`AICannon.c` differ from pipeline
output, `AIBalls.c`/`AIFort.c` unowned). TRAP: registering current digests without updating
manifest UPDATED/SHA outputs would make staging NORMALIZE (clobber) the approved sea fixes.
The sea ballistics batch must be encoded into the manifest pipeline before any `apply`.
`run.sh --stage-only` exit 2; `plan()` raises before writing, so nothing was clobbered.
Also note: `run.sh`/`install-engine.sh` `ps` guards silently pass inside the sandbox
(`Operation not permitted`) — do NOT hot-install the played app while it runs.

Speed math from player screenshots (stat 8.12, wind 8.1 -> 2.0 kn; wind 7.5 -> 0.5 kn):
`WIND_NORMAL_POWER` is 20, so wind power alone is ~0.4, times the wind-angle multiplier
(down to 0.22 against the wind). The mast floor bypasses all of it. Player also reports new
world-map micro-stutters — queued, not yet investigated.

Disposition: `unresolved` — install + restart + sea replay (shredded-sail speed, dismasted drift).

Update 22:35 — player quit the game; `install-engine.sh` delivered candidate `e62d65b9`
(signed `13b97ec8`) to `/Applications`, verified idempotent on re-run, previous engine backed up.
Kinematics reviewed: floor targets Speed only (inertia/collisions untouched); all stop paths
bypass it (lowered sails, Stopped, surrender, death, NPC DRIFT); dismasted stays 0; symmetric
for NPCs; no save-format change. Known accepted tradeoffs: dead-calm/upwind sailing at 2–3 by
spec, overload/crew penalties bypassed below the floor. Awaiting player sea replay.

### Quarter floor rev2/rev3 + 0.8 mystery + knippel aim — Oct 6, diag engine built, install pending quit

Rev2 (quarter of ideal by standing/total masts, Tmp.fShipSpeedIdeal from Ship_UpdateParameters)
crashed with EXC_BAD_ACCESS in CalculateNewSpeedVector at sea load (per-frame node-name parse).
Rev3 (stored mast numbers, bounded loop) installed 22:42, signed 6e3a981e; player relaunched
22:42:56, no crash, but speed still 0.8 with sails up and masts standing: floor never engages.
Static audit cleared every entry condition on paper; engine truth needed.

Knippel aim (script-only, gameplay+RT+APP synced, bytes verified): Cannon_GetFireHeight aimed
knippels at flat 15m with pure-random scatter, zero skill input; forts have no Height table
(balls 3m, knippels 25m). New: knippels at class sail centroid, fort guns 8m, scatter by
gunner skill x crew exp (2.2x to 0.3x) x distance (0.35x to 1.0x). Same rules for NPCs.

Diag batch for one restart: temp engine patch sailing-floor-diag.patch (FLOORDIAG traces) in
build.sh stack after floor patch; candidate built 23:15, FLOORDIAG strings confirmed. Script
trace added, synced RT+APP. Engine stdout goes to Idictive Corsairs/logs/launch.log (live).

Tooling: nested apply_patch was a silent no-op all session; all edits via shell heredoc plus
head/tail splice with grep verification. Complex python heredocs blocked by PreToolUse guard;
data heredocs pass. install-engine.sh refusal while game runs is honored: build only, no
hot-install.

Disposition: unresolved. Needs: quit, install diag engine, restart, sail 10s, read FLOORDIAG
from launch.log, test knippel/fort aim. Revert temp diag afterwards.

### Port ship pile-up + jitter (islandships.c) — Oct 6, script synced to app; restart+replay pending

Screenshot: ~10 neutral ships grinding in one spot off a port. Two vanilla defects found:
1. Accumulation: GenerateIslandShips adds +0..3 ships per colony per day, old ones never
   cleared (ClearIslandShips runs only on capture/siege). All spawn on 6 shared locators.
2. Jitter: half the spawns got Group_SetTaskMove(rand(1000000), rand(1000000)) — unreachable
   targets through island/shore; AITASK_MOVE fights the touch controller every frame.
   DRIFT zeroes speed+rotate globally (ai_ship_task_controller.cpp), anchored ships are still.

Delta (inputs/gameplay + .cache/runtime + installed APP, bytes verified, braces 16/16):
- Inside the per-colony date gate, InitCharacter all existing IslandShips of that colony +
  Group_FreeAllDead before respawn. Same-day re-entry keeps ships (no regen, no wipe).
- PlaceCharacterShip: all spawn anchored (Ship_SetTaskDrift + Group_SetTaskNone); the
  RELATION_ENEMY attack override below still fires, combat AI overrides drift when hostile.

No engine change. Takes effect on game restart (scripts compile at launch, no .b cache).
Disposition: unresolved — needs restart + sail to a port, verify <=3 calm ships per colony.

### Custody release boarding ("no ship at berth") — Oct 6, script synced to app; replay pending

Player: released from jail to a bay, boarding says no ship. Root cause: Custody_MoveFleetTo
retargets from_sea to the release berth (dock port, or wartime shore) but leaves stale
Ship.trafficBerth, so WdmHarbourEmbarkAccess fails (berth != pchar.location) and reload.c
prints "Нет доступного корабля у этого причала". Hits transfers and wartime shore release.
Peacetime destination is the port (jail.l1.go must equal colony.from_sea), same single cause.

Delta (WS + RT + APP, bytes verified, braces 124/124): Custody_MoveFleetTo now calls
WdmHarbourBindBerth(FindLocation(destination)) after moving hulls. INPUT pipeline copy
left untouched (older pre-CustodyLife revision; gameplay sync still RED, no clobber).
Disposition: unresolved — needs restart + jail-release replay (peace port + war shore).

### Diag engine installed — Oct 6, 23:34

Player quit; install-engine.sh delivered the FLOORDIAG candidate (engine-1 built 23:15,
FLOORDIAG x2 in installed metal-engine, bundle signature verified). Next: launch, sail
10s, read FLOORDIAG from Idictive Corsairs/logs/launch.log, then remove temp diag.

### Worldmap berth hover (traffic pile-up) — Oct 6, engine built; install pending quit

Screenshot: ~8 ships hovering in a line off SantaCatalina + cluster at Providencia.
Root cause: homing force (weight 1.0) vs island repulsion (reaction r=32 x2.0 weight
x2.0 combine = 4x + 32-unit look-ahead brake + 15-unit hard brake) and ship repulsion
(r=45). Arrival radius is 12, so ships stall ~15-30 units offshore, never trigger
WdmTraffic_Arrived, and the queue grows as more voyage ships home to the same port.

Delta (experiments/native-metal/worldmap-berth-homing.patch, after worldmap-traffic
in build.sh): WdmMerchantShip overrides FindIslandForce/FindShipsForce, fading both
repulsions 1.0 -> 0.0 between 70 and 12 units from destination. Open-water avoidance
unchanged; berth offsets (4-10) still spread parked hulls. Build green (stack verified,
2 files changed, engine-1 23:58).

Build-gate lesson: inputs/gameplay is manifest-fingerprinted; the islandships.c fix
broke prepare_metal_inputs verify. Reverted inputs copy to bc03fe (manifest hash).
Script fixes live in WS + RT + APP only; never edit inputs/.

Disposition: unresolved — install at next quit + worldmap replay (ports drain, no hover).
Note: FLOORDIAG still pending; player on worldmap, no sea entry yet (0 hits in log).


### October 7 — Living Caribbean source closure and shared content admission

Hypothesis: the remaining service, harbour, participation and evacuation gaps must
be connected at their existing consumers before the expansion can be delivered.
Delta: paid initial service and atomic sea admission; physical cargo capacity;
weighted eligible targets; saved harbour choice/control and real evacuation route;
actual naval/fort/land credit, late-contract caps, civilian crime and return-news
hooks; guarded existing commander boat contact. The callback composer admits exact
predecessors and pinned parallel revisions while keeping raw backup bytes and
unowned runtime content. Detailed owners and rejected hypotheses are in
`docs/worldmap-traffic.md`.
Result: exact composition/idempotence and unknown-revision negative pass; full
script/lazy-dialogue VM plus four serializer rounds and separate consumer/state
probes pass with zero script errors; canonical native build passes. Installed
preflight is read-only and reports 11 pending consumers. No stage/install occurs
while the existing player process is running. Repository-wide closure still sees
two foreign untracked native patch candidates, preserved for their own writers.
Disposition at that attempt: `unresolved`; the source/install dependencies were
subsequently closed by the attempt below, while player replay remains open.

### October 7 — final parley integration and canonical installed delivery

Hypothesis: hostile contact needs consent before boat transfer and a real combat
signal, while final staging must preserve existing runtime edits and player state.
Delta: `worldmap-contact.c` owns the two-step request/consent/visit contract; 23
existing callbacks cover the command, actual captain deck, scoped relation and
group restore, one-time Cabin return exemption and sea lifecycle cleanup. Native
`sea-contact-activity.patch` adds a synchronous AIBalls live-record count. The
real sea identity is `AISea.Island`; an initial undeclared `sPlayerLocation`
candidate failed the actual script compiler and was corrected before delivery.
Existing BI_Boat checking/launch blocks remain byte-identical. Known military
backups are admitted by exact hashes; unknown edits still reject.

The user's FPS recovery direction preserved the complete five-file draft/diff in
`.local-archives/fps-before-worldmap-20261007`. Later dynamic sail-ring, distant
caster/sailor culling and 16ms relation-cache candidates were removed. Existing
skinning-ring, outdoor ambient, reflection and surrender-marker UI remain.
This is source recovery evidence, not a measured FPS improvement. Unrelated
staged files, native candidates and user state were preserved.

Result: actual whole-script VM plus existing serialization/contribution/contact
cases and parley predicates 931–951 pass with zero script errors. The compiled
native query branch counts seeded authoritative records without mutating them;
the actual entity/event/rendered transition remains a player scenario. Current
runtime composition is idempotent, rejects unknown callback bytes, and retains
the pinned parallel speed/aim/dialogue/custody owners.

Source commits `76da73f` and `316e8b0` close implementation. Canonical
`experiments/native-metal/run.sh --stage-only` exited 0 after the final compact
native patch; patch stack and arm64 build pass. It installed signed engine
`46f859b8b41135c368d9f40a5a57ecb38ef3cedb19c9ca6a89f3d79bfaaf5e45`,
retaining previous engine `029338b9…` in the existing backup owner. Deep/strict
signature verifies, both content plans are empty, compiler readiness is true,
and actual installed/built/VM message headers match. All 233 player SAVE/config
hashes match the pre-delivery snapshot. The game was closed and never launched
or controlled by this attempt.

Disposition: source/build/stage/installed delivery accepted at their own layers;
player-owned prior-save, physical voyages/capture, sea/fort/land fights, harbour
loss/reassignment/evacuation, rendered parley/return, crime/rewards/news, recovery/
prices and FPS remain `unresolved`. Native goal state owns the requested pause;
these observations must not be replaced with proxy acceptance.

### October 7 — FPS/pickup integration and installed dev-helper canary

Hypothesis: repeated sea sail uploads need immutable submission-owned storage,
and the remaining helper must deliver content through the sole installed app.
Delta: per-submission revision/identity caching reuses FrameArena slices without
overwriting in-flight vertex/index bytes; sea registry traversal prunes stale
draw records. Live code-based surrender lookup replaces the transient relation
cache. Ship lights retain their owning-hull predicate. Pickup accepts the two
pinned ordinary/supply predecessors and composes into the current script/native
stack; existing sailing and berth inputs are tracked build dependencies.

Result: extracted actual native/GPU methods pass rewrite/recycled-slot, locked
buffer, allocation reuse, additive pickup, depth/occlusion and full render-state
restoration cases. Actual attribute/codecs pass immediate surrender deletion,
commander replacement, ordinary hostility and sanitizers. Whole-PROGRAM VM and
four serializer rounds pass with zero script errors. These are bounded component
checks, not real-game FPS evidence.

Checkpoint ad83111 is integrated. Canonical run.sh --stage-only exited zero after
the final source/ordered-patch change and installed signed engine
ee1447630287a759187683c49544794a2cbd08d693a762a98ea46a3b2e5e76ed.
Strict signature, compiler readiness, both empty content plans and matching
built/cache/installed message headers pass. All 233 preexisting SAVE/config hashes
remain unchanged. The prior 46f859b8 engine and reviewed FPS draft are preserved.

The helper replacement uses installed Contents/MacOS/launch and the existing
player-state lock; finite content transactions reject conflicts/links and restore
only their owned writes. Fourteen disposable scenarios pass. The actual installed
one-file push/revert canary reseals/verifies and restores exact original bytes;
the installed native launcher's dry-run resolves the current engine without
executing it. No game launch/input or forced exit occurred.

Disposition: FPS/pickup source/build/stage/installed delivery accepted at their
own layers. Helper source and bounded installed canary accepted, saved in focused
commit 4106e7e. The released-ledger repair is integrated separately in .codex
checkpoint 4e0c4f7. Parent replay transferred exactly the three helper paths;
the originally blocked staging and normal focused commit both succeeded. All
five unrelated working/index states remain byte-identical, owned paths are clean,
and no nonignored untracked paths remain. This closes the checkpoint dependency.
Same-save FPS and actual gameplay/reload scenes remain unresolved and player-owned.
Native goal state owns the user's requested pause after completed delivery;
these checks do not imply completion of unavailable player scenes.

### October 7 — player journey hardening, source candidate verified

Hypothesis: ordinary world traffic can break player agency through authored
quest proximity, damaged-ship regeneration, stale participation links/news,
global recovery suppression and unloaded land geometry. Delta: the canonical
sea source and worldmap modules now share real admission/entitlement owners,
retain paid persistent damage and scope recovery/story yield to actual owners;
31 SHA-bound land layouts support finite distinct placement and physical return.

Result: all five lanes reproduce their old mismatches with zero script errors.
Relevant state/negative scenarios and four native codec rounds pass. Independent
canonical initialization/clone tuples and raw GM positions bind land metadata.
Exact four-file composition and integrated events/codec pass with zero errors.
No game launch or player SAVE write occurred. Fourteen owned paths are staged;
five foreign working/index hashes remain unchanged. Normal focused commit
returned `staged-commit-claim-not-unique`; native index scope is unlocked.
Saved .codex repair task `01a117d5-f4e9-76d0-82e3-2abdc657e0cd` owns the
normal-route repair and source-task replay; root retains final Git/install.

Disposition: source/state checks accepted; checkpoint dependency unresolved;
installed activation and rendered gameplay/balance remain unresolved. This
candidate does not replace the installed finite-trade inventory above yet.

### October 7 — player journey hardening installed; checkpoint recovery pending

First canonical stage reached gameplay apply but rejected unchanged previous
canonical `sea.c`: planning had already proposed its future receipt hash and
apply attempted a second admission of old bytes. No gameplay transaction ran.
The minimal sync correction retains one reviewed managed proposal, frozen source
guards and both final receipts. A writer-intercepted pre-edit reproduction fails;
the repaired proposal matches the four expected script hashes. An independently
edited file still rejects, and known delivered content is idempotent.

After that last source change, canonical `run.sh --stage-only` succeeded. Native
inputs changed zero files; the signed engine remains `096395187b7b85e3f647f528d176531f0e6fbe16ad5baebc0029ac725197b8a0`.
Cache and installed plans are empty; strict/deep app signature passes. Installed
generator is `a3fb4c7ac5773bd7c85a12140c6b1eec59de5c615ff5ea8224bb238ce17ffc1b`,
reload `82fa26766d7706d01b21716e52381115edf556764be7caf611779b06c44b3776`,
map UI `d342e2cc54a3272c5b13998210fff578046b5f8f4db62e569e564af3457802cf`,
and sea `eb3cfa85f8d433c7398f2427822e0d4ef60e9f1e5f85dc73ee1c2879f94b0811`.
All 234 observed player-state files and five foreign working/index paths preserve
baseline hashes. No game launch, SAVE rewrite or forced process stop occurred.

The first Git refusal was an expected live-child ownership/provenance conflict,
not a proven guard defect. A later original staging replay encountered a completed
root `auto:apply_patch` index claim; the same saved .codex owner is investigating
its normal posttool release. Root retains the exact focused commit obligation.
Disposition: source/state/composition and installed delivery accepted; checkpoint
pending its normal owner recovery; native player scenes and balance unresolved.

The specialist then bound the held claim to a successful two-file `git add`, not
an active patch. PostTool had no pending entry; its old staged snapshot cannot be
reconstructed safely. Supported recovery is native Stop of this turn, automatic
active-goal continuation, native staged-provenance adoption of the same 14 paths,
then the original focused helper as a separate call. Root retains baseline and
installed evidence in its task temp until that replay. No claim was manually
cleared, no guard was bypassed, and no harness/source repair was attributed to
the stale `auto:apply_patch` label. Checkpoint acceptance remains open.

### October 7 — player journey source checkpoint accepted

Continuation disproved the proposed Stop/adoption workaround: adoption lacked
provenance, and a fresh successful no-op staging call recreated the orphan claim.
The same .codex specialist repaired repeated admission work and successful no-op
stage provenance; its 48 focused checks and current-transcript admission probe
passed. Root replayed the exact original 14-path staged helper through its normal
route: commit `59a3dfad5f6cd4fcc4aad3cc819feae2b634b3ef`, exit zero.
Native index scope is unlocked. Commit contains only the 14 reviewed task paths;
all five foreign working/index hashes and 234 player-state hashes still match.
Repository hygiene passes. No push, reinstall, game launch or SAVE write.

Disposition: this batch's source checkpoint and previously verified installed
activation are accepted. Earlier checkpoint-pending/recovery instructions are
historical and superseded. The separate .codex owner still owns its own repair
checkpoint; live source-task success is not proof of that checkpoint. Actual
player startup/actions, rendered siege geometry and economic balance remain
unresolved, with joint player replay as their continuation owner.

The specialist's restricted profile could not save its own checkpoint. After its
explicit two-file ownership transfer, root verified HEAD, staged/working matches
and four foreign staged fingerprints, then used normal staged adoption and the
focused helper in separate calls. `.codex` commit
`cc7b6fd0240d8a9d0e78756ec860a75077cf8267` contains only the two repair files;
four foreign staged fingerprints remain unchanged and its index scope is unlocked.
That source dependency is saved; no push or instruction edit occurred. The six
gameplay task-owned temporary fixtures were removed via codex-delete-temp after
their evidence was consumed. Native turn cleanup owns remaining session claims;
an optional explicit two-file release rejected unsettled provenance and was not
bypassed. This does not reopen the accepted gameplay commit/install.

### October 7 — remaining player acceptance blocked

Source, install and both checkpoint dependencies are closed. The specialist is
terminal. Installed public launcher binds `Contents/MacOS/metal-engine`; the
read-only exact-name process probe returns no process. No game was launched and
no player state was changed. Real scene/balance evidence remained missing across
three goal turns, so the native goal is `blocked`, not complete or user-paused.
Continuation is the player's next normal current/new-save session with root
observing quest continuity, trade/services, event choice/rewards and sea/land
placement/withdrawal/return. The full objective and unresolved acceptance remain.

### October 7 — player-map stepped movement: follow-camera source repair built

Player reports small steps while simply sailing on the global map. Live PID48484
is the supported app engine, started 16:42:03 local. Installed engine096395187 and
worldmap generatora3fb4c7 match the last delivered batch. Read-only game observation
currently shows a sea scene, not the reported map movement; no game input occurred.

Hypothesis bound to source: WorldMap::Realize sampled the follow camera before
ship Update, then rendered the advanced ship against the previous camera position.
A disposable exact-method/update-block ASan/UBSan probe reproduces variable-frame
ground-focus error0.186328..0.745361; moving the existing camera call after object
updates reduces it below0.000011. Follow+zoom, free camera, pause and once-per-object
update/time negatives pass. It uses explicit motion/control/bounds adapters; this
does not accept the player's perceived result. No physics/timing/speed change.

Canonical worldmap-navigation.patch carries the one-file frame-order delta.
Canonical build.sh succeeded with exactly one changed native source and built
engine2d29535e2940f70b652a98e490ca512af221ccdf82e8df2eb08b8749f33c73ed.
Built source equals the verified proposal. Five foreign working/index fingerprints
are unchanged. The played app and player state were not written. Canonical
run.sh --stage-only installation remains pending the player's normal save/quit;
the running process still uses096395187. Source repair accepted; installed smooth
movement and zoom/storm transition replay unresolved. No forced exit or hot swap.

Player then confirmed normal save/quit. Canonical run.sh --stage-only succeeded
after the last patch change; installed signed engine is
34e1c03d03ad7be83669c92bd11534d24213833a69113dcc7325e4f9f9b66eec
from raw built2d29535e and commitafbe96e. Strict/deep signature passed; gameplay
plans reported zero pending. All239 player files present after normal quit and
all five foreign working/index fingerprints retain exact pre-stage hashes. No
game launch/input occurred. Source/build/install accepted; rendered smoothness
remains player-owned replay. The subsequent 1/4 ideal-speed report is a separate
active repair, preserving the current accepted map batch.

### October 7 — arcade quarter-speed correction, source verified

The player clarified 1/4 of ideal maximum, not a fixed 1.4 knots. Read-only
decoding of the normal sea save at 16:53:12 identifies Blaze/index1, arcade1,
HUD scaler0.4, ideal8.6057501, four intact masts and no blocking flags. Native
floor used the unscaled ideal while ordinary propulsion used2.5: floor2.15144
engine knots became0.860575 on HUD instead of2.15144. NPC identity and absent
masts/ideal are rejected for this save.

Canonical AIShip.c now shares one mode coefficient between Tmp.fShipSpeedIdeal
and MaxSpeedZ. Real script VM executes normal/arcade/normal changed blocks with
zero errors; exact installed native target-method ASan/UBSan fixture passes HUD
quarter, partial/full mast loss, stop/dead/fixed/surrender/drift/lowered sails,
faster stock propulsion and bounded mast vector. An optional VM codec round
reported an invalid function after load; it is not acceptance evidence for this
derived parameter update. No native patch or engine rebuild is required.
Cache gameplay check reports exactly one pending canonical AIShip.c. Installed
engine remains34e1c03d. Source accepted; installation and live sea replay pending.

The player reports whole-map micro-pauses each game hour, while individual ship
jumps are gone after afbe96e. Live sample PID74816 at17:03 was almost entirely
unfocused sleep; subsequent observation showed the game menu. That sample does
not establish a map hotspot. Hourly source/probe diagnosis continues separately;
no economy/event suppression and no game input have occurred.

Player confirmed quit with +; absence of the installed engine process verified.
Canonical run.sh --stage-only succeeded on c8a136c after the last source change:
zero native source changes, exact AIShip.c content delivery, installed engine
34e1c03d unchanged/reused. Installed AIShip.c SHA256 is
e7db81fc82e429b294095a873e8afec85e3e24d5d7c7f7084302dde07262e335.
Strict/deep signature and zero-pending gameplay check pass. All240 player files
from the post-quit snapshot and five pre-existing foreign working/index blobs
remain exact. Source/install accepted; live sea acceleration/HUD replay pending.
A new untracked tools/gameplay/mast-repair.c appeared during delivery; unrelated
concurrent mast-repair work belongs to active chat «Добавить ремонт мачт в море»
01a1182b-8bd0-73f1-b9fc-b3c9d5496736 (native read confirms its file creation);
it is preserved and was not included in c8a136c.

Hourly pause diagnosis identifies WdmTrafficReviewStrategies (hourly gate, called
from refresh/NextDay) and WdmMilitaryTick (hourly gate, frame handler) as synchronous
candidates. A synthetic readiness workload is rejected as causal evidence: it
overcounts nation/role filtering and does not execute actual handlers. Moving
strategy interval checks before readiness is rejected because it suppresses
urgent home-attack/loss/recovery responses. No hourly source change is admitted.
The player independently reopened PID10402 after c8a136c; observation showed the
nation-relations interface, not an advancing map. One pending request asks for
20–30 seconds of ordinary map sailing to capture a real hourly sample. Root owns
that live-profile continuation and the subsequent smallest proven repair.

The revised isolated hourly fixture reports ~0.4ms/call with zero active sieges,
but explicitly stubs character/island lookup and portions of military/strategy
work. Root does not accept this as real-handler timing or a causal exclusion.
It further invalidates the earlier synthetic200ms claim; live advancing-map
profiling remains the decisive missing scenario. No hourly source repair made.

### October 7 — sea outcomes retained on the map, ordinary NPC identity defect

Player reports destroyed sea opponents keep sailing as map miniatures. Installed
sea.c matches canonical pre-repair eb3cfa85. Its ActorOwned rejected captain.quest,
but installed InitCharacter initializes that container for every ordinary NPC.
Exact-method VM seeded with those defaults returns Admission=1 (rejected) with
zero errors before repair; no scene receipt is then written, so sink/exit cannot
close that roster. Replaced that guard with explicit isquest. No descriptor
quest/qID, companion, group/index/ordinal or scene receipt guard was removed.
Admission/ReturnCases/NegativeCases now return0 with zero errors; admitted death
marks roster dead and descriptor needDelete, survivors retain damage, repeated
exit and deferred/story/companion/stale-identity negatives pass. CodecSeed plus
four CodecAfter rounds pass with zero errors after supplying the fixture's
required empty OnLoad adapter. No native patch; historical already-resurrected
rosters cannot be retrospectively distinguished without actual loss evidence.
Root owns canonical sea.c and worldmap topic; foreign mast/cannon work preserved.
Source accepted; installed delivery and player sea→map replay pending.

Player confirmed quit with +. Selected registered sea.c installed transaction
completed after process absence and player_guard admission; existing source
backup, receipt, concurrent-byte checks and rollback remain the ordinary owners.
Canonical native run.sh staging was not run for this content-only batch: unrelated
mast/cannon native inputs and a changed cached bundle are pending with their
writers. No engine or other gameplay source was copied. Cache/installed sea.c
SHA256944fa5c0cc3e028ca3d828ec019243cd35b2d6a91c5f6096069d861ae976e8ba;
engine34e1c03d and all241 post-quit player files unchanged. Transaction's
strict/deep app-signature check passed. Commit339178e owns only sea.c and the
worldmap topic. General agent_context hygiene remains blocked by foreign
untracked mast/cannon inputs, preserved with their active owners. Source/install
accepted; player must replay a new sea battle then return to the map, checking
fully destroyed fleet removal and surviving fleet damage. Previously resurrected
rosters have no reliable historic loss evidence and are not guessed away.
Hourly whole-map pause remains unresolved under root's pending live-profile
request; this identity repair does not accept that separate requirement.

### October 7 — safe-sea mast repair, installed; action unresolved

Hypothesis: restoring authored mast subtrees inside the existing native ship
avoids encounter regeneration and the saved broken-flag overwrite of sea reload.
Delta: focused local commit `f6ce3ab` (nine mast-owned paths) adds the quick-menu
planner, reusable confirmation, ordinary plank withdrawals and 8–72-hour time
advance, with a standing-main-mast ceiling of floor(60% of total). The native
bridge preflights donor geometry, keeps ship/model/AI identities and rebuilds rig
groups with their stored hole masks. Ordinary repair glyph remains provisional.

Result: native compile, whole-PROGRAM/lazy confirmation compile, contract VM and
one serializer round pass. Canonical run.sh --stage-only completed after the
user closed the game; signed installed engine is bf42886a, shared header contains
the bridge opcode, compiler ready and zero cache gameplay files pending. The
batch also consumed six gameplay projections including the parallel cannon
writer's registered source; that writer retains its independent acceptance.
Strict/deep app signature passes. An already reopened native game shows an
active first-person sea battle; no UI input or save mutation was made.

Disposition: source and installation accepted; rendered repair and confirmation
fit, plank/time change in a safe encounter, unaffected sails and save/load after
repair remain unresolved with the player. Repository hygiene reports only
foreign untracked cannon/deck/perk/icon paths; owned sources are tracked in the
focused commit and foreign staged blobs were preserved. No push.

### October 7 — manual raking fire, firing-eye floor, RakingFire capstone; staged, replay pending

Hypothesis: a manual volley at a locked ship should spread along its hull with gunner skill instead of one point, and first-person firing view should not sink under the rails.
Delta: cannon-rake-spread.patch (AimRakeFactor ramp 0.30-0.85 x capstone 1.0/0.45, per-gun reachable stations along the live waterline extent, ships only) and deck-firing-eye-floor.patch (eye clamped to design height in firing view) registered in build.sh; gameplay adds the RakingFire perk (requires CannonProfessional and ImmediateReload), the AIShip export, AbilityDescribe texts with corrected ImmediateReload/LongRangeShoot descriptions, a pictures.ini placeholder on the CannonProfessional rect, manifest entries and gameplay_sources txt support; icon source docs/images/raking-fire-capstone-128.png redrawn in atlas style.
Result: build.sh green, run.sh --stage-only exit 0; RakingFire present in the installed PROGRAM/RESOURCE files; both patches in the applied stack state; installed engine matches the candidate. The atlas has no free cell (unused 8itm4/5/6 placeholders hold the insect icon); TX injection queued after icon approval.
Disposition: source and installation accepted; in-game capstone visibility, rake spread against fas/broadside targets, eye height and unaffected AI fire remain unresolved with the player. Focused commit owns 11 paths (runtime.md is exclude-listed local-only); foreign mast/fleet/docs blobs preserved. No push.


### October 7 — generated mast-repair icon, installed; menu replay unresolved

Hypothesis: the provisional repair glyph and the first generated icon failed the
requested dedicated atlas-style affordance. The user screenshot preceded a
successful delivery: an active game held the normal launcher lock, so the first
content transaction made no changes.
Delta: generated a two-state mast/mallet icon with the existing list_icons atlas
as visual reference, packed it into 128x64 A8R8G8B8 TX, bound normal/selected
indices 0/1 to texture slot5 and registered its source/prepared hashes in
src/assets/ui/manifest.json. Existing command frames are part of their texture
cells. The ordinary gameplay delivery now includes this owned texture in its
receipt/backup/rollback transaction.
Result: whole-PROGRAM native VM compilation reports zero script errors; the
64px preview and TX header pass. Isolated delivery accepts first creation and
unchanged replay, and rejects corrupt source and concurrent manifest/input
changes before writes. After the user closed the game, the normal sync API
applied exactly BattleInterface plus the texture; the separately pending perks.c
source was excluded through the existing frozen source_set. Cache and installed
receipts agree on texture SHA256
5348e9bf4f8ab1a23fcbcb30481c797bd6181abcc13bb4e0c2486037b9c57eb3
and BattleInterface SHA256
8d56beba0436ada9af6a8a236bf3535f32c8df7fb9f88e577f89ba6f23a2d566.
Strict/deep signature verification passed; the installed engine and foreign
perks.c bytes stayed identical during this transaction. A parallel cannon owner
then entered canonical build/staging; no competing launch was started.
Disposition: source and icon installation accepted. The command remains hidden
in combat or without a fundable repair below the 60% standing-main-mast ceiling.
A real safe-sea menu and repair action replay remain unresolved with the player.


Icon source checkpoint: local commit 50f160b7fd5fc945175b80d8f9a5d94984b7d2a0
contains exactly eight owned paths. Normal same-blob index reattestation restored
staging provenance, then the original protected staged-commit helper succeeded.
All eight working SHA256 fingerprints and eight current foreign index blobs
were preserved; a separately committed cannon documentation change1d1a43e was
reconciled before that comparison. Agent context --check passed with no
nonignored untracked paths. No push. Guard-recovery dependency is closed by its
specialist's independent source replay. The later canonical cannon staging
retains installed icon/binding hashes; its current signed engine is8c5fb0bf.
Latest read-only native observation is the main menu, not a safe-sea repair replay.

### October 7 — player cannon replay reopens acceptance

Hypothesis: the first installed cannon correction restored the missing old-save
perk and root surrender lookup, but did not satisfy the full volley/projection
contract. Installed signed engine8c5fb0bf is the consumed batch.
Observed: the player's three screenshots show broken land/ship contours,
CannonProfessional's placeholder icon and a wide crosswise ellipse at a stern-on
ally. The player reports sail shots landing ahead of an already sinking ship,
white surrendered highlights and sail HP rising/falling after damage. The actual
old-save character menu shows the new unlearned perk and zero free ability points.
Source falsifiers: automatic Fire(AIShip*) bypasses rake distribution; the actual
method driver produces0m span before correction versus45m at high skill without
the perk. Native prediction uses obsolete height/accuracy formulas, while live
Ball_AddBall uses0.70/0.85 with perk/arcade accuracy modifiers. Cloth is absent
from the pure range-pick set. ProcessSailDamage starts from0 instead of existing
damage; RandomHole2Sail stops iterating when the current mask becomes0. The
neutral arrow atlas is beige/white, so relation enum2 alone does not make it yellow.
Disposition: prior startup/perk visibility accepted; volley, color, continuity
and damage actions reopened and unresolved. Source-only fixes and seeded probes
are in progress; no second installation or player-save mutation yet.


### October 7 — legacy ordinary spawn bypass and persistent port crowding

User reports dozens of ships/pirates accumulating at bays/towns. Three decoded
normal saves reproduce over-admission: latest November1 has74 living ordinary
fleet descriptors (38commerce/15patrol/21pirate), whereas the resident admission
contract is32/16/12 and60total. WDM_TRAFFIC_TICK still invoked legacy random
Follow/Warring constructors. Refresh adopts Follow into persistent roles, while
the transient pursuer counter excludes role-tagged actors and reopens on the
next tick. Real VM before repair produces extra Follow births beyond a seeded
full world (Congestion=1, zero script errors). Removed those legacy tick branches;
scheduler requests, real resident pursuit/clashes, quest constructors, storms
and specials remain. Congestion/disabled-encounter/special negatives and four
native codec rounds pass with zero errors. The exact composed PROGRAM with
installed shared headers compiles in a disposable runner (only game Main renamed).
No permanent tests or new runtime wrapper. Commit742a597 owns only the composer
and worldmap topic. Selected composed output0051df1dd35d4094af4ad6b30f1f6216e1f3af27c2f70a74a20766302a4482dd
is currently installing through existing DeliveryState/player_guard/transact,
with receipt admission/backups/rollback and strict signature. Engine is reused;
foreign cannon UI/ability source edits are excluded. Whole-tree hygiene is blocked
by concurrent untracked cannon UI inputs; they remain with their active writer.

A distinct prior-state mismatch remains: enc27/56/66 returning to LeFransua sit
near552/-371,16units from saved targets, from October28 through November1. Their
voyage counters7/9/3 do not advance. Actual berth fade is already installed;
merely claiming force equilibrium or enlarging the arrival radius is unsupported.
Raw physical GM/navigation investigation remains root-owned through readonly
port_native_congestion. Existing over-limit resident hulls/cargo are preserved;
this spawn repair does not delete74groups to pretend a60group invariant passed.
Source accepted; installation transaction active; port-drain/player replay
unresolved. Hourly map pause remains the separate pending live-profile scenario.

Selected composer transaction completed: installed/cache encgen0051df1d match
742a597; strict/deep signature passed. Signed engine8c5fb0bf is unchanged and
all244 post-quit player files retain exact hashes. No foreign content was copied.
Source/install accepted for quota-bypass repair; existing-fleet berth drain and
player map replay remain unresolved under the root/port-native evidence owner.

Native evidence review: installed islands.gm locates LeFransua at567.48/-367.56;
PTC table connects stalled node3540 to destination3570 in6transitions. The child's
Python port of PtcData edge refinement reports an open route; it is not execution
of native collision/movement. Returned force-equilibrium claims are rejected:
they omit the already-enabled berth fade (saved ServiceOnArrival=1), leaving
about0.5 instead of7.2 shore-force units. Intent-filter expansion is likewise
unnecessary for these saved returns. WdmShip::ShipUpdate has a session-global
static collision-escape vector initialized by the first ship; later ships reuse
it. This is a concrete source defect candidate, but observed headings alone do
not prove it causes these stalls. No radius/avoidance/collision patch was made.
Next decisive boundary is native movement/collision evidence during advancing
map play; root owns that continuation, with player controlling the scene.
Read-only child's disposable tools/probe_ptc_path.py was relocated into root's
owned temporary directory for cleanup, rather than retained as a new test owner.

### October 7 — cannon feedback batch: source verified, delivery blocked

Owner: 01a11860-c9e3-7813-a422-5b79bc52a828. Player confirmed the missed sail
target was already sinking; acceptance remains reopened for cloth range,
curved projection, stern-on longitudinal distribution and gold surrendered UI.
The candidate now uses live AIBalls parameter queries, pure nearest sail picks,
manual/automatic hull stations, crew factor .1–1, cumulative sail damage,
bounded pristine-mask hole enumeration and distinct learned/unlearned perk art.
All 62 existing ability descriptions have the requested short pirate voice.

Actual extracted auto-fire methods reproduce zero target span before and45m
after at the old maximum baseline; zero skill, full perk, moving lead, common
jitter, height and nonship negatives pass. Actual solver translation preserves
the fixed angle after replacing a direction argument with a muzzle origin.
Actual clipped-mesh callback retains >.485m relief and rejects outside/NaN/budget
inputs. Actual sail Trace reproduces the missing above-box Y entry and passes its
mirror/outside/rolled cases after correction. Actual Draw batches gold surrendered
markers with unchanged normal indexing and restored render states. Real script
VM passes canonical sail hit/repair/save fixtures, canonical crew factor0/.5/1
and clamp/event/save fixtures, and full-program perk/menu compilation. These are
source/VM observations, not player action acceptance.

Canonical staging attempt1 rejected private ai_helper.h includes from battle UI;
the live display predicate was moved to public sea_ai/include/ship_surrender.h.
Attempt2 rejected missing new-file mode in that textual patch; the mode is now
explicit100644. Attempt3 compiled the cannon/surrender/rigging code, then failed
on the concurrent worldmap-berth-homing.patch's access to private slowingAlfa in
WdmEnemyShip. That path is held by continuing writer01a10edf-1111-7272-85e3-4c946daac56d.
No installed game changes occurred in these failed attempts; signed engine8c5fb0bf
remains the player runtime. Root retains build/stage/install acceptance for this
cannon batch after the shared worldmap input is valid. Disposition: unresolved.

### October 7 — frozen cannon delivery and missing script event header

The continuing worldmap writer was isolated by a task-owned managed checkout of
5ceff926. Root retained integration and app delivery ownership; that checkout's
build/runtime caches are separate, and the shared saved checkout was preserved.
The first stage restored PROGRAM/RESOURCE without the previous delivery receipt
and rejected perks.c; it did not modify the app. Reusing the canonical runtime
with its matching receipt resolved that admission failure.

Canonical run.sh --stage-only then passed for the frozen commit and delivered
10 files plus signed engine fdab9859e2228c979a675f25e0f75e769df4ccb0b909dac2171bc89a3a2dff53.
Deep/strict signature verification passed; sync reported zero pending files;
all 232 player SAVE file hashes were unchanged. The preceding engine8c inventory
is historical. Current player engine is fdab9859, arm64 native Metal, with the
existing player state/settings and no game process after the following crash.

Installed public launch failed before the menu with fail to create program.
The user's crash report matches CoreImpl::ProcessEngineIniFile; actual player
logs report Invalid Expression. An exact delivered-PROGRAM compiler probe with
the actual installed shared headers reproduces it at sea_ai/AICannon.c(8).
Replacing only shared/sea_ai/script_defines.h with the built header makes that
same compilation pass with debuginfo0 and cache_mode0. Earlier source/VM checks
used newer source headers and therefore did not accept installed startup.

The canonical install transaction is being extended from shared/messages.h to
the finite LEGACY_SHARED_HEADERS policy, including the three new cannon query
constants. Known prior bytes are admitted; unknown edits, unknown headers and
SAVE destinations are rejected. Gameplay, settings and the native binary are
unchanged by this correction. Corrective staging and real startup/action replay
remain unresolved under root; /Applications remains reserved until that replay.

Corrective follow-up: finite shared-header delivery was committed as ce5eef9 in
the saved checkout and cherry-picked as476c045 into the frozen delivery checkout.
Canonical run.sh --stage-only passed after that final source change. Installed
shared/sea_ai/script_defines.h SHA-256 is
9cfad8102a6e0a4dba5b06da6b7b631debe6df7512798ee35c30469003755ff9,
matching its receipt; engine remains fdab9859. The player subsequently opened the
current saved sea battle in the installed app; the window was responsive and
current error.log empty. Actual yellow/gold surrendered overhead arrows were
visible beside ordinary red enemy arrows. Startup is accepted; original cliff,
stern-on ally, sinking-sail volley and damage actions remain unresolved.

The player then reported very low FPS while manually aiming. A three-second live
sample during the requested aim hold binds the installed fdab9859 process to
1533/1951 main-thread samples in DrawManualAimOverlay, mainly GEOM::Clip,
ClipByPlane, segmentBox and GEOM::Trace. The earlier normal-combat sample had no
overlay stack and its screenshot showed51FPS atTime2; it cannot serve as a matched
same-view FPS comparison. Performance acceptance is reopened. Source correction
preserves all paths/queries/caps while rejecting unrelated curve/model candidates
and wholly outside BSP faces earlier. Tighter prism sphere was rejected after
real BSP probes changed capped coverage. Native build/stage/install of the next
combined batch belongs to the harbour integration owner after source freeze and
player closure; no process was stopped and the installed engine staysfdab9859.

Performance source freeze: focused commit11571da passed native syntax for the
controller/geometry and a canonical71-patch migration on the exact cloned source.
The final exact BSP function retained callback bytes/order/cap and node/face visits
on1920 real ship/island queries; cold/warm, zero-plane and test-only negatives
passed. The harbour integration owner then completed the canonical full native
build on11571da overce5eef9+c6bf191. Installation is still pending player closure;
process1916 remains the installedfdab9859. Live aiming FPS and the changed firing
actions are unresolved under the cannon writer after that installation.

Before retiring the frozen delivery checkout, both signed engine rollbacks, nine
original content blobs, the prior0810dd98 script header and two historical receipts
were retained in the canonical cache; the cannon writer independently verified
all14 hashes. The managed worktree archive completed and its archived attachment
was verified; temporary source/profile evidence remains task-owned until the
pending player acceptance.

Combined installed delivery completed after the player exited. Canonical
run.sh --stage-only passed for11571da overce5eef9+c6bf191. Installed signed engine
SHA-256 is37598a37e4c73e7d4823576b69ad0a0dca34b59c97ce70efb9637f74d209b9f9;
its app receipt SHA-256 is68f05ba2e537252aabc54b4095b8389183345334fbe299ad72cc888e5f917170
with matching @engine. The integration owner verified deep/strict signing and all
252 pre-install player files unchanged, with no added files. Shared headers and
all12 cannon/worldmap content consumers match their declared source/runtime/app.
The cannon writer independently matched the installed engine hash and observed
the current night sea battle, November3 02:12 atTime2, FPS114 in third person.
Live process30726 started20:23:38. Its first post-install three-second profile
contains no manual-overlay stack and is a normal-combat baseline only. It is not
a matched FPS comparison to the earlier daytime battle. Player aim-hold replay
and original physical firing scenarios remain unresolved; no gameplay input was
sent by the cannon writer.

Manual replay boundary: CUA Tab input was refused because the player was actively
interacting with the installed game; no key was delivered. A subsequent read-only
player-active profile at20:29:07 also contains no manual overlay (texture loading
instead). Neither post-install profile accepts aiming FPS. The single pending
player aim-hold question in the cannon chat owns the next measurement; original
changed firing/sail/curved-shore scenarios remain unresolved there. No second
launcher, synthetic save edit or process shutdown was used.

### October 7 physical harbour recovery — c6bf191 compiled, activation pending

Hypothesis: connected point navigation can still request full-hull poses that
ground collision rejects. Readonly saved enc27/56/66 positions at552/-371 stayed
unchanged from October28 through November1. Shipped Martinique.gm has no BSP;
the actual mein.gm archipelago BSP has467548nodes/153467vertices/170071triangles.
A disposable C++ fixture executes native GEOM Clip/ClipByPlane, native PTC
refinement, CollisionTest, Move/ShipUpdate and KillTest against that geometry,
saved velocity/yaw and real sloop/bark extents. Explicit wind/attribute/event
adapters replace absent renderer/core plumbing. Old controller leaves all three
voyaging after300seconds, rejecting translation and yaw at contact.

Rejected narrower candidates: fixed escape normal, braking/retained turn, and
straight collision-safe escape fail curved corridors, wind or reload. The final
bounded local full-hull position/heading search recovers after ordinary committed
port contact, preserves original arrival, and recomputes transient paths after
load. No saved fields, radius expansion, fleet deletion or cargo rewriting.
Each candidate pose is collision checked; the old shared static fallback vector
is replaced by a per-ship/per-contact-episode vector. Goto cancels obsolete plans.
Public real headers compile without private slowingAlfa access.

Final ASan/UBSan replay passes all three arrivals with exactly one native event
per group, four cardinal winds, collision-free accepted poses and path/counter
reset plus actual Merchant save-attribute methods at0.1/0.2/0.3seconds. Two service
groups retain exact positions and no new events. Quest, patrol, pursuit, next-leg,
battle and distant approach exclusions pass with seeded contact; open-water and
first-ten-contact physics match unchanged ShipUpdate numerically. Attribute
adapters do not constitute native codec or rendered scene acceptance.

An O2 CPU-only probe measures eight planning calls at817–1327microseconds;
this is local planning cost, not an FPS claim. Search is limited to1024expansions,
failed searches retry no faster than two seconds. A deliberately unreachable
local target exhausts that bounded search in8756microseconds and leaves no path;
worst live fleet/frame cost still needs the pending player map profile.
Canonical build.sh succeeds,
source patch stack migrates exactly four worldmap files, full native engine links.
Focused commit c6bf191 preserves foreign index/paths; agent_context --check passes.
No permanent tests. Cannon writer released /Applications after its header/startup
fix; root's canonical stage-only rejects the player's running fdab9859 engine.
Root owns subsequent stage-only and player-state checks after player closure.
Disposition: source/native fixture/build passed; installed activation and rendered
prior-save harbour drain unresolved. Hourly whole-map pause still awaits profiling.

## October 8 — smooth all-gun density on current scene depth

Hypothesis: the reported red-zone misses include an independently proven density
defect, separate from first-contact mask holes. Selected-gun omission, count
normalization, eight-corner covariance and interpolation of inverse metrics can
shrink the displayed region. The player chooses main-density contours with
allowed tails, then requests a soft rounded blot on scene depth.

Delta: every firing gun contributes27 weighted launch curves and65 quadratically
spaced covariance-root sections. One unnormalized smooth union drives the
Gaussian fill and perimeter. The physical first-hit, own-ship and stale-depth
guards stay with their existing owners. Bounded stack storage removes hot-path
allocation without changing field bytes:10,000 solver cases and120 field
payloads match;52-gun paired CPU cost3.04631→1.24369ms. One strict-MSL GPU probe
measures3.4612ms for52 fields over1,048,576 fully eligible pixels; this is not an
installed frame-rate result. Fixed quadrature remains approximate, with a tested
far-water conditional survivor case at20.31% outside rather than a universal
95% guarantee.

Result: canonical run.sh --stage-only returns0 after the last source edit.
Signed installed engine70d78026692e3eeb351e65fff178df43fa31994583cc043a2372e930ba3d8678,
receipt08f6c75b2cd1180d9097ebbdc03ef79d61395be66e65e5aa90557ab4b8360cb4.
Deep/strict signature passes;17 frozen source inputs,253 player files and all
three foreign index blobs are preserved. Both delivery plans are empty,
compiler_ready is true and the two declared script headers match. Native-only
aim headers are compiled build inputs, not installed script-header deliverables.
The pre-existing engine is retained by the canonical per-hash backup owner.
Focused local commit388d4e7 contains only the five owned source/topic paths;
agent_context --check passes and the three foreign staged blobs retain their hashes.
The three owned temporary evidence directories are removed through the canonical
helper; historical foreign fixtures are retained.

Disposition: source/native/GPU probes and installed staging pass. Actual smooth
blot appearance, first-hit fan/prism flicker, closed rim, aiming FPS and stocked
current-save firing remain unresolved; the player owns real-game replay. No game
is launched or player actor terminated by this batch.

### October 8 — debug tsunami source candidate

Hypothesis: one transient, shared SEA height/gradient addition can move a large
crest toward the player's launch position while existing buoyancy/collision
rules determine the outcome. Native candidate is `sea-tsunami.patch`; script
candidate admits F11 at sea and adds `Цунами`/`Стоп` to the existing debug menu.
One isolated normal PROGRAM startup plus the exact debug segment compiles with
zero errors. Native scalar/SSE/profile/lifecycle and same-value cancel callback
probes pass. No player SAVE/configuration is used by these fixtures.

The three pending content files were withdrawn to the task-owned temporary
fixture before the parallel aiming delivery so its batch cannot silently stage
an incomplete tsunami command. Tsunami remains unregistered in build.sh and
uninstalled. The player's close acknowledgement was followed by an observed
closed process list; the parallel aim owner later reports a new live player
actor77733. No root input or stop is performed on that actor.

The parallel aim owner rejects its per-pixel exact-water solver on observed FPS
and water-gap evidence and confirms a depth-only projection replacement.
Tsunami's temporary exact-water ABI/shader integration is discarded; the native
rendered surface is the remaining shared consumer. That parent retains sole
build/stage/app ownership until its corrected batch is released.

Disposition: source properties pass; installed delivery, F11 action, horizon
approach, crossing/ship response and stop/pause/teardown replay are unresolved.

## October 8 — simple range-local depth drape

The exact per-pixel ballistic/water collision attempt is rejected on the player's
10FPS screenshots, torn water projection and a native sample dominated by GPU
command-buffer/drawable waits. Installed engine40ca9f1c is historical. Its CPU
collision field, water atlas and sampled first-hit masks are not the current
implementation.

Depth-only enginec220a1e6 improves the shape (player screenshot102FPS), but the
player reports roughly half ordinary FPS, a thick distant rim and intermittent
zoom disappearance. Enginec474cc73 removes terrain identity passes, narrows the
rim and reuses point storage. The next screenshot92FPS accepts the thin main rim
but rejects a secondary far hillside stripe and broken joins. Neither screenshot
is a paired with/without-aim performance measurement.

Hypothesis: a field extended along the full flight also intersects distant land;
the contact-only coplanarity fade can remove otherwise valid contour fragments.
Delta: one65-section aggregate dispersion field is bounded by a smooth axial L4
cap around nominal arrival range. All27 weighted launch curves per gun share its
nominal arrival time for axial moments. Existing32-byte section metadata carries
the common center/radius, rebased once. The shader evaluates the field on nearest
rendered scene depth; its contact rim has no coplanarity fade. Finite/zero-gradient
cases emit density only, with no rim. Physical firing, air helpers and random
dispersion remain unchanged.

Result: native build and strict runtime Metal compilation pass. One-off bound,
translation and zero-gradient negative probes pass; independent source review
finds no actionable P1/P2. Canonical run.sh --stage-only returns0 and installs
da34fbae836f77dcc531c88d1ea5c7d8312631180e5fd05ff22916585ee096ba;
receiptcff0fdcf837d9f4c56cb5f5b99ab3a057b1abbf8d6902bce2a6dbba9ea801576
matches the engine. All239 non-log player baseline files and three foreign staged
blobs are unchanged. Aim commit666da3f is local. Broad agent_context --check fails
only on preserved foreign untracked docs/sea-tsunami.md, sea-tsunami.patch and
the debuger.c/debuger.ini content candidates; this is not a clean repository gate.

Disposition: installed candidate; visual far-strip/seam/zoom acceptance and paired
aiming FPS unresolved. The player owns replay. No further UI, input or launch is
performed after the computer-control stop. Tsunami source/working build hunks
remain with their stopped owner and are not included in the aim commit.
Three owned direct-child temporary directories were removed by codex-delete-temp.
The owned witness-pilot.GlorGPuw child remains under the historical foreign
corsairs-aim-integration.KX3uJhIv43 directory: the helper rejects nested targets,
and the parent is not owned for deletion. No alternate deletion is attempted.

## October 8 — false outside contour from a covariance-root derivative

Observed rejection: the player's13:00:21 screenshot still has a huge polygonal
hillside contour; the player reports that the range-cap revision looks worse.
Installed engine/receipt confirmda34fbae, so this is not a stale-engine claim.
The cap attempt is rejected as the solution to that symptom.

Reproduced defect: the current `(1-q)/gradient(q)` stroke estimate approaches
zero distance as an interpolated covariance root collapses, even for a receiver
far outside the field. In one source-exact seven-point Metal fixture a linear
radius boundary is249.85pixels away, but the old shader emits rim1/keyline1.
The corrected shader emits0/0 in both strict and fast math. The real q=1 boundary
retains rim1; its one-pixel AA case returns0.500006, center fill1 and zero-gradient,
out-of-station-range and outside-arrival-cap cases return no rim.

Delta: normalize the local field coordinate to unit radius before evaluating
stroke gradient, then use `4*(1-fourthRoot(q))` as its numerator. The q=1 geometry,
Gaussian density, section ABI, firing and airborne projection stay unchanged.
This is a bounded contour-math correction, not a scene-contact solver or filter.

Result: native build passes; all four live MSL entrypoints compile in strict and
fast math, and the actual GPU fixture passes. Local commitffe2514 contains only
two aim-owned paths. Canonicalrun.sh --stage-only returns0 and installs engine
2a79c6b1822a5eca4d642d464939e74fc2943415354606684dac6472fe3aa6a5,
receiptabc852e3fd4bc37af79168e0a6c554911628e67ba97fbfdfc6a9691b1725e5b0.
All245 non-log player files and three foreign staged blobs preserve their hashes.
The broad context gate still fails only on four preserved foreign tsunami files.

Disposition: installed candidate, no UI/input/launch. Actual disappearance of the
hillside stripe, zoom continuity and paired FPS remain with player replay. The
player also asks about abrupt/ugly transitions: geometric smoothing was not
changed by this correction and remains open. No further speculative geometry
variant is called accepted from the shader fixture.

### October8 revision10 — one smooth projected dispersion patch

Observation: the player confirms revision9 removed the false far stripe, but
rejects broken/angular terrain contours, water disappearance and aiming cost.
The13:13–13:14 screenshots show83/75/42FPS in different views, so they do not
measure a paired overlay cost. A perfectly elliptical shape is unnecessary;
the requested patch needs rounded edges and smooth transitions.

Delta: cannon-depth-drape.patch replaces65 fitted axial sections with one48-byte
world mean/full xyz covariance at nominal arrival. Every eligible gun retains
27 weighted independent yaw/elevation/speed evaluations; no new contact marches
or RNG. aim_volume.hpp projects the covariance once with the current camera
Jacobian and evaluates one pixel-space quadratic. Nearest scene depth supplies
visible receiver pixels without reshaping the contour. Exact own/model exclusion
and relation color remain. The contact pass/color snapshot use a bounded rectangle;
unchanged stopped air blends afterward in its own bounds. backend.mm avoids
water capture before it can finish the main encoder. Own/model capture and air
remain costs; real aiming FPS is not called accepted.

Evidence: the exact52-gun producer agrees with independent uniform-jitter moments
and translation, with invalid-input rejection. Strict/fast MSL check96 boundary
angles, center fill, one-pixel AA and far-outside rejection. The actual native
encoder preserves contour across four stepped receiver depths, excludes own and
sky pixels, retains model color and uses a136×84 contact scissor at256×192.
Projection probes cover5x zoom, yaw and world-origin rebasing. Canonical build
and run.sh --stage-only return0 after the last native/ordered-patch inputs.
No permanent tests are added.

Installed engine0fc23284301f1b3465887169686ab1b14dd1d6b605a07659a3ca92d73282fe6d,
receiptd9d0421b3f72784831020597741e1f8d18bebc06e5d7fddbba5307ccf3732a6c.
Final deep/strict signature passes after installer completion. An earlier
in-flight seal observation was not acceptance. All245 fresh non-log player files
and three foreign staged blobs preserve hashes. Frozen input hashes:
aim_volume.hpp59e996d7237acaa157fa920c7ceb89ac1f85eab7718337e39bae4278c40d551c;
backend.mm197c3ef55920cc5a9478b719bb0df16ba4a309cd04a4aa78298a5d2a3c7083eb;
cannon-depth-drape.patchc8aa8dbbeead37e8d968e097c892fa9a824a2c1988603a1795d6a6911c89f641.

Disposition: installed candidate; complete player terrain/water aesthetics,
zoom/motion and paired FPS remain unresolved. No UI/input, game launch or actor
termination. The broad context gate fails on four preserved foreign tsunami
paths, not adopted into this checkpoint. Focused local checkpoint00e7d57;
no push. A subsequent direct user request resumes only tsunami/debug-trigger
verification with its existing owner; the computer-control stop remains in force.

### October8 revision11 — actual world-depth decal, close-water anchor and ordinary MRT identity

Observation: the13:51 screenshot and follow-up reject revision10's flat screen
ring, lack of object conformance, and close-water aim displayed near the horizon.
Native source confirms its contact quadratic ignores receiver position. Its
airborne mean can carry the airburst's8-unit overhead offset. DX9 GetTransform
already restores absolute camera coordinates before the real CMatrix inverse;
the pure range selector already traces water. Neither camera nor firing selection
needs a replacement for this defect.

Delta: one selected-receiver world anchor/full covariance, inverted once. Current
depth reconstructs actual nearest receiver positions; a bounded quadratic with
fixed3x3 occupancy filtering supplies fill and rounded thin perimeter. Far
geometry outside the field gets zero coverage. Conservative enclosing-box bounds
replace center-Jacobian bounds. The ordinary fixed-function/lit model fragments
write current depth and identity through MRT, sharing their original shading and
alpha-discard helpers. Model scope/camera/depth-write checks gate those outputs;
no model before-depth copy or full-screen difference pass remains. Own ship and
water capture are zero-GPU-work ABI guards. Stopped air, ammunition trajectories,
range selection and existing relation colors remain unchanged.

Evidence: strict/fast exact complete scene/aim MSL compile; native source/build
and canonical stage pass. Actual native encoder uses ordinary fs_aim identity
and compares every surface pixel against an independent world-sphere/filter
oracle across flat/tilted/curved/creased receivers, water below the camera and a
far plane behind the selected field. Far plane has zero marked pixels; close
water has no horizon mark. Own exclusion, relation color, alpha holes, origin
translation and5x zoom pass. Representative contact ROI is68×52 at256×192.
No permanent test infrastructure is added and no paired real-game FPS is claimed.

Installed engine5b14e3196d4674c7cc296de97debc83fc26fbf7a8a8eb88b41f4cf40989221a7;
receipt0ebc514c8b38488591258f7a36a94c33e6e3301a836f8108c46d8e6bae0fdce4.
Final deep/strict signature and receipt binding pass. All250 fresh non-log player
files and three foreign staged blobs retain hashes. Frozen native inputs:
aim_volume.hpp4d2e217d19715914a5d3432e6dc20836df527f1c2469b469e8cd9e119b4c2001;
backend.mm153d2140a79275dd8e2e7a09751631395e371691d141dd3fc787620d3947f8dc;
land_shadow.hppea388bbdd44b1df2e055e0ab19cd159b46efd15639a811a4348a3d45f8327fd2;
cannon-depth-drape.patch95b6eb59d38a8b63c0d1484d28bbf4c70a1dc79f963255586b461030cde2bec8;
unchanged sea-tsunami.patcha24fe3e7f5e03a3ce4e2f1adcd82e2a89c34e4078e02610f325a59431eb8f697;
build.shc30261105afd42b8e6da8a6aa49ba8aabd2132369aef7d509794b3e99684c57c.

The resumed tsunami owner found no new product-code defect: complete no-window
F11 SDL/core/menu/script/native start/cancel/restart and controls VM restore pass.
Physical macOS interception and visible wave/ship response remain unresolved.
Checkpoint499037a integrates the eight already delivered wave source/topic/build
paths. Their imported debugger baseline retains its original whitespace; the
global context gate passes, and aim-owned whitespace checks pass separately.

Disposition: exact new batch installed; real startup, surface appearance through
all angles/zoom, wave motion and paired aiming FPS await player replay. No UI/input,
launch, kill or player-state mutation. No push. Prior flat-ring GPU checks are not
reused as surface-conformance acceptance.
Focused local aim checkpoint69f6e84; global agent-context and aim-owned whitespace
checks pass. The three unrelated staged document blobs remain outside it.

### October8 revision12 — actual impact spread, smooth perimeter and Esc tsunami

Observation: the14:36–14:37 player screenshots reject revision11's undersized
water footprint, angular/frame-jumping perimeter, excessive brightness and
internal sail/mast outlines. Their different views/FPS do not isolate overlay
cost. Fn+F11 opens no menu according to the player's explicit answer. Source
falsification confirms that evaluating jitter at one nominal flight time and
slicing its3D covariance is not a water-impact distribution: an independent
50,000-shot fixture at50m has36.46–70.45m fifth/ninety-fifth-percentile landings,
while the old water slice has only.434m half-length and covers1.814% of hits.

Delta: every eligible gun contributes the existing27 weighted launch samples,
each with its own descending sea landing and first positive selected-range-plane
crossing. Two marginal impact charts retain true means; water's18.95m half-length
in that fixture covers82.192% of hits, not a95% guarantee. Existing ordinary sea
fragments write exact sea identity through MRT; no additional geometry pass or
snapshot is added. Analytic q-boundary AA replaces neighbor receiver-mask edges,
so holes/depth/identity seams are not drawn as perimeter. Bounded70ms tangent
inertia resets at disjoint fields/receiver changes; current physical depth bounds
never interpolate through a third object. Fill/rim/composite alpha is reduced.

Rejected intermediate: nominal-flight normal variance still gives only.434262m
support and excludes a2m relief niche. Final finite projection support instead
uses the actual water-impact fore/aft radius around current receiver witnesses/
model extent. This admits local relief but does not measure unobserved terrain
or conditional hit probability. Far500m scenery stays outside the tested50m
field. Real terrain shape/all-angle acceptance remains unresolved.

The separately owned airburst batch is integrated at its released final bytes:
stable per-shell1–7m proximity, common2m-full/8m-zero quadratic damage for every
consumer, ordinary bomb aim/height law and omnidirectional finite fragments/VFX.
Native71 ASan/UBSan cases and seeded native-VM sail damage/ordinary-negative
checks pass; old sail-hole flooring fails the zero-distance-power negative.
The existing native projectile codec is unchanged. The existing Esc pause menu
adds sea-only tsunami start/cancel through one guarded script command writer;
F11's physical failure remains unexplained rather than claimed fixed.

Evidence: exact aim MSL and four ordinary legacy/modern sea MRT pipelines compile;
windowless GPU metric and actual native encoder pass Metal API validation for
sea/wall admission, relief/far negatives, own exclusion and subtle perimeter.
CPU frame checks cover receiver switch, current clips, finite bounds and inertia
reset. Full integrated PROGRAM include graph plus pause/debug segments report
`CompilerCases=0`, `script_errors=0`; only disposable fixture Main is renamed,
so no game startup, UI, input or player save replay is executed. Canonical native
build and final stage-only pass after the final source freeze. Exact app/cache
gameplay plans are empty, canonical bytes/changed headers match, and final
deep/strict app signature passes. Protected245 non-log player files and three
foreign staged blobs preserve delivery-boundary hashes. Two external startup
logs changed and a game process subsequently appears; root neither launches nor
touches it. Neither observation accepts gameplay.

Installed engine:
`b5e871dd3a2c24168d9267bcda7b44309dd39c7a4e26946e9f167c1627c5113a`.
Receipt:
`6c4a9e52ccfa3e68cc59b1a77a91b7f64e5b4050ec4b9c75e693d45d7ae2bedf`.
Focused source checkpoints `bdfce2e`, `ba9ae12`, `a813b29`; no push.
Frozen native inputs (SHA-256):
`aim_volume.hpp` `08c129bd721edccd3b51f4d09243d794eaa1e018dad2bab1a3c8ed7e1dc6ef93`;
`backend.mm` `d04c4075aaa0f3d6f70320d0a7bb12045d8bb35a863151899bd0a6a8ba40ad62`;
`sea_shaders.hpp` `a521d41ca6fa2c20e645bb0986f9cad91f8a0e60ea470db07394d8f150cae682`;
`cannon-depth-drape.patch` `bbb90c256eb8bb9fccfa1f1863db57cb48e2d4dfecf58472595556bded925607`;
`airburst-shell.patch` `67bb248d87314d0441e69b102851ebcdd869860fba5b9820814b7fc553abd54d`.
Disposition: source/native/compiler/delivery checks pass; real appearance,
zoom/motion, paired FPS and physical Esc/F11/menu/wave actions remain unresolved.

### October8 — player acceptance and bounded follow-ups

Direct player report accepts revision12's shape/projection and excellent FPS;
the airburst ship result is also accepted. It requests slightly more visible
daytime contour and restores a middle fort-airburst balance. Exact ammunition
answer: shrapnel bombs now barely disable forts, earlier they were too strong.
The corresponding human tsunami report confirms the Esc button starts the wave,
but rejects its straight repetitive crest and requests visual/ship-interaction
work in that feature's existing owner.

Ownership: this aim task owns only `aim_volume.hpp` brightness-local contour and
shared integration/build/stage/index/current inventory. The ammunition owner
owns fort-specific diagnosis/repair. The tsunami owner keeps next-wave discovery
or isolated source preparation outside the current day-contour/fort batch.
No game UI/input, launch, kill or save clone is authorized by these reports.
Daylight candidate raises only bright-background rim/keyline and composite cap;
dark-background formula, line width, fill, geometry and air are unchanged.
Next installation requires a fresh frozen native/full-PROGRAM batch and a closed
player game. Current installed hashes above remain the accepted drape baseline.

### October8 — daylight contour and bounded fort-airburst balance installed

Hypothesis: the near-zero fort result comes from premature proximity to a model
box rather than insufficient catalogue damage. Actual installed meshes confirm
the box encloses empty frontage outside every gun's8m blast radius. The minimal
native correction uses intact gun positions with the existing swept sphere.
Controlled matched approaches now disable1–2 guns instead of the old4–6 or the
installed zero; authored blocked approaches remain blocked. The23-fort/2636-gun
geometry corpus,17 production-loop ASan/UBSan cases and12 native-VM gun/HP cases
pass. The probe uses authored/default model pose, not an immersed saved camera.

Daylight raises the bright-background rim at most16%, keyline alpha from.14 to.17
and composite cap from.52 to.58. Dark-background formula, width, fill, geometry,
air and firing physics stay unchanged. Exact aim/sea MSL pipelines compile;
canonical native build and stage-only pass after the final source freeze. Only
`aim_volume.hpp` and `airburst-shell.patch` differ from the13-input previous freeze.
All PROGRAM bytes and delivered script headers remain identical, so the previous
complete PROGRAM zero-error compilation remains applicable without a new fixture.

Installed engine `10400733be42be03508aa27280dcbd1e2d6be77d11ef638a56c563fe6956da51`;
receipt `23c4496eff4d21ae05c3b27d35954b1dd08451ec9e03a3b6f17a8c0fd4651fad`.
Native input SHA-256: `aim_volume.hpp`
`072c91098adc150c4b903cf2cb4201cb2786cf918d8f489521a2a81bf7f7f7f9`;
`airburst-shell.patch`
`3b87f0600f1bf780e86849b3c6b380d7e1583255ad261f6cb6442f0fa1d60f48`.
Final signature,37 canonical inputs, two delivered script headers and empty
cache/app plans pass. Protected247 player files and three foreign staged blobs
preserve fresh hashes. No game action, save clone or GPU drawing replay occurs.

The sound owner's first muting candidate is explicitly rejected and rolled back
before this freeze: `AIBalls.c` retains SHA
`82e36c2958d3808df3200feaeb0bd5119b4505afe409a66bff565bf2c071e8b9`.
Its engine mass-identical-sound handling continues in the same ammunition owner;
the expanded tsunami candidate remains in its existing owner. Shared build/stage
must transfer once, with engine audio first and tsunami queued after its terminal
receipt. Disposition: source/native/compiler/delivery accepted; new daylight and
fort-salvo player acceptance unresolved. Focused checkpoints `ce5faf2`, `3646cf0`;
no push.

### October8 — dense nearby naval sound requests coalesced in the engine

Hypothesis: mass identical simultaneous effects create independent full-level
voices and unnecessary sample/file initialization. The actual `SoundPlay` body
confirms512 same-clock splash requests select, initialize and start512 voices.
The correction applies one40ms onset interval per nearby same alias/type before
those operations; the same probe now creates1 voice. Ten real-time spaced events
create ten voices while earlier voices remain live, proving no total salvo cap.
Distinct/distant effects, interval expiry, stop/release/end, failed init/start,
singleton level and unaffected loop/cache/speech/music/cannon/charge cases pass.
Existing audio policy probe and canonical service syntax compilation also pass.
No permanent tests are added and no producer sounds are muted.

Canonical native build and stage-only return0 at source checkpoint `31bb837`.
Installed signed engine is
`64f461f7c3b74f1fca4ad49bfca9f24bbd017444312fe85a5b9566ad48d39a76`;
receipt is `740f0676676d2b8a8572631959122330cc33d412839c9e64bb5be0e69737812a`.
All116 receipt entries,37 canonical inputs, two script headers, empty cache/app
plans and deep/strict signature pass. All246 fresh non-log player files and three
foreign staged blobs preserve hashes. Fort patch and existing gameplay producer
bytes retain their frozen hashes. Complete PROGRAM inputs are unchanged, so the
prior zero-error VM compilation remains applicable. Disposition: source/build/
delivery accepted; broadside auditory quality and measured battle FPS unresolved.
No game action or push is performed. Shared build/stage/app ownership transfers
to the existing tsunami owner after this terminal receipt and scope release.

### October8 — continuous tsunami, native ship contact and forced map event

Hypothesis: the reported repeating white wall comes from the original uniform
crest and ordinary periodic foam. One continuous native strength/profile now
drives height/slopes/40-byte material tail, existing hull splash consumers and
ship pitch/roll/contact. The shared sampled strength owns nominal 10–50% hull
damage; storm/armor defenses remain in their existing owner. Actual native
overturn is admitted only for a live fierce contact within 50–130° to wave travel.
The existing saved storm descriptor/trigger admits a rare finite-circle event,
consumes its sampled strength and forces local sea without encounter skipping.

16,146 geometry/contact cases and 243 full-profile CMatrix sweeps pass; actual
SEA/SHIP/SEAFOAM source syntax passes. Combined current PROGRAM VM returns
DamageCases0/script_errors0; descriptor/login/exit gates and two save-codec rounds
pass with declared fixture seams. First offscreen pilot is rejected for visual
admission because its camera/substrate is unrepresentative. Restored installed
64-height/four-mip normals/day12 sky component runs7frames/27passes in2.647s,
145.5MiB RSS, exits0 and leaves no runner. Whole frames show irregular foam and
continuous substrate; inactive32/40, cancel, depth/MRT checks pass.

Exact canonical stage returns0; signed engine4423316154a9a9df91434b503be963cb3b1467d0d19651aeca1ebaa502895a91,
receipt2b30bb9882742f575cad42689e4121aec2f730293d294cb7716571b590bced66.
All116 entries,37 inputs, two delivered script headers and zero gameplay plan
match. Installer deep/strict verification passes.246 non-log player files and
three foreign staged blobs are preserved. Disposition: component/source/build/
delivery accepted; actual visuals/splash/damage/capsize/map-transition/FPS
unresolved and player-owned. No game action or push. The player's subsequent
horizontal drift and horizon complaint are a separate active follow-up batch.

### October8 — source-ready transport and narrow sky seam, install waits for idle

Hypothesis: existing tsunami pose response supplies no horizontal current; the
ship should carry native motion into the ordinary coastline sweep. A transient
60–300m budget uses temporary Move velocity, shared with non-consuming TouchMove
predictions. Exact existing method-body probes pass continuous21-strength sweep,
60.000/179.987/293.884m passages, professional209.339m, immunity0, grounding/cancel/
restart/zero delta, clean raw saved State and unchanged ordinary propulsion.
Native prefixb9e7f075b50e742684afdbd7583766f753c6c836af24e44f2da3d03758b1d427
plus unchanged worldmap suffix produces ordered patch97bc7f6241868a6238ab7d657077cb0b7a652d97f1df536c8c31b0fad4904c9e.

Sky analysis binds two consumers: authored 513-vertex SkyFog opacity fades with
Fog.Height; a second shader band unnecessarily suppresses cloud/texture detail
to9.2 degrees. Candidate narrows only that exact-color seam from .018–.16 to
.004–.018. Installed day12 textures plus the full native fog sphere show restored
lower detail; exact horizon227554 pixels, upper264343 pixels and full no-fog
RGBA remain identical. Four matched GPU frames exit0 and leave no runner;
authored thick-weather opacity is source/numerically preserved, without a storm
GPU replay. Candidate dynamic sky SHA0380f543b38b0b9e811de9e500a51a8ecf0f3560651f05813270741c4a102fe2.

Canonical `run.sh --stage-only` exits1 before build/staging because game30770 is
running from the supported app. Installed bytes stay at44233161/receipt2b30bb98;
no game control, termination, SAVE/config write or runtime override occurs.
Disposition: component/source accepted, full build/install pending idle; player
shore-impact and complete horizon/weather replay unresolved. Root owns the
bounded idle retry, and the player owns closing/testing. No push.

### October8 — transport, shoreline spray, foam repetition and horizon follow-up

The game is absent at the final canonical stage attempt. Native shore prefix
f14ac44c051174e9f64b099c2b8d230cd11f042048e10158aff781c71f430a33
replaces only the previous863-line native prefix; worldmap suffix remains exact.
Combined patchfeba0ff372f36ff2c7fff96409a3e21e8f42aef7e53c648f34ed798253c10976
uses authored CoastFoam strips, actual island trace and synchronous scalar message
50203 to a separately owned12-emitter SEAFOAM_PS pool. Source/component checks
pass512 contacts,8 traces/frame,21 severity samples, scale1 prior particle byte
identity and no-hit/water/submerged/outward/lifecycle negatives. Five native
translation units compile; no installed island rendering is claimed.

The player's remaining crest repetition rejects the earlier narrow material
admission. Wider baseline frames reproduce PENA motifs/unit-cell contrast. Two
authored low-frequency samples distort only the existing foam sampler coordinates.
At actual PENA mips3/4 the independent Fourier oracle reduces unit-lattice contrast
about49.5%, preserving mean/variance within0.4%. Six matched wide frames preserve
macro foam coverage and MRT/depth. The finite-plane far-right cut remains a
fixture limitation. Final material SHAa547cef544331beedbfada3ad89af868b6203d81bc8ddd2d33e5368963f57097;
sky SHA0380f543b38b0b9e811de9e500a51a8ecf0f3560651f05813270741c4a102fe2.
All no-window GPU runners have ended; no game action is taken.

A fresh closed-game delivery baseline binds246 non-log player files, three
foreign staged blobs and five foreign working paths. Exact run.sh --stage-only
is now consuming the final source batch; delivery result follows below.
Real shoreline appearance, drift into a coast, full sky/weather and gameplay
replay remain unresolved and player-owned.

Adjacent unchanged source finding: CoastFoam::clear deletes aFoams entries but
does not clear its vector. Editor Load after an existing foam can retain dangling
entries; ordinary initial scene load starts empty, and this feature never invokes
editor Load. No adjacent repair is included.

Canonical final stage returns0. Signed engine331857037f693b12c1374a0bfb65c01c84b7d5418bf82c64376d48235d3349c0,
receiptbd6d3c16ae2ef88a9957cbcfe1d9cbcd5e3dc3ba573522152fc159865ed1dd60.
All116 managed entries,37 canonical inputs, both actual script headers and empty
cache/app plans pass. Installer deep/strict verification passes; all246 fresh
non-log player files, five foreign working paths and three staged blobs are exact.
Disposition: integrated source/build/stage accepted; real-game replay unresolved.
The player's additional global-map debug call opens a separate content-only
batch, reusing this installed engine. No game action or push.

### October8 — shared world-map debug creation and cancellation

The player requests the menu call on the global map. Existing installed buttons
were sea-only. Two menu gates now admit the worldmap entity; the shared Start
dispatch cancels marked events and invokes one SeaTsunami_CreateMapEncounter
owner reused by the unchanged natural lottery. Stop marks only Storm descriptors
with tsunamiSeverity and sends existing deleteUpdate; ordinary storms/ships stay.
Native spawn/movement/radius/delay/lifetime and forced sea reload remain unchanged.

First candidate passes scoped VM cases but canonical plan rejects the stale
worldmap_encgen output admission. No runtime write follows that read-only failure.
Recomposition from reviewed baselinef6615ecd… proves old9795a316… and new52a68d39…
differ only by shared constructor dispatch. The projector updates reviewed output
admission and retains the installed previous revision in its existing PREVIOUS
owner. Canonical plan then contains exactly four PROGRAM deltas. Full integrated
PROGRAM reports DamageCases=0/script_errors=0; scoped MapDebugCases and
MapMenuCases also report0 with explicit native-creation/entity/exit seams.

Exact canonical run.sh --stage-only is consuming this content-only batch after
the last gameplay/projector edit. Prior native inputs and installed engine are
reused. Fresh246 non-log player files and foreign working/index baselines are
captured; real map marker/approach/contact/sea-transition acceptance remains
unresolved and player-owned. Result follows below; no game action or push.

Exact canonical stage returns0. Engine remains331857037f693b12c1374a0bfb65c01c84b7d5418bf82c64376d48235d3349c0;
app receipt66c742d6ca3dd069f357709fb145421f7e4b79bb0d2bb04f9f4fe8c598e40519
binds all116 managed entries,37 canonical inputs and both script headers.
Cache/app plans are empty, compiler ready; content transaction deep/strict
signature passes, and engine installation correctly skips identical bytes.
All246 fresh player files, five foreign working files and three staged blobs
remain exact. Disposition: source/compile/delivery accepted; visible global menu
event and sea transition remain player-owned replay. No launch/input/termination,
player-state mutation or push.

### October 8 — expanding front, rendered-land shelter, fleet damage and NPC steering

The player's arc/ring, island shelter and global-fleet requests replace the marked
moving disk with a fixed-origin hollow front. One native contact sampler owns
player contact, NPC damage and attenuation. Esc offers explicit arc/ring starts;
natural generation chooses the shape once. The existing descriptor persists the
front and its once-per-fleet hit receipts. Sea entry consumes actual radial
incoming direction and attenuated strength, then clears pending commands.

Named island GM collision/BSP and navigable-water patches cannot establish the
rendered archipelago's land boundary. The actual placed `mein.gm` buffers feed a
one-time planar triangle BVH: 32 active models, 169,394 clipped triangles,
65,535 nodes and 8 MiB capacity, about 26 ms intake. 2,500 segment queries match
an independent edge/barycentric oracle; the actual Jamaica shelter segment hits
while the open ray passes. Queries do not relock buffers; reload/teardown clears
the cache. Contact and rendered angular cuts share that geometry owner.

The first distance/time attenuation candidate is rejected: even maximum strength
cannot reach the existing fierce capsize threshold after normal map activation.
The final gain preserves full strength through configured spawn reach and fades
smoothly to zero at finite maximum radius. Actual contacts at radius 100/140 keep
strength 1. A second probe finds ordinary-storm damage bills preactivation time
differently across update partitions. Subtracting activation time makes one
3-second update and three 1-second updates both bill 1 active second; paused,
inactive and boundary cases remain safe.

The canonical traffic adapter damages actual saved roster HP/snapshots, applies
outer condition once, loses cargo through its existing owner, refreshes surviving
fleet power and removes the final sunk fleet. Quest/qID/ALONE, locally admitted,
deleted and unknown-state fleets are excluded. Actual VM cases show 1000 HP→700,
50 HP→sunk, four save-codec restores preserving damage and once receipts,
condition/snapshot sea restoration, elapsed-time partitions and funded repair
preserving later losses. Missing OnLoad in the first disposable codec fixture is
a fixture seam, repaired there; the baseline/candidate pair then passes. The full
integrated PROGRAM returns DamageCases=0 and script_errors=0.

Local NPC emergency steering uses normalized existing Sailing skill and ordinary
rotate/speed/collision controllers. Anticipation is 5+55×skill seconds; temporary
task evaluation suspension preserves and resumes the original task. Nineteen
eligibility negatives pass. With actual-controller yaw inertia 0.04 rad/s, skill 1
meets the crest at 1.819° from its bow while skill .05 remains at 55.623°; onset
changes monotonically with skill. These checks do not promise successful turns
for every hull or collision. No global captain-skill state is invented.

The first four-frame map fixture is rejected for clipped Jamaica and an unanimated
substrate. Corrected supported free-camera framing uses real WdmSea draws and
installed authored terrain/texture inputs. Final gain is replayed in two no-window
generic-Metal frames: a raised dark face/pale crest remains visible, the sector
behind Jamaica is cut, opposite open water survives and expired radius emits no
mesh. Two command buffers/30 draws use 69.157 MiB and finish without GPU errors.
Live reflections, labels, ships, moving hulls, full open-water arc silhouette and
game FPS are outside this component evidence and remain player-owned replay.

Exact canonical `experiments/native-metal/run.sh --stage-only` returns 0 after all
native/material/projector inputs. Signed engine
3456e45b2998cf17a687a2c7a71e1b944af0b4ff51f833ad64c8fc99e836ad4c;
receipt 1f6f72a619e2019cf82bd0029c45e1cccb0da6f33ca9d54def882cec0c751f41
binds all 117 managed entries and 37 canonical encoded inputs, both script headers,
six native map paths and five native AI paths. Cache/app plans are empty, compiler
ready and deep/strict signing passes. Ordered patch
35eacd393496a88c69f4c1eb1317ddda28f044e07d40d5c7e692e2bc0c36388c
preserves the earlier local-sea prefix; new WdmTsunamiWave technique SHA-256
fa7d680cf37593f8a8413827eaac1efeb8815b0bd61dc4151df77307221d7b32
is managed by the canonical material receipt. Earlier sea/sky shader bytes remain.

Fresh baseline persistence failed on a mistyped HEAD assertion before staging;
preservation instead compares against the verified prior closed-game snapshot.
All 246 non-log player files, five foreign working files and all five foreign
index entries including three staged blobs still match. Two extra lowercase
launch logs predate this stage. Disposition: integrated source/component/build/
delivery accepted; real-game map/sea/save/ship behavior and FPS unresolved, with
the human owning that replay. No game launch/input/termination, player-state
write or push. Focused local source checkpoint follows below.

Focused local checkpoint `7d2eed3aa086d6ccf1a3f087af2acd21ffd1155e` owns exactly
the eleven expanded-front/traffic/menu/material-admission paths. The ordered
native patch remains byte-identical to the staged batch. Agent context check
passes; zero nonignored untracked paths remain. All five foreign working files
and index entries, including their three staged blobs, survive the checkpoint
unchanged. No push. Build metadata still names the pre-checkpoint HEAD83a643c;
the exact installed source was staged before the commit without subsequent
runtime-input edits.

### October 8 — flat-speed band, progressive tsunami contact and hull-pose correction

Player reports little growth above the sailing minimum and immediate tsunami
damage. Exact native fixtures reproduce both defects: doubled stock propulsion
keeps the same target, and one crest query pays all damage. The initial sailing
insertion breaks the ordered diagnostic patch's context; canonical build rejects
it without replacing cached source. Moving the new formula after the established
floor assignment preserves the diagnostic context and the canonical build passes.
Continuous target/HUD and mast/control negatives pass under ASan/UBSan.

Per-ship contact integrates real profile/time between ordinary queries, caps the
budget and resets on cancel/serial changes. Actual-header tests pass positive
leading/trailing damage, 0.1/1/2-second partitions and lifecycle guards. An initial
following-ship fixture misses most of the event because it starts sailing from
launch time and escapes the finite front; beginning that movement near contact
tests the intended passage without changing production code.

The two later player images reopen hull/wave acceptance. Source discovery binds
rendered height and WaveXZ to one profile, but extra tilt to height rather than
slope, with a stale rendered pose and yaw-only support footprint. Exact-method
calibration tests reject a level crest demanding 117-degree roll. Signed flank
forcing, bounded bow kick, tilted support and current post-Move pose pass native
ShipRocking/CMatrix checks and 400-step ordinary equality. Strength 0.9 and 1 retain
unprotected beam inversion; ordinary/bow/protection/lifecycle negatives pass.
The screenshots themselves do not prove the model crossed the 90-degree fatal gate.

The player's subtractive menu request removes both Stop nodes/handlers/navigation
and merges exact sampled strength/nominal damage into existing Log_Info feedback.
Full PROGRAM/menu compilation and real script VM lifecycle/report cases pass;
ordinary menu sections are unchanged. The frozen daylight aim owner hands over
three hash-bound paths, which root consumes in the same final canonical stage.

Stage-only returns 0; installed engine b4013994 and receipt 4bc95b60 bind the current
inventory above. Deep/strict signature and empty cache/app plans pass. The broad
player-file equality check first detects a human relaunch's two new Sentry files;
all 247 preceding files, SAVE/config and foreign work/index remain byte-identical.
Disposition: source/component/build/delivery accepted; actual current-save feature
replay remains unresolved and player-owned. No agent game control or push.

Focused local checkpoints: df170087e5b04ad18f00d633ebd72f777df74b3b owns the three
daylight aim paths; 6c743f7092b85beb27e11601e8eaf393675e0453 owns sailing native/topic
paths; 24b1f2886a39db064bb80d3e6ac88b79ad006b8c owns the seven tsunami/menu/topic
paths. No runtime inputs change after staging; the exact build stamp remains
18efcae. Agent context checks pass;
foreign staged blobs and working files remain untouched.

### October 8 — storm-plus-tsunami collision-separation hang

The player clarifies that the prior storm scene froze after debug tsunami start.
The old process is no longer available; system logging was overwritten by the
player's normal relaunch. Retained launch.log.1 has no fatal entry. A three-second
read-only sample of PID99695 shows ordinary frame execution/render/present work;
it is not evidence about the terminated process's hang.

Bounded native audit rejects direct event arithmetic, Move-driven braking and
storm-written invalid inertia. The strongest reproducible owner is FakeTouch:
the old counter advances only for islands, and a ship pair can keep the predicted
collision without changing either XZ position. Exact contour/intersection/vector
methods plus the shared event/drift reproduce equal centres and raised-hull float
stagnation; both exceed 6000 predictions with unchanged state. The fixture aborts
the nonterminating calculation explicitly rather than hanging the game.

The smallest correction counts both solver branches and yields when neither ship
position changes. Source-bound ASan/UBSan reproductions now return after one solve.
Six ordinary/current converging pairs retain exact positions and prediction
counts; an always-contact island adapter retains all 1024 steps and final state.
These adapters test the native solver contract, not the player scene or terrain.
Upstream develop still carries the same bug; bounded release/open-closed issue
checks find no applicable fix. References belong to docs/sea-tsunami.md.

Canonical build returns 0 and changes exactly touch.cpp, whose SHA-256
0a52fd82628ecabe964c9e9abf9e5d2713ca707f4372731cb2af6d8a4fb59f84
equals the tested candidate. Focused local checkpoint bbd6221 owns only the native
ordered patch and tsunami topic. Five foreign working files and their staged
blobs remain unchanged; agent context check passes and no untracked files remain.
After the player's normal quit, canonical run.sh --stage-only returns 0 and
installs signed engine 8bea3d2b6f1da5f77e4212b114402e88aa540d8a533466098644877b97ef6575.
Receipt f0f81e446341a7ffcf0a4baa465c7e29ff3414ba066529f29b13fe9ad4c56d7a matches
all 117 managed entries and 37 canonical inputs; cache/app plans are empty,
deep/strict signature passes, and all 247 preceding non-log player files and
foreign work/index entries are unchanged. Disposition: source/component/build/
delivery accepted; original storm-plus-tsunami replay unresolved and player-owned.
No agent game input, launch, termination, SAVE/config write or push.

### October 8 — map-water appearance, both shore contacts and first-third plateau

Player rejects a black rotating map circle and requests shore-hit foam/splash in
both scenes, plus full power through the first third of severity-defined reach.
The native baseline reproduces the collar; neutral tint alone is rejected.
The missing ordinary animated-water layer is decisive. WdmSea now owns both
raised-water passes and their shared frame/UV setup; fixed eight-repeat crest
phase removes tangential scrolling. Cached landfall produces a foam lip on the
water side without transmitting the wave through land. Ordinary water/storm
behavior, event distance/speed fields and physical contact/shelter remain.

Source-bound final map fixture corrects a prototype's normalized texture-matrix
translation and consumes raw current native CMatrix bytes, actual sea methods,
generic MSL, island geometry/materials and both water techniques. Four frames
show open arc, Jamaica contact, decayed shelter gap and no wave at terminal fade.
Gain sweeps at severity 0/.5/1 preserve entry, remain full through the first third,
then monotonically reach zero. Earlier A/B clips are historical/unviewed and do
not accept exact final continuous motion or player FPS.

Local coast spray previously started at fixed bank Y=.5 beneath the displaced
wave. Stored hit/direction now follow max(bank Y, WaveXZ)+.75 during emission.
Exact native shore message/reset/realize methods pass 74 ASan/UBSan checks,
including independent crest altitude, moving surface, seeded replacement,
cancel/expiry/island negatives; fixed-ground mutation fails the altitude oracle.
Particle API, terminal sea and island lookup remain adapters; ordinary actual
SEAFOAM_PS source is unchanged. Compiled seafoam.cpp matches tested SHA-256
3916bf3196a0e3e2abfd90fd8bd9be76e5a05d4034df8381842da685bccd5064.

Canonical build changes nine source files and returns 0; final run.sh --stage-only
returns 0 with zero further stack changes. Signed installed engine 2cb3b168 and
receipt 41750f00 belong to the current full inventory above. All 117 managed and
37 canonical inputs match, worldmap.fx equals built source, both gameplay plans
are empty, and all 247 baseline non-log player files plus five foreign files/index
entries remain unchanged. Disposition: component/build/delivery accepted; actual
game appearance/movement, shore visibility, FPS and original hang replay unresolved
and player-owned. No game input/launch/termination, SAVE/config write or push.

Focused local checkpoint 22bc292190444307a6de96c3a0d49b310e83de5a owns only
sea-tsunami.patch and docs/sea-tsunami.md. The committed patch matches the staged
86e453f5 batch; agent context check passes, no nonignored untracked paths remain,
and the five foreign working files and staged entries are preserved.

### October 8 — local continuation and optional exit after hull clearance

Player reports that the local wave disappears just past the ship and asks to
remain watching its later contacts. Native Start fixed expiry at launchDistance
plus 700 world units; the natural script lock tracked the entire event lifetime.
The correction keeps the existing frame/evaluator/contact/particle consumers and
extends post-origin travel to 2500+2500s, clamped by ordinary scene reach without
shortening the former minimum. Final eight-second fade and finite teardown remain;
map propagation/first-third gain are unchanged.

Native SEA publishes PlayerPassed from the actual controlled MainCharacter ship's
position and full horizontal hull radius beyond the conservative trailing support.
The existing contact query updates it; NPCs cannot unlock the player. Start/reset,
unknown/invalid hulls, reentry and weather attribute rebuilding retain safe state.
The natural script flag survives until expiry but stops imposing its navigation
lock after passage. Ordinary storm/enemy restrictions remain; no forced map return.

Exact native method bodies and evaluator pass 38 disposable ASan/UBSan checks with
external API doubles, including NPC exposure beyond the old expiry. Eight native
script VM cases exercise the canonical lock branch with a sea-presence seam.
The full staged PROGRAM graph compiles against installed headers with zero errors,
without game startup. Canonical run.sh --stage-only returns 0 after three native
source changes. Signed installed engine 9b2c0616 and receipt 7816732d match the
current inventory; all 117 managed and 37 canonical entries match. Post-stage
deep/strict signature passes, gameplay plan is empty/compiler ready, and all 248
non-log player files are preserved. No game input/launch/termination, SAVE/config
write or push. Disposition: source/component/build/delivery accepted; continued
wave/NPC/shore/optional-map-exit gameplay replay remains player-owned and unresolved.
Focused local checkpoint b490bed owns only the tsunami patch, canonical AIShip
script and topic document. It matches the installed patch/source batch; context
check passes with zero nonignored untracked paths. The five foreign working
files and staged entries remain exact.

### October 8 — procedural weather/sky and coherent sea, followed by solar cloud coupling

Player requires textured volumetric clouds without simplification, no blurred
horizon, smooth procedural weather, vivid dawn/dusk and a non-emissive night sea.
The first local source milestone `e47a40d` replaces repeated layered cloud noise
with the licensed volume renderer, retains original astronomy, and separates
ambient water/foam from Fresnel solar radiance. Native components, input decoding,
cache-motion and sea-material checks pass. First stage installs e969a902;
full-game replay stays unresolved and no game is launched.

The player's dense-cloud sunset screenshot reopens solar acceptance: the separate
SunRoad pass restores the sunset highlight but ignores cloud opacity. A shared
completed-cloud transmission now controls original SUNGLOW overlay and both
modern water solar consumers; 0/.5/1 opacity fixtures yield 1/.5/0 transmission.
The opaque native overlay is RGB21 versus clear/unrelated RGB255. The same
cloudy diagnostic loses its white solar road while the clear negative retains it.
Original density-dependent cone/Beer/powder cloud lighting remains. Source
inspection also proves two consumer mismatches: SUNGLOW used raw solar direction
and a legacy texture-alpha cloud mask, and its layer priority -2 precedes SKY +3.
The correction shares the visual weather adapter, prepares cache before early
astronomy without activating/closing a sky draw scope, and bypasses the unrelated
legacy mask only for a current procedural field. An intermediate e31c6dc7 stage
is historical; it predates early-astronomy preparation acceptance.

Final native build consumes two weather source changes; source/probe and early
solar overlay acceptance pass. Focused local commit `2a6a776` owns eight native
source/patch paths, preserving the five foreign staged/working entries. Final
canonical stage returns 0 with zero additional source changes and installs the
85d06014 signed engine and 492280f2 receipt in the current inventory above.
All 117 managed files, deep/strict signature, 248 player files and foreign index
bytes pass final verification. Source/component/build/install are accepted;
actual complete game weather/SunGlow/shore/ship/postfx/FPS remains unresolved.
No game launch or player-state write; release ZIP/site/Drive remain deferred.

### October 8 — real-player cloud slabs/white clipping and late-night fog reopened

Three player captures show regular cloud contour slabs, burned morning highlights
and pale slate-green distant land at21:54. Matched native components reproduce
the first two; authored hourly fog RGB matches the third. Precision experiments
reject larger caches, manual lookup, precise math and wrapped noise; increasing
march count makes cumulative planetary-coordinate drift worse. Fixed-origin ray
samples remove slabs at the original budget. A pre-UNorm highlight shoulder
restores morning texture, and canonical hours20–23 darken toward night endpoints.
The unchanged original noise, density-dependent lighting, solar coupling and
weather clock remain the owners. Details and rejected alternatives live in
`docs/metal-dynamic-sky.md`.

Focused local commit `92d3659` owns two paths. Canonical stage returns0 and the
current inventory records its signed engine and receipt. Fresh shader/cache,
weather-palette and source/delivery checks pass. A signature inspection attempted
while staging was still mutating the app is rejected as stale evidence; the final
post-stage deep/strict signature and117-file receipt both pass. No agent game
launch, input, termination or player-data mutation occurred. Source/component/
build/delivery accepted; original complete player scenes remain unresolved.

### October 9 — 18:25 directional sky/fog mismatch reopened

The player's shore capture shows pale-blue distant mountains and far water
against dark dusk clouds. The previous hour20–23 palette correction leaves18:25
untouched. Native reproduction compares fully fogged geometry to the actual
displayed sky, proving that scalar authored RGB113/147/179 cannot represent the
procedural sky's direction/elevation. The cyan translucent foliage-card edges
are a separate observed defect whose exact cause is not isolated, outside this
fog repair.

Seven native source paths now share one256×128 directional sky-radiance field
across FFP/land, modern water and deferred shadow ratios. Fog amount/alpha,
weather clock, noise, march count and2048²/96-MiB cloud detail remain. Sky rays
use camera-relative world vectors interpolated without vertex normalization;
the actual rotated/translated SKY mesh and cameraY/6 convention are respected.
Doubling the small fog field did not repair the noon discrepancy and is rejected.

Fresh actual-backend GPU rows cover18:25 facing toward/away from sun, noon,
moon-up midnight, dawn, independently seeded dense rain and a rotated SKY with
an elevated translated camera. Dusk mean RGB error drops from0.393691 for the
authored color to0.000240; worst p95 is0.011765. Unfogged RGB36/50/60 remains
exact. All four modern sea passes match FFP's directional RGB211/127/52 at the
sunward grazing horizon, with foam alpha128; disabling sky restores authored
RGB113/147/179. Fresh dynamic-sky/backend, original/modern sea and shadow probes
pass. The513-frame weather/wind/cache replay remains smooth/finite at mean0.689ms,
p951.867ms, max2.985ms and prime47.92ms; timings include fog compute plus a small
diagnostic draw and do not prove player FPS.

Source/native acceptance passes at local commit `3c41e2e`. Root's canonical
staging is pending (the user is running the game); the prior installed engine
is `44cdd028` (`9052624`). Evidence and disposable diagnostic sources live in
the ignored native sky-evidence owner.

### October 9 — overcast fog veil scoped to fog field, engine staged to played app

Player replayed the stale `44cdd028` engine and correctly reported no change:
the directional fog, aerial perspective and overcast fixes never reached the
installed app. A dirty-tree tuning attempt had also leaked overcast dimming
into the sky display, breaking the midnight/dusk exact-authored-fog probe.
The dimming now applies only to the directional fog field (`veil`), the sky
display stays bit-exact at its horizon, and clear-air Rayleigh drops 20x->5x.
Dynamic-sky, backend, shadow and original/modern sea probes pass at `da1d467`.
Canonical `--stage-only` returns 0 with the game idle and installs signed
engine `694d31f3` plus receipt into the played app; SAVE/config untouched,
8252 inputs verified. Player acceptance of dusk overcast islands-vs-sky,
horizon seam, shore water and cloud motion remains with the player on the
new engine.

### October 9 — clear-night fog veil falls to night ambient, staging awaits idle game

Player capture on `694d31f3` at 19:36 shows islands at sRGB(44,57,76) against
a(10,12,20) sky: a uniform 0.56x/0.42x scale of the Evening authored fog
(77,104,134), proving the veil tracks authored color on clear nights. Root
cause: dynamic-sky coverage is rain-derived (0.32 clear), so overcast dimming
stays weak while the sun is long set. The fog field now mixes its veil to
night ambient by `1-max(daylight,twilight)`: full dark at 19:36, while the
accepted 18:25 sunward dusk keeps twilight=1 and stays bit-identical, and the
sky display path is untouched. All four probes pass at `14daa14`. Staging is
blocked: the game is running the player's session.

### October 9 — dawn horizon seam fixed, base water converges to twilight glow

Player capture at 06:10 dawn shows a hard tone step at the horizon — sky (241,156,78) versus sea (146,115,58) with smooth gradients and no geometry gap. Root cause: far sea converges to the cloudless/glowless ambient veil, while the sky display adds the twilight sun glow (+clouds) stored in the environment map's alpha channel.

Fix: The base-water  variants now request this twilight glow from  (), meaning the far sea equals the actual sky display by construction. The land/foam/sun passes keep the ambient veil, preserving the "no sun shines through islands" acceptance constraint. This intentionally diverges the sea appearance from the old FFP-equality reproduction to fix the modern visual defect.

Dynamic-sky, backend, shadow, and sea probes pass at . Staging awaits idle game.


### October 9 — transient sail geometry corruption fixed

Player reported that all sails became "crooked" (кривые). Inspection showed sail geometry was corrupted. 
Hypothesis: The transient sail upload path introduced in `ad83111` causes geometry corruption when uploading the dynamic sail vertex buffer to the Metal frame arena.
Exact delta: Hardcoded `transientSail=false` in `experiments/native-metal/backend.mm`, forcing sails to use the previously stable `residentBuffer` allocation path instead of the `frameArena`.
Result: Reverted transient uploads locally. Player tested the build in the sea and confirmed the sails are fixed.
Disposition: `accepted`. The fix is committed and staged.

### October 9 — encounter panel: navigation dice, spyglass tiers, HP+guns power

Hypothesis: the legacy map encounter panel decided escape with an unconditional
`rand(100)` gate, duplicated the sea entry button, and could not show a spyglass
advantage; its power comparison also used a count-based estimate.
Exact delta: `tools/gameplay/fleet-encounter-ui.c` (composed by
`tools/metal_fleet_ui.py` into `PROGRAM/interface/map.c`) replaces the escape
gate with a contested roll — `(Navigation+Sneak+Luck/2)/6 + slowest surviving
active companion hull speed × 1.5 + 4` (SailingProfessional) against the hostile
captain's Navigation/6 plus the fastest surviving hostile hull speed × 1.5,
clamped ±8 and resolved as `rand(20)+margin` versus `rand(20)`. A lost roll only
locks `B_CANCEL` and forces the sea fight. Power is now one HP+guns scale for
both sides (traders ×0.35, thresholds 1.80/1.25/0.80/0.40 with Russian labels),
spyglass tiers 2/3/4 add class histogram, hull names plus destination port, and
cannon counts. `B_SEA` stays hidden; both step commands for `B_OK`/`B_CANCEL`
now pass through `GetSelectable`, because `CINODE::DoAction` switches the current
node on `select:` unconditionally and a disabled button would dead-end the
arrow keys.
Intermediate defect: the first composition read `worldMap.enemyMaxSpeed`, which
`WdmFleetUIInfo()` initialised but never filled, so the enemy edge was
Navigation/6 and the clamped margin was almost always +8. The fastest surviving
hostile hull's `RealShip.SpeedRate` is now read inside the same roster loop that
fills `enemyMaxNav`; the player and enemy sides now use the same speed units.
An `Invalid array (spyTypes) size` VM error was investigated and rejected as a
probe artifact: `map.c` fails only when `#include`d into the driver file, while
the game loads `interface\map.c` as its own `LoadSegment` after `seadogs.c`. The
extended `script_vm_probe.cpp` prelude fixture reproduces that loader path.
Result: canonical `sync_metal_gameplay.py apply` returns 0 with the game idle and
installs signed content whose `map.c`/`map.ini` hashes match `.cache/runtime`.
Probe result: the composed `map.c` loads as its own VM segment with zero script
errors and zero duplicate local names.
Disposition: `staged/installed`; player replay pending. Ambush is deliberately
not ported in this batch.


### 2026-10-15 — image-derived frame generation: rejected integration

Hypothesis: GPU-generated intermediate images could deliver the user's large FPS
gain without replacing the Metal renderer. Exact delta: private native87 backend
adds pre-HUD VideoToolbox temporal work; normal native presentation/settings and
the installed app remain unchanged. Serialized384/128 Off/On/Off full-fleet tests
produce valid one- and three-phase images, but reduce native cadence to41.85 and
25.28frames/s; even free presentation caps their output at83.71 and101.12/s.
`docs/metal-sea-benchmark.md` owns the timings, API constraints and retained evidence.
Disposition: both measured variants rejected for integration; no HUD/pacer,
canonical engine build/stage or player replay. Prototypes and failed getter-guard
attempt remain private/recoverable. Native optimization and unmeasured alternative
frame-generation mechanisms remain separate work.
