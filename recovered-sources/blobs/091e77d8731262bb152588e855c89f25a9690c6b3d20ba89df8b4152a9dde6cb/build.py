import pathlib, shlex, subprocess, sys
root=pathlib.Path(__file__).resolve().parents[3]
owner=pathlib.Path(__file__).resolve().parent
build=root/'experiments/native-metal/.cache/build'
commands=subprocess.check_output(['ninja','-C',str(build),'-t','commands','dynamic_lighting-probe'],text=True).splitlines()
compile=shlex.split(next(s for s in commands if ' -c ' in s and s.endswith('/backend.mm')))
compile[compile.index('-c')+1]=str(owner/'probe.mm')
before=len(sys.argv)>1 and sys.argv[1]=='before'
if before: compile.insert(1,'-DBEFORE=1')
compile[compile.index('-o')+1]=str(owner/'probe.o')
for flag in ['-MD','-MT','-MF']:
    if flag in compile:
        i=compile.index(flag);del compile[i:i+(1 if flag=='-MD' else 2)]
subprocess.run(compile,cwd=build,check=True,timeout=240)
link=shlex.split(commands[-1].removeprefix(': && ').removesuffix(' && :'))
link[link.index('CMakeFiles/dynamic_lighting-probe.dir/dynamic_lighting_probe.cpp.o')]=str(owner/'probe.o')
link[link.index('-o')+1]=str(owner/('probe-before' if before else 'probe'))
subprocess.run(link,cwd=build,check=True,timeout=60)
