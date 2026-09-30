#!/usr/bin/env python3
"""Prepare active sand and paving replacements; never writes to a game runtime."""
import hashlib, io, json, struct, urllib.request
from pathlib import Path
from PIL import Image, ImageDraw, __version__ as pillow_version
ROOT = Path(__file__).resolve().parent
SOURCES = [
 ('Sandtile', 'coast_sand_01', 'b3569f6ba0a1e3274dd637701cad6cc460513601094e83e706add0b18a34ea3d', 1024, 'DXT1'),
 ('tileU2', 'generated/tileU2.png', None, (512, 512), 'DXT1'),
]
PROMPTS = {
 'tileU2': 'Seamless square orthographic diffuse material: muted warm-gray coastal cobblestone paving with pale mortar, occasional subdued rusty-brown stones, evenly distributed medium-scale detail, flat neutral lighting; no perspective, text, watermark, oversharpening, halos, crunchy micro-contrast, baked strong lighting, deep black cracks, vignette.',
}
def sha(data): return hashlib.sha256(data).hexdigest()
def decode_tx(data):
 flags,w,h,mips,fmt,size = struct.unpack('<6I',data[:24])
 raw=data[24:24+size]
 if fmt == 21:  # D3DFMT_A8R8G8B8 stored little-endian as BGRA bytes
  return Image.frombytes('RGBA',(w,h),raw,'raw','BGRA')
 if fmt == 25:  # D3DFMT_A4R4G4B4
  raw=bytes(c for (v,) in struct.iter_unpack('<H',raw) for c in (((v>>8)&15)*17,((v>>4)&15)*17,(v&15)*17,((v>>12)&15)*17))
  return Image.frombytes('RGBA',(w,h),raw)
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
  generated=asset.startswith('generated')
  url=('generated/pierWood1.png' if asset=='generated' else asset) if generated else f'https://dl.polyhaven.org/file/ph-assets/Textures/png/4k/{asset}/{asset}_diff_4k.png'
  path=ROOT/url if generated else ROOT/'cache'/f'{asset}_diff_4k.png'
  if not path.exists() and not generated: urllib.request.urlretrieve(url,path)
  source_bytes=path.read_bytes()
  if digest: assert sha(source_bytes)==digest, f'Unexpected source content: {path}'
  source=Image.open(path).convert('RGBA'); assert source.getextrema()[3]==(255,255)
  if target == 'tileU2': assert source.width >= height[0] and source.height >= height[1]
  assert source.getextrema()[3]==(255,255)
  if isinstance(height, tuple):
   tile=source.resize(height,Image.Resampling.LANCZOS)
  else:
   tile=source.resize((height,height),Image.Resampling.LANCZOS)
  if generated:
   im=tile
  else:
   im=Image.new('RGBA',(height*2,height)); im.paste(tile,(0,0)); im.paste(tile,(height,0))
  levels=[]; mip=im
  while True:
   levels.append(encode_level(mip,codec))
   if mip.size==(1,1): break
   mip=mip.resize((max(1,mip.width//2),max(1,mip.height//2)),Image.Resampling.LANCZOS)
  data=struct.pack('<6I',0,im.width,im.height,len(levels),int.from_bytes(codec.encode(),'little'),len(levels[0]))+b''.join(levels)
  output=ROOT/'prepared'/f'{target}.tga.tx'; output.write_bytes(data)
  restored=decode_tx(data); assert restored.size==im.size
  preview_w=800
  preview_h=max(1,round(800*restored.height/restored.width))
  restored.resize((preview_w,preview_h)).save(ROOT/'previews'/f'{target}-replacement.png')
  original=ROOT/'previews'/f'{target}-original.png'
  if original.exists(): comparisons.append((target,Image.open(original).convert('RGB'),restored.convert('RGB')))
  records.append(dict(target=output.name,role='seamless diffuse material' if generated and target!='pierWood1' else ('UV atlas diffuse' if generated else 'seamless diffuse material'),source=url,page=None if generated else f'https://polyhaven.com/a/{asset}',prompt=PROMPTS.get(target),author='OpenAI image generation; generated material' if generated and target!='pierWood1' else ('OpenAI image generation; original atlas supplied by user' if generated else 'Rob Tuytel'),license='Generated material; not CC0' if generated and target!='pierWood1' else ('Generated edit of user-supplied game atlas; not CC0' if generated else 'CC0-1.0'),license_url=None if generated else 'https://polyhaven.com/license',source_width=source.width,source_height=source.height,source_sha256=sha(source_bytes),output_sha256=sha(data),width=im.width,height=im.height,format=codec,mip_count=len(levels),bytes=len(data),alpha='opaque',composition=('Uniform resize to original target dimensions; no crop, rearrangement or tiling' if generated and target!='pierWood1' else ('Uniform 1254-to-1024 downsample; no crop, rearrangement or tiling' if generated else 'Square tile repeated twice horizontally; no nonuniform stretching or cropping')),pillow_version=pillow_version))
  print(output.name,im.size,codec,len(levels),'mips',len(data),'bytes',sha(data))
 (ROOT/'manifest.json').write_text(json.dumps(records,indent=2)+'\n')
 if comparisons:
  sheet=Image.new('RGB',(1600,440*len(comparisons)),(25,25,25)); draw=ImageDraw.Draw(sheet)
  for row,(name,original,new) in enumerate(comparisons):
   y=row*440; draw.text((10,y+10),name+' ORIGINAL',fill='white'); draw.text((810,y+10),name+' REPLACEMENT (decoded .tx)',fill='white')
   size=(400,400) if original.width==original.height else (800,400)
   sheet.paste(original.resize(size),(0,y+40)); sheet.paste(new.resize(size),(800,y+40))
  sheet.save(ROOT/'previews/comparison.png')
if __name__=='__main__': main()
