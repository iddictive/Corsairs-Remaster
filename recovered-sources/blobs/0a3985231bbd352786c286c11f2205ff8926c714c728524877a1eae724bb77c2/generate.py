#!/usr/bin/env python3
"""One-off real SHIP queue bodies with injected prepare and fake GEOM/publication.
Source/CPU state-machine evidence only; no GPU, app, player SAVE, or frame-time proof.
"""
from pathlib import Path
import hashlib,json,subprocess,tempfile
HERE=Path(__file__).resolve().parent
ROOT=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
SOURCE=ROOT/'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.cpp'
MARKERS=['bool finite(const CVECTOR &v)','storm_ship_laser::Plane cutField(','void returnImpact(','storm_ship_laser::Plane nodeCutField(','float contourArea(','void SHIP::CancelPhonkLaserWork()','bool SHIP::ApplyPhonkLaserCut(','bool SHIP::StartPhonkLaserWork()','void SHIP::PollPhonkLaserWork(']
def extract(source,marker):
    start=source.index(marker);begin=source.index('{',start);depth=1;end=begin+1
    while depth:
        depth+=(source[end]=='{')-(source[end]=='}');end+=1
    if marker.startswith('bool SHIP::Start'):
        tail='\ncatch (const std::exception &) { return false; }'
        assert source[end:end+len(tail)]==tail;end+=len(tail)
    return source[start:end]
source=SOURCE.read_text();fixture=(HERE/'fixture.cpp').read_text()
for marker in MARKERS:
    old=extract(fixture,marker)
    new=extract(source,marker).replace('storm_ship_laser::prepare(', 'storm_ship_laser::probePrepare(')
    assert fixture.count(old)==1
    fixture=fixture.replace(old,new)
(HERE/'queue_probe.cpp').write_text(fixture)
paths=[SOURCE,ROOT/'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.hpp',ROOT/'experiments/native-metal/engine/src/libs/ship/src/ship.h',ROOT/'experiments/native-metal/engine/src/libs/ship/src/ship.cpp',ROOT/'experiments/native-metal/ship_laser_geometry.hpp']
(HERE/'production-sha256.json').write_text(json.dumps({str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in paths},indent=2)+'\n')
scratch = Path(tempfile.mkdtemp(prefix='corsairs-queue-probe-build-'))
if True:
    binary=Path(scratch)/'queue_probe'
    command=['xcrun','clang++','-std=c++20','-O0','-pthread','-Wno-implicit-const-int-float-conversion','-I'+str(ROOT/'experiments/native-metal'),'-I'+str(ROOT/'experiments/native-metal/engine/src/libs/ship/src'),'-I'+str(ROOT/'experiments/native-metal/engine/src/libs/math/include'),str(HERE/'queue_probe.cpp'),'-o',str(binary)]
    compiled=subprocess.run(command,text=True,capture_output=True)
    log='COMMAND '+json.dumps(command)+'\nCOMPILE EXIT '+str(compiled.returncode)+'\n'+compiled.stdout+compiled.stderr
    if compiled.returncode==0:
        tested=subprocess.run([str(binary)],text=True,capture_output=True,timeout=10)
        log+='RUN EXIT '+str(tested.returncode)+'\n'+tested.stdout+tested.stderr
    (HERE/'probe.log').write_text(log)
    print(log)
    assert compiled.returncode==0 and tested.returncode==0

subprocess.run(['/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp',str(scratch)],check=True)
