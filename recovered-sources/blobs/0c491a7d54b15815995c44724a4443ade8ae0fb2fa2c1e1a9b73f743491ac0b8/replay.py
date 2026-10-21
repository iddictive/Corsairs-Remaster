"""Private installed-batch replay; never builds or controls the player app."""
from pathlib import Path
import hashlib
import io
import json
import os
import shlex
import shutil
import subprocess
import sys
import tarfile

repo = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
native = repo / 'experiments/native-metal'
build = native / '.cache/build'
evidence = Path(__file__).resolve().parent
scratch = Path(os.environ['CODEX_SCRATCH_DIR']).resolve()
reference = '6e2cc7b8b6ed115c3446921066b97b15df782aab'
retained = evidence / 'frozen-inputs-6e2cc7b'
library_root = retained / 'libraries' if (retained / 'libraries').is_dir() else build

def player_idle():
    listing = subprocess.check_output(['ps', '-axo', 'pid=,comm='], text=True)
    player = '/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/metal-engine'
    assert not any(row.strip().endswith(player) for row in listing.splitlines()), 'player active'

def run(command, name, cwd, env=None, enforce_idle=False):
    (evidence / (name + '-command.json')).write_text(json.dumps(command, indent=2) + '\n')
    with (evidence / (name + '.log')).open('w') as log:
        process = subprocess.Popen(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT, env=env)
        try:
            while process.poll() is None:
                if enforce_idle:
                    player_idle()
                try:
                    process.wait(timeout=.5)
                except subprocess.TimeoutExpired:
                    pass
        except BaseException:
            process.terminate()
            process.wait(timeout=10)
            raise
    print(name, 'exit', process.returncode, flush=True)
    if process.returncode:
        print((evidence / (name + '.log')).read_text()[-6000:], flush=True)
    return process.returncode

player_idle()
paths = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', reference,
                                 'experiments/native-metal'], cwd=repo, text=True).splitlines()
paths = [p for p in paths if '/engine/src/' in p or '/third_party/' in p or
         (str(Path(p).parent) == 'experiments/native-metal' and Path(p).suffix == '.hpp')]
snapshot = scratch / 'frozen'
snapshot.mkdir()
payload = subprocess.check_output(['git', 'archive', reference, '--', *paths], cwd=repo)
with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
    archive.extractall(snapshot, filter='data')
frozen = snapshot / 'experiments/native-metal'
lines = json.loads((retained / 'command-template.json').read_text()) if (retained / 'command-template.json').exists() else subprocess.check_output([
    str(native / 'inputs/toolchain/bin/ninja'), '-C', str(build), '-t', 'commands', 'sea-fleet-benchmark'], text=True).splitlines()
compiles = [line for line in lines if ' -c ' in line and '/sea_fleet_benchmark.cpp' in line]
links = [line for line in lines if ' -o bin/sea-fleet-benchmark ' in line]
assert len(compiles) == len(links) == 1
original_object = 'native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o'
libraries = {str(p.relative_to(library_root)): hashlib.sha256(p.read_bytes()).hexdigest()
             for p in (library_root / 'lib').glob('*.a')}
binding = evidence / 'frozen-binding.json'
if binding.exists():
    prior = json.loads(binding.read_text())
    assert prior['ref'] == reference and prior['libraries'] == libraries, 'native libraries changed: frozen replay is no longer ABI-matched'
(evidence / 'frozen-binding.json').write_text(json.dumps({
    'ref': reference, 'snapshot_paths': paths, 'libraries': libraries,
    'installed_engine_sha256': hashlib.sha256(Path('/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/metal-engine').read_bytes()).hexdigest(),
    'game_control': False, 'player_save': False, 'acceptance': 'diagnostic-only',
}, indent=2) + '\n')

