# Particle field cache comparison

On 2026-11-05, three alternating baseline/candidate pairs used the parent
directory's native probe, fixture, build driver, runtime assets and 30-frame
workloads. Baseline source was `48b4956ab90f329ab8059cf6afeca54326f0cdcd`;
the candidate is the per-emitter billboard field cache in the accompanying
source change. Both executable files statically linked the engine and particles;
system libraries and SDL were shared. The game process was absent. A separate
WebKit GPU process consumed roughly 101–103% CPU before each run; this external
load was not controlled and attribution of the timing difference is limited.

For 512 `grapes_tracer` systems, total manager Execute time across 30 frames
was 77.745, 72.008, 79.847 ms baseline and 47.144, 45.190, 43.594 ms
candidate. GPU-fenced wall time remained near the presentation floor. These
component timings do not establish battle FPS or player-visible acceptance.
The raw logs include balls, grapes, knippels and bomb smoke at 1/64/256/512
systems and mixed pause/release. All 20 count/peak/lifecycle records matched
within each pair after excluding timings. Counts are not pixel or full particle
state comparisons.

The earlier sorted-hash index candidate was rejected: the unpaired
`baseline-quiet-*` and `index-*` runs put Grapes 512 at 30.792–31.698 ms and
38.079–76.923 ms respectively. Later index runs were visibly noisy, so this is
not a precise estimate of regression. The sorted-index prototype was removed.

The candidate also executes `lifecycle.inc` before initialising the scene. It
checks case-insensitive names, a real hash collision (`ax`/`bh`), duplicate
names across types, repeated Load, Convert, DelAll, reload, live bool/string
edits, and cache revision/rebinding after structural changes. It compares the
lookup results with a linear scan of the actual fields. All three candidate
runs printed `field lifecycle parity: passed`. This is a disposable probe,
not an editor interaction or visual acceptance run.

To replay, follow the parent README's temporary-directory setup and build
driver. Build once from the baseline source and save its probe executable.
For the candidate, copy `lifecycle.inc` next to `probe.cpp`, insert
`#include "lifecycle.inc"` immediately before `main`, call `lifecycleProbe()`
at the beginning of `main`, build the candidate sources and save that separate
executable. Run the two executables alternately against the same temporary
fixture and asset directory. The original baseline probe and the candidate
probe differ only by that pre-scene lifecycle check; the per-workload random
seed remains 1337. Both were built with the project's configured native build
and run on Apple M5 Max at 640×480, 1 sample, with 1/60 second steps. Binary
SHA-256: baseline `3f4f9afdd1ba2b8a3044a6770e36c9c1af8ee323561db554a12888e062452ba7`,
candidate `483f033c88ef51b352983b42088b6b6e8817231c51f88a428359e110f4201734`.
These identify the measured binaries; they are not retained build outputs.
