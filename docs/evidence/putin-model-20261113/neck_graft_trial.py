"""Add a diagnostic neck skirt to the multiview head graft."""
import sys
from pathlib import Path

here=Path(__file__).resolve().parent
path=here/'multiview_graft_trial.py'
source=path.read_text()
needle="newfaces=np.vstack((kept,outfaces+n)).astype('<u2')"
insertion='''# Build faces along the low open boundary; this is not a stitched skinning seam.
edges=np.vstack((outfaces[:,[0,1]],outfaces[:,[1,2]],outfaces[:,[2,0]]))
canonical=np.sort(edges,axis=1)
_,first,counts=np.unique(canonical,axis=0,return_index=True,return_counts=True)
boundary=edges[first[counts==1]]
pp=np.array([struct.unpack('<3ffI3fI2f',row)[:3] for row in packed])
boundary=boundary[np.all(pp[boundary,1]<1.627,axis=1)]
bottom={}; skirt=[]
for a,b in boundary:
    for vertex in (int(a),int(b)):
        if vertex not in bottom:
            original=struct.unpack('<3ffI3fI2f',packed[vertex])
            x,y,z=original[:3]; uu=original[9]
            bx=.010+(x-.010)*.82; bz=.06+(z-.06)*.80
            bottom[vertex]=len(packed)
            packed.append(struct.pack('<3ffI3fI2f',bx,1.597,bz,1.,16,*original[5:8],0xffffffff,uu,.999))
    skirt.extend(((int(b),int(a),bottom[int(a)]),(int(b),bottom[int(a)],bottom[int(b)])))
outfaces=np.vstack((outfaces,np.asarray(skirt,dtype=np.int32)))
print('low boundary edges',len(boundary),'skirt triangles',len(skirt))
'''+needle
assert source.count(needle)==1
source=source.replace(needle,insertion)
for old,new in [('multiview-graft-atlas.png','neck-graft-atlas.png'),('multiview-graft-prototype.gm','neck-graft-prototype.gm')]:
    assert source.count(old)==1
    source=source.replace(old,new)
sys.argv=[str(path)]
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
