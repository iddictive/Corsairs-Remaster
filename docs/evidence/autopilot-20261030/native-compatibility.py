"""Source-owned save/camera contract evidence; no engine or user-state execution."""
import hashlib
import json
from pathlib import Path
import subprocess

ROOT=Path(__file__).resolve().parents[3]
OUT=Path(__file__).resolve().parent
BASE='experiments/native-metal/engine/src/libs/'
BASE_COMMIT='60984eef27ca52717cdba9cac88a4ebd848d0b47'

def body(source,signature):
    start=source.index(signature);cursor=source.index('{',start)+1;depth=1
    while depth:
        depth+=(source[cursor]=='{')-(source[cursor]=='}');cursor+=1
    return source[start:cursor]

checks={}
for owner in ['sea_ai/src/ai_ship','sea_ai/src/ai_ship_move_controller','sea_ai/src/ai_ship_rotate_controller',
              'sea_ai/src/ai_ship_speed_controller','sea_ai/src/ai_ship_touch_controller','ship/src/ship']:
    path=BASE+owner+'.cpp';now=(ROOT/path).read_text()
    before=subprocess.check_output(['git','show',BASE_COMMIT+':'+path],cwd=ROOT,text=True)
    cls={'ai_ship':'AIShip','ai_ship_move_controller':'AIShipMoveController','ai_ship_rotate_controller':'AIShipRotateController',
         'ai_ship_speed_controller':'AIShipSpeedController','ai_ship_touch_controller':'AIShipTouchController','ship':'SHIP'}[Path(owner).name]
    for method in ['Save','Load']:
        signature=f'void {cls}::{method}(CSaveLoad *pSL)'
        a,b=body(before,signature),body(now,signature)
        checks[f'{cls}::{method}']={'unchanged':a==b,'sha256':hashlib.sha256(b.encode()).hexdigest()}
ship=(ROOT/(BASE+'sea_ai/src/ai_ship.cpp')).read_text()
before=subprocess.check_output(['git','show',BASE_COMMIT+':'+BASE+'sea_ai/src/ai_ship.cpp'],cwd=ROOT,text=True)
def camera(source):
    a=source.index('    if (isMainCharacter())',source.index('void AIShip::Execute('))
    b=source.index('    if (!isDead())',a)
    return source[a:b]
checks['AIShip camera ownership/execute branch']={'unchanged':camera(ship)==camera(before),'sha256':hashlib.sha256(camera(ship).encode()).hexdigest()}
result={'scope':'Exact save/load method bodies and camera-ownership/execute source branch compared with HEAD; proves native byte layout code unchanged, not loaded private SAVE/game replay.',
        'base_commit':BASE_COMMIT,
        'checks':checks,'all_unchanged':all(v['unchanged'] for v in checks.values())}
(OUT/'native-compatibility.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result));raise SystemExit(0 if result['all_unchanged'] else 1)
