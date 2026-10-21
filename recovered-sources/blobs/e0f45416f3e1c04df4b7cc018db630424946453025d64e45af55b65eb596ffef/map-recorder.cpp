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
struct WdmMetalQuad3D{CVECTOR pos,right,up;float width,height;uint32_t color;std::array<float,4>uv;};void WdmMetalDrawQuad3D(VDX9RENDER*,const WdmMetalQuad3D&,const char*){}
#include <array>
#include <algorithm>
#include <cmath>

// WEATHER owns fronts and their calendar. This projects the same cells using
// authored cloud/rain art; the map retains camera, input and label ownership.
class WdmWeatherRegions final : public WdmRenderObject
{
    static constexpr unsigned puffs=18, drops=12;
    std::array<RS_RECT,storm_weather::Engine::frontCount*puffs> cloudRects{};
    std::array<RS_RECT,storm_weather::Engine::frontCount*drops> rainRects{};
    IDirect3DVertexDeclaration9 *declaration=nullptr;
    int32_t cloudTexture=-1, cloudLight=-1, rainTexture=-1, lightningTexture=-1;
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
        rainTexture=rs->TextureCreate("\\WorldMap\\rain.tga");
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
        for (int32_t texture:{cloudTexture,cloudLight,rainTexture,lightningTexture})
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
                if (rainTexture>=0) for (unsigned drop=0;drop<drops;++drop) {
                    const float angle=noise(front.id,100+drop*3)*6.2831853f;
                    const float radial=std::sqrt(noise(front.id,101+drop*3))*.48f;
                    const uint32_t alpha=uint32_t(130.f*opacity*front.rain);
                    if (!alpha) continue;
                    auto &r=rainRects[rainCount++];
                    const float phase=float(std::fmod(animationSeconds,.8)/.8)+noise(front.id,102+drop*3);
                    r.vPos=CVECTOR(front.x+std::sin(angle)*radial*front.radius,
                                  3.f+18.f*(1.f-(phase-std::floor(phase))),
                                  front.z+std::cos(angle)*radial*front.radius);
                    r.fSize=front.radius*.16f; r.fAngle=0.f; r.dwSubTexture=0;
                    r.dwColor=(alpha<<24)|0x00c8d8e8u;
                }
            }
            drawClouds(rs,cloudCount,cloudLight);
        }
        if (rainCount) {
            rs->TextureSet(0,rainTexture); rs->TextureSet(1,-1);
            rs->DrawRects(rainRects.data(),rainCount,"WdmRain",1,1);
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
const Batch& cloud(VDX9RENDER&rs){for(auto&b:rs.batches)if(b.technique=="WdmClouds")return b;assert(false);return rs.batches[0];}
bool sameGeometry(const RS_RECT&a,const RS_RECT&b){return a.vPos.x==b.vPos.x&&a.vPos.y==b.vPos.y&&a.vPos.z==b.vPos.z&&a.fSize==b.fSize&&a.fAngle==b.fAngle&&a.dwSubTexture==b.dwSubTexture;}
int main(){VDX9RENDER rs;Objects objects{&rs};wdmObjects=&objects;WdmWeatherRegions renderer;weather.fronts={{123,650,-180,300,.8f,0,0,.7f,50},{456,-100,200,400,.6f,0,0,.5f,60}};
renderer.LRender(&rs);Batch dry=cloud(rs);assert(dry.rects.size()==36);assert(rs.names[dry.texture1].find("cloudslight")!=std::string::npos);
for(float height:{50.f,500.f}){cameraHeight=height;for(auto pair:std::vector<std::pair<float,float>>{{.0299f,.0499f},{.0301f,.0501f},{.3f,.4f},{.8f,.9f},{1,1}}){for(auto&f:weather.fronts){f.rain=pair.first;f.storm=pair.second;}rs.batches.clear();renderer.LRender(&rs);auto&c=cloud(rs);assert(c.texture0==dry.texture0&&c.texture1==dry.texture1);assert(c.rects.size()==dry.rects.size());for(unsigned i=0;i<c.rects.size();++i){assert(sameGeometry(c.rects[i],dry.rects[i]));unsigned alpha=c.rects[i].dwColor>>24;assert(alpha<=unsigned(255*(height==50?.12f:.95f)));auto&f=weather.fronts[i/18];float radial=std::hypot(c.rects[i].vPos.x-f.x,c.rects[i].vPos.z-f.z)/f.radius;unsigned expected=unsigned(255*(height==50?.12f:.95f)*f.coverage*(1-radial*radial));assert(std::abs(int(alpha)-int(expected))<=1);unsigned red=(c.rects[i].dwColor>>16)&255;assert(red>=120&&red<=245);} } }
// Threshold crossing keeps cloud order/material/alpha; only <=1/255 shade quantization.
cameraHeight=500;for(auto&f:weather.fronts){f.rain=.0299f;f.storm=.0499f;}rs.batches.clear();renderer.LRender(&rs);Batch before=cloud(rs);for(auto&f:weather.fronts){f.rain=.0301f;f.storm=.0501f;}rs.batches.clear();renderer.LRender(&rs);Batch after=cloud(rs);for(unsigned i=0;i<before.rects.size();++i){assert(sameGeometry(before.rects[i],after.rects[i]));assert((before.rects[i].dwColor>>24)==(after.rects[i].dwColor>>24));int a=(before.rects[i].dwColor>>16)&255,b=(after.rects[i].dwColor>>16)&255;assert(std::abs(a-b)<=1);} 
std::printf("PASS actual map header:36 stable lobes; dry/rain/storm use same atlas/order/geometry; .03/.05 crossing shade<=1/255; .12/.95 alpha bounds\n");}
