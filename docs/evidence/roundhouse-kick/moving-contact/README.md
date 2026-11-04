# Moving K contact replay

The previous frozen-heading observation is historical: K now tracks its
originally selected eligible target through the active strike, stopping after
contact; it does not reacquire a lost steering target. The physical limb sweep
still takes precedence. If it misses, one close opponent in a 60-degree
forward sector can be hit at the middle of the authored strike window, within
1.75 m. This fallback is a melee approximation, not proof of foot/body contact;
visual timing and actual gameplay remain for the player to judge.

`baseline.json` and `final.json` contain the 20 complete window-free native
replays using the same freshly built libraries. Baseline overlays only the
combat source from `3f33a20`; final links the fresh `liblocation.a` directly
(SHA-256 `7638c5ba45e17dbbe638d849c96ba9b38878bc8b6d07e7bc002e533246795e2c`).
The baseline source SHA is `615f2ae000b2d1279fd1f2f4f56ae9b62514e21672713286e1de5d4a64ccaf84`;
the final source SHA is `c2b4cf0c42b68acd997aec0019a90c3a4ba174acdc9d04a59e661246a254e5b1`.
All processes exited zero. At native loaded turn speed 2 rad/s, left orbit and
25-degree-offset moving attack at 80 degrees/s changed from miss to hit; the
corresponding right cases already hit. At 1.1 and 1.5 m, attack poses 0.4 and
0.6 changed from miss to one hit. The no-lock and two-NPC cases also changed
from zero to one hit, with the second NPC undamaged. Normal forward and
incoming damage cases stayed positive; far, behind, wall, immortal, allied,
escaped, ineligible and erased targets stayed negative. The latter three
exercise lost steering targets without reacquisition. The 40 degrees/s moving
cases and incoming-damage replay also passed against the prior build of the
same source.

The replay uses installed animation resources, real collision geometry, the
script damage consumer and Jolt, but substitutes an in-memory geometry
service and presentation-free actor update. Attack poses are intentionally
held for the pose matrix cases; the orbit cases advance the opponent's attack
animation. These probes do not prove rendered motion, physical contact for the
fallback, or every rig and encounter. An unsupported rig is rejected before
contact; common generated NPC aliases man, man_B and towngirl are supported,
as are musket aliases. No evidence establishes how often other aliases occur.

To reproduce from this checkout with its built archives and matching installed
resources, run each phase separately with a fresh scratch directory:

```sh
CODEX_SCRATCH_DIR="$(mktemp -d)" CORSAIRS_APP='/Applications/Corsairs Iddictive Remaster.app' python3 docs/evidence/roundhouse-kick/moving-contact/replay.py aim-final
CODEX_SCRATCH_DIR="$(mktemp -d)" CORSAIRS_APP='/Applications/Corsairs Iddictive Remaster.app' python3 docs/evidence/roundhouse-kick/moving-contact/replay.py aim-baseline
```

Despite their inherited names, `aim-final` overlays the historical baseline and
`aim-baseline` links the current built archive directly. The retained JSON
contains full case output and redacted path placeholders; it is not rewritten
by the replay. Compare source, archive and generated asset hashes before
interpreting a rerun against these results.
