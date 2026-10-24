# Native verification status

The original ordinary baseline and final candidate both pass their 1024-ball
native live-count/callback predicates and full rendered fleet coverage.

| 128 warmed frames after 32 warmup | Original baseline | Final candidate |
| --- | ---: | ---: |
| CPU mean ms | 17.1061 | 16.3489 |
| CPU median ms | 15.58065 | 14.0494 |
| CPU nearest-rank p95 ms | 27.5415 | 36.9468 |
| Max sampled RSS bytes | 1159118848 | 1848672256 |

The complete candidate image retains the fleet, sea, sky, sails, lighting and
shadows. Mean absolute RGB differences from the baseline are approximately
0.10–0.12 per 255-level channel. The raised p95 was not attributable from that
cross-world pilot; binary worlds differ beyond AIBalls. The original source/object/header are frozen
and hash-verified in the task's ignored local archive for a same-world probe.

The completed same-world counterfactual comparison now passes ordinary native
count/callback and full output coverage. Old/current mean was 14.1361/14.1041 ms,
median 14.07135/14.0661 ms, and p95 14.9472/15.0122 ms. No material slowdown was
observed in this one pair; the earlier long tail did not reproduce. See
`native-counterfactual-build/comparison.json` and `abi.md` for exact source/layout
isolation, raw input hashes, resource admission and claim ceiling. This does not
prove whole-game or dart-heavy performance equivalence.

The final benchmark binary SHA256 is
`58fa47015a6215053953e12075a2627a7c67e5f470df75f4c1ea4cc7d3937297`.

The functional fixture first stopped at the driver's RESOURCE-symlink preflight
(exit 64); its corrected input-layout retry passed native counters and final
seven-ship acknowledgement. This covers ordinary/dart/unarmed-Bombs identity,
native BSP hit receiver/xyz, actual in-memory SEA_AI/CSaveLoad round trip, fresh
and loaded camera metadata, and Phonk Weapon 0 retained across fresh dart flight
and native hit. It does not prove camera choreography or damage effects.

The first functional retry's complete image failed visual acceptance: sea and sky were
black and sail/hull fills are missing. The task overlay linked whole technique
subdirectories, while CTechnique::InnerDecodeFiles uses FILE_SERVICE's recursive
directory iterator without following directory symlinks. Other techniques were
therefore omitted. The source-only correction now creates real technique
directories and links individual files, retaining the driver's required
RESOURCE root symlink. The corrected overlay replay passed: complete scene
rendering is restored. The earlier claim that its crop showed a dart was
retracted: the crop shows the central ship's bow. The final frame and hit
counters do not establish that a projectile was still visible. Its exact native
VM counters are BenchBad=0, four expected identity hits=1 each, explosions=0,
fresh/loaded metadata=1 each, and two Phonk Weapon 0 checks. RSS was 1551335424
bytes; the finite pilot took 6.83 seconds. Raw inputs, snapshot and image are
retained in `native-dart-contract-overlay/`.

The first 512-frame range fixture exited 68 after 7.12 seconds. It did execute
the complete frame population and remove the ships on its final reject path,
but the exact failed counter is unavailable from that run. **There is no native
65–75% hit-band acceptance.** No physics tuning follows this opaque failure.

The discriminating overlay replay used the existing SaveEngineState VM
serializer only at the diagnostic's owned cwd, before its final reject path.
It exports a small task-local `range-state.bin` or `dart-state.bin`; it never
loads state or touches the player SAVE. The retained reader validates the
native header/zlib sizes and unique integer scalar records before reporting
exact counters. This is an existing source-backed export route, not a new driver
or test framework. Its exact range snapshot was frames=512, RangeHits0=0,
RangeHits1=0, RangeBad=1. The saved AIBalls object had no launch attributes;
Goods, cannon profiles and muzzles were initialized correctly.

The existing non-rendering script-vm-probe exposed the fixture defect: local
`int cannon` shadowed global `Cannon[]` in the case-insensitive script language,
yielding `Invalid '[' operator, cannon - isnt array` before any launch. Renaming
that local to `cannonIndex` fixed the prepared range source. The same real VM now
executes the launch/scatter helpers with `Probe=0`, `script_errors=0`. This was a
fixture error, not a native hit-rate result; no production physics was tuned.
The corrected old range fixture was superseded by the later 30-degree physics
request and must not be replayed as acceptance for that new contract. The root
owns the new source cut and subsequent range binding. Exact state extraction is reproducible through
`read_native_range_state.py <snapshot> [--dart|--attributes]`.

Final30 fixtures are now prepared offline; see
[`native-angle30-contract.md`](native-angle30-contract.md). They retain normal
16ms/TimeScale1 sampling, actual AICannon queues and real GM labels, one queued
80degree CSaveLoad replay, native AIBalls trajectory snapshots, prior-format
ordinary-state decoding, and2040real BSP shots with exact counters. The task-only
bridge requires one isolated translation unit/link after the final source cut.
The corrected fc87 functional replay passed all four identity hits, BenchBad0,
fresh/loaded metadata1/1 and two Phonk Weapon0 checks in3.37seconds, with
approximately2.7845GB RSS. The angle attempt stopped at fixture compile before
its first frame; there is no native angle, hit-rate or sprite acceptance.
The corrected scripts subsequently passed the existing CPU Storm parser;
that result proves syntax only, not their native execution.

No benchmark or game process survived these runs. Gameplay QA was handed to
the user before installation. The isolated ammunition/audio/F11 cut was
installed from commit e72a91e82d2946428fe32d757d203cfd966a2ee3; see
`ammo-only-cut/install-result.json`. The original SAVE was preserved. Remaining
angle, precision, sprite and player-camera acceptance belongs to the user;
these prepared fixtures are not an instruction to resume testing automatically.
