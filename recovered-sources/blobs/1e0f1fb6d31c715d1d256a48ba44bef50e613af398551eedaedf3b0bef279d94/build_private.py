import hashlib, json, os, pathlib, re, shlex, shutil, subprocess, time

OWNER = pathlib.Path(__file__).resolve().parent
REPO = pathlib.Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
SOURCE = OWNER / 'source/experiments/native-metal'
BUILD = pathlib.Path(os.environ['CODEX_SCRATCH_DIR']) / 'build'
FLAGS = '-O1 -g -DNDEBUG -fsanitize=address -fno-omit-frame-pointer -fno-optimize-sibling-calls -fsanitize-address-use-after-scope'
START = time.monotonic()

def run(cmd, name):
    with (OWNER / name).open('w') as out:
        result = subprocess.run(cmd, stdout=out, stderr=subprocess.STDOUT, timeout=max(1, 1750-(time.monotonic()-START)))
    if result.returncode:
        raise RuntimeError(f'{name} failed: {result.returncode}')

def capture(cmd):
    return subprocess.check_output(cmd, text=True)

def sha(path):
    h = hashlib.sha256()
    with pathlib.Path(path).open('rb') as f:
        for chunk in iter(lambda: f.read(1024*1024), b''): h.update(chunk)
    return h.hexdigest()

deps = REPO / 'experiments/native-metal/inputs/native'
cmd = ['/opt/homebrew/bin/cmake', '-S', str(SOURCE), '-B', str(BUILD), '-G', 'Ninja',
       '-DCMAKE_BUILD_TYPE=Release', '-DCMAKE_OSX_ARCHITECTURES=arm64', '-DCMAKE_OSX_DEPLOYMENT_TARGET=15.0',
       '-DSTORM_ENABLE_AUDIO=ON', '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
       f'-DMETAL_DEPENDENCIES={deps}', f'-DCONAN_DATA={REPO}/experiments/native-metal/inputs/conan',
       f'-DSDL2_DIR={deps}/d3d9/sdl2-fixed/lib/cmake/SDL2',
       '-DCMAKE_EXE_LINKER_FLAGS=-fsanitize=address', '-DCMAKE_SHARED_LINKER_FLAGS=-fsanitize=address']
for lang in ('C', 'CXX', 'OBJCXX'): cmd.append(f'-DCMAKE_{lang}_FLAGS_RELEASE={FLAGS}')
(OWNER/'configure-command.json').write_text(json.dumps(cmd, indent=2)+'\n')
run(cmd, 'configure.log')
commands = capture(['/opt/homebrew/bin/ninja', '-C', str(BUILD), '-t', 'commands', 'sea-fleet-benchmark'])
(OWNER/'target-commands.txt').write_text(commands)
target_objects = set()
for line in commands.splitlines():
    parts = shlex.split(line)
    if '-c' in parts and '-o' in parts: target_objects.add(parts[parts.index('-o')+1])
compile_rows = [c for c in json.loads((BUILD/'compile_commands.json').read_text())
                if shlex.split(c['command'])[shlex.split(c['command']).index('-o')+1] in target_objects]
assert len(compile_rows) == 395, len(compile_rows)
assert all('-fsanitize=address' in c['command'] for c in compile_rows)
assert not any('/src/apps/engine/src/main.cpp' in c['file'] for c in compile_rows)
assert all('mimalloc-new-delete.h' not in pathlib.Path(c['file']).read_text(errors='replace') for c in compile_rows)
shutil.copy2(BUILD/'compile_commands.json', OWNER/'compile_commands.json')
(OWNER/'target-compile-commands.json').write_text(json.dumps(compile_rows, indent=2)+'\n')
(OWNER/'configured.json').write_text(json.dumps({'build':str(BUILD),'compile_commands':len(compile_rows),'commands':len(commands.splitlines()),'status':'waiting for root compile slot'},indent=2)+'\n')
print(f'Configured {len(compile_rows)} private ASan TUs. Waiting for compile slot.', flush=True)
while not (OWNER/'compile-slot-granted').exists():
    if time.monotonic()-START > 600: raise RuntimeError('Root compile slot not granted within bounded wait')
    time.sleep(1)
