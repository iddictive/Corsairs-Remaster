# Physical connectivity comparison

The 2026-11-13 sample `experiments/native-metal/.cache/diagnostics/game-20261113-165614-62295.sample.txt` places 763 of 800 observations in one worker under `preparePhysicalCache` → `convex::prepare` → `reconnect`; 563 of those are under `touching`. Counts are sampled stack observations, not frame time.

The direct, headless probe uses the existing `timberSurface`, `cellSource`, structure build/seed, `preparePhysicalCache`, `convex::edit` and `convex::ready` paths with the root Brig1 GM (SHA-256 `0dec3e566390bd410486861ad58844b7589f34684fe87520f1568b5e9a93eaec`) and its configured wood thicknesses 0.22/0.18. It reads all 7,797 source triangles. Compiler: Apple clang 21.0.0, `-O2 -std=c++20`, arm64. No game, renderer, shared build, or installed application is used.

| Run | Initial cache | Local cut | Cold rebuild after cut |
| --- | ---: | ---: | ---: |
| Baseline 1 | 8415.51 ms | 298.03 ms | 9666.76 ms |
| Candidate 1 | 6188.29 ms | 269.25 ms | 9502.49 ms |
| Candidate, actual edited header | 6112.14 ms | 241.37 ms | 8824.40 ms |
| Baseline 2 | 7021.91 ms | 254.98 ms | 10119.30 ms |

The initial graph had 44,206 pieces, 1,802,178 directed edges and six components. A cubic cut changed 26 cells; the cold reconstructed graph had eight components. For baseline versus each candidate, `cmp` passed on the full ordered directed edge lists and ordered component membership for initial, edited and cold-reconstructed graphs, plus serialized changed-cell boundaries. The warm cache returned the same object. This compares each implementation with the baseline at each state; it does not prove that a cold reconstructed graph equals the pre-reconstruction graph. Earlier evidence in the parent directory identified a pre-existing difference there.

The times are individual unisolated local observations, not a controlled fleet benchmark or FPS measurement. This probe does not exercise the complete laser renderer, collision response, save codec, installed game, or real-frame responsiveness.

To reconstruct the comparison, use an empty task-owned scratch directory as `$WORK` and run from the repository root. `baseline.patch` converts the evaluated candidate header into its exact pre-change version; do not apply it to the repository.

```sh
E=docs/evidence/ship-destruction-20261105/physical-connectivity-20261113
GM=experiments/native-metal/.cache/runtime/RESOURCE/MODELS/Ships/Brig1/Brig1.gm
python3 "$E/extract.py" "$GM" "$WORK/mesh.txt"
mkdir "$WORK/baseline"
cp experiments/native-metal/ship*.hpp experiments/native-metal/geometry_work*.hpp "$WORK/baseline/"
patch "$WORK/baseline/ship_hull_convex_material.hpp" < "$E/baseline.patch"
clang++ -O2 -std=c++20 -I"$WORK/baseline" -Iexperiments/native-metal -Iexperiments/native-metal/engine/src/libs/shared_headers/include "$E/probe.cpp" -o "$WORK/baseline-probe"
clang++ -O2 -std=c++20 -Iexperiments/native-metal -Iexperiments/native-metal/engine/src/libs/shared_headers/include "$E/probe.cpp" -o "$WORK/candidate-probe"
"$WORK/baseline-probe" "$WORK/mesh.txt" "$WORK/baseline.graph"
"$WORK/candidate-probe" "$WORK/mesh.txt" "$WORK/candidate.graph"
cmp "$WORK/baseline.graph" "$WORK/candidate.graph"
```

## Candidate enumeration follow-up

An experimental candidate queried the existing material-cell broadphase with each piece's bounds, conservatively padded for piece overflow beyond its authored cell. It intersected the result with the original per-cell candidate list and retained the original traversal order. The prior path remained in use for small dirty sets. The exact, **rejected**, scratch-only candidate is retained as `rejected-enumeration.patch`; applying it to a private copy of the current header in place of `baseline.patch` reproduces the candidate using the commands above.

The same Brig1 probe, with the previously accepted optimization as baseline, returned these uninstrumented elapsed times:

| Run | Initial cache | Local cut | Cold rebuild after cut |
| --- | ---: | ---: | ---: |
| Current baseline | 5593.97 ms | 235.64 ms | 8374.95 ms |
| Enumeration candidate | 6296.50 ms | 232.17 ms | 8803.90 ms |

The complete serialized graph and changed-cell boundary output matched (`cmp`). Separate instrumented runs counted initial reconnect visits of 2,338,897 versus 1,317,331 candidate cells, and 15,879,716 versus 8,843,534 calls to `touching` after the owner check. The local edit used the unchanged path. The added index queries did not pay for themselves on this workload; the candidate was not applied. Counts and timing are from separate runs, and the result does not establish a general limit for other hulls or a definitive cause for the slowdown.
