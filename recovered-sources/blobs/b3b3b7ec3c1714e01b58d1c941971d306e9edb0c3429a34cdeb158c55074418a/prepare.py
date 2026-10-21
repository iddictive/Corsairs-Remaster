#!/usr/bin/env python3
"""Prepare two diffuse-only CC0 replacements; never writes to a game runtime."""
import hashlib, io, json, struct, urllib.request
from pathlib import Path
from PIL import Image, ImageDraw, __version__ as pillow_version
ROOT = Path(__file__).resolve().parent
SOURCES = [
 ('Sandtile', 'coast_sand_01', 'b3569f6ba0a1e3274dd637701cad6cc460513601094e83e706add0b18a34ea3d', 1024, 'DXT1'),
 ('rockK3', 'rock_06', '53e17b2a604be2c3caa1dd4f248a40fed053d1930a21972d5195c799268dc2d5', 2048, 'DXT5'),
]
def sha(data): return hashlib.sha256(data).hexdigest()
def decode_tx(data):
 flags,w,h,mips,fmt,size = struct.unpack('<6I',data[:24])
 raw=data[24:24+size]
 if fmt == 23:
  raw=bytes(c for (v,) in struct.iter_unpack('<H',raw) for c in (((v>>11)&31)*255//31,((v>>5)&63)*255//63,(v&31)*255//31,255))
  return Image.frombytes('RGBA',(w,h),raw)
 codec='DXT1' if fmt==int.from_bytes(b'DXT1','little') else 'DXT5'
 return Image.frombytes('RGBA',(w,h),raw,'bcn',(1 if codec=='DXT1' else 3,codec))
def encode_level(im,codec):
 w,h=im.size
 padded=Image.new('RGBA',(max(4,w),max(4,h)))
 padded.paste(im)
 if w<4 or h<4:
  for y in range(padded.height):
   for x in range(padded.width): padded.putpixel((x,y),im.getpixel((min(x,w-1),min(y,h-1))))
 out=io.BytesIO(); padded.save(out,format='DDS',pixel_format=codec)
 data=out.getvalue(); assert data[84:88]==codec.encode()
 payload=data[128:]
 assert len(payload)==((w+3)//4)*((h+3)//4)*(8 if codec=='DXT1' else 16)
 decoded=Image.frombytes('RGBA',padded.size,payload,'bcn',(1 if codec=='DXT1' else 3,codec))
 assert decoded.getextrema()[3]==(255,255)
 return payload

def main():
 for directory in ('cache','prepared','previews'): (ROOT/directory).mkdir(exist_ok=True)
 records=[]; comparisons=[]
 for target,asset,digest,height,codec in SOURCES:
  url=f'https://dl.polyhaven.org/file/ph-assets/Textures/png/4k/{asset}/{asset}_diff_4k.png'
  path=ROOT/'cache'/f'{asset}_diff_4k.png'
  if not path.exists(): urllib.request.urlretrieve(url,path)
  assert sha(path.read_bytes())==digest, f'Unexpected source content: {path}'
  source=Image.open(path).convert('RGBA'); assert source.size==(4096,4096)
  assert source.getextrema()[3]==(255,255)
  tile=source.resize((height,height),Image.Resampling.LANCZOS)
  im=Image.new('RGBA',(height*2,height)); im.paste(tile,(0,0)); im.paste(tile,(height,0))
  levels=[]; mip=im
  while True:
   levels.append(encode_level(mip,codec))
   if mip.size==(1,1): break
   mip=mip.resize((max(1,mip.width//2),max(1,mip.height//2)),Image.Resampling.LANCZOS)
  data=struct.pack('<6I',0,im.width,im.height,len(levels),int.from_bytes(codec.encode(),'little'),len(levels[0]))+b''.join(levels)
  output=ROOT/'prepared'/f'{target}.tga.tx'; output.write_bytes(data)
  restored=decode_tx(data); assert restored.size==im.size
  restored.resize((800,400)).save(ROOT/'previews'/f'{target}-replacement.png')
  original=ROOT/'previews'/f'{target}-original.png'
  if original.exists(): comparisons.append((target,Image.open(original).convert('RGB'),restored.convert('RGB')))
  records.append(dict(target=output.name,role='seamless diffuse material',source=url,page=f'https://polyhaven.com/a/{asset}',author='Rob Tuytel',license='CC0-1.0',license_url='https://polyhaven.com/license',source_sha256=digest,output_sha256=sha(data),width=im.width,height=im.height,format=codec,mip_count=len(levels),bytes=len(data),alpha='opaque',composition='Square tile repeated twice horizontally; no nonuniform stretching or cropping',pillow_version=pillow_version))
  print(output.name,im.size,codec,len(levels),'mips',len(data),'bytes',sha(data))
 (ROOT/'manifest.json').write_text(json.dumps(records,indent=2)+'\n')
 if comparisons:
  sheet=Image.new('RGB',(1600,440*len(comparisons)),(25,25,25)); draw=ImageDraw.Draw(sheet)
  for row,(name,original,new) in enumerate(comparisons):
   y=row*440; draw.text((10,y+10),name+' ORIGINAL',fill='white'); draw.text((810,y+10),name+' REPLACEMENT (decoded .tx)',fill='white')
   sheet.paste(original.resize((800,400)),(0,y+40)); sheet.paste(new.resize((800,400)),(800,y+40))
  sheet.save(ROOT/'previews/comparison.png')
if __name__=='__main__': main()
