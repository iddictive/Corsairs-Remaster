# Missile effects: airburst and smoke

The ground explosion volume stores its early density at the bottom of the
64-voxel Y axis: at source frames 3 and 6 the density-weighted Y positions
are 1.7 and 3.0. Centering the entire volume therefore started an airburst
below its world-space center. The updated air path samples the source outward
in both vertical directions, beginning at the detonation point; land and sea
retain their original mapping. `airburst-*.png` show four stages in a synthetic
Metal scene, not the installed game.

`trail-before.png` and `trail-after.png` use four synthetic emitters. The
second uses a lower-density, lighter and distance-attenuated trail. The before
already includes the initial color correction, so it is not the original black
smoke baseline. The two CSV files contain single samples: the first includes
pipeline compilation and neither is a defensible gameplay performance result.
Player visual and frame-rate acceptance remains open.

The attached Objective-C++ fixtures and header snapshots are the exact inputs.
They use the current source assets `src/assets/cluster-vfx/WispySmoke02_8x8.png`
and `src/assets/cluster-vfx/ground-vdb/GroundExplosion64.rg8`, with Apple clang
21, Metal and AppKit on Apple M5 Max/macOS 27.2. Build each fixture with
`clang++ -std=c++20 -O2 -fobjc-arc -framework Metal -framework AppKit` and
include the matching header snapshot under its original header name in the
fixture directory; pass an output directory and `close` for the trail. The
asset files are currently untracked in the existing working tree.

The attached source-level probes returned: water contact during an evasive
trajectory at 5.43 s; 105.7 m lead for a moving target; seven mid-flight
curvature reversals; 685.7 m difference between the two Kinzhal arc extremes;
four hostile missiles intercepted with a friendly and a distant missile left
intact; sector angles 0/90/134 degrees accepted and 136/180 rejected; and 79
radar markers drawn, with failed allocation capped safely. The spawn-menu
fixture returned `Run=0` and `script_errors=0` in `script-vm-probe`. These are
component checks, not an installed-game or save-reload replay.

Compile the `.cpp.txt` probes as C++20 with clang; use the retained missile
header snapshots under their original `.hpp` names plus the project math and
shared-header include paths. The radar probe additionally uses ASan. The
spawn fixture is `cases.c` for the repository's `script-vm-probe <dir> Run 0`.
