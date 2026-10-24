"""One-off real-VM evidence for retained map fleet admission, in managed scratch.

Scene creation is a counted fixture seam, not rendered-game acceptance. The
ordinary importer, encounter dispatch and whole-battle admission bodies execute.
"""
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
WORK = Path(os.environ['CODEX_SCRATCH_DIR'])
sys.path.insert(0, str(ROOT / 'tools'))
import gameplay_sources

mode = sys.argv[1]
assert mode in ('before', 'after', 'compile')
program = WORK / 'program'
shutil.copytree(ROOT / 'experiments/native-metal/.cache/runtime/PROGRAM', program)
sources, _ = gameplay_sources.read()
baseline_commit = 'f2dc48016e91f983e39a864b03c8254ad7b09de7'
sea_source = 'PROGRAM/sea_ai/sea.c'
if mode == 'before':
    _, legacy, encoding = sources[sea_source]
    original = subprocess.check_output(
        ['git', 'show', baseline_commit + ':src/gameplay/' + sea_source], cwd=ROOT)
    sources[sea_source] = (gameplay_sources.runtime_bytes(sea_source, original, encoding),
                           legacy, encoding)
for name, (data, _, _) in sources.items():
    if name.startswith('PROGRAM/'):
        target = program / name[8:]
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
binary = WORK / 'script-vm-probe'
shutil.copy2(ROOT / 'experiments/native-metal/.cache/build/bin/script-vm-probe', binary)
shutil.copytree(ROOT / 'experiments/native-metal/engine/src/libs/shared_headers/include/shared',
                WORK / 'resource/shared')
files = {p: p.read_bytes().decode('latin1') for p in program.rglob('*.c')}
main = program / 'seadogs.c'
files[main], n = re.subn(r'void\s+Main\s*\(\s*\)', 'void GameMainNotExecuted()', files[main])
assert n == 1
fixture = Path(__file__).with_name('traffic-admission-fixture.c.txt').read_text()
if mode == 'compile':
    fixture = '#include "seadogs.c"\nint Run(){return 0;}\n'
else:
    seams = ('WdmTrafficEnsureRoster', 'WdmTrafficConsumeSupplies',
             'WdmFleetSeaAssemblyReady', 'SeaTraffic_Mount',
             'SeaTraffic_HasOriginalGroup', 'SeaTraffic_RegisterHarbours',
             'WdmFleetSeaApplyTask', 'SeaTraffic_Notify',
             'SeaTraffic_LoginRegionGroups', 'SeaTraffic_ReportPositions')
    for name in seams:
        matches = 0
        for path, body in files.items():
            body, count = re.subn(r'(\b(?:void|bool|int|ref|float|string)\s+)' + name + r'(\s*\()',
                                  r'\1Unused_' + name + r'\2', body)
            matches += count
            files[path] = body
        assert matches == 1, (name, matches)
    sea = program / 'sea_ai/sea.c'
    start = files[sea].index('void Sea_LoginEncounterGroups(')
    end = files[sea].index('bool Sea_GroupHasMountedShips(', start)
    body = files[sea][start:end].replace('ReloadProgressUpdate()', 'FixtureProgress()')
    body = body.replace('WdmFleetSeaSnapshotSides()', 'FixtureProgress()')
    files[sea] = files[sea][:start] + body + files[sea][end:]
for path, body in files.items():
    path.write_bytes(body.encode('latin1'))
(program / 'cases.c').write_text(fixture)
started = time.perf_counter()
result = subprocess.run([str(binary), str(program), 'Run', '0'],
                        cwd=WORK, capture_output=True, text=True)
record = {'mode': mode, 'seconds': time.perf_counter() - started,
          'baseline_commit': baseline_commit,
          'runtime_source_sha256': {name: hashlib.sha256(sources[name][0]).hexdigest()
                                    for name in (sea_source, 'PROGRAM/sea_ai/sea_traffic.c')},
          'probe_sha256': hashlib.sha256(binary.read_bytes()).hexdigest(),
          'fixture_sha256': hashlib.sha256(fixture.encode()).hexdigest(),
          'exit': result.returncode,
          'stdout': result.stdout, 'stderr': result.stderr,
          'logs': {p.name: p.read_text(errors='replace') for p in
                   (program / '.vm-userdata/Logs').glob('*') if p.stat().st_size}}
Path(__file__).with_name('traffic-admission-' + mode + '.json').write_text(
    json.dumps(record, indent=2, ensure_ascii=False) + '\n')
print(json.dumps(record, ensure_ascii=False))
raise SystemExit(result.returncode)
