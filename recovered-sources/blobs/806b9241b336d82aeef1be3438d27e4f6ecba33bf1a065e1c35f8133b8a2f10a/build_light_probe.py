from pathlib import Path
import shlex,subprocess,json
root=Path.cwd();native=root/'experiments/native-metal';base=root/'.local-archives/metal-renderer-evidence/destruction-player-followup-20261018'
commands=subprocess.check_output(['ninja','-C',str(native/'.cache/build'),'-t','commands','ffp_lighting_gpu-probe'],text=True).splitlines()
compile=next(shlex.split(c) for c in commands if 'ffp_lighting_gpu_probe.cpp' in c and ' -c ' in c)
compile[compile.index('-c')+1]=str(base/'sky_light_probe.cpp');compile[compile.index('-o')+1]=str(base/'sky_light_probe.o');compile[compile.index('-MF')+1]=str(base/'sky_light_probe.o.d')
link=next(shlex.split(c) for c in commands if ' -o bin/ffp_lighting_gpu-probe ' in c)[2:-2]
link[link.index('-o')+1]=str(base/'sky_light_probe')
idx=next(i for i,a in enumerate(link) if a.endswith('/ffp_lighting_gpu_probe.cpp.o'));link[idx]=str(base/'sky_light_probe.o')
(base/'sky-light-build.json').write_text(json.dumps({'compile':compile,'link':link},indent=2)+'\n')
with (base/'sky-light-build.log').open('w') as log:
 subprocess.run(compile,cwd=native/'.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)
 subprocess.run(link,cwd=native/'.cache/build',stdout=log,stderr=subprocess.STDOUT,check=True)
