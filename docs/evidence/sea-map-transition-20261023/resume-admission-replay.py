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
# Historical guard replay: retained authored admission supersedes these guards.
current = subprocess.check_output(
    ['git', 'show', 'a9deaca2db69141177c22e2f88c099effd595aa6:' + relative], text=True)
prelude = '''object worldMap; object wdmLoginToSea;
string LastTrace;
void Sea_TransitionTrace(string step){LastTrace=step;}
bool QuestSelected=false;
int wdmGetNumberShipEncounters(){return 1;}
bool wdmSetCurrentShipData(int i){worldMap.encounter.select=QuestSelected;return true;}
bool WdmEncounterUsesAuthoredAdmission(){return QuestSelected;}
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
    helper_start = current.index('bool WdmRejectResidentSea(')
    helper_end = current.index('bool WdmTryResidentSea()', helper_start)
    helper = current[helper_start:helper_end]
    fixture = prelude + helper + 'bool Evaluate(){\n' + source[start:end] + driver
    if label == 'current':
        capture_start = current.index('    string residentRejection =')
        capture_end = current.index('    if (Sea_ResidentSuspended())', capture_start)
        capture = current[capture_start:capture_end]
        emit = next(row.strip() for row in current.splitlines()
                    if 'Sea_TransitionTrace("cold-admission:"' in row)
        fixture += 'string CaptureReason(){\n' + capture + 'return residentRejection;}\n'
        fixture += '''int Diagnostic(){
 DeleteAttribute(&worldMap, ""); QuestSelected=false;
 worldMap.playerInStorm=0; worldMap.GreatRecipeZoneAttack=1;
 if(Evaluate()) return 1;
 string residentRejection=CaptureReason();
 DeleteAttribute(&wdmLoginToSea, "");
 EMIT
 if(LastTrace!="cold-admission:authored-offshore") return 2;
 DeleteAttribute(&worldMap,"GreatRecipeZoneAttack");
 worldMap.stormId="contact"; worldMap.playerInStorm=1;
 if(Evaluate() || wdmLoginToSea.residentRejection!="authored-storm") return 3;
 worldMap.playerInStorm=0; QuestSelected=true;
 if(Evaluate() || wdmLoginToSea.residentRejection!="authored-encounter") return 4;
 QuestSelected=false;
 if(!Evaluate()) return 5;
 DeleteAttribute(&wdmLoginToSea, "");
 if(CheckAttribute(&wdmLoginToSea,"residentRejection")) return 6;
 residentRejection=CaptureReason();
 EMIT
 if(LastTrace!="cold-admission:unclassified") return 7;
 return 0;
}
'''.replace('EMIT', emit)
    (case / 'cases.c').write_text(fixture)
    result = subprocess.run([str(probe), str(case), 'Run', '0'], cwd=case,
                            text=True, capture_output=True)
    results[label] = {'exit': result.returncode, 'stdout': result.stdout, 'stderr': result.stderr}
    if label == 'current':
        diagnostic = subprocess.run([str(probe), str(case), 'Diagnostic', '0'], cwd=case,
                                    text=True, capture_output=True)
        results['diagnostic'] = {'exit': diagnostic.returncode, 'stdout': diagnostic.stdout,
                                 'stderr': diagnostic.stderr}
print(json.dumps(results, indent=2))
