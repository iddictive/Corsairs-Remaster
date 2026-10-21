import hashlib, json, pathlib, shlex, shutil, subprocess, sys, time

ROOT = pathlib.Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
OWNER = pathlib.Path(__file__).resolve().parent
BUILD = ROOT / 'experiments/native-metal/.cache/build'
TAG = sys.argv[1]
assert TAG in ('baseline', 'fixed')
OUT = OWNER / TAG
OUT.mkdir(exist_ok=True)
assert shutil.disk_usage(ROOT).free > 32 * 1024**3
started = time.monotonic()
def sha(p): return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
def run(args, name):
    with (OUT / name).open('w') as log:
        result = subprocess.run(args, cwd=BUILD, stdout=log, stderr=subprocess.STDOUT, timeout=180)
    if result.returncode: raise RuntimeError((name, result.returncode))

raw = subprocess.run(['ninja', '-C', str(BUILD), '-t', 'commands', 'rigging'],
                     capture_output=True, text=True, check=True).stdout
rows = [shlex.split(x) for x in raw.splitlines() if '/rigging/src/' in x and ' -c ' in x]
assert len(rows) == 7
commands = []
objects = []
for row in rows + [rows[1]]:
    args = list(row)
    source = pathlib.Path(args[args.index('-c') + 1])
    if len(commands) == 7:
        source = OWNER / 'enrolled_probe.cpp'
        args[args.index('-c') + 1] = str(source)
    obj = OUT / (source.stem + '.cpp.o')
    objects.append(str(obj))
    args[args.index('-o') + 1] = str(obj)
    args[args.index('-MF') + 1] = str(obj) + '.d'
    args[args.index('-MT') + 1] = str(obj)
    args = ['-O1' if x == '-O3' else x for x in args]
    args += ['-g', '-fsanitize=address,undefined', '-fno-omit-frame-pointer',
             '-fno-optimize-sibling-calls', '-fno-access-control',
             '-I' + str(ROOT / 'experiments/native-metal/engine/src/libs/rigging/src'),
             '-I' + str(ROOT / 'experiments/native-metal/engine/src/libs/core/src')]
    commands.append(args)
archive = OUT / 'librigging.a'
seed = json.loads((OWNER / 'plan.json').read_text())
link = list(seed['link'])
for i, value in enumerate(link):
    if value == str(OWNER / 'probe'): link[i] = str(OUT / 'probe')
    elif value == str(OWNER / 'probe.cpp.o'): link[i] = objects[-1]
    elif value.endswith('/librigging.a') or value == 'lib/librigging.a': link[i] = str(archive)
link += ['-fsanitize=address,undefined', '-fno-omit-frame-pointer']
inputs = {}
for value in link:
    if value.endswith(('.a', '.dylib')) and value != str(archive):
        path = pathlib.Path(value)
        if not path.is_absolute(): path = BUILD / path
        inputs[str(path)] = sha(path)
for cmd in commands:
    source = cmd[cmd.index('-c') + 1]
    inputs[source] = sha(source)
for path in (ROOT / 'experiments/native-metal/engine/src/libs/rigging/src').glob('*.h'):
    inputs[str(path)] = sha(path)
receipt = dict(tag=TAG, cwd=str(BUILD), commands=commands, link=link, inputs=inputs,
               sanitizer='ASan+UBSan on all7 rigging translation units + enrolled fixture; other libs uninstrumented',
               core='real CoreImpl EntityManager::InsertEntity / GetEntityId / Send_Message; Init bypassed for CPU-only mounting',
               resource_limit='8 sequential TUs,180s each,32GiB host reserve,no game/GPU/SAVE')
(OUT / 'plan.json').write_text(json.dumps(receipt, indent=2) + '\n')
def check():
    for p, h in inputs.items(): assert sha(p) == h, p
check()
for i, args in enumerate(commands): run(args, f'compile-{i}.log')
run(['/usr/bin/ar', '-crs', str(archive)] + objects[:-1], 'archive.log')
run(link, 'link.log')
check()
receipt.update(binary_sha256=sha(OUT / 'probe'), seconds=time.monotonic()-started,
               input_hashes_unchanged=True)
(OUT / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps(dict(binary=receipt['binary_sha256'], seconds=receipt['seconds'], inputs=len(inputs))))
