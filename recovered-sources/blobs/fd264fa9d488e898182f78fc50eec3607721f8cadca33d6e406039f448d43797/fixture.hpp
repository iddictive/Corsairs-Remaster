#include <cassert>
#include <fstream>
#include <iostream>
using namespace storm_world_cut;
inline Vertex vertex(Vec3 p){Vertex v;v.position=p;v.normal={0,1,0};return v;}
inline void tri(Mesh&m,int id,Vec3 a,Vec3 b,Vec3 c){Triangle t{vertex(a),vertex(b),vertex(c)};surfaceNormal(t);m[id].push_back(t);}
inline Mesh fixture(){Mesh m;tri(m,0,{-4,-2,-4},{4,-2,4},{4,-2,-4});tri(m,0,{-4,-2,-4},{-4,-2,4},{4,-2,4});
 Vec3 p[8]={{-1,1,-1},{1,1,-1},{1,3,-1},{-1,3,-1},{-1,1,1},{1,1,1},{1,3,1},{-1,3,1}};
 int f[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{0,1,5,4},{3,7,6,2}};
 for(auto&a:f){tri(m,1,p[a[0]],p[a[1]],p[a[2]]);tri(m,1,p[a[0]],p[a[2]],p[a[3]]);}return m;}
inline bool same(const std::vector<Triangle>&a,const std::vector<Triangle>&b){if(a.size()!=b.size())return false;for(size_t t=0;t<a.size();++t)for(int v=0;v<3;++v)if(storm::ship::material::vertexWords(a[t][v])!=storm::ship::material::vertexWords(b[t][v]))return false;return true;}
inline double area(const std::vector<Triangle>&a){double sum=0;for(auto&t:a)sum+=storm_ship_hull::area(t);return sum;}
