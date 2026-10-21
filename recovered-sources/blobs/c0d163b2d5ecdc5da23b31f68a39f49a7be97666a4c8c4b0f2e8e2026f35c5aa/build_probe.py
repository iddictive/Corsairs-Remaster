import hashlib, json, os, pathlib, shlex, shutil, subprocess, time

ROOT = pathlib.Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
OWNER = pathlib.Path(__file__).resolve().parent
OLD = ROOT / '.local-archives/metal-renderer-evidence/full-asan-native-fleet'
STAGED = ROOT / '.local-archives/metal-renderer-evidence/layered-installed-acceptance'
PLAN = json.loads((STAGED / 'plan.json').read_text())
ROWS = json.loads((OLD / 'target-compile-commands.json').read_text())
OLD_SOURCE = str(OLD / 'source/experiments/native-metal')
SOURCE = str(ROOT / 'experiments/native-metal')
started = time.monotonic()
assert shutil.disk_usage(ROOT).free > 32 * 1024**3
def sha(p): return hashlib.sha256(pathlib.Path(p).read_bytes()).hexdigest()
inputs = json.loads((STAGED / 'linked-libraries.json').read_text())
def check():
    for p, h in inputs.items(): assert sha(p) == h, p
def run(args, name):
    with (OWNER / name).open('w') as log:
        result = subprocess.run(args, cwd=PLAN['cwd'], stdout=log, stderr=subprocess.STDOUT, timeout=120)
    if result.returncode: raise RuntimeError((name, result.returncode))
commands = []
for stem, source in [('rope', SOURCE + '/engine/src/libs/rigging/src/rope.cpp'),
                     ('sailone', SOURCE + '/engine/src/libs/rigging/src/sailone.cpp'),
                     ('probe', str(OWNER / 'probe.cpp'))]:
    suffix = '/rigging/src/' + stem + '.cpp' if stem != 'probe' else '/sea_fleet_benchmark.cpp'
    row = next(x for x in ROWS if x['file'].endswith(suffix))
    args = [x.replace(OLD_SOURCE, SOURCE) for x in shlex.split(row['command'])]
    args[args.index('-c') + 1] = source
    args[args.index('-o') + 1] = str(OWNER / (stem + '.cpp.o'))
    args += ['-fno-access-control', '-MMD', '-MF', str(OWNER / (stem + '.d')),
             '-I' + SOURCE + '/engine/src/libs/rigging/src']
    commands.append(args)
    inputs[source] = sha(source)
archive = OWNER / 'librigging.a'
link = list(PLAN['link'])
old_binary = str(STAGED / 'sea-fleet-msaa')
old_object = str(STAGED / 'driver.cpp.o')
for i, value in enumerate(link):
    if value == old_binary: link[i] = str(OWNER / 'probe')
    elif value == old_object: link[i] = str(OWNER / 'probe.cpp.o')
    elif value.endswith('/librigging.a') or value == 'lib/librigging.a': link[i] = str(archive)
link += ['-fsanitize=address', '-fno-omit-frame-pointer']
assert str(OWNER / 'probe.cpp.o') in link and str(OWNER / 'probe') in link
receipt = {'purpose': 'Actual ROPE deletion then original SAILONE wave write; CPU only, no game/GPU/SAVE',
           'allocator': 'system new/delete + ASan; two original instrumented TUs, remaining libraries uninstrumented',
           'resource_limit': '3 TUs sequential,120s each, estimated<200MiB retained,32GiB host reserve',
           'commands': commands, 'link': link, 'inputs': inputs}
(OWNER / 'plan.json').write_text(json.dumps(receipt, indent=2) + '\n')
check()
for i, command in enumerate(commands): run(command, f'compile-{i}.log')
shutil.copy2(pathlib.Path(PLAN['cwd']) / 'lib/librigging.a', archive)
run(['/usr/bin/ar', '-r', str(archive), str(OWNER / 'rope.cpp.o'), str(OWNER / 'sailone.cpp.o')], 'archive.log')
run(link, 'link.log')
check()
receipt.update(binary_sha256=sha(OWNER / 'probe'), seconds=time.monotonic()-started,
               input_hashes_unchanged=True)
(OWNER / 'build-receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
print(json.dumps({'binary': receipt['binary_sha256'], 'seconds': receipt['seconds']}))
