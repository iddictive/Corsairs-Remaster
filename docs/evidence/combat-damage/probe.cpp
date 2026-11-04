#include "combat_damage_geometry.h"
#include <fstream>
#include <iostream>
#include <chrono>
#include <set>
using namespace storm::combat;
int main(int argc,char **argv) {
 std::ifstream in(argv[1]);size_t count;in>>count;storm_ship_laser::Plane plane;in>>plane.normal.x>>plane.normal.y>>plane.normal.z>>plane.distance;plane.charred=false;
 Mesh mesh;std::vector<Triangle> posed(count),moved(count);
 for(size_t t=0;t<count;++t){Triangle tri;for(int c=0;c<3;++c){auto &v=tri[c];in>>v.position.x>>v.position.y>>v.position.z;v.uv={0,0,float(t),0,0,0,0,0};v.uv[3+c]=1;v.color=0xffbac9d6;
 posed[t][c]=moved[t][c]=v;in>>posed[t][c].position.x>>posed[t][c].position.y>>posed[t][c].position.z>>moved[t][c].position.x>>moved[t][c].position.y>>moved[t][c].position.z>>v.uv[7];
 }mesh[0].push_back(tri);}
 const auto begin=std::chrono::steady_clock::now();auto cut=sever(mesh,plane);double cutMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
 if(cut.kept.empty()||cut.detached.empty()){std::cerr<<"FAIL cut rejected\n";return 1;}
 using Key=storm_ship_laser::PositionKey;using Edge=std::pair<Key,Key>;std::map<Edge,int> capEdges;const auto edgeKey=[](Vec3 a,Vec3 b){auto x=storm_ship_laser::positionKey(a),y=storm_ship_laser::positionKey(b);if(y<x)std::swap(x,y);return Edge{x,y};};for(const auto&t:cut.kept[-2])for(int c=0;c<3;++c)++capEdges[edgeKey(t[c].position,t[(c+1)%3].position)];int missing=0;for(const auto&e:cut.seam)if(capEdges[edgeKey(e.a.position,e.b.position)]!=1)++missing;std::cout<<"cap_seam_missing="<<missing<<" ";if(missing)return 6;size_t receiver=0;float nearest=1e9;
 for(size_t t=0;t<count;++t){auto center=(posed[t][0].position+posed[t][1].position+posed[t][2].position)*(1.f/3);float d=length(center-Vec3{0,1.3f,.16f});if(d<nearest){nearest=d;receiver=t;}}
 auto hit=(posed[receiver][0].position+posed[receiver][1].position+posed[receiver][2].position)*(1.f/3);auto normal=normalized(cross(posed[receiver][1].position-posed[receiver][0].position,posed[receiver][2].position-posed[receiver][0].position));
 auto before=std::chrono::steady_clock::now();auto mark=wound(posed,hit,normal,false);double woundMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-before).count();
 auto miss=wound(posed,hit+Vec3{0,0,10},normal,false);
 if(!triangleCount(mark)||triangleCount(miss)){std::cerr<<"FAIL projection positive/negative\n";return 2;}
 std::set<size_t> active;float maxError=0;
 for(auto *part:{&cut.kept,&cut.detached,&mark})for(const auto &entry:*part)for(const auto &tri:entry.second)for(const auto &v:tri){size_t id=size_t(std::lround(v.uv[2]));if(id>=count)return 3;float sum=v.uv[3]+v.uv[4]+v.uv[5];if(std::abs(sum-1)>1e-4)return 4;
 Vec3 p{};for(int c=0;c<3;++c)p=p+mesh[0][id][c].position*v.uv[3+c];if(part!=&mark)maxError=std::max(maxError,length(p-v.position));else active.insert(id);}
 if(maxError>1e-4){std::cerr<<"FAIL anchor mismatch "<<maxError<<"\n";return 5;}
 std::ofstream out(argv[2]);out<<"{\"cut_ms\":"<<cutMs<<",\"wound_ms\":"<<woundMs<<",\"source_triangles\":"<<count<<",\"body_triangles\":"<<triangleCount(cut.kept)<<",\"part_triangles\":"<<triangleCount(cut.detached)<<",\"cap_triangles\":"<<cut.kept[-2].size()<<",\"wound_triangles\":"<<triangleCount(mark)<<",\"wound_skin_triangles\":"<<active.size()<<",\"max_anchor_error\":"<<maxError<<",\"frames\":[";
 for(int frame=0;frame<3;++frame){if(frame)out<<',';out<<'[';bool first=true;
 auto emit=[&](const Mesh &part,const std::vector<Triangle>&pose,bool detached){for(const auto &entry:part)for(const auto &tri:entry.second){if(!first)out<<',';first=false;out<<'[';for(int c=0;c<3;++c){if(c)out<<',';const auto &v=tri[c];const auto &src=pose.at(size_t(std::lround(v.uv[2])));Vec3 p{};for(int j=0;j<3;++j)p=p+src[j].position*v.uv[3+j];if(detached)p=p+Vec3{.55f,-.15f,0};if(entry.first==-1)p=p+normal*(.001f+v.uv[6]*.0004f);out<<'['<<p.x<<','<<p.y<<','<<p.z<<','<<(v.color>>16&255)<<','<<(v.color>>8&255)<<','<<(v.color&255)<<','<<(v.color>>24)<<']';}out<<']';}};
 if(frame<2){emit(mesh,frame?moved:posed,false);emit(mark,frame?moved:posed,false);}else{emit(cut.kept,moved,false);emit(cut.detached,posed,true);}out<<']';}
 out<<"]}";std::cout<<"PASS triangles="<<count<<" caps="<<cut.kept[-2].size()<<" wound="<<triangleCount(mark)<<" active="<<active.size()<<" anchor_error="<<maxError<<" cut_ms="<<cutMs<<" wound_ms="<<woundMs<<"\n";
}
