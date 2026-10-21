from pathlib import Path
import subprocess, shlex, os, json
root=Path(__file__).resolve().parents[4]
fixture=Path(__file__).resolve().parent
ninja=(root/'experiments/native-metal/.cache/build/build.ninja').read_text()
block=ninja.split('build native-engine/storm/libs/core/CMakeFiles/core.dir/src/attributes.cpp.o:')[1].split('\n\n')[0]
includes=next(l.split(' = ',1)[1] for l in block.splitlines() if l.startswith('  INCLUDES = '))
flags=['clang++','-std=c++20','-O1','-g','-fsanitize=address,undefined','-include','memory','-DFMT_HEADER_ONLY','-DFMT_CONSTEVAL=']+shlex.split(includes)+['-I'+str(root/'experiments/native-metal/engine/src/libs/core/src')]
commands=[]
for label,source in [('before',fixture/'before.cpp'),('after',fixture/'attributes-after.cpp')]:
    cmd=flags+[str(fixture/'probe.cpp'),str(source),'-o',str(fixture/label)]
    commands.append(cmd)
    built=subprocess.run(cmd,text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
    (fixture/(label+'-build.log')).write_text(built.stdout)
    if built.returncode: raise SystemExit(built.stdout)
    run=subprocess.run([str(fixture/label)],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,env={**os.environ,'ASAN_OPTIONS':'alloc_dealloc_mismatch=0'})
    (fixture/(label+'-run.log')).write_text(run.stdout)
    print(label,run.returncode,run.stdout)
    assert (run.returncode!=0) if label=='before' else (run.returncode==0)
cmd=['clang++','-std=c++20','-O2','-I'+str(root/'experiments/native-metal/engine/src/libs/util/include'),str(fixture/'rand-probe.cpp'),'-o',str(fixture/'rand-probe')]
commands.append(cmd); subprocess.run(cmd,check=True)
run=subprocess.run([str(fixture/'rand-probe')],text=True,stdout=subprocess.PIPE,stderr=subprocess.STDOUT)
(fixture/'rand-probe.log').write_text(run.stdout)
print('rand',run.returncode,run.stdout)
(fixture/'commands.json').write_text(json.dumps(commands,indent=2))
