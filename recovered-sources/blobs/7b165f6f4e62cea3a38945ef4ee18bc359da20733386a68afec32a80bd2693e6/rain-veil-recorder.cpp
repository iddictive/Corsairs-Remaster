#include "weather-engine-snapshot.hpp"
#include <vector>
#include <string>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <map>
float cameraHeight=50;
struct CVECTOR {float x,y,z;CVECTOR(float f=0):x(f),y(f),z(f){} CVECTOR(float a,float b,float c):x(a),y(b),z(c){}};
struct CMatrix {float matrix[16]{};void Transposition(){} void MulToInv(CVECTOR,CVECTOR&v){v=CVECTOR(0,cameraHeight,0);}CVECTOR Vx(){return CVECTOR(1,0,0);}};
struct RS_RECT {CVECTOR vPos;float fSize=0,fAngle=0;uint32_t dwSubTexture=0,dwColor=0;};
struct IDirect3DVertexDeclaration9{void Release(){}};
struct D3DVERTEXELEMENT9{int a,b,c,d,e,f;};
#define D3DDECL_END() D3DVERTEXELEMENT9{0,0,0,0,0,0}
enum{D3DDECLTYPE_FLOAT3,D3DDECLTYPE_D3DCOLOR,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,D3DDECLUSAGE_COLOR,D3DDECLUSAGE_TEXCOORD,D3DTS_VIEW,D3DTS_PROJECTION};
struct Batch{std::string technique;int texture0,texture1;std::vector<RS_RECT> rects;};
struct VDX9RENDER {std::vector<Batch> batches;int t[2]{-1,-1},next=0;std::map<int,std::string> names;IDirect3DVertexDeclaration9 d;
int TextureCreate(const char*s){names[next]=s;return next++;}void TextureRelease(int){}void CreateVertexDeclaration(const D3DVERTEXELEMENT9*,IDirect3DVertexDeclaration9**p){*p=&d;}void SetVertexDeclaration(IDirect3DVertexDeclaration9*){}void TextureSet(int a,int b){t[a]=b;}void GetTransform(int,CMatrix&){}void SetVertexShaderConstantF(int,const CMatrix&,int){}void DrawRects(RS_RECT*r,unsigned n,const char*s,int,int){batches.push_back({s,t[0],t[1],{r,r+n}});}};
struct WdmRenderObject{virtual ~WdmRenderObject()=default;virtual void Update(float){}virtual void LRender(VDX9RENDER*){}};
struct WEATHER_BASE{std::vector<storm_weather::Front> fronts;bool GetWeatherFront(unsigned i,storm_weather::Front&f){if(i>=fronts.size())return false;f=fronts[i];return true;}} weather;
struct Core{int GetEntityId(const char*){return 1;}void*GetEntityPointer(int){return &weather;}}core;
struct Objects{VDX9RENDER*rs;}; Objects*wdmObjects;
struct WdmMetalQuad3D{CVECTOR pos,right,up;float width,height;uint32_t color;std::array<float,4>uv;};struct QuadCall{WdmMetalQuad3D quad;std::string technique;};std::vector<QuadCall> quadCalls;void WdmMetalDrawQuad3D(VDX9RENDER*,const WdmMetalQuad3D&q,const char*t){quadCalls.push_back({q,t});}
#include <array>
#include <algorithm>
#include <cmath>