def fixture(kind):
    source = retained / kind if (retained / kind).is_dir() else repo / '.local-archives/metal-renderer-evidence' / (
        'phonk-auto-scene-20261016' if kind == 'ship' else 'phonk-world-async-20261016')
    local = scratch / kind
    local.mkdir()
    for p in source.iterdir():
        if p.is_file() and p.suffix in ('.cpp', '.hpp', '.c', '.ini'):
            shutil.copy2(p, local / p.name)
    (local / 'RESOURCE').symlink_to('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE', target_is_directory=True)
    probe = local / 'probe.cpp'
    text = probe.read_text()
    assert text.count('setenv("STORM_QUIET_CLIENT","1",1);') + text.count('setenv("STORM_QUIET_CLIENT", "1", 1);') == 1
    text = text.replace('setenv("STORM_QUIET_CLIENT","1",1);', 'setenv("STORM_QUIET_CLIENT","0",1);')
    text = text.replace('setenv("STORM_QUIET_CLIENT", "1", 1);', 'setenv("STORM_QUIET_CLIENT", "0", 1);')
    text = text.replace('spdlog::set_level(spdlog::level::off);', 'spdlog::set_level(spdlog::level::info);')
    probe.write_text(text)
    shutil.copy2(probe, evidence / (kind + '-probe.cpp'))
    if kind == 'ship':
        ini = local / 'engine.ini'
        settings = ini.read_text()
        assert settings.count('screen_x=1920') == settings.count('screen_y=1200') == 1
        ini.write_text(settings.replace('screen_x=1920', 'screen_x=960').replace('screen_y=1200', 'screen_y=600'))
    compile = shlex.split(compiles[0])
    for flag in ('-MD', '-MT', '-MF'):
        at = compile.index(flag)
        del compile[at:at + (1 if flag == '-MD' else 2)]
    for index, value in enumerate(compile):
        if value.startswith('-I' + str(native)):
            old = Path(value[2:]).resolve()
            if old == native or old.is_relative_to(native / 'engine/src'):
                compile[index] = '-I' + str(frozen / old.relative_to(native))
    obj = local / (kind + '.o')
    compile[compile.index(str(native / 'sea_fleet_benchmark.cpp'))] = str(probe)
    compile[compile.index(original_object)] = str(obj)
    compile.remove('-v')
    compile += ['-UNDEBUG', '-I' + str(frozen / 'engine/src/libs/sea_ai/src')]
    link = shlex.split(links[0])
    assert link[:3] == [':', '&&', compile[0]] and link[-2:] == ['&&', ':']
    link = link[2:-2]
    link[link.index(original_object)] = str(obj)
    binary = local / 'native-replay'
    link[link.index('bin/sea-fleet-benchmark')] = str(binary)
    link.remove('-v')
    for index, value in enumerate(link):
        path = Path(value)
        if not value.endswith('.a'):
            continue
        relative = path.relative_to(build) if path.is_absolute() and path.is_relative_to(build) else path
        if (library_root / relative).is_file():
            link[index] = str(library_root / relative)
    if kind == 'ship' and len(sys.argv) > 2 and sys.argv[2] == 'ram-diagnostic':
        diagnostic = frozen / 'engine/src/libs/ship/src/ship_phonk_laser.cpp'
        code = diagnostic.read_text()
        first = code.index('bool SHIP::StartPhonkLaserWork()')
        last = code.index('void SHIP::PollPhonkLaserWork', first)
        method = code[first:last]
        assert method.count('return false;') == 5
        method = method.replace('return false;', 'return diagnosticCutReject(__LINE__);')
        capture = '        if (!node->geo->CaptureLaserInput('
        assert method.count(capture) == 1
        method = method.replace(capture, '''        const auto diagnosticField = nodeCutField(model->mtx, node, plane);
        float diagnosticRadius = 0.f;
        for (const auto &beam : diagnosticField.beams) diagnosticRadius = std::max(diagnosticRadius, beam.radius);
        core.Trace("Diagnostic cut snapshot node=%d name=%s radius=%.9g valid=%d", n, target.name.c_str(), diagnosticRadius, int(storm_ship_laser::valid(diagnosticField)));
''' + capture)
        code = code[:first] + 'static bool diagnosticCutReject(int line) { core.Trace("Diagnostic cut snapshot rejection line=%d", line); return false; }\n' + method + code[last:]
        diagnostic.write_text(code)
        shutil.copy2(diagnostic, evidence / 'ram-diagnostic-source.cpp')
        commands = subprocess.check_output([str(native / 'inputs/toolchain/bin/ninja'), '-C', str(build),
            '-t', 'commands', 'native-engine/storm/libs/ship/CMakeFiles/ship.dir/src/ship_phonk_laser.cpp.o'], text=True).splitlines()
        commands = [shlex.split(line) for line in commands if ' -c ' in line and line.endswith('/ship_phonk_laser.cpp')]
        assert len(commands) == 1
        diagnostic_compile = commands[0]
        for flag in ('-MD', '-MT', '-MF'):
            at = diagnostic_compile.index(flag)
            del diagnostic_compile[at:at + (1 if flag == '-MD' else 2)]
        for index, value in enumerate(diagnostic_compile):
            if value.startswith('-I' + str(native)):
                old = Path(value[2:]).resolve()
                if old == native or old.is_relative_to(native / 'engine/src'):
                    diagnostic_compile[index] = '-I' + str(frozen / old.relative_to(native))
        diagnostic_compile[diagnostic_compile.index(str(native / 'engine/src/libs/ship/src/ship_phonk_laser.cpp'))] = str(diagnostic)
        diagnostic_compile[diagnostic_compile.index('-o') + 1] = str(local / 'ship_phonk_laser.cpp.o')
        if '-v' in diagnostic_compile:
            diagnostic_compile.remove('-v')
        assert run(diagnostic_compile, 'ram-diagnostic-compile', build) == 0
        archive = local / 'libship.a'
        shutil.copy2(library_root / 'lib/libship.a', archive)
        assert run(['/usr/bin/ar', 'r', str(archive), str(local / 'ship_phonk_laser.cpp.o')], 'ram-diagnostic-archive', build) == 0
        link = [str(archive) if value == str(library_root / 'lib/libship.a') else value for value in link]
    assert run(compile, kind + '-compile', build) == 0
    assert run(link, kind + '-link', build) == 0
    return local, binary

