#!/usr/bin/env python3
"""One-off source-bound CPU evidence; no game, GPU, build-cache or test-suite writes."""
import os,pathlib,subprocess,hashlib
root=pathlib.Path.cwd();work=pathlib.Path(os.environ['CODEX_SCRATCH_DIR'])
import sys,shlex,time
if sys.argv[1:]==['--syntax']:
 ninja=(root/'experiments/native-metal/.cache/build/build.ninja').read_text().splitlines()
 base=root/'experiments/native-metal/engine/src/libs'
 env=dict(os.environ,CLANG_MODULE_CACHE_PATH=str(work/'module-cache'))
 for name in ['ai_cannon','ai_balls']:
  start=next(i for i,line in enumerate(ninja) if line.startswith('build native-engine/storm/libs/sea_ai/') and f'/{name}.cpp.o:' in line)
  values={}
  for line in ninja[start+1:]:
   if not line.startswith('  '):break
   k,_,v=line.strip().partition(' = ');values[k]=v
  argv=['/usr/bin/c++']+shlex.split(values['DEFINES'])+shlex.split(values['INCLUDES'])+[a for a in shlex.split(values['FLAGS']) if a!='-v']+['-fsyntax-only',str(base/f'sea_ai/src/{name}.cpp')]
  print('compiler_argv',shlex.join(argv).replace(str(root),'<repo>').replace(str(work),'<scratch>'),flush=True)
  start=time.monotonic();p=subprocess.run(argv,env=env,text=True,capture_output=True)
  print(name,'native_translation_unit_syntax',p.returncode,'seconds',round(time.monotonic()-start,3),flush=True)
  if p.returncode:print(p.stderr);raise SystemExit(p.returncode)
 argv=['/usr/bin/c++','-std=c++20','-O2','-I'+str(base/'shared_headers/include'),'-I'+str(base/'math/include'),str(root/'experiments/native-metal/cannon_aim_geometry_probe.cpp'),'-o',str(work/'aim-probe')]
 print('existing_probe_compiler_argv',shlex.join(argv).replace(str(root),'<repo>').replace(str(work),'<scratch>'),flush=True)
 p=subprocess.run(argv,env=env,text=True,capture_output=True)
 if p.returncode:print(p.stderr);raise SystemExit(p.returncode)
 p=subprocess.run([str(work/'aim-probe')],env=env,text=True,capture_output=True)
 print('existing_aim_geometry_probe',p.returncode,p.stdout.strip(),p.stderr.strip(),flush=True)
 raise SystemExit(p.returncode)

path='experiments/native-metal/engine/src/libs/sea_ai/src/ai_cannon.cpp'
old=subprocess.check_output(['git','show','8e635ad50f95ead29c5212ab5c4ad4790b3e30f6:'+path],text=True);new=(root/path).read_text()
def method(s,key):
 start=s.index(key);p=s.index('{',start);depth=1;end=p+1
 while depth:depth+=(s[end]=='{')-(s[end]=='}');end+=1
 return s[start:end]
