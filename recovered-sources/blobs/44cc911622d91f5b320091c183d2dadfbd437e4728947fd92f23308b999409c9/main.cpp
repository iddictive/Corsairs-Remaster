#include "world_map_island_area.hpp"
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
struct Area{std::string name;GEOS geo;};
bool prior(GEOS&g,float x,float z){GEOS::INFO i;g.GetInfo(i);float dx=x-i.boxcenter.x,dz=z-i.boxcenter.z;float r=.01f+std::sqrt(i.boxsize.x*i.boxsize.x+i.boxsize.z*i.boxsize.z)*.51f;if(dx*dx+dz*dz+i.boxcenter.y*i.boxcenter.y>r*r)return false;GEOS::PLANE p[]={{{0,0,1},z+.01f},{{0,0,-1},-(z-.01f)},{{1,0,0},x+.01f},{{-1,0,0},-(x-.01f)}};return g.Clip(p,4,{x,0,z},1000,nullptr);}
int main(int,char**argv){std::ifstream in(argv[1]);int count;in>>count;std::vector<Area>areas(count);size_t samples=0,mismatch=0;for(auto&a:areas){int n;in>>a.name>>n;a.geo.triangles.resize(n);GEOS::VERTEX lo{1e9,1e9,1e9},hi{-1e9,-1e9,-1e9};for(auto&t:a.geo.triangles)for(auto&v:t){in>>v.x>>v.y>>v.z;lo={std::min(lo.x,v.x),std::min(lo.y,v.y),std::min(lo.z,v.z)};hi={std::max(hi.x,v.x),std::max(hi.y,v.y),std::max(hi.z,v.z)};}a.geo.info={{(lo.x+hi.x)/2,(lo.y+hi.y)/2,(lo.z+hi.z)/2},{hi.x-lo.x,hi.y-lo.y,hi.z-lo.z}};for(int x=-1040;x<=1040;x+=17)for(int z=-1040;z<=1040;z+=17){++samples;if(prior(a.geo,x,z)!=storm_map_resources::ContainsIslandArea(&a.geo,x,z))++mismatch;}if(storm_map_resources::ContainsIslandArea(&a.geo,NAN,0)||storm_map_resources::ContainsIslandArea(&a.geo,0,INFINITY))return 2;}
if(storm_map_resources::ContainsIslandArea(nullptr,0,0))return 3;
for(auto [z,expected]:{std::pair{-400.f,"Barbados"},std::pair{100.f,"Antigua"}}){bool matched=false;for(auto&a:areas)if(a.name==expected&&storm_map_resources::ContainsIslandArea(&a.geo,900,z))matched=true;if(!matched)return 4;}
std::cout<<"areas="<<count<<" parity_samples="<<samples<<" mismatches="<<mismatch<<" route=Barbados-Antigua null/nonfinite=pass\n";return mismatch?1:0;}
