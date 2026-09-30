#!/usr/bin/env python3
"""Encode generated RGB foliage detail under the original game alpha contract.

No runtime writes. Existing silhouettes, alpha holes and atlas coordinates are
authoritative; generated alpha drift never changes the geometry-facing mask.
"""
from pathlib import Path
import hashlib,importlib.util,json,struct
from PIL import Image,ImageDraw
ROOT=Path(__file__).resolve().parent
ALPHA_REF=160
SOURCE_TEXTURES=ROOT.parent.parent.parent/"native-storm/.cache/runtime/RESOURCE/Textures"
codec_spec=importlib.util.spec_from_file_location("material_codec",ROOT.parent/"prepare.py")
codec=importlib.util.module_from_spec(codec_spec);codec_spec.loader.exec_module(codec)
ASSETS={
 'treePalms': ['Islands/Bermudes/Bermudes.gm','Islands/Nevis/Nevis.gm','Islands/Martinique/Martinique.gm','Islands/SentMartin/SentMartin_fort1.gm'],
 'trees':['Islands/Nevis/Nevis.gm','Islands/SentMartin/SentMartin_fort1.gm'],
}
PROMPTS={
 'treePalms':'Precise-object-edit: upgrade palm frond and palm bark UV atlas to natural realistic diffuse material; exact 2:1 atlas layout, all four half-fronds, stem baselines, region boundaries and silhouettes preserved; muted green, subtle veins, tactile weathered bark, neutral light; no new objects, repacking, halos, sharpening, strong contrast, baked highlights, text or checkerboard; genuine original transparency.',
 'trees':'Precise-object-edit: upgrade exact square tropical tree UV atlas; four upper-left foliage strips, upper-right tree, top-right bark strip, central bark rectangle and two lower foliage masses remain fixed; natural realistic muted leaf/bark detail, neutral diffuse light; no new branches, repacking, outlines, sharpening, strong contrast or text; genuine original transparency.',
}
def sha(data):return hashlib.sha256(data).hexdigest()
def coverage(alpha):
 # Storm's shared geometry state is D3DCMP_GREATER with AlphaRef 0xa0.
 hist=alpha.histogram();return sum(hist[ALPHA_REF+1:])/(alpha.width*alpha.height)
def preserve_coverage(image,target):
 alpha=image.getchannel('A');low,high=0.,8.;best=alpha;error=abs(coverage(alpha)-target)
 for _ in range(16):
  scale=(low+high)/2;candidate=alpha.point(lambda x:min(255,round(x*scale)))
  current=coverage(candidate)
  if abs(current-target)<error:best=candidate;error=abs(current-target)
  if current<target:low=scale
  else:high=scale
 image.putalpha(best);return image
def main():
 (ROOT/'prepared').mkdir(exist_ok=True);records=[]
 for name,consumers in ASSETS.items():
  original=codec.decode_tx((SOURCE_TEXTURES/f'{name}.tga.tx').read_bytes()).convert('RGBA')
  generated=Image.open(ROOT/f'{name}-generated.png').convert('RGBA');source_size=generated.size
  generated=generated.resize(original.size,Image.Resampling.LANCZOS)
  # Registration is uniform, never crop/warp/repack. Original pixels fill holes
  # invented by the generator; its new opaque regions outside the atlas vanish.
  trusted=generated.getchannel('A').point(lambda a:255 if a>=240 else 0)
  result=Image.composite(generated,original,trusted);result.putalpha(original.getchannel('A'))
  assert result.getchannel('A').tobytes()==original.getchannel('A').tobytes()
  result.save(ROOT/f'{name}-replacement.png')
  base_coverage=coverage(result.getchannel('A'));levels=[];mips=[];mip=result
  while True:
   raw=mip.tobytes('raw','BGRA');levels.append(raw);mips.append({'width':mip.width,'height':mip.height,'alpha_ref':ALPHA_REF,'comparison':'GREATER','coverage':coverage(mip.getchannel('A')),'bytes':len(raw)})
   if mip.size==(1,1):break
   # Premultiplied filtering prevents hidden transparent RGB from bleeding into
   # leaf edges. Alpha-coverage preservation avoids distant foliage thinning.
   size=(max(1,mip.width//2),max(1,mip.height//2))
   mip=mip.convert('RGBa').resize(size,Image.Resampling.LANCZOS).convert('RGBA')
   mip=preserve_coverage(mip,base_coverage)
  data=struct.pack('<6I',0,result.width,result.height,len(levels),21,len(levels[0]))+b''.join(levels)
  target=ROOT/'prepared'/f'{name}.tga.tx';target.write_bytes(data)
  _,w,h,count,fmt,size=struct.unpack('<6I',data[:24]);decoded=Image.frombytes('RGBA',(w,h),data[24:24+size],'raw','BGRA')
  assert decoded.tobytes()==result.tobytes() and count==len(mips) and fmt==21
  assert len(data)==24+sum(x['bytes'] for x in mips)
  records.append({'target':target.name,'prepared_path':str(target.relative_to(ROOT.parent)),'role':'alpha-tested foliage UV atlas','source':f'foliage-new/{name}-generated.png','source_sha256':sha((ROOT/f'{name}-generated.png').read_bytes()),'original_decoded_sha256':sha(original.tobytes()),'output_sha256':sha(data),'width':w,'height':h,'source_width':source_size[0],'source_height':source_size[1],'format':'A8R8G8B8','mip_count':count,'bytes':len(data),'alpha':'Level0 byte-exact original alpha; premultiplied-filtered mip coverage preserved for Storm GREATER 0xa0 alpha test','alpha_test':{'ref':ALPHA_REF,'comparison':'GREATER','source':'native-storm renderer SetCommonStates'},'composition':'Uniform resize, no crop/repack; generated RGB under original alpha; original RGB retained wherever generated alpha <240','tool':'builtin imagegen','prompt':PROMPTS[name],'license':'Generated edit of user-supplied game atlas; not CC0','consumers':consumers,'mips':mips,'runtime_status':'candidate only; not accepted or staged by this pipeline'})
  # Inspect the real encoded level0 against white and dark backgrounds.
  thumb=(768,max(1,round(768*h/w)));sheet=Image.new('RGB',(1536,thumb[1]*2+40),'#252525');draw=ImageDraw.Draw(sheet)
  draw.text((10,10),name+' original / encoded replacement, original alpha retained',fill='white')
  for row,bg in enumerate(('#f3f3f3','#122027')):
   for col,im in enumerate((original,decoded)):
    tile=Image.new('RGBA',im.size,bg);tile.alpha_composite(im);sheet.paste(tile.convert('RGB').resize(thumb),(col*768,40+row*thumb[1]))
  sheet.save(ROOT/f'{name}-comparison.png')
  print(name,result.size,len(levels),'mips',len(data),'bytes',sha(data))
 (ROOT/'manifest.json').write_text(json.dumps(records,indent=2)+'\n')
if __name__=='__main__':main()