shared=r'''
#include <cmath>
#include <algorithm>
#include <cstdio>
#include <limits>
#include "c_vector.h"
constexpr float PI=3.14159265358979323846f;
#define Assert(x) ((void)(x))
struct CMatrix { CVECTOR p{0.f}; CVECTOR operator*(CVECTOR v)const{return v+p;} void Get3X3(CMatrix &r){r=CMatrix{};} };
struct Attr { bool fixed=true; float height=.5773502691896258f; void SetAttributeUseFloat(const char*,float){} void SetAttributeUseDword(const char*,int){} };
struct Holder {Attr a;Attr *GetACharacter(){return &a;}};
struct Obj {CMatrix m;CMatrix *GetMatrix(){return &m;}void Fire(CVECTOR){}};
struct AIHelper {static constexpr float fGravity=9.81f;};
struct AIBalls {static inline AIBalls *pAIBalls=nullptr;Attr *AttributesPointer;};
struct Data {float GetFloat(){return .5f;}};
struct Core {float shot[9]{};Data d;Data*Event(int,const char*,Attr*){return &d;}void Event(int,const char*,Attr*,float a,float b,float c,float d,float e,float f,float g,float h,float i){float v[]={a,b,c,d,e,f,g,h,i};std::copy(v,v+9,shot);}} core;
constexpr int CANNON_FIRE=1,CANNON_GET_FIRE_TIME=2;
#define SQR(x) ((x)*(x))
float Clamp(float x){return std::clamp(x,0.f,1.f);}float NormalizeAngle(float x){return x;}
struct AICannon {Holder holder;Holder *pAHolder=&holder;Obj obj;CVECTOR vPos{0.f},vDir{0.f,0.f,1.f},vEnemyPos{0.f},dartCameraTarget{0.f};bool dartCameraTargetValid=false,bFired=false,bReady2Fire=true,bEmpty=false;int targetCharacter=-1;float fSpeedV0=140.f,fMaxFireDistance=2000.f,fTotalTime2Action=0,fTime2Action=0;enum{CANNONTYPE_NORMAL,CANNONTYPE_MORTAR};int CannonType=CANNONTYPE_NORMAL;bool UsesFixedBallisticFrame(Attr*a)const{return a->fixed;}float GetBallisticHeightScale(Attr*a)const{return a->height;}Obj *GetAIObjPointer(){return &obj;}CVECTOR GetPos(){return obj.m*vPos;}void Recharge(){}bool CanFireTo(CVECTOR){return true;}float CalcHeightFireAngle(float,const CVECTOR&,const CVECTOR&)const;bool Fire(float,const CVECTOR&,float,int,float);void RealFire();};
'''
main=r'''
int main(){
 for(int mode=0;mode<4;++mode)for(float delay:{.05f,.5f,3.5f,10.f})for(float enemySpeed:{-6.f,0.f,6.f}){
  AICannon gun;gun.obj.m.p=CVECTOR(0.f,5.f,0.f);gun.holder.a.fixed=mode!=1;
  const float raw=mode==2?1.2f:std::numeric_limits<float>::quiet_NaN();
  const CVECTOR point(1000.f,5.f,enemySpeed*(1000.f/280.f+delay));
  gun.Fire(140.f,point,delay,-1,raw);
  if(mode==3)gun.dartCameraTargetValid=false;
  gun.obj.m.p.z+=8.f*delay;gun.obj.m.p.y+=.3f*delay;
  gun.RealFire();const auto*s=core.shot;
  const float range=std::hypot(point.x-s[0],point.z-s[2]);
  const float t=range/(s[3]*std::cos(s[5]));
  const CVECTOR hit(s[0]+s[3]*t*std::cos(s[5])*std::sin(s[4]),s[1]+gun.holder.a.height*(s[3]*t*std::sin(s[5])-9.81f*t*t/2),s[2]+s[3]*t*std::cos(s[5])*std::cos(s[4]));
  std::printf("%d %.2f %.1f %.9g %.9g %.9g %.9g\n",mode,delay,enemySpeed,(hit-point).GetLength(),s[4],s[5],s[8]);
 }
}
'''
rows={}
for label,source in [('baseline',old),('candidate',new)]:
 cpp=shared+'\n'.join(method(source,k) for k in ['float AICannon::CalcHeightFireAngle','void AICannon::RealFire','bool AICannon::Fire('])+main
 file=work/(label+'.cpp');file.write_text(cpp);exe=work/label
 p=subprocess.run(['/usr/bin/c++','-std=c++20','-O2','-I'+str(root/'experiments/native-metal/engine/src/libs/math/include'),str(file),'-o',str(exe)],text=True,capture_output=True)
 if p.returncode:print(p.stderr);raise SystemExit(p.returncode)
 output=subprocess.check_output([str(exe)],text=True);rows[label]=[line.split() for line in output.splitlines()]
 print(label+'_raw_begin');print(output,end='');print(label+'_raw_end')
 print(label,'source_sha256',hashlib.sha256(source.encode()).hexdigest(),'native_actual_method_cases',len(rows[label]))
