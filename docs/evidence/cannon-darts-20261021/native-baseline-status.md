# Native ordinary-projectile baseline

The first bounded pilot failed before measurement with exit `-6`: uncaught
`std::runtime_error: invalid entity name`. The fixture used `seaai`; the installed
stock bootstrap at `PROGRAM/SEA_AI/sea.c:267` uses `sea_ai`, matching the registered
native class `SEA_AI`. No native source or binary was modified for this attempt.

Reproduction: `python3 docs/evidence/cannon-darts-20261021/native_workload.py baseline`.
The first-result retention guard intentionally rejects overwrite. The root
authorized one input-corrected retry. Its invocation was the same command with
`baseline-retry` instead of `baseline`; the corrected fixture and its complete
small include inputs are retained in `native-fixture/`. The failed original
root fixture is retained in `native-baseline/fixture.c`.

The workload is the existing eight-Manowar hidden diagnostic scene at 640×480,
160 frames, with 1024 ordinary `Balls` shots: 512 use the ordinary visible sprite
path and 512 use ordinary callback updates. AIBalls native live-count queries
must return 1024 at creation and before the final frame; callback count must be
81408 and callback final position must remain above water and advance forward.
Predicate failure removes Ship entities, which makes the existing driver fail
its final scene check. No game executable, installed player, focus, or user SAVE
is involved. The projectile texture uses the stock four-lane atlas contract.

Raw first failure, frozen fixture input hashes, and binary hash are in
`native-baseline/`. The run producer owns process deadlines and scratch cleanup;
its source is `native_workload.py`, using `tools/metal_storage.py::diagnostic_run`.
The fixture and its small include inputs are retained in `native-fixture/` for a
candidate replay without relying on expiring diagnostic inputs.

Resource admission was inherited from the root's current host probe: 48 GiB,
16 cores, 55% memory free, no compiler/benchmark/game process, estimated 2 GiB
additional peak and 10 GiB reserve, no concurrent builds. The pilot ended after
1.97 seconds; a process inventory afterward showed no benchmark/game survivor.

The corrected retry passed with exit 0, `ships=8`, 640×480 hidden backbuffer,
53180 animated draws, and all 160 frames. Its native count/callback predicates
passed. The renderer logged the `ALLBALLS.TGA` ordinary draw path. The complete
frame shows the same eight-ship sea scene, clouds, lit hulls and sails, water
reflections, and no game UI. The shots are too small for this image alone to
prove visible trajectory quality.

The 128 frames after 32-frame warmup measured CPU core-wall mean 17.1061 ms,
median 15.58065 ms, nearest-rank p95 27.5415 ms. Max sampled RSS was 1159118848 bytes; total
pilot time was 6.89 seconds. Raw CSVs, image pixels, binary/input hashes and the
existing fleet summarizer's output are in `native-baseline-retry/`. No owned
benchmark/game process survived the completed run.

This is a successful ordinary-projectile native workload baseline for the same
fixture's candidate comparison. It proves neither installed-player hit rate nor
dart-specific output quality. Performance acceptance remains pending candidate
replay on the same binary consumer path and retained ordinary output contract.
