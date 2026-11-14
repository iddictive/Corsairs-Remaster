# Underwater medium isolated Metal probe

On Apple M5 Max, macOS 27.2, Apple clang 21.0.0, the 1280×800 synthetic probe below used a constant BGRA scene and depth texture. `underwater_medium.hpp` SHA-256 was `bbd104bae5bb643a2aa5cbe35a3f49fbbec06bb9e6b4a7d472a17dd66eccae83`. Thirty measured frames followed six warmups; CPU includes command completion wait. This is a component measurement, not a game-frame or battle benchmark.

```text
depth 0.01 BGRA 180 120 200 255
depth 0.40 BGRA 123 85 104 255
depth 20.00 BGRA 67 49 9 255
scene depth 0.1 BGRA 163 105 131 255
scene depth 0.8 BGRA 95 59 14 255
scene depth 1.0 BGRA 67 49 9 255
copy GPU mean 0.037 ms CPU+wait mean 0.189 ms n=30
copy+medium GPU mean 0.093 ms CPU+wait mean 0.248 ms n=30
```

The original synchronous source is retained as `underwater-medium-before.hpp.txt` (matching the hash above), and `init-before-probe.mm` measured first `initialize()` calls of 198.259, 1.386 and 1.164 ms in three successive processes. The first result may include cold system compiler caches; it is not a guaranteed worst case. For reproduction, copy the retained header to a disposable directory as `underwater_medium.hpp`, compile `init-before-probe.mm` with that directory as the first include path, and link Foundation and Metal.

The revised `medium-probe.mm` uses asynchronous preparation. On the same machine its first observed cold request returned in 0.018 ms while readiness took 182.222 ms; a later warmed request returned in 0.018 ms and readiness took 1.555 ms. The warmed revised synthetic run measured copy GPU 0.038 ms versus copy plus medium 0.065 ms (30 frames after six warmups). CPU-with-wait was 0.314 versus 0.243 ms and does not establish a CPU improvement. Compile with `/usr/bin/clang++ -std=c++20 -fobjc-arc -O2 -I experiments/native-metal` and link Foundation and Metal. The revised shader hash is `bdd844bdd60eb5213f411b690a1018f3687e0e1ffb7d56ab6b61503bc542fede`.

`boundary-probe.cpp` exercises the actual Metal backend compiled separately at `-O0` from the repository source, linked against the existing static backend support and SDL, with a hidden 64×64 device; it never launches the game. At native resolution and again with MSAA4+FXAA and MetalFX Spatial mode 1 (43×43 → 64×64), the unchanged above-water color `ffc878b4` became `ff093143` underwater and in Present fallback. The test preserved a subsequent HUD clear, ignored an offscreen-target publication, preserved outside-viewport pixels, and produced the same result for homogeneous projection matrices `I` and `-I`. A separate native run returned `ffc878b4`, `ff68557b`, `ff093143`, `ff093143` for camera depths .01, .4, 2 and 20. The fixture uses uniform clears and an identity projection; it does not prove geometry-edge quality, the real camera's full motion, or all supported projection/viewport modes. Its 600 ms wait is fixture setup, not a runtime wait.

`retry-probe.mm` injects a failing library compiler: repeated same-format requests inside five seconds caused one attempt, a retry after five seconds caused a second, and a changed format caused an immediate third. A subsequent real Metal request succeeded; destroying a separate owner while a request was pending did not crash. An initial attempt to use a depth-only format as a color target instead hit Metal's validation assertion; the controlled failure above replaces that invalid test and does not imply recovery from a driver abort.

The component and boundary fixtures do not establish island lifecycle, actual sea modes and waves, player-visible scene quality, audio, or battle performance. The attenuation integrates distance against a locally flat plane at camera water height, not the full displaced wave surface. Close to the surface the fade intentionally approaches zero, so a mirrored sky can remain visible just below the waterline.