for label,data in rows.items():print(label,'max_point_endpoint_miss_m',max(float(r[3]) for r in data if r[0]=='0'))
for mode,name in [(1,'ordinary_event_yaw_pitch_raw_unchanged'),(2,'manual_sky_vector_unchanged'),(3,'loaded_captured_vector_fallback_unchanged')]:
 a=[r[4:] for r in rows['baseline'] if int(r[0])==mode];b=[r[4:] for r in rows['candidate'] if int(r[0])==mode]
 print(name,a==b,'cases',len(a));assert a==b
assert max(float(r[3]) for r in rows['candidate'] if r[0]=='0')<.01
for key in ['void AICannon::Save','void AICannon::Load']:
 print(key,'byte_identical',method(old,key)==method(new,key));assert method(old,key)==method(new,key)
balls='experiments/native-metal/engine/src/libs/sea_ai/src/ai_balls.cpp';prior=subprocess.check_output(['git','show','8e635ad50f95ead29c5212ab5c4ad4790b3e30f6:'+balls],text=True);current=(root/balls).read_text()
print('projectile_Execute_byte_identical',method(prior,'void AIBalls::Execute(')==method(current,'void AIBalls::Execute('));assert method(prior,'void AIBalls::Execute(')==method(current,'void AIBalls::Execute(')

render_shared=r'''
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>
#include "c_vector.h"
struct CMatrix {float m[4][4]{};CVECTOR p{0.f};CMatrix(){for(int i=0;i<4;++i)m[i][i]=1.f;}CVECTOR operator*(CVECTOR v)const{return v+p;}void Get3X3(CMatrix&r)const{r=CMatrix{};}};
namespace storm_render {enum {TransformWorld};struct Viewport{unsigned Height=1285;};}
struct RS_RECT {CVECTOR vPos;float fSize,fAngle;unsigned dwColor,dwSubTexture;};
struct RS_LINE {CVECTOR vPos;unsigned dwColor;};
struct Renderer {CMatrix world,view,projection;storm_render::Viewport viewport;CVECTOR trace{0.f};float dartSize=0,ordinarySize=0;int lines=0,rects=0;CMatrix GetView(){return view;}CMatrix GetProjection(){return projection;}void GetViewport(storm_render::Viewport*v){*v=viewport;}void GetTransform(int,CMatrix&m){m=world;}void SetTransform(int,const CMatrix&m){world=m;}void TextureSet(int,int){}void DrawLines(RS_LINE*r,size_t n,const char*){trace=world*r[0].vPos;lines+=int(n);}void DrawRects(RS_RECT*r,size_t,const char*,int sx,int sy){++rects;if(sx==1&&sy==1)dartSize=r[0].fSize;else ordinarySize=r[0].fSize;}};
struct AIHelper {static inline Renderer *pRS=nullptr;};
struct AIBalls {std::vector<RS_LINE>dartTrace;std::vector<RS_RECT>dartRects,aBallRects;std::vector<CVECTOR>dartDirections;int dartTexture=1,dwTextureIndex=2,dwSubTexX=2,dwSubTexY=2,dwFireBallFromCameraTime=0;void Realize(unsigned);};
'''
render_main=r'''
int main(){
 for(float depth:{10.f,100.f,500.f,1000.f,2000.f,4200.f,-100.f}){
  Renderer rs;AIHelper::pRS=&rs;rs.world.p=CVECTOR(400.f,0.f,250.f);rs.projection.m[1][1]=1.f/std::tan(3.14159265f/6.f);
  AIBalls balls;const CVECTOR head(0.f,5.f,depth);
  for(int i=0;i<3;++i){balls.dartTrace.push_back({head,0xffffffff});balls.dartTrace.push_back({head-CVECTOR(0.f,0.f,7.f/3.f),0});}
  balls.dartRects.push_back({head,.4f,0,0xffffff,0});balls.dartDirections.push_back(CVECTOR(0.f,0.f,1.f));
  balls.aBallRects.push_back({head,.35f,0,0xffffff,0});
  balls.Realize(16);
  const float outer=depth>0.f?rs.dartSize*1285.f*rs.projection.m[1][1]/depth:0.f;
  std::printf("render depth_m=%.0f trace_world_error_m=%.6f world_restore_error_m=%.6f dart_outer_px=%.6f ordinary_radius_m=%.6f line_segments=%d rect_draws=%d\n",depth,(rs.trace-head).GetLength(),(rs.world.p-CVECTOR(400.f,0.f,250.f)).GetLength(),outer,rs.ordinarySize,rs.lines,rs.rects);
 }
}
'''
render_rows={}
for label,source in [('baseline',prior),('candidate',current)]:
 cpp=render_shared+method(source,'void AIBalls::Realize(')+render_main
 file=work/(label+'-render.cpp');file.write_text(cpp);exe=work/(label+'-render')
 p=subprocess.run(['/usr/bin/c++','-std=c++20','-O2','-I'+str(root/'experiments/native-metal/engine/src/libs/math/include'),str(file),'-o',str(exe)],text=True,capture_output=True)
 if p.returncode:print(p.stderr);raise SystemExit(p.returncode)
 output=subprocess.check_output([str(exe)],text=True)
 print(label+'_render_raw_begin');print(output,end='');print(label+'_render_raw_end')
 render_rows[label]=output.splitlines()
