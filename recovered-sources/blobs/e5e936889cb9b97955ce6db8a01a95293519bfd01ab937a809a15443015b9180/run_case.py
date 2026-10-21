"""Finite retained-fixture replay. Reads one coherent closure; owns scratch only."""
from pathlib import Path
import hashlib,json,os,resource,shlex,shutil,subprocess,tarfile,time,traceback

HERE=Path(__file__).resolve().parent
REPO=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs');NATIVE=REPO/'experiments/native-metal'
CLOSURE=HERE.parent/'coherent-23ac5eb-production-abi'
SCRATCH=Path(os.environ['CODEX_SCRATCH_DIR']).resolve();CASE=SCRATCH/'case';HEADERS=SCRATCH/'headers'
CASE.mkdir();HEADERS.mkdir()
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()
receipt={'source_commit':'23ac5eba107a3f5476f188d3e96c0a5b7dba7801','player_control':False,'player_save_loaded':False,'phases':[],'native_started':False,'output_owner':str(HERE),'scratch':str(SCRATCH),'admission':{'cpu_subprocess_seconds':60,'compile_link_combined_cpu_seconds':120,'native_wall_seconds':15,'scratch_bytes':1073741824,'reserve_bytes':8589934592,'native_frames':401,'viewport':[960,600],'concurrency':'one compile then link then hidden native; root released build/stage','host_ram_bytes':51539607552,'host_free_percent':52,'estimated_extra_peak_bytes':2147483648,'host_reserve_bytes':8589934592}}
def save(): (HERE/'receipt.json').write_text(json.dumps(receipt,indent=2)+'\n')
def players():
 return [r for r in subprocess.check_output(['ps','-axo','pid=,comm='],text=True).splitlines() if '/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/' in r or r.strip().endswith('/metal-engine')]
def limit():resource.setrlimit(resource.RLIMIT_CPU,(60,60))
def phase(name,command,cwd,env=None,native=False):
 (HERE/(name+'-command.json')).write_text(json.dumps(command,indent=2)+'\n')
 if native:assert not players(),'player active; fixture not started'
 start=time.monotonic();before=resource.getrusage(resource.RUSAGE_CHILDREN);reason=None
 log=HERE/(name+'.log');proc=None
 with log.open('w') as out:
  proc=subprocess.Popen(command,cwd=cwd,env=env,stdout=out,stderr=subprocess.STDOUT,preexec_fn=limit)
  receipt['owned_pid']=proc.pid;receipt['native_started']|=native;save()
  while proc.poll() is None:
   if native and players():reason='player started'
   if time.monotonic()-start>(15 if native else 70):reason='native15s cap' if native else 'build70s wall cap'
   if log.stat().st_size>(4<<20):reason='4MiB log cap'
   if sum(p.stat().st_size for p in CASE.rglob('*') if p.is_file() and not p.is_symlink())>(256<<20):reason='256MiB case output cap'
   if reason:
    proc.terminate()
    try:proc.wait(timeout=2)
    except subprocess.TimeoutExpired:proc.kill();proc.wait(timeout=2)
    break
   time.sleep(.1)
 after=resource.getrusage(resource.RUSAGE_CHILDREN)
 row={'name':name,'pid':proc.pid,'exit':proc.returncode,'wall_seconds':time.monotonic()-start,'cpu_seconds':after.ru_utime+after.ru_stime-before.ru_utime-before.ru_stime,'stopped':reason,'terminal':proc.poll() is not None}
 receipt['phases'].append(row);save();print(json.dumps(row),flush=True)
 if proc.returncode:raise RuntimeError(name+' failed; exact log retained')
 if not native:assert sum(r['cpu_seconds'] for r in receipt['phases'] if r['name']!='native')<=101.522862
