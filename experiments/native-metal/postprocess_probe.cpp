#include <SDL.h>
#include <d3d9.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <fstream>
#include <string>
#include <vector>
static void check(bool ok,const char* what){if(!ok){std::fprintf(stderr,"FAIL: %s\n",what);std::exit(1);}}
int main(int argc,char**argv){
 check(argc==2,"usage: postprocess-probe RESOURCE/techniques");
 std::ifstream f(std::string(argv[1])+"/postprocess/postprocess_shader.pso",std::ios::binary|std::ios::ate);check(bool(f),"original shader");auto size=f.tellg();check(size>0&&size%4==0,"shader size");std::vector<DWORD> code(size_t(size)/4);f.seekg(0);f.read((char*)code.data(),size);
 check(SDL_Init(SDL_INIT_VIDEO)==0,SDL_GetError());auto*w=SDL_CreateWindow("Postprocess and FFP fixture",0,0,64,64,SDL_WINDOW_METAL|SDL_WINDOW_HIDDEN);check(w,SDL_GetError());
 auto*api=Direct3DCreate9(D3D_SDK_VERSION);D3DPRESENT_PARAMETERS pp{};pp.BackBufferWidth=pp.BackBufferHeight=64;pp.BackBufferCount=1;pp.BackBufferFormat=D3DFMT_A8R8G8B8;pp.Windowed=TRUE;pp.hDeviceWindow=w;pp.SwapEffect=D3DSWAPEFFECT_DISCARD;
 IDirect3DDevice9*d=nullptr;check(SUCCEEDED(api->CreateDevice(0,D3DDEVTYPE_HAL,w,0,&pp,&d)),"device");
 IDirect3DPixelShader9*ps=nullptr;check(SUCCEEDED(d->CreatePixelShader(code.data(),&ps))&&ps,"original postprocess accepted");
 IDirect3DTexture9*textures[4]{};DWORD colors[]={0xff204060,0xff406080,0xff6080a0,0xff80a0c0};
 for(int i=0;i<4;i++){check(SUCCEEDED(d->CreateTexture(2,2,1,0,D3DFMT_A8R8G8B8,D3DPOOL_MANAGED,&textures[i],nullptr)),"texture");D3DLOCKED_RECT r{};check(SUCCEEDED(textures[i]->LockRect(0,&r,nullptr,0)),"texture lock");for(int y=0;y<2;y++)for(int x=0;x<2;x++)((DWORD*)((char*)r.pBits+y*r.Pitch))[x]=colors[i];textures[i]->UnlockRect(0);d->SetTexture(i,textures[i]);}
 struct V{float x,y,z,rhw;DWORD diffuse,specular;float uv[8];};
 V vertices[]={{0,0,.5f,1,0xff102030,0xff305070,{0,0,0,0,0,0,0,0}},{64,0,.5f,1,0xff102030,0xff305070,{1,0,1,0,1,0,1,0}},{0,64,.5f,1,0xff102030,0xff305070,{0,1,0,1,0,1,0,1}},{64,64,.5f,1,0xff102030,0xff305070,{1,1,1,1,1,1,1,1}}};
 d->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR|D3DFVF_TEX4);d->SetRenderState(D3DRS_ZENABLE,FALSE);d->SetRenderState(D3DRS_LIGHTING,FALSE);d->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);d->SetRenderState(D3DRS_ALPHABLENDENABLE,FALSE);
 IDirect3DSurface9 *target=nullptr,*readback=nullptr;check(SUCCEEDED(d->GetRenderTarget(0,&target)),"target");check(SUCCEEDED(d->CreateOffscreenPlainSurface(64,64,D3DFMT_A8R8G8B8,D3DPOOL_SYSTEMMEM,&readback,nullptr)),"readback");
 auto pixel=[&](){check(SUCCEEDED(d->Clear(0,nullptr,D3DCLEAR_TARGET,0xff000000,1,0)),"clear");check(SUCCEEDED(d->BeginScene()),"begin");check(SUCCEEDED(d->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP,2,vertices,sizeof(V))),"draw");check(SUCCEEDED(d->EndScene()),"end");check(SUCCEEDED(d->GetRenderTargetData(target,readback)),"readback copy");D3DLOCKED_RECT r{};check(SUCCEEDED(readback->LockRect(&r,nullptr,D3DLOCK_READONLY)),"readback lock");DWORD value;memcpy(&value,(char*)r.pBits+32*r.Pitch+32*4,4);readback->UnlockRect();return value;};
 auto near=[&](DWORD value,DWORD expected,const char*what){std::printf("%s pixel=%08x expected=%08x\n",what,value,expected);for(int shift:{0,8,16,24})check(std::abs(int((value>>shift)&255)-int((expected>>shift)&255))<=2,what);};
 d->SetPixelShader(ps);float ignored[]={1,1,1,1};d->SetPixelShaderConstantF(1,ignored,1);near(pixel(),0xff507090,"original four sample mean with shader-local c1");
 d->SetPixelShader(nullptr);d->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_TEXTURE);d->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_TEXTURE);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);near(pixel(),colors[0],"unaffected fixed function sample");
 d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_SPECULAR);near(pixel(),0xff305070,"worldmap specular argument");
 d->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);d->SetTextureStageState(0,D3DTSS_RESULTARG,D3DTA_TEMP);d->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_ADD);d->SetTextureStageState(1,D3DTSS_COLORARG1,D3DTA_TEMP);d->SetTextureStageState(1,D3DTSS_COLORARG2,D3DTA_SPECULAR);d->SetTextureStageState(1,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);d->SetTextureStageState(1,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);near(pixel(),0xff4070a0,"temporary combiner register");
 check(FAILED(d->Reset(&pp)),"Reset rejects retained backbuffer reference");target->Release();target=nullptr;readback->Release();readback=nullptr;pp.BackBufferWidth=48;pp.BackBufferHeight=32;check(SUCCEEDED(d->Reset(&pp)),"Reset resizes target");check(SUCCEEDED(d->GetRenderTarget(0,&target)),"new target");D3DSURFACE_DESC desc{};target->GetDesc(&desc);check(desc.Width==48&&desc.Height==32,"actual reset target dimensions");D3DVIEWPORT9 vp{};d->GetViewport(&vp);check(vp.Width==48&&vp.Height==32,"reset viewport");IDirect3DPixelShader9*bound=nullptr;d->GetPixelShader(&bound);check(!bound,"reset shader binding cleared");
 target->Release();ps->Release();for(auto*t:textures)t->Release();d->Release();api->Release();SDL_DestroyWindow(w);SDL_Quit();std::puts("PASS: postprocess GPU, FFP specular/TEMP, Reset");
}