for row in render_rows['candidate']:
 values=dict(item.split('=') for item in row.split()[1:])
 assert float(values['trace_world_error_m'])==0
 assert float(values['world_restore_error_m'])==0
 assert float(values['ordinary_radius_m'])==.35
 assert values['line_segments']=='3' and values['rect_draws']=='2'
 if float(values['depth_m'])>0:assert float(values['dart_outer_px'])>=7.9999
print('render_world_state_sprite_support_ordinary_draw_controls PASS')
import struct,math
asset=root/'src/assets/ui/darts-projectile.tga.tx';data=asset.read_bytes()
_,width,height,mips,fmt,_=struct.unpack('<6I',data[:24]);pixels=data[24:24+width*height*4]
points=[(i%width,i//width) for i in range(width*height) if pixels[i*4+3]>=128]
axis=(.77,-.64);norm=math.hypot(*axis);axis=tuple(v/norm for v in axis)
stations=[(x*axis[0]+y*axis[1],-x*axis[1]+y*axis[0]) for x,y in points]
length=max(u for u,v in stations)-min(u for u,v in stations);mid=(max(u for u,v in stations)+min(u for u,v in stations))/2
shaft=[v for u,v in stations if abs(u-mid)<length*.125]
print('atlas_sha256',hashlib.sha256(data).hexdigest(),'dimensions',width,height,'mips',mips,'format',fmt)
print('atlas_opaque_coverage',len(points)/(width*height),'opaque_area_at_8px',len(points)/(width*height)*64,'central_shaft_width_at_8px',(max(shaft)-min(shaft))/width*8)
offset=24
for level in range(mips):
 w,h=max(1,width>>level),max(1,height>>level);alpha=data[offset+3:offset+w*h*4:4]
 if w<=8:print('atlas_actual_mip',level,'dimensions',w,h,'max_alpha',max(alpha),'pixels_alpha_ge_128',sum(a>=128 for a in alpha),'summed_alpha_area',sum(alpha)/255)
 offset+=w*h*4
print('ai_balls_baseline_sha256',hashlib.sha256(prior.encode()).hexdigest(),'ai_balls_candidate_sha256',hashlib.sha256(current.encode()).hexdigest())
print('compiler',subprocess.check_output(['/usr/bin/c++','--version'],text=True).splitlines()[0])
print('CPU_SOURCE_COMPONENT_REPLAY PASS; native stern appearance/FPS not measured')
