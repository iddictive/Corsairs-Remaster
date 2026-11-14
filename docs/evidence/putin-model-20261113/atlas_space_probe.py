"""Find actually unused stock-atlas rectangles after the face UV split."""
import struct
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFilter

here=Path(__file__).resolve().parent
data=(here/'identity-geometry-face-prototype.gm').read_bytes();h=struct.unpack_from('<11i7f',data)
tri_at=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]+108*h[7]+104*h[8]
faces=np.frombuffer(data,dtype='<u2',count=h[9]*3,offset=tri_at).reshape(-1,3)
buff=tri_at+6*h[9];kind,size=struct.unpack_from('<2i',data,buff);assert kind==4
dt=np.dtype({'names':['uv'],'formats':[('<f4',2)],'offsets':[36],'itemsize':44})
uv=np.frombuffer(data,dtype=dt,count=size//44,offset=buff+8)['uv']
stock=faces[np.all(faces<2083,axis=1)]
im=Image.new('L',(1024,1024));draw=ImageDraw.Draw(im)
for face in stock:
    q=uv[face]*2048
    draw.polygon([tuple(map(float,p)) for p in q],fill=255)
mask=np.asarray(im.filter(ImageFilter.MaxFilter(17)))>0
integral=np.pad(mask.astype(np.int32),((1,0),(1,0))).cumsum(0).cumsum(1)
print('occupied incl padding',int(mask.sum()),'of',mask.size)
for side in (512,384,256,192,128,96,64):
    sums=integral[side:,side:]-integral[:-side,side:]-integral[side:,:-side]+integral[:-side,:-side]
    xy=np.argwhere(sums==0)
    print('square',side,'available placements',len(xy),'first xy',tuple(int(x) for x in xy[0][::-1]) if len(xy) else None)
Image.fromarray((mask*255).astype('uint8')).save(here/'stock-atlas-used-mask.png')
