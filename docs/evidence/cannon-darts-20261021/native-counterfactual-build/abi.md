# Same-world native counterfactual

Both serial compiles and the isolated link passed. The old AIBalls source and
header are the frozen, hash-verified pre-edit inputs. The matching factory is
compiled from current sea_ai.cpp with Clang's supported VFS overlay redirecting
the canonical ai_balls.h path to that old header; other headers remain current.
No shared source, object, archive, target or compiler cache was modified.

Installed `/usr/bin/c++ --help` confirms `-ivfsoverlay`; the actual successful
compilation is its local compatibility probe. The upstream contract is LLVM's
[VirtualFileSystem documentation](https://llvm.org/docs/doxygen/VirtualFileSystem_8h_source.html).

Native factory disassembly independently confirms the header selection:
`AIBallsvmacd::CreateClass` passes allocation size 0xa8 (168 bytes) in the
counterfactual factory and 0x108 (264 bytes) in the current factory. Both use
their matching constructor/destructor and BALL_PARAMS layout.

Remaining AIBalls consumers are AICannon's inherited Entity::AttributesPointer
and the cannon controller's inline GetMultiplier. The member prefix through
fDeltaTimeMultiplier is unchanged. Save/Load and the factory belong to the
replaced sea_ai.cpp translation unit. BALL_PARAMS has no consumer outside
ai_balls.cpp. This comparison therefore includes the complete AIBalls layout
and loop delta, retaining the same remaining current native world.

The two compiles used peak process-group RSS 477822976 and 453083136 bytes;
the link used 282968064 bytes. They took 3.17, 2.43 and 0.29 seconds respectively,
well inside the admitted 2 GiB/120-second bounds. All process groups exited.

The counterfactual executable SHA256 is
`8863f4d0c6d8f7d8f5e822edacaea68ff6ff0e5e243f7a7d29ff5ad86d8e2604`.
The paired current-world executable was frozen before the 30-degree source cut:
`58fa47015a6215053953e12075a2627a7c67e5f470df75f4c1ea4cc7d3937297`.
Both live under the task's ignored local archive; the public manifest retains
input hashes, exact normalized Ninja argv and selected source boundaries.

After the root schedules one exclusive GPU window, the unchanged 160-frame,
1024-projectile ordinary fixture is run serially:

```sh
python3 docs/evidence/cannon-darts-20261021/native_workload.py perf-old-world
python3 docs/evidence/cannon-darts-20261021/native_workload.py perf-current-world
```

The serial paired GPU replay passed both native count/callback predicates and
full rendering coverage. Old/current CPU mean was 14.1361/14.1041 ms (-0.23%),
median 14.07135/14.0661 ms (-0.04%), and p95 14.9472/15.0122 ms (+0.43%).
GPU execution p95 was 8.14325/7.68321 ms. Both frames retain fleet, sails, sea,
sky, reflections and eight completed shadow cubes; mean absolute RGB difference
is 0.07–0.15/255. See `comparison.json` and the two native-perf result folders.

No material ordinary full-path slowdown was observed in this one controlled
pair. The previous 36.95 ms tail did not reproduce when only the complete
AIBalls representation/loop differed. Its cause remains unattributed; these
measurements do not prove whole-game or dart-heavy performance equivalence.

Native RSS was 2.769/2.797 GB. The first paired pilot exceeded the earlier
2 GiB estimate; current admission had 58% of 48 GiB free, so the observed peak
plus the 12 GiB reserve still fit. The finite paired estimate was updated to
3 GiB before starting the second process. Both runs exited and the exclusive
GPU window was released before offline image comparison. The superseded
13.9-degree range fixture was not rerun.
