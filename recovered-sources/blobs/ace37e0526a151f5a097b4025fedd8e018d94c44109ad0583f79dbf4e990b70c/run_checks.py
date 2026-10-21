import subprocess,pathlib,json,time,hashlib,concurrent.futures
root=pathlib.Path('/REQUIRED_EXTERNAL_INPUT/Corsairs');tmp=pathlib.Path(__file__).parent
common=['clang++','-std=c++20','-O1','-g','-UNDEBUG','-DSTORM_METAL_SHIP_HULL_DAMAGE','-fsanitize=address,undefined','-fno-omit-frame-pointer','-Iexperiments/native-metal','-Iexperiments/native-metal/engine/src/libs/math/include','-Iexperiments/native-metal/engine/src/libs/geometry/src','-Iexperiments/native-metal/engine/src/libs/geometry/third_party/clipper2/include']
link=['experiments/native-metal/engine/src/libs/geometry/src/world_cut_planar.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.engine.cpp','experiments/native-metal/engine/src/libs/geometry/third_party/clipper2/src/clipper.triangulation.cpp']
results=[]
for name in ['beam','ship','mast']:
 command=common+[str(tmp/(name+'.cpp'))]+link+['-o',str(tmp/(name+'-checked'))]
 result=subprocess.run(command,cwd=root,text=True,capture_output=True);(tmp/(name+'-compile.log')).write_text(result.stdout+result.stderr)
 if result.returncode:print(result.stderr[-3000:]);raise SystemExit(result.returncode)
 before=time.monotonic();run=subprocess.run([str(tmp/(name+'-checked'))],cwd=root,text=True,capture_output=True)
 (tmp/(name+'-run.log')).write_text(run.stdout+run.stderr);results.append({'name':name,'command':command,'result':run.returncode,'elapsed':time.monotonic()-before,'stdout':run.stdout,'stderr':run.stderr})
 print(name,run.returncode,run.stdout,run.stderr[-1000:],flush=True)
 if run.returncode:break
(tmp/'checks.json').write_text(json.dumps(results,indent=2))
