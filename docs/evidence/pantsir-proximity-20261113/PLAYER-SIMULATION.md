# Player PVO against long-range missiles: isolated simulation

This is a source-derived **approximation**, not a game run, observed win rate, or proof of ship survival. It supersedes the incomplete matrix status in [PLAYER-LONG-RANGE-PARTIAL.md](PLAYER-LONG-RANGE-PARTIAL.md), while retaining that document's trajectory measurements.

## Scenario and settings

One stationary attacker fires toward one stationary player, from 19,000 m, with target X/Z offsets sampled deterministically within ±80 m. The player's available, enabled defense is in full (non-economy) mode: 8,000 m spherical range, 32 simultaneous interceptors, eight per carrier and 0.05 s launch/scan interval. No shells compete for attention. Each type is forced separately. The single scenario fires one missile; the salvo scenario fires three salvos of four missiles at t≈0, 4 and 8 s, with source 0.2–0.35 s intra-salvo spacing. This corresponds to a custom forced-type, four-shot loadout: in the default non-custom launcher Kinzhal is limited to two. Stationary targets and continued attacks are assumed even after a missile reaches the nominal endpoint.

There are 300 synthetic entity/serial seeds per cell: 300 carrier launches for single, 3,600 for salvo. Seeds are paired between systems and delay settings; they are deterministic synthetic IDs, not measured distributions of real game saves. The mixed mode uses the source hash for each prospective interceptor, including serial increments during failed reaction checks; it is not an average of two ship-level tests. In the 12.5 ms salvo run the mix was 47–51% Pantsir depending on type (Oreshnik 42%).

The columns below are modeled carriers intercepted / carriers launched. The fraction of trials with all twelve carriers intercepted is given separately: it is not the ship's survival probability, since the model has neither damage nor warhead execution. 100% means all 300 sampled cases, not a guarantee.

### Current source delays: reaction 120–280 ms, fuse 1–3 ms

12.5 ms simulation frame; 300 trials per cell.

| Carrier | Single Pantsir | Single Patriot | Single mixed | 3×4 Pantsir | 3×4 Patriot | 3×4 mixed |
|---|---:|---:|---:|---:|---:|---:|
| Kinzhal | 24.7% | 98.7% | 97.7% | 14.3% | 51.8% | 47.3% |
| Kalibr | 97.7% | 100% | 100% | 65.0% | 94.9% | 88.2% |
| Shtorm | 100% | 100% | 100% | 79.8% | 95.4% | 93.0% |
| Oreshnik | 56.7% | 98.7% | 90.0% | 25.4% | 34.4% | 36.2% |

For all 12 carriers intercepted, the 3×4 values for Pantsir / Patriot / mixed were: Kinzhal 0/0/0%; Kalibr 0.3/59.3/21.0%; Shtorm 2.7/58.0/38.0%; Oreshnik 0/0/0%. [Raw counts](player-current-fine.csv).

### Simulation-only delay alternative: reaction 50–250 ms, fuse 2–7 ms

The runner replaces only those two source function bounds in its temporary program; it does not modify engine source or installed settings. Same 300 seeds and 12.5 ms frame:

| Carrier | Single Pantsir | Single Patriot | Single mixed | 3×4 Pantsir | 3×4 Patriot | 3×4 mixed |
|---|---:|---:|---:|---:|---:|---:|
| Kinzhal | 30.3% | 98.7% | 96.7% | 16.0% | 49.5% | 51.3% |
| Kalibr | 98.7% | 100% | 100% | 74.9% | 87.6% | 91.4% |
| Shtorm | 100% | 100% | 100% | 88.3% | 93.6% | 97.1% |
| Oreshnik | 44.0% | 94.0% | 79.0% | 23.8% | 30.6% | 30.4% |

[Alternative raw counts](player-custom-fine.csv). Faster reaction and longer fuse are changed together, so their individual causal effects cannot be separated from this comparison.

## Mechanism and limits

The harness extracts current flight, observation, planning, reaction, aim/tracking error, evasion, Patriot motor/guidance and proximity functions; it uses the source batch collision solver. It reproduces player threat-time ranking among reachable targets, per-target and global caps, Oreshnik Patriot fallback, serial advancement before reaction, deterministic mixed selection, acceleration, launch variation, sensor enable and delayed splash. Source hashes in each CSV are checked before and after each run; inputs are current working files, not proof of an installed build.

This is not the full engine update loop. Scanner source cursors, 128-item truncation, ship-frame ordering, variable frame cadence and evasion/trajectory re-planning after an event are approximated. One attacker has at most 12 carriers, no competing cannonballs; live ship positions, hull geometry, warhead aim offsets, guidance updates, surface and terrain collisions, Patriot lost-target reacquisition, shot fragments, cluster splitting/submunitions, blast, damage and ship destruction are omitted. Missile endpoint arrival is counted as not intercepted. In reality a carrier may split or hit terrain before the modeled endpoint, and destroying a carrier need not neutralize every damaging effect. Oreshnik's high arc is modeled: at first geometric eligibility in a separate 300-sample probe it averaged 6,542 m altitude, versus a roughly 8,995 m peak; the sphere is not a horizontal disk. See [trajectory data](long-range-speed.csv).

## Verification and sensitivity

Reproduce the main data from repository root:

```sh
python3 docs/evidence/pantsir-proximity-20261113/player-simulation.py --trials 300 --seed 19 --dt .0125 > docs/evidence/pantsir-proximity-20261113/player-current-fine.csv
python3 docs/evidence/pantsir-proximity-20261113/player-simulation.py --trials 300 --seed 19 --dt .0125 --custom-delays > docs/evidence/pantsir-proximity-20261113/player-custom-fine.csv
```

Python 3.9.6 and Apple clang 21.0.0, C++20, arm64. Only an isolated temporary executable was compiled; no game, engine build or install was run. With defense disabled, all 24 scenario/type/mode rows produced zero launches and zero intercepts ([negative](player-disabled.csv)). With defense enabled, observed max assignments never exceeded eight, peak airborne never exceeded 32, all requested carriers were initialized, and no trajectories remained unresolved at the 160 s bound. The negative establishes only basic accounting, not full engine parity.

The result is materially sensitive to frame step. Paired 300-seed results at 25 ms are in [player-current.csv](player-current.csv): for example, 3×4 Oreshnik/Patriot is 45.3% at 25 ms versus 34.4% at 12.5 ms, and 3×4 Kinzhal/mixed is 54.5% versus 47.3%. At 100 paired seeds, an additional 6.25 ms run gives Oreshnik/Patriot 28.8% versus 34.6% at 12.5 ms ([6.25 ms](player-quarter-step.csv), [12.5 ms](player-half-step-100.csv)). Patriot's own source integrator advances in 25 ms internal steps, while the reconstructed frame solver and event timing vary with the outer step. Thus the figures have **not converged**; differences of several percentage points and larger are plausible model artifacts. Use the tables as scenario comparisons with this sensitivity, not precise probabilities. Full source/runtime parity and gameplay acceptance remain unverified.
