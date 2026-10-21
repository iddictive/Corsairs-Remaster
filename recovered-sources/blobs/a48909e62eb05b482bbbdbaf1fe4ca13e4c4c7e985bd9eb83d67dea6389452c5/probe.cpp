#include "query.hpp"
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
using namespace storm::ship::contact;
FrameBody box(double x,double end){
    FrameBody f;f.body.hull={{-1,-1},{1,-1},{1,1},{-1,1}};f.body.mass=1000;
    f.body.start.position={x,0};f.body.end.position={end,0};f.height=2;return f;
}
std::shared_ptr<const MaterialSnapshot> timber(unsigned count){
    auto m=std::make_shared<MaterialSnapshot>();
    for(unsigned n=0;n<count;++n){
        const double r=1.-.001*n;
        assert(appendMaterialPiece(*m,{{-r,-1,-r},{r,-1,-r},{r,1,r},{-r,1,r}},n));
    }
    buildMaterialIndex(*m);return m;
}
frame_detail::Proxy proxy(const FrameBody&f){
    frame_detail::Proxy p;p.sweep=f.body;p.pose=f.body.start;p.hull=detail::prepare(f.body);
    p.velocity=f.body.end.position-f.body.start.position;p.material=f.material;return p;
}
void event(const char*name,FrameBody a,FrameBody b,int repeat){
    auto pa=proxy(a),pb=proxy(b);frame_detail::exactCalls=0;
    frame_detail::Event e;const auto start=std::chrono::steady_clock::now();
    for(int n=0;n<repeat;++n)e=frame_detail::nextPair(a,b,pa,pb,0,1,1,{});
    const auto us=std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now()-start).count();
    std::printf("event %s %d %d %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g %.17g\n",name,e.hit,e.barrier,e.time,e.contact.point.x,e.contact.point.z,e.contact.normal.x,e.contact.normal.z,e.contact.penetration,e.contact.separation,e.response.deltaVA.x,e.response.energyLost);
    std::fprintf(stderr,"%s exact_calls=%zu repeats=%d elapsed_us=%lld\n",name,frame_detail::exactCalls,repeat,(long long)us);
}
int main(int argc,char**argv){
    const int repeats=argc>1?std::atoi(argv[1]):1;
    auto a=box(0,.1),b=box(1.5,1.5);a.material=timber(32);b.material=timber(32);
    event("persistent",a,b,repeats);
    auto future=a;future.body.start.position.x=-4;future.body.end.position.x=1;
    event("positive-toi",future,b,1);
    auto away=a;away.body.end.position.x=-.1;event("departing-overlap",away,b,1);
    auto separate=b;separate.body.start.position.x=10;separate.body.end.position.x=11;event("disjoint",a,separate,1);
    auto empty=a;empty.material=std::make_shared<MaterialSnapshot>();event("empty",empty,b,1);
    std::map<uint32_t,std::vector<MaterialPiece>> removed;
    for(unsigned n=0;n<32;++n)removed[n]={};
    auto cut=a;cut.material=applyMaterialReplacements(a.material,std::move(removed));assert(cut.material);event("all-removed",cut,b,1);
    for(int n=0;n<5;++n){
        auto x=box(0,4),y=box(3,3);
        if(n==1){x.material=timber(12);y.material=timber(12);}
        if(n==2){x.material=empty.material;y.material=timber(12);}
        if(n==3){x.body.end=x.body.start;y.body.start.position.x=12;y.body.end.position.x=13;}
        FrameLimits limits;if(n==4)limits.maxEvents=0;
        auto out=integrateFrame({x,y,box(-8,n==4?8:-8)},1,limits);
        std::printf("frame %d %d %zu\n",n,out.exhausted,out.impacts.size());
        for(const auto&m:out.motions)std::printf("motion %d %.17g %.17g %.17g %.17g %.17g %.17g\n",m.touched,m.finalPose.position.x,m.finalPose.position.z,m.finalPose.yaw,m.deltaV.x,m.deltaV.z,m.deltaW);
        for(const auto&i:out.impacts)std::printf("impact %.17g %.17g %.17g %.17g\n",i.point.x,i.point.z,i.normalImpulse,i.energyLostA+i.energyLostB);
        if(n==4)assert(out.exhausted&&out.motions[2].finalPose.position.x<=out.motions[0].finalPose.position.x-2.+1.e-5);
    }
}
