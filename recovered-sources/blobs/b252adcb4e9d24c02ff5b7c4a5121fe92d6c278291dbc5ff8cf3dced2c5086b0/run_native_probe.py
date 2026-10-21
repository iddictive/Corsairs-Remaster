from pathlib import Path
import json, os, shutil, subprocess, sys, time

root=Path.cwd()
sys.path.insert(0,str(root/'tools'))
from metal_storage import diagnostic_run
from metal_graphics_settings import launch_environment
base=root/'.local-archives/metal-renderer-evidence/destruction-player-followup-20261018'
assert subprocess.run(['pgrep','-f',r'/(metal-engine|engine-1|sea-fleet-benchmark)$'],capture_output=True).returncode==1
run=base/(sys.argv[1] if len(sys.argv)>1 else 'baseline-native-run')
with diagnostic_run(run) as (temporary,budget):
    source=base/'baseline-native'
    for path in source.iterdir():
        if path.is_file() and not path.is_symlink():shutil.copy2(path,run/path.name)
    (run/'RESOURCE').symlink_to((source/'RESOURCE').resolve(),target_is_directory=True)
    env=launch_environment(Path('/REQUIRED_EXTERNAL_INPUT/Library/Application Support/Iddictive Corsairs'),dict(os.environ))
    env.update(TMPDIR=str(temporary)+os.sep,STORM_METAL_FRAME_TRACE=str(run/'renderer-frames.csv'),STORM_QUIET_CLIENT='1')
    if len(sys.argv)>2 and sys.argv[2]=='trace':env['STORM_METAL_DAMAGE_TRACE']='1'
    command=[str(base/'native_damage_probe'),str(run),'5000','1920','1080']
    (run/'manifest.json').write_text(json.dumps({'command':command,'graphics_environment':{k:v for k,v in env.items() if k.startswith('STORM_METAL_')},'hard_timeout_seconds':65,'soft_deadline_seconds':45},indent=2)+'\n')
    with (run/'run.log').open('w') as log:
        process=budget.start(command,env=env,stdout=log,stderr=subprocess.STDOUT)
        print('native diagnostic pid',process.pid,flush=True)
        code=budget.wait(process,65)
    print('exit',code,flush=True)
    print((run/'damage-latency.csv').read_text(),flush=True)
    if code:raise SystemExit(code)
