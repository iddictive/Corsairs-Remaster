#pragma once
#include "entity.h"
#include <vector>
#include <cstring>
namespace storm_render {
inline bool succeeded(int n){return n==0;} struct SurfaceDesc{unsigned Width=1280,Height=720;int Format=1;}; struct Surface {int tag=0;int GetDesc(SurfaceDesc*d){*d={};return 0;} void Release(){} }; struct Texture{Surface surface;int GetSurfaceLevel(int,Surface**s){*s=&surface;return 0;} void Release(){delete this;}};
namespace usage{constexpr int renderTarget=1;} enum class MemoryClass{Default}; constexpr int TextureFilterNone=0,PrimitiveTriangleList=0; namespace vertexFormat{constexpr int xyzwTransformed=1,diffuse=2,tex1=4;} struct Viewport{unsigned Width=1280,Height=720;}; }
struct VDX9RENDER {
 CVECTOR position{100,40,200},angles{.2f,.7f,.1f}; float fov=1.1f; int cameraSets=0,lookCameraSets=0,perspectiveSets=0,captureCalls=0,textureCalls=0,failStep=0; storm_render::Surface source{17}; storm_render::Texture*bound=nullptr; std::vector<uint32_t> colors; std::vector<int> frameTags;
 void GetCamera(CVECTOR&p,CVECTOR&a,float&f){p=position;a=angles;f=fov;}
 void SetCamera(CVECTOR p,CVECTOR a,float f){position=p;angles=a;fov=f;cameraSets++;}
 void SetCamera(CVECTOR p,CVECTOR,CVECTOR){position=p;lookCameraSets++;}
 void SetPerspective(float f){fov=f;perspectiveSets++;}
 int GetRenderTarget(storm_render::Surface**s){captureCalls++;*s=failStep==1?nullptr:&source;return failStep==1?1:0;}
 int CreateTexture(unsigned,unsigned,int,int,int,storm_render::MemoryClass,storm_render::Texture**t){if(failStep==2)return 1;*t=new storm_render::Texture;return 0;}
 int StretchRect(storm_render::Surface*s,void*,storm_render::Surface*d,void*,int){if(failStep==3)return 1;d->tag=s->tag;return 0;}
 void GetViewport(storm_render::Viewport*v){*v={};}
 void SetTexture(int,storm_render::Texture*t){bound=t;textureCalls++;}
 void DrawPrimitiveUP(int,int,int,const void*v,size_t,const char*){uint32_t c;std::memcpy(&c,static_cast<const char*>(v)+16,4);colors.push_back(c);frameTags.push_back(bound?bound->surface.tag:-1);}
};
