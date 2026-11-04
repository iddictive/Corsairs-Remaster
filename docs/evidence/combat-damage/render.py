import json,math,sys
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw,ImageFont
root=Path(__file__).resolve().parent
data=json.loads((root/'man.json').read_text())
def panel(triangles,label,close=False):
 w,h=600,700
 pixels=np.zeros((h,w,3),np.uint8);pixels[:]=(25,30,38);depth=np.full((h,w),-np.inf)
 a=math.radians(-18);right=np.array([math.cos(a),0,-math.sin(a)]);front=np.array([math.sin(a),0,math.cos(a)])
 zoom=340 if not close else 1350;floor=0 if not close else 1.05
 light=np.array([.3,.65,.7]);light/=np.linalg.norm(light)
 for triangle in triangles:
  t=np.array(triangle);p=t[:,:3];xy=np.column_stack((w/2+(p@right)*zoom,h-35-(p[:,1]-floor)*zoom));z=p@front
  lo=np.maximum(np.floor(xy.min(0)).astype(int),0);hi=np.minimum(np.ceil(xy.max(0)).astype(int),(w-1,h-1))
  if np.any(lo>hi):continue
  (x0,y0),(x1,y1),(x2,y2)=xy;det=(y1-y2)*(x0-x2)+(x2-x1)*(y0-y2)
  if abs(det)<1e-9:continue
  y,x=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]+.5;u=((y1-y2)*(x-x2)+(x2-x1)*(y-y2))/det;v=((y2-y0)*(x-x2)+(x0-x2)*(y-y2))/det;q=1-u-v
  zz=u*z[0]+v*z[1]+q*z[2];area=np.s_[lo[1]:hi[1]+1,lo[0]:hi[0]+1];mask=(u>=0)&(v>=0)&(q>=0)&(zz>depth[area])
  normal=np.cross(p[1]-p[0],p[2]-p[0]);brightness=.45+.55*abs(normal@light)/max(np.linalg.norm(normal),1e-9)
  color=u[...,None]*t[0,3:6]+v[...,None]*t[1,3:6]+q[...,None]*t[2,3:6];alpha=(u*t[0,6]+v*t[1,6]+q*t[2,6])[...,None]/255;pixels[area][mask]=np.clip(color[mask]*brightness*alpha[mask]+pixels[area][mask]*(1-alpha[mask]),0,255);depth[area][mask]=zz[mask]
 out=Image.fromarray(pixels);d=ImageDraw.Draw(out);font=ImageFont.truetype('/System/Library/Fonts/Supplemental/Arial.ttf',20);d.text((14,14),label,font=font,fill='white');return out
labels=['Impact / guard pose','Same wound / bind pose','Fatal cut / detached part']
canvas=Image.new('RGB',(1800,1400))
for i,frame in enumerate(data['frames']):
 canvas.paste(panel(frame,labels[i]),(i*600,0));canvas.paste(panel(frame,'Surface detail',True),(i*600,700))
canvas.save(root/'man-component.png')
print(root/'man-component.png')
