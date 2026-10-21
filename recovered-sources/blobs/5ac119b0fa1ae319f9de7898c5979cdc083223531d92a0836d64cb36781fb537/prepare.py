"""One-off extension of the existing current-ABI CORE driver; no product build."""
from pathlib import Path
import hashlib
import json
import os
import re
import shlex
import shutil
import subprocess
import sys
import time

repo = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
here = Path(__file__).resolve().parent
base = repo / '.local-archives/metal-renderer-evidence/phonk-current-20261016'
build = repo / 'experiments/native-metal/.cache/build'
weapon = sys.argv[1]
assert weapon in ('impact', 'laser', 'blast')
action = sys.argv[2] if len(sys.argv) > 2 else '--prepare'
assert action in ('--prepare', '--syntax-only', '--run')
scratch = Path(os.environ['CODEX_SCRATCH_DIR']).resolve(strict=True)
assert scratch.is_relative_to(Path('/private/var/folders')) or scratch.is_relative_to(Path('/tmp').resolve())
local = scratch / ('secondary-' + weapon)
local.mkdir()
for path in (base / 'manual-inputs').iterdir():
    if path.is_file() and path.suffix in ('.cpp', '.hpp', '.c', '.ini'):
        shutil.copy2(path, local / path.name)
(local / 'scene_exercise.hpp').rename(local / 'scene_base.hpp')
shutil.copy2(here / 'scene_exercise.hpp', local / 'scene_exercise.hpp')
(local / 'RESOURCE').symlink_to('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE', target_is_directory=True)
probe = local / 'probe.cpp'
value = probe.read_text()
old = 'if (frames > 1 && !destruction_scene::automatic() && !StormMetalRawSkinnedDraws(device)) return 74;'
assert value.count(old) == 1
# The fixture deliberately puts orphan material away from either original hull.
# Animated crew draw count cannot be a geometry-damage oracle at that camera.
value = value.replace(old, 'if (!destruction_secondary::completed()) return 74;')
probe.write_text(value)
compile = json.loads((base / 'manual-compile-command.json').read_text())
# Including the concrete SHIP class requires its producer's full ABI defines.
# The generic benchmark only used SHIP_BASE and omits target-private features.
ninja = (build / 'build.ninja').read_text()
ship_rule = re.search(r'^build .*?/ship_phonk_laser.cpp\.o:.*?\n(?P<body>(?:  .*\n)+)', ninja, re.M)
assert ship_rule
ship_variables = dict(re.findall(r'^  (\w+) = (.*)$', ship_rule['body'], re.M))
ship_defines = [p for p in shlex.split(ship_variables['DEFINES'] + ' ' + ship_variables['FLAGS']) if p.startswith('-D')]
compile = [p for p in compile if not p.startswith('-D')]
compile[1:1] = ship_defines
old_obj = compile[compile.index('-o') + 1]
old_source = compile[compile.index('-c') + 1]
compile[compile.index(old_obj)] = str(local / 'probe.o')
compile[compile.index(old_source)] = str(probe)
compile += ['-I' + str(repo / 'experiments/native-metal/engine/src/libs/ship/src'), '-MMD', '-MF', str(local / 'probe.d')]
link = json.loads((base / 'manual-link-command.json').read_text())
assert link.count(old_obj) == 1
link[link.index(old_obj)] = str(local / 'probe.o')
link[link.index('-o') + 1] = str(local / 'replay')
native = [str(local / 'replay'), str(local), '320', '960', '600']
env = dict(os.environ, DESTRUCTION_SCENE_MODE='baseline', DEBRIS_NATIVE_WEAPON=weapon,
           STORM_METAL_FRAME_TRACE=str(local / 'frame-trace.csv'), STORM_METAL_PROFILE='1')
for name in ('DESTRUCTION_AUTO', 'DESTRUCTION_NUCLEAR', 'DESTRUCTION_AIM_HEIGHT', 'DESTRUCTION_NO_CUT'):
    env.pop(name, None)
runner = Path(__file__).read_bytes()
digest = hashlib.sha256(value.encode() + (local / 'scene_exercise.hpp').read_bytes() + weapon.encode() + runner + json.dumps(ship_defines).encode()).hexdigest()[:16]
retained = here / 'runs' / (weapon + '-' + action.removeprefix('--') + '-' + digest)
retained.mkdir(parents=True, exist_ok=False)
inputs = retained / 'inputs'
inputs.mkdir()
for path in local.iterdir():
    if path.is_file():
        shutil.copy2(path, inputs / path.name)
