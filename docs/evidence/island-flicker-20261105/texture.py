import sys,struct,pathlib
sys.path.insert(0,'experiments/native-metal/materials');import prepare
base=pathlib.Path('experiments/native-metal/.cache/runtime/RESOURCE/Textures'); out=pathlib.Path(sys.argv[1])
for i,name in enumerate(('jungle_gray','rockA2','rockK4','beach1')):
 data=(base/(name+'.tga.tx')).read_bytes();flags,w,h,mips,fmt,size=struct.unpack('<6I',data[:24]);offset=24
 with (out/f'tex{i}.bin').open('wb') as f:
  f.write(struct.pack('<III',w,h,mips))
  for level in range(mips):
   width=max(1,w>>level);height=max(1,h>>level);size=((width+3)//4)*((height+3)//4)*8;chunk=data[offset:offset+size];offset+=size
   im=prepare.decode_tx(struct.pack('<6I',flags,width,height,1,fmt,size)+chunk).convert('RGBA');f.write(im.tobytes('raw','BGRA'))
 print(name,w,h,mips,offset==len(data))
