#include <array>

#include <cmath>
#include <cstdio>
#include <cstring>
#include "render_device_api.hpp"
using CMatrix=storm_render::Matrix;
struct CVECTOR{float x=0,y=0,z=0;};
CVECTOR operator!(CVECTOR a){float n=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);return {a.x/n,a.y/n,a.z/n};}
namespace GEOS{struct PLANE{CVECTOR nrm;float d;};}
GEOS::PLANE ViewPlane[4]{};
struct Device{CMatrix view{};storm_render::Result GetTransform(storm_render::TransformSlot,CMatrix*out){*out=view;return storm_render::result::ok;}};
#define CHECKD3DERR(x) (x)
extern "C" bool StormMetalGetAuthoredWorld(void*,float*){return false;}
struct DX9RENDER{Device*renderDevice;CVECTOR vViewRelationPos{},vWordRelationPos{};void GetTransform(int32_t,storm_render::Matrix*);};
struct MODELR{void FindPlanes(const CMatrix&,const CMatrix&);};
void DX9RENDER::GetTransform(int32_t type, storm_render::Matrix *mtx)
{
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    if (type == storm_render::TransformWorld && mtx && StormMetalGetAuthoredWorld(renderDevice, &mtx->_11)) return;
#endif
    CHECKD3DERR(renderDevice->GetTransform(static_cast<storm_render::TransformSlot>(type), mtx));

    if (type == storm_render::TransformView)
    {
        mtx->_41 -= vViewRelationPos.x;
        mtx->_42 -= vViewRelationPos.y;
        mtx->_43 -= vViewRelationPos.z;
    }
    else if (type == storm_render::TransformWorld)
    {
        mtx->_41 -= vWordRelationPos.x;
        mtx->_42 -= vWordRelationPos.y;
        mtx->_43 -= vWordRelationPos.z;
    }
}void MODELR::FindPlanes(const CMatrix &view, const CMatrix &proj)
{
    CVECTOR v[4];
    // left
    v[0].x = proj.m[0][0];
    v[0].y = 0.0f;
    v[0].z = 1.0f;
    // right
    v[1].x = -proj.m[0][0];
    v[1].y = 0.0f;
    v[1].z = 1.0f;
    // top
    v[2].x = 0.0f;
    v[2].y = -proj.m[1][1];
    v[2].z = 1.0f;
    // bottom
    v[3].x = 0.0f;
    v[3].y = proj.m[1][1];
    v[3].z = 1.0f;
    v[0] = !v[0];
    v[1] = !v[1];
    v[2] = !v[2];
    v[3] = !v[3];

    CVECTOR pos;

    pos.x = -view.m[3][0] * view.m[0][0] - view.m[3][1] * view.m[0][1] - view.m[3][2] * view.m[0][2];
    pos.y = -view.m[3][0] * view.m[1][0] - view.m[3][1] * view.m[1][1] - view.m[3][2] * view.m[1][2];
    pos.z = -view.m[3][0] * view.m[2][0] - view.m[3][1] * view.m[2][1] - view.m[3][2] * view.m[2][2];

    ViewPlane[0].nrm.x = v[0].x * view.m[0][0] + v[0].y * view.m[0][1] + v[0].z * view.m[0][2];
    ViewPlane[0].nrm.y = v[0].x * view.m[1][0] + v[0].y * view.m[1][1] + v[0].z * view.m[1][2];
    ViewPlane[0].nrm.z = v[0].x * view.m[2][0] + v[0].y * view.m[2][1] + v[0].z * view.m[2][2];

    ViewPlane[1].nrm.x = v[1].x * view.m[0][0] + v[1].y * view.m[0][1] + v[1].z * view.m[0][2];
    ViewPlane[1].nrm.y = v[1].x * view.m[1][0] + v[1].y * view.m[1][1] + v[1].z * view.m[1][2];
    ViewPlane[1].nrm.z = v[1].x * view.m[2][0] + v[1].y * view.m[2][1] + v[1].z * view.m[2][2];

    ViewPlane[2].nrm.x = v[2].x * view.m[0][0] + v[2].y * view.m[0][1] + v[2].z * view.m[0][2];
    ViewPlane[2].nrm.y = v[2].x * view.m[1][0] + v[2].y * view.m[1][1] + v[2].z * view.m[1][2];
    ViewPlane[2].nrm.z = v[2].x * view.m[2][0] + v[2].y * view.m[2][1] + v[2].z * view.m[2][2];

    ViewPlane[3].nrm.x = v[3].x * view.m[0][0] + v[3].y * view.m[0][1] + v[3].z * view.m[0][2];
    ViewPlane[3].nrm.y = v[3].x * view.m[1][0] + v[3].y * view.m[1][1] + v[3].z * view.m[1][2];
    ViewPlane[3].nrm.z = v[3].x * view.m[2][0] + v[3].y * view.m[2][1] + v[3].z * view.m[2][2];

    ViewPlane[0].nrm.x = -ViewPlane[0].nrm.x;
    ViewPlane[0].nrm.y = -ViewPlane[0].nrm.y;
    ViewPlane[0].nrm.z = -ViewPlane[0].nrm.z;
    ViewPlane[1].nrm.x = -ViewPlane[1].nrm.x;
    ViewPlane[1].nrm.y = -ViewPlane[1].nrm.y;
    ViewPlane[1].nrm.z = -ViewPlane[1].nrm.z;
    ViewPlane[2].nrm.x = -ViewPlane[2].nrm.x;
    ViewPlane[2].nrm.y = -ViewPlane[2].nrm.y;
    ViewPlane[2].nrm.z = -ViewPlane[2].nrm.z;
    ViewPlane[3].nrm.x = -ViewPlane[3].nrm.x;
    ViewPlane[3].nrm.y = -ViewPlane[3].nrm.y;
    ViewPlane[3].nrm.z = -ViewPlane[3].nrm.z;

    ViewPlane[0].d = (pos.x * ViewPlane[0].nrm.x + pos.y * ViewPlane[0].nrm.y + pos.z * ViewPlane[0].nrm.z);
    ViewPlane[1].d = (pos.x * ViewPlane[1].nrm.x + pos.y * ViewPlane[1].nrm.y + pos.z * ViewPlane[1].nrm.z);
    ViewPlane[2].d = (pos.x * ViewPlane[2].nrm.x + pos.y * ViewPlane[2].nrm.y + pos.z * ViewPlane[2].nrm.z);
    ViewPlane[3].d = (pos.x * ViewPlane[3].nrm.x + pos.y * ViewPlane[3].nrm.y + pos.z * ViewPlane[3].nrm.z);
}
bool admit(CVECTOR cnt,float radius){
    int32_t p;
    for (p = 0; p < 4; p++)
    {
        const auto dist =
            cnt.x * ViewPlane[p].nrm.x + cnt.y * ViewPlane[p].nrm.y + cnt.z * ViewPlane[p].nrm.z - ViewPlane[p].d;
        if (dist > radius)
            break;
    }
return p==4;}