kind = sys.argv[1] if len(sys.argv) > 1 else 'ship'
scenario = sys.argv[2] if len(sys.argv) > 2 else 'manual'
local, binary = fixture(kind)
player_idle()
env = dict(os.environ)
env['STORM_METAL_FRAME_TRACE'] = str(local / 'frame-trace.csv')
env['STORM_METAL_PROFILE'] = '1'
if kind == 'ship':
    assert scenario in ('manual', 'auto', 'ram', 'ram-diagnostic', 'baseline')
    env.update(DESTRUCTION_SCENE_MODE='ram' if scenario.startswith('ram') else scenario if scenario == 'baseline' else 'laser')
    if scenario == 'auto':
        env['DESTRUCTION_AUTO'] = '1'
    result = run([str(binary), str(local), '401', '960', '600'], 'ship-' + scenario, local, env, enforce_idle=True)
else:
    result = run([str(binary), str(local), 'Antigua', '1'], 'world-antigua', local, env, enforce_idle=True)
prefix = kind + '-' + scenario if kind == 'ship' else kind
for name in ('scene-states.csv', 'cpu-frames.csv', 'frame-trace.csv'):
    if (local / name).exists():
        shutil.copy2(local / name, evidence / (prefix + '-' + name))
for p in local.glob('*.ppm'):
    shutil.copy2(p, evidence / (prefix + '-' + p.name))
logs = local / 'userdata/Logs'
if logs.is_dir():
    destination = evidence / (kind + '-runtime-logs')
    destination.mkdir(exist_ok=True)
    for p in logs.rglob('*'):
        if p.is_file() and p.stat().st_size < 16 * 1024 * 1024:
            relative = p.relative_to(logs)
            (destination / relative).parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(p, destination / relative)
print('diagnostic_result', result, 'player_acceptance=0', flush=True)
raise SystemExit(result)
