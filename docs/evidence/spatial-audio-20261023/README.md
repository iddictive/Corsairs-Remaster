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
