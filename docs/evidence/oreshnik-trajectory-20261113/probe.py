from pathlib import Path
import os, subprocess
root=Path(__file__).resolve().parents[3]
tmp=Path(os.environ['PROBE_DIR'])
cpp=(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp').read_text()
def part(a,b):
 return cpp[cpp.index(a):cpp.index(b,cpp.index(a))]
shot=part('float shotVariation(', 'float conventionalWarheadScale(')
vary=part('void varyMissile(', 'void evadeInterceptor(')
scatter=part('const float angle = 6.2831853f * shotVariation(missile.shotId, index', '\n                    }\n                    if (offset')
solve=part('const float descentVariation =', '                        if (missile.intercepted) {')
release=part('const float flightX = missile.target.x - missile.launch.x;', '                // A rocket may cruise low')
(tmp/'c_vector.h').write_text('''#pragma once
#include <cmath>
struct CVECTOR { float x,y,z; CVECTOR(float a=0):x(a),y(a),z(a){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){} CVECTOR operator+(CVECTOR b)const{return {x+b.x,y+b.y,z+b.z};} CVECTOR operator-(CVECTOR b)const{return {x-b.x,y-b.y,z-b.z};} CVECTOR operator*(float s)const{return {x*s,y*s,z*s};} CVECTOR& operator+=(CVECTOR b){x+=b.x;y+=b.y;z+=b.z;return *this;} float operator~()const{return x*x+y*y+z*z;} CVECTOR operator!()const {float l=std::sqrt(~*this);return *this*(1/l);} };
''')
source='''#include <cstdint>\n#include <cstdio>\n#include <algorithm>\nusing entid_t=uint64_t;\n#include "'''+str(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.hpp')+'''"\nusing namespace storm::ship::phonk;\n'''+shot+vary+'''\nint main(){\nfloat minH=1e9,maxH=0,minV=1e9,maxV=0,minTime=1e9,maxTime=0,minGap=1e9,maxGap=0;\nfor(unsigned serial=1;serial<=12;++serial){ NuclearMissile missile; missile.launch=CVECTOR(0,10,0);missile.target=CVECTOR(9000,0,0);missile.owner=7;missile.type=missile::Type::Oreshnik;missile.cluster=missile.split=true;missile.shotId=serial;varyMissile(missile,7);\nfloat age=0,release=2; CVECTOR before,after,burstPosition; while(age<missile.duration){before=nuclearPosition(missile,age);age=std::min(missile.duration,age+1.f/60.f);after=nuclearPosition(missile,age); if(age>=missile.duration*.55f){\n'''+release+'''\n} if(release<=1){burstPosition=before+(after-before)*release;break;}}\nif(release>1)return 2;missile.age=age;CVECTOR heading=!(missile.target-missile.launch);heading.y=0;heading=!heading;float first=1e9,last=0;\nfor(unsigned index=0;index<24;++index){auto target=missile.target;\n'''+scatter+'''\nNuclearMissile child;child.launch=burstPosition;child.target=target;child.type=missile.type;child.cluster=true;child.initialVelocity=nuclearVelocity(missile,missile.age);const float inheritedSpeed=std::sqrt(~child.initialVelocity);const float carrierMeanSpeed=missile.nominalMeanSpeed;child.duration=std::max(.25f,std::sqrt(~(target-burstPosition))/(inheritedSpeed*1.05f));\n'''+solve+'''\nchild.continueVelocity=true;child.duration=30.f;float lo=0,hi=20;for(int i=0;i<50;++i){float mid=(lo+hi)*.5f;if(nuclearPosition(child,mid).y>0)lo=mid;else hi=mid;}float impact=(lo+hi)*.5f;auto v=nuclearVelocity(child,impact); float speed=std::sqrt(~v);minV=std::min(minV,speed);maxV=std::max(maxV,speed);minTime=std::min(minTime,impact);maxTime=std::max(maxTime,impact);first=std::min(first,impact);last=std::max(last,impact);}\nminH=std::min(minH,burstPosition.y);maxH=std::max(maxH,burstPosition.y);minGap=std::min(minGap,last-first);maxGap=std::max(maxGap,last-first);printf("shot=%u release=%.0fm duration=[%.3f,%.3f] spread=%.3fs\\n",serial,burstPosition.y,first,last,last-first);}\nprintf("summary release=[%.0f,%.0f]m impact-speed=[%.0f,%.0f]m/s flight=[%.3f,%.3f]s per-shot-spread=[%.3f,%.3f]s\\n",minH,maxH,minV,maxV,minTime,maxTime,minGap,maxGap);}\n'''
(tmp/'probe.cpp').write_text(source)
subprocess.run(['xcrun','clang++','-std=c++20','-O0','-I'+str(tmp),str(tmp/'probe.cpp'),'-o',str(tmp/'probe')],check=True)
subprocess.run([str(tmp/'probe')],check=True)
