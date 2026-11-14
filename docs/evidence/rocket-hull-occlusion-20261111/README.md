# Synthetic hull depth probe

The subsequent finite-hull, two-observer investigation and its rejected downward-volume prototype are in [ship-probe](ship-probe/README.md).

The standalone Metal fixture calls the actual `ClusterImpactRenderer` with HE warhead 1, land material (the backend mapping for surface 0/Other), age 0.12 and 0.55 seconds, radius 1.25, and an infinite opaque depth-writing vertical plane at z=0. The camera is at (0, 2.6, 5.5), looking toward (0, 0.65, 0); impact centers are (0, 0.65, z). It does not execute the backend bridge, projectile tracing, a ship mesh, or the game. This is a synthetic depth counterexample, not a measured game hit or representative blast scale.

At z=-5 (behind plane), 0 of 518,400 pixels changed by more than two RGB levels at either age; at z=0, 160,152 and 51,736 changed; at z=2, 468,222 and 158,500 changed. `baseline.png`, `behind.png`, `on.png`, and `front.png` show age 0.55. No performance conclusion follows from this fixture. For actual projectiles the producer currently offsets a direct Other hit two metres opposite incoming direction and reduces its radius to 70%; an opposite-side observer can still have an opaque hull between the observer and the effect. Neither the direction of the reported hit nor its actual depth was captured.

Reproduce from repository root using an empty output directory for each case:

```sh
xcrun clang++ -std=c++20 -O1 -fobjc-arc -framework Metal -framework AppKit -I experiments/native-metal docs/evidence/rocket-hull-occlusion-20261111/probe.mm -o /tmp/corsairs-hull-depth-probe
CENTER_Z=-5 /tmp/corsairs-hull-depth-probe /path/to/behind single 1
CENTER_Z=0 /tmp/corsairs-hull-depth-probe /path/to/on single 1
CENTER_Z=2 /tmp/corsairs-hull-depth-probe /path/to/front single 1
```

The probe reads `src/assets/cluster-vfx` and compiles the adjacent frozen copy of the dirty renderer header, SHA-256 `a1b27de25eb01eb7eb4144113fb0ca71ad178e6b7edfdf2255db9063de705dda`; source fixture SHA-256 `35394f4fb853bac272ecf9afa92cd442f1fefca5fb5f9c1e6338c75a59c6dc16`; this probe SHA-256 `2ab2c261198323d387540ac8dcedc806de7360129b887ca3b93b5707cfd28f4a`. The canonical renderer may subsequently change; this copy preserves only the evidence. Tested with Apple clang 21.0.0, macOS SDK 27.2 and local Metal device. Images were compared against the count-zero image from the same case using an RGB per-channel threshold >2/255; the four `*-0.12.png` files preserve the earlier age.
