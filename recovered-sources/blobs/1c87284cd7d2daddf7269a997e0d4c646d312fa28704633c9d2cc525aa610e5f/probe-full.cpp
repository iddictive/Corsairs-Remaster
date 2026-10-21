#include <ship_contact_frame.hpp>
#include <chrono>
#include <iostream>
#include <iomanip>
#include <cstring>
#include <random>
#include <cstdint>
using namespace storm::ship::contact;
void scalar(double x){uint64_t bits;std::memcpy(&bits,&x,8);std::cout<<std::hex<<bits<<' ';}
void vec(Vec2 v){scalar(v.x);scalar(v.z);} void pose(Pose p){vec(p.position);scalar(p.yaw);}
void emit(const FrameResult&r){std::cout<<r.exhausted<<' '<<r.motions.size()<<' '<<r.impacts.size()<<' ';for(const auto&m:r.motions){pose(m.finalPose);vec(m.deltaV);scalar(m.deltaW);std::cout<<m.touched<<' ';}for(const auto&i:r.impacts){std::cout<<i.a<<' '<<i.b<<' ';vec(i.point);vec(i.normal);vec(i.localPointA);vec(i.localPointB);vec(i.localNormalA);vec(i.localNormalB);scalar(i.normalClosing);scalar(i.normalImpulse);scalar(i.energyLostA);scalar(i.energyLostB);scalar(i.specificEnergyA);scalar(i.specificEnergyB);}std::cout<<'\n';}
FrameBody body(int n,Vec2 pos,Vec2 travel,double yaw=0,double rotation=0){FrameBody f;f.height=12;f.body.mass=1000;f.body.start={pos,yaw};f.body.end={pos+travel,yaw+rotation};if(n==4)f.body.hull={{-4,-15},{4,-15},{4,15},{-4,15}};else for(int i=0;i<n;++i){double a=i*6.283185307179586/n;f.body.hull.push_back({4*std::cos(a),15*std::sin(a)});}return f;}
int main(int argc,char**argv){
 if(argc>1){for(int count:{6,16,32})for(int points:{4,19,30,51}){std::vector<FrameBody> fleet;for(int i=0;i<count;++i)fleet.push_back(body(points,{i*200.,0},{.05,.02},.1*i,.001));detail::prepareCalls=detail::queryCalls=0;auto r=integrateFrame(fleet,.016);if(r.exhausted||!r.impacts.empty())return 1;for(size_t i=0;i<fleet.size();++i)if(r.motions[i].touched||r.motions[i].finalPose.position.x!=fleet[i].body.end.position.x||r.motions[i].finalPose.position.z!=fleet[i].body.end.position.z||r.motions[i].finalPose.yaw!=fleet[i].body.end.yaw)return 2;auto prep=detail::prepareCalls,queries=detail::queryCalls;auto begin=std::chrono::steady_clock::now();for(int n=0;n<100;++n){auto q=integrateFrame(fleet,.016);if(q.motions.size()!=fleet.size())return 3;}std::cout<<count<<'x'<<points<<" prepare="<<prep<<" query="<<queries<<" us/frame="<<std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count()/100<<'\n';}return 0;}
 std::vector<std::vector<FrameBody>> cases;
 cases.push_back({});cases.push_back({body(4,{0,0},{0,0})});
 for(int n:{4,19,30,51})for(double dt:{.016,.05,.1}){
  for(auto ab:std::vector<std::pair<FrameBody,FrameBody>>{
   {body(n,{-40,0},{80,0}),body(n,{40,0},{-80,0})},
   {body(n,{0,0},{0,0},0,3.5),body(n,{20,0},{0,0},0,-1.5)},
   {body(n,{0,0},{0,0}),body(n,{0,0},{0,0})},
   {body(n,{0,0},{-4,0}),body(n,{7,0},{4,0})},
   {body(n,{0,0},{0,0}),body(n,{8,0},{0,4})},
   {body(n,{0,0},{500,0},0,12.566370614359172),body(n,{250,0},{0,0})}}){auto a=ab.first,b=ab.second;cases.push_back({a,b});emit(integrateFrame(cases.back(),dt));}
 }
 auto a=body(4,{0,0},{10,0}),b=body(4,{7,0},{0,0});
 cases.push_back({a,b,body(30,{18,0},{-5,0})});
 auto dry=b;dry.yStart=100;dry.yEnd=90;cases.push_back({a,dry});dry.yEnd=-100;cases.push_back({a,dry});
 auto wreck=b;wreck.wreck=true;cases.push_back({a,wreck});a.wreck=true;cases.push_back({a,wreck});a.wreck=false;
 for(int kind=0;kind<9;++kind){auto bad=b;if(kind==0)bad.body.mass=0;if(kind==1)bad.body.mass=-1;if(kind==2)bad.body.mass=NAN;if(kind==3)bad.body.hull[0].x=NAN;if(kind==4)bad.body.hull.clear();if(kind==5)bad.body.hull.resize(129);if(kind==6)bad.body.start.position.x=INFINITY;if(kind==7)bad.body.end.yaw=NAN;if(kind==8)bad.body.start.yaw=INFINITY;cases.push_back({a,bad});}
 std::mt19937 rng(462353);std::uniform_real_distribution<double> pos(-70,70),travel(-20,20),angle(-3.14,3.14);
 for(int i=0;i<80;++i){std::vector<FrameBody> fleet;for(int n=0;n<6;++n){auto f=body(4+n%3*15,{pos(rng),pos(rng)},{travel(rng),travel(rng)},angle(rng),angle(rng));f.body.mass=100+n*200;fleet.push_back(f);}cases.push_back(fleet);}
 for(const auto&fixture:cases)emit(integrateFrame(fixture,.016));
 for(double dt:{0.,-1.,double(NAN),double(INFINITY)})emit(integrateFrame({a,b},dt));
}
