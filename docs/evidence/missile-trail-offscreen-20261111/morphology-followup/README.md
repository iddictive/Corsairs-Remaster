# Smoke morphology and stalled-emitter probe

The reported game frame showed a long, evenly ribbed gray cylinder behind a
Kalibr. This folder holds synthetic offscreen comparisons; none is a capture
of the updated game. `baseline-header.hpp.txt` is the exact pre-change module
(SHA-256 `4b1b768e8e8441668fde0cdc3e8f0142e843ffb4ae6053af952b0a3e3ae1a4e3`);
the evaluated candidate module's hash was
`7296fa19957fc0acad1edc7e049af70fa8308b901fd4d34a364252335df9f27e`.

The existing authored atlas and Metal sprite renderer remain the owners.
Epic's [Niagara smoke guide](https://dev.epicgames.com/documentation/en-us/unreal-engine/how-to-create-a-smoke-effect-using-sprite-particles-in-niagara-for-unreal-engine)
uses size variation, sprite rotation and SubUV animation; the installed Metal
pipeline adopts those compatible ideas without adding a Niagara dependency.
The previous renderer gave every puff the same size, camera orientation and
age-indexed atlas frame. The candidate varies those properties and emission
position, blends some adjacent atlas frames, and reduces older-particle detail
under pool saturation. World-space origin, atlas, colors and 2048-puff bound
remain; no emission is queued for later.

`side-probe.mm.txt` uses a 260 m/s straight synthetic path, a 200 m side
camera, 960x540 output and 90 updates at 60 Hz. `side-before.png` and
`side-after.png` show the same camera and motion; `side-after-post.png` shows
the smoke 0.5 s after emission ends. They omit ship geometry, game lighting,
camera movement and actual gameplay trajectories. `fast-probe.mm.txt` uses
900 m/s with a 900 m camera and the same 60 Hz cadence. Atlas loading uses the
source PNG. The retained fast frames show that the more distant trail remains
thin and somewhat discontinuous. Both fixtures draw a synthetic sea and sky
and cycle missile types.

To replay from repository root, compile each `.mm.txt` with `xcrun clang++
-std=c++20 -O2 -fobjc-arc -mmacosx-version-min=15.0 -I experiments/native-metal
-framework Metal -framework AppKit -x objective-c++` into a task-owned temp
directory, then invoke it with that directory as its first argument and `close`
for the side view. For the old module, first copy `baseline-header.hpp.txt` as
`missile_trail_vfx.hpp` into a separate temporary include directory and put that
directory first in the include path. `stall-probe.mm.txt` uses Foundation and
asserts: four seconds stationary add no trail puffs beyond the launch burst;
10 m/s for two seconds emits five puffs; a long segment is capped at 24; a
five-second gap clears the pool. The check passed.

On Apple M5 Max, macOS 27.2, 12 warm samples per count, serial before/after
side-view runs show GPU median 0.038/0.058 ms (one missile), 0.230/0.595 ms
(16), and 0.463/1.088 ms (64); CPU scene+encode medians 0.018/0.018,
0.047/0.056 and 0.056/0.058 ms respectively. The 64 case saturates the pool.
The faster-path GPU times were highly noisy even at zero particles, so the
raw `fast-*.csv` files cannot support a speed conclusion. These measurements
exclude shader compilation, atlas loading, game scene load and frame scheduling.
The candidate may cost more GPU time under dense overlap; real-game performance
and acceptance remain open.
