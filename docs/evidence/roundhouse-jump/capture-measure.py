import sys,json,pathlib,math,struct,importlib.util,hashlib,os
sys.path.insert(0,'tools');import prepare_roundhouse as m
from PIL import Image
out=pathlib.Path(os.environ['CODEX_SCRATCH_DIR']); c=json.load(open(m.RECIPE));d=json.load(open(m.SOURCES));man=json.load(open(m.MANIFEST));spec=importlib.util.spec_from_file_location('render','docs/evidence/roundhouse-kick/render.py');render=importlib.util.module_from_spec(spec);spec.loader.exec_module(render)
records=json.loads((render.INPUTS/'meshes.json').read_text()); report={}; phases=list(range(0,121,10));
for name in ('man','man1','man2','Jessika','YokoDias','Danielle'):
 s=d['rigs'][name];r=m.source_rig(s);j=m.jump_kick(s,c);k,offset,_=m.kick(s,c);b=s['body']['joints']; bound=m.bind_skin(s,s['support_mesh']['vertices']); bindp,bindg=m.fk(r,s['bind_pose'],s['bind_root']); offsets=[m.rotate(m.qi(bindg[foot]),tuple(c['heel_bind_offset'])) for foot in (b[11],b[14])]; old=m.rig((m.OUTPUT/f'RESOURCE/animation/Roundhouse_{name}.an').read_bytes());ranges=man['banks'][name]['actions'];act={'roundhouse_kick':k,'roundhouse_jump':j}
 for action in ('getup_front','getup_back'):
  a,z=ranges[action];act[action]=[(old['roots'][i],old['pose'](i)) for i in range(a,z+1)]
 data,rr=m.pack(s,act,c['fps']);an=m.rig(data);a,z=rr['roundhouse_kick'];old_a,_=ranges['roundhouse_kick'];assert all(struct.pack('<3f',*an['roots'][i])==struct.pack('<3f',*old['roots'][old_a+i-a]) and all(struct.pack('<4f',*v)==struct.pack('<4f',*w) for v,w in zip(an['pose'](i),old['pose'](old_a+i-a))) for i in range(a,z+1));(out/'RESOURCE/animation'/f'Roundhouse_{name}.an').write_bytes(data)
 rows=[]
 for root,q in j:
  p,g=m.fk(r,q,root)
  def world(v):return [-v[0],v[1],v[2]]
  heels=[world(m.add(p[foot],m.rotate(g[foot],off))) for foot,off in zip((b[11],b[14]),offsets)]
  rows.append({'root':root,'native_heel':heels,'native_knee':[world(p[b[10]]),world(p[b[13]])],'floor':min(m.skin_coordinates(p,g,bound,y_only=True)),'pose':q})
 ys=[v['root'][1] for v in rows]; vs=[ys[i+1]-ys[i] for i in range(120)]; accel=[vs[i+1]-vs[i] for i in range(119)]; report[name]={'frames':rows,'max_root_y_delta_per_frame':max(map(abs,vs)),'max_root_y_second_delta':max(map(abs,accel)),'max_root_step':max(math.dist(j[i][0],j[i+1][0]) for i in range(120)),'min_mesh_floor':min(v['floor'] for v in rows),'guard_floor':rows[0]['floor'],'end_root_offset':m.sub(j[-1][0],j[0][0]),'end_pose_guard':all(abs(abs(m.dot(q,p))-1)<1e-6 for q,p in zip(j[-1][1],s['guard_pose'])),'k_exact_serialized_frames':121}
 verts,faces=render.mesh((render.INPUTS/(records[name]['model']+'.gm')).read_bytes()); assert hashlib.sha256((render.INPUTS/(records[name]['model']+'.gm')).read_bytes()).hexdigest()==records[name]['sha256']; boundmesh=m.bind_skin(s,verts);sheet=Image.new('RGB',(13*260,2*310));first,_=rr['roundhouse_jump']
 for col,frame in enumerate(phases):
  p,g=m.fk(an,an['pose'](first+frame),an['roots'][first+frame]);points=m.skin_coordinates(p,g,boundmesh);cx=(min(v[0] for v in points)+max(v[0] for v in points))/2;cz=(min(v[2] for v in points)+max(v[2] for v in points))/2;points=[(x-cx,y,z-cz) for x,y,z in points]
  for row,view in enumerate((-55,65)):render.panel(sheet,(col*260,row*310,260,310),points,faces,f'{name} frame {frame} view {view}',azimuth=view,view_height=3.6)
 sheet.save(out/f'{name}-full.png');print(name,'floor',round(report[name]['min_mesh_floor'],4),'guard',round(report[name]['guard_floor'],4),'max_dy',round(report[name]['max_root_y_delta_per_frame'],4),'max_ddY',round(report[name]['max_root_y_second_delta'],4),'rootend',tuple(round(x,3) for x in report[name]['end_root_offset']),flush=True)
(out/'trajectory.json').write_text(json.dumps(report))
