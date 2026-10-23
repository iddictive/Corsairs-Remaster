# F2 section dispatch — 2026-10-23

The player reported Ship, Journal, Nations and Trade retaining the Character
screen. `CINODE::DoAction` sends `ievnt_command`; the F2 section scripts normalize
their icon/label node names, exit without releasing the interface picture, and
post `LaunchIAfterFrame`. The shared dispatcher rejects a node different from
`InterfaceStates.LaunchAfterFrame`.

`game_menu.c` sets that attribute to `I_DEBUGER` when opening the debugger.
Neither that transition nor any successful interface preparation consumed it.
The exact dispatcher in the existing CPU script VM launches Ship without the
attribute but rejects Ship after seeding `I_DEBUGER` (`Run=2`, zero script errors).
This proves the stale-state mechanism; the player's exact preceding actions
were not observed.

The repair consumes the deferred target after a successful `LoadSegment` in
`procInterfacePrepare`. Direct F2 entry also clears an already-stale value.
Denied preparation preserves the pending target; the dispatcher still rejects a
different target while the original request is pending. No layout, input mapping,
per-section script, engine, build, installed file or player state is changed here.

Baseline: `d47560909c544a41a7210330a951e299b04ca957`.
Before the edit, all six checked installed scripts matched the canonical
CP1251/CRLF projection: `interface.c`, `game_menu.c`, `character_all.c`,
`character.c`, `ship.c`, `tradebook.c`. Installed `interface.c` SHA-256 was
`eb47d81311607d35e3b5506995638a2f4c03ff1e11baccad80b11880c26e3c9a`.
Probe SHA-256: `b3e137d5c3408d8e7a5f0b822c93918d69e63bc8e13e1dbbfab6ddfe50b3528d`;
its tracked owner is `experiments/native-metal/script_vm_probe.cpp`. Existing
Release binary only; no shared build/cache writes. Host generator: Python 3.14.7;
cached compiler selection: Apple Clang 21, arm64, deployment target macOS 15.

## Reproduce the bounded CPU method check

From the repository root, set `TASK_SCRATCH_DIR` to an empty disposable
temporary directory, then run the following block with `python3`. Fixtures and
logs are written beneath that directory; remove it after inspecting the results.
The fixture uses the exact dispatcher, admission predicate and preparation
methods, real `LoadSegment`, attributes and script events. Renderer/gameplay
callbacks are stubs; it is not a rendered/native-game replay.

