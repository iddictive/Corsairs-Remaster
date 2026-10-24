"""One-off evidence replay using the existing native link graph; run in managed scratch."""
import json
import hashlib
import os
from pathlib import Path
import shlex
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
BUILD = ROOT / 'experiments/native-metal/.cache/build'
SCRATCH = Path(os.environ['CODEX_SCRATCH_DIR'])
EVIDENCE = Path(__file__).parent
mode = sys.argv[1]
assert mode in ('baseline', 'candidate', 'distinct-before', 'distinct-after', 'density-after',
                'cannon-before', 'cannon-after', 'cannon-effects-control')
if mode == 'cannon-before':
    for relative in ('experiments/native-metal/audio/audio_voice_policy.h',
                     'experiments/native-metal/audio/sound_service.cpp',
                     'experiments/native-metal/audio/sound_service.h',
                     'src/gameplay/RESOURCE/INI/aliases/sound_alias.ini'):
        original = subprocess.check_output(['git', 'show',
            'e83a955d8fd0581e856e088de445564253ea6ee8:' + relative], cwd=ROOT)
        assert (ROOT / relative).read_bytes() == original, 'cannon-before requires the e83a955 source/build.'
if mode == 'distinct-before':
    for relative in ('experiments/native-metal/audio/audio_voice_policy.h',
                     'experiments/native-metal/audio/sound_service.cpp'):
        original = subprocess.check_output(['git', 'show',
            '3360097b6c4040c6b32dd55b94214eefb98b39b8:' + relative], cwd=ROOT)
        assert (ROOT / relative).read_bytes() == original, \
            'distinct-before requires native sources and libraries built at 3360097.'
commands = subprocess.check_output([str(ROOT / 'experiments/native-metal/inputs/toolchain/bin/ninja'),
                                   '-C', str(BUILD), '-t', 'commands', 'script-vm-probe'], text=True).splitlines()
compile_command = next(shlex.split(c) for c in commands if ' -c ' in c and c.endswith('/script_vm_probe.cpp'))
link = shlex.split(commands[-1])
assert link[:2] == [':', '&&'] and link[-2:] == ['&&', ':']
link = link[2:-2]
source = SCRATCH / 'replay.cpp'
source.write_bytes((EVIDENCE / 'service-replay.cpp.txt').read_bytes())
obj = SCRATCH / 'replay.o'
compile_command[compile_command.index('-o') + 1] = str(obj)
compile_command[compile_command.index('-c') + 1] = str(source)
if '-MF' in compile_command:
    compile_command[compile_command.index('-MF') + 1] = str(SCRATCH / 'replay.d')
compile_command += ['-I' + str(ROOT / 'experiments/native-metal/audio'),
                    '-I' + str(ROOT / 'experiments/native-storm/audio')]
compile_command = [x for x in compile_command if x != '-v']
def run(command, **kwargs):
    result = subprocess.run(command, capture_output=True, text=True, **kwargs)
    if result.returncode:
        print((result.stdout + result.stderr)[-5000:])
        raise SystemExit(result.returncode)
    return result
run(compile_command, cwd=BUILD)
old = next(x for x in link if x.endswith('/script_vm_probe.cpp.o'))
link[link.index(old)] = str(obj)
link[link.index('-o') + 1] = str(SCRATCH / 'replay')
run([x for x in link if x != '-v'], cwd=BUILD)
runtime = SCRATCH / 'runtime'
runtime.mkdir()
installed = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE')
if mode == 'baseline':
    assert hashlib.sha256((installed / 'INI/aliases/sound_alias.ini').read_bytes()).hexdigest() == \
        '8c4aaf4efbc3571b1a78acf99d75e6e66fb662b35c17c9aa1457d6ea89f5426b', \
        'Baseline requires the original packaged aliases and a native build at 9910d769f1aa00ed62a27c053a42a6a48a7debe6.'
    (runtime / 'resource').symlink_to(installed, target_is_directory=True)
else:
    aliases = runtime / 'resource/ini/aliases'
    aliases.mkdir(parents=True)
    (runtime / 'resource/sounds').symlink_to(installed / 'Sounds', target_is_directory=True)
    for path in (installed / 'INI/aliases').iterdir():
        target = aliases / path.name
        if path.name == 'sound_alias.ini':
            target.write_bytes((ROOT / 'src/gameplay/RESOURCE/INI/aliases/sound_alias.ini').read_bytes())
        else:
            target.symlink_to(path, target_is_directory=path.is_dir())
arguments = ['distinct'] if mode.startswith('distinct-') else (['density'] if mode == 'density-after' else [])
if mode in ('cannon-before', 'cannon-after'):
    arguments = ['cannon']
elif mode == 'cannon-effects-control':
    arguments = ['distinct']
result = run([str(SCRATCH / 'replay'), str(runtime), *arguments])
rows = [json.loads(line) for line in result.stdout.splitlines() if line.startswith('{')]
(EVIDENCE / f'service-{mode}.json').write_text(json.dumps(rows, indent=2) + '\n')
print(json.dumps([row for row in rows if row['case'] != 'cannon_distance'] if mode.startswith('cannon-')
                 else (rows if arguments else rows[:3])))
