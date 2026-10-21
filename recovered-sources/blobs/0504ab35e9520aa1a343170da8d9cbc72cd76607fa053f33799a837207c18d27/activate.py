import hashlib
import io
import json
from pathlib import Path
import shlex
import subprocess
import tarfile

ROOT = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
NATIVE = ROOT / 'experiments/native-metal'
BASE = Path(__file__).resolve().parent
REF = '87b4acb4e635af2c18526dcce3d433c8424f13d5'
META = json.loads((BASE / 'metadata.json').read_text())
assert hashlib.sha256(Path(META['driver_source']).read_bytes()).hexdigest() == META['driver_sha256']
assert hashlib.sha256(Path(META['scene_header']).read_bytes()).hexdigest() == META['scene_header_sha256']
OUT = BASE / 'frozen-87b4acb'
OUT.mkdir(exist_ok=True)
assert not (OUT / 'plan.json').exists(), 'a completed packet must not be overwritten'

def digest(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest()

def replace_once(value, old, new):
    assert value.count(old) == 1, old
    return value.replace(old, new)

# Freeze source headers as the same Git tree that produced the canonical libs.
names = subprocess.check_output(['git', 'ls-tree', '-r', '--name-only', REF, '--',
    'experiments/native-metal'], cwd=ROOT, text=True).splitlines()
headers = [p for p in names if Path(p).suffix in ('.h', '.hpp', '.inl', '.ipp', '.rh', '.inc', '.def')]
snapshot = OUT / 'headers'
snapshot.mkdir(exist_ok=True)
payload = subprocess.check_output(['git', 'archive', REF, '--', *headers], cwd=ROOT)
with tarfile.open(fileobj=io.BytesIO(payload)) as archive:
    archive.extractall(snapshot, filter='data')
driver = Path(META['driver_source']).read_text()
driver = replace_once(driver,
    '        destruction_scene::beforeFrame(frame);\n        const auto begin = std::chrono::steady_clock::now();',
    '        const auto begin = std::chrono::steady_clock::now();\n        destruction_scene::beforeFrame(frame);')
(OUT / 'driver.cpp').write_text(driver)
exercise = Path(META['scene_header']).read_text()
exercise = replace_once(exercise,
    '    aimLocal=CVECTOR(hull.boxcenter.x,1.f,hull.boxcenter.z+hull.boxsize.z*.08f);',
    '    const char *aimHeight=std::getenv("DESTRUCTION_AIM_HEIGHT");\n'
    '    aimLocal=CVECTOR(hull.boxcenter.x,aimHeight?std::strtof(aimHeight,nullptr):1.f,hull.boxcenter.z+hull.boxsize.z*.08f);')
exercise = replace_once(exercise,
    '    require(mode()=="baseline" ? firstCutFrame<0 : firstCutFrame>=0,',
    '    require(mode()=="baseline" || std::getenv("DESTRUCTION_AIM_HEIGHT") ? firstCutFrame<0 : firstCutFrame>=0,')
(OUT / 'scene_exercise.hpp').write_text(exercise)

commands = []
for command in META['compile_commands']:
    result = []
    for value in command:
        if value == META['driver_source']:
            value = str(OUT / 'driver.cpp')
        elif value.startswith(str(BASE) + '/'):
            value = str(OUT / Path(value).name)
        elif value.startswith('-I'):
            path = Path(value[2:]).resolve()
            try:
                relative = path.relative_to(ROOT)
            except ValueError:
                relative = None
            if path == BASE.parent:
                value = '-I' + str(OUT)
            elif relative is not None and (str(relative).startswith('experiments/native-metal/engine') or
                    path == NATIVE or (snapshot / relative).is_dir()):
                value = '-I' + str(snapshot / relative)
        result.append(value)
    result += ['-MMD', '-MF', str(OUT / 'driver.d')]
    commands.append(result)
link = [str(OUT / Path(v).name) if v.startswith(str(BASE) + '/') else v for v in META['link_command']]
cwd = Path(META['cwd'])
archives = {}
for value in link:
    if value.endswith(('.a', '.dylib')):
        path = Path(value)
        path = path if path.is_absolute() else cwd / path
        archives[str(path.resolve())] = digest(path)
assert len(archives) >= 49
(OUT / 'archive-before.json').write_text(json.dumps(archives, indent=2) + '\n')
for n, command in enumerate(commands + [link]):
    previous=OUT / f'compile-{n}.log'
    if previous.exists() and not (OUT / f'compile-{n}-first-failure.log').exists():
        (OUT / f'compile-{n}-first-failure.log').write_bytes(previous.read_bytes())
    with (OUT / f'compile-{n}.log').open('w') as log:
        r = subprocess.run(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT, timeout=150)
    print('compile_step', n, 'exit', r.returncode, flush=True)
    if r.returncode:
        raise RuntimeError(f'compile failed: {OUT / f"compile-{n}.log"}')
assert all(digest(p) == expected for p, expected in archives.items())
dependency_text = (OUT / 'driver.d').read_text().replace('\\\n', ' ')
dependencies = shlex.split(dependency_text.split(':', 1)[1])
closure = {}
for value in dependencies:
    path = Path(value)
    path = path if path.is_absolute() else cwd / path
    path = path.resolve()
    assert not str(path).startswith(str(NATIVE / 'engine') + '/'), path
    closure[str(path)] = digest(path)
binary = Path(link[link.index('-o') + 1])
receipt = {'source_commit': REF, 'binary': str(binary), 'binary_sha256': digest(binary),
    'compiled_source_identity': digest(OUT / 'driver.cpp'), 'linked_input_sha256': archives,
    'header_closure': {'files': closure, 'installed_dependency_sha256': {}},
    'input_sha256': {str(OUT / 'driver.cpp'): digest(OUT / 'driver.cpp'),
        str(OUT / 'scene_exercise.hpp'): digest(OUT / 'scene_exercise.hpp')},
    'compile_commands': commands, 'link_command': link, 'cwd': str(cwd),
    'timer': 'CPU wall includes driver action and capture I/O; not fleet FPS acceptance',
    'gpu_run': False}
(OUT / 'plan.json').write_text(json.dumps(receipt, indent=2) + '\n')
runner = Path(META['driver_source']).with_name('run_variant.py').read_text()
old_base = str(Path(META['driver_source']).parent)
assert old_base in runner
runner = runner.replace(old_base, str(OUT))
runner = runner.replace("'36cfbfee46a602030dfdee64ae28c5a6eb6071ef'", repr(REF))
(OUT / 'run_variant.py').write_text(runner)
for name in ('fixture.c', 'ship-type.c'):
    (OUT / name).write_bytes(Path(META['driver_source']).with_name(name).read_bytes())
    receipt['input_sha256'][str(OUT / name)] = digest(OUT / name)
(OUT / 'plan.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps({'output': str(OUT), 'binary': str(binary), 'sha256': receipt['binary_sha256'],
    'archives': len(archives), 'header_dependencies': len(closure), 'gpu_run': False}), flush=True)
