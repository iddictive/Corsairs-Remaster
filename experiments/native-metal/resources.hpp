#pragma once
#import <Metal/Metal.h>
#include "render_device_api.hpp"
#include "render_diagnostics.hpp"
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdint>
#include <atomic>
#include <string>
#include "texture_material_compat.hpp"
#include "frame_flight.hpp"
namespace sm {
inline uint64_t nextBufferIdentity(){static std::atomic<uint64_t> next{1};return next.fetch_add(1,std::memory_order_relaxed);}
void retainDeviceResource(storm_render::Device*);
void releaseDeviceResource(storm_render::Device*);
storm_render::Result synchronizeDevice(storm_render::Device*);
struct ResourceParent {
 storm_render::Device* owner=nullptr;
 ResourceParent()=default;
 ResourceParent(const ResourceParent&)=delete;
 ResourceParent& operator=(const ResourceParent&)=delete;
 void bindOwner(storm_render::Device* next){if(next==owner)return;if(next)retainDeviceResource(next);auto*old=owner;owner=next;if(old)releaseDeviceResource(old);}
 ~ResourceParent(){if(owner)releaseDeviceResource(owner);}
 storm_render::Result getOwner(storm_render::Device**out){if(!out)return storm_render::result::invalidCall;*out=owner;if(!owner)return storm_render::result::invalidCall;owner->AddRef();return storm_render::result::ok;}
};
inline bool compressed(storm_render::AssetFormat f){return f==storm_render::AssetFormat::DXT1||f==storm_render::AssetFormat::DXT3||f==storm_render::AssetFormat::DXT5;}
inline unsigned pixelBytes(storm_render::AssetFormat f){switch(f){case storm_render::AssetFormat::R8G8B8:return 3;case storm_render::AssetFormat::R5G6B5:case storm_render::AssetFormat::X1R5G5B5:case storm_render::AssetFormat::A1R5G5B5:case storm_render::AssetFormat::A4R4G4B4:case storm_render::AssetFormat::A8L8:return 2;case storm_render::AssetFormat::A8:case storm_render::AssetFormat::L8:return 1;default:return 4;}}
inline uint16_t u16(const uint8_t*p){return p[0]|uint16_t(p[1])<<8;}
inline void rgb565(uint16_t v,uint8_t*p){p[0]=((v>>11)&31)*255/31;p[1]=((v>>5)&63)*255/63;p[2]=(v&31)*255/31;p[3]=255;}
struct Surface:storm_render::Surface,ResourceParent{
 storm_render::Result  GetDevice(storm_render::Device**out)override{return getOwner(out);}
 id<MTLTexture> gpu=nil;storm_render::SurfaceDesc desc{};std::vector<uint8_t>bytes;unsigned pitch=0,mip=0,slice=0;bool locked=false,readOnly=false,gpuDirty=false;
 Surface(id<MTLDevice>dev,uint32_t w,uint32_t h,storm_render::AssetFormat fmt,bool depth=false){
  desc.Format=fmt;desc.Type=storm_render::ResourceKind::Surface;desc.Pool=storm_render::MemoryClass::Default;desc.Width=w;desc.Height=h;desc.MultiSampleType=storm_render::SampleCount::None;desc.Usage=depth?storm_render::usage::depthStencil:0;
  pitch=compressed(fmt)?std::max(1u,(w+3)/4)*(fmt==storm_render::AssetFormat::DXT1?8:16):w*pixelBytes(fmt);bytes.resize(size_t(pitch)*(compressed(fmt)?std::max(1u,(h+3)/4):h));
  auto*d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:depth?MTLPixelFormatDepth32Float:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:NO];d.storageMode=depth?MTLStorageModePrivate:MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;gpu=[dev newTextureWithDescriptor:d];
 }
 storm_render::ResourceKind  GetType()override{return storm_render::ResourceKind::Surface;}
 storm_render::Result  GetDesc(storm_render::SurfaceDesc*out)override{if(!out)return storm_render::result::invalidCall;*out=desc;return storm_render::result::ok;}
 storm_render::Result download(){
  if(!gpuDirty)return storm_render::result::ok;
  if(compressed(desc.Format))return unsupported("compressed GPU readback");
  if(!gpu||(desc.Usage&storm_render::usage::depthStencil))return storm_render::result::invalidCall;
  if(owner){auto hr=synchronizeDevice(owner);if(storm_render::failed(hr))return hr;}
  const unsigned w=desc.Width,h=desc.Height;std::vector<uint8_t>rgba(size_t(w)*h*4);
  if(gpu.pixelFormat!=MTLPixelFormatRGBA8Unorm&&gpu.pixelFormat!=MTLPixelFormatBGRA8Unorm)return unsupported("Surface::download GPU format");
  [gpu getBytes:rgba.data() bytesPerRow:w*4 bytesPerImage:w*h*4 fromRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:mip slice:slice];
  for(size_t n=0;n<size_t(w)*h;n++){auto*q=rgba.data()+n*4;if(gpu.pixelFormat==MTLPixelFormatBGRA8Unorm)std::swap(q[0],q[2]);auto*p=bytes.data()+n*pixelBytes(desc.Format);uint16_t packed=0;
   switch(desc.Format){
    case storm_render::AssetFormat::A8R8G8B8:case storm_render::AssetFormat::X8R8G8B8:case storm_render::AssetFormat::R8G8B8:p[0]=q[2];p[1]=q[1];p[2]=q[0];if(desc.Format!=storm_render::AssetFormat::R8G8B8)p[3]=desc.Format==storm_render::AssetFormat::X8R8G8B8?255:q[3];break;
    case storm_render::AssetFormat::A8B8G8R8:case storm_render::AssetFormat::X8B8G8R8:memcpy(p,q,4);if(desc.Format==storm_render::AssetFormat::X8B8G8R8)p[3]=255;break;
    case storm_render::AssetFormat::R5G6B5:packed=(uint16_t(q[0]>>3)<<11)|(uint16_t(q[1]>>2)<<5)|(q[2]>>3);memcpy(p,&packed,2);break;
    case storm_render::AssetFormat::A1R5G5B5:case storm_render::AssetFormat::X1R5G5B5:packed=(uint16_t(q[0]>>3)<<10)|(uint16_t(q[1]>>3)<<5)|(q[2]>>3)|((desc.Format==storm_render::AssetFormat::X1R5G5B5||q[3]>=128)?0x8000:0);memcpy(p,&packed,2);break;
    case storm_render::AssetFormat::A4R4G4B4:packed=(uint16_t(q[3]>>4)<<12)|(uint16_t(q[0]>>4)<<8)|(uint16_t(q[1]>>4)<<4)|(q[2]>>4);memcpy(p,&packed,2);break;
    case storm_render::AssetFormat::A8:p[0]=q[3];break;
    case storm_render::AssetFormat::L8:case storm_render::AssetFormat::A8L8:p[0]=q[0];if(desc.Format==storm_render::AssetFormat::A8L8)p[1]=q[3];break;
    default:return unsupported("Surface::download asset format");
   }
  }gpuDirty=false;return storm_render::result::ok;
 }
 storm_render::Result  LockRect(storm_render::LockedRect*out,const storm_render::Rect*r,uint32_t flags)override{
  if(!out||locked||(desc.Usage&storm_render::usage::depthStencil))return storm_render::result::invalidCall;
  uint32_t x=r?r->left:0,y=r?r->top:0;if(r&&(r->left<0||r->top<0||r->right>int32_t(desc.Width)||r->bottom>int32_t(desc.Height)||r->right<=r->left||r->bottom<=r->top))return storm_render::result::invalidCall;
  if(compressed(desc.Format)&&((x%4)||(y%4)))return storm_render::result::invalidCall;
  if(gpuDirty&&!(flags&storm_render::lock::discard)){auto hr=download();if(storm_render::failed(hr))return hr;}
  out->Pitch=pitch;out->pBits=bytes.data()+(compressed(desc.Format)?size_t(y/4)*pitch+(x/4)*(desc.Format==storm_render::AssetFormat::DXT1?8:16):size_t(y)*pitch+x*pixelBytes(desc.Format));locked=true;readOnly=flags&storm_render::lock::readOnly;return storm_render::result::ok;
 }
 void upload(){
  if(!gpu||(desc.Usage&storm_render::usage::depthStencil))return;unsigned w=desc.Width,h=desc.Height;std::vector<uint8_t>rgba(size_t(w)*h*4);
  if(desc.Format==storm_render::AssetFormat::DXT1||desc.Format==storm_render::AssetFormat::DXT3||desc.Format==storm_render::AssetFormat::DXT5||desc.Format==storm_render::AssetFormat::A8R8G8B8||desc.Format==storm_render::AssetFormat::X8R8G8B8||desc.Format==storm_render::AssetFormat::A4R4G4B4||desc.Format==storm_render::AssetFormat::A1R5G5B5||desc.Format==storm_render::AssetFormat::X1R5G5B5||desc.Format==storm_render::AssetFormat::A8){
   if(!storm_metal::compat::decodeToRgba(desc.Format,w,h,bytes.data(),bytes.size(),pitch,rgba)){unsupported("Surface::upload invalid texture payload");return;}
  }else for(size_t i=0;i<size_t(w)*h;i++){
   const auto*p=bytes.data()+i*pixelBytes(desc.Format);auto*q=rgba.data()+i*4;q[3]=255;
   switch(desc.Format){
    case storm_render::AssetFormat::A8R8G8B8:case storm_render::AssetFormat::X8R8G8B8:case storm_render::AssetFormat::R8G8B8:q[0]=p[2];q[1]=p[1];q[2]=p[0];if(desc.Format==storm_render::AssetFormat::A8R8G8B8)q[3]=p[3];break;
    case storm_render::AssetFormat::A8B8G8R8:case storm_render::AssetFormat::X8B8G8R8:memcpy(q,p,4);if(desc.Format==storm_render::AssetFormat::X8B8G8R8)q[3]=255;break;
    case storm_render::AssetFormat::R5G6B5:rgb565(u16(p),q);break;
    case storm_render::AssetFormat::A1R5G5B5:case storm_render::AssetFormat::X1R5G5B5:{auto v=u16(p);q[0]=((v>>10)&31)*255/31;q[1]=((v>>5)&31)*255/31;q[2]=(v&31)*255/31;if(desc.Format==storm_render::AssetFormat::A1R5G5B5)q[3]=(v&32768)?255:0;break;}
    case storm_render::AssetFormat::A4R4G4B4:{auto v=u16(p);q[0]=((v>>8)&15)*17;q[1]=((v>>4)&15)*17;q[2]=(v&15)*17;q[3]=(v>>12)*17;break;}
    case storm_render::AssetFormat::L8:case storm_render::AssetFormat::A8L8:q[0]=q[1]=q[2]=p[0];if(desc.Format==storm_render::AssetFormat::A8L8)q[3]=p[1];break;
    case storm_render::AssetFormat::A8:q[0]=q[1]=q[2]=255;q[3]=p[0];break;
    default:unsupported("Surface::upload format");return;
   }
  }
  if(gpu.pixelFormat==MTLPixelFormatBGRA8Unorm)for(size_t i=0;i<rgba.size();i+=4)std::swap(rgba[i],rgba[i+2]);
  [gpu replaceRegion:MTLRegionMake2D(0,0,w,h) mipmapLevel:mip slice:slice withBytes:rgba.data() bytesPerRow:w*4 bytesPerImage:w*h*4];gpuDirty=false;
 }
 storm_render::Result  UnlockRect()override{if(!locked)return storm_render::result::invalidCall;locked=false;if(!readOnly)upload();return storm_render::result::ok;}
};
struct Texture:storm_render::Texture,ResourceParent{
 storm_render::Result  GetDevice(storm_render::Device**out)override{return getOwner(out);}
 const uint64_t cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0;
 std::vector<Surface*>levels;uint32_t lod=0;uint32_t rockK2Variant=0;bool weatherWaterEffect=false,clampBackdropV=false,antiTiling=false;std::string diagnosticLabel;mutable int8_t meaningfulAlphaCache=-1;mutable int8_t coverageMaskCache=-1;
 Texture(id<MTLDevice>dev,uint32_t w,uint32_t h,uint32_t count,uint32_t usage,storm_render::AssetFormat fmt){unsigned max=1;for(unsigned n=std::max(w,h);n>1;n>>=1)max++;count=count?std::min(count,max):max;
  auto*d=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:w height:h mipmapped:count>1];d.mipmapLevelCount=count;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;id<MTLTexture>texture=[dev newTextureWithDescriptor:d];
  for(unsigned i=0;i<count;i++){auto*s=new Surface(dev,std::max(1u,w>>i),std::max(1u,h>>i),fmt);s->gpu=texture;s->mip=i;s->desc.Usage=usage;levels.push_back(s);}
 }
 void bindOwner(storm_render::Device*o){ResourceParent::bindOwner(o);for(auto*s:levels)s->bindOwner(o);}
 ~Texture(){for(auto*s:levels)s->Release();}
 id<MTLTexture>gpu()const{return levels.empty()?nil:levels[0]->gpu;}
 bool meaningfulAlpha()const{
  if(meaningfulAlphaCache>=0)return meaningfulAlphaCache;if(levels.empty())return false;const auto*s=levels[0];std::vector<uint8_t>rgba;
  if(!storm_metal::compat::decodeToRgba(s->desc.Format,s->desc.Width,s->desc.Height,s->bytes.data(),s->bytes.size(),s->pitch,rgba))return meaningfulAlphaCache=0;
  bool transparent=false,opaque=false;for(size_t i=3;i<rgba.size()&&!(transparent&&opaque);i+=4){transparent|=rgba[i]<128;opaque|=rgba[i]>=128;}
  meaningfulAlphaCache=transparent&&opaque;return meaningfulAlphaCache;
 }
 bool coverageMask()const{
  if(coverageMaskCache>=0)return coverageMaskCache;if(levels.empty())return false;const auto*s=levels[0];
  if(s->desc.Format==storm_render::AssetFormat::DXT1){
   const unsigned w=s->desc.Width,h=s->desc.Height,columns=std::max(1u,(w+3)/4),rows=std::max(1u,(h+3)/4);
   if(s->pitch<columns*8||s->bytes.size()<size_t(s->pitch)*rows)return coverageMaskCache=0;
   // DXT1 alpha is binary; reuse the decoder's palette rule without decoding RGB.
   size_t clear=0,pixels=size_t(w)*h;
   for(unsigned by=0;by<rows;++by)for(unsigned bx=0;bx<columns;++bx){
    const auto*block=s->bytes.data()+size_t(by)*s->pitch+size_t(bx)*8;
    if(storm_metal::compat::dxtColors(block,false)[3].a)continue;
    const uint32_t bits=uint32_t(block[4])|uint32_t(block[5])<<8|uint32_t(block[6])<<16|uint32_t(block[7])<<24;
    for(unsigned y=0;y<4&&by*4+y<h;++y)for(unsigned x=0;x<4&&bx*4+x<w;++x)clear+=((bits>>(2*(y*4+x)))&3)==3;
   }
   const size_t solid=pixels-clear;
   return coverageMaskCache=pixels&&clear*50>=pixels&&solid*50>=pixels;
  }
  std::vector<uint8_t>rgba;
  if(!storm_metal::compat::decodeToRgba(s->desc.Format,s->desc.Width,s->desc.Height,s->bytes.data(),s->bytes.size(),s->pitch,rgba))return coverageMaskCache=0;
  size_t clear=0,solid=0,transition=0,pixels=rgba.size()/4;for(size_t i=3;i<rgba.size();i+=4){const uint8_t a=rgba[i];if(a<=24)++clear;else if(a>=231)++solid;else ++transition;}
  // Coverage textures contain real holes and real surfaces. A broad middle band
  // is data (gloss/specular/opacity), not geometry coverage, and must never clip
  // weapons or other opaque materials merely because their alpha channel varies.
  coverageMaskCache=pixels&&clear*50>=pixels&&solid*50>=pixels&&(clear+solid)*5>=pixels*3;return coverageMaskCache;
 }
 storm_render::ResourceKind  GetType()override{return storm_render::ResourceKind::Texture;}
 uint32_t  GetLevelCount()override{return levels.size();}
 uint32_t  SetLOD(uint32_t n)override{auto old=lod;lod=n;return old;}
 uint32_t  GetLOD()override{return lod;}
 storm_render::Result  GetLevelDesc(uint32_t n,storm_render::SurfaceDesc*out)override{return n<levels.size()?levels[n]->GetDesc(out):storm_render::result::invalidCall;}
 storm_render::Result  GetSurfaceLevel(uint32_t n,storm_render::Surface**out)override{if(!out||n>=levels.size())return storm_render::result::invalidCall;*out=levels[n];levels[n]->AddRef();return storm_render::result::ok;}
 storm_render::Result  LockRect(uint32_t n,storm_render::LockedRect*out,const storm_render::Rect*r,uint32_t flags)override{return n<levels.size()?levels[n]->LockRect(out,r,flags):storm_render::result::invalidCall;}
 storm_render::Result  UnlockRect(uint32_t n)override{if(n>=levels.size())return storm_render::result::invalidCall;auto hr=levels[n]->UnlockRect();if(storm_render::succeeded(hr)){++cacheRevision;meaningfulAlphaCache=-1;coverageMaskCache=-1;}return hr;}
};
struct VertexBuffer:storm_render::VertexBuffer,ResourceParent{
 storm_render::Result  GetDevice(storm_render::Device**out)override{return getOwner(out);}
 const uint64_t cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0;
 std::vector<uint8_t>bytes;uint32_t usage=0,fvf=0;bool locked=false,gpuSkinned=false;id<MTLBuffer> gpuSkinningBuffer=nil;id<MTLBuffer> skinningRing[FrameFlightRing::count]={nil,nil,nil};size_t skinningRingBytes[FrameFlightRing::count]={0,0,0};explicit VertexBuffer(uint32_t n):bytes(n){}
 storm_render::ResourceKind  GetType()override{return storm_render::ResourceKind::VertexBuffer;}
 storm_render::Result  Lock(uint32_t offset,uint32_t size,void**out,uint32_t)override{if(!out||locked||offset>bytes.size()||(size&&size>bytes.size()-offset))return storm_render::result::invalidCall;*out=bytes.data()+offset;locked=true;return storm_render::result::ok;}
 storm_render::Result  Unlock()override{if(!locked)return storm_render::result::invalidCall;locked=false;++cacheRevision;return storm_render::result::ok;}
 storm_render::Result  GetDesc(storm_render::VertexBufferDesc*out)override{if(!out)return storm_render::result::invalidCall;*out={};out->Type=storm_render::ResourceKind::VertexBuffer;out->Usage=usage;out->Size=bytes.size();out->FVF=fvf;return storm_render::result::ok;}
};
struct IndexBuffer:storm_render::IndexBuffer,ResourceParent{
 storm_render::Result  GetDevice(storm_render::Device**out)override{return getOwner(out);}
 const uint64_t cacheIdentity=nextBufferIdentity();uint64_t cacheRevision=0;
 std::vector<uint8_t>bytes;storm_render::IndexFormat format;uint32_t usage=0;bool locked=false;IndexBuffer(uint32_t n,storm_render::IndexFormat fmt):bytes(n),format(fmt){}
 storm_render::ResourceKind  GetType()override{return storm_render::ResourceKind::IndexBuffer;}
 storm_render::Result  Lock(uint32_t offset,uint32_t size,void**out,uint32_t)override{if(!out||locked||offset>bytes.size()||(size&&size>bytes.size()-offset))return storm_render::result::invalidCall;*out=bytes.data()+offset;locked=true;return storm_render::result::ok;}
 storm_render::Result  Unlock()override{if(!locked)return storm_render::result::invalidCall;locked=false;++cacheRevision;return storm_render::result::ok;}
 storm_render::Result  GetDesc(storm_render::IndexBufferDesc*out)override{if(!out)return storm_render::result::invalidCall;*out={};out->Format=format;out->Type=storm_render::ResourceKind::IndexBuffer;out->Usage=usage;out->Size=bytes.size();return storm_render::result::ok;}
};
}
namespace sm {
struct CubeTexture:storm_render::CubeTexture,ResourceParent{
 storm_render::Result  GetDevice(storm_render::Device**out)override{return getOwner(out);}
 std::vector<Surface*>faces[6];id<MTLTexture>texture=nil;uint32_t lod=0;
 // The sea producer publishes a coherent face set; SunRoad and ordinary cubes
 // never allocate these optional captures. Their lifetime follows this cube.
 id<MTLTexture>reflectionDepth=nil,reflectionDepthArray=nil,reflectionBackground=nil;
 uint64_t reflectionFrame=0;uint32_t reflectionExpected=0,reflectionDepthMask=0,reflectionBackgroundMask=0;
 float reflectionOrigin[3]{},reflectionProjection[16]{};
 CubeTexture(id<MTLDevice>dev,uint32_t edge,uint32_t count,uint32_t usage,storm_render::AssetFormat fmt){unsigned max=1;for(unsigned n=edge;n>1;n>>=1)max++;count=count?std::min(count,max):max;
  auto*d=[MTLTextureDescriptor textureCubeDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm size:edge mipmapped:count>1];d.mipmapLevelCount=count;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead|MTLTextureUsageRenderTarget;texture=[dev newTextureWithDescriptor:d];
  for(unsigned face=0;face<6;face++)for(unsigned i=0;i<count;i++){auto*s=new Surface(dev,std::max(1u,edge>>i),std::max(1u,edge>>i),fmt);s->gpu=texture;s->mip=i;s->slice=face;s->desc.Usage=usage;faces[face].push_back(s);}
 }
 void bindOwner(storm_render::Device*o){ResourceParent::bindOwner(o);for(auto&f:faces)for(auto*s:f)s->bindOwner(o);}
 ~CubeTexture(){for(auto&f:faces)for(auto*s:f)s->Release();}
 id<MTLTexture>gpu()const{return texture;}
 storm_render::ResourceKind  GetType()override{return storm_render::ResourceKind::CubeTexture;}
 uint32_t  GetLevelCount()override{return faces[0].size();}
 uint32_t  SetLOD(uint32_t n)override{auto old=lod;lod=n;return old;}
 uint32_t  GetLOD()override{return lod;}
 storm_render::Result  GetLevelDesc(uint32_t n,storm_render::SurfaceDesc*out)override{return n<faces[0].size()?faces[0][n]->GetDesc(out):storm_render::result::invalidCall;}
 storm_render::Result  GetCubeMapSurface(storm_render::CubeFace f,uint32_t n,storm_render::Surface**out)override{if(out)*out=nullptr;if(!out||unsigned(f)>=6||n>=faces[unsigned(f)].size())return storm_render::result::invalidCall;*out=faces[unsigned(f)][n];faces[unsigned(f)][n]->AddRef();return storm_render::result::ok;}
 storm_render::Result  LockRect(storm_render::CubeFace f,uint32_t n,storm_render::LockedRect*out,const storm_render::Rect*r,uint32_t flags)override{return unsigned(f)<6&&n<faces[unsigned(f)].size()?faces[unsigned(f)][n]->LockRect(out,r,flags):storm_render::result::invalidCall;}
 storm_render::Result  UnlockRect(storm_render::CubeFace f,uint32_t n)override{return unsigned(f)<6&&n<faces[unsigned(f)].size()?faces[unsigned(f)][n]->UnlockRect():storm_render::result::invalidCall;}
};
struct Volume:storm_render::Volume,ResourceParent{
 storm_render::Result  GetDevice(storm_render::Device**out)override{return getOwner(out);}
 storm_render::ResourceKind GetType()override{return storm_render::ResourceKind::Volume;}
 id<MTLTexture>gpu=nil;storm_render::VolumeDesc desc{};std::vector<uint8_t>bytes;unsigned rowPitch=0,slicePitch=0,mip=0;bool locked=false,readOnly=false;
 Volume(uint32_t w,uint32_t h,uint32_t depth,storm_render::AssetFormat fmt,uint32_t usage){desc.Format=fmt;desc.Type=storm_render::ResourceKind::Volume;desc.Usage=usage;desc.Pool=storm_render::MemoryClass::Managed;desc.Width=w;desc.Height=h;desc.Depth=depth;rowPitch=w*pixelBytes(fmt);slicePitch=rowPitch*h;bytes.resize(size_t(slicePitch)*depth);}
 storm_render::Result  GetDesc(storm_render::VolumeDesc*out)override{if(!out)return storm_render::result::invalidCall;*out=desc;return storm_render::result::ok;}
 storm_render::Result  LockBox(storm_render::LockedBox*out,const storm_render::Box*b,uint32_t flags)override{if(!out||locked)return storm_render::result::invalidCall;if(compressed(desc.Format))return unsupported("Volume::LockBox compressed format");if(b&&(b->Left>=b->Right||b->Top>=b->Bottom||b->Front>=b->Back||b->Right>desc.Width||b->Bottom>desc.Height||b->Back>desc.Depth))return storm_render::result::invalidCall;out->RowPitch=rowPitch;out->SlicePitch=slicePitch;out->pBits=bytes.data()+(b?size_t(b->Front)*slicePitch+size_t(b->Top)*rowPitch+b->Left*pixelBytes(desc.Format):0);locked=true;readOnly=flags&storm_render::lock::readOnly;return storm_render::result::ok;}
 storm_render::Result  UnlockBox()override{if(!locked)return storm_render::result::invalidCall;locked=false;if(readOnly)return storm_render::result::ok;if(desc.Format!=storm_render::AssetFormat::A8R8G8B8&&desc.Format!=storm_render::AssetFormat::X8R8G8B8)return unsupported("Volume::UnlockBox non-BGRA format");if(!gpu)return storm_render::result::invalidCall;std::vector<uint8_t>rgba(bytes);for(size_t n=0;n<rgba.size();n+=4){std::swap(rgba[n],rgba[n+2]);if(desc.Format==storm_render::AssetFormat::X8R8G8B8)rgba[n+3]=255;}[gpu replaceRegion:MTLRegionMake3D(0,0,0,desc.Width,desc.Height,desc.Depth) mipmapLevel:mip slice:0 withBytes:rgba.data() bytesPerRow:rowPitch bytesPerImage:slicePitch];return storm_render::result::ok;}
};
struct VolumeTexture:storm_render::VolumeTexture,ResourceParent{
 storm_render::Result  GetDevice(storm_render::Device**out)override{return getOwner(out);}
 std::vector<Volume*>levels;id<MTLTexture>texture=nil;uint32_t lod=0;
 VolumeTexture(id<MTLDevice>dev,uint32_t w,uint32_t h,uint32_t depth,uint32_t count,uint32_t usage,storm_render::AssetFormat fmt){unsigned max=1;for(unsigned n=std::max({w,h,depth});n>1;n>>=1)max++;count=count?std::min(count,max):max;auto*d=[MTLTextureDescriptor new];d.textureType=MTLTextureType3D;d.pixelFormat=MTLPixelFormatRGBA8Unorm;d.width=w;d.height=h;d.depth=depth;d.mipmapLevelCount=count;d.storageMode=MTLStorageModeShared;d.usage=MTLTextureUsageShaderRead;texture=[dev newTextureWithDescriptor:d];for(unsigned i=0;i<count;i++){auto*v=new Volume(std::max(1u,w>>i),std::max(1u,h>>i),std::max(1u,depth>>i),fmt,usage);v->gpu=texture;v->mip=i;levels.push_back(v);}}
 void bindOwner(storm_render::Device*o){ResourceParent::bindOwner(o);for(auto*v:levels)v->bindOwner(o);}
 ~VolumeTexture(){for(auto*v:levels)v->Release();}
 id<MTLTexture>gpu()const{return texture;}
 storm_render::ResourceKind  GetType()override{return storm_render::ResourceKind::VolumeTexture;}
 uint32_t  GetLevelCount()override{return levels.size();}
 uint32_t  SetLOD(uint32_t n)override{auto old=lod;lod=n;return old;}
 uint32_t  GetLOD()override{return lod;}
 storm_render::Result  GetLevelDesc(uint32_t n,storm_render::VolumeDesc*out)override{return n<levels.size()?levels[n]->GetDesc(out):storm_render::result::invalidCall;}
 storm_render::Result  GetVolumeLevel(uint32_t n,storm_render::Volume**out)override{if(out)*out=nullptr;if(!out||n>=levels.size())return storm_render::result::invalidCall;*out=levels[n];levels[n]->AddRef();return storm_render::result::ok;}
 storm_render::Result  LockBox(uint32_t n,storm_render::LockedBox*out,const storm_render::Box*b,uint32_t flags)override{return n<levels.size()?levels[n]->LockBox(out,b,flags):storm_render::result::invalidCall;}
 storm_render::Result  UnlockBox(uint32_t n)override{return n<levels.size()?levels[n]->UnlockBox():storm_render::result::invalidCall;}
};
}
