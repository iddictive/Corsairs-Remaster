#include <SDL.h>
#include <d3d9.h>
#include <array>
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include "weather_surface_color.hpp"

static void check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
static std::vector<DWORD> shader(const std::string& path) {
    std::ifstream f(path,std::ios::binary|std::ios::ate);check(bool(f),path.c_str());auto bytes=f.tellg();
    check(bytes>0&&size_t(bytes)%4==0,"shader token length");std::vector<DWORD> data(size_t(bytes)/4);f.seekg(0);f.read((char*)data.data(),bytes);check(bool(f),"read original shader");return data;
}
static void paint(IDirect3DTexture9* t,DWORD color) {
    D3DLOCKED_RECT r{};check(SUCCEEDED(t->LockRect(0,&r,nullptr,0)),"texture lock");
    for(int y=0;y<4;y++)for(int x=0;x<4;x++)reinterpret_cast<DWORD*>((char*)r.pBits+y*r.Pitch)[x]=color;
    check(SUCCEEDED(t->UnlockRect(0)),"texture unlock");
}
static void paintVolume(IDirect3DVolumeTexture9* t,DWORD color) {
    D3DLOCKED_BOX b{};check(SUCCEEDED(t->LockBox(0,&b,nullptr,0)),"volume lock");
    for(int z=0;z<4;z++)for(int y=0;y<4;y++)for(int x=0;x<4;x++)reinterpret_cast<DWORD*>((char*)b.pBits+z*b.SlicePitch+y*b.RowPitch)[x]=color;
    check(SUCCEEDED(t->UnlockBox(0)),"volume unlock");
}
static void paintCube(IDirect3DCubeTexture9* t,bool distinct,DWORD uniformColor=0xff4080c0) {
    const DWORD colors[]={0xffee2222,0xff22ee22,0xff2244ee,0xffeebb22,0xffdd22dd,0xff22dddd};
    for(int face=0;face<6;face++){D3DLOCKED_RECT r{};check(SUCCEEDED(t->LockRect(D3DCUBEMAP_FACES(face),0,&r,nullptr,0)),"cube lock");for(int y=0;y<4;y++)for(int x=0;x<4;x++)reinterpret_cast<DWORD*>((char*)r.pBits+y*r.Pitch)[x]=distinct?colors[face]:uniformColor;check(SUCCEEDED(t->UnlockRect(D3DCUBEMAP_FACES(face),0)),"cube unlock");}
}
int main(int argc,char**argv) {
    check(argc==2,"usage: sea-probe RESOURCE/techniques directory");std::string techniques=argv[1];
    const char* modernEnv=std::getenv("STORM_METAL_MODERN_WATER");const bool modern=modernEnv&&std::strcmp(modernEnv,"1")==0;
    check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Original sea shader GPU fixture",0,0,128,128,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);check(w,SDL_GetError());
    auto*api=Direct3DCreate9(D3D_SDK_VERSION);check(api,"Direct3DCreate9");D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=128;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.BackBufferCount=1;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
    IDirect3DDevice9*d=nullptr;check(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,D3DCREATE_HARDWARE_VERTEXPROCESSING,&pp,&d)),"device");
    IDirect3DTexture9 *flat=nullptr,*reflection=nullptr,*sun=nullptr;
    for(auto**p:{&flat,&reflection,&sun})check(SUCCEEDED(d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,p,nullptr)),"2D texture");
    paint(flat,0xff204080);paint(reflection,0xff204060);paint(sun,0xff102030);
    IDirect3DVolumeTexture9*volume=nullptr;check(SUCCEEDED(d->CreateVolumeTexture(4,4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&volume,nullptr)),"volume texture");paintVolume(volume,0xff80ff80);
    IDirect3DCubeTexture9*cube=nullptr;check(SUCCEEDED(d->CreateCubeTexture(4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&cube,nullptr)),"cube texture");paintCube(cube,false);
    struct V {float x,y,z,nx,ny,nz,u,v;};const V vertices[]={{-.8f,.5f,-.8f,0,1,0,0,0},{.8f,.5f,-.8f,0,1,0,1,0},{0,.5f,.8f,0,1,0,.5f,1}};
    const WORD indices[]={0,1,2};IDirect3DVertexBuffer9*vb=nullptr;IDirect3DIndexBuffer9*ib=nullptr;void*bytes=nullptr;
    check(SUCCEEDED(d->CreateVertexBuffer(sizeof(vertices),0,0,D3DPOOL_MANAGED,&vb,nullptr)),"VB");check(SUCCEEDED(vb->Lock(0,0,&bytes,0)),"VB lock");memcpy(bytes,vertices,sizeof(vertices));vb->Unlock();
    check(SUCCEEDED(d->CreateIndexBuffer(sizeof(indices),0,D3DFMT_INDEX16,D3DPOOL_MANAGED,&ib,nullptr)),"IB");check(SUCCEEDED(ib->Lock(0,0,&bytes,0)),"IB lock");memcpy(bytes,indices,sizeof(indices));ib->Unlock();
    const D3DVERTEXELEMENT9 elements[]={{0,0,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_POSITION,0},{0,12,D3DDECLTYPE_FLOAT3,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_NORMAL,0},{0,24,D3DDECLTYPE_FLOAT2,D3DDECLMETHOD_DEFAULT,D3DDECLUSAGE_TEXCOORD,0},D3DDECL_END()};
    IDirect3DVertexDeclaration9*decl=nullptr;check(SUCCEEDED(d->CreateVertexDeclaration(elements,&decl)),"original sea declaration");d->SetVertexDeclaration(decl);d->SetStreamSource(0,vb,0,sizeof(V));d->SetIndices(ib);
    float c[256][4]{};auto set=[&](int n,float x,float y,float z,float a){c[n][0]=x;c[n][1]=y;c[n][2]=z;c[n][3]=a;};
    set(0,0,1,.5f,-.04f);set(1,2,-1,0,0);set(2,.25f,0,0,0);set(3,0,1,1,0);set(23,0,3,0,1);
    set(24,1,0,0,0);set(25,0,0,1,0);set(26,0,0,0,.5f);set(27,0,0,0,1);
    set(28,1,1,1,0);set(30,.5f,.5f,.5f,1);set(33,1,0,0,1);set(34,0,1,.5f,1);set(36,1,0,0,0);set(37,0,1,0,0);set(38,0,0,1,0);set(58,0,1,0,1);
    check(SUCCEEDED(d->SetVertexShaderConstantF(0,&c[0][0],256)),"VS constants");
    d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);
    // Explicit daylight weather. Unassigned ambient/fog are black, and must
    // no longer be mistaken for a lit sea by relying on an emission floor.
    d->SetRenderState(D3DRS_AMBIENT,0xff69645a);d->SetRenderState(D3DRS_FOGCOLOR,0xff9cc3e8);
    for(int stage=0;stage<8;stage++){d->SetSamplerState(stage,D3DSAMP_MINFILTER,D3DTEXF_LINEAR);d->SetSamplerState(stage,D3DSAMP_MAGFILTER,D3DTEXF_LINEAR);d->SetSamplerState(stage,D3DSAMP_MIPFILTER,D3DTEXF_NONE);d->SetSamplerState(stage,D3DSAMP_ADDRESSU,D3DTADDRESS_WRAP);d->SetSamplerState(stage,D3DSAMP_ADDRESSV,D3DTADDRESS_WRAP);d->SetSamplerState(stage,D3DSAMP_ADDRESSW,D3DTADDRESS_WRAP);d->SetTexture(stage,flat);}
    IDirect3DSurface9 *target=nullptr,*readback=nullptr;check(SUCCEEDED(d->GetRenderTarget(0,&target)),"target");check(SUCCEEDED(d->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr)),"readback");
    auto readPixel=[&](int x,int y){check(SUCCEEDED(d->GetRenderTargetData(target,readback)),"GPU readback");D3DLOCKED_RECT r{};check(SUCCEEDED(readback->LockRect(&r,nullptr,D3DLOCK_READONLY)),"readback lock");DWORD pixel;memcpy(&pixel,(char*)r.pBits+y*r.Pitch+x*4,4);readback->UnlockRect();return pixel;};
    DWORD background=0xff070b13;
    auto draw=[&](){check(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET,background,1,0)),"clear");check(SUCCEEDED(d->BeginScene()),"begin");HRESULT result=d->DrawIndexedPrimitive(D3DPT_TRIANGLELIST,0,0,3,0,1);check(SUCCEEDED(d->EndScene()),"end");return result;};
    auto near=[&](DWORD pixel,int red,int green,int blue,const char*name){std::printf("%s pixel=%08x expectedRGB=%d,%d,%d\n",name,pixel,red,green,blue);check(std::abs(int((pixel>>16)&255)-red)<=3&&std::abs(int((pixel>>8)&255)-green)<=3&&std::abs(int(pixel&255)-blue)<=3,name);};
    const char*names[]={"sea2","sea3","sea2foam","sea2sunroad"};
    for(int mode=0;mode<4;mode++){
        auto vsData=shader(techniques+"/weather/sea_"+names[mode]+"_vertex_shader.vso"),psData=shader(techniques+"/weather/sea_"+names[mode]+"_pixel_shader.pso");IDirect3DVertexShader9*vs=nullptr;IDirect3DPixelShader9*ps=nullptr;
        check(SUCCEEDED(d->CreateVertexShader(vsData.data(),&vs)),"original VS");check(SUCCEEDED(d->CreatePixelShader(psData.data(),&ps)),"original PS");d->SetVertexShader(vs);d->SetPixelShader(ps);
        d->SetTexture(0,mode==2?static_cast<IDirect3DBaseTexture9*>(flat):volume);d->SetTexture(1,reflection);d->SetTexture(2,volume);d->SetTexture(3,mode==1?static_cast<IDirect3DBaseTexture9*>(sun):cube);d->SetTexture(4,volume);
        check(SUCCEEDED(draw()),names[mode]);auto p=readPixel(64,64);if(!modern&&(mode==0||mode==1))near(p,32,64,96,names[mode]);
        if(mode==2){if(!modern)near(p,32,64,128,names[mode]);else{auto foam=storm_weather_surface::deriveFoam({156/255.f,195/255.f,232/255.f},{105/255.f,100/255.f,90/255.f});near(p,int(32*foam[0]),int(64*foam[1]),int(128*foam[2]),"weather foam RGB");check((p>>24)==128,"weather foam preserves authored alpha");}}
        if(mode==3){if(!modern)near(p,64,128,192,names[mode]);else{check((p&255)>((p>>8)&255)&&((p>>8)&255)>((p>>16)&255),"specular road preserves authored chroma");check((p&255)>0&&(p&255)<64,"overhead water applies low Fresnel reflectance to direct light");}}
        if(modern&&mode<2){
            auto brightness=[](DWORD value){return int(value&255)+int((value>>8)&255)+int((value>>16)&255);};
            check(brightness(p)>12&&brightness(p)<720,"modern water produces nonblack, nonclipped color");
            check((p>>24)>32&&(p>>24)<160,"modern overhead transmission remains bounded");
            // Solid environments isolate reflection response from texture direction.
            // Black/white inputs and a grazing camera must change actual GPU output.
            if(mode==0)paintCube(cube,false,0xff000000);else{paint(reflection,0xff000000);paint(sun,0xff000000);}
            check(SUCCEEDED(draw()),"modern black reflection draw");DWORD dark=readPixel(64,64);
            if(mode==0)paintCube(cube,false,0xffffffff);else paint(reflection,0xffffffff);
            check(SUCCEEDED(draw()),"modern white reflection draw");DWORD bright=readPixel(64,64);
            check(brightness(bright)>brightness(dark)+9,"modern water responds to actual reflection texture");
            set(23,0,.55f,10,1);check(SUCCEEDED(d->SetVertexShaderConstantF(23,c[23],1)),"grazing camera constant");
            check(SUCCEEDED(draw()),"modern grazing reflection draw");DWORD grazing=readPixel(64,64);
            check(brightness(grazing)>brightness(bright)+30,"grazing reflection stronger than overhead reflection");
            if(mode==0){
                paintCube(cube,true);check(SUCCEEDED(draw()),"modern forward cube face");DWORD forward=readPixel(64,64);
                set(23,0,.55f,-10,1);d->SetVertexShaderConstantF(23,c[23],1);check(SUCCEEDED(draw()),"modern reverse cube face");DWORD reverse=readPixel(64,64);
                check((forward&0xffffff)!=(reverse&0xffffff),"modern reflected view direction selects different cube faces");
            }
            set(23,0,3,0,1);d->SetVertexShaderConstantF(23,c[23],1);
            if(mode==0)paintCube(cube,false,0xff000000);else paint(reflection,0xff000000);
            set(30,0,0,0,1);d->SetVertexShaderConstantF(30,c[30],1);
            d->SetRenderState(D3DRS_AMBIENT,0xff1c1c23);d->SetRenderState(D3DRS_FOGCOLOR,0xff020202);set(29,5/255.f,10/255.f,20/255.f,1);d->SetVertexShaderConstantF(29,c[29],1);
            check(SUCCEEDED(draw()),"actual midnight palette draw");DWORD night=readPixel(64,64);
            check(brightness(night)<brightness(dark),"weather illumination darkens water body at night");
            d->SetRenderState(D3DRS_AMBIENT,0xff69645a);d->SetRenderState(D3DRS_FOGCOLOR,0xff9cc3e8);set(29,0,0,0,0);d->SetVertexShaderConstantF(29,c[29],1);
            set(30,.5f,.5f,.5f,1);d->SetVertexShaderConstantF(30,c[30],1);
            paintCube(cube,false);paint(reflection,0xff204060);paint(sun,0xff102030);
            // The game has already drawn underwater objects into this target.
            // Alternate their known color to prove actual destination compositing.
            d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_SRCALPHA);
            background=0xffc02020;check(SUCCEEDED(draw()),"underwater red background");DWORD underRed=readPixel(64,64);
            background=0xff2020c0;check(SUCCEEDED(draw()),"underwater blue background");DWORD underBlue=readPixel(64,64);
            check(int((underRed>>16)&255)>int((underBlue>>16)&255)+32&&int(underBlue&255)>int(underRed&255)+32,"underwater object color visible through overhead water");
            set(23,0,.55f,10,1);d->SetVertexShaderConstantF(23,c[23],1);
            background=0xffc02020;check(SUCCEEDED(draw()),"grazing red background");DWORD horizonRed=readPixel(64,64);
            background=0xff2020c0;check(SUCCEEDED(draw()),"grazing blue background");DWORD horizonBlue=readPixel(64,64);
            check(std::abs(int((horizonRed>>16)&255)-int((horizonBlue>>16)&255))<=5&&std::abs(int(horizonRed&255)-int(horizonBlue&255))<=5,"grazing water suppresses underwater color");
            set(23,0,3,0,1);d->SetVertexShaderConstantF(23,c[23],1);background=0xff070b13;d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
            check(SUCCEEDED(draw()),"restore baseline after transmission fixture");
            std::printf("transmission %s overhead %08x/%08x horizon %08x/%08x\n",names[mode],underRed,underBlue,horizonRed,horizonBlue);
            std::printf("modern %s baseline=%08x black=%08x white=%08x grazing=%08x night=%08x\n",names[mode],p,dark,bright,grazing,night);
        }
        check((readPixel(2,2)&0xffffff)==0x070b13,"indexed triangle preserves outside pixel");
        if(mode==3){if(modern){set(23,10,3,0,1);d->SetVertexShaderConstantF(23,c[23],1);}paintCube(cube,true);check(SUCCEEDED(draw()),"cube face draw");DWORD first=readPixel(64,64);paintVolume(volume,0xff8080ff);check(SUCCEEDED(draw()),"changed bump draw");DWORD second=readPixel(64,64);check((first&0xffffff)!=(second&0xffffff),"original volume normal changes selected cube face");std::printf("volume-to-cube direction %08x -> %08x\n",first,second);}
        vs->Release();ps->Release();
    }
    const DWORD unknown[]={0xfffe0101,0x00000001,0xc00f0000,0x90e40000,0x0000ffff};IDirect3DVertexShader9*unsupported=nullptr;check(FAILED(d->CreateVertexShader(unknown,&unsupported))&&unsupported==nullptr,"unknown shader rejected without substituting sea");
    d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetVertexDeclaration(nullptr);d->SetStreamSource(0,nullptr,0,0);d->SetIndices(nullptr);for(int i=0;i<8;i++)d->SetTexture(i,nullptr);
    readback->Release();target->Release();decl->Release();vb->Release();ib->Release();flat->Release();reflection->Release();sun->Release();volume->Release();cube->Release();d->Release();api->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS: original sea bytecode -> indexed Metal draw -> GPU pixels (4 passes + volume/cube direction + negative)");
}
