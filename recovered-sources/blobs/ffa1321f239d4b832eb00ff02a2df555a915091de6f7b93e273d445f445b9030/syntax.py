from pathlib import Path
import re, shlex, subprocess
repo=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
build=repo/'experiments/native-metal/.cache/build'
text=(build/'build.ninja').read_text()
for library,name in [('ship','ship'),('ship','ship_hull_contact'),('touch','touch')]:
    block=re.search(r'^build [^\n]*'+re.escape('/'+name+'.cpp.o:')+r'[^\n]*\n(?:  [^\n]*\n)+',text,re.M).group()
    values=dict(line.strip().split(' = ',1) for line in block.splitlines()[1:] if ' = ' in line)
    flags=shlex.split(values['DEFINES'])+shlex.split(values['FLAGS'])+shlex.split(values['INCLUDES'])
    flags=[x for x in flags if x!='-v']
    source=repo/f'experiments/native-metal/engine/src/libs/{library}/src/{name}.cpp'
    result=subprocess.run(['clang++',*flags,'-w','-fsyntax-only',str(source)],cwd=build,capture_output=True,text=True)
    Path(__file__).with_name(name+'-syntax.log').write_text(result.stdout+result.stderr)
    if result.returncode:print(result.stdout+result.stderr);raise SystemExit(result.returncode)
    print(name,'native compiler flags: PASS',flush=True)
