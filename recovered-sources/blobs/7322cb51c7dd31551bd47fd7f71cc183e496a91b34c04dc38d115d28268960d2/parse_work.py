from pathlib import Path
import struct,json
HERE=Path(__file__).resolve().parent
class Reader:
    def __init__(self,path):self.file=path.open('rb')
    def raw(self,n):
        x=self.file.read(n);assert len(x)==n;return x
    def pod(self,fmt):return struct.unpack('<'+fmt,self.raw(struct.calcsize('<'+fmt)))[0]
    def string(self):return self.raw(self.pod('Q')).decode()
    def vec(self):return list(struct.unpack('<3f',self.raw(12)))
    def vectors(self):return [self.vec() for _ in range(self.pod('Q'))]
    def plane(self):
        p={'normal':self.vec(),'distance':self.pod('f'),'charred':self.pod('?'),'grain':self.vec(),'roughness':self.pod('f'),'spacing':self.pod('f'),'phase':self.pod('f')}
        p['beams']=[dict(zip(['start_x','start_y','start_z','direction_x','direction_y','direction_z','tangent_x','tangent_y','tangent_z','radius','depth'],struct.unpack('<11f',self.raw(44)))) for _ in range(self.pod('Q'))];return p
    def work(self):
        assert self.raw(8)==b'CWORK001'
        d={'reason':self.string(),'stamp':self.pod('Q'),'epoch':self.pod('Q'),'model':self.pod('Q'),'model_name':self.string(),'valid':self.pod('?')}
        d['request']={'plane':self.plane(),'owner':self.pod('i'),'attacker':self.pod('Q'),'pay_partial':self.pod('?'),'predecessors':self.pod('Q'),'impulse':self.vec(),'impact_radius':self.pod('f')}
        if self.pod('?'):d['request']['impact']={'point':self.vec(),'normal':self.vec(),'energy':self.pod('f'),'age':self.pod('f'),'other':self.pod('i')}
        d['original_contour']=self.vectors();d['contour']=self.vectors();d['original_keel']=self.vectors();d['original_area']=self.pod('f');d['targets']=[]
        for _ in range(self.pod('Q')):d['targets'].append({'node':self.pod('i'),'name':self.string(),'local_axes_position':[self.vec() for _ in range(4)],'generation':self.pod('Q'),'revision':self.pod('Q'),'fragment_limit':self.pod('I'),'projection_changed':self.pod('?'),'projection_fragment_count_after_staging_move':self.pod('Q')})
        assert not self.file.read(1);return d
for name in ['first-accepted','first-rejected']:
    d=Reader(HERE/'capture-ship'/(name+'.work.bin')).work();(HERE/(name+'.work.json')).write_text(json.dumps(d,indent=2)+'\n')
    print(name,'stamp',d['stamp'],'predecessors',d['request']['predecessors'],'root rev',d['targets'][0]['revision'],'root local',d['targets'][0]['local_axes_position'],'beams',d['request']['plane']['beams'])
