# Projectile attribute write proxy

On 2026-11-05 the installed-process sample `game-20261105-083534-9434.sample.txt`
contained 290 of 1193 main-thread samples in `AIBalls::Execute` offsets
1400–1732, where the sampled stacks lead into repeated attribute writes.
Another 34 samples at offset 6844 include the later material-energy write.
These are stack samples, not time measurements. The original sample is in the
ignored diagnostics cache and may not survive cache cleanup.

The retained [probe](ball-attributes-probe.cpp.txt) links the actual compiled
`ATTRIBUTES` implementation and uses the actual `STRING_CODEC`; it extracts the
candidate `BallAttributeCodes` from `ai_balls.cpp`. It compares the 15
per-projectile writes, matching `%g` and integer formatting, in their source
order. It tests 1,000 sequences and compares the resulting strings, including
prior mutation and deletion/recreation of attributes. It then times 200 frames
of 100 writes apiece, including construction of the candidate code cache once
per frame. The game's existing process was running during these measurements.

Apple clang 21.0.0, arm64, macOS SDK 27.2, minimum deployment 15.0, `-O3`,
using the existing Release `attributes.cpp.o`. Raw output, milliseconds:

```text
original 17.573 ms / 20000 updates, 200 frames
coded 9.774 ms / 20000 updates, 200 frames
original 17.719 ms / 20000 updates, 200 frames
coded 9.301 ms / 20000 updates, 200 frames
original 17.807 ms / 20000 updates, 200 frames
coded 9.690 ms / 20000 updates, 200 frames
```

Run from the repository root after preparing the native inputs and Release
build. This uses the build's existing object just as the measurement did. The
extracted helper must come from the candidate source containing this evidence.
The probe exits nonzero on a string mismatch.

```sh
probe_dir=$(mktemp -d "${TMPDIR%/}/corsairs-ball-attributes.XXXXXX")
python3 - "$probe_dir" <<'PY'
from pathlib import Path
import sys
source = Path('experiments/native-metal/engine/src/libs/sea_ai/src/ai_balls.cpp').read_text()
start = source.index('struct BallAttributeCodes\n')
end = source.index('\nbool BombContactDetonates', start)
Path(sys.argv[1], 'ball_codes.hpp').write_text(source[start:end])
PY
fmt=experiments/native-metal/inputs/conan/fmt/8.0.1/_/_/package/8c0a53bc9b0a08b9311ca89254091ba7fb4f89b4/include
c++ -O3 -std=c++20 -arch arm64 -mmacosx-version-min=15.0 \
  -I "$probe_dir" -I experiments/native-metal \
  -I experiments/native-metal/engine/src/libs/core/include \
  -I experiments/native-metal/engine/src/libs/core/src \
  -I experiments/native-metal/engine/src/libs/util/include \
  -I experiments/native-metal/inputs/native \
  -I experiments/native-metal/inputs/native/d3d9/dxvk-native/include/native/windows \
  -I "$fmt" -x c++ docs/evidence/naval-ordnance-perf-20261104/ball-attributes-probe.cpp.txt \
  -x none experiments/native-metal/.cache/build/native-engine/storm/libs/core/CMakeFiles/core.dir/src/attributes.cpp.o \
  -o "$probe_dir/probe"
"$probe_dir/probe"
```

The probe is a synthetic attribute workload. It does not instantiate
`AIBalls::Execute`, dispatch script callbacks, simulate ammunition, test hit
events or measure a game frame/FPS. The 100 projectiles per frame are a
measurement input, not a claim about the captured scene. Compiler flags,
machine load, project source and object revision must match when comparing
results. A real same-consumer replay remains necessary to establish the effect
on a naval battle and verify behavior end to end.
