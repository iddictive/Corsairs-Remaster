"""Retain the exact private compiler inputs, including its non-system closure."""
from pathlib import Path
import hashlib,json,shlex,subprocess

root=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
here=Path(__file__).resolve().parent

def retain(name,fixture,command,expected_headers,evidence_sources=()):
    destination=here/'input-sets'/name
    assert not destination.exists(),destination
    includes=[v for v in command if v.startswith('-I')]
    sources=[Path(v) for v in command if v.endswith('.cpp')]
    assert Path(fixture) in sources
    dependencies=set(sources)
    for source in sources:
        result=subprocess.run(['clang++','-std=c++20','-DSTORM_METAL_SHIP_HULL_DAMAGE',*includes,'-MM','-MT','fixture',str(source)],text=True,capture_output=True,check=True,timeout=20)
        dependency_text=result.stdout.replace('\\\n',' ')
        for line in dependency_text.splitlines():
            dependencies.update(Path(v) for v in shlex.split(line.split(':',1)[1]))
    dependencies.update(root/p for p in expected_headers)
    dependencies.update(evidence_sources)
    current={p:hashlib.sha256((root/p).read_bytes()).hexdigest() for p in expected_headers}
    assert current==expected_headers,[(p,h,current[p]) for p,h in expected_headers.items() if current[p]!=h]
    assert sum(p.stat().st_size for p in dependencies)<30_000_000
    baseline=subprocess.check_output(['git','rev-parse','HEAD'],cwd=root,text=True).strip()
    records=[];external=[]
    for path in sorted(dependencies):
        data=path.read_bytes()
        try:relative=path.relative_to(root)
        except ValueError:
            external.append({'path':str(path),'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),'owner':'installed compiler/SDK input'})
            continue
        saved=destination/'repo'/relative
        saved.parent.mkdir(parents=True,exist_ok=True);saved.write_bytes(data)
        baseline_result=subprocess.run(['git','show',baseline+':'+str(relative)],cwd=root,capture_output=True)
        sha=hashlib.sha256(data).hexdigest()
        records.append({'path':str(relative),'sha256':sha,'bytes':len(data),'saved':str(saved.relative_to(here)),'matches_baseline':baseline_result.returncode==0 and hashlib.sha256(baseline_result.stdout).hexdigest()==sha})
    replay=[]
    for token in command:
        if token.startswith('-I'):
            path=Path(token[2:])
            try:relative=path.relative_to(root)
            except ValueError:replay.append(token)
            else:replay.extend(['-I'+str(destination/'repo'/relative),token])
        elif token.endswith('.cpp'):
            replay.append(str(destination/'repo'/Path(token).relative_to(root)))
        else:replay.append(token)
    manifest={'baseline_git_ref':baseline,'actual_fixture':str(Path(fixture).relative_to(root)),'command':command,'replay_command_with_retained_inputs':replay,'entries':records,'external_conditions':external,'compiler':subprocess.check_output(['clang++','--version'],text=True),'claim_ceiling':'Exact extracted scene methods and value kernel/contact/native math; mocked entity/model/GEOS transaction, no native render or installed scene.'}
    (destination/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
    return destination/'manifest.json',replay

if __name__=='__main__':
    import sys
    receipt_path=here/sys.argv[1];receipt=json.loads(receipt_path.read_text());name=sys.argv[2];fixture=here/sys.argv[3]
    command=receipt['command'];command=[str(fixture) if x==str(here/'probe.cpp') else x for x in command]
    manifest,_=retain(name,fixture,command,receipt['headers']);print(manifest)
