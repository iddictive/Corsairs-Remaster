#include "engine/src/libs/touch/src/ship_contact_frame.hpp"
#include <cstdio>
using namespace storm::ship::contact;
int main(){
    auto material=std::make_shared<MaterialSnapshot>();
    for(unsigned n=0;n<8;++n){std::vector<MaterialVertex>v;
        for(unsigned corner=0;corner<8;++corner)v.push_back({n*.6+((corner&1)? .4:0.),(corner&2)?1.:-1.,(corner&4)?1.:-1.});
        if(!appendMaterialPiece(*material,v,n,n))return 2;
    }
    buildMaterialIndex(*material);
    FrameBody ram,target;
    ram.body.hull={{-.2,-1},{.2,-1},{.2,1},{-.2,1}};ram.body.mass=100;
    ram.body.start.position={-2,0};ram.body.end.position={8,0};ram.height=2;
    target.body.hull={{0,-1},{5,-1},{5,1},{0,1}};target.body.mass=1;target.height=2;target.material=material;
    auto mask=std::make_shared<std::vector<uint8_t>>(8,1);unsigned calls=0;
    const auto fracture=[&](const FractureEvent&){
        FractureCommit result;unsigned n=0;while(n<mask->size()&&!mask->at(n))++n;
        if(n==mask->size())return result;
        mask=std::make_shared<std::vector<uint8_t>>(*mask);mask->at(n)=0;
        auto next=std::make_shared<MaterialSnapshot>();next->base=material;next->retainedSupports=mask;
        result.complete=result.replaceB=true;result.materialB=next;result.workB=.0001;++calls;return result;
    };
    FrameLimits limits;limits.fracturePasses=16;
    auto output=integrateFrame({ram,target},1.,limits,fracture);
    std::printf("successive_fractures=%u final_ram_x=%.9f remaining=%u exhausted=%d\n",calls,output.motions[0].finalPose.position.x,unsigned(std::count(mask->begin(),mask->end(),1)),output.exhausted);
    auto ordinary=integrateFrame({ram,target},1.);
    std::printf("ordinary_ram_x=%.9f\n",ordinary.motions[0].finalPose.position.x);
    return calls==8&&output.motions[0].finalPose.position.x>7.9&&ordinary.motions[1].deltaV.x>1?0:1;
}
