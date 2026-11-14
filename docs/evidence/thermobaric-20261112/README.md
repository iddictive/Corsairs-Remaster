# Thermobaric missile warhead

For the later bounded Aerial Explosion source and multi-angle investigation,
see [aerial-selection](aerial-selection/README.md).

## Direction and sources

The [2024 field-test paper](https://scindeks-clanci.ceon.rs/data/pdf/proc-0018/2024/proc-00182400263B.pdf)
shows bright, irregular fireballs with differing shapes. The [2023 thermal
imaging study](https://pmc.ncbi.nlm.nih.gov/articles/PMC10433483/) records
expansion, cooling and dissipation; its infrared colors are not a visible-light
color reference. These are examples, not a universal shape or game-timing rule.

The selected direction is a localized bright core, expanding irregular fire,
a brief refractive pressure front, then dispersing smoke. Existing sea/land
depth and ship-fire owners remain in charge. The MIT-licensed
[Fire Pro](https://github.com/dgreenheck/threejs-fire-pro) is a maintained
WebGPU/Three.js fluid renderer, but its runtime and compute simulation do not
fit the existing native Metal scene. The candidate adapts the existing Metal
volume raymarch and noise texture instead; no new runtime dependency or asset
download is required.

## Implementation

Warhead `6` is thermobaric; `5` remains the random selector and legacy `3`
still maps to HE. Player selection, random non-nuclear selection, and custom
enemy/allied ship loadouts use the new type. Its game-tuned blast radius is
1.5 times the corresponding HE radius, hull multiplier is 1.2 times HE, and
crew damage and hull-attached ignition reuse the existing damage owners. These
are gameplay values, not a real-weapon performance claim. The blast center
stays at the actual impact or interception position.

## Evidence and limits

`air-*.png` are six phases of a synthetic airborne effect; `water-030.png`
and `land-030.png` exercise the surface branches. `salvo-055.png` contains
eight overlapping instances. These are standalone Metal frames without a
game ship, normal camera, scene lighting, or gameplay, and do not prove the
installed result or its visual acceptance.
The boundary fixture produced identical background PNGs at 0 and 5 seconds;
the 0.016-second frame differed, confirming entry and exit in that fixture.

The same 960×540 synthetic scene was sampled three warmed times for one and
eight overlapping effects, with the user's game process open and idle. At
0.30/0.55/1.40 seconds the median GPU times for eight were 1.934/0.413/0.405 ms
for HE, 4.476/4.475/1.770 ms for the initial thermobaric prototype, and
1.169/0.531/0.581 ms after bounding the raymarch. Raw samples are in the
three CSVs. This is not a frame-rate or full-game benchmark; game usability
under load remains unverified.

The isolated script VM compiled the current complete program plus the missile
menu and returned `Run=0`, `script_errors=0`. `menu-probe.c.txt` also exercises
the thermobaric menu and spawned-ship selection; `damage-probe.cpp.txt` checks
HE, cluster and thermobaric radius/falloff. The signed installed app was still
running during the work, so installation and actual gameplay remain pending.

For replay, copy `renderer-snapshot.hpp.txt` as `cluster_impact_render.hpp`
beside the chosen `.mm.txt` fixture, and `warhead-id-snapshot.hpp.txt` as
`engine/src/libs/shared_headers/include/shared/sea_ai/phonk_warhead.hpp` under
the same temporary include root. Compile with Apple clang 21, C++20, ARC,
Metal and AppKit; run from the repository root with an output directory and
arguments `single 6`. The fixtures read the existing cluster VFX atlas and
volume from `src/assets/cluster-vfx`, which are currently untracked. The HE
baseline uses `docs/evidence/missile-vfx-20261112/airburst-header.hpp.txt`
and `single 1`. The VM fixture uses the retained script-defines snapshot as
`defs.h` with `script-vm-probe <fixture-dir> Run 0`.