CMatrix identity(){CMatrix a{};for(int i=0;i<4;i++)a.m[i][i]=1;return a;}
std::array<float,4> transform(const CMatrix&a,const std::array<float,4>&p){std::array<float,4>q{};for(int j=0;j<4;j++)for(int i=0;i<4;i++)q[j]+=p[i]*a.m[i][j];return q;}
bool clips(const CMatrix&v,const CMatrix&p,CVECTOR x){auto q=transform(p,transform(v,{x.x,x.y,x.z,1}));return q[3]>0&&std::abs(q[0])<=q[3]&&std::abs(q[1])<=q[3]&&q[2]>=0&&q[2]<=q[3];}
int main(){
 Device device;DX9RENDER renderer{&device};MODELR model;int failures=0;
 auto run=[&](const char*name,CMatrix absolute,CMatrix projection,CVECTOR relation,CVECTOR point,bool clipExpected){
  device.view=absolute;renderer.vViewRelationPos=relation;CMatrix consumer{};
  renderer.GetTransform(storm_render::TransformView,&consumer);model.FindPlanes(consumer,projection);
  bool cpu=admit(point,.01f),gpu=clips(absolute,projection,point);
  std::printf("%s clip=%d model=%d consumerTranslation=(%.1f,%.1f,%.1f)\n",name,gpu,cpu,consumer._41,consumer._42,consumer._43);
  if(gpu!=clipExpected)return false;
  failures+=cpu!=gpu;return true;
 };
 CMatrix ortho=identity();ortho._11=ortho._22=2.f/96;ortho._33=1.f/240;
 CMatrix light=identity();light._41=-200;light._43=-280;
 if(!run("sun absolute view / zero camera relation",light,ortho,{},{200,0,400},true))return 2;
 if(!run("sun absolute view / retained main-camera relation",light,ortho,{200,0,400},{200,0,400},true))return 2;
 if(!run("orthographic near-slice caster",identity(),ortho,{},{40,0,.2f},true))return 2;
 CMatrix perspective{};perspective._11=perspective._22=1;perspective._33=1;perspective._34=1;perspective._43=-.1f;
 if(!run("perspective normal unrebased inside",identity(),perspective,{},{0,0,20},true))return 2;
 if(!run("perspective normal outside",identity(),perspective,{},{50,0,20},false))return 2;
 std::printf("CPU-cull versus depth-clip mismatches=%d\n",failures);return failures?1:0;
}
