"""Clip crossing sculpture triangles instead of dropping them at the neck."""
from pathlib import Path
import sys
import os
import numpy as np

here=Path(__file__).resolve().parent

def clip_mesh(triangles, plane):
    heights=triangles[:,:,2]
    inside=heights>=plane
    complete=triangles[np.all(inside,axis=1)]
    crossing=triangles[np.any(inside,axis=1)&~np.all(inside,axis=1)]
    cache={}; clipped=[]
    def intersect(a,b):
        ka,kb=tuple(map(float,a)),tuple(map(float,b))
        key=tuple(sorted((ka,kb)))
        if key not in cache:
            p,q=map(lambda v:np.asarray(v,dtype=float),key)
            v=p+(q-p)*((plane-p[2])/(q[2]-p[2]));v[2]=plane
            cache[key]=v.astype('float32')
        return cache[key]
    for triangle in crossing:
        polygon=[]
        previous=triangle[-1]; prev_in=previous[2]>=plane
        for current in triangle:
            cur_in=current[2]>=plane
            if cur_in!=prev_in:polygon.append(intersect(previous,current))
            if cur_in:polygon.append(current)
            previous,prev_in=current,cur_in
        for j in range(1,len(polygon)-1):clipped.append((polygon[0],polygon[j],polygon[j+1]))
    out=np.concatenate((complete,np.asarray(clipped,dtype='float32')))
    print('crossing triangles',len(crossing),'clipped triangles',len(clipped))
    return out

path=here/'neck_graft_trial.py'
source=path.read_text()
needle="source=path.read_text()"
insert=needle+'''\nold='source=source[np.all(source[:,:,2]>=83.,axis=1)]'\nassert source.count(old)==1\nsource=source.replace(old,'source=clip_mesh(source,83.)')'''
connected=len(sys.argv)>1 and sys.argv[1]=='connected'
edge=len(sys.argv)>1 and sys.argv[1]=='edge'
low=len(sys.argv)>1 and sys.argv[1] in ('low','low-noskirt','low-scarf','low-overlap','low-clip','low-fit','low-seam','seam-high','seam-inset')
noskirt=len(sys.argv)>1 and sys.argv[1] in ('low-noskirt','low-scarf','low-overlap','low-clip','low-fit','low-seam','seam-high','seam-inset')
scarf=len(sys.argv)>1 and sys.argv[1]=='low-scarf'
overlap=len(sys.argv)>1 and sys.argv[1]=='low-overlap'
bodyclip=len(sys.argv)>1 and sys.argv[1] in ('low-clip','low-fit','low-seam','seam-high','seam-inset')
skinfit=len(sys.argv)>1 and sys.argv[1] in ('low-fit','low-seam','seam-high','seam-inset')
seam=len(sys.argv)>1 and sys.argv[1] in ('low-seam','seam-high','seam-inset')
seam_high=len(sys.argv)>1 and sys.argv[1] in ('seam-high','seam-inset')
inset=len(sys.argv)>1 and sys.argv[1]=='seam-inset'
if bodyclip:os.environ['PUTIN_TRIAL_BODY_CLIP']='1'
if skinfit:os.environ['PUTIN_TRIAL_SKIN_FIT']='1'
if seam:os.environ['PUTIN_TRIAL_BODY_PLANE']='1.595'
if connected:
    insert+="\nsource=source.replace('bobnojio-head-reduced-1.5.STL','bobnojio-head-connected-1.5.STL')"
if edge:
    insert+="\nsource=source.replace('bobnojio-head-reduced-1.5.STL','bobnojio-head-edge-collapse.STL')"
if low:
    insert+="\nsource=source.replace('bobnojio-head-reduced-1.5.STL','bobnojio-head-edge-collapse-68.STL')"
    insert+="\nsource=source.replace('source=clip_mesh(source,83.)','source=clip_mesh(source,76.78571428571429)')" if seam_high else "\nsource=source.replace('source=clip_mesh(source,83.)','source=clip_mesh(source,75.)')"
    insert+="\nsource=source.replace('centers[:,1]>1.6174','centers[:,1]>1.625')" if overlap else "\nsource=source.replace('centers[:,1]>1.6174','centers[:,1]>1.595')"
    insert+="\nsource=source.replace('(1.813-1.6174)','(1.813-1.595)')"
if scarf:
    insert+='''\nneedle='kept=faces[~remove]'\nreplacement="""uv=np.array([struct.unpack_from('<2f',data,start+i*44+36) for i in range(n)])\nuv=(uv[faces].mean(axis=1)%1)\ntex=np.asarray(Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024)),dtype=int)\ncolors=tex[np.clip((uv[:,1]*1024).astype(int),0,1023),np.clip((uv[:,0]*1024).astype(int),0,1023)]\ncloth=(centers[:,1]<1.65)&(colors[:,2]>colors[:,0]*1.1)&(colors.max(axis=1)<120)\nprint('restored scarf faces',int((remove&cloth).sum()))\nremove&=~cloth\nkept=faces[~remove]"""\nassert source.count(needle)==1\nsource=source.replace(needle,replacement)'''
if inset:
    insert+='''\nneedle="pos=np.column_stack((.010+points[:,0]*.0025,1.385+points[:,2]*.0028,.06-points[:,1]*.0025))"\nreplacement=needle+"\\nt=np.clip((1.635-pos[:,1])/.035,0,1); t=t*t*(3-2*t); pos[:,2]-=.04*t"\nassert source.count(needle)==1\nsource=source.replace(needle,replacement)'''
assert source.count(needle)==1
source=source.replace(needle,insert)
if low:
    source=source.replace('pp[boundary,1]<1.627','pp[boundary,1]<1.600')
    source=source.replace('bx,1.597,bz','bx,1.575,bz')
if noskirt:
    source=source.replace('outfaces=np.vstack((outfaces,np.asarray(skirt,dtype=np.int32)))','outfaces=outfaces')
prefix='connected-neck' if connected else 'edge-neck' if edge else 'seam-inset' if inset else 'seam-high' if seam_high else 'low-seam' if seam else 'low-fit' if skinfit else 'low-clip' if bodyclip else 'low-overlap' if overlap else 'low-scarf' if scarf else 'low-noskirt' if noskirt else 'low-neck' if low else 'exact-neck'
source=source.replace("'neck-graft-atlas.png'",repr(prefix+'-graft-atlas.png'))
source=source.replace("'neck-graft-prototype.gm'",repr(prefix+'-graft-prototype.gm'))
old="{'__file__':str(path),'__name__':'__main__'}"
assert source.count(old)==1
source=source.replace(old,"{'__file__':str(path),'__name__':'__main__','clip_mesh':clip_mesh}")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__','clip_mesh':clip_mesh})
