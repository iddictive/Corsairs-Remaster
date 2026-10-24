#!/usr/bin/env python3
"""Rebuild the evidence executable only; never rebuild or launch the engine.

Usage: python3 .../run-native-animation-cpu-probe.py <output-dir> <installed-animation-dir>
Output dir must be task-owned temporary storage. Existing build objects are read-only inputs.
"""
import hashlib
import json
from pathlib import Path
import re
import shlex
import subprocess
import sys

here = Path(__file__).resolve().parent
repo = here.parents[2]
build = repo / 'experiments/native-metal/.cache/build'
out = Path(sys.argv[1]).resolve()
out.mkdir(parents=True, exist_ok=True)
rigs = Path(sys.argv[2]).resolve()
ninja = (build / 'build.ninja').read_text()
block = re.search(r'build native-engine/storm/libs/animation/CMakeFiles/animation.dir/src/animation_imp.cpp.o:.*?(?=\nbuild )', ninja, re.S).group()
fields = dict(re.findall(r'^  (DEFINES|FLAGS|INCLUDES) = (.*)$', block, re.M))
core_header = repo / 'experiments/native-metal/engine/src/libs/core/include/core.h'
methods = re.findall(r'virtual\s+([^;{}]+?)\s*=\s*0;', core_header.read_text(), re.S)
assert len(methods) >= 48
shim = out / 'core-link-support.cpp'
shim.write_text('#include "core.h"\n#include <cstdlib>\nclass ProbeCore final : public Core { public:\n' +
                '\n'.join(' '.join(m.split()) + ' override { std::abort(); }' for m in methods) +
                '\n};\nstatic ProbeCore probeCore;\nCore &core = probeCore;\n')
objects = [build / ('native-engine/storm/libs/animation/CMakeFiles/animation.dir/src/' + n + '.cpp.o')
           for n in ['action_info', 'action_player_imp', 'animation_imp', 'animation_info', 'animation_timer_imp', 'bone']]
assert all(o.is_file() for o in objects)
binary = out / 'native-animation-cpu-probe'
cmd = ['/usr/bin/c++']
for key in ['DEFINES', 'FLAGS', 'INCLUDES']:
    cmd += [a for a in shlex.split(fields[key]) if a != '-v']
cmd += ['-I' + str(repo / 'experiments/native-metal/engine/src/libs/animation/src'), str(here / 'native-animation-cpu-probe.cpp'), str(shim)]
cmd += [str(o) for o in objects] + ['-o', str(binary)]
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
metadata = {'compiler': subprocess.check_output(['/usr/bin/c++', '--version'], text=True),
            'compile_argv': cmd, 'run_argv': [str(binary), str(rigs)],
            'inputs_sha256': {str(p.relative_to(repo)) if p.is_relative_to(repo) else str(p): sha(p)
                              for p in objects + [here / 'native-animation-cpu-probe.cpp'] +
                              [rigs / (name + suffix) for name in ['man', 'man2'] for suffix in ['.an', '.ani']]}}
(out / 'metadata.json').write_text(json.dumps(metadata, indent=2) + '\n')
subprocess.run(cmd, check=True)
with (out / 'raw.txt').open('w') as log:
    subprocess.run([str(binary), str(rigs)], stdout=log, check=True)
print((out / 'raw.txt').read_text())
