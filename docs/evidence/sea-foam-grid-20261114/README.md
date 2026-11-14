# High-altitude foam grid, 14 November 2026

The original 400 m top-down fixture is in `before.png`; `after.png` uses
per-fragment wave heights and the existing anti-tiling sampler. Both show the
same eight-ship scene at 1280×800. Disabling foam entirely removed the regular
bands in this fixture, so the result supports the foam diagnosis, not a claim
about all sea artefacts or gameplay acceptance. The 40 m near view retained
the existing foam appearance in a visual comparison.

The two 4K CSV files capture 12 frames at 4112×2580 on Apple M5 Max. With the
same current sea geometry, but the old shader from `33bed15` for the baseline,
median GPU command time for frames 4–11 was 4.373 ms before and 4.909 ms after.
This short sequential fixture does not measure battle load, all weather
transitions, the installed game, or the cost of the changed vertex layout.

From the repository root, use the maintained scratch runner; `BASE_CURRENT=1`
selects the baseline shader, while omitting it uses the current shader:

```sh
BASE_CURRENT=1 CANDIDATE=0 FOAMFIX=prod STORM_METAL_MODERN_WATER=1 PROBE_FRAMES=12 PROBE_HEIGHT=400 PROBE_WIDTH=4112 PROBE_VIEW_HEIGHT=2580 OUTPUT_FRAME=/absolute/output.ppm "$HOME/.codex/bin/codex-run-scratch" run --max-bytes 2147483648 --max-files 30000 --seconds 300 --reserve-bytes 10737418240 -- python3 docs/evidence/sea-foam-grid-20261114/replay.py
```

`smoke16-gpu.csv` and `smoke24-gpu.csv` record a separate 4K synthetic impact
fixture using `smoke-probe.mm.txt`. At eight concurrent impacts and 1.4 s, the
median was 13.939 ms for 16 integration steps and 18.980 ms for 24; the runs
were sequential. More steps were not adopted: this isolated fixture cannot
prove the user's reported bands originate in the explosion, and the extra
cost was substantial. Reproduce with the current `cluster_impact_render.hpp`,
the saved fixture compiled as Objective-C++, then replace both `(far-near)/16.`
and `i<16 && sampleVolume` with 24 in a scratch copy of the header and repeat.
