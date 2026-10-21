from pathlib import Path
import hashlib,json,resource,shlex,subprocess,time

HERE=Path(__file__).resolve().parent
CLOSURE=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/phonk-current-20261016/integrated-sea-laser/coherent-f8c45b2')
PRODUCER=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
NATIVE=PRODUCER/'experiments/native-metal'
manifest=json.loads((CLOSURE/'manifest.json').read_text())
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert manifest['source_commit']=='f8c45b299eec4b1fa1ab76f89a00aa7f33e5b8f0'
assert all(sha(HERE/'headers'/p)==v for p,v in manifest['header_sources'].items())
assert all(sha(HERE/'headers'/p)==v for p,v in manifest['vendor_sources'].items())
archives={Path(p).name:CLOSURE/p for p in manifest['retained'] if p.endswith('.a')}
assert len(archives)==49
archive_hashes={str(p):sha(p) for p in archives.values()}
assert all(value==manifest['retained'][str(Path(p).relative_to(CLOSURE))]['sha256'] for p,value in archive_hashes.items())
lines=(CLOSURE/'producer-commands.txt').read_text().splitlines()
original_compile=shlex.split(next(x for x in lines if 'sea_fleet_benchmark.cpp.o' in x and ' -c ' in x))
original_link=shlex.split(next(x for x in lines if 'sea_fleet_benchmark.cpp.o' in x and ' -c ' not in x))
assert original_link[:2]==[':','&&'] and original_link[-2:]==['&&',':']
def project_path(value):
    if value.startswith(str(NATIVE/'inputs')+'/'):return value
    if value.startswith(str(NATIVE)+'/') or value==str(NATIVE):return str(HERE/'headers'/Path(value).relative_to(PRODUCER))
    return value
compile_command=[]
n=0
while n<len(original_compile):
    value=original_compile[n]
    if value in ['-MT','-MF','-o','-c']:n+=2;continue
    if value in ['-MD','-v']:n+=1;continue
    if value.startswith('-I') and len(value)>2:value='-I'+project_path(value[2:])
    elif value.startswith('-isystem') and len(value)>8:value='-isystem'+project_path(value[8:])
    elif value.startswith(str(NATIVE)):value=project_path(value)
    compile_command.append(value);n+=1
for directory in ['ship/src','sea_ai/src','sea/src','rigging/src']:
    include=HERE/'headers/experiments/native-metal/engine/src/libs'/directory
    assert include.is_dir()
    option='-I'+str(include)
    if option not in compile_command:compile_command.append(option)
compile_command+=['-o',str(HERE/'probe.o'),'-c',str(HERE/'source/probe.cpp')]
link_command=[]
used=set()
for value in original_link[2:-2]:
    if value.endswith('sea_fleet_benchmark.cpp.o'):value=str(HERE/'probe.o')
    elif value=='sea-fleet-benchmark':value=str(HERE/'replay')
    elif value.endswith('.a'):
        value=str(archives[Path(value).name]);used.add(value)
    elif value.startswith('-Wl,') and '.a' in value:raise AssertionError(('unhandled archive syntax',value))
    link_command.append(value)
assert used==set(archive_hashes)
source_hashes={p.name:sha(p) for p in (HERE/'source').iterdir() if p.is_file() and p.name!='source-sha256.json'}
(HERE/'source/source-sha256.json').write_text(json.dumps(source_hashes,indent=2))
(HERE/'compile-command.json').write_text(json.dumps(compile_command,indent=2))
(HERE/'link-command.json').write_text(json.dumps(link_command,indent=2))
(HERE/'library-sha256.json').write_text(json.dumps(archive_hashes,indent=2))
results=[]
before_cpu=resource.getrusage(resource.RUSAGE_CHILDREN)
def cpu_limit():resource.setrlimit(resource.RLIMIT_CPU,(60,60))
for name,command in [('compile',compile_command),('link',link_command)]:
    started=time.monotonic()
    with (HERE/(name+'.log')).open('w') as output:
        result=subprocess.run(command,cwd=HERE,stdout=output,stderr=subprocess.STDOUT,preexec_fn=cpu_limit,timeout=70)
    current=resource.getrusage(resource.RUSAGE_CHILDREN)
    cpu=current.ru_utime+current.ru_stime-before_cpu.ru_utime-before_cpu.ru_stime
    results.append({'phase':name,'exit':result.returncode,'wall_seconds':time.monotonic()-started,'aggregate_child_cpu_seconds':cpu})
    (HERE/'build-results.json').write_text(json.dumps(results,indent=2))
    print(name,result.returncode,'aggregate_cpu',round(cpu,3),flush=True)
    if result.returncode:
        print((HERE/(name+'.log')).read_text()[-4500:]);raise SystemExit(result.returncode)
    assert cpu<=120
assert source_hashes=={p.name:sha(p) for p in (HERE/'source').iterdir() if p.is_file() and p.name!='source-sha256.json'}
(HERE/'binary-sha256.json').write_text(json.dumps({'executable':sha(HERE/'replay'),'object':sha(HERE/'probe.o'),'source':source_hashes},indent=2))
print('BINARY_READY',sha(HERE/'replay'),flush=True)
