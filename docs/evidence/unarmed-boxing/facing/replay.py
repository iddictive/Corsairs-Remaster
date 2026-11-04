from pathlib import Path
import hashlib, json, math, subprocess, sys, types, time
ROOT = Path(__file__).resolve().parents[4]
OUT = Path(sys.argv[1])
assert OUT.is_dir()
sys.path.insert(0, str(ROOT/'tools'))
import boxing_mocap as candidate
import prepare_unarmed as bank_author
from prepare_roundhouse import SOURCES, source_rig, animated_mesh, bind_skin, skin_coordinates, pack
from prepare_port_rowboat import fk, rotate, qi, rig
from render_unarmed_atlas import panel, INPUTS
from PIL import Image

baseline = types.ModuleType('baseline')
baseline.__file__ = str(ROOT/'tools/boxing_mocap.py')
old_source = subprocess.check_output(['git','show','b09b887374411b2fc99b30d49aa774519848f092:tools/boxing_mocap.py'], cwd=ROOT).decode()
exec(compile(old_source, baseline.__file__, 'exec'), baseline.__dict__)
sources = json.loads(SOURCES.read_text())['rigs']
recipe = json.loads(candidate.RECIPE.read_text())
mesh_rows = json.loads((INPUTS/'meshes.json').read_text())
report = {'baseline_commit':subprocess.check_output(['git','rev-parse','b09b887374411b2fc99b30d49aa774519848f092'],cwd=ROOT).decode().strip(),
          'conditions':'Offline actual GM skin and native AN round-trip; no gameplay',
          'source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest()
           for p in [SOURCES, ROOT/'tools/boxing_mocap.py', candidate.RECIPE]}, 'rigs':{}}
overview = Image.new('RGB',(2160,1200))
closeup = Image.new('RGB',(1920,840))
for name,s in sources.items():
    r=source_rig(s); body=s['body']['joints']
    current=candidate.Retargeter(s,candidate.load_capture(recipe['actions']['unarmed_cross']))
    chest,neck,head=current.gaze_joints
    _,bind_world=fk(r,s['bind_pose'],s['bind_root'])
    def headings(pose):
        _,world=fk(r,pose[1],pose[0]); result={}
        for label,j in [('chest',chest),('neck',neck),('head',head)]:
            f=rotate(world[j],rotate(qi(bind_world[j]),(0.,0.,1.)))
            result[label]=dict(yaw_degrees=math.degrees(math.atan2(-f[0],f[2])),forward_y=f[1])
        return result
    before_guard=baseline.make_guard(s); after_guard=candidate.make_guard(s)
    before=baseline.make_strikes(s,guard=before_guard)
    after=candidate.make_strikes(s,guard=after_guard)
    entry={'gaze_joints':[chest,neck,head], 'guard_before':headings(before_guard),
           'guard_after':headings(after_guard), 'max_face_yaw_degrees':0.,
           'max_adjacent_yaw_degrees':0., 'max_root_error_m':0.,
           'max_wrist_error_m':0., 'max_non_gaze_local_quat_error':0.,
           'max_head_tilt_error':0., 'actions':{}}
    for action,new_frames in after.items():
        old_frames=before[action]; count=len(new_frames)
        assert new_frames[0]==new_frames[-1]==after_guard
        for old,new in zip(old_frames,new_frames):
            oldp,oldg=fk(r,old[1],old[0]);newp,newg=fk(r,new[1],new[0])
            h=headings(new);oldh=headings(old)
            face=abs(h['head']['yaw_degrees']); adjacent=max(abs(h['chest']['yaw_degrees']-h['neck']['yaw_degrees']),abs(h['neck']['yaw_degrees']-h['head']['yaw_degrees']))
            entry['max_face_yaw_degrees']=max(entry['max_face_yaw_degrees'],face)
            entry['max_adjacent_yaw_degrees']=max(entry['max_adjacent_yaw_degrees'],adjacent)
            entry['max_root_error_m']=max(entry['max_root_error_m'],math.dist(old[0],new[0]))
            entry['max_wrist_error_m']=max(entry['max_wrist_error_m'],*[math.dist(oldp[j],newp[j]) for j in (body[5],body[8])])
            entry['max_non_gaze_local_quat_error']=max(entry['max_non_gaze_local_quat_error'],*[abs(a-b) for j,(oldq,newq) in enumerate(zip(old[1],new[1])) if j not in (neck,head) for a,b in zip(oldq,newq)])
            entry['max_head_tilt_error']=max(entry['max_head_tilt_error'],abs(h['head']['forward_y']-oldh['head']['forward_y']))
            assert face<1e-10 and entry['max_root_error_m']<1e-10 and entry['max_wrist_error_m']<1e-10 and entry['max_non_gaze_local_quat_error']<1e-12, (name,action,entry)
            assert all(math.isfinite(x) for q in new[1] for x in q)
        contact=recipe['actions'][action]['contact_output_frame']
        entry['actions'][action]={'frames':count,'contact_before':headings(old_frames[contact]),'contact_after':headings(new_frames[contact])}
    if name in ('man','Jessika'):
        # Verify the actual bank author shares every boundary and keeps fingers.
        authored=bank_author.make_unarmed_actions(s)
        old_guard_fn,old_strike_fn=bank_author.make_guard,bank_author.make_strikes
        bank_author.make_guard,bank_author.make_strikes=baseline.make_guard,baseline.make_strikes
        try: old_authored=bank_author.make_unarmed_actions(s)
        finally: bank_author.make_guard,bank_author.make_strikes=old_guard_fn,old_strike_fn
        entry['authored_max_face_yaw_by_action']={a:max(abs(headings(p)['head']['yaw_degrees']) for p in f) for a,f in authored.items()}
        for frames in authored.values(): assert frames[0]==frames[-1]==authored['unarmed_idle'][0]
        packed,ranges=pack(s,authored,30.)
        native=rig(packed);first=ranges['unarmed_idle'][0]
        np,ng=fk(native,native['pose'](first),native['roots'][first])
        face=rotate(ng[head],rotate(qi(bind_world[head]),(0.,0.,1.)))
        entry['native_an_face_yaw_degrees']=math.degrees(math.atan2(-face[0],face[2]))
        mesh=(INPUTS/(mesh_rows[name]['model']+'.gm')).read_bytes()
        assert hashlib.sha256(mesh).hexdigest()==mesh_rows[name]['sha256']
        vertices,faces=animated_mesh(mesh);bound=bind_skin(s,vertices)
        row=('man','Jessika').index(name)
        for state,pose in enumerate((old_authored['unarmed_idle'][0],authored['unarmed_idle'][0])):
            p,g=fk(r,pose[1],pose[0]);points=skin_coordinates(p,g,bound)
            for col,angle in enumerate((0.,-55.,90.)):
                label=f'{name} '+('before' if state==0 else 'candidate')+f' {angle:g} deg'
                x=state*3+col
                overview.paste(panel(points,faces,angle,(360,600),label),(360*x,600*row))
                closeup.paste(panel(points,faces,angle,(320,420),label,framing=(0.,.92,480.)),(320*x,420*row))
    report['rigs'][name]=entry
report['summary']={key:max(r[key] for r in report['rigs'].values()) for key in (
    'max_face_yaw_degrees','max_adjacent_yaw_degrees','max_root_error_m','max_wrist_error_m',
    'max_non_gaze_local_quat_error','max_head_tilt_error')}
overview.save(OUT/'guard-full-body.png');closeup.save(OUT/'guard-upper-body.png')
(OUT/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report['summary'],indent=2))
print(OUT)
