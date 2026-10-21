from pathlib import Path
import re,subprocess,os,time,json
r=Path(__file__).parent
base=Path('/REQUIRED_EXTERNAL_INPUT/scratch').read_text()
old=(r/'old.c').read_text().strip();assert base.count(old)==1
base=base.replace(old,(r/'candidate.c').read_text().replace('IsEntity(worldMap)','probeMapActive')).replace('int serviceChecks;','int serviceChecks;bool probeMapActive=true;')
start=base.index(' for(int tick=');base=base[:start]
timing={}
for name,loop,assertion in [('seed','', ''),('admit','WdmTrafficCancelRefresh();WdmTrafficBeginRefresh();',''),('frame1','wdmTrafficRefreshCursor=0;wdmTrafficRefreshPending=true;Event("WorldMap_TrafficFrame","f",1.0/60.0);','if(wdmTrafficRefreshCursor!=1)return 10;'),('frame2','wdmTrafficRefreshCursor=0;wdmTrafficRefreshPending=true;Event("WorldMap_TrafficFrame","f",2.0/60.0);','if(wdmTrafficRefreshCursor!=2)return 11;'),('frame3','wdmTrafficRefreshCursor=0;wdmTrafficRefreshPending=true;Event("WorldMap_TrafficFrame","f",3.0/60.0);','if(wdmTrafficRefreshCursor!=3)return 12;')]:
 d=r/('timing-'+name);d.mkdir(exist_ok=True);count=1000
 body=base+'\nWdmTrafficBeginRefresh();\nfor(int tick=0;tick<'+str(count)+';tick++){'+loop+assertion+'}\nreturn 0;}\n';(d/'cases.c').write_text(body)
 t=time.perf_counter();p=subprocess.run(['/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/.cache/build/bin/script-vm-probe',str(d),'Probe'],env={**os.environ,'DYLD_LIBRARY_PATH':'/opt/homebrew/lib'},capture_output=True,text=True);elapsed=(time.perf_counter()-t)*1000
 print(name,p.returncode,p.stdout.strip(),round(elapsed,3));assert p.returncode==0
 timing[name]=elapsed
for name in list(timing)[1:]:timing[name+'-avg-ms']=(timing[name]-timing['seed'])/1000
(r/'timings.json').write_text(json.dumps(timing,indent=2)+'\n');print(json.dumps(timing,indent=2))
