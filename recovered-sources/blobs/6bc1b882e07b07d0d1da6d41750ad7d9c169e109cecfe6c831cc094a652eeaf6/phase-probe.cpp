
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#define FAILED(value) ((value)<0)
constexpr int D3DRTYPE_TEXTURE=3,D3DFMT_X8R8G8B8=22,D3DFMT_A8R8G8B8=21,D3DLOCK_READONLY=16;
struct D3DSURFACE_DESC {uint32_t Width=64,Height=64;int Format=22;};
struct D3DLOCKED_RECT {int Pitch;void*pBits;};
struct Base {int GetType(){return 3;}};
struct IDirect3DTexture9:Base {D3DSURFACE_DESC desc;unsigned levels=5,locks=0,unlocks=0;std::vector<uint8_t>bytes;bool fail=false;
unsigned GetLevelCount(){return levels;}int GetLevelDesc(unsigned,D3DSURFACE_DESC*d){*d=desc;return 0;}
int LockRect(unsigned,D3DLOCKED_RECT*l,void*,int flags){if(flags!=16||fail)return -1;++locks;*l={int(desc.Width*4),bytes.data()};return 0;}
int UnlockRect(unsigned){++unlocks;return 0;}};
struct VDX9RENDER {IDirect3DTexture9*current=nullptr;Base*GetTextureFromID(int id){return id<0?nullptr:current;}};
void need(bool pass,const char*message){if(!pass){fprintf(stderr,"FAIL %s\n",message);exit(1);}}
uint32_t makeRGB(uint32_t r,uint32_t g,uint32_t b){return r<<16|g<<8|b;}
static float MoonReflectionGain(VDX9RENDER *renderer, int32_t textureId)
{
    auto *base = renderer->GetTextureFromID(textureId);
    if (!base || base->GetType() != D3DRTYPE_TEXTURE)
        return 1.f;
    auto *texture = static_cast<IDirect3DTexture9 *>(base);
    const auto levels = texture->GetLevelCount();
    if (!levels)
        return 1.f;
    const auto level = levels - 1;
    D3DSURFACE_DESC desc{};
    if (FAILED(texture->GetLevelDesc(level, &desc)) || desc.Width != 64 || desc.Height != 64 ||
        (desc.Format != D3DFMT_X8R8G8B8 && desc.Format != D3DFMT_A8R8G8B8))
        return 1.f;
    D3DLOCKED_RECT lock{};
    if (FAILED(texture->LockRect(level, &lock, nullptr, D3DLOCK_READONLY)))
        return 1.f;
    double energy = 0.;
    for (uint32_t y = 0; y < desc.Height; ++y)
        for (uint32_t x = 0; x < desc.Width; ++x)
        {
            const auto *pixel = static_cast<const uint8_t *>(lock.pBits) + y * lock.Pitch + x * 4;
            energy += .2126 * std::pow(pixel[2] / 255., 2.2) + .7152 * std::pow(pixel[1] / 255., 2.2) +
                      .0722 * std::pow(pixel[0] / 255., 2.2);
        }
    texture->UnlockRect(level);
    // Mean linear energy of the shipped Moon15 64x64 mip. Preserve its
    // reflection template at full moon; other phases use their actual pixels.
    constexpr double fullMoonEnergy = .05697321255454745;
    const auto relativeEnergy = std::clamp(energy / (64. * 64. * fullMoonEnergy), 0., 1.);
    // SUNGLOW writes display RGB; both modern sea paths decode it with pow(2.2).
    return static_cast<float>(std::pow(relativeEnergy, 1. / 2.2));
}


