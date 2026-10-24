"""Cartographic colour in the atlas frame; never sea-scale repeating UVs.

The original stock mesh is a source for the authored painted relief. A bake
provides the image-edit input, while the selected full-field PNG is the durable
source. Only chart render models consume this material, never sea models.
"""
import argparse
import importlib.util
import json
import math
from pathlib import Path
import struct

from PIL import Image, ImageChops, ImageDraw, ImageFilter

from patch_metal_portobello_ground import parse_gm
from prepare_puerto_rico_city import vertices, metadata
from prepare_seamless_sea_assets import OUTPUT, OWNER, source, sha

ROOT=Path(__file__).resolve().parents[1]
TEXTURE='RESOURCE/Textures/WorldMap/geometry/SeamlessSea_land.tga'
LAND_TEXTURES={'jungle1.tga','jungle_gray.tga','rockk1.tga','rockk1a.tga',
               'rockk2.tga','rockk4.tga','rockk4s.tga','rockk5.tga',
               'rocksh1.tga','continent1.tga','continent2.tga','continent3.tga'}


def texture_codec():
    # Reuse the established native texture producer and its mip/opacity checks.
    path=ROOT/'experiments/native-metal/materials/prepare.py'
    spec=importlib.util.spec_from_file_location('native_material_codec',path)
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    return module


def frame():
    chart=json.loads((OUTPUT/'chart/manifest.json').read_bytes())
    x0,z0,x1,z1=chart['chart']['bounds']['physical_min_xz_max_xz']
    # A square preserves map-space scale. Include the authored mainland too.
    x0,z0=min(x0,-2010.),min(z0,-1760.)
    x1,z1=max(x1,1120.),max(z1,990.)
    side=math.ceil(max(x1-x0,z1-z0)/32)*32+64
    return [math.floor((x0+x1-side)/2),math.floor((z0+z1-side)/2),side]


def bake(source_root, bounds=None):
    """Rasterize original authored UVs, with no lighting, symbols or labels."""
    codec=texture_codec();bounds=bounds or frame();x0,z0,side=bounds;size=4096
    g=parse_gm(source(source_root,'RESOURCE/MODELS/WorldMap/islands/mein.gm').read_bytes())
    result=Image.new('RGBA',(size,size));images={};faces=0
    for obj in g['objects']:
        name=g['textures'][g['materials'][obj[12]][10]]
        if name.lower() not in ('continent1.tga','continent2.tga','continent3.tga',
                                'islands.tga','islands2.tga','stkitts.tga'):continue
        if name not in images:
            images[name]=codec.decode_tx(source(source_root,
                'RESOURCE/Textures/WorldMap/geometry/'+name+'.tx').read_bytes()).convert('RGB')
        image=images[name]
        for tri in vertices(g,obj):
            if max(v[1] for v in tri)<0:continue
            pts=[((v[0]-x0)/side*size,(z0+side-v[2])/side*size) for v in tri]
            left=max(0,math.floor(min(p[0] for p in pts)));top=max(0,math.floor(min(p[1] for p in pts)))
            right=min(size,math.ceil(max(p[0] for p in pts)));bottom=min(size,math.ceil(max(p[1] for p in pts)))
            if right<=left or bottom<=top:continue
            a,b,c=pts;det=(b[0]-a[0])*(c[1]-a[1])-(c[0]-a[0])*(b[1]-a[1])
            if abs(det)<1e-8:continue
            # UV coordinates wrap in the source renderer. All vertices of a
            # stock face share a continuous chart of the packed source image.
            us=[v[7] for v in tri];vs=[v[8] for v in tri]
            offsets=(math.floor(min(us)),math.floor(min(vs)))
            affine=[]
            for values,extent,offset in ((us,image.width,offsets[0]),(vs,image.height,offsets[1])):
                da,db=values[1]-values[0],values[2]-values[0]
                dx=(da*(c[1]-a[1])-db*(b[1]-a[1]))/det
                dy=((b[0]-a[0])*db-(c[0]-a[0])*da)/det
                affine.extend((dx*extent,dy*extent,
                    (values[0]-offset+dx*(left-a[0])+dy*(top-a[1]))*extent))
            tile=image.transform((right-left,bottom-top),Image.Transform.AFFINE,affine,
                                 Image.Resampling.BILINEAR)
            mask=Image.new('L',tile.size);ImageDraw.Draw(mask).polygon(
                [(x-left,y-top) for x,y in pts],fill=255)
            result.paste(tile,(left,top),mask);faces+=1
    target=OUTPUT/'chart/albedo-source.png';result.save(target)
    (OUTPUT/'chart/albedo-frame.json').write_text(json.dumps({'frame':bounds,'size':size,'faces':faces},indent=2)+'\n')
    print(json.dumps({'image':str(target),'frame':bounds,'size':size,'faces':faces}))
    return result


