"""Rebuild the accepted native replay in an isolated, disposable directory."""
from pathlib import Path
import hashlib
import shlex
import shutil
import subprocess
import sys
import tempfile

evidence = Path(__file__).resolve().parent
root = evidence.parents[2]
native = root / 'experiments/native-metal'
build = native / '.cache/build'
runtime = native / '.cache/runtime/RESOURCE'
source = native / 'engine/src/libs/ship/src/ship_reactive_jet.cpp'
expected = {
    source: 'a7a3a44f85237f84eb7b0fc46917dbccf8079261f736e7124eb207a8d13e1a89',
    native / 'roundhouse_ragdoll.cpp': '8c9ca87ec4f4dbb80594ba7f42a05e4939340e4d2be259107af918480a56b46f',
    build / 'lib/libship.a': 'fbd75156ccc6a407edeff9557a9febae057f67d32e763aade6771b03dc7312b4',
    build / 'libroundhouse_ragdoll.a': 'af01681afe82fbbd48d446fc48f160adbecd57bc2c1a1ea5495bcce694389a51',
    runtime / 'MODELS/Ships/Brig1/Brig1.gm': '0dec3e566390bd410486861ad58844b7589f34684fe87520f1568b5e9a93eaec',
    runtime / 'MODELS/Islands/Nevis/Nevis.gm': '90456f323bead05e8675eb206637c15ddf8a40ea1c4e33dfa28c9b73e87c1b95',
    evidence / 'solver-owner-replay.cpp.txt': '0a32bd5068b9bd7fbb546abe7a2f418124cca8d3b1462e6586ab96259fd3cfae',
    evidence / 'solver-owner-build.py.txt': '4619792232e007a3b82c949ef0ab8885713eb63a2c1e2c0303b01045e5795f69',
    evidence / 'solver-owner-instrumentation.patch': 'fe70f594091c9d7f3acc185462b2bbed603a3ef771ed06289f18428f2f57cf94',
    evidence / 'isolated-reactive-build.py.txt': 'd7f6dd641f17b1a6746b1ee9863952d611f1429ef01785cdab9cf89b5ae07b54',
    evidence / 'native-fixture.c.txt': '87d157ff94a3d52fdc2074c92f89909eafcd169017a947a317a671050a956bd2',
    root / 'docs/evidence/particle-tracer-capacity-20261029/engine.ini.txt': 'ec749ff87d9603d2f502fd350f55c6c7a96ef11ec9128e15c5bcc7dc762cd7e0',
}
for path, digest in expected.items():
    if not path.is_file() or hashlib.sha256(path.read_bytes()).hexdigest() != digest:
        sys.exit(f'Input missing or changed: {path}; do not treat this as the pinned replay')

work = Path(tempfile.mkdtemp(prefix='corsairs-solver-owner-'))
print(f'scratch={work}', flush=True)
success = False
try:
    shutil.copy2(root / 'docs/evidence/particle-tracer-capacity-20261029/engine.ini.txt', work / 'engine.ini')
    shutil.copy2(evidence / 'native-fixture.c.txt', work / 'fixture.c')
    shutil.copy2(evidence / 'solver-owner-replay.cpp.txt', work / 'probe.cpp')
    (work / 'RESOURCE').mkdir()
    for entry in runtime.iterdir():
        if entry.name.casefold() != 'shared':
            (work / 'RESOURCE' / entry.name).symlink_to(entry)
    shutil.copytree(native / 'engine/src/libs/shared_headers/include/shared', work / 'RESOURCE/shared')
    subprocess.run([sys.executable, str(evidence / 'isolated-reactive-build.py.txt'), str(root), str(work)], check=True)
    instrumented = work / 'ship_reactive_jet.cpp'
    shutil.copy2(source, instrumented)
    with (evidence / 'solver-owner-instrumentation.patch').open('rb') as patch:
        subprocess.run(['/usr/bin/patch', str(instrumented)], stdin=patch, check=True)
    ninja = native / 'inputs/toolchain/bin/ninja'
    commands = subprocess.check_output([str(ninja), '-C', str(build), '-t', 'commands', 'lib/libship.a'], text=True).splitlines()
    command = next(shlex.split(line) for line in commands if ' -c ' in line and line.endswith('/ship_reactive_jet.cpp'))
    command[command.index('-c') + 1] = str(instrumented)
    command.append('-I' + str(source.parent))
    for flag in ('-o', '-MF', '-MT'):
        if flag in command:
            command[command.index(flag) + 1] = str(work / ('ship_reactive_jet.cpp.o.d' if flag == '-MF' else 'ship_reactive_jet.cpp.o'))
    subprocess.run([part for part in command if part != '-v'], cwd=build, check=True)
    subprocess.run(['/usr/bin/ar', 'r', str(work / 'libship.a'), str(work / 'ship_reactive_jet.cpp.o')], check=True)
    subprocess.run([sys.executable, str(evidence / 'solver-owner-build.py.txt'), str(root), str(work)], check=True)
    subprocess.run([str(work / 'probe'), str(work)], check=True)
    success = True
finally:
    if success:
        subprocess.run(['/Users/md/.codex/bin/codex-delete-temp', str(work)], check=True)
    else:
        print(f'Preserved failed replay for the integrator: {work}', file=sys.stderr)
