# Spatial audio range repair — 2026-10-23

## October25 successor: natural distant tails

The player subsequently rejected ordinary effects vanishing too early, including
salvo splashes beyond180m. Ordinary spatial voices now retain the existing
exponential0.45 curve without its maximum-distance plateau; `SoundPlay` no longer
rejects those voices before decoding. This intentionally changes the older
linear contract below. The separately requested local collision range remains:
`coll_ship2rock`/`coll_ship2ship` are still silent at350m and5km. Ship fire keeps
its local linear range and half gain. Near gain, quiet effects, 2D reload and
voice limits are preserved.

The [raw before/after mixer measurements](natural-tail-20261025.json) use actual
`SoundPlay`/`SoundSet3DParam`, miniaudio0.11.25, no device,48kHz stereo and a mono
16-bit440Hz sine at amplitude8000. Each observation renders4096 frames and
measures samples1024…8191. Splash RMS at181/1200/10000m becomes
0.007624/0.003255/0.001254, demonstrating a decreasing tail without a floor.
The32-voice graph measured36–39µs per5.333ms buffer in one noisy pass; newly
audible distant voices now perform real decode/mix work within unchanged limits.
Separate actual-method lifecycle checks cover one-shot Resume/Restart/focus,
completed-sound negatives and master id0: eight baseline failures become sixteen
passing checks. These CPU observations do not establish perceived game loudness.

## Original October23 contract and evidence

The player reported distant ship/shore collisions audible at approximately 5 km.
The active native Metal sound service uses vendored miniaudio 0.11.25, not the
legacy FMOD service. Collision callers already supply world positions. Aliases
`coll_ship2rock` and `coll_ship2ship` author a 30–350 m range.

