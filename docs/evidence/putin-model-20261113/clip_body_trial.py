"""Clip original body triangles at the head join, preserving UV and skin data."""
import struct
import os
import numpy as np

def clip_body(rows,faces,points,plane):
    result=[]; cache={}; removed=0; crossing=0; max_discarded=0.
    fitter=None
    if os.environ.get('PUTIN_TRIAL_SKIN_FIT')=='1':
        from fit_skin_trial import SkinFit
        fitter=SkinFit()
    def intersect(ia,ib):
        nonlocal max_discarded
        key=tuple(sorted((int(ia),int(ib))))
        if key in cache:return cache[key]
        a=struct.unpack('<3ffI3fI2f',rows[key[0]])
        b=struct.unpack('<3ffI3fI2f',rows[key[1]])
        t=(plane-a[1])/(b[1]-a[1]); assert -1e-5<=t<=1.00001
        lerp=lambda x,y:(1-t)*x+t*y
        pos=np.array([lerp(a[i],b[i]) for i in range(3)]);pos[1]=plane
        normal=np.array([lerp(a[i],b[i]) for i in range(5,8)])
        normal/=max(np.linalg.norm(normal),1e-12)
        bones={}
        for row,mix in ((a,1-t),(b,t)):
            packed=row[4];w=row[3]
            for bone,weight in ((packed&255,w),((packed>>8)&255,1-w)):
                bones[bone]=bones.get(bone,0.)+mix*weight
        influences=sorted(bones.items(),key=lambda p:-p[1]);keep=influences[:2]
        max_discarded=max(max_discarded,sum(x[1] for x in influences[2:]))
        if len(keep)==1:keep.append((keep[0][0],0.))
        total=keep[0][1]+keep[1][1];weight=keep[0][1]/max(total,1e-12)
        if fitter is not None:
            pair,weight=fitter.fit(a,b,t,pos,((keep[0][0],keep[1][0]),weight))
            keep=[(pair[0],weight),(pair[1],1-weight)]
        packed=keep[0][0]|(keep[1][0]<<8)
        uv=[lerp(a[i],b[i]) for i in (9,10)]
        row=struct.pack('<3ffI3fI2f',*pos,weight,packed,*normal,a[8],*uv)
        cache[key]=len(rows);rows.append(row)
        return cache[key]
    for face in faces:
        xyz=points[face];center=xyz.mean(axis=0)
        local=abs(center[0]-.01)<.14 and -.12<center[2]<.19
        if not local or np.all(xyz[:,1]<=plane):
            result.append(tuple(map(int,face)));continue
        if np.all(xyz[:,1]>plane):
            removed+=1;continue
        crossing+=1;polygon=[];prev=int(face[-1]);prev_in=points[prev,1]<=plane
        for item in face:
            current=int(item);cur_in=points[current,1]<=plane
            if cur_in!=prev_in:polygon.append(intersect(prev,current))
            if cur_in:polygon.append(current)
            prev,prev_in=current,cur_in
        for j in range(1,len(polygon)-1):result.append((polygon[0],polygon[j],polygon[j+1]))
    print('body removed',removed,'crossing',crossing,'new boundary vertices',len(cache),'max discarded skin weight',round(max_discarded,5))
    if fitter is not None:fitter.report()
    return rows,np.asarray(result,dtype='<u2')
