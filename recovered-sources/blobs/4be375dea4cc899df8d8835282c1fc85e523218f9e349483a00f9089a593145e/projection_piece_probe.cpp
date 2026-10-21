#ifndef PROJECTION_HEADER
#define PROJECTION_HEADER "ship_contact_material.hpp"
#endif
#include PROJECTION_HEADER
#include <cstdio>
using namespace storm::ship::contact;
int main(){std::vector<MaterialVertex> stock={{0x1.fcb63ep-33,0x1.fdda56p+2,-0x1.c4fcacp+4},{0x1.600c7ap-32,0x1.00777cp+3,-0x1.c4c604p+4},{-0x1.484c8p-6,0x1.00989p+3,-0x1.c4fcacp+4},{0x1.612d3ep-32,0x1.00940ep+3,-0x1.c4fcacp+4},{0x1.600c7ap-32,0x1.00777cp+3,-0x1.c4c604p+4},{0x1.fcb63ep-33,0x1.fdda56p+2,-0x1.c4fcacp+4},{0x1.600c7ap-32,0x1.00777cp+3,-0x1.c4c604p+4},{0x1.612d3ep-32,0x1.00940ep+3,-0x1.c4fcacp+4},{-0x1.484c8p-6,0x1.00989p+3,-0x1.c4fcacp+4},{0x1.fcb63ep-33,0x1.fdda56p+2,-0x1.c4fcacp+4},{-0x1.484c8p-6,0x1.00989p+3,-0x1.c4fcacp+4},{0x1.612d3ep-32,0x1.00940ep+3,-0x1.c4fcacp+4}};
 for(double scale:{1.,1.e-6,1.e6})for(Vec2 shift:std::vector<Vec2>{{0,0},{100,200}}){auto v=stock;for(auto&p:v){p.x=p.x*scale+shift.x;p.y*=scale;p.z=p.z*scale+shift.z;}MaterialSnapshot s;bool ok=appendMaterialPiece(s,v,1001,28);std::printf("thin scale=%g shift=%g accepted=%d points=%zu inertia=%g\n",scale,shift.x,ok,ok?s.pieces[0].hull.points.size():0,ok?s.pieces[0].hull.inertia:0);
#ifndef BASELINE
 if(!ok)return 1;
 const Vec2 a{v[2].x,v[2].z},b{v[1].x,v[1].z},c{v[3].x,v[3].z};const double expectedI=(dot(a,a)+dot(b,b)+dot(c,c)+dot(a,b)+dot(a,c)+dot(b,c))/6.;if(std::abs(s.pieces[0].hull.inertia-expectedI)>1.e-12*std::max(1.e-30,expectedI))return 5;
 const double expectedA=std::abs(cross(b-a,c-a));const auto&h=s.pieces[0].hull.points;double actualA=0;Vec2 centroid{};for(size_t n=0;n<h.size();++n){const auto x=h[n]-a,y=h[(n+1)%h.size()]-a;const double w=cross(x,y);actualA+=w;centroid+=(x+y)*w;}centroid=a+centroid/(3*actualA);const auto expectedC=(a+b+c)/3.;if(std::abs(actualA-expectedA)>1.e-10*expectedA||length(centroid-expectedC)>1.e-12*std::max(1.,length(expectedC)))return 6;
#endif
 }
 MaterialSnapshot s;if(!appendMaterialPiece(s,{{13,0,7},{15,0,7},{15,1,11},{13,1,11}},1))return 2;double expected=14.*14.+9.*9.+(2.*2.+4.*4.)/12.;std::printf("ordinary inertia=%.17g expected=%.17g\n",s.pieces[0].hull.inertia,expected);if(std::abs(s.pieces[0].hull.inertia-expected)>1.e-12)return 3;
 MaterialSnapshot invalid;if(appendMaterialPiece(invalid,{{0,0,0},{1,0,1},{2,1,2}},2))return 4;std::puts("collinear rejected");
}
