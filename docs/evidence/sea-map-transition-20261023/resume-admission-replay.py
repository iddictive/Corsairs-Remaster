"""CPU replay of the actual resident-return admission guards, without a game.

Run from the repository root with TASK_SCRATCH_DIR naming an empty directory.
The surrounding live scene/entity preconditions and later scene creation are
outside this bounded oracle. Encounter enumeration is a fixture callback.
"""
import json
import os
from pathlib import Path
import subprocess

root = Path.cwd()
work = Path(os.environ['TASK_SCRATCH_DIR'])
relative = 'src/gameplay/PROGRAM/worldmap/worldmap_reload.c'
baseline = subprocess.check_output(
    ['git', 'show', '3e7d19c1140979cc3e4744dbc8a798f2b27fdc6d:' + relative], text=True)
current = (root / relative).read_text()
prelude = '''object worldMap;
bool QuestSelected=false;
int wdmGetNumberShipEncounters(){return 1;}
bool wdmSetCurrentShipData(int i){worldMap.encounter.select=QuestSelected;return true;}
bool WdmEncounterUsesAuthoredAdmission(){return QuestSelected;}
bool Evaluate(){
'''
driver = '''return true;}
int Run(){
 worldMap.playerInStorm=0;
 if(!Evaluate()) return 1;
 // Native TestInStorm leaves stormId behind after contact ends.
 worldMap.stormId="previous-storm";
 if(!Evaluate()) return 2;
 worldMap.playerInStorm=1;
 if(Evaluate()) return 3;
 DeleteAttribute(&worldMap,"playerInStorm"); if(Evaluate()) return 6;
 // Regional native weather has no authored encounter to reload.
 worldMap.playerInStorm=1; worldMap.stormId=""; if(!Evaluate()) return 7;
 worldMap.playerInStorm=0; DeleteAttribute(&worldMap,"stormId");
 QuestSelected=true; if(Evaluate()) return 4;
 QuestSelected=false; worldMap.GreatRecipeZoneAttack=1; if(Evaluate()) return 5;
 return 0;
}
'''
probe = root / 'experiments/native-metal/.cache/build/bin/script-vm-probe'
results = {}
for label, source in [('baseline', baseline), ('current', current)]:
    start = source.index('    // Explicit authored encounters and scripted offshore scenes')
    end = source.index('    DeleteAttribute(&wdmLoginToSea', start)
    case = work / label
    case.mkdir()
    (case / 'cases.c').write_text(prelude + source[start:end] + driver)
    result = subprocess.run([str(probe), str(case), 'Run', '0'], cwd=case,
                            text=True, capture_output=True)
    results[label] = {'exit': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr}
print(json.dumps(results, indent=2))
