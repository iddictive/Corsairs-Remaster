// Baseline root GPU run: reflection-only max1; refraction max211, 120 channel jumps.
// Rejected weighted-tap candidate: refraction max144, still 120 jumps; depth negatives passed.
// Candidate was reverted. Stable-scene counters below distinguish a shader-only
// discontinuity from the upstream foreground edge changing rasterized coverage.
#include <SDL.h>
#include <algorithm>
#include <initializer_list>
#include <d3d9.h>
#include <vector>
#include <fstream>
#include <string>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void require(bool b,const char*s){if(!b){fprintf(stderr,"FAIL %s\n",s);exit(1);}}
static std::vector<DWORD> load(const std::string&p){std::ifstream f(p,std::ios::binary|std::ios::ate);require(bool(f),p.c_str());size_t n=f.tellg();std::vector<DWORD>v(n/4);f.seekg(0);f.read((char*)v.data(),n);return v;}
int main(int argc,char**argv){
 require(argc==2,"water-camera-probe techniques");setenv("STORM_METAL_MODERN_WATER","1",1);SDL_Init(SDL_INIT_VIDEO);auto*w=SDL_CreateWindow("Fixed-wave camera sweep",0,0,128,128,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);require(w,"window");auto*a=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS p{};p.Windowed=TRUE;p.BackBufferWidth=p.BackBufferHeight=128;p.BackBufferFormat=D3DFMT_A8R8G8B8;p.hDeviceWindow=w;p.SwapEffect=D3DSWAPEFFECT_DISCARD;IDirect3DDevice9*d=nullptr;require(SUCCEEDED(a->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&p,&d)),"device");
 D3DMATRIX identity{};identity._11=identity._22=identity._33=identity._44=1;D3DMATRIX projection{};projection._11=projection._22=1;projection._33=100.f/99.f;projection._34=1;projection._43=-100.f/99.f;d->SetTransform(D3DTS_WORLD,&identity);d->SetTransform(D3DTS_VIEW,&identity);d->SetTransform(D3DTS_PROJECTION,&projection);
 IDirect3DTexture9*bump=nullptr;d->CreateTexture(4,4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&bump,nullptr);auto paintBump=[&](DWORD value){D3DLOCKED_RECT l{};bump->LockRect(0,&l,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=value;bump->UnlockRect(0);};paintBump(0xff80ff80);
 IDirect3DCubeTexture9*cube=nullptr;d->CreateCubeTexture(4,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&cube,nullptr);auto paintCube=[&](DWORD value){for(int f=0;f<6;f++){D3DLOCKED_RECT l{};cube->LockRect(D3DCUBEMAP_FACES(f),0,&l,nullptr,0);for(int y=0;y<4;y++)for(int x=0;x<4;x++)((DWORD*)((char*)l.pBits+y*l.Pitch))[x]=value;cube->UnlockRect(D3DCUBEMAP_FACES(f),0);}};paintCube(0xff000000);
 auto vc=load(std::string(argv[1])+"/weather/sea_sea2_vertex_shader.vso"),pc=load(std::string(argv[1])+"/weather/sea_sea2_pixel_shader.pso");IDirect3DVertexShader9*vs=nullptr;IDirect3DPixelShader9*ps=nullptr;d->CreateVertexShader(vc.data(),&vs);d->CreatePixelShader(pc.data(),&ps);
 float c[256][4]{};auto set=[&](int i,float x,float y,float z,float w){c[i][0]=x;c[i][1]=y;c[i][2]=z;c[i][3]=w;};set(0,0,1,.5,-.04);set(1,2,-1,0,0);set(23,0,0,0,1);set(24,1,0,0,0);set(25,0,1,0,0);set(26,0,0,projection._33,projection._43);set(27,0,0,1,0);set(28,1,1,1,0);set(29,0,0,0,1);set(30,0,0,0,1);set(33,1,0,0,1);set(34,.75,1,.5,1);set(36,1,0,0,0);set(37,0,1,0,0);set(38,0,0,1,0);set(58,0,1,0,1);d->SetVertexShaderConstantF(0,&c[0][0],256);
 IDirect3DSurface9*target=nullptr,*read=nullptr,*originalDepth=nullptr;d->GetRenderTarget(0,&target);d->GetDepthStencilSurface(&originalDepth);d->CreateOffscreenPlainSurface(128,128,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&read,nullptr);
 auto pixel=[&](int x=64,int y=64){require(SUCCEEDED(d->GetRenderTargetData(target,read)),"readback");D3DLOCKED_RECT l{};read->LockRect(&l,nullptr,D3DLOCK_READONLY);DWORD v;memcpy(&v,(char*)l.pBits+y*l.Pitch+x*4,4);read->UnlockRect();return v;};
 struct V{float x,y,z;DWORD color;};struct S{float x,y,z,nx,ny,nz,u,v;};S sea[]={{-8,-8,4,0,0,-1,0,0},{8,-8,4,0,0,-1,1,0},{0,8,4,0,0,-1,.5,1}};
 auto geometry=[&](float depth,DWORD color,float left=-2,float right=2){V v[]={{left*depth,-2*depth,depth,color},{right*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,-2*depth,depth,color},{right*depth,2*depth,depth,color},{left*depth,2*depth,depth,color}};d->SetVertexShader(nullptr);d->SetPixelShader(nullptr);d->SetVertexDeclaration(nullptr);d->SetFVF(D3DFVF_XYZ|D3DFVF_DIFFUSE);d->SetTexture(0,nullptr);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);require(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,2,v,sizeof(V))),"background geometry");};
 auto water=[&](){d->SetVertexShader(vs);d->SetPixelShader(ps);d->SetTexture(0,bump);d->SetTexture(3,cube);d->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);d->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_ONE);d->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_SRCALPHA);require(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLELIST,1,sea,sizeof(S))),"water draw");};
 auto start=[&](){require(SUCCEEDED(d->Present(nullptr,nullptr,nullptr,nullptr)),"frame boundary");d->SetRenderState(D3DRS_ZENABLE,TRUE);d->SetRenderState(D3DRS_ZWRITEENABLE,TRUE);d->SetRenderState(D3DRS_ZFUNC,D3DCMP_LESSEQUAL);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_FOGENABLE,FALSE);require(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER,0xff000000,1,0)),"fresh color+depth");d->BeginScene();};
 auto finish=[&](){d->EndScene();return pixel();};

 auto framePixels=[&](){require(SUCCEEDED(d->GetRenderTargetData(target,read)),"frame readback");D3DLOCKED_RECT r{};read->LockRect(&r,nullptr,D3DLOCK_READONLY);std::vector<DWORD> out(128*128);for(int y=0;y<128;y++)memcpy(out.data()+128*y,(char*)r.pBits+r.Pitch*y,512);read->UnlockRect();return out;};
 auto sweep=[&](bool refract,bool foreground=true){
  const char* label=refract?(foreground?"refraction-edge":"refraction-no-foreground"):"reflection-only";
  paintBump(0xff80ffff);paintCube(refract?0xff000000:0xff90a0b0);
  std::vector<DWORD> previous,previousScene;int maximum=0,stableMaximum=0,stableFrames=0,jumpFrames=0;double sum=0;int changed=0;
  for(int frame=0;frame<81;frame++){
   float cameraX=-.025f+frame*.000625f;D3DMATRIX view=identity;view._41=-cameraX;d->SetTransform(D3DTS_VIEW,&view);
   set(23,cameraX,0,0,1);set(24,1,0,0,-cameraX);d->SetVertexShaderConstantF(0,&c[0][0],256);
   start();
   // Stationary underwater striped wall and stationary above-water silhouette.
   if(refract){for(int i=0;i<32;i++)geometry(8,i%2?0xff202020:0xffffffff,-1+i*.0625f,-1+(i+1)*.0625f);if(foreground)geometry(2,0xffff0000,-1,.1875f);}
   else geometry(8,0xff000000);
   auto sceneBeforeWater=framePixels();bool stableScene=!previousScene.empty()&&sceneBeforeWater==previousScene;
   if(stableScene)stableFrames++;
   if(!refract)d->SetDepthStencilSurface(nullptr);
   water();d->EndScene();auto current=framePixels();if(!refract)d->SetDepthStencilSurface(originalDepth);
   int frameMaximum=0;
   if(!previous.empty())for(int y=54;y<74;y++)for(int x=79;x<92;x++)for(int shift:{0,8,16}){
    int delta=abs(int(current[y*128+x]>>shift&255)-int(previous[y*128+x]>>shift&255));maximum=std::max(maximum,delta);frameMaximum=std::max(frameMaximum,delta);if(stableScene)stableMaximum=std::max(stableMaximum,delta);sum+=delta;if(delta>24)changed++;
   }
   if(frameMaximum>24){
    int changedInputs=0;for(size_t i=0;i<sceneBeforeWater.size();i++)if(sceneBeforeWater[i]!=previousScene[i])changedInputs++;
    printf("camera event %s frame %d x %.6f max %d changed pre-water pixels %d\n",label,frame,cameraX,frameMaximum,changedInputs);jumpFrames++;
   }
   previous=std::move(current);previousScene=std::move(sceneBeforeWater);
  }
  printf("camera sweep %s max delta %d sum %.0f jumps %d\n",label,maximum,sum,changed);
  printf("camera stable-scene %s frames %d max delta %d\n",label,stableFrames,stableMaximum);
  printf("camera jump-event frames %d\n",jumpFrames);
  return maximum;
 };
 int reflected=sweep(false),unobstructed=sweep(true,false),refracted=sweep(true);
 require(reflected<=3,"fixed uniform cubemap and fixed waves remain continuous under camera translation");
 require(unobstructed<=24,"fixed underwater plane without foreground remains continuous");
 // Identical camera and phase replay must be exactly deterministic.
 auto still=[&](){start();geometry(8,0xff406080);water();d->EndScene();return framePixels();};require(still()==still(),"stationary camera/waves exact replay");
 require(refracted<=24,"subpixel camera movement does not switch the entire refraction ray");
 originalDepth->Release();target->Release();read->Release();bump->Release();cube->Release();vs->Release();ps->Release();d->Release();a->Release();SDL_DestroyWindow(w);SDL_Quit();puts("PASS fixed-wave camera temporal fixture");}
