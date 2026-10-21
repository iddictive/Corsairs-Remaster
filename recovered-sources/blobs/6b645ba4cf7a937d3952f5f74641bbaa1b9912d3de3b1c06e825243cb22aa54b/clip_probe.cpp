#ifndef MATERIAL_HEADER
#define MATERIAL_HEADER "ship_hull_material.hpp"
#endif
#include MATERIAL_HEADER
#include <chrono>
#include <cstdio>
using namespace storm::ship::material;
Triangle tri(Vec3 a,Vec3 b,Vec3 c){Triangle t{};t[0].position=a;t[1].position=b;t[2].position=c;return t;}
Mesh cube(){Mesh m;const Vec3 v[]={{0,0,0},{1,0,0},{1,1,0},{0,1,0},{0,0,1},{1,0,1},{1,1,1},{0,1,1}};const unsigned f[][3]={{0,2,1},{0,3,2},{4,5,6},{4,6,7},{0,1,5},{0,5,4},{3,7,6},{3,6,2},{0,4,7},{0,7,3},{1,2,6},{1,6,5}};for(const auto&t:f)m[0].push_back(tri(v[t[0]],v[t[1]],v[t[2]]));return m;}
int main(){auto solid=cube();
 for(int kind=0;kind<6;++kind){std::vector<storm_ship_hull::Plane>p={{{1,0,0},kind==0?2.f:kind==1?-1.f:kind==2?.5f:kind==3?1.f:kind==4?1.f+float(PlanarUnit)*.5f:1.f-float(PlanarUnit)*.5f},{{-1,0,0},1},{{0,1,0},2},{{0,-1,0},1},{{0,0,1},2},{{0,0,-1},1}};Mesh kept;const auto start=std::chrono::steady_clock::now();for(int i=0;i<1000;++i){kept.clear();if(!clipCell(solid,p,kept))return 2;}const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();std::printf("case=%d volume=%.9f triangles=%zu ms=%.3f\n",kind,closedVolume(kept),kept.empty()?0:kept.begin()->second.size(),ms);if(kind==0&&!kept.empty())return 3;if(std::abs(closedVolume(kept)-(kind==0?0.:kind==1?1.:kind==2?.5:0.))>1.e-6)return 4;}
}
