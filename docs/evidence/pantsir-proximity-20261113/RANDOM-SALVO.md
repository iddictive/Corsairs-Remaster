# Random-spawn salvo: isolated model, not gameplay

Reproduce from repository root:

```sh
python3 docs/evidence/pantsir-proximity-20261113/random-salvo.py --trials 300 --seed 19
```

The retained output is [random-salvo-results.csv](random-salvo-results.csv); its header hashes the current native sources, spawn script and both model files. Python 3.9.6 and Apple clang 21.0.0 on arm64 compiled only a temporary standalone program. The engine, gameplay, installation and game were not run or changed. Source changes during execution invalidate the output. Preserve the file if repeating; the command prints to stdout.

For the paired charge-limit comparison, run the command above with and without `--unlimited-charges`. Results are retained in [random-salvo-paired-baseline.csv](random-salvo-paired-baseline.csv) and [random-salvo-unlimited.csv](random-salvo-unlimited.csv). The override removes only the replenishing-charge gate and consumption; launch interval and concurrent-interceptor cap remain. No game configuration is changed.

After changing the spawn range to 1200–3600 m, the same paired scenario was rerun and saved as [limited](random-salvo-range-1200-3600-limited.csv) and [unlimited](random-salvo-range-1200-3600-unlimited.csv). Each output records its source hashes; the older tables below describe the earlier 1200–2000 m range.

The later [2–8 concurrent, no charge bucket run](random-salvo-range-1200-3600-cap-2-8-patriot-interval-15.csv) uses 1200–3600 m, 0.2–0.8 s Pantsir intervals and 1.5× Patriot intervals (0.3–1.2 s). Run with `--unlimited-charges` to match the current removal of the game charge bucket. This scenario uses the previously requested simulation-only reaction 50–250 ms and fuse 2–7 ms, which are not currently the game settings. Earlier files and the table below retain their original configurations.

The paired `--defense min` and `--defense max` runs retain the same randomized attacking ships and salvo seed, overriding only defender parameters: minimum is 1200 m, 2 in flight, 0.8 s Pantsir/1.2 s Patriot; maximum is 3600 m, 8 in flight, 0.2 s Pantsir/0.3 s Patriot. With `--unlimited-charges`, outputs are [minimum](random-salvo-defense-min.csv) and [maximum](random-salvo-defense-max.csv). These are simultaneous endpoints, not the minimum or maximum observed from 300 random ships.

## Scenario and meaning

Two stationary opposing hypothetical test ships, 10 km apart, are studied for 45 s of firing and 60 s of additional resolution. Each trial samples the script's discrete random ship class, volley, cooldown range, defense range, interval and airborne cap; ship and defense parameters stay fixed within the trial. This assumes independent uniform script draws and synthetic entity/character IDs; neither is an observed save or a measurement of the runtime random-number generator. Each trial is paired across Pantsir and Patriot; the two defender types are forced separately, then can be averaged equally to represent the script's random 0/1 selection. Neither side is the player's separate defense system. The fixed launch geometry and ±80 m target offsets are scenario assumptions, not the actual hull/warhead aiming calculation. This is a model of repeated salvos against one randomly configured defender, not a measured average over real games.

The source's 5% Kinzhal / 5% Oreshnik / 45% Kalibr / 45% Shtorm choice is made per volley using source `shotVariation` and synthetic IDs, not per projectile. The script's `MissileType` is ignored by the native non-custom launcher. The model reproduces the default Kinzhal limit of two per volley and doubled cooldown, the normal 1–3 or manowar 3–5 shot request, 2–6 s sampled cooldown minimum, maximum minimum+3–6 s, .2–.35 s shot spacing, and 64 active missile-group cap. Type and cooldown serial evolution follows the source. Active-group behavior is simplified to active carrier count because this model contains no fragments or split children.

