from pathlib import Path
import struct,json,sys
destination=Path('/tmp/corsairs-world-cut-replay-mJ2rBP')
root=Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/MODELS/Islands')
def extract(path):
 data=path.read_bytes();at=0
 def read(fmt):
  nonlocal at
  result=struct.unpack_from('<'+fmt,data,at);at+=struct.calcsize('<'+fmt);return result
 h=read('11i7f');strings=data[at:at+h[2]];at+=h[2]+h[3]*4
 name=lambda index:strings[index:].split(b'\0',1)[0].decode('cp1251','replace')
 textures=[name(read('i')[0]) for _ in range(h[4])];materials=[read('2i4f8i') for _ in range(h[5])]
 at+=h[6]*80+h[7]*108
 objects=[read('3i4f6i8i4ii') for _ in range(h[8])]
 triangles=[read('3H') for _ in range(h[9])];descriptors=[read('2i') for _ in range(h[10])];buffers=[]
 for typ,size in descriptors:
  if typ>>2:raise ValueError('animated asset')
  stride=36+(typ&3)*8;buffers.append((data[at:at+size],stride));at+=size
 output=[];meta=[]
 for o,obj in enumerate(objects):
  flags=obj[2];vb,count,start,vertex_count,vertex_start,material=obj[7:13];raw,stride=buffers[vb];mat=materials[material];texture=textures[mat[10]] if 0<=mat[10]<len(textures) else ''
  meta.append(dict(object=o,flags=flags,texture=texture,texture_type=mat[6],triangles=count,stride=stride))
  if not flags&17:continue
  for t in triangles[start:start+count]:
   encoded=struct.pack('<i',o)
   for index in t:
    index+=vertex_start;value=raw[index*stride:(index+1)*stride];assert len(value)==stride
    encoded+=value+bytes(60-stride)
   output.append(encoded)
 target=destination/(path.parent.name+'-'+path.stem+'.bin');target.write_bytes(struct.pack('<I',len(output))+b''.join(output))
 info=dict(asset=str(path),input=str(target),objects=meta,bounds=[h[11:14],h[14:17]],triangles=len(output))
 target.with_suffix('.json').write_text(json.dumps(info,indent=2));print(json.dumps(info))
for island in ('Antigua','Cumana'):
 for path in sorted((root/island).glob('*.gm')):
  if 'locator' in path.name.lower():continue
  extract(path)
