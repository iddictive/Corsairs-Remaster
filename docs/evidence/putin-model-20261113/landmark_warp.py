"""Piecewise-affine reference alignment to a real GM portrait, offline only."""
import json
import sys
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFilter

here=Path(__file__).resolve().parent
names=('contour','leftBrow','rightBrow','leftEye','rightEye','nose','noseCrest','outerLips')
baseline_name=sys.argv[1] if len(sys.argv)>1 else 'officer_5-original-preview'
source_name=sys.argv[2] if len(sys.argv)>2 else 'reference-putin-2024'
ref=json.loads((here/(source_name+'-landmarks.json')).read_text())
dst=json.loads((here/(baseline_name+'-landmarks.json')).read_text())
src=np.array([p for n in names for p in ref[n]],dtype=float)
pts=np.array([p for n in names for p in dst[n]],dtype=float)
assert len(src)==len(pts)

# Bowyer-Watson Delaunay triangulation over detected, paired landmarks.
base=len(pts); allpts=np.vstack((pts,[[-1000,-1000],[2000,-1000],[256,2000]]))
triangles=[(base,base+1,base+2)]
def inside(p,a,b,c):
    ax,ay=a-p; bx,by=b-p; cx,cy=c-p
    det=(ax*ax+ay*ay)*(bx*cy-by*cx)-(bx*bx+by*by)*(ax*cy-ay*cx)+(cx*cx+cy*cy)*(ax*by-ay*bx)
    orient=(b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
    return det*orient>1e-6
for i,p in enumerate(pts):
    bad=[t for t in triangles if inside(p,*(allpts[j] for j in t))]
    edges={}
    for a,b,c in bad:
        for e in ((a,b),(b,c),(c,a)):
            key=tuple(sorted(e)); edges[key]=edges.get(key,0)+1
    triangles=[t for t in triangles if t not in bad]
    triangles.extend((a,b,i) for (a,b),count in edges.items() if count==1)
triangles=[t for t in triangles if max(t)<base]
photo=np.asarray(Image.open(here/(source_name+'.png')).convert('RGB'))
baseline_path=here/(baseline_name+'.png') if len(sys.argv)>1 else here/'officer_5-candidate-preview.png'
out=np.asarray(Image.open(baseline_path).convert('RGB')).copy()
cover=np.zeros((512,512),dtype='uint8')
for t in triangles:
    d=pts[list(t)]; s=src[list(t)]
    lo=np.maximum(np.floor(d.min(0)).astype(int),0); hi=np.minimum(np.ceil(d.max(0)).astype(int),511)
    if np.any(lo>hi):continue
    mat=np.vstack((d.T,np.ones(3))); det=np.linalg.det(mat)
    if abs(det)<1e-8:continue
    yy,xx=np.mgrid[lo[1]:hi[1]+1,lo[0]:hi[0]+1]
    w=np.linalg.inv(mat)@np.stack((xx.ravel()+.5,yy.ravel()+.5,np.ones(xx.size)))
    mask=(w>=-1e-6).all(axis=0).reshape(xx.shape)
    xy=(s.T@w).reshape(2,*xx.shape); ix=np.clip(xy[0].astype(int),0,photo.shape[1]-1); iy=np.clip(xy[1].astype(int),0,photo.shape[0]-1)
    part=out[lo[1]:hi[1]+1,lo[0]:hi[0]+1]; part[mask]=photo[iy,ix][mask]
    cover[lo[1]:hi[1]+1,lo[0]:hi[0]+1][mask]=255
base_image=np.asarray(Image.open(baseline_path).convert('RGB')).astype(float)
mask=cover>0
reference=out.astype(float)
for channel in range(3):
    before=reference[:,:,channel]; after=base_image[:,:,channel]
    mean=before[mask].mean(); std=before[mask].std()
    reference[:,:,channel]=(before-mean)*(after[mask].std()/max(std,1))+after[mask].mean()
out[mask]=np.clip(reference[mask],0,255).astype('uint8')
Image.fromarray(out).save(here/'landmark-target.png')
Image.fromarray(cover).save(here/'landmark-coverage.png')
print('landmarks',len(pts),'triangles',len(triangles),'covered',int((cover>0).sum()))