The earlier runs sampled 1200–2000 m (later 1200–3600 m), .2–.6 s in .1 s steps and 2–5 interceptors in flight. Their source used a bucket of 2 or 4 charges, refilling at 5–20 per minute. The current source has no charge bucket; the newer run above uses 2–8 interceptors and the stated type-specific intervals. The ordinary 2 s planning limit, scan interval, forward sector, source flight/aim/evasion helpers and source proximity resolver are included. The script also samples `DefenseSpeed` 450–750, but the native test-defense path reads `interceptorProfile(testPatriot).planningSpeed` instead, so that attribute does not set speed in this path. Spawn writes `PhonkTest.AirDefenseType`, and the current native source reads that same key. Fixed 0/1 choices are therefore honored; unset/random produces a one-time 0/1 draw in the script.

**Simulation-only overrides:** fuse delay 2–7 ms and reaction 50–250 ms. The current source instead has 1–3 ms and 120–280 ms. Neither override edits the engine. Runtime uses deterministic hash variation, not independent uniform draws at every shot. Reactions here are still source reaction state logic with just its bounds changed.

## Results

Per row: volleys, launched attacking carriers, carriers hit by modeled interceptor splash, and hits divided by launched carriers. All trials share attack inputs across systems. There were 5,650 carriers in 2,376 volleys for each system. A 50/50 arithmetic mixture of the two modeled systems is 3,518 / 11,300 = 31.13% carrier interceptions; it is not a runtime kill probability, ship survival rate, or estimate of damage.

| Defense | Kinzhal | Kalibr | Shtorm | Oreshnik | All carriers |
| --- | ---: | ---: | ---: | ---: | ---: |
| Pantsir | 20/216 (9.26%) | 531/2586 (20.53%) | 691/2566 (26.93%) | 1/282 (0.35%) | 1243/5650 (22.00%) |
| Patriot | 42/216 (19.44%) | 1103/2586 (42.65%) | 1128/2566 (43.96%) | 2/282 (0.71%) | 2275/5650 (40.27%) |

The source-weighted volley shares are not necessarily the same as carrier shares: Kinzhal is capped and classes request different salvo sizes. Observed sample volleys are 118 Kinzhal, 1,068 Kalibr, 1,067 Shtorm, 123 Oreshnik. The model launched 3,267 Pantsir and 3,268 Patriot interceptors across trials; maximum simultaneous active interceptors was 4 and 5 respectively. The last CSV counts are time steps with a low bucket or full airborne cap, **not** opportunities lost or individual denied shots.

## Exact omissions and qualification

- The model scans alive carriers in vector order on each scan; the real ship/source cursors, 64-item scan budget and frame scheduling across up to eight ships differ. It uses one attacker and one defender and assumes hostility, registration and a live target. It has no competing ships, cannonballs, player system or defender's own attack.
- Each interceptor is resolved sequentially per 25 ms step against all currently live carriers; the engine batches multiple interceptors and missile frame plans, then revalidates identity and resolves substeps. Same-frame contacts can differ. The fuse/trajectory helpers are actual source functions, but surrounding orchestration is reconstructed.
- Patriot lost-target reacquisition/timeout and full target identity lifecycle are not reproduced. Model guidance receives null after a target is lost. Motion uses one segment per fixed step, with simplified phase ordering and a flat stub sea for evasion; the game uses variable frame timing, sea, terrain and surface collision.
- The real script's randomly selected warhead is consumed; this model does not sample warheads, perform coverage-based aim offsets or hull aim, track moving targets/ships, construct terrain routes, guided-ship updates, split cluster carriers, release submunitions, simulate blast damage or debris. Oreshnik and cluster outcomes cannot be interpreted as total warhead neutralization. Land/water and ship collisions before nominal endpoint are absent.
- The spawn script's random type is sampled but intentionally not used by the default native launcher; script random generation, real entity IDs and interactions between their hash values are not replayed. The sampled hash distribution is a scenario, not proof of live frequencies. Type weights and model control flow are transcribed and must be checked against the hashed source if changed.
- The 60 s drain is finite; no in-game replay establishes that all trajectories in every possible configuration end by then. Results count initialized carriers, including any unresolved at the end, as denominator. No native/gameplay acceptance or visual/performance claim follows.
