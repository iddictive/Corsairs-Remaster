# Projectile tracer capacity

The actual K2 default particle manager shared a fixed 4096-slot billboard pool
between cannon tracers, smoke, fire and other effects. Exhaustion silently
discarded particles. Projectile creation itself had no matching shot limit.

The native Metal bridge already accepts a dynamic particle count. Its storage
now uses stable `std::deque` addresses and a reusable free list. The non-Metal
fixed vertex/index buffer limit and each XPS emitter's particle cap are retained.
No projectile, save, material or emission-rate contract changed.

## Native observations

`baseline.log` and `candidate.log` use the same real XPS assets, manager, Metal
draw, GPU readback and Present path for 30 frames at 640x480. At 512 concurrent
round-shot tracers, empty systems fall from 270 to zero and live particles rise
from 3112 to 7680. At 512 chain-shot tracers, empty systems fall from 365 to zero
and live particles rise from 1024 to 13824. Even 64 bomb-smoke systems exceeded
the old pool during their emission cycle.

`candidate-lifecycle.log` repeats those workloads and then runs two mixed groups
of 64 bomb-smoke and 192 tracer systems in the same reused manager. Both groups
have zero empty systems; pausing emission drains every particle within two
seconds. Releasing every group also leaves zero particles. The XPS caps remain
100 particles per tracer emitter and 200+100 for bomb smoke.

The measured 512-shot round-tracer Execute+Draw CPU time falls from 4.91 to
2.11ms/frame, and chain-shot time from 7.07 to 3.91ms/frame. These are component
measurements, not whole-battle performance acceptance. GPU-fenced Present wall
time is usually masked by the 120Hz presentation floor, and the concurrent
player session adds noise. The extreme 512-bomb workload now renders 33071
particles instead of 2205, so restored effect work must not be described as
free or as a guaranteed whole-frame speedup.

The maximum observed pool high-water mark is 37039 particles. With 352-byte
payloads, current libc++ deque blocks retain about 12.44MiB of payload storage,
plus roughly 1MiB of active/free pointer capacity. Slots are reused until the
manager is destroyed; ordinary effect retirement and emitter limits remain in
force. Growth replaces the old silent loss of visible effects.

## Replay inputs

`inputs.json` binds source and real asset hashes. `native-probe.cpp.txt`,
`build-probe.py.txt`, `engine.ini.txt` and `fixture.c.txt` preserve the disposable
diagnostic, without adding a maintained test target. The archived probe includes
the optional mixed lifecycle phase added after the first paired measurement.

To reproduce this component observation, use a temporary
directory, copy those four inputs without `.txt`, name the C++ input `probe.cpp`
and the Python input `build_probe.py`, and symlink `RESOURCE` to the prepared
native runtime's `RESOURCE`. Run `python3 build_probe.py <repository> <temporary>`
with the existing configured build, then `<temporary>/probe <temporary>`.
The driver derives compiler/linker arguments from `ninja -t commands
sea-fleet-benchmark`; it never loads a save or writes player data. These
component results do not establish visual acceptance in a complete battle.