try:
 assert not (HERE/'compile-command.json').exists(),'one compile attempt only'
 manifest=json.loads((CLOSURE/'manifest.json').read_text());assert manifest['source_commit']==receipt['source_commit']
 for name,info in manifest['retained'].items():assert sha(CLOSURE/name)==info['sha256'],name
 with tarfile.open(CLOSURE/'headers.tar.gz','r:*') as archive:
  members=archive.getmembers();assert all(not Path(m.name).is_absolute() and '..' not in Path(m.name).parts and not m.issym() and not m.islnk() for m in members)
  archive.extractall(HEADERS)
 for name,value in {**manifest['header_sources'],**manifest['vendor_sources']}.items():assert sha(HEADERS/name)==value,name
 archives={Path(p).name:CLOSURE/p for p in manifest['retained'] if p.endswith('.a')};assert len(archives)==49
 receipt['closure_manifest_sha256']=sha(CLOSURE/'manifest.json');receipt['headers_archive_sha256']=sha(CLOSURE/'headers.tar.gz');receipt['archive_hashes']={p:sha(v) for p,v in archives.items()};receipt['header_count']=len(manifest['header_sources'])
 for src in (HERE/'source').iterdir():
  if src.suffix in {'.cpp','.hpp','.c','.ini'}:shutil.copy2(src,CASE/src.name)
 (CASE/'RESOURCE').symlink_to('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE',target_is_directory=True)
 source_hashes={p.name:sha(p) for p in (HERE/'source').iterdir() if p.is_file()};receipt['fixture_source_sha256']=source_hashes
 lines=(CLOSURE/'producer-commands.txt').read_text().splitlines();benchmark_compile=shlex.split(next(x for x in lines if 'sea_fleet_benchmark.cpp.o' in x and ' -c ' in x));original_link=shlex.split(next(x for x in lines if 'sea_fleet_benchmark.cpp.o' in x and ' -c ' not in x))
 commands=json.loads((CLOSURE/'compile-commands.json').read_text());owners=[c for c in commands if c['file'].endswith('/ship_phonk_laser.cpp')];assert len(owners)==1;owner=owners[0];original_compile=shlex.split(owner['command']);receipt['production_compile_entry']=owner;receipt['production_compdb_sha256']=sha(CLOSURE/'compile-commands.json')
 def map_path(p):
  if p.startswith(str(NATIVE/'inputs')+'/'):return p
  if p.startswith(str(NATIVE)+'/') or p==str(NATIVE):return str(HEADERS/Path(p).relative_to(REPO))
  return p
 owner_command=[];n=0
 while n<len(original_compile):
  v=original_compile[n]
  if v in ['-MT','-MF','-o','-c']:n+=2;continue
  if v in ['-MD','-v']:n+=1;continue
  if v.startswith('-I') and len(v)>2:v='-I'+map_path(v[2:])
  elif v.startswith(str(NATIVE)):v=map_path(v)
  owner_command.append(original_compile[n]);n+=1
 compile_command=[]
 for v in owner_command:
  if v.startswith('-I') and len(v)>2:v='-I'+map_path(v[2:])
  elif v.startswith(str(NATIVE)):v=map_path(v)
  compile_command.append(v)
 # Fixture-only include closure comes from the benchmark; definitions belong
 # exclusively to the actual production SHIP translation unit above.
 n=0
 while n<len(benchmark_compile):
  v=benchmark_compile[n]
  if v.startswith('-I') and len(v)>2:
   option='-I'+map_path(v[2:])
   if option not in compile_command:compile_command.append(option)
  elif v=='-isystem':
   pair=['-isystem',map_path(benchmark_compile[n+1])]
   if pair[1] not in compile_command:compile_command.extend(pair)
   n+=1
  n+=1
 for directory in ['ship/src','sea_ai/src','sea/src','rigging/src','sea_cameras/src']:
  d=HEADERS/'experiments/native-metal/engine/src/libs'/directory;assert d.is_dir();compile_command.append('-I'+str(d))
 # Reuse the completed independent preflight: same source hashes, structured
 # production command and fixture flags, with only scratch path relocation.
 preflight=json.loads((HERE/'abi-preflight.json').read_text())
 definitions=lambda a:sorted(v for v in a if v.startswith(('-D','-U')))
 assert preflight['equal'] and definitions(owner_command)==preflight['production_definitions'] and definitions(compile_command)==preflight['fixture_definitions']
 assert sha(HERE/'production-SHIP-layout-clean.txt')==preflight['clean_layout_sha256'] and sha(HERE/'fixture-SHIP-layout-clean.txt')==preflight['clean_layout_sha256']
 for name,digest in manifest['header_sources'].items():assert sha(REPO/name)==digest,name
 receipt['abi_preflight']='reused identical production/fixture full SHIP record layout and definitions';receipt['abi_preflight_receipt']='abi-preflight-receipt.json';save()
 compile_command+=['-o',str(CASE/'probe.o'),'-c',str(CASE/'probe.cpp')]
 assert original_link[:2]==[':','&&'] and original_link[-2:]==['&&',':']
 link_command=[];used=set()
 for v in original_link[2:-2]:
  if v.endswith('sea_fleet_benchmark.cpp.o'):v=str(CASE/'probe.o')
  elif v=='bin/sea-fleet-benchmark':v=str(CASE/'replay')
  elif v.endswith('.a'):v=str(archives[Path(v).name]);used.add(v)
  link_command.append(v)
 assert used==set(map(str,archives.values()))
 save();phase('compile',compile_command,CASE);phase('link',link_command,CASE)
 receipt['binary_sha256']=sha(CASE/'replay')
 protected=[Path('/Applications/Corsairs Iddictive Remaster.app/Contents/MacOS/metal-engine'),Path('/REQUIRED_EXTERNAL_INPUT/Library/Application Support/Iddictive Corsairs/engine.ini'),Path('/REQUIRED_EXTERNAL_INPUT/Library/Application Support/Iddictive Corsairs/options')]
 receipt['protected_before']={str(p):sha(p) for p in protected};assert next(iter(receipt['protected_before'].values()))=='147e4d1f2302e249440e5c00a969132566912eb6748c1f6ef89072f66eabbf81'
 env={k:v for k,v in os.environ.items() if not k.startswith(('DESTRUCTION_','HELD_','POSTFIX_'))};env.update(DESTRUCTION_SCENE_MODE='laser',HELD_CASE='ship',STORM_METAL_FRAME_TRACE=str(CASE/'frame-trace.csv'))
 receipt['native_environment']={k:v for k,v in env.items() if k.startswith(('DESTRUCTION_','HELD_','STORM_METAL_FRAME_TRACE'))};save()
 phase('native',[str(CASE/'replay'),str(CASE),'401','960','600'],CASE,env,True)
 receipt['protected_after']={str(p):sha(p) for p in protected};receipt['protected_unchanged']=receipt['protected_before']==receipt['protected_after'];assert receipt['protected_unchanged']
 receipt['status']='native complete; parent rendered observation pending; player acceptance open'
except BaseException as exc:
 receipt['status']='failed';receipt['error']=str(exc);(HERE/'failure.txt').write_text(traceback.format_exc());print(traceback.format_exc(),flush=True)
finally:
 output=HERE/'outputs';output.mkdir(exist_ok=True)
 for p in CASE.iterdir():
  if p.is_file() and not p.is_symlink() and p.suffix in {'.csv','.ppm','.bin','.tsv'}:shutil.copy2(p,output/p.name)
 logs=CASE/'userdata/Logs'
 if logs.exists():shutil.copytree(logs,output/'Logs',dirs_exist_ok=True)
 receipt['outputs']={str(p.relative_to(HERE)):{'sha256':sha(p),'bytes':p.stat().st_size} for p in output.rglob('*') if p.is_file()}
 receipt['scratch_bytes_at_terminal']=sum(p.stat().st_size for p in SCRATCH.rglob('*') if p.is_file() and not p.is_symlink());receipt['owned_processes_terminal']=all(r['terminal'] for r in receipt['phases']);save()
 if receipt['status']=='failed':raise SystemExit(1)