run(['/opt/homebrew/bin/cmake','--build',str(BUILD),'--target','sea-fleet-benchmark','-j','2'], 'build.log')
binary = BUILD/'bin/sea-fleet-benchmark'
shutil.copy2(binary, OWNER/'sea-fleet-benchmark-asan')
run(['/usr/bin/dsymutil',str(binary),'-o',str(OWNER/'sea-fleet-benchmark-asan.dSYM')], 'dsymutil.log')
objects = [BUILD / shlex.split(c['command'])[shlex.split(c['command']).index('-o')+1] for c in compile_rows]
instrumentation = {}
for obj in objects:
    symbols = capture(['/usr/bin/nm','-u',str(obj)])
    instrumentation[str(obj.relative_to(BUILD))] = {'sha256':sha(obj),'asan': '__asan_' in symbols}
assert all(v['asan'] for v in instrumentation.values())
(OWNER/'object-instrumentation.json').write_text(json.dumps(instrumentation,indent=2)+'\n')
dependencies = capture(['/opt/homebrew/bin/ninja','-C',str(BUILD),'-t','deps'])
(OWNER/'compiler-dependencies.txt').write_text(dependencies)
paths = {pathlib.Path(line.strip()) for line in dependencies.splitlines() if line.startswith('    ')}
paths.update(pathlib.Path(c['file']) for c in compile_rows)
paths.update([SOURCE/'CMakeLists.txt', SOURCE/'engine/CMakeLists.txt', SOURCE/'script_vm_probe.cmake'])
hashes = {str(p):sha(p) for p in sorted(paths) if p.is_file()}
(OWNER/'input-hashes.json').write_text(json.dumps(hashes,indent=2)+'\n')
link = commands.splitlines()[-1]
parts=shlex.split(link)
archives=[str((BUILD/pathlib.Path(p)).resolve()) for p in parts if p.endswith('.a') and not p.startswith('-')]
force_archives=re.findall(r'-force_load,([^\s]+\.a)',link)
archives+= [str((BUILD/pathlib.Path(p)).resolve()) for p in force_archives]
archives=sorted(set(archives))
assert len(archives)==49,len(archives)
assert all(pathlib.Path(a).is_relative_to(BUILD) for a in archives)
archive_hashes={a:sha(a) for a in archives}
libs=capture(['/usr/bin/otool','-L',str(binary)])
(OWNER/'linked-dylibs.txt').write_text(libs)
assert 'libclang_rt.asan_osx_dynamic.dylib' in libs
nm=capture(['/usr/bin/nm','-m',str(binary)])
(OWNER/'binary-symbols.txt').write_text(nm)
new_lines=[line for line in nm.splitlines() if re.search(r'\b__Zna|\b__Znw|\b__Zda|\b__Zdl',line)]
assert new_lines and all('(undefined)' in line for line in new_lines),new_lines
receipt={'status':'build_complete_no_gpu_execution','binary_sha256':sha(OWNER/'sea-fleet-benchmark-asan'),
 'binary_bytes':binary.stat().st_size,'compile_TUs':len(compile_rows),'instrumented_TUs':sum(v['asan'] for v in instrumentation.values()),
 'private_archives':archive_hashes,'input_hash_count':len(hashes),'new_delete_symbols':new_lines,
 'elapsed_seconds':time.monotonic()-START,'coverage_limit':'No BattleInterface/HUD runtime in stock fixture; no fleet/GPU execution authorized in this phase',
 'scratch_build':str(BUILD),'scratch_lifecycle':'codex-run-scratch owns teardown after producer exits'}
(OWNER/'build-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({k:v for k,v in receipt.items() if k not in ('private_archives','new_delete_symbols')}),flush=True)
