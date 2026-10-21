#pragma once
#include <vector>
#include <array>
#include <cmath>
#include <algorithm>
class GEOS { public:
struct VERTEX {float x,y,z;};struct INFO{VERTEX boxcenter,boxsize;};struct PLANE{VERTEX nrm;float d;};
INFO info{}; std::vector<std::array<VERTEX,3>>triangles;
void GetInfo(INFO&i){i=info;}
bool Clip(const PLANE*p,int count,const VERTEX&c,float,void*){
 if(!std::isfinite(c.x)||!std::isfinite(c.z))return false;
 for(const auto&t:triangles){std::vector<VERTEX>poly(t.begin(),t.end());for(int j=0;j<count&&!poly.empty();++j){std::vector<VERTEX>next;auto prev=poly.back();float pd=p[j].d-p[j].nrm.x*prev.x-p[j].nrm.z*prev.z;for(auto v:poly){float vd=p[j].d-p[j].nrm.x*v.x-p[j].nrm.z*v.z;if((vd>=0)!=(pd>=0)){float k=pd/(pd-vd);next.push_back({prev.x+(v.x-prev.x)*k,0,prev.z+(v.z-prev.z)*k});}if(vd>=0)next.push_back(v);prev=v;pd=vd;}poly=next;}if(!poly.empty())return true;}return false;}
};
