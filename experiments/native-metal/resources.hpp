#pragma once
#import <Metal/Metal.h>
#undef TRUE
#undef FALSE
#define BOOL D3D_NATIVE_BOOL
#include "stubs.hpp"
#undef BOOL
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <atomic>
#include <string>
#include "texture_material_compat.hpp"
namespace sm {
inline uint64_t nextBufferIdentity(){static std::atomic<uint64_t> next{1};return next.fetch_add(1,std::memory_order_relaxed);}
void retainDeviceResource(IDirect3DDevice9*);
void releaseDeviceResource(IDirect3DDevice9*);
HRESULT synchronizeDevice(IDirect3DDevice9*);
struct ResourceParent {
 IDirect3DDevice9* owner=nullptr;
 ResourceParent()=default;
 ResourceParent(const ResourceParent&)=delete;
 ResourceParent& operator=(const ResourceParent&)=delete;
 void bindOwner(IDirect3DDevice9* next){if(next==owner)return;if(next)retainDeviceResource(next);auto*old=owner;owner=next;if(old)releaseDeviceResource(old);}
 ~ResourceParent(){if(owner)releaseDeviceResource(owner);}
 HRESULT getOwner(IDirect3DDevice9**out){if(!out)return D3DERR_INVALIDCALL;*out=owner;if(!owner)return D3DERR_INVALIDCALL;owner->AddRef();return S_OK;}
};
inline bool compressed(D3DFORMAT f){return f==D3DFMT_DXT1||f==D3DFMT_DXT3||f==D3DFMT_DXT5;}
inline unsigned pixelBytes(D3DFORMAT f){switch(f){case D3DFMT_R8G8B8:return 3;case D3DFMT_R5G6B5:case D3DFMT_X1R5G5B5:case D3DFMT_A1R5G5B5:case D3DFMT_A4R4G4B4:case D3DFMT_A8L8:return 2;case D3DFMT_A8:case D3DFMT_L8:return 1;default:return 4;}}
inline uint16_t u16(const uint8_t*p){return p[0]|uint16_t(p[1])<<8;}
inline void rgb565(uint16_t v,uint8_t*p){p[0]=((v>>11)&31)*255/31;p[1]=((v>>5)&63)*255/63;p[2]=(v&31)*255/31;p[3]=255;}
struct Surface:StubIDirect3DSurface9,ResourceParent{
 HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**out)override{return getOwner(out);}
 id<MTLTexture> gpu=nil;D3DSURFACE_DESC desc{};std::vector<uint8_t>bytes;unsigned pitch=0,mip=0,slice=0;bool locked=false,readOnly=false,gpuDirty=false;
 Surface(id<MTLDevice>dev,UINT w,UINT h,D3DFORMAT fmt,bool depth=false){
  desc.Format=fmt;desc.Type=D3DRTYPE_SURFACE;desc.Pool=D3DPOOL_DEFAULT;desc.Width=w;desc.Height=h;desc.MultiSampleType=D3DMULTISAMPLE_NONE;desc.Usage=depth?D3DUSAGE_DEPTHSTENCIL:0;
  pitch=compressed(fmt)?std::max(1u,(w+3)/4)*(fmt==D3DFMT_DXT1?8:16):w*pixelBytes(fmt);bytes.resize(size_t(pitch)*(compressed(fmt)?std::max(1u,(h+3)/4):h));
  auto*d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:depth?MTLPixelFormatDepth32Float:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];d.storageMode=depth?MTLStorageModePrivate:MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;gpu=[dev newTextureWithDescriptor:d];
 }
 D3DRESOURCETYPE STDMETHODCALLTYPE GetType()override{return D3DRTYPE_SURFACE;}
 HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC*out)override{if(!out)return D3DERR_INVALIDCALL;*out=desc;return S_OK;}
 HRESULT download(){
  if(!gpuDirty)return S_OK;
  if(compressed(desc.Format))return unsupported("compressed GPU readback");
  if(!gpu||(desc.Usage&D3DUSAGE_DEPTHSTENCIL))return D3DERR_INVALIDCALL;
  if(owner){auto hr=synchronizeDevice(owner);if(FAILED(hr))return hr;}
  const unsigned w=desc.Width,h=desc.Height;std::vector<uint8_t>rgba(size_t(w)*h*4);
  if(gpu.pixelFormat!=MTLPixelFormatRGBA8Unorm&&gpu.pixelFormat!=MTLPixelFormatBGRA8Unorm)return unsupported("Surface::download GPU format");
  [gpu getBytes:rgba.data() bytesPerRow:w*4 bytesPerImage:w*h*4 fromRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:mip slice:slice];
  for(size_t n=0;n<size_t(w)*h;n++){auto*q=rgba.data()+n*4;if(gpu.pixelFormat==MTLPixelFormatBGRA8Unorm)std::swap(q[0],q[2]);auto*p=bytes.data()+n*pixelBytes(desc.Format);uint16_t packed=0;
   switch(desc.Format){
    case D3DFMT_A8R8G8B8:case D3DFMT_X8R8G8B8:case D3DFMT_R8G8B8:p[0]=q[2];p[1]=q[1];p[2]=q[0];if(desc.Format!=D3DFMT_R8G8B8)p[3]=desc.Format==D3DFMT_X8R8G8B8?255:q[3];break;
    case D3DFMT_A8B8G8R8:case D3DFMT_X8B8G8R8:memcpy(p,q,4);if(desc.Format==D3DFMT_X8B8G8R8)p[3]=255;break;
    case D3DFMT_R5G6B5:packed=(uint16_t(q[0]>>3)<<11)|(uint16_t(q[1]>>2)<<5)|(q[2]>>3);memcpy(p,&packed,2);break;
    case D3DFMT_A1R5G5B5:case D3DFMT_X1R5G5B5:packed=(uint16_t(q[0]>>3)<<10)|(uint16_t(q[1]>>3)<<5)|(q[2]>>3)|((desc.Format==D3DFMT_X1R5G5B5||q[3]>=128)?0x8000:0);memcpy(p,&packed,2);break;
    case D3DFMT_A4R4G4B4:packed=(uint16_t(q[3]>>4)<<12)|(uint16_t(q[0]>>4)<<8)|(uint16_t(q[1]>>4)<<4)|(q[2]>>4);memcpy(p,&packed,2);break;
    case D3DFMT_A8:p[0]=q[3];break;
    case D3DFMT_L8:case D3DFMT_A8L8:p[0]=q[0];if(desc.Format==D3DFMT_A8L8)p[1]=q[3];break;
    default:return unsupported("Surface::download D3D format");
   }
  }gpuDirty=false;return S_OK;
 }
 HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT*out,const RECT*r,DWORD flags)override{
  if(!out||locked||(desc.Usage&D3DUSAGE_DEPTHSTENCIL))return D3DERR_INVALIDCALL;
  UINT x=r?r->left:0,y=r?r->top:0;if(r&&(r->left<0||r->top<0||r->right>LONG(desc.Width)||r->bottom>LONG(desc.Height)||r->right<=r->left||r->bottom<=r->top))return D3DERR_INVALIDCALL;
  if(compressed(desc.Format)&&((x%4)||(y%4)))return D3DERR_INVALIDCALL;
  if(gpuDirty&&!(flags&D3DLOCK_DISCARD)){auto hr=download();if(FAILED(hr))return hr;}
  out->Pitch=pitch;out->pBits=bytes.data()+(compressed(desc.Format)?size_t(y/4)*pitch+(x/4)*(desc.Format==D3DFMT_DXT1?8:16):size_t(y)*pitch+x*pixelBytes(desc.Format));locked=true;readOnly=flags&D3DLOCK_READONLY;return S_OK;
 }
 void upload(){
  if(!gpu||(desc.Usage&D3DUSAGE_DEPTHSTENCIL))return;unsigned w=desc.Width,h=desc.Height;std::vector<uint8_t>rgba(size_t(w)*h*4);
  if(desc.Format==D3DFMT_DXT1||desc.Format==D3DFMT_DXT3||desc.Format==D3DFMT_DXT5||desc.Format==D3DFMT_A8R8G8B8||desc.Format==D3DFMT_X8R8G8B8||desc.Format==D3DFMT_A4R4G4B4||desc.Format==D3DFMT_A1R5G5B5||desc.Format==D3DFMT_X1R5G5B5||desc.Format==D3DFMT_A8){
   if(!storm_metal::compat::decodeToRgba(desc.Format,w,h,bytes.data(),bytes.size(),pitch,rgba)){unsupported("Surface::upload invalid texture payload");return;}
  }else for(size_t i=0;i<size_t(w)*h;i++){
   const auto*p=bytes.data()+i*pixelBytes(desc.Format);auto*q=rgba.data()+i*4;q[3]=255;
   switch(desc.Format){
    case D3DFMT_A8R8G8B8:case D3DFMT_X8R8G8B8:case D3DFMT_R8G8B8:q[0]=p[2];q[1]=p[1];q[2]=p[0];if(desc.Format==D3DFMT_A8R8G8B8)q[3]=p[3];break;
    case D3DFMT_A8B8G8R8:case D3DFMT_X8B8G8R8:memcpy(q,p,4);if(desc.Format==D3DFMT_X8B8G8R8)q[3]=255;break;
    case D3DFMT_R5G6B5:rgb565(u16(p),q);break;
    case D3DFMT_A1R5G5B5:case D3DFMT_X1R5G5B5:{auto v=u16(p);q[0]=((v>>10)&31)*255/31;q[1]=((v>>5)&31)*255/31;q[2]=(v&31)*255/31;if(desc.Format==D3DFMT_A1R5G5B5)q[3]=(v&32768)?255:0;break;}
    case D3DFMT_A4R4G4B4:{auto v=u16(p);q[0]=((v>>8)&15)*17;q[1]=((v>>4)&15)*17;q[2]=(v&15)*17;q[3]=(v>>12)*17;break;}
    case D3DFMT_L8:case D3DFMT_A8L8:q[0]=q[1]=q[2]=p[0];if(desc.Format==D3DFMT_A8L8)q[3]=p[1];break;
    case D3DFMT_A8:q[0]=q[1]=q[2]=255;q[3]=p[0];break;
    default:unsupported("Surface::upload format");return;
   }
  }
  if(gpu.pixelFormat==MTLPixelFormatBGRA8Unorm)for(size_t i=0;i<rgba.size();i+=4)std::swap(rgba[i],rgba[i+2]);
  [gpu replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:mip slice:slice withBytes:rgba.data() bytesPerRow:w*4 bytesPerImage:w*h*4];gpuDirty=false;
 }
 HRESULT STDMETHODCALLTYPE UnlockRect()override{if(!locked)return D3DERR_INVALIDCALL;locked=false;if(!readOnly)upload();return S_OK;}
};
struct Texture:StubIDirect3DTexture9,ResourceParent{
 HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**out)override{return getOwner(out);}
 const uint64_t cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0;
 std::vector<Surface*>levels;DWORD lod=0;uint32_t rockK2Variant=0;bool weatherWaterEffect=false,clampBackdropV=false,antiTiling=false;std::string diagnosticLabel;mutable int8_t meaningfulAlphaCache=-1;mutable int8_t coverageMaskCache=-1;
 Texture(id<MTLDevice>dev,UINT w,UINT h,UINT count,DWORD usage,D3DFORMAT fmt){unsigned max=1;for(unsigned n=std::max(w,h);n>1;n>>=1)max++;count=count?std::min(count,max):max;
  auto*d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:count>1];d.mipmapLevelCount=count;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;id<MTLTexture>texture=[dev newTextureWithDescriptor:d];
  for(unsigned i=0;i<count;i++){auto*s=new Surface(dev,std::max(1u,w>>i),std::max(1u,h>>i),fmt);s->gpu=texture;s->mip=i;s->desc.Usage=usage;levels.push_back(s);}
 }
 void bindOwner(IDirect3DDevice9*o){ResourceParent::bindOwner(o);for(auto*s:levels)s->bindOwner(o);}
 ~Texture(){for(auto*s:levels)s->Release();}
 id<MTLTexture>gpu()const{return levels.empty()?nil:levels[0]->gpu;}
 bool meaningfulAlpha()const{
  if(meaningfulAlphaCache>=0)return meaningfulAlphaCache;if(levels.empty())return false;const auto*s=levels[0];std::vector<uint8_t>rgba;
  if(!storm_metal::compat::decodeToRgba(s->desc.Format,s->desc.Width,s->desc.Height,s->bytes.data(),s->bytes.size(),s->pitch,rgba))return meaningfulAlphaCache=0;
  bool transparent=false,opaque=false;for(size_t i=3;i<rgba.size()&&!(transparent&&opaque);i+=4){transparent|=rgba[i]<128;opaque|=rgba[i]>=128;}
  meaningfulAlphaCache=transparent&&opaque;return meaningfulAlphaCache;
 }
 bool coverageMask()const{
  if(coverageMaskCache>=0)return coverageMaskCache;if(levels.empty())return false;const auto*s=levels[0];std::vector<uint8_t>rgba;
  if(!storm_metal::compat::decodeToRgba(s->desc.Format,s->desc.Width,s->desc.Height,s->bytes.data(),s->bytes.size(),s->pitch,rgba))return coverageMaskCache=0;
  size_t clear=0,solid=0,transition=0,pixels=rgba.size()/4;for(size_t i=3;i<rgba.size();i+=4){const uint8_t a=rgba[i];if(a<=24)++clear;else if(a>=231)++solid;else ++transition;}
  // Coverage textures contain real holes and real surfaces. A broad middle band
  // is data (gloss/specular/opacity), not geometry coverage, and must never clip
  // weapons or other opaque materials merely because their alpha channel varies.
  coverageMaskCache=pixels&&clear*50>=pixels&&solid*50>=pixels&&(clear+solid)*5>=pixels*3;return coverageMaskCache;
 }
 D3DRESOURCETYPE STDMETHODCALLTYPE GetType()override{return D3DRTYPE_TEXTURE;}
 DWORD STDMETHODCALLTYPE GetLevelCount()override{return levels.size();}
 DWORD STDMETHODCALLTYPE SetLOD(DWORD n)override{auto old=lod;lod=n;return old;}
 DWORD STDMETHODCALLTYPE GetLOD()override{return lod;}
 void STDMETHODCALLTYPE PreLoad()override{}
 HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT n,D3DSURFACE_DESC*out)override{return n<levels.size()?levels[n]->GetDesc(out):D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT n,IDirect3DSurface9**out)override{if(!out||n>=levels.size())return D3DERR_INVALIDCALL;*out=levels[n];levels[n]->AddRef();return S_OK;}
 HRESULT STDMETHODCALLTYPE LockRect(UINT n,D3DLOCKED_RECT*out,const RECT*r,DWORD flags)override{return n<levels.size()?levels[n]->LockRect(out,r,flags):D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE UnlockRect(UINT n)override{if(n>=levels.size())return D3DERR_INVALIDCALL;auto hr=levels[n]->UnlockRect();if(SUCCEEDED(hr)){++cacheRevision;meaningfulAlphaCache=-1;coverageMaskCache=-1;}return hr;}
 HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT*)override{return S_OK;}
};
struct VertexBuffer:StubIDirect3DVertexBuffer9,ResourceParent{
 HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**out)override{return getOwner(out);}
 const uint64_t cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0;
 std::vector<uint8_t>bytes;DWORD usage=0,fvf=0;bool locked=false,gpuSkinned=false;id<MTLBuffer> gpuSkinningBuffer=nil;explicit VertexBuffer(UINT n):bytes(n){}
 D3DRESOURCETYPE STDMETHODCALLTYPE GetType()override{return D3DRTYPE_VERTEXBUFFER;}
 HRESULT STDMETHODCALLTYPE Lock(UINT offset,UINT size,void**out,DWORD)override{if(!out||locked||offset>bytes.size()||(size&&size>bytes.size()-offset))return D3DERR_INVALIDCALL;*out=bytes.data()+offset;locked=true;return S_OK;}
 HRESULT STDMETHODCALLTYPE Unlock()override{if(!locked)return D3DERR_INVALIDCALL;locked=false;++cacheRevision;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetDesc(D3DVERTEXBUFFER_DESC*out)override{if(!out)return D3DERR_INVALIDCALL;*out={};out->Format=D3DFMT_VERTEXDATA;out->Type=D3DRTYPE_VERTEXBUFFER;out->Usage=usage;out->Size=bytes.size();out->FVF=fvf;return S_OK;}
};
struct IndexBuffer:StubIDirect3DIndexBuffer9,ResourceParent{
 HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**out)override{return getOwner(out);}
 const uint64_t cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0;
 std::vector<uint8_t>bytes;D3DFORMAT format;DWORD usage=0;bool locked=false;IndexBuffer(UINT n,D3DFORMAT fmt):bytes(n),format(fmt){}
 D3DRESOURCETYPE STDMETHODCALLTYPE GetType()override{return D3DRTYPE_INDEXBUFFER;}
 HRESULT STDMETHODCALLTYPE Lock(UINT offset,UINT size,void**out,DWORD)override{if(!out||locked||offset>bytes.size()||(size&&size>bytes.size()-offset))return D3DERR_INVALIDCALL;*out=bytes.data()+offset;locked=true;return S_OK;}
 HRESULT STDMETHODCALLTYPE Unlock()override{if(!locked)return D3DERR_INVALIDCALL;locked=false;++cacheRevision;return S_OK;}
 HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC*out)override{if(!out)return D3DERR_INVALIDCALL;*out={};out->Format=format;out->Type=D3DRTYPE_INDEXBUFFER;out->Usage=usage;out->Size=bytes.size();return S_OK;}
};
}
namespace sm {
struct CubeTexture:StubIDirect3DCubeTexture9,ResourceParent{
 HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**out)override{return getOwner(out);}
 std::vector<Surface*>faces[6];id<MTLTexture>texture=nil;DWORD lod=0;
 CubeTexture(id<MTLDevice>dev,UINT edge,UINT count,DWORD usage,D3DFORMAT fmt){unsigned max=1;for(unsigned n=edge;n>1;n>>=1)max++;count=count?std::min(count,max):max;
  auto*d=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm size:edge mipmapped:count>1];d.mipmapLevelCount=count;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;texture=[dev newTextureWithDescriptor:d];
  for(unsigned face=0;face<6;face++)for(unsigned i=0;i<count;i++){auto*s=new Surface(dev,std::max(1u,edge>>i),std::max(1u,edge>>i),fmt);s->gpu=texture;s->mip=i;s->slice=face;s->desc.Usage=usage;faces[face].push_back(s);}
 }
 void bindOwner(IDirect3DDevice9*o){ResourceParent::bindOwner(o);for(auto&f:faces)for(auto*s:f)s->bindOwner(o);}
 ~CubeTexture(){for(auto&f:faces)for(auto*s:f)s->Release();}
 id<MTLTexture>gpu()const{return texture;}
 D3DRESOURCETYPE STDMETHODCALLTYPE GetType()override{return D3DRTYPE_CUBETEXTURE;}
 DWORD STDMETHODCALLTYPE GetLevelCount()override{return faces[0].size();}
 DWORD STDMETHODCALLTYPE SetLOD(DWORD n)override{auto old=lod;lod=n;return old;}
 DWORD STDMETHODCALLTYPE GetLOD()override{return lod;}
 HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT n,D3DSURFACE_DESC*out)override{return n<faces[0].size()?faces[0][n]->GetDesc(out):D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE GetCubeMapSurface(D3DCUBEMAP_FACES f,UINT n,IDirect3DSurface9**out)override{if(out)*out=nullptr;if(!out||unsigned(f)>=6||n>=faces[f].size())return D3DERR_INVALIDCALL;*out=faces[f][n];faces[f][n]->AddRef();return S_OK;}
 HRESULT STDMETHODCALLTYPE LockRect(D3DCUBEMAP_FACES f,UINT n,D3DLOCKED_RECT*out,const RECT*r,DWORD flags)override{return unsigned(f)<6&&n<faces[f].size()?faces[f][n]->LockRect(out,r,flags):D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE UnlockRect(D3DCUBEMAP_FACES f,UINT n)override{return unsigned(f)<6&&n<faces[f].size()?faces[f][n]->UnlockRect():D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE AddDirtyRect(D3DCUBEMAP_FACES f,const RECT*)override{return unsigned(f)<6?S_OK:D3DERR_INVALIDCALL;}
};
struct Volume:StubIDirect3DVolume9,ResourceParent{
 HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**out)override{return getOwner(out);}
 id<MTLTexture>gpu=nil;D3DVOLUME_DESC desc{};std::vector<uint8_t>bytes;unsigned rowPitch=0,slicePitch=0,mip=0;bool locked=false,readOnly=false;
 Volume(UINT w,UINT h,UINT depth,D3DFORMAT fmt,DWORD usage){desc.Format=fmt;desc.Type=D3DRTYPE_VOLUME;desc.Usage=usage;desc.Pool=D3DPOOL_MANAGED;desc.Width=w;desc.Height=h;desc.Depth=depth;rowPitch=w*pixelBytes(fmt);slicePitch=rowPitch*h;bytes.resize(size_t(slicePitch)*depth);}
 HRESULT STDMETHODCALLTYPE GetDesc(D3DVOLUME_DESC*out)override{if(!out)return D3DERR_INVALIDCALL;*out=desc;return S_OK;}
 HRESULT STDMETHODCALLTYPE LockBox(D3DLOCKED_BOX*out,const D3DBOX*b,DWORD flags)override{if(!out||locked)return D3DERR_INVALIDCALL;if(compressed(desc.Format))return unsupported("Volume::LockBox compressed format");if(b&&(b->Left>=b->Right||b->Top>=b->Bottom||b->Front>=b->Back||b->Right>desc.Width||b->Bottom>desc.Height||b->Back>desc.Depth))return D3DERR_INVALIDCALL;out->RowPitch=rowPitch;out->SlicePitch=slicePitch;out->pBits=bytes.data()+(b?size_t(b->Front)*slicePitch+size_t(b->Top)*rowPitch+b->Left*pixelBytes(desc.Format):0);locked=true;readOnly=flags&D3DLOCK_READONLY;return S_OK;}
 HRESULT STDMETHODCALLTYPE UnlockBox()override{if(!locked)return D3DERR_INVALIDCALL;locked=false;if(readOnly)return S_OK;if(desc.Format!=D3DFMT_A8R8G8B8&&desc.Format!=D3DFMT_X8R8G8B8)return unsupported("Volume::UnlockBox non-BGRA format");if(!gpu)return D3DERR_INVALIDCALL;std::vector<uint8_t>rgba(bytes);for(size_t n=0;n<rgba.size();n+=4){std::swap(rgba[n],rgba[n+2]);if(desc.Format==D3DFMT_X8R8G8B8)rgba[n+3]=255;}[gpu replaceRegion:MTLRegionMake3D(0,0,0,desc.Width,desc.Height,desc.Depth) mipmapLevel:mip slice:0 withBytes:rgba.data() bytesPerRow:rowPitch bytesPerImage:slicePitch];return S_OK;}
};
struct VolumeTexture:StubIDirect3DVolumeTexture9,ResourceParent{
 HRESULT STDMETHODCALLTYPE GetDevice(IDirect3DDevice9**out)override{return getOwner(out);}
 std::vector<Volume*>levels;id<MTLTexture>texture=nil;DWORD lod=0;
 VolumeTexture(id<MTLDevice>dev,UINT w,UINT h,UINT depth,UINT count,DWORD usage,D3DFORMAT fmt){unsigned max=1;for(unsigned n=std::max({w,h,depth});n>1;n>>=1)max++;count=count?std::min(count,max):max;auto*d=[MTLTextureDescriptor new];d.textureType=MTLTextureType3D;d.pixelFormat=MTLPixelFormatRGBA8Unorm;d.width=w;d.height=h;d.depth=depth;d.mipmapLevelCount=count;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead;texture=[dev newTextureWithDescriptor:d];for(unsigned i=0;i<count;i++){auto*v=new Volume(std::max(1u,w>>i),std::max(1u,h>>i),std::max(1u,depth>>i),fmt,usage);v->gpu=texture;v->mip=i;levels.push_back(v);}}
 void bindOwner(IDirect3DDevice9*o){ResourceParent::bindOwner(o);for(auto*v:levels)v->bindOwner(o);}
 ~VolumeTexture(){for(auto*v:levels)v->Release();}
 id<MTLTexture>gpu()const{return texture;}
 D3DRESOURCETYPE STDMETHODCALLTYPE GetType()override{return D3DRTYPE_VOLUMETEXTURE;}
 DWORD STDMETHODCALLTYPE GetLevelCount()override{return levels.size();}
 DWORD STDMETHODCALLTYPE SetLOD(DWORD n)override{auto old=lod;lod=n;return old;}
 DWORD STDMETHODCALLTYPE GetLOD()override{return lod;}
 HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT n,D3DVOLUME_DESC*out)override{return n<levels.size()?levels[n]->GetDesc(out):D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE GetVolumeLevel(UINT n,IDirect3DVolume9**out)override{if(out)*out=nullptr;if(!out||n>=levels.size())return D3DERR_INVALIDCALL;*out=levels[n];levels[n]->AddRef();return S_OK;}
 HRESULT STDMETHODCALLTYPE LockBox(UINT n,D3DLOCKED_BOX*out,const D3DBOX*b,DWORD flags)override{return n<levels.size()?levels[n]->LockBox(out,b,flags):D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE UnlockBox(UINT n)override{return n<levels.size()?levels[n]->UnlockBox():D3DERR_INVALIDCALL;}
 HRESULT STDMETHODCALLTYPE AddDirtyBox(const D3DBOX*)override{return S_OK;}
};
}
