from pathlib import Path
import json,hashlib,os,subprocess,time

HERE=Path(__file__).resolve().parent
request=json.loads((HERE/'corrected-invocation.json').read_text())
identity=json.loads((HERE/'ready.json').read_text())
case=Path(request['command'][1]);sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
assert sha(Path(request['command'][0]))==identity['binary_sha256']
assert all(sha(case/name)==value for name,value in identity['fixture_source_sha256'].items())
assert not (case/'run.log').exists()
env=dict(os.environ)
for key in list(env):
    if key.startswith('DESTRUCTION_'):env.pop(key)
env.update(request['environment'])
assert env['DESTRUCTION_SCENE_MODE']=='laser' and env['HELD_CASE']=='ship'
assert Path(env['STORM_METAL_FRAME_TRACE']).is_absolute() and not Path(env['STORM_METAL_FRAME_TRACE']).exists()
assert not any('DESTRUCTION_'+key in env for key in ['AUTO','NUCLEAR','NO_CUT','AIM_HEIGHT','DISTANCE','NEUTRAL'])
selected={key:env[key] for key in sorted(env) if key.startswith('DESTRUCTION_') or key in ['HELD_CASE','STORM_METAL_FRAME_TRACE']}
protected=[Path('/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/metal-engine'),Path('/REQUIRED_EXTERNAL_INPUT/Library/Application Support/Iddictive Corsairs/engine.ini'),Path('/REQUIRED_EXTERNAL_INPUT/Library/Application Support/Iddictive Corsairs/options'),Path('/REQUIRED_EXTERNAL_INPUT/Library/Application Support/Iddictive Corsairs/SAVE/Кампот/Кампот AutoSave 4')]
def players():
    found=[]
    for row in subprocess.check_output(['ps','-axo','pid=,comm='],text=True).splitlines():
        bits=row.strip().split(None,1)
        if len(bits)==2 and (Path(bits[1]).name in ['metal-engine','Corsairs Iddictive Remaster'] or bits[1].startswith('/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/')):found.append(int(bits[0]))
    return found
assert not players(),'player active; no fixture launch'
receipt={'command':request['command'],'actual_selected_environment':selected,'binary_sha256':sha(Path(request['command'][0])),'protected_before':{str(p):sha(p) for p in protected},'source_commit':identity['source_commit'],'native_fingerprint':identity['native_fingerprint'],'cap_seconds':15,'player_control':False,'player_save_loaded':False,'preflight_before_Popen':True,'gpu_release':False}
(HERE/'corrected-preflight.json').write_text(json.dumps(receipt,indent=2)+'\n')
start=time.monotonic();reason=None
with (case/'run.log').open('w') as log:
    proc=subprocess.Popen(request['command'],cwd=case,env=env,stdout=log,stderr=subprocess.STDOUT)
    while proc.poll() is None:
        if players():reason='player started'
        if time.monotonic()-start>15:reason='15s execution cap'
        size=sum(p.stat().st_size for p in case.glob('first-*') if p.is_file())
        if (case/'laser-verdicts.tsv').exists():size+=(case/'laser-verdicts.tsv').stat().st_size
        if size>(964<<20):reason='diagnostic output cap'
        if reason:
            proc.terminate()
            try:proc.wait(timeout=2)
            except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=2)
            break
        time.sleep(.15)
receipt.update({'exit':proc.returncode,'wall_seconds':time.monotonic()-start,'stopped':reason,'protected_after':{str(p):sha(p) for p in protected},'gpu_release':True})
receipt['protected_unchanged']=receipt['protected_before']==receipt['protected_after']
(HERE/'corrected-run-receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({k:receipt[k] for k in ['exit','wall_seconds','stopped','protected_unchanged','gpu_release']}),flush=True)
