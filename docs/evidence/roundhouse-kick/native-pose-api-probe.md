# Native pose API acceptance

The [disposable source](native-pose-api-probe.cpp.txt) passed against the real
Release `AnimationImp` objects and both installed rigs. The
[raw outcome](native-pose-api-result.txt) records every property and final
palette hashes. Linked object hashes exactly match
[the after CPU metadata](animation-cpu-after-metadata.json), including its
compiler and flags. No timing rerun was performed.

Two actors share one real `AnimationInfo` and start from identical native walk
states. The probe checks getter ownership, parent indices, immediate pose
installation, copied caller input, rejection of wrong counts/null input both
with and without an active override, override persistence across both actors'
`Execute` calls, and per-instance palette isolation. Clearing immediately
restores the native pose at the current playback time and normal stepping.
`EvaluatePose` applies an explicitly changed action position without advancing
time. Parent indices and inverse bind matrices remain unchanged throughout.

Reproduce from the repository root after the native animation objects exist:

```sh
python3 - <<'PY'
from pathlib import Path
import subprocess, sys, tempfile
owner = Path('docs/evidence/roundhouse-kick').resolve()
driver = owner / 'run-native-animation-cpu-probe.py'
scratch = Path(tempfile.mkdtemp(prefix='corsairs-roundhouse-pose-api-'))
rigs = '/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/animation'
sys.argv = [str(driver), str(scratch), rigs]
context = {'__file__': str(driver)}
# Reuse command preparation and abort-only Core support; omit metadata/build/run.
prefix = driver.read_text().split('metadata =', 1)
assert len(prefix) == 2
exec(compile(prefix[0], str(driver), 'exec'), context)
source = scratch / 'native-pose-api-probe.cpp'
source.write_text((owner / 'native-pose-api-probe.cpp.txt').read_text())
cmd = context['cmd']
old = str(owner / 'native-animation-cpu-probe.cpp')
assert cmd.count(old) == 1
cmd[cmd.index(old)] = str(source)
cmd.insert(1, '-I' + str(owner))
subprocess.run(cmd, check=True)
subprocess.run([str(context['binary']), rigs], check=True)
print('Disposable probe output:', scratch)
PY
```

This compiles only a temporary acceptance executable and uses the existing
read-only animation objects. It creates no CMake target, registered test,
runtime installation, engine process or game window. Abort-only Core/service
link support is not exercised. The API probe does not establish ragdoll,
get-up or rendered player acceptance; those remain separate scene consumers.