struct Owner {VDX9RENDER*pRS;int32_t iMoonTex=1,iMoonRadianceTexture=-1;float fMoonReflectionGain=1;std::string sMoonRadianceTexture;struct {std::string sMoonTexture;} Glow;
void refresh(){
    if (sMoonRadianceTexture != Glow.sMoonTexture || iMoonRadianceTexture != iMoonTex)
    {
        fMoonReflectionGain = MoonReflectionGain(pRS, iMoonTex);
        sMoonRadianceTexture = Glow.sMoonTexture;
        iMoonRadianceTexture = iMoonTex;
    }

}
uint32_t draw(bool bMoon){struct {uint32_t dwColor=0xff99ccff;} Reflection;struct {uint32_t dwColor;} r_spr{Reflection.dwColor};
    if (bMoon)
        r_spr.dwColor = (r_spr.dwColor & 0xFF000000) |
                        makeRGB(static_cast<uint32_t>(((Reflection.dwColor >> 16) & 255) * fMoonReflectionGain),
                                static_cast<uint32_t>(((Reflection.dwColor >> 8) & 255) * fMoonReflectionGain),
                                static_cast<uint32_t>((Reflection.dwColor & 255) * fMoonReflectionGain));

return r_spr.dwColor;}};
IDirect3DTexture9 load(const std::string&path){std::ifstream f(path,std::ios::binary);std::vector<uint8_t>b((std::istreambuf_iterator<char>(f)),{});need(b.size()>24,"authored texture present");auto u=[&](size_t o){return uint32_t(b[o])|uint32_t(b[o+1])<<8|uint32_t(b[o+2])<<16|uint32_t(b[o+3])<<24;};uint32_t w=u(4),h=u(8),levels=u(12);size_t offset=24;for(unsigned i=1;i<levels;i++){offset+=w*h*4;w=std::max(1u,w/2);h=std::max(1u,h/2);}IDirect3DTexture9 t;t.desc={w,h,int(u(16))};t.levels=levels;t.bytes.assign(b.begin()+offset,b.begin()+offset+w*h*4);return t;}
int main(int argc,char**argv){need(argc==2,"asset root argument");VDX9RENDER renderer;Owner o{&renderer};float gains[32]{};
for(int n=1;n<=31;n++){char name[30];snprintf(name,sizeof(name),"Moon%02d.tga.tx",n);auto texture=load(std::string(argv[1])+"/"+name);renderer.current=&texture;o.Glow.sMoonTexture=name;o.refresh();gains[n]=o.fMoonReflectionGain;need(std::isfinite(gains[n])&&gains[n]>=0&&gains[n]<=1,"bounded phase gain");unsigned before=texture.locks;o.refresh();need(texture.locks==before,"same texture skips decode");need(texture.unlocks==texture.locks,"read lock balanced");need(o.draw(false)==0xff99ccff,"sun preserves RGB and alpha");need((o.draw(true)>>24)==0xff,"moon preserves alpha");printf("phase %02d display %.9f linear %.9f\n",n,gains[n],std::pow(gains[n],2.2));}
need(gains[15]==1.f,"fullmoon exact identity");need(gains[31]<.011f&&gains[31]>.01f,"dark authored phase below 1.1 percent display");need(gains[8]>.63f&&gains[8]<.65f,"partial authored phase has partial radiance");
auto full=load(std::string(argv[1])+"/Moon15.tga.tx");renderer.current=&full;o.Glow.sMoonTexture="Moon15";o.refresh();need(o.draw(true)==o.draw(false),"fullmoon preserves arbitrary RGB byte for byte");auto locked=full.locks;++o.iMoonTex;o.refresh();need(full.locks==locked+1,"texture identity change invalidates cached energy");
for(int scenario=0;scenario<4;scenario++){auto bad=full;if(scenario==0)bad.desc.Width=32;if(scenario==1)bad.desc.Format=0;if(scenario==2)bad.levels=0;if(scenario==3)bad.fail=true;renderer.current=&bad;++o.iMoonTex;o.refresh();need(o.fMoonReflectionGain==1.f,"incompatible mip/format/lock keeps legacy reflection");}
renderer.current=nullptr;o.iMoonTex=-1;o.refresh();need(o.fMoonReflectionGain==1.f,"missing texture keeps legacy reflection");
puts("PASS actual helper/cache/color code: 31 authored phases, fullmoon identity, sun/alpha preservation, texture invalidation, mip/format/lock/missing negatives");}
