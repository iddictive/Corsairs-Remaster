# Slow nuclear wave: isolated sea replay

The hidden fleet fixture renders eight ships with the real sea and a fixed 16 ms simulation delta. It executes no game script or save. Both variants use the same frozen linked libraries and scene; the original sea version is reconstructed from `45154403d0364d97bbb159db1cc2797d21560dc7` only for the nuclear wave and its bounds. `measured-source.patch` records the first slow-wave candidate; `optimized-source.patch` records the subsequent precomputation. The copied fixture is in `fixture/`. The replay still needs the indicated existing build libraries and installed RESOURCE assets; their availability and identity are not guaranteed after cleanup or installation changes.

The first replay finished all cases but its final wrapper exited nonzero because the unrelated backend source changed concurrently. That run linked the existing backend library, not that source, and the recorded `lib/*.a` hashes were unchanged across the run. The optimized replay used matching `lib/*.a` hashes. The larger engine source fingerprint also changed during the optimized run; the sea.cpp bytes matched the recorded candidate throughout the observed comparison, but this is not a clean current-source build. Root-level library modification times preceded both runs, but were not hashed before the first link. These conditions limit causal and reproducibility claims.

CPU wall time around the full `core_internal.Run()` call, including source injection, 1280x800, p50/p95 in milliseconds:

| Scenario | Original | Slow, before precomputation | Slow, precomputed |
| --- | --- | --- | --- |
| One blast, tier 1 | 8.58 / 9.12 | 8.39 / 9.26 | 8.21 / 9.30 |
| One per second, tier 1 | 8.15 / 9.17 | 8.80 / 9.34 | 8.55 / 9.37 |
| One per second, tier 10 | 8.54 / 9.05 | 13.67 / 14.04 | 10.59 / 11.74 |
| One per frame, tier 10 | 12.33 / 13.11 | 18.97 / 20.36 | 10.09 / 11.01 |

The last row compares frames 2700–3299 in all runs. Other windows are in `initial-results.json` and `optimized/results.json`; raw per-frame CSVs and scene captures are alongside them. The one-per-frame input is 62.5 launches/s, an extreme stress case. Observed maximum counts were 7/27 for original/slow tier-1 at one per second, 7/87 for tier-10 at one per second, and 501/160 for tier-10 at one per frame. Diagnostic sampling is once per 63 frames, not an instantaneous peak. A separate lifecycle probe checked 12,000 launches at 62.5/s: every newest wave was present, maximum 159 after advancing, and 300 simultaneous coincident launches retained the newest with 256 total. Exhausting that retirement reserve can still remove the oldest retiring wave immediately.

`geometry-check.cpp` compared the precomputed implementation with the first slow candidate at 11,808 points across scales, ages, angles and crest/trough positions. All five printed fields matched byte-for-byte; `geometry-samples.txt` has SHA-256 `ea18c481e2dc8baa6f4cede15d9f47faacbac30c1c77d86d117abd32ab7a7cb8`. This proves those samples, not every possible coordinate. `lifecycle-check.cpp` preserves the separate lifecycle probe; its admission function is copied from the observed sea source and is evidence rather than a maintained test.

The [rendered scene](scene/nuclear-wave.png) shows the synthetic single-blast frame 139 frames after the trigger. It uses the frozen renderer library and is not a verification of Feynman's current shader or the installed game. For a replay, set `WAVE_EVIDENCE_DIR` to a new existing output directory and run `replay.py` with `WAVE_OPTIMIZED=1` under `codex-run-scratch`; `WAVE_PICTURE=1` selects the short scene instead. The runner only writes generated builds in scratch. Exact results require the preserved source patch, matching libraries/assets, and comparable host conditions.
