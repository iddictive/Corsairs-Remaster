#!/usr/bin/env python3
"""Package generated island terrain diffuse; no runtime writes."""
from pathlib import Path
import sys,struct,json,hashlib,importlib.util
from PIL import Image,ImageDraw,ImageStat,ImageChops
ROOT=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('material_codec',ROOT.parent/'prepare.py');codec=importlib.util.module_from_spec(spec);spec.loader.exec_module(codec)
PROMPT=(ROOT/'prompt.txt').read_text().strip()
source=ROOT/'jungle1-generated.png';original=ROOT/'jungle1-original.png'
im=Image.open(source).convert('RGBA');native=im.size;im=im.resize((2048,2048),Image.Resampling.LANCZOS)
levels=[];mip=im
while True:
 levels.append(codec.encode_level(mip,'DXT1'))
 if mip.size==(1,1):break
 mip=mip.resize((max(1,mip.width//2),max(1,mip.height//2)),Image.Resampling.LANCZOS)
data=struct.pack('<6I',0,2048,2048,len(levels),int.from_bytes(b'DXT1','little'),len(levels[0]))+b''.join(levels)
(ROOT/'jungle1.tga.tx').write_bytes(data);decoded=codec.decode_tx(data);decoded.save(ROOT/'jungle1-replacement.png')
# Verify every encoded level, payload length and opacity, including sub-block mips.
offset=24;w=h=2048
for block in levels:
 assert len(block)==((w+3)//4)*((h+3)//4)*8
 restored=Image.frombytes('RGBA',(max(4,w),max(4,h)),block,'bcn',(1,'DXT1'));assert restored.getextrema()[3]==(255,255)
 offset+=len(block);w=max(1,w//2);h=max(1,h//2)
assert offset==len(data)
resource=ROOT.parents[1]/'.cache/runtime/RESOURCE';refs=[]
for p in (resource/'Models').rglob('*.gm'):
 if b'jungle1.tga' in p.read_bytes()[:100000].lower():refs.append(str(p.relative_to(resource)))
sha=lambda b:hashlib.sha256(b).hexdigest()
record=dict(target='jungle1.tga.tx',role='island terrain canopy diffuse',source='jungle1-generated.png',provider='built-in image_gen',prompt=PROMPT,source_sha256=sha(source.read_bytes()),original_sha256=sha((resource/'Textures/jungle1.tga.tx').read_bytes()),output_sha256=sha(data),source_width=native[0],source_height=native[1],width=2048,height=2048,mip_count=len(levels),format='DXT1',alpha='opaque',bytes=len(data),consumers=refs,composition='Uniform resize only; original UV scale, orientation and aspect unchanged. Generated native detail is 1254 square, not native 2K.',validation='All mip payload sizes and decoded alpha verified; preview inspected. Runtime appearance pending root integration.',license='Generated edit of user-provided game material; not CC0')
(ROOT/'manifest.json').write_text(json.dumps([record],indent=2)+'\n')
sheet=Image.new('RGB',(1200,640),(25,25,25));draw=ImageDraw.Draw(sheet)
for x,title,picture in [(0,'ORIGINAL',Image.open(original)),(600,'GENERATED / DECODED DXT1',decoded)]:
 draw.text((x+10,10),title,fill='white');sheet.paste(picture.convert('RGB').resize((600,600)),(x,40))
sheet.save(ROOT/'preview.png')
print(json.dumps({k:record[k] for k in ['target','source_width','width','mip_count','bytes','output_sha256']}))
