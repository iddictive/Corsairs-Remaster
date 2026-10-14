# Sea reflection motion

The player reports ship/shore reflections changing rapidly with a stationary
camera. Native `fTmp` advances one 64-slice normal-volume cycle per second;
authored physical wave phases advance 1–8 slices per second. The two rates are
independent. This establishes a temporal-rate mismatch, not the cause of every
visible reflection change.

## Owner and contract

`renderer-rain-weather.patch` publishes existing physical phases
`fFrame1 / FRAMES` and `fFrame2 / FRAMES` in vertex constant 2.Y/Z, with W=1 as
the native marker. X retains original bump/foam animation; rain constant 244
retains its existing `fTmp`. `sea_shaders.hpp::modernSeaBumpPhases` consumes Y/Z
only for the marked normal volume. Modern open-sea body, additive SunRoad and
SimpleSea share that helper. Original-fidelity entries, preblended 2D fallback
and unmarked fixtures retain the original phase. No new clock, temporal history,
lower cubemap cadence or flattened ripple amplitude is introduced.

The dynamic ship/shore cube retains complete same-frame face updates;
`sea-reflection-budget.patch` only amortizes the separate distant sun-road cube.
Rolling dynamic faces and alternate-frame refresh remain rejected because they
introduce torn or lagging ship reflections (historical evidence in
`metal-frame-pacing.md` and `runtime.md`).

## Evidence and acceptance

The exact ordered stack reverse/replay and canonical native build pass.
Explicitly rebuilt `sea-probe` passes original and modern texture/Fresnel/night/
foam/cube-direction/unsupported cases. A disposable real-Metal kernel compiles
the full current sea MSL and accepts marked phases and both fallbacks. A 64-slice
sinusoidal volume sampled over 120 frames at 120 Hz has total variation 0.223287
at the physical 6-slice/s cadence versus 1.578761 at the old 64-slice/s cadence.
This is a temporal component fixture, not a scene or FPS result.

Rejected loop hypothesis: 64 shipped height frames sampled on an 8-by-8 spatial
grid have mean adjacent step 1.85 and wrap step 1.73, with peaks 10 and 6;
there is no exceptional wrap jump in those samples. The first early native
patch insertion conflicted with later rain-patch context; publication belongs
in the existing rain/sea uniform hunk and passes the complete ordered stack.

The accepting replay is the player's ship/shore reflection with stationary
camera, then ordinary camera movement with moving ships. Exact installed batch
and outstanding player replay are recorded separately in `runtime.md`.

The Phonk integration root consumed source hashes `4d43fa4a` / `b4250832`
through the canonical `run.sh --stage-only` combined batch. Installed signed
engine SHA256 is `cfe07b6eac1216c3c4adea8ee693c65ccc8cc5603921ff09a68e753410a4172b`;
unsigned combined engine is `f4eeaf4cb1e90f22d489ce7df39ea754f425259ce1ab3a14d986b8cb8a064e98`.
The canonical transaction verified the app signature and preserved 502 protected
SAVE/archive/settings hashes. Player scene acceptance remains unresolved.
