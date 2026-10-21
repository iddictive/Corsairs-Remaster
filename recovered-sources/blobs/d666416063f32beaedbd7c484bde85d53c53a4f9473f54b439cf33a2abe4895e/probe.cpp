#include <array>

#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include "render_device_api.hpp"
using CMatrix=storm_render::Matrix;
CMatrix operator*(const CMatrix&a,const CMatrix&b){CMatrix q{};for(int i=0;i<4;i++)for(int j=0;j<4;j++)for(int k=0;k<4;k++)q.m[i][j]+=a.m[i][k]*b.m[k][j];return q;}
struct CVECTOR{float x=0,y=0,z=0;};
CVECTOR operator!(CVECTOR a){float n=std::sqrt(a.x*a.x+a.y*a.y+a.z*a.z);return {a.x/n,a.y/n,a.z/n};}
namespace GEOS{struct PLANE{CVECTOR nrm;float d;};}
GEOS::PLANE ViewPlane[4]{};
struct Device{struct{struct{bool inPass=false;}prepass;}landShadow;std::map<storm_render::TransformSlot,CMatrix>matrices;storm_render::Result GetTransform(storm_render::TransformSlot slot,CMatrix*out){*out=matrices.at(slot);return storm_render::result::ok;}};
#define CHECKD3DERR(x) (x)
extern "C" bool StormMetalGetAuthoredWorld(void*,float*){return false;}
struct DX9RENDER{Device*renderDevice;CVECTOR vViewRelationPos{},vWordRelationPos{};void*GetNativeDevice(){return renderDevice;}void GetTransform(int32_t,storm_render::Matrix*);void GetTransform(int32_t t,storm_render::Matrix&m){GetTransform(t,&m);}};
struct MODELR{DX9RENDER*rs;void FindPlanes(const CMatrix&,const CMatrix&);CMatrix consumer();};
extern "C" bool StormMetalShadowPassMatrices(void*pointer,float*outView,float*outProjection){
 if(!pointer||!outView||!outProjection)return false;auto*d=static_cast<Device*>(static_cast<Device*>(pointer));if(!d->landShadow.prepass.inPass)return false;
 std::memcpy(outView,&d->matrices.at(storm_render::TransformView),64);std::memcpy(outProjection,&d->matrices.at(storm_render::TransformProjection),64);return true;
}void DX9RENDER::GetTransform(int32_t type, storm_render::Matrix *mtx)
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
    if (proj.m[2][3] == 0.f && proj.m[3][3] != 0.f)
    {
        // Orthographic side planes are parallel, not a cone through the eye.
        // Row-vector clip bounds are -w<=x,y<=w; keep unit plane normals for
        // NODER's world-space bounding-sphere radius.
        const CMatrix clip = view * proj;
        const float signs[] = {-1.f, 1.f, 1.f, -1.f};
        for (int plane = 0; plane < 4; ++plane)
        {
            const int column = plane < 2 ? 0 : 1;
            auto &p = ViewPlane[plane];
            p.nrm.x = signs[plane] * clip.m[0][column] - clip.m[0][3];
            p.nrm.y = signs[plane] * clip.m[1][column] - clip.m[1][3];
            p.nrm.z = signs[plane] * clip.m[2][column] - clip.m[2][3];
            const float inverseLength = 1.f / std::sqrt(p.nrm.x*p.nrm.x + p.nrm.y*p.nrm.y + p.nrm.z*p.nrm.z);
            p.nrm.x *= inverseLength;
            p.nrm.y *= inverseLength;
            p.nrm.z *= inverseLength;
            p.d = (clip.m[3][3] - signs[plane] * clip.m[3][column]) * inverseLength;
        }
        return;
    }
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
CMatrix MODELR::consumer(){
    CMatrix view, proj;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    // Shadow depth consumes absolute light matrices. The normal renderer
    // getter would subtract the still-active main-camera relation a second time.
    if (!StormMetalShadowPassMatrices(rs->GetNativeDevice(), &view.m[0][0], &proj.m[0][0]))
#endif
    {
        rs->GetTransform(storm_render::TransformView, view);
        rs->GetTransform(storm_render::TransformProjection, proj);
    }
    FindPlanes(view, proj);
return view;}
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
 Device device;DX9RENDER renderer{&device};MODELR model{&renderer};int failures=0;
 auto run=[&](const char*name,CMatrix absolute,CMatrix projection,CVECTOR relation,CVECTOR point,bool clipExpected,bool shadow=true){
  device.matrices[storm_render::TransformView]=absolute;device.matrices[storm_render::TransformProjection]=projection;device.landShadow.prepass.inPass=shadow;renderer.vViewRelationPos=relation;
  CMatrix consumer=model.consumer(),expected=absolute;if(!shadow){expected._41-=relation.x;expected._42-=relation.y;expected._43-=relation.z;}
  bool cpu=admit(point,.01f),gpu=clips(expected,projection,point);
  std::printf("%s clip=%d model=%d consumerTranslation=(%.1f,%.1f,%.1f)\n",name,gpu,cpu,consumer._41,consumer._42,consumer._43);
  if(gpu!=clipExpected)return false;
  failures+=cpu!=gpu;return true;
 };
 CMatrix ortho=identity();ortho._11=ortho._22=2.f/96;ortho._33=1.f/240;
 CMatrix light=identity();light._41=-200;light._43=-280;
 if(!run("sun absolute view / zero camera relation",light,ortho,{},{200,0,400},true))return 2;
 if(!run("sun absolute view / retained main-camera relation",light,ortho,{200,0,400},{200,0,400},true))return 2;
 if(!run("orthographic near-slice caster",identity(),ortho,{},{40,0,.2f},true))return 2;
 if(!run("orthographic outside side plane",identity(),ortho,{},{60,0,120},false))return 2;
 CMatrix rotated=identity();rotated._11=rotated._33=0;rotated._13=1;rotated._31=-1;rotated._41=400;rotated._43=-80;
 if(!run("rotated translated orthographic focus",rotated,ortho,{13,5,-21},{200,0,400},true))return 2;
 if(!run("rotated orthographic near-slice caster",rotated,ortho,{13,5,-21},{80.2f,0,360},true))return 2;
 CMatrix perspective{};perspective._11=perspective._22=1;perspective._33=1;perspective._34=1;perspective._43=-.1f;
 if(!run("translated point-shadow perspective face",light,perspective,{200,0,400},{200,0,400},true))return 2;
 if(!run("perspective normal unrebased inside",identity(),perspective,{},{0,0,20},true,false))return 2;
 if(!run("perspective normal outside",identity(),perspective,{},{50,0,20},false,false))return 2;
 if(!run("shadow scope exit retains rebased main-view getter",identity(),perspective,{200,0,400},{200,0,420},true,false))return 2;
 CMatrix unused{};if(StormMetalShadowPassMatrices(&device,&unused._11,&unused._11)||StormMetalShadowPassMatrices(nullptr,&unused._11,&unused._11))return 3;
 std::printf("CPU-cull versus depth-clip mismatches=%d\n",failures);return failures?1:0;
}