```python
import hashlib, os, pathlib, subprocess
root = pathlib.Path.cwd()
scratch = pathlib.Path(os.environ['TASK_SCRATCH_DIR'])
path = 'src/gameplay/PROGRAM/interface/interface.c'
before = subprocess.check_output(['git', 'show',
    'd47560909c544a41a7210330a951e299b04ca957:' + path], text=True)
after = (root / path).read_text()
prelude = '''#define INTERFACE_FORTCAPTURE 10
#define INTERFACE_RANSACK_MAIN 11
#define RC_INTERFACE_ERROR -1
#define INTERFACE_REALIZE 1
#define INTERFACE_EXECUTE 2
object InterfaceStates; object Interfaces[20]; object GameInterface;
object reload_fader; object pchar;
int DialogRun; bool g_ibVideoExecuting; int interfaceResultCommand;
string LastLaunch;
void Telescope_Off(){} void EngineLayersOffOn(bool on){}
void ISetSoundEvents(){} bool IsPerkIntoList(string name){return false;}
void Start_InterfaceDoExit(){}
void RecordLaunch(string name){if(procInterfacePrepare(0)) LastLaunch=name;}
void LaunchMainCharacter(){RecordLaunch("I_CHARACTER");}
void LaunchShipState(){RecordLaunch("I_SHIP");}
void LaunchQuestBook(){RecordLaunch("I_QUESTBOOK");}
void LaunchTradeBook(){RecordLaunch("I_TRADEBOOK");}
void LaunchNationRelation(){RecordLaunch("I_NATIONS");}
void LaunchItems(){RecordLaunch("I_ITEMS");}
void LaunchCabinChest(){RecordLaunch("I_CABIN_CHEST");}
void LaunchDebuderMenu(){RecordLaunch("I_DEBUGER");}
ref GetCharacter(int index){return &pchar;}
void LaunchTransferMain(ref a,ref b,string name){RecordLaunch("TransferMain");}
'''
cases = '''int Run(){
 SetEventHandler("LaunchIAfterFrame","ILaunchAfterFrame",0);
 Interfaces[0].SectionName="prepared.c";
 InterfaceStates.Launched=true;
 InterfaceStates.LaunchAfterFrame="I_DEBUGER";
 if(procInterfacePrepare(0)) return 1;
 if(InterfaceStates.LaunchAfterFrame!="I_DEBUGER") return 2;
 InterfaceStates.Launched=false;
 if(!procInterfacePrepare(0)) return 3;
 if(CheckAttribute(&InterfaceStates,"LaunchAfterFrame")) return 4;
 InterfaceStates.Launched=false;
 InterfaceStates.LaunchAfterFrame="I_DEBUGER";
 Event("LaunchIAfterFrame","sl","I_SHIP",0);
 if(LastLaunch!="") return 5;
 Event("LaunchIAfterFrame","sl","I_DEBUGER",0);
 if(LastLaunch!="I_DEBUGER") return 6;
 if(CheckAttribute(&InterfaceStates,"LaunchAfterFrame")) return 7;
 string routes[6];
 routes[0]="I_CHARACTER"; routes[1]="I_SHIP"; routes[2]="I_QUESTBOOK";
 routes[3]="I_TRADEBOOK"; routes[4]="I_NATIONS"; routes[5]="I_ITEMS";
 for(int i=0;i<6;i++){
   InterfaceStates.Launched=false; LastLaunch="";
   Event("LaunchIAfterFrame","sl",routes[i],0);
   if(LastLaunch!=routes[i]) return 10+i;
 }
 InterfaceStates.Launched=false; LastLaunch="";
 Event("LaunchIAfterFrame","sl","unknown",0);
 if(LastLaunch!="") return 20;
 return 0;
}
'''
probe = root / 'experiments/native-metal/.cache/build/bin/script-vm-probe'
print('probe_sha256=' + hashlib.sha256(probe.read_bytes()).hexdigest())
for name, source in [('baseline', before), ('current', after)]:
    def method(start, end):
        first = source.index(start)
        return source[first:source.index(end, first)]
    selected = method('bool procEnableInterfaceLaunch(', '\nvoid ReturnToMainMenu()')
    selected += method('bool procInterfacePrepare(', '\nvoid Start_InterfaceDoExit()')
    selected += method('void ILaunchAfterFrame()', '\nbool procInterfacePrepare(')
    directory = scratch / name
    directory.mkdir()
    (directory / 'prepared.c').write_text('void PreparedSegmentMarker(){}\n')
    (directory / 'cases.c').write_text(prelude + selected + cases)
    result = subprocess.run([str(probe), str(directory), 'Run', '0'],
        cwd=directory, capture_output=True, text=True)
    print(name + ': ' + result.stdout.replace('\n', '; ') +
          'exit=' + str(result.returncode))
    print(result.stderr, end='')
    errors = directory / '.vm-userdata/Logs/error.log'
    if errors.exists() and errors.stat().st_size:
        print(errors.read_text(errors='replace'))
```

Recorded result after the canonical edit:

```text
baseline: Run=4; script_errors=0; exit=1
current: Run=0; script_errors=0; exit=0
```

The baseline fails because successful preparation retains the old target.
The current method clears stale state, preserves denied preparation and the
in-flight mismatch guard, opens the debugger, routes each of the six F2 sections
after it, and rejects an unknown route. `git diff --check` passes.

## Delivered script

After the player closed the game, the canonical targeted command
`python3 tools/sync_metal_gameplay.py apply --path PROGRAM/interface/interface.c`
delivered the file. Both runtime and installed bytes equal the canonical
CP1251/CRLF projection, SHA-256
`03ac11e1a2cf3e87ce4d4a511d22b51efff5da8d53943ea3e64c1c4dcb73d4e7`.
The follow-up check reports zero pending files. The native engine signature
passes; [joined build identity](../spatial-audio-20261023/build.json) retains the
engine and source hashes.

The sea-only installer did not include this interface file. Targeted delivery
initially rejected the stale cached staged engine despite a completed build.
`build.sh` now stages its verified unsigned output; `run.sh` no longer duplicates
that copy. A cached native build retained the exact same engine hash. Current
built/staged identity passes, missing/incompatible engines remain rejected.
This permits the existing targeted command without invoking gameplay probes.

The actual pointer/keyboard menu replay remains with the player. The game was
not launched or controlled.