def material(source_root):
    image_path=OWNER/'chart-albedo.png'
    spec=json.loads((OWNER/'chart-albedo.json').read_bytes())
    if sha(image_path.read_bytes())!=spec['sha256']:raise ValueError('Chart albedo source changed')
    image=Image.open(image_path).convert('RGBA')
    if image.getextrema()[3]!=(255,255):raise ValueError('Chart land albedo must be opaque')
    # Four unique overlapping authored regions supply detail, not a repeated
    # ground tile. Their common coordinate frame fixes every model seam.
    regions=spec['detail_regions'];size=4096;span=round(1728/3200*size);offset=size-span
    overlap=span-offset
    halves=[]
    for row in ('north','south'):
        half=Image.new('RGBA',(size,span))
        for column in ('west','east'):
            region=regions[row+'-'+column];path=OWNER/region['file']
            if sha(path.read_bytes())!=region['sha256']:raise ValueError('Chart detail region changed')
            part=Image.open(path).convert('RGBA')
            if part.getextrema()[3]!=(255,255):raise ValueError('Transparent chart detail region')
            part=part.resize((span,span),Image.Resampling.LANCZOS)
            if column=='west':half.paste(part,(0,0))
            else:
                ramp=Image.new('L',(span,1),255)
                ramp.putdata([round(255*(x/overlap)**2*(3-2*x/overlap)) if x<overlap else 255 for x in range(span)])
                half.paste(part,(offset,0),ramp.resize((span,span)))
        halves.append(half)
    image=Image.new('RGBA',(size,size));image.paste(halves[0],(0,0))
    ramp=Image.new('L',(1,span),255)
    ramp.putdata([round(255*(y/overlap)**2*(3-2*y/overlap)) if y<overlap else 255 for y in range(span)])
    image.paste(halves[1],(0,offset),ramp.resize((size,span)))
    # Keep the original cartography's actual detail, not an AI redraw of it.
    # The authored extension only fills outside that image. Feather inward
    # beyond its obsolete beach line, so old coast outlines cannot reappear
    # as yellow stripes in the new inland. This is an offline texture bake.
    original=bake(source_root,spec['frame'])
    coverage=original.getchannel('A').filter(ImageFilter.BoxBlur(96)).point(
        lambda a:max(0,min(255,round((a-192)*255/63))))
    coverage=ImageChops.multiply(coverage,original.getchannel('A'))
    image=image.resize(original.size,Image.Resampling.LANCZOS)
    image.paste(original,(0,0),coverage)
    codec=texture_codec();levels=[];mip=image
    while True:
        levels.append(codec.encode_level(mip,'DXT1'))
        if mip.size==(1,1):break
        mip=mip.resize((max(1,mip.width//2),max(1,mip.height//2)),Image.Resampling.LANCZOS)
    data=struct.pack('<6I',0,image.width,image.height,len(levels),int.from_bytes(b'DXT1','little'),len(levels[0]))+b''.join(levels)
    return spec,data


def project(data,world,spec):
    """One colour field across coastal children, islands and continental fill."""
    g=parse_gm(data);result=bytearray(data);x0,z0,side=spec['frame']
    a,b,c,d,tx,tz=world
    selected={i for i,name in enumerate(g['textures'])
              if name.replace('\\','/').split('/')[-1].lower() in LAND_TEXTURES}
    used,retained=set(),set()
    for obj in g['objects']:
        target=used if g['materials'][obj[12]][10] in selected else retained
        target.update((obj[7],v+obj[11]) for tri in g['triangles'][obj[9]:obj[9]+obj[8]] for v in tri)
    if used & retained:raise ValueError('Chart land shares UV vertices with retained scenery')
    for buffer,index in used:
        buf=g['buffers'][buffer]
        offset=buf['payload_offset']+index*buf['stride']
        x,_,z=struct.unpack_from('<3f',data,offset)
        u=(a*x+b*z+tx-x0)/side;v=(z0+side-c*x-d*z-tz)/side
        if not (-.001<=u<=1.001 and -.001<=v<=1.001):
            raise ValueError('Chart vertex outside nonrepeating albedo')
        # Painted cartographic relief already contains its illumination.
        # Sea-scale cliff normals/colours would add polygon-shaped patches.
        struct.pack_into('<3fI2f',result,offset+12,0.,1.,0.,0xff7f7f7f,u,v)
    return metadata(bytes(result),texture_names={i:TEXTURE.replace('/','\\') for i in selected})


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bake-source',type=Path,required=True)
    bake(parser.parse_args().bake_source)
