import pathlib,subprocess,shlex,sys
source=pathlib.Path(__file__).resolve().parent
root=source.parents[3]; native=root/'experiments/native-metal'; build=native/'.cache/build'
if len(sys.argv)!=2:
    sys.exit('usage: python3 run.py EMPTY_SCRATCH_DIR')
work=pathlib.Path(sys.argv[1]).resolve()
if not work.is_dir() or any(work.iterdir()):
    sys.exit('scratch directory must exist and be empty')
commands=subprocess.check_output([str(native/'inputs/toolchain/bin/ninja'),'-C',str(build),'-t','commands','gpu-skinning-offscreen-probe'],text=True).splitlines()
s=(native/'backend.mm').read_text(); edits=[('if(!window||!presentation||!presentation->width||!presentation->height)','if(!presentation||!presentation->width||!presentation->height)'),('view=SDL_Metal_CreateView((SDL_Window*)window);layer=(__bridge CAMetalLayer*)SDL_Metal_GetLayer(view);','view=nullptr;layer=nil;'),('SDL_Metal_GetDrawableSize((SDL_Window*)window,&drawableWidth,&drawableHeight);',''),('SDL_Metal_DestroyView(view);','if(view)SDL_Metal_DestroyView(view);')]
for a,b in edits:
 assert s.count(a)==1,(a,s.count(a));s=s.replace(a,b)
(work/'backend.mm').write_text(s)
cmd=shlex.split(next(c for c in commands if ' -c ' in c and c.endswith('/backend.mm')))
cmd[cmd.index('-o')+1]=str(work/'backend.o');cmd[cmd.index('-c')+1]=str(work/'backend.mm');cmd[cmd.index('-MF')+1]=str(work/'backend.d');subprocess.run(cmd,cwd=build,check=True)
cmd=shlex.split(next(c for c in commands if ' -c ' in c and c.endswith('/gpu_skinning_offscreen_probe.cpp')));cmd[cmd.index('-o')+1]=str(work/'scene.o');cmd[cmd.index('-c')+1]=str(source/'scene.cpp');cmd[cmd.index('-MF')+1]=str(work/'scene.d');subprocess.run(cmd,cwd=build,check=True)
cmd=shlex.split(commands[-1])[2:-2];cmd[cmd.index('CMakeFiles/gpu-skinning-offscreen-probe.dir/gpu_skinning_offscreen_probe.cpp.o')]=str(work/'scene.o');cmd[cmd.index('-o')+1]=str(work/'scene');cmd.insert(cmd.index('libstorm_metal.a'),str(work/'backend.o'));subprocess.run(cmd,cwd=build,check=True)
subprocess.run([str(work/'scene')],cwd=work,check=True,timeout=90)