The native service selected exponential attenuation with rolloff 0.45. That
model clamps distance to the maximum before evaluating its curve: beyond 350 m,
the collision retains `(350/30)^-0.45`, approximately 33% of its near amplitude.
The actual offline mixer reproduced nonzero RMS `0.0292599076513` at 5 km.
This is a local policy mismatch, not a miniaudio defect; the existing built-in
linear model reaches silence at the authored maximum. See the
[official spatialization documentation](https://miniaud.io/docs/manual/) and
the vendored `ma_attenuation_model_exponential`/linear implementations.

`audio/audio_voice_policy.h` now selects linear rolloff 1 for valid bounded
ranges. `audio/sound_service.cpp` applies it after resolving native min/max,
including later range changes. Miniaudio's default `FLT_MAX` is deliberately
excluded: sounds without authored ranges retain their exponential curve.
Ship fire retains its existing half-gain and 50–100 m linear range. Stereo,
voice admission, impact clustering, streaming, sample rate and mixer cadence
are unchanged. No audio/render hot-loop work is added.

## Scope and independent controls

Alias review covered collisions, grounding, creaks/sails/ship water, hull/sail
hits, flybys, splinters, masts, cannon destruction, explosion/sinking, fire,
cannon calibres, environmental locators and animation effects. Finite ranges
already exist for those groups. Missing-range storm water and tavern actors
are preserved; no arbitrary global range is invented. The silent `ship_bow`
0/0, volume-zero placeholder stays silent. Thunder's 1000–8000 m range means
5 km thunder must remain audible.

Runtime and installed `RESOURCE/INI/aliases/sound_alias.ini` matched SHA-256
`8c4aaf4efbc3571b1a78acf99d75e6e66fb662b35c17c9aa1457d6ea89f5426b`.
Native source baseline: `d47560909c544a41a7210330a951e299b04ca957`.

`replay.cpp` feeds the actual miniaudio engine a deterministic mono sine and
reads stereo PCM with `noDevice=true`, 48 kHz. It opens no sound device or game.
Each row renders 16384 frames and computes RMS over the last 8192 stereo frames,
after initial gain smoothing. Thirteen sound/range cases at seven distances
produce 91 rows in each retained JSONL file.

Results: every valid finite range is silent at and beyond its maximum; gain at
or inside the minimum is unchanged. Missing-range, stereo and silent controls
are byte-for-byte equal numerically. Fire remains unchanged. Thunder remains
audible at 5 km and becomes silent at 8 km. Existing `audio_voice_policy_probe`
also passed cannon classification, concurrency, broadside gain and replacement
checks. These are offline mixer/source observations, not player-scene audio
acceptance or full-game performance measurements.

## Reproduce

Use a disposable `TASK_SCRATCH_DIR` from the repository root. Apple Clang 21,
C++20, arm64, deployment target macOS 15 were used. Baseline mode deliberately
omits the range arguments, preserving the old default curve on the current
header; candidate mode supplies actual native distances.

```sh
c++ -std=c++20 -O2 -mmacosx-version-min=15.0 \
  -Iexperiments/native-storm/audio -Iexperiments/native-metal/audio \
  docs/evidence/spatial-audio-20261023/replay.cpp -o "$TASK_SCRATCH_DIR/baseline"
"$TASK_SCRATCH_DIR/baseline" > "$TASK_SCRATCH_DIR/baseline.jsonl"
c++ -std=c++20 -O2 -mmacosx-version-min=15.0 -DAUDIO_RANGE_POLICY \
  -Iexperiments/native-storm/audio -Iexperiments/native-metal/audio \
  docs/evidence/spatial-audio-20261023/replay.cpp -o "$TASK_SCRATCH_DIR/candidate"
"$TASK_SCRATCH_DIR/candidate" > "$TASK_SCRATCH_DIR/candidate.jsonl"
```

Compare matching rows with tolerance `1e-9`: spatial finite ranges require zero
RMS when `distance >= max`, and identical baseline/candidate RMS when
`distance <= min`. All other controls require identical RMS. Check thunder at
5000 m separately for positive RMS. Inspect the retained raw rows for the
intermediate attenuation change. No permanent test-suite change was added.

Frozen source SHA-256:

- `audio/audio_voice_policy.h`: `dcc27a3ca9f89ec7c40780f20df2cdc2f7aa5b5a3394f6ebc18f378c541fa048`
- `audio/sound_service.cpp`: `46166e8c572ac482cdf852ffc66ab47c36178dbfe34fd9ea4f84f63da803cea1`

The coordinated native build passed 8243 portable inputs and 9 renderer coverage
classes. [Build identity](build.json) binds the unsigned engine and menu script
source. After the player closed the game, installation succeeded; signed engine
identity and its signature were verified. The menu script was separately delivered
through the canonical targeted gameplay command and matched the runtime and app
projection. The player owns the gameplay replay; the game was not launched or
controlled for this fix.

## Overlap and voice starvation repair — 2026-10-24

Baseline: `9910d769f1aa00ed62a27c053a42a6a48a7debe6`. The prior range repair
was already installed; it was not the remaining defect. `AIShip.c` still played
both readiness phrases in one event. `bortreloaded_all` resolves to the anchor
sample `ship/jakor_002.wav`, so it cannot replace the two spoken phrases.

Readiness now keeps one pending bit per side, waits for the actual sound to end,
and rejects obsolete callbacks after ammunition/environment changes. The real
script VM reproducer returns `Run=10` for the old overlapping behavior and
`Run=0`, `script_errors=0` for sequential delivery, duplicate coalescing,
single-side delivery, late arrivals, NPC/bow exclusion, boarding/sea exit and
stale-generation rejection. Its playback-status bridge is stubbed; see
[fixture/procedure](reload-replay.py), [baseline](reload-baseline.txt) and
[candidate](reload-candidate.txt).

The actual native `SoundService` and packaged WAVs were replayed through
miniaudio 0.11.25 with no output device, stereo 48 kHz, master FX 0.5:

| Scenario | Baseline | Candidate |
| --- | --- | --- |
| 96 distributed splashes | 96 voices; peak 21.660; 54,839 clipped samples | 8 voices; zero clipped samples |
| 64 destruction requests, synthetic 100 ms admission intervals | 64 voices; peak 8.138; 68,288 clipped samples | 2 voices; zero clipped samples |
| 12 inaudible cannon requests at 5 km, then one near | Near request rejected | Near request admitted; 1 voice |

The destruction replay ages admission timestamps without advancing PCM. It
isolates repeat admission and worst-case coherent summing, not a recorded battle.

Impact admission now caps the whole group at eight and each nearby repeated
alias at two, with 120 ms repeat suppression inside 20 m. Shared inverse-count
gain bounds summed impact amplitude; isolated gain returns to one. The replay
also exercises mixed-alias capacity, release/replacement and gain recovery.
Finite-range, uncached one-shots beyond their maximum are rejected before
decoding/admission. Stereo and movable distant loops retain valid handles.

The canonical alias configuration is now Git-owned and delivered through the
existing gameplay manifest. The only sound-setting changes are splash radius
1200 → 180 m, splash gain 1.35 → 0.65, and left/right readiness gain 0.70.
Other aliases retain their packaged settings. Collision/destruction are silent
at 5 km; thunder deliberately remains audible there and is silent at 10 km.
See [raw baseline](service-baseline.json), [raw candidate](service-candidate.json),
[native harness](service-replay.cpp.txt) and [runner](service-replay.py).

Run the two Python procedures from a managed disposable scratch environment
providing `CODEX_SCRATCH_DIR`; each accepts `baseline` or `candidate`. The native
runner reuses the existing `script-vm-probe` compile/link graph. Native baseline
reproduction requires the baseline engine build and original packaged alias
SHA-256 listed above; candidate uses current built libraries and canonical
aliases. Restore only the three documented setting deltas to reconstruct the
baseline alias semantics. Apple Clang 21, Python 3.14.7, arm64 and deployment
target macOS 15 were used. No permanent test-suite/CI changes were added.

Incremental native compilation passed all 8243 portable inputs and nine renderer
coverage classes. These measurements prove mixer/VM behavior; they do not
establish subjective balance or full-game frame timing in a live battle.

Engine installation and targeted gameplay delivery completed with signature
verification. Runtime and installed projections both report zero pending files.
Installed engine SHA-256: `79c0e642f5d16f77c73c3ad3af412237d5f076a46e174f0924c8a37d2ed8ac4c`;
delivered `AIShip.c`: `8fe449a05daf167c1e41b03b54ff01d695da8d8f97167a89c8b3561b1865b5a9`;
delivered aliases: `556581433909956846e46cc74e517c7b6b943357293c23566c3d041e2a926dab`.
The game was not launched and player saves/configuration were not modified.

## Distinct effects retain their level — 2026-10-24

Player feedback rejected the sterile mix while retaining the improvement in
clarity. The required distinction is repeated copies of one nearby effect
versus independent effects; zero clipped samples alone cannot accept this mix.
Cannon distances, splash settings and the readiness queue remain unchanged.

The real-service baseline at `3360097b6c4040c6b32dd55b94214eefb98b39b8`
confirmed two policy errors: adding three different effects reduced the first
effect's volume from 0.325 to 0.08125, and eight splash voices rejected an
unrelated destruction sound. Two splashes 80 m apart also halved each other's
volume. These are group-admission/gain defects, not new attenuation evidence.

Admission is now capped at eight per alias/type rather than across different
effects. Nearby duplicates retain the two-voice limit within 20 m. The repeat
window is 50 ms; only the same alias/type inside that radius contributes to
another voice's gain reduction. Group release restores the remaining copy's
level. Normalization scans a compact list of active impacts rather than
repeatedly scanning all 4095 slots for every impact.

The [before](service-distinct-before.json) and
[after](service-distinct-after.json) native PCM replays prove:

- Different effects and separate regions retain volume 0.325 → 0.325.
- A full splash group admits a different effect while rejecting a ninth splash.
- An immediate local duplicate is rejected, a repeat after 70 ms is admitted,
  and a third overlapping local copy is rejected; releasing a copy restores gain.
- Nine different battle effects are admitted. RMS rises from 0.01753 to 0.14034;
  peak is 0.61989 with zero clipped samples at the same FX master volume 0.5.

The [same 96-request splash path](service-density-after.json) still admits
eight voices with zero clipped samples; peak is 0.73815. Admission took 1268 µs
versus the retained 1389 µs baseline observation. This is a bounded event-path
observation, not a full-game frame-time claim. No render/audio callback changed.

Use `service-replay.py distinct-before`, `distinct-after` or `density-after`
inside the managed scratch environment described above. `distinct-before`
requires the baseline source/build; its source check prevents overwriting the
baseline with a candidate run. All modes use the existing packaged WAVs and
the actual native SoundService/miniaudio engine, with no output device.
Admission timestamps are aged for repeat-window checks. Live battle balance
remains a listening check; these PCM results establish the independent-level
and duplicate-suppression contracts.

The native build and signed installation passed. Installed engine SHA-256 is
`be8ec5c0a06521fc9c584f788f79ea1f6ce2be4f5ee0856525499bac4083769d`.
Targeted runtime/app delivery reports zero pending files; readiness-script and
alias-file hashes still match the previous installation. No player save or
configuration changed, and the game was not launched for this replay.

## Competing ship gunfire — 2026-10-24

The player still reported nearly inaudible nearby ships. The prior independent
impact fix did not cover cannon admission or cannon alias ranges. Baseline
`e83a955d8fd0581e856e088de445564253ea6ee8` reproduced both independently:

- Twelve own shots completely rejected another ship's shot at 100 m.
- Six-pounder fire was silent from 250 m, twelve-pounder from 280 m, and
  sixteen-pounder from 300 m, despite their gameplay firing ranges reaching
  300, 400 and 450 m respectively. `Ball_AddBall` uses each firing character's
  `rCannon.Sound` and world muzzle position without restricting sound to the
  main character.

The cannon pool retains twelve active voices. Under saturation, a different
shot now takes the oldest eligible one-shot's slot with a 30 ms tail fade;
ordinary replacement retains at most two fading tails. A fresh duplicate of
the same alias/type inside 20 m and 50 ms is still rejected under saturation,
so a local burst cannot create an unbounded replacement backlog. Paused and
looping handles cannot be stolen. A failed incoming decode leaves prior
voices intact. Increasing the global voice limit alone would not address
first-arrival starvation and would add simultaneous noise; the implementation
reuses miniaudio's existing fade-stop and the service's release lifecycle.

Only the maximum distances of the eighteen cannon/culverine/fort aliases
changed: finite endpoints now span 600–2400 m. Minimum distances, sample
choices and authored volume are preserved. This is game mix tuning; the
endpoints are not claims about physical gunshot propagation. Splash/debris,
independent-effect mixing and the broadside readiness queue are unchanged.

[Before](service-cannon-before.json) and [after](service-cannon-after.json)
use the real native service and packaged samples, stereo 48 kHz, FX master
0.5. Eighteen aliases at seven distances produce 126 distance rows per run.
The own-salvo case now admits the other ship at 100 m and retains it when a
fresh local duplicate follows. All aliases have unchanged PCM peak/RMS at
10 m and zero output at 5 km. At 300 m, six/twelve/sixteen-pounder RMS rises
from zero to 0.01842 / 0.03233 / 0.03469. These sources remain audible at 500 m.

The 96-request local burst still admits twelve, with identical peak 0.78115
and no clipped samples. Admission time was 691 µs before and 617 µs after,
an event-path observation rather than a full-game performance claim. Thirty-two
spatially separate shots are admitted while retaining twelve active voices
and at most fourteen allocated voices in that replacement scenario; peak is
0.62533 with no clipped samples. Failed decode, cached cannon and stereo UI
controls pass. The [independent-effect control](service-cannon-effects-control.json)
retains the previous levels, local duplicate handling and nine-effect mix.

Run `service-replay.py cannon-before`, `cannon-after` or
`cannon-effects-control` in the managed scratch environment above. The before
mode requires the baseline native sources/build and canonical aliases. Distance
rows reset the alias RNG to keep sample choice comparable. These are native PCM
and admission results, not listening acceptance of the player's live battle.

Native compilation and signed app delivery passed. Installed engine SHA-256:
`7e611d8b3b92caf08c214912aead5f7bb5ca084bfcf2b67fc53f513a10f34acb`;
installed/runtime alias SHA-256:
`97e6db7f5a5018009b1ad44d133f6baf404b7d34461131bbb11466b9fd43b85d`.
Targeted delivery reports zero pending files; the readiness script retains its
prior identity. No player save/configuration changed and no game was launched.
