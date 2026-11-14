# Player defense, long-range attack: measured partial

This is **not** an interception or survival matrix. The retained experiment measures only undefended carrier trajectories at the player's 8 km detection boundary. It cannot answer how often the ship survives repeated salvos. No game, engine build, install, or delivery was run.

## Settings and source boundary

The player's maximum range is 8,000 m, minimum launch interval 0.05 s, maximum 32 airborne interceptors, and maximum eight assigned per target. The slider exposes 32 despite separate older button logic limiting increments to 12. For maximum firing rather than conserving shots, use full mode (`AirDefenseEconomy=0`); mode, enabled state and bridge availability are independent. The player has Pantsir (0), Patriot (1), or mixed (2). Mixed is selected by a deterministic hash **for each prospective interceptor serial**, not a one-time random selection of the ship or an independent guaranteed 50/50 draw. An arithmetic average of two pure-defense outcomes is therefore not a simulation of mixed mode.

The current source uses 1–3 ms fuse and 120–280 ms reaction; the earlier `random-salvo` model **overrides** these to 2–7 ms and 50–250 ms. Those earlier values and its 3,600 m / 8 interceptor endpoint are simulation-only NPC-defense scenarios; they must not be attributed to the player, current source, or installed game. Current source is dirty and may change; the CSV hashes the exact files sampled. Source inspection is not proof of installation.

The adjustable player missile-targeting range is capped at 20,000 m in both UI and native code; the probe uses 19,000 m, leaving 1,000 m headroom. This does not establish that the NPC test launcher is constrained by that setting: its launch path selects hostile ships without that range check. Nor does it establish that normal game spawning can place a ship at exactly 19 km. The modeled launch is a controlled geometry within the player's adjustable range, not a claim about an observed game situation.

## Trajectory experiment

Run `python3 docs/evidence/pantsir-proximity-20261113/long-range-speed.py > docs/evidence/pantsir-proximity-20261113/long-range-speed.csv` from the repository root. This compiles a temporary standalone C++20 probe using current source position, velocity and flight variation functions and removes its own temporary directory. Results are in [long-range-speed.csv](long-range-speed.csv). Python 3.9.6, Apple clang 21.0.0, arm64; inputs are SHA-256 recorded in the CSV and checked again after execution.

For each type, 300 synthetic shot serials, owner 2, stationary launch (0,10,19000), target (0,5.5,0), default speed and acceleration multipliers, and 10 ms trajectory sampling. Detection is the first point satisfying the player's 3D range, approaching and horizontal-speed conditions. This excludes actual scanner cadence, planning and reaction; it is a **geometric eligibility point**, not proof of acquisition. No evasion or guidance updates are present before it.

| Carrier | eligible / 300 | speed at boundary, min–mean–max m/s | mean height there | mean highest sampled height |
|---|---:|---:|---:|---:|
| Kinzhal | 300 | 1,738–1,794–1,852 | 468 m | 635 m |
| Kalibr | 300 | 988–1,012–1,036 | 78 m | 116 m |
| Shtorm | 300 | 771–794–819 | 979 m | 1,006 m |
| Oreshnik | 300 | 4,163–4,349–4,544 | 6,542 m | 8,995 m |

Kalibr is already fast but averages only 78% of its varied configured profile speed here; the 20 km cap does not establish that this missile has reached that speed. Kinzhal and Shtorm average approximately 100% and 99%; Oreshnik approximately 87%. These are 3D instantaneous speeds, and configured profile speed is not a universal bound on instantaneous 3D speed. A 19 km launch therefore cannot be described as reaching full speed for all four carriers.

Oreshnik's high ballistic trajectory is essential: 8 km is a **sphere**, so it crosses the boundary while still about 6.5 km above the player. A flat, horizontal 8 km arrival would supply a substantially different interception geometry. Source flight variation, trajectory discretization, aiming offsets and pre-detection guidance can shift these values. The modeled path assumes open water and does not check terrain, early collision, ship damage or cluster splitting. A source carrier reaching the boundary does not imply that its warhead reaches or destroys the ship.

## Why the requested salvo matrix is still unresolved

The existing `random-salvo.cpp` models test-ship defense: it chooses the first reachable vector entry, enforces a forward sector and a two-second planning horizon. The player's source instead ranks threats by time-to-approach, allows up to eight simultaneous assignments per target, applies its own scan cursors, 60-second planning bound, serial-dependent type and speed variation, and optional economy rules. Its serial is advanced before reaction testing. Reusing the existing numeric percentages, changing only the range/cap, or averaging two pure systems would not model that contract. The model also lacks cluster release, hull/terrain impact, damage, and ship survival. A defensible completed matrix needs a separately validated player-selection/reaction implementation and multiple-interceptor same-frame resolution against repeated salvos, then a separately stated ship/terrain survival approximation. No percentage in the older NPC results is evidence of player survival or full warhead neutralization.