// WEATHER owns fronts and their calendar. This projects the same cells using
// authored cloud/rain art; the map retains camera, input and label ownership.
class WdmWeatherRegions final : public WdmRenderObject
{
    static constexpr unsigned puffs=18;
    std::array<RS_RECT,storm_weather::Engine::frontCount*puffs> cloudRects{};
    std::array<WdmMetalQuad3D,storm_weather::Engine::frontCount> rainQuads{};
    IDirect3DVertexDeclaration9 *declaration=nullptr;
    int32_t cloudTexture=-1, cloudLight=-1, lightningTexture=-1;
    double animationSeconds=0;
    static float noise(uint32_t id,unsigned index)
    {
        uint32_t v=id^(index*0x9e3779b9u); v^=v>>16; v*=0x7feb352du;
        v^=v>>15; v*=0x846ca68bu; v^=v>>16;
        return float(v>>8)/16777216.f;
    }
    void drawClouds(VDX9RENDER *rs,unsigned count,int32_t light)
    {
        if (!count || light<0) return;
        rs->SetVertexDeclaration(declaration);
        rs->TextureSet(0,cloudTexture); rs->TextureSet(1,light);
        CMatrix view, constants;
        rs->GetTransform(D3DTS_VIEW,view); view.Transposition();
        rs->GetTransform(D3DTS_PROJECTION,constants);
        rs->SetVertexShaderConstantF(0,constants,4);
        constants.matrix[0]=view.matrix[1]; constants.matrix[1]=view.matrix[5];
        constants.matrix[2]=view.matrix[9]; constants.matrix[3]=view.matrix[13]-24.f;
        constants.matrix[4]=40.f*1.4f; constants.matrix[5]=0.f;
        constants.matrix[6]=200.f*1.4f; constants.matrix[7]=.5f;
        constants.matrix[8]=.8f; constants.matrix[9]=.6f;
        constants.matrix[10]=1.f; constants.matrix[11]=1.f;
        rs->SetVertexShaderConstantF(4,constants,3);
        rs->DrawRects(cloudRects.data(),count,"WdmClouds",2,2);
    }
public:
    void Update(float deltaSeconds) override
    {
        if (std::isfinite(deltaSeconds) && deltaSeconds>0)
            animationSeconds+=deltaSeconds;
    }
    WdmWeatherRegions()
    {
        auto *rs=wdmObjects->rs;
        cloudTexture=rs->TextureCreate("\\WorldMap\\clouds.tga");
        cloudLight=rs->TextureCreate("\\WorldMap\\cloudslight.tga");
        lightningTexture=rs->TextureCreate("\\WorldMap\\lightnings.tga");
        const D3DVERTEXELEMENT9 elements[]={
            {0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},
            {0,12,D3DDECLTYPE_D3DCOLOR,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_COLOR,0},
            {0,16,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
        rs->CreateVertexDeclaration(elements,&declaration);
    }
    ~WdmWeatherRegions() override
    {
        auto *rs=wdmObjects->rs;
        for (int32_t texture:{cloudTexture,cloudLight,lightningTexture})
            if (texture>=0) rs->TextureRelease(texture);
        if (declaration) declaration->Release();
    }
    void LRender(VDX9RENDER *rs) override
    {
        const auto id=core.GetEntityId("weather");
        if (!id || !rs || !wdmObjects || !declaration || cloudTexture<0) return;
        auto *weather=static_cast<WEATHER_BASE *>(core.GetEntityPointer(id));
        if (!weather) return;
        CMatrix view; rs->GetTransform(D3DTS_VIEW,view);
        CVECTOR camera; view.MulToInv(CVECTOR(0.f),camera);
        // The native Tab overview rises to500; ship view starts at50.
        float zoom=std::clamp((camera.y-60.f)/360.f,0.f,1.f);
        zoom=zoom*zoom*(3.f-2.f*zoom);
        const float opacity=.12f+.83f*zoom;
        unsigned rainCount=0, cloudCount=0;
        {
            for (unsigned slot=0;slot<storm_weather::Engine::frontCount;++slot) {
                storm_weather::Front front;
                if (!weather->GetWeatherFront(slot,front) || front.coverage<=.01f) continue;
                for (unsigned puff=0;puff<puffs;++puff) {
                    const float angle=noise(front.id,puff*4)*6.2831853f;
                    const float radial=std::sqrt(noise(front.id,puff*4+1))*.70f;
                    auto &r=cloudRects[cloudCount++];
                    r.vPos=CVECTOR(front.x+std::sin(angle)*radial*front.radius,24.f,
                                  front.z+std::cos(angle)*radial*front.radius);
                    r.fSize=front.radius*(.26f+.10f*noise(front.id,puff*4+2));
                    r.fAngle=noise(front.id,puff*4+3)*6.2831853f;
                    r.dwSubTexture=puff%4;
                    const uint32_t alpha=uint32_t(255.f*opacity*front.coverage*(1.f-radial*radial));
                    // The same lobes and light atlas survive the whole cloud's
                    // lifecycle. Its wet core darkens continuously; changing
                    // precipitation never swaps material or alpha-sort order.
                    const float core=1.f-radial*radial;
                    const uint32_t shade=uint32_t(245.f-(95.f*front.rain+30.f*front.storm)*core);
                    const uint32_t size=uint32_t(255.f*std::clamp((r.fSize-40.f)/200.f,0.f,1.f));
                    r.dwColor=(alpha<<24)|(shade<<16)|size;
                }
                const uint32_t rainAlpha=uint32_t(160.f*opacity*front.rain);
                if (rainAlpha) {
                    // One world-space veil shares the sampled rain footprint.
                    // No restarting/falling sprite cards or grain texture.
                    rainQuads[rainCount++]={{front.x,6.f,front.z},{1.f,0.f,0.f},{0.f,0.f,1.f},
                        front.radius,front.radius,(rainAlpha<<24)|0x00687078u,{0.f,0.f,1.f,1.f}};
                }
            }
            for (unsigned index=0;index<rainCount;++index)
                WdmMetalDrawQuad3D(rs,rainQuads[index],"WdmRain");
            drawClouds(rs,cloudCount,cloudLight);
        }
        if (lightningTexture>=0) for (unsigned slot=0;slot<storm_weather::Engine::frontCount;++slot) {
            storm_weather::Front front;
            if (!weather->GetWeatherFront(slot,front) || front.storm<.5f) continue;
            const float phase=float(std::fmod(animationSeconds+noise(front.id,190)*11.,11.));
            if (phase>.18f) continue;
            rs->TextureSet(0,lightningTexture); rs->TextureSet(1,-1);
            const uint32_t color=(uint32_t(200.f*opacity*front.storm)<<24)|0x00e0e8ffu;
            CMatrix cameraView; rs->GetTransform(D3DTS_VIEW,cameraView); cameraView.Transposition();
            const WdmMetalQuad3D bolt{{front.x,12.f,front.z},cameraView.Vx(),
                {0.f,-1.f,0.f},6.f,12.f,color,{float(front.id%4)*.25f,.01f,.25f,.98f}};
            WdmMetalDrawQuad3D(rs,bolt,"WdmLightning");
        }
    }
};
std::vector<WdmMetalQuad3D> rain(){std::vector<WdmMetalQuad3D>out;for(auto&c:quadCalls)if(c.technique=="WdmRain")out.push_back(c.quad);return out;}
const Batch&cloud(VDX9RENDER&rs){for(auto&b:rs.batches)if(b.technique=="WdmClouds")return b;assert(false);return rs.batches[0];}
void reset(VDX9RENDER&rs){rs.batches.clear();quadCalls.clear();}
bool same(const CVECTOR&a,const CVECTOR&b){return a.x==b.x&&a.y==b.y&&a.z==b.z;}
float smooth(float t){t=std::clamp(t,0.f,1.f);return t*t*(3-2*t);}float shaderKernel(float u,float v){float qx=u*2-1,qz=v*2-1;float k=smooth(1-qx*qx-qz*qz);return k*k;}
int main(){VDX9RENDER rs;Objects obj{&rs};wdmObjects=&obj;WdmWeatherRegions renderer;
for(auto&entry:rs.names)assert(entry.second.find("rain.tga")==std::string::npos);
for(unsigned i=0;i<12;++i)weather.fronts.push_back({100+i,float(i*20),float(i*30),200.f+i,.8f,0,0,.7f,50});
renderer.LRender(&rs);Batch dry=cloud(rs);assert(dry.rects.size()==216&&rain().empty());
for(float h:{50.f,500.f}){cameraHeight=h;for(float intensity:{.03f,.1f,.5f,1.f}){for(auto&f:weather.fronts){f.rain=intensity;f.storm=intensity;}reset(rs);renderer.LRender(&rs);auto quads=rain();assert(quads.size()==(unsigned(160*(h==50?.12f:.95f)*intensity)?12:0));auto current=cloud(rs);assert(current.texture1==dry.texture1&&current.rects.size()==216);for(unsigned i=0;i<216;++i){assert(same(current.rects[i].vPos,dry.rects[i].vPos));assert(current.rects[i].fSize==dry.rects[i].fSize&&current.rects[i].fAngle==dry.rects[i].fAngle&&current.rects[i].dwSubTexture==dry.rects[i].dwSubTexture);}if(quads.empty())continue;for(unsigned i=0;i<12;++i){auto&q=quads[i];auto&f=weather.fronts[i];assert(q.pos.x==f.x&&q.pos.y==6&&q.pos.z==f.z);assert(same(q.right,CVECTOR(1,0,0))&&same(q.up,CVECTOR(0,0,1)));assert((q.width==f.radius&&q.height==f.radius&&q.uv==std::array<float,4>({0,0,1,1})));unsigned a=q.color>>24;assert(a==unsigned(160*(h==50?.12f:.95f)*intensity));}
for(float dt:{.1f,.4f,.8f,10.f}){renderer.Update(dt);reset(rs);renderer.LRender(&rs);auto after=rain();assert(after.size()==quads.size());for(unsigned i=0;i<12;++i){assert(same(after[i].pos,quads[i].pos)&&same(after[i].right,quads[i].right)&&same(after[i].up,quads[i].up));assert(after[i].width==quads[i].width&&after[i].height==quads[i].height&&after[i].color==quads[i].color&&after[i].uv==quads[i].uv);}}
// World X/Z -> actual quad UV and shader radial profile equals field's squared kernel.
for(unsigned i=0;i<12;++i){auto&q=quads[i];auto&f=weather.fronts[i];for(int a=-12;a<=12;++a)for(int b=-12;b<=12;++b){float x=f.x+f.radius*a/10,z=f.z+f.radius*b/10;float dx=(x-f.x)/f.radius,dz=(z-f.z)/f.radius;float field=smooth(1-dx*dx-dz*dz);float u=.5f+.5f*(x-q.pos.x)/q.width,v=.5f+.5f*(z-q.pos.z)/q.height;assert(std::abs(shaderKernel(u,v)-field*field)<1e-6f);}}
}}
for(float h:{50.f,500.f}){cameraHeight=h;unsigned previousAlpha=0;for(unsigned step=0;step<=1000;++step){for(auto&f:weather.fronts){f.rain=step/1000.f;f.storm=0;}reset(rs);renderer.LRender(&rs);auto emitted=rain();unsigned alpha=emitted.empty()?0:emitted[0].color>>24;assert(alpha>=previousAlpha&&alpha-previousAlpha<=1);assert(alpha<=152);previousAlpha=alpha;}}
for(auto&f:weather.fronts){f.rain=0;f.storm=0;}reset(rs);renderer.LRender(&rs);assert(rain().empty());for(auto&b:rs.batches)assert(b.technique!="WdmRain");
std::puts("PASS actual rain veil header: <=12 XZ quads at y6/radius, no rain texture/cards, stable across animation, exact squared field kernel, dry zero; cloud material/lobes unchanged");}
