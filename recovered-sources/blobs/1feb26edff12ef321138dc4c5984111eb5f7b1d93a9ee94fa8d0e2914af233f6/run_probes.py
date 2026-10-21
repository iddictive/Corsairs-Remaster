from pathlib import Path
import hashlib
import json
import subprocess

repo = Path.cwd()
out = Path(__file__).resolve().parent
native = repo / 'experiments/native-metal'
objects = native / '.cache/build/native-engine/storm/libs/geometry/CMakeFiles/geometry.dir'
link = [objects / 'src/world_cut_planar.cpp.o', objects / 'third_party/clipper2/src/clipper.engine.cpp.o', objects / 'third_party/clipper2/src/clipper.triangulation.cpp.o']
base = '98313f9'
headers = {'material': 'experiments/native-metal/ship_hull_material.hpp', 'contact': 'experiments/native-metal/engine/src/libs/touch/src/ship_contact_frame.hpp'}
receipt = {'baseline': base, 'meaning': 'CPU source falsifiers only; no engine build, staging, or player replay', 'cases': [], 'source_sha256': {}}
for kind, path in headers.items():
    receipt['source_sha256'][path] = hashlib.sha256((repo / path).read_bytes()).hexdigest()
    prior = out / (kind + '_baseline.hpp')
    prior.write_bytes(subprocess.check_output(['git', 'show', base + ':' + path], cwd=repo))
    for baseline in [True, False]:
        label = kind + ('_baseline' if baseline else '_fixed')
        command = ['clang++', '-std=c++20', '-O1', '-I', str(native)]
        if baseline:
            command += ['-D' + kind.upper() + '_HEADER="' + str(prior) + '"']
        command += [str(out / (kind + '_probe.cpp'))]
        if kind == 'material':
            command += [str(p) for p in link] + ['-Wl,-dead_strip']
        command += ['-o', str(out / label)]
        build = subprocess.run(command, cwd=repo, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (out / (label + '-compile.txt')).write_text(build.stdout)
        if build.returncode:
            raise SystemExit(build.stdout)
        result = subprocess.run([str(out / label)], cwd=repo, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
        (out / (label + '.txt')).write_text(result.stdout)
        receipt['cases'].append({'label': label, 'command': command, 'run': [str(out / label)], 'exit': result.returncode, 'stdout': label + '.txt'})
        print(label, 'exit', result.returncode)
for p in link:
    receipt['source_sha256'][str(p.relative_to(repo))] = hashlib.sha256(p.read_bytes()).hexdigest()
for name in ['material_probe.cpp', 'contact_probe.cpp', 'run_probes.py']:
    receipt['source_sha256'][name] = hashlib.sha256((out / name).read_bytes()).hexdigest()
(out / 'receipt.json').write_text(json.dumps(receipt, indent=2) + '\n')
assert [c['exit'] for c in receipt['cases']] == [5, 0, 1, 0], receipt['cases']
