#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <vector>

namespace storm_ship_hull {
struct Vec3 {float x=0,y=0,z=0;};
inline Vec3 operator+(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
inline Vec3 operator-(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
inline Vec3 operator*(Vec3 a,float b){return {a.x*b,a.y*b,a.z*b};}
inline float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
inline Vec3 cross(Vec3 a,Vec3 b){return {a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x};}
inline float length(Vec3 a){return std::sqrt(dot(a,a));}
inline Vec3 normalized(Vec3 a){float n=length(a);return n>1.e-8f?a*(1.f/n):Vec3{};}
inline bool finite(Vec3 a){return std::isfinite(a.x)&&std::isfinite(a.y)&&std::isfinite(a.z);}
inline uint32_t randomWord(uint32_t&seed){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
inline float randomUnit(uint32_t&seed){return float(randomWord(seed)&0xffffffu)/16777216.f;}
struct Impact {
 Vec3 center,normal;
 float radiusU=.08f,radiusV=.08f,halfDepth=.05f;
 uint32_t seed=1;int32_t object=-1;
};
inline bool valid(const Impact&i){return finite(i.center)&&finite(i.normal)&&length(i.normal)>.9f&&length(i.normal)<1.1f&&
 std::isfinite(i.radiusU)&&std::isfinite(i.radiusV)&&std::isfinite(i.halfDepth)&&i.radiusU>=.01f&&i.radiusU<=1.f&&i.radiusV>=.01f&&i.radiusV<=1.f&&i.halfDepth>=.01f&&i.halfDepth<=.25f&&i.object>=0;}
struct Frame {Vec3 tangent,bitangent;};
inline Frame frame(Vec3 normal){auto t=normalized(cross(std::abs(normal.y)<.9f?Vec3{0,1,0}:Vec3{0,0,1},normal));return {t,cross(normal,t)};}
struct Plane {Vec3 normal;float distance;};
// A convex, irregular seeded octagon extruded only through finite receiver
// support. This field stays in node-local metres as the MODEL moves.
inline std::array<Plane,10> prism(const Impact&i){
 auto f=frame(i.normal);std::array<std::array<float,2>,8>corners{};uint32_t seed=i.seed?i.seed:1;
 for(unsigned n=0;n<8;n++){float angle=float(n)*.7853981634f,r=.92f+randomUnit(seed)*.16f;corners[n]={std::cos(angle)*i.radiusU*r,std::sin(angle)*i.radiusV*r};}
 std::array<Plane,10>planes{};
 for(unsigned n=0;n<8;n++){auto a=corners[n],b=corners[(n+1)%8];Vec3 outward=normalized(f.tangent*(b[1]-a[1])+f.bitangent*(a[0]-b[0]));Vec3 point=i.center+f.tangent*a[0]+f.bitangent*a[1];planes[n]={outward,dot(outward,point)};}
 planes[8]={i.normal,dot(i.normal,i.center)+i.halfDepth};planes[9]={i.normal*-1.f,-dot(i.normal,i.center)+i.halfDepth};return planes;
}
inline bool contains(const Impact&i,Vec3 p){for(auto plane:prism(i))if(dot(plane.normal,p)-plane.distance>1.e-6f)return false;return true;}
struct Vertex {Vec3 position,normal;uint32_t color=0xffffffffu;std::array<float,8>uv{};};
using Triangle=std::array<Vertex,3>;
inline Vertex interpolate(const Vertex&a,const Vertex&b,float t){
 Vertex v;v.position=a.position+(b.position-a.position)*t;v.normal=a.normal+(b.normal-a.normal)*t;
 for(unsigned n=0;n<8;n++)v.uv[n]=a.uv[n]+(b.uv[n]-a.uv[n])*t;
 v.color=0;for(unsigned n=0;n<4;n++){float x=float((a.color>>(n*8))&255u),y=float((b.color>>(n*8))&255u);v.color|=uint32_t(std::clamp(std::lround(x+(y-x)*t),0l,255l))<<(n*8);}return v;
}
inline float area(const Triangle&t){return length(cross(t[1].position-t[0].position,t[2].position-t[0].position))*.5f;}
inline void fan(const std::vector<Vertex>&p,std::vector<Triangle>&out){for(size_t n=1;n+1<p.size();n++){Triangle t={p[0],p[n],p[n+1]};if(area(t)>1.e-10f)out.push_back(t);}}
inline void split(const std::vector<Vertex>&p,const Plane&plane,std::vector<Vertex>&inside,std::vector<Vertex>&outside){
 inside.clear();outside.clear();if(p.empty())return;
 for(size_t n=0;n<p.size();n++){const auto&a=p[n];const auto&b=p[(n+1)%p.size()];float da=dot(plane.normal,a.position)-plane.distance,db=dot(plane.normal,b.position)-plane.distance;bool ai=da<=0,bi=db<=0;
  (ai?inside:outside).push_back(a);if(ai!=bi){auto v=interpolate(a,b,std::clamp(da/(da-db),0.f,1.f));inside.push_back(v);outside.push_back(v);}
 }
}
struct Cut {std::vector<Triangle>surface,rim;float removedArea=0;};
// A triangle clipped by ten convex planes has at most thirteen distinct vertices;
// reserve thirty-two for coincident boundary vertices introduced by float splits.
// Fixed scratch avoids allocating polygons for each local source triangle.
struct ScratchPolygon {
 std::array<Vertex,32>vertices;unsigned count=0;
 void append(const Vertex&v){vertices[count++]=v;}
 void assign(const ScratchPolygon&other){count=other.count;std::copy_n(other.vertices.begin(),count,vertices.begin());}
};
inline void split(const ScratchPolygon&p,const Plane&plane,ScratchPolygon&inside,ScratchPolygon&outside){inside.count=outside.count=0;
 for(unsigned n=0;n<p.count;n++){const auto&a=p.vertices[n];const auto&b=p.vertices[(n+1)%p.count];float da=dot(plane.normal,a.position)-plane.distance,db=dot(plane.normal,b.position)-plane.distance;bool ai=da<=0,bi=db<=0;
  (ai?inside:outside).append(a);if(ai!=bi){auto v=interpolate(a,b,std::clamp(da/(da-db),0.f,1.f));inside.append(v);outside.append(v);}}
}
inline void fan(const ScratchPolygon&p,std::vector<Triangle>&out){for(unsigned n=1;n+1<p.count;n++){Triangle t={p.vertices[0],p.vertices[n],p.vertices[n+1]};if(area(t)>1.e-10f)out.push_back(t);}}
inline bool mayIntersect(const Triangle&t,const Impact&i){Vec3 lo=t[0].position,hi=lo;for(unsigned n=1;n<3;n++){auto p=t[n].position;lo={std::min(lo.x,p.x),std::min(lo.y,p.y),std::min(lo.z,p.z)};hi={std::max(hi.x,p.x),std::max(hi.y,p.y),std::max(hi.z,p.z)};}
 Vec3 q={std::clamp(i.center.x,lo.x,hi.x),std::clamp(i.center.y,lo.y,hi.y),std::clamp(i.center.z,lo.z,hi.z)};auto d=q-i.center;float r=std::max(i.radiusU,i.radiusV)*1.08f;return dot(d,d)<=r*r+i.halfDepth*i.halfDepth;}
inline Cut subtract(const Triangle&t,const Impact&i,const std::array<Plane,10>&planes){
 Cut result;if(!valid(i)||!mayIntersect(t,i)||dot(normalized(t[0].normal+t[1].normal+t[2].normal),i.normal)<.30f){result.surface.push_back(t);return result;}
 ScratchPolygon remaining,inside,outside;for(const auto&v:t)remaining.append(v);
 for(auto plane:planes){split(remaining,plane,inside,outside);remaining.assign(inside);if(remaining.count<3)break;}
 for(unsigned n=1;n+1<remaining.count;n++)result.removedArea+=area({remaining.vertices[0],remaining.vertices[n],remaining.vertices[n+1]});
 if(result.removedArea<=1.e-10f){result.surface.push_back(t);result.removedArea=0;return result;}
 remaining.count=0;for(const auto&v:t)remaining.append(v);
 for(auto plane:planes){split(remaining,plane,inside,outside);fan(outside,result.surface);remaining.assign(inside);if(remaining.count<3)break;}
 // Actual small wood edge geometry, using the original texture coordinates.
 // Never add a disk/black backing inside the removed opening.
 float thickness=std::min(.055f,i.halfDepth*.75f);
 for(unsigned n=0;n<remaining.count;n++){auto a=remaining.vertices[n],b=remaining.vertices[(n+1)%remaining.count];bool boundary=false;
  for(unsigned side=0;side<8;side++)if(std::abs(dot(planes[side].normal,a.position)-planes[side].distance)<2.e-5f&&std::abs(dot(planes[side].normal,b.position)-planes[side].distance)<2.e-5f){boundary=true;break;}
  if(!boundary||length(b.position-a.position)<1.e-5f)continue;auto c=b,d=a;c.position=c.position-i.normal*thickness;d.position=d.position-i.normal*thickness;
  auto normal=normalized(cross(i.normal,b.position-a.position));a.normal=b.normal=c.normal=d.normal=normal;
  Triangle first={a,b,c},second={a,c,d};if(area(first)>1.e-10f)result.rim.push_back(first);if(area(second)>1.e-10f)result.rim.push_back(second);
 }
 return result;
}
inline Cut subtract(const Triangle&t,const Impact&i){return subtract(t,i,prism(i));}
// Rim normals face into the aperture, so receiver-facing admission must use the
// original hull triangle. Subtract only volume here; never create another bevel.
inline std::vector<Triangle> clipRim(const Triangle&t,const Impact&i,const std::array<Plane,10>&field){
 if(!valid(i)||!mayIntersect(t,i))return {t};auto planes=field;for(auto&plane:planes)plane.distance-=2.e-5f;
 ScratchPolygon remaining,inside,outside;for(const auto&v:t)remaining.append(v);
 for(auto plane:planes){split(remaining,plane,inside,outside);remaining.assign(inside);if(remaining.count<3)return {t};}
 float removed=0;for(unsigned n=1;n+1<remaining.count;n++)removed+=area({remaining.vertices[0],remaining.vertices[n],remaining.vertices[n+1]});if(removed<=1.e-10f)return {t};
 std::vector<Triangle>result;remaining.count=0;for(const auto&v:t)remaining.append(v);
 for(auto plane:planes){split(remaining,plane,inside,outside);fan(outside,result);remaining.assign(inside);if(remaining.count<3)break;}return result;
}
// Native static GM layout: position, normal, ARGB, then one to four UV pairs.
inline Vertex decode(const uint8_t*p,unsigned stride){Vertex v;std::memcpy(&v.position,p,12);std::memcpy(&v.normal,p+12,12);std::memcpy(&v.color,p+24,4);std::memcpy(v.uv.data(),p+28,std::min(stride-28,32u));return v;}
inline void encode(const Vertex&v,uint8_t*p,unsigned stride){std::memcpy(p,&v.position,12);std::memcpy(p+12,&v.normal,12);std::memcpy(p+24,&v.color,4);std::memcpy(p+28,v.uv.data(),std::min(stride-28,32u));}
struct Bounds {Vec3 lo,hi;};
inline Bounds merge(Bounds a,Bounds b){return {{std::min(a.lo.x,b.lo.x),std::min(a.lo.y,b.lo.y),std::min(a.lo.z,b.lo.z)},{std::max(a.hi.x,b.hi.x),std::max(a.hi.y,b.hi.y),std::max(a.hi.z,b.hi.z)}};}
inline bool overlaps(Bounds a,Bounds b){return a.lo.x<=b.hi.x&&a.hi.x>=b.lo.x&&a.lo.y<=b.hi.y&&a.hi.y>=b.lo.y&&a.lo.z<=b.hi.z&&a.hi.z>=b.lo.z;}
inline bool rayBounds(Bounds box,Vec3 start,Vec3 end){float lo=0,hi=1;const float a[3]={start.x,start.y,start.z},b[3]={end.x,end.y,end.z},minimum[3]={box.lo.x,box.lo.y,box.lo.z},maximum[3]={box.hi.x,box.hi.y,box.hi.z};
 for(unsigned axis=0;axis<3;++axis){const float delta=b[axis]-a[axis];if(std::abs(delta)<1.e-12f){if(a[axis]<minimum[axis]-1.e-5f||a[axis]>maximum[axis]+1.e-5f)return false;continue;}float first=(minimum[axis]-1.e-5f-a[axis])/delta,last=(maximum[axis]+1.e-5f-a[axis])/delta;if(first>last)std::swap(first,last);lo=std::max(lo,first);hi=std::min(hi,last);if(lo>hi)return false;}return true;}
// Immutable render-triangle broadphase, separate from the authored collision BSP.
// Built once per instance/object; a hit visits only local receiver candidates.
class Broadphase {
 struct Node {Bounds bounds;uint32_t first=0,count=0,left=0,right=0;};
 std::vector<Bounds>bounds;std::vector<uint32_t>order;std::vector<Node>nodes;
 uint32_t buildNode(uint32_t first,uint32_t count){uint32_t index=uint32_t(nodes.size());nodes.emplace_back();Bounds box=bounds[order[first]];for(uint32_t n=1;n<count;n++)box=merge(box,bounds[order[first+n]]);nodes[index].bounds=box;
  if(count<=16){nodes[index].first=first;nodes[index].count=count;return index;}
  auto extent=box.hi-box.lo;unsigned axis=extent.x>=extent.y&&extent.x>=extent.z?0:extent.y>=extent.z?1:2;auto center=[&](uint32_t n){auto b=bounds[n];return axis==0?b.lo.x+b.hi.x:axis==1?b.lo.y+b.hi.y:b.lo.z+b.hi.z;};
  uint32_t half=count/2;std::nth_element(order.begin()+first,order.begin()+first+half,order.begin()+first+count,[&](uint32_t a,uint32_t b){return center(a)<center(b);});uint32_t left=buildNode(first,half),right=buildNode(first+half,count-half);nodes[index].left=left;nodes[index].right=right;return index;}
 void visit(uint32_t index,Bounds box,std::vector<uint32_t>&out)const{const auto&node=nodes[index];if(!overlaps(node.bounds,box))return;if(node.count){for(uint32_t n=0;n<node.count;n++){auto triangle=order[node.first+n];if(overlaps(bounds[triangle],box))out.push_back(triangle);}return;}visit(node.left,box,out);visit(node.right,box,out);}
 void visitRay(uint32_t index,Vec3 start,Vec3 end,std::vector<uint32_t>&out)const{const auto&node=nodes[index];if(!rayBounds(node.bounds,start,end))return;if(node.count){for(uint32_t n=0;n<node.count;++n){auto triangle=order[node.first+n];if(rayBounds(bounds[triangle],start,end))out.push_back(triangle);}return;}visitRay(node.left,start,end,out);visitRay(node.right,start,end,out);}
 public:
 void build(const std::vector<Bounds>&boxes){bounds=boxes;order.clear();nodes.clear();order.reserve(bounds.size());for(uint32_t n=0;n<bounds.size();++n)order.push_back(n);nodes.reserve(bounds.size()*2);if(!bounds.empty())buildNode(0,uint32_t(bounds.size()));}
 void build(const std::vector<uint8_t>&vertices,unsigned stride,const std::vector<uint16_t>&indices){bounds.clear();order.clear();nodes.clear();bounds.reserve(indices.size()/3);order.reserve(indices.size()/3);
  for(size_t n=0;n<indices.size();n+=3){Vec3 p;std::memcpy(&p,vertices.data()+size_t(indices[n])*stride,12);Bounds box{p,p};for(unsigned corner=1;corner<3;corner++){std::memcpy(&p,vertices.data()+size_t(indices[n+corner])*stride,12);box=merge(box,{p,p});}order.push_back(uint32_t(bounds.size()));bounds.push_back(box);}nodes.reserve(bounds.size()*2);if(!bounds.empty())buildNode(0,uint32_t(bounds.size()));}
 void query(const Impact&i,std::vector<uint32_t>&out,float supportPadding=0)const{out.clear();if(nodes.empty()||!valid(i))return;auto f=frame(i.normal);Vec3 e{std::abs(f.tangent.x)*i.radiusU*1.08f+std::abs(f.bitangent.x)*i.radiusV*1.08f+std::abs(i.normal.x)*i.halfDepth,std::abs(f.tangent.y)*i.radiusU*1.08f+std::abs(f.bitangent.y)*i.radiusV*1.08f+std::abs(i.normal.y)*i.halfDepth,std::abs(f.tangent.z)*i.radiusU*1.08f+std::abs(f.bitangent.z)*i.radiusV*1.08f+std::abs(i.normal.z)*i.halfDepth};float padding=2.e-5f+std::max(0.f,supportPadding);e=e+Vec3{padding,padding,padding};visit(0,{i.center-e,i.center+e},out);}
 void queryRay(Vec3 start,Vec3 end,std::vector<uint32_t>&out)const{out.clear();if(!nodes.empty()&&finite(start)&&finite(end))visitRay(0,start,end,out);}
 void queryBounds(Bounds box,std::vector<uint32_t>&out)const{out.clear();if(!nodes.empty()&&finite(box.lo)&&finite(box.hi)&&box.lo.x<=box.hi.x&&box.lo.y<=box.hi.y&&box.lo.z<=box.hi.z)visit(0,box,out);}
};
inline uint64_t signature(const std::vector<Impact>&cuts){uint64_t h=1469598103934665603ull;auto add=[&](const void*p,size_t size){auto*b=static_cast<const uint8_t*>(p);for(size_t n=0;n<size;n++){h^=b[n];h*=1099511628211ull;}};
 for(const auto&i:cuts){add(&i.center,12);add(&i.normal,12);add(&i.radiusU,4);add(&i.radiusV,4);add(&i.halfDepth,4);add(&i.seed,4);add(&i.object,4);}return h;}
}