shutil.copy2(Path(__file__), inputs / 'prepare.py')
receipt = {'weapon': weapon, 'action': action, 'compile': compile, 'link': link, 'native': native,
           'ship_abi_defines': ship_defines, 'abi_define_owner': 'build.ninja ship_phonk_laser.cpp.o',
           'environment_delta': {k: env[k] for k in ('DESTRUCTION_SCENE_MODE', 'DEBRIS_NATIVE_WEAPON', 'STORM_METAL_PROFILE')},
           'baseline_git_ref': subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=repo, text=True).strip(),
           'compiler': subprocess.check_output([compile[0], '--version'], text=True),
           'native_receipt': json.loads((repo / 'experiments/native-metal/.cache/gameplay-compiler.json').read_text()),
           'libraries': {str(p): hashlib.sha256(p.read_bytes()).hexdigest() for p in build.rglob('*.a')},
           'inputs': {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs.iterdir()},
           'limitation': 'Private native CORE; fixture separating cut seeds the real token. Impact/Blast bridges and actual manual laser. No full AIBalls ballistic callback chain, player scene, SAVE or product build.'}

def save():
    (retained / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')

def run(command, label, gpu=False):
    def idle():
        processes = subprocess.check_output(['ps', '-axo', 'comm='], text=True).splitlines()
        assert not any(p.strip().endswith('/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/metal-engine') for p in processes), 'player active'
    if gpu: idle()
    with (retained / (label + '.log')).open('w') as log:
        process = subprocess.Popen(command, cwd=build if not gpu else local, env=env, stdout=log, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + (150 if gpu else 90)
        try:
            while process.poll() is None:
                if gpu: idle()
                assert time.monotonic() < deadline, 'bounded native fixture timeout'
                try: process.wait(timeout=.5)
                except subprocess.TimeoutExpired: pass
        except BaseException:
            process.terminate(); process.wait(timeout=10)
            raise
    receipt.setdefault('exit_codes', {})[label] = process.returncode
    save()
    assert process.returncode == 0, str(retained / (label + '.log'))

save()
print(json.dumps({'retained': str(retained), 'local': str(local), 'action': action}), flush=True)
if action == '--syntax-only':
    syntax = compile.copy()
    for flag in ('-o', '-MF'):
        at = syntax.index(flag)
        del syntax[at:at + 2]
    syntax.remove('-c'); syntax.remove('-MMD'); syntax += ['-fsyntax-only']
    receipt['syntax_command'] = syntax
    run(syntax, 'syntax')
elif action == '--run':
    model = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/models/ships/Flyingdutchman1')
    geometry = {str(p.relative_to(model)): hashlib.sha256(p.read_bytes()).hexdigest() for p in model.rglob('*') if p.is_file() and p.suffix.lower() == '.gm'}
    assert geometry, 'actual model inputs missing'
    asset_digest = hashlib.sha256(json.dumps(geometry, sort_keys=True).encode()).hexdigest()
    asset_copy = here / 'assets' / asset_digest
    for name in geometry:
        destination = asset_copy / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        if not destination.exists(): shutil.copy2(model / name, destination)
        assert hashlib.sha256(destination.read_bytes()).hexdigest() == geometry[name]
    receipt['retained_model_geometry'] = {'path': str(asset_copy.relative_to(here)), 'sha256': geometry}
    save()
    run(compile, 'compile')
    # Preserve exact local/project header inputs reported by this compiler.
    dependencies = (local / 'probe.d').read_text().replace('\\\n', ' ').split(':', 1)[1].split()
    for raw in dependencies:
        source = Path(raw)
        if source.is_file() and source.is_relative_to(repo):
            destination = inputs / 'repo' / source.relative_to(repo)
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
    assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest() == h for p, h in receipt['libraries'].items()), 'native library closure changed during fixture compilation'
    run(link, 'link')
    try:
        run(native, 'native', True)
    finally:
        for path in local.iterdir():
            if path.is_file() and path.suffix in ('.csv', '.ppm', '.d'):
                shutil.copy2(path, retained / path.name)
        logs = local / 'userdata/Logs'
        if logs.exists():
            shutil.copytree(logs, retained / 'logs', dirs_exist_ok=True)
        receipt['retained_sha256'] = {str(p.relative_to(retained)): hashlib.sha256(p.read_bytes()).hexdigest() for p in retained.rglob('*') if p.is_file() and p.name != 'receipt.json'}
        save()
print('secondary_hook_' + action.removeprefix('--') + '=complete', flush=True)
