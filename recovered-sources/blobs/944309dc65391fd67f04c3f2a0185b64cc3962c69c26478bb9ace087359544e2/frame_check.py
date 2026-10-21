from pathlib import Path
import subprocess
repo=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
temp=Path(__file__).parent
include=repo/'experiments/native-metal/engine/src/libs/touch/src'
relative='experiments/native-metal/engine/src/libs/touch/src/ship_contact_frame.hpp'
before=subprocess.check_output(['git','show','4832b2e:'+relative],cwd=repo,text=True)
(temp/'before').mkdir(exist_ok=True)
(temp/'before/ship_contact_frame.hpp').write_text(before)
source=r'''
#include "ship_contact_frame.hpp"
#include <cassert>
#include <cstdio>
using namespace storm::ship::contact;
FrameBody hull(double x,double end,double y,double nextY,double height){
 FrameBody b;b.body.hull={{-2,-8},{2,-8},{2,8},{-2,8}};b.body.mass=1000;
 b.body.start.position={x,0};b.body.end.position={end,0};b.yStart=y;b.yEnd=nextY;b.height=height;return b;
}
int main(){
 for(int n=0;n<5;++n){
  auto a=hull(-12,0,0,0,10),b=hull(0,0,3,3,4);
  if(n==1){b.yStart=20;b.yEnd=2;}
  if(n==2){b.yStart=50;b.yEnd=50;}
  if(n==3){a.body.start.position.x=-3;a.body.end.position.x=-12;}
  if(n==4){b.body.mass=10000;b.wreck=true;b.yStart=-3;b.yEnd=-3;}
  auto result=integrateFrame({a,b},1.0);
  if(n==0||n==1||n==4)assert(!result.impacts.empty());
  if(n==2||n==3)assert(result.impacts.empty());
  #ifdef HEIGHT
  if(n==0)assert(std::abs(result.impacts[0].height-3)<1e-6);
  if(n==1)assert(result.impacts[0].height>=1&&result.impacts[0].height<=5.000001);
  if(n==4)assert(std::abs(result.impacts[0].height+3)<1e-6);
  #endif
  printf("%d %d %zu\n",n,result.exhausted,result.impacts.size());
  for(auto&m:result.motions)printf("%.17g %.17g %.17g %.17g %.17g\n",m.finalPose.position.x,m.finalPose.position.z,m.deltaV.x,m.deltaV.z,m.deltaW);
  for(auto&i:result.impacts)printf("%.17g %.17g %.17g %.17g\n",i.specificEnergyA,i.specificEnergyB,i.normalImpulse,i.normalClosing);
 }
}
'''
(temp/'frame.cpp').write_text(source)
outputs=[]
for name,dir in [('before',temp/'before'),('after',include)]:
    flags=['-DHEIGHT'] if name=='after' else []
    subprocess.run(['clang++','-std=c++20','-O1','-fsanitize=address,undefined',*flags,'-I'+str(dir),'-I'+str(include),str(temp/'frame.cpp'),'-o',str(temp/('frame-'+name))],check=True)
    outputs.append(subprocess.check_output([str(temp/('frame-'+name))],text=True))
assert outputs[0]==outputs[1]
print('PASS exact before/after contact motion and energy; local height at unequal/sinking bands and landing; airborne/separating negatives')
