# Spatial audio range repair — 2026-10-23

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
