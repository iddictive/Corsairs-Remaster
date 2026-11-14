"""Fit two skin influences against sampled real man animation frames."""
import hashlib
import itertools
import json
import sys
from pathlib import Path
import numpy as np

root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'tools'))
from prepare_roundhouse import bind_skin,skin_coordinates
from prepare_port_rowboat import rig,fk

class SkinFit:
    def __init__(self):
        self.source=json.loads((root/'src/assets/roundhouse/rigs.json').read_text())['rigs']['man']
        data=(root/self.source['source_path']).read_bytes()
        assert hashlib.sha256(data).hexdigest()==self.source['source_sha256']
        animation=rig(data)
        self.train=np.linspace(0,animation['frames']-1,32,dtype=int)
        # Interleave disjoint frames so this reports error outside the fit set.
        self.test=np.rint((self.train[:-1]+self.train[1:])/2).astype(int)
        self.poses=[fk(animation,animation['pose'](int(i)),animation['roots'][int(i)]) for i in np.r_[self.train,self.test]]
        self.metrics=[]
    def fit(self,a,b,t,pos,default):
        ids=sorted({a[4]&255,(a[4]>>8)&255,b[4]&255,(b[4]>>8)&255})
        vertices=[(tuple(pos),1.,i,i) for i in ids]
        vertices += [(tuple(r[:3]),r[3],r[4]&255,(r[4]>>8)&255) for r in (a,b)]
        bound=bind_skin(self.source,vertices)
        coords=np.array([skin_coordinates(p,q,bound) for p,q in self.poses])
        target=coords[:,-2]*(1-t)+coords[:,-1]*t
        n=len(self.train); pairs=list(itertools.combinations(ids,2))
        if not pairs:pairs=[(ids[0],ids[0])]
        def evaluate(pair,weight):
            x,y=(coords[:,ids.index(i)] for i in pair)
            predicted=x*weight+y*(1-weight)
            return np.linalg.norm(predicted-target,axis=1)
        best=None
        for pair in pairs:
            x,y=(coords[:n,ids.index(i)] for i in pair)
            d=x-y;den=np.sum(d*d)
            w=float(np.clip(np.sum(d*(target[:n]-y))/den,0,1)) if den>1e-14 else 1.
            errors=evaluate(pair,w);score=float(np.mean(errors[:n]**2))
            if best is None or score<best[0]:best=(score,pair,w,errors)
        base=evaluate(default[0],default[1]);self.metrics.append((base,best[3]))
        return best[1],best[2]
    def report(self):
        if not self.metrics:return
        a=np.concatenate([v[0][len(self.train):] for v in self.metrics]);b=np.concatenate([v[1][len(self.train):] for v in self.metrics])
        print('fit vertices',len(self.metrics),'train/test frames',len(self.train),len(self.test),
              'held-out mean default/fitted',round(float(a.mean()),7),round(float(b.mean()),7),
              'held-out max default/fitted',round(float(a.max()),7),round(float(b.max()),7))
