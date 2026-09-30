#pragma once
#include <d3d9.h>
#include <cstdio>
#include <set>
#include <string>
inline HRESULT unsupported(const char* method) { static std::set<std::string> seen; if(seen.insert(method).second) std::fprintf(stderr,"[StormMetal] unsupported: %s\n",method); return E_NOTIMPL; }
struct StubIDirect3D9 : IDirect3D9 { virtual ~StubIDirect3D9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
UINT STDMETHODCALLTYPE GetAdapterCount() override { return {}; }
UINT STDMETHODCALLTYPE GetAdapterModeCount(UINT Adapter, D3DFORMAT Format) override { return {}; }
HMONITOR STDMETHODCALLTYPE GetAdapterMonitor(UINT Adapter) override { return {}; }
HRESULT STDMETHODCALLTYPE RegisterSoftwareDevice(void* pInitializeFunction) override { return unsupported("IDirect3D9::RegisterSoftwareDevice"); }
HRESULT STDMETHODCALLTYPE GetAdapterIdentifier(UINT Adapter, DWORD Flags, D3DADAPTER_IDENTIFIER9* pIdentifier) override { return unsupported("IDirect3D9::GetAdapterIdentifier"); }
HRESULT STDMETHODCALLTYPE EnumAdapterModes(UINT Adapter, D3DFORMAT Format, UINT Mode, D3DDISPLAYMODE* pMode) override { return unsupported("IDirect3D9::EnumAdapterModes"); }
HRESULT STDMETHODCALLTYPE GetAdapterDisplayMode(UINT Adapter, D3DDISPLAYMODE* pMode) override { return unsupported("IDirect3D9::GetAdapterDisplayMode"); }
HRESULT STDMETHODCALLTYPE CheckDeviceType(UINT iAdapter, D3DDEVTYPE DevType, D3DFORMAT DisplayFormat, D3DFORMAT BackBufferFormat, WINBOOL bWindowed) override { return unsupported("IDirect3D9::CheckDeviceType"); }
HRESULT STDMETHODCALLTYPE CheckDeviceFormat(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, DWORD Usage, D3DRESOURCETYPE RType, D3DFORMAT CheckFormat) override { return unsupported("IDirect3D9::CheckDeviceFormat"); }
HRESULT STDMETHODCALLTYPE CheckDeviceMultiSampleType(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SurfaceFormat, WINBOOL Windowed, D3DMULTISAMPLE_TYPE MultiSampleType, DWORD* pQualityLevels) override { return unsupported("IDirect3D9::CheckDeviceMultiSampleType"); }
HRESULT STDMETHODCALLTYPE CheckDepthStencilMatch(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT AdapterFormat, D3DFORMAT RenderTargetFormat, D3DFORMAT DepthStencilFormat) override { return unsupported("IDirect3D9::CheckDepthStencilMatch"); }
HRESULT STDMETHODCALLTYPE CheckDeviceFormatConversion(UINT Adapter, D3DDEVTYPE DeviceType, D3DFORMAT SourceFormat, D3DFORMAT TargetFormat) override { return unsupported("IDirect3D9::CheckDeviceFormatConversion"); }
HRESULT STDMETHODCALLTYPE GetDeviceCaps(UINT Adapter, D3DDEVTYPE DeviceType, D3DCAPS9* pCaps) override { return unsupported("IDirect3D9::GetDeviceCaps"); }
HRESULT STDMETHODCALLTYPE CreateDevice(UINT Adapter, D3DDEVTYPE DeviceType, HWND hFocusWindow, DWORD BehaviorFlags, D3DPRESENT_PARAMETERS* pPresentationParameters, struct IDirect3DDevice9** ppReturnedDeviceInterface) override { if(ppReturnedDeviceInterface)*ppReturnedDeviceInterface=nullptr; return unsupported("IDirect3D9::CreateDevice"); }
};
struct StubIDirect3DVolume9 : IDirect3DVolume9 { virtual ~StubIDirect3DVolume9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DVolume9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DVolume9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DVolume9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DVolume9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) override { if(ppContainer)*ppContainer=nullptr; return unsupported("IDirect3DVolume9::GetContainer"); }
HRESULT STDMETHODCALLTYPE GetDesc(D3DVOLUME_DESC* pDesc) override { return unsupported("IDirect3DVolume9::GetDesc"); }
HRESULT STDMETHODCALLTYPE LockBox(D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags) override { return unsupported("IDirect3DVolume9::LockBox"); }
HRESULT STDMETHODCALLTYPE UnlockBox() override { return unsupported("IDirect3DVolume9::UnlockBox"); }
};
struct StubIDirect3DSwapChain9 : IDirect3DSwapChain9 { virtual ~StubIDirect3DSwapChain9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void **ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
HRESULT STDMETHODCALLTYPE Present(const RECT *src_rect, const RECT *dst_rect, HWND dst_window_override,
            const RGNDATA *dirty_region, DWORD flags) override { return unsupported("IDirect3DSwapChain9::Present"); }
HRESULT STDMETHODCALLTYPE GetFrontBufferData(struct IDirect3DSurface9 *pDestSurface) override { return unsupported("IDirect3DSwapChain9::GetFrontBufferData"); }
HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, struct IDirect3DSurface9 **ppBackBuffer) override { if(ppBackBuffer)*ppBackBuffer=nullptr; return unsupported("IDirect3DSwapChain9::GetBackBuffer"); }
HRESULT STDMETHODCALLTYPE GetRasterStatus(D3DRASTER_STATUS *pRasterStatus) override { return unsupported("IDirect3DSwapChain9::GetRasterStatus"); }
HRESULT STDMETHODCALLTYPE GetDisplayMode(D3DDISPLAYMODE *pMode) override { return unsupported("IDirect3DSwapChain9::GetDisplayMode"); }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9 **ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DSwapChain9::GetDevice"); }
HRESULT STDMETHODCALLTYPE GetPresentParameters(D3DPRESENT_PARAMETERS *pPresentationParameters) override { return unsupported("IDirect3DSwapChain9::GetPresentParameters"); }
};
struct StubIDirect3DResource9 : IDirect3DResource9 { virtual ~StubIDirect3DResource9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DResource9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DResource9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DResource9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DResource9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DResource9::FreePrivateData"); }
};
struct StubIDirect3DSurface9 : IDirect3DSurface9 { virtual ~StubIDirect3DSurface9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DSurface9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DSurface9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DSurface9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DSurface9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DSurface9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE GetContainer(REFIID riid, void** ppContainer) override { if(ppContainer)*ppContainer=nullptr; return unsupported("IDirect3DSurface9::GetContainer"); }
HRESULT STDMETHODCALLTYPE GetDesc(D3DSURFACE_DESC* pDesc) override { return unsupported("IDirect3DSurface9::GetDesc"); }
HRESULT STDMETHODCALLTYPE LockRect(D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags) override { return unsupported("IDirect3DSurface9::LockRect"); }
HRESULT STDMETHODCALLTYPE UnlockRect() override { return unsupported("IDirect3DSurface9::UnlockRect"); }
HRESULT STDMETHODCALLTYPE GetDC(HDC* phdc) override { return unsupported("IDirect3DSurface9::GetDC"); }
HRESULT STDMETHODCALLTYPE ReleaseDC(HDC hdc) override { return unsupported("IDirect3DSurface9::ReleaseDC"); }
};
struct StubIDirect3DVertexBuffer9 : IDirect3DVertexBuffer9 { virtual ~StubIDirect3DVertexBuffer9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DVertexBuffer9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DVertexBuffer9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DVertexBuffer9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DVertexBuffer9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DVertexBuffer9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) override { if(ppbData)*ppbData=nullptr; return unsupported("IDirect3DVertexBuffer9::Lock"); }
HRESULT STDMETHODCALLTYPE Unlock() override { return unsupported("IDirect3DVertexBuffer9::Unlock"); }
HRESULT STDMETHODCALLTYPE GetDesc(D3DVERTEXBUFFER_DESC* pDesc) override { return unsupported("IDirect3DVertexBuffer9::GetDesc"); }
};
struct StubIDirect3DIndexBuffer9 : IDirect3DIndexBuffer9 { virtual ~StubIDirect3DIndexBuffer9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DIndexBuffer9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DIndexBuffer9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DIndexBuffer9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DIndexBuffer9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DIndexBuffer9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE Lock(UINT OffsetToLock, UINT SizeToLock, void** ppbData, DWORD Flags) override { if(ppbData)*ppbData=nullptr; return unsupported("IDirect3DIndexBuffer9::Lock"); }
HRESULT STDMETHODCALLTYPE Unlock() override { return unsupported("IDirect3DIndexBuffer9::Unlock"); }
HRESULT STDMETHODCALLTYPE GetDesc(D3DINDEXBUFFER_DESC* pDesc) override { return unsupported("IDirect3DIndexBuffer9::GetDesc"); }
};
struct StubIDirect3DBaseTexture9 : IDirect3DBaseTexture9 { virtual ~StubIDirect3DBaseTexture9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DBaseTexture9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetLOD() override { return {}; }
DWORD STDMETHODCALLTYPE GetLevelCount() override { return {}; }
D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return {}; }
void STDMETHODCALLTYPE GenerateMipSubLevels() override { unsupported("IDirect3DBaseTexture9::GenerateMipSubLevels"); }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DBaseTexture9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DBaseTexture9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DBaseTexture9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DBaseTexture9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) override { return unsupported("IDirect3DBaseTexture9::SetAutoGenFilterType"); }
};
struct StubIDirect3DCubeTexture9 : IDirect3DCubeTexture9 { virtual ~StubIDirect3DCubeTexture9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DCubeTexture9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetLOD() override { return {}; }
DWORD STDMETHODCALLTYPE GetLevelCount() override { return {}; }
D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return {}; }
void STDMETHODCALLTYPE GenerateMipSubLevels() override { unsupported("IDirect3DCubeTexture9::GenerateMipSubLevels"); }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DCubeTexture9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DCubeTexture9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DCubeTexture9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DCubeTexture9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) override { return unsupported("IDirect3DCubeTexture9::SetAutoGenFilterType"); }
HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level,D3DSURFACE_DESC* pDesc) override { return unsupported("IDirect3DCubeTexture9::GetLevelDesc"); }
HRESULT STDMETHODCALLTYPE GetCubeMapSurface(D3DCUBEMAP_FACES FaceType, UINT Level, IDirect3DSurface9** ppCubeMapSurface) override { if(ppCubeMapSurface)*ppCubeMapSurface=nullptr; return unsupported("IDirect3DCubeTexture9::GetCubeMapSurface"); }
HRESULT STDMETHODCALLTYPE LockRect(D3DCUBEMAP_FACES face, UINT level,
            D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags) override { return unsupported("IDirect3DCubeTexture9::LockRect"); }
HRESULT STDMETHODCALLTYPE UnlockRect(D3DCUBEMAP_FACES FaceType, UINT Level) override { return unsupported("IDirect3DCubeTexture9::UnlockRect"); }
HRESULT STDMETHODCALLTYPE AddDirtyRect(D3DCUBEMAP_FACES face, const RECT *dirty_rect) override { return unsupported("IDirect3DCubeTexture9::AddDirtyRect"); }
};
struct StubIDirect3DTexture9 : IDirect3DTexture9 { virtual ~StubIDirect3DTexture9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DTexture9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetLOD() override { return {}; }
DWORD STDMETHODCALLTYPE GetLevelCount() override { return {}; }
D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return {}; }
void STDMETHODCALLTYPE GenerateMipSubLevels() override { unsupported("IDirect3DTexture9::GenerateMipSubLevels"); }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DTexture9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DTexture9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DTexture9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DTexture9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) override { return unsupported("IDirect3DTexture9::SetAutoGenFilterType"); }
HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DSURFACE_DESC* pDesc) override { return unsupported("IDirect3DTexture9::GetLevelDesc"); }
HRESULT STDMETHODCALLTYPE GetSurfaceLevel(UINT Level, IDirect3DSurface9** ppSurfaceLevel) override { if(ppSurfaceLevel)*ppSurfaceLevel=nullptr; return unsupported("IDirect3DTexture9::GetSurfaceLevel"); }
HRESULT STDMETHODCALLTYPE LockRect(UINT level, D3DLOCKED_RECT *locked_rect, const RECT *rect, DWORD flags) override { return unsupported("IDirect3DTexture9::LockRect"); }
HRESULT STDMETHODCALLTYPE UnlockRect(UINT Level) override { return unsupported("IDirect3DTexture9::UnlockRect"); }
HRESULT STDMETHODCALLTYPE AddDirtyRect(const RECT *dirty_rect) override { return unsupported("IDirect3DTexture9::AddDirtyRect"); }
};
struct StubIDirect3DVolumeTexture9 : IDirect3DVolumeTexture9 { virtual ~StubIDirect3DVolumeTexture9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
DWORD STDMETHODCALLTYPE SetPriority(DWORD PriorityNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetPriority() override { return {}; }
void STDMETHODCALLTYPE PreLoad() override { unsupported("IDirect3DVolumeTexture9::PreLoad"); }
D3DRESOURCETYPE STDMETHODCALLTYPE GetType() override { return {}; }
DWORD STDMETHODCALLTYPE SetLOD(DWORD LODNew) override { return {}; }
DWORD STDMETHODCALLTYPE GetLOD() override { return {}; }
DWORD STDMETHODCALLTYPE GetLevelCount() override { return {}; }
D3DTEXTUREFILTERTYPE STDMETHODCALLTYPE GetAutoGenFilterType() override { return {}; }
void STDMETHODCALLTYPE GenerateMipSubLevels() override { unsupported("IDirect3DVolumeTexture9::GenerateMipSubLevels"); }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DVolumeTexture9::GetDevice"); }
HRESULT STDMETHODCALLTYPE SetPrivateData(REFGUID guid, const void *data, DWORD data_size, DWORD flags) override { return unsupported("IDirect3DVolumeTexture9::SetPrivateData"); }
HRESULT STDMETHODCALLTYPE GetPrivateData(REFGUID refguid, void* pData, DWORD* pSizeOfData) override { return unsupported("IDirect3DVolumeTexture9::GetPrivateData"); }
HRESULT STDMETHODCALLTYPE FreePrivateData(REFGUID refguid) override { return unsupported("IDirect3DVolumeTexture9::FreePrivateData"); }
HRESULT STDMETHODCALLTYPE SetAutoGenFilterType(D3DTEXTUREFILTERTYPE FilterType) override { return unsupported("IDirect3DVolumeTexture9::SetAutoGenFilterType"); }
HRESULT STDMETHODCALLTYPE GetLevelDesc(UINT Level, D3DVOLUME_DESC *pDesc) override { return unsupported("IDirect3DVolumeTexture9::GetLevelDesc"); }
HRESULT STDMETHODCALLTYPE GetVolumeLevel(UINT Level, IDirect3DVolume9** ppVolumeLevel) override { if(ppVolumeLevel)*ppVolumeLevel=nullptr; return unsupported("IDirect3DVolumeTexture9::GetVolumeLevel"); }
HRESULT STDMETHODCALLTYPE LockBox(UINT level, D3DLOCKED_BOX *locked_box, const D3DBOX *box, DWORD flags) override { return unsupported("IDirect3DVolumeTexture9::LockBox"); }
HRESULT STDMETHODCALLTYPE UnlockBox(UINT Level) override { return unsupported("IDirect3DVolumeTexture9::UnlockBox"); }
HRESULT STDMETHODCALLTYPE AddDirtyBox(const D3DBOX *dirty_box) override { return unsupported("IDirect3DVolumeTexture9::AddDirtyBox"); }
};
struct StubIDirect3DVertexDeclaration9 : IDirect3DVertexDeclaration9 { virtual ~StubIDirect3DVertexDeclaration9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DVertexDeclaration9::GetDevice"); }
HRESULT STDMETHODCALLTYPE GetDeclaration(D3DVERTEXELEMENT9*, UINT* pNumElements) override { return unsupported("IDirect3DVertexDeclaration9::GetDeclaration"); }
};
struct StubIDirect3DVertexShader9 : IDirect3DVertexShader9 { virtual ~StubIDirect3DVertexShader9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DVertexShader9::GetDevice"); }
HRESULT STDMETHODCALLTYPE GetFunction(void*, UINT* pSizeOfData) override { return unsupported("IDirect3DVertexShader9::GetFunction"); }
};
struct StubIDirect3DPixelShader9 : IDirect3DPixelShader9 { virtual ~StubIDirect3DPixelShader9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DPixelShader9::GetDevice"); }
HRESULT STDMETHODCALLTYPE GetFunction(void*, UINT* pSizeOfData) override { return unsupported("IDirect3DPixelShader9::GetFunction"); }
};
struct StubIDirect3DStateBlock9 : IDirect3DStateBlock9 { virtual ~StubIDirect3DStateBlock9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DStateBlock9::GetDevice"); }
HRESULT STDMETHODCALLTYPE Capture() override { return unsupported("IDirect3DStateBlock9::Capture"); }
HRESULT STDMETHODCALLTYPE Apply() override { return unsupported("IDirect3DStateBlock9::Apply"); }
};
struct StubIDirect3DQuery9 : IDirect3DQuery9 { virtual ~StubIDirect3DQuery9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
D3DQUERYTYPE STDMETHODCALLTYPE GetType() override { return {}; }
DWORD STDMETHODCALLTYPE GetDataSize() override { return {}; }
HRESULT STDMETHODCALLTYPE GetDevice(struct IDirect3DDevice9** ppDevice) override { if(ppDevice)*ppDevice=nullptr; return unsupported("IDirect3DQuery9::GetDevice"); }
HRESULT STDMETHODCALLTYPE Issue(DWORD dwIssueFlags) override { return unsupported("IDirect3DQuery9::Issue"); }
HRESULT STDMETHODCALLTYPE GetData(void* pData, DWORD dwSize, DWORD dwGetDataFlags) override { return unsupported("IDirect3DQuery9::GetData"); }
};
struct StubIDirect3DDevice9 : IDirect3DDevice9 { virtual ~StubIDirect3DDevice9() = default; ULONG refs=1;
HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override { if(ppvObject)*ppvObject=nullptr; return E_NOINTERFACE; }
ULONG STDMETHODCALLTYPE AddRef() override { return ++refs; }
ULONG STDMETHODCALLTYPE Release() override { auto n=--refs; if(!n) delete this; return n; }
UINT STDMETHODCALLTYPE GetAvailableTextureMem() override { return {}; }
void STDMETHODCALLTYPE SetCursorPosition(int X,int Y, DWORD Flags) override { unsupported("IDirect3DDevice9::SetCursorPosition"); }
WINBOOL STDMETHODCALLTYPE ShowCursor(WINBOOL bShow) override { return {}; }
UINT STDMETHODCALLTYPE GetNumberOfSwapChains() override { return {}; }
void STDMETHODCALLTYPE SetGammaRamp(UINT swapchain_idx, DWORD flags, const D3DGAMMARAMP *ramp) override { unsupported("IDirect3DDevice9::SetGammaRamp"); }
void STDMETHODCALLTYPE GetGammaRamp(UINT iSwapChain, D3DGAMMARAMP* pRamp) override { unsupported("IDirect3DDevice9::GetGammaRamp"); }
WINBOOL STDMETHODCALLTYPE GetSoftwareVertexProcessing() override { return {}; }
float STDMETHODCALLTYPE GetNPatchMode() override { return {}; }
HRESULT STDMETHODCALLTYPE TestCooperativeLevel() override { return unsupported("IDirect3DDevice9::TestCooperativeLevel"); }
HRESULT STDMETHODCALLTYPE EvictManagedResources() override { return unsupported("IDirect3DDevice9::EvictManagedResources"); }
HRESULT STDMETHODCALLTYPE GetDirect3D(IDirect3D9** ppD3D9) override { if(ppD3D9)*ppD3D9=nullptr; return unsupported("IDirect3DDevice9::GetDirect3D"); }
HRESULT STDMETHODCALLTYPE GetDeviceCaps(D3DCAPS9* pCaps) override { return unsupported("IDirect3DDevice9::GetDeviceCaps"); }
HRESULT STDMETHODCALLTYPE GetDisplayMode(UINT iSwapChain, D3DDISPLAYMODE* pMode) override { return unsupported("IDirect3DDevice9::GetDisplayMode"); }
HRESULT STDMETHODCALLTYPE GetCreationParameters(D3DDEVICE_CREATION_PARAMETERS *pParameters) override { return unsupported("IDirect3DDevice9::GetCreationParameters"); }
HRESULT STDMETHODCALLTYPE SetCursorProperties(UINT XHotSpot, UINT YHotSpot, IDirect3DSurface9* pCursorBitmap) override { return unsupported("IDirect3DDevice9::SetCursorProperties"); }
HRESULT STDMETHODCALLTYPE CreateAdditionalSwapChain(D3DPRESENT_PARAMETERS* pPresentationParameters, IDirect3DSwapChain9** pSwapChain) override { if(pSwapChain)*pSwapChain=nullptr; return unsupported("IDirect3DDevice9::CreateAdditionalSwapChain"); }
HRESULT STDMETHODCALLTYPE GetSwapChain(UINT iSwapChain, IDirect3DSwapChain9** pSwapChain) override { if(pSwapChain)*pSwapChain=nullptr; return unsupported("IDirect3DDevice9::GetSwapChain"); }
HRESULT STDMETHODCALLTYPE Reset(D3DPRESENT_PARAMETERS* pPresentationParameters) override { return unsupported("IDirect3DDevice9::Reset"); }
HRESULT STDMETHODCALLTYPE Present(const RECT *src_rect, const RECT *dst_rect,
            HWND dst_window_override, const RGNDATA *dirty_region) override { return unsupported("IDirect3DDevice9::Present"); }
HRESULT STDMETHODCALLTYPE GetBackBuffer(UINT iSwapChain, UINT iBackBuffer, D3DBACKBUFFER_TYPE Type, IDirect3DSurface9** ppBackBuffer) override { if(ppBackBuffer)*ppBackBuffer=nullptr; return unsupported("IDirect3DDevice9::GetBackBuffer"); }
HRESULT STDMETHODCALLTYPE GetRasterStatus(UINT iSwapChain, D3DRASTER_STATUS* pRasterStatus) override { return unsupported("IDirect3DDevice9::GetRasterStatus"); }
HRESULT STDMETHODCALLTYPE SetDialogBoxMode(WINBOOL bEnableDialogs) override { return unsupported("IDirect3DDevice9::SetDialogBoxMode"); }
HRESULT STDMETHODCALLTYPE CreateTexture(UINT Width, UINT Height, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DTexture9** ppTexture, HANDLE* pSharedHandle) override { if(ppTexture)*ppTexture=nullptr; return unsupported("IDirect3DDevice9::CreateTexture"); }
HRESULT STDMETHODCALLTYPE CreateVolumeTexture(UINT Width, UINT Height, UINT Depth, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DVolumeTexture9** ppVolumeTexture, HANDLE* pSharedHandle) override { if(ppVolumeTexture)*ppVolumeTexture=nullptr; return unsupported("IDirect3DDevice9::CreateVolumeTexture"); }
HRESULT STDMETHODCALLTYPE CreateCubeTexture(UINT EdgeLength, UINT Levels, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DCubeTexture9** ppCubeTexture, HANDLE* pSharedHandle) override { if(ppCubeTexture)*ppCubeTexture=nullptr; return unsupported("IDirect3DDevice9::CreateCubeTexture"); }
HRESULT STDMETHODCALLTYPE CreateVertexBuffer(UINT Length, DWORD Usage, DWORD FVF, D3DPOOL Pool, IDirect3DVertexBuffer9** ppVertexBuffer, HANDLE* pSharedHandle) override { if(ppVertexBuffer)*ppVertexBuffer=nullptr; return unsupported("IDirect3DDevice9::CreateVertexBuffer"); }
HRESULT STDMETHODCALLTYPE CreateIndexBuffer(UINT Length, DWORD Usage, D3DFORMAT Format, D3DPOOL Pool, IDirect3DIndexBuffer9** ppIndexBuffer, HANDLE* pSharedHandle) override { if(ppIndexBuffer)*ppIndexBuffer=nullptr; return unsupported("IDirect3DDevice9::CreateIndexBuffer"); }
HRESULT STDMETHODCALLTYPE CreateRenderTarget(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Lockable, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override { if(ppSurface)*ppSurface=nullptr; return unsupported("IDirect3DDevice9::CreateRenderTarget"); }
HRESULT STDMETHODCALLTYPE CreateDepthStencilSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DMULTISAMPLE_TYPE MultiSample, DWORD MultisampleQuality, WINBOOL Discard, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override { if(ppSurface)*ppSurface=nullptr; return unsupported("IDirect3DDevice9::CreateDepthStencilSurface"); }
HRESULT STDMETHODCALLTYPE UpdateSurface(IDirect3DSurface9 *src_surface, const RECT *src_rect,
            IDirect3DSurface9 *dst_surface, const POINT *dst_point) override { return unsupported("IDirect3DDevice9::UpdateSurface"); }
HRESULT STDMETHODCALLTYPE UpdateTexture(IDirect3DBaseTexture9* pSourceTexture, IDirect3DBaseTexture9* pDestinationTexture) override { return unsupported("IDirect3DDevice9::UpdateTexture"); }
HRESULT STDMETHODCALLTYPE GetRenderTargetData(IDirect3DSurface9* pRenderTarget, IDirect3DSurface9* pDestSurface) override { return unsupported("IDirect3DDevice9::GetRenderTargetData"); }
HRESULT STDMETHODCALLTYPE GetFrontBufferData(UINT iSwapChain, IDirect3DSurface9* pDestSurface) override { return unsupported("IDirect3DDevice9::GetFrontBufferData"); }
HRESULT STDMETHODCALLTYPE StretchRect(IDirect3DSurface9 *src_surface, const RECT *src_rect,
            IDirect3DSurface9 *dst_surface, const RECT *dst_rect, D3DTEXTUREFILTERTYPE filter) override { return unsupported("IDirect3DDevice9::StretchRect"); }
HRESULT STDMETHODCALLTYPE ColorFill(IDirect3DSurface9 *surface, const RECT *rect, D3DCOLOR color) override { return unsupported("IDirect3DDevice9::ColorFill"); }
HRESULT STDMETHODCALLTYPE CreateOffscreenPlainSurface(UINT Width, UINT Height, D3DFORMAT Format, D3DPOOL Pool, IDirect3DSurface9** ppSurface, HANDLE* pSharedHandle) override { if(ppSurface)*ppSurface=nullptr; return unsupported("IDirect3DDevice9::CreateOffscreenPlainSurface"); }
HRESULT STDMETHODCALLTYPE SetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9* pRenderTarget) override { return unsupported("IDirect3DDevice9::SetRenderTarget"); }
HRESULT STDMETHODCALLTYPE GetRenderTarget(DWORD RenderTargetIndex, IDirect3DSurface9** ppRenderTarget) override { if(ppRenderTarget)*ppRenderTarget=nullptr; return unsupported("IDirect3DDevice9::GetRenderTarget"); }
HRESULT STDMETHODCALLTYPE SetDepthStencilSurface(IDirect3DSurface9* pNewZStencil) override { return unsupported("IDirect3DDevice9::SetDepthStencilSurface"); }
HRESULT STDMETHODCALLTYPE GetDepthStencilSurface(IDirect3DSurface9** ppZStencilSurface) override { if(ppZStencilSurface)*ppZStencilSurface=nullptr; return unsupported("IDirect3DDevice9::GetDepthStencilSurface"); }
HRESULT STDMETHODCALLTYPE BeginScene() override { return unsupported("IDirect3DDevice9::BeginScene"); }
HRESULT STDMETHODCALLTYPE EndScene() override { return unsupported("IDirect3DDevice9::EndScene"); }
HRESULT STDMETHODCALLTYPE Clear(DWORD rect_count, const D3DRECT *rects, DWORD flags,
            D3DCOLOR color, float z, DWORD stencil) override { return unsupported("IDirect3DDevice9::Clear"); }
HRESULT STDMETHODCALLTYPE SetTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix) override { return unsupported("IDirect3DDevice9::SetTransform"); }
HRESULT STDMETHODCALLTYPE GetTransform(D3DTRANSFORMSTATETYPE State, D3DMATRIX* pMatrix) override { return unsupported("IDirect3DDevice9::GetTransform"); }
HRESULT STDMETHODCALLTYPE MultiplyTransform(D3DTRANSFORMSTATETYPE state, const D3DMATRIX *matrix) override { return unsupported("IDirect3DDevice9::MultiplyTransform"); }
HRESULT STDMETHODCALLTYPE SetViewport(const D3DVIEWPORT9 *viewport) override { return unsupported("IDirect3DDevice9::SetViewport"); }
HRESULT STDMETHODCALLTYPE GetViewport(D3DVIEWPORT9* pViewport) override { return unsupported("IDirect3DDevice9::GetViewport"); }
HRESULT STDMETHODCALLTYPE SetMaterial(const D3DMATERIAL9 *material) override { return unsupported("IDirect3DDevice9::SetMaterial"); }
HRESULT STDMETHODCALLTYPE GetMaterial(D3DMATERIAL9* pMaterial) override { return unsupported("IDirect3DDevice9::GetMaterial"); }
HRESULT STDMETHODCALLTYPE SetLight(DWORD index, const D3DLIGHT9 *light) override { return unsupported("IDirect3DDevice9::SetLight"); }
HRESULT STDMETHODCALLTYPE GetLight(DWORD Index, D3DLIGHT9*) override { return unsupported("IDirect3DDevice9::GetLight"); }
HRESULT STDMETHODCALLTYPE LightEnable(DWORD Index, WINBOOL Enable) override { return unsupported("IDirect3DDevice9::LightEnable"); }
HRESULT STDMETHODCALLTYPE GetLightEnable(DWORD Index, WINBOOL* pEnable) override { return unsupported("IDirect3DDevice9::GetLightEnable"); }
HRESULT STDMETHODCALLTYPE SetClipPlane(DWORD index, const float *plane) override { return unsupported("IDirect3DDevice9::SetClipPlane"); }
HRESULT STDMETHODCALLTYPE GetClipPlane(DWORD Index, float* pPlane) override { return unsupported("IDirect3DDevice9::GetClipPlane"); }
HRESULT STDMETHODCALLTYPE SetRenderState(D3DRENDERSTATETYPE State, DWORD Value) override { return unsupported("IDirect3DDevice9::SetRenderState"); }
HRESULT STDMETHODCALLTYPE GetRenderState(D3DRENDERSTATETYPE State, DWORD* pValue) override { return unsupported("IDirect3DDevice9::GetRenderState"); }
HRESULT STDMETHODCALLTYPE CreateStateBlock(D3DSTATEBLOCKTYPE Type, IDirect3DStateBlock9** ppSB) override { if(ppSB)*ppSB=nullptr; return unsupported("IDirect3DDevice9::CreateStateBlock"); }
HRESULT STDMETHODCALLTYPE BeginStateBlock() override { return unsupported("IDirect3DDevice9::BeginStateBlock"); }
HRESULT STDMETHODCALLTYPE EndStateBlock(IDirect3DStateBlock9** ppSB) override { if(ppSB)*ppSB=nullptr; return unsupported("IDirect3DDevice9::EndStateBlock"); }
HRESULT STDMETHODCALLTYPE SetClipStatus(const D3DCLIPSTATUS9 *clip_status) override { return unsupported("IDirect3DDevice9::SetClipStatus"); }
HRESULT STDMETHODCALLTYPE GetClipStatus(D3DCLIPSTATUS9* pClipStatus) override { return unsupported("IDirect3DDevice9::GetClipStatus"); }
HRESULT STDMETHODCALLTYPE GetTexture(DWORD Stage, IDirect3DBaseTexture9** ppTexture) override { if(ppTexture)*ppTexture=nullptr; return unsupported("IDirect3DDevice9::GetTexture"); }
HRESULT STDMETHODCALLTYPE SetTexture(DWORD Stage, IDirect3DBaseTexture9* pTexture) override { return unsupported("IDirect3DDevice9::SetTexture"); }
HRESULT STDMETHODCALLTYPE GetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD* pValue) override { return unsupported("IDirect3DDevice9::GetTextureStageState"); }
HRESULT STDMETHODCALLTYPE SetTextureStageState(DWORD Stage, D3DTEXTURESTAGESTATETYPE Type, DWORD Value) override { return unsupported("IDirect3DDevice9::SetTextureStageState"); }
HRESULT STDMETHODCALLTYPE GetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD* pValue) override { return unsupported("IDirect3DDevice9::GetSamplerState"); }
HRESULT STDMETHODCALLTYPE SetSamplerState(DWORD Sampler, D3DSAMPLERSTATETYPE Type, DWORD Value) override { return unsupported("IDirect3DDevice9::SetSamplerState"); }
HRESULT STDMETHODCALLTYPE ValidateDevice(DWORD* pNumPasses) override { return unsupported("IDirect3DDevice9::ValidateDevice"); }
HRESULT STDMETHODCALLTYPE SetPaletteEntries(UINT palette_idx, const PALETTEENTRY *entries) override { return unsupported("IDirect3DDevice9::SetPaletteEntries"); }
HRESULT STDMETHODCALLTYPE GetPaletteEntries(UINT PaletteNumber,PALETTEENTRY* pEntries) override { return unsupported("IDirect3DDevice9::GetPaletteEntries"); }
HRESULT STDMETHODCALLTYPE SetCurrentTexturePalette(UINT PaletteNumber) override { return unsupported("IDirect3DDevice9::SetCurrentTexturePalette"); }
HRESULT STDMETHODCALLTYPE GetCurrentTexturePalette(UINT *PaletteNumber) override { return unsupported("IDirect3DDevice9::GetCurrentTexturePalette"); }
HRESULT STDMETHODCALLTYPE SetScissorRect(const RECT *rect) override { return unsupported("IDirect3DDevice9::SetScissorRect"); }
HRESULT STDMETHODCALLTYPE GetScissorRect(RECT* pRect) override { return unsupported("IDirect3DDevice9::GetScissorRect"); }
HRESULT STDMETHODCALLTYPE SetSoftwareVertexProcessing(WINBOOL bSoftware) override { return unsupported("IDirect3DDevice9::SetSoftwareVertexProcessing"); }
HRESULT STDMETHODCALLTYPE SetNPatchMode(float nSegments) override { return unsupported("IDirect3DDevice9::SetNPatchMode"); }
HRESULT STDMETHODCALLTYPE DrawPrimitive(D3DPRIMITIVETYPE PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override { return unsupported("IDirect3DDevice9::DrawPrimitive"); }
HRESULT STDMETHODCALLTYPE DrawIndexedPrimitive(D3DPRIMITIVETYPE, INT BaseVertexIndex, UINT MinVertexIndex, UINT NumVertices, UINT startIndex, UINT primCount) override { return unsupported("IDirect3DDevice9::DrawIndexedPrimitive"); }
HRESULT STDMETHODCALLTYPE DrawPrimitiveUP(D3DPRIMITIVETYPE primitive_type,
            UINT primitive_count, const void *data, UINT stride) override { return unsupported("IDirect3DDevice9::DrawPrimitiveUP"); }
HRESULT STDMETHODCALLTYPE DrawIndexedPrimitiveUP(D3DPRIMITIVETYPE primitive_type, UINT min_vertex_idx, UINT vertex_count,
            UINT primitive_count, const void *index_data, D3DFORMAT index_format, const void *data, UINT stride) override { return unsupported("IDirect3DDevice9::DrawIndexedPrimitiveUP"); }
HRESULT STDMETHODCALLTYPE ProcessVertices(UINT SrcStartIndex, UINT DestIndex, UINT VertexCount, IDirect3DVertexBuffer9* pDestBuffer, IDirect3DVertexDeclaration9* pVertexDecl, DWORD Flags) override { return unsupported("IDirect3DDevice9::ProcessVertices"); }
HRESULT STDMETHODCALLTYPE CreateVertexDeclaration(const D3DVERTEXELEMENT9 *elements,
            IDirect3DVertexDeclaration9 **declaration) override { if(declaration)*declaration=nullptr; return unsupported("IDirect3DDevice9::CreateVertexDeclaration"); }
HRESULT STDMETHODCALLTYPE SetVertexDeclaration(IDirect3DVertexDeclaration9* pDecl) override { return unsupported("IDirect3DDevice9::SetVertexDeclaration"); }
HRESULT STDMETHODCALLTYPE GetVertexDeclaration(IDirect3DVertexDeclaration9** ppDecl) override { if(ppDecl)*ppDecl=nullptr; return unsupported("IDirect3DDevice9::GetVertexDeclaration"); }
HRESULT STDMETHODCALLTYPE SetFVF(DWORD FVF) override { return unsupported("IDirect3DDevice9::SetFVF"); }
HRESULT STDMETHODCALLTYPE GetFVF(DWORD* pFVF) override { return unsupported("IDirect3DDevice9::GetFVF"); }
HRESULT STDMETHODCALLTYPE CreateVertexShader(const DWORD *byte_code, IDirect3DVertexShader9 **shader) override { if(shader)*shader=nullptr; return unsupported("IDirect3DDevice9::CreateVertexShader"); }
HRESULT STDMETHODCALLTYPE SetVertexShader(IDirect3DVertexShader9* pShader) override { return unsupported("IDirect3DDevice9::SetVertexShader"); }
HRESULT STDMETHODCALLTYPE GetVertexShader(IDirect3DVertexShader9** ppShader) override { if(ppShader)*ppShader=nullptr; return unsupported("IDirect3DDevice9::GetVertexShader"); }
HRESULT STDMETHODCALLTYPE SetVertexShaderConstantF(UINT reg_idx, const float *data, UINT count) override { return unsupported("IDirect3DDevice9::SetVertexShaderConstantF"); }
HRESULT STDMETHODCALLTYPE GetVertexShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override { return unsupported("IDirect3DDevice9::GetVertexShaderConstantF"); }
HRESULT STDMETHODCALLTYPE SetVertexShaderConstantI(UINT reg_idx, const int *data, UINT count) override { return unsupported("IDirect3DDevice9::SetVertexShaderConstantI"); }
HRESULT STDMETHODCALLTYPE GetVertexShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override { return unsupported("IDirect3DDevice9::GetVertexShaderConstantI"); }
HRESULT STDMETHODCALLTYPE SetVertexShaderConstantB(UINT reg_idx, const WINBOOL *data, UINT count) override { return unsupported("IDirect3DDevice9::SetVertexShaderConstantB"); }
HRESULT STDMETHODCALLTYPE GetVertexShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount) override { return unsupported("IDirect3DDevice9::GetVertexShaderConstantB"); }
HRESULT STDMETHODCALLTYPE SetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9* pStreamData, UINT OffsetInBytes, UINT Stride) override { return unsupported("IDirect3DDevice9::SetStreamSource"); }
HRESULT STDMETHODCALLTYPE GetStreamSource(UINT StreamNumber, IDirect3DVertexBuffer9** ppStreamData, UINT* OffsetInBytes, UINT* pStride) override { if(ppStreamData)*ppStreamData=nullptr; return unsupported("IDirect3DDevice9::GetStreamSource"); }
HRESULT STDMETHODCALLTYPE SetStreamSourceFreq(UINT StreamNumber, UINT Divider) override { return unsupported("IDirect3DDevice9::SetStreamSourceFreq"); }
HRESULT STDMETHODCALLTYPE GetStreamSourceFreq(UINT StreamNumber, UINT* Divider) override { return unsupported("IDirect3DDevice9::GetStreamSourceFreq"); }
HRESULT STDMETHODCALLTYPE SetIndices(IDirect3DIndexBuffer9* pIndexData) override { return unsupported("IDirect3DDevice9::SetIndices"); }
HRESULT STDMETHODCALLTYPE GetIndices(IDirect3DIndexBuffer9** ppIndexData) override { if(ppIndexData)*ppIndexData=nullptr; return unsupported("IDirect3DDevice9::GetIndices"); }
HRESULT STDMETHODCALLTYPE CreatePixelShader(const DWORD *byte_code, IDirect3DPixelShader9 **shader) override { if(shader)*shader=nullptr; return unsupported("IDirect3DDevice9::CreatePixelShader"); }
HRESULT STDMETHODCALLTYPE SetPixelShader(IDirect3DPixelShader9* pShader) override { return unsupported("IDirect3DDevice9::SetPixelShader"); }
HRESULT STDMETHODCALLTYPE GetPixelShader(IDirect3DPixelShader9** ppShader) override { if(ppShader)*ppShader=nullptr; return unsupported("IDirect3DDevice9::GetPixelShader"); }
HRESULT STDMETHODCALLTYPE SetPixelShaderConstantF(UINT reg_idx, const float *data, UINT count) override { return unsupported("IDirect3DDevice9::SetPixelShaderConstantF"); }
HRESULT STDMETHODCALLTYPE GetPixelShaderConstantF(UINT StartRegister, float* pConstantData, UINT Vector4fCount) override { return unsupported("IDirect3DDevice9::GetPixelShaderConstantF"); }
HRESULT STDMETHODCALLTYPE SetPixelShaderConstantI(UINT reg_idx, const int *data, UINT count) override { return unsupported("IDirect3DDevice9::SetPixelShaderConstantI"); }
HRESULT STDMETHODCALLTYPE GetPixelShaderConstantI(UINT StartRegister, int* pConstantData, UINT Vector4iCount) override { return unsupported("IDirect3DDevice9::GetPixelShaderConstantI"); }
HRESULT STDMETHODCALLTYPE SetPixelShaderConstantB(UINT reg_idx, const WINBOOL *data, UINT count) override { return unsupported("IDirect3DDevice9::SetPixelShaderConstantB"); }
HRESULT STDMETHODCALLTYPE GetPixelShaderConstantB(UINT StartRegister, WINBOOL* pConstantData, UINT BoolCount) override { return unsupported("IDirect3DDevice9::GetPixelShaderConstantB"); }
HRESULT STDMETHODCALLTYPE DrawRectPatch(UINT handle, const float *segment_count, const D3DRECTPATCH_INFO *patch_info) override { return unsupported("IDirect3DDevice9::DrawRectPatch"); }
HRESULT STDMETHODCALLTYPE DrawTriPatch(UINT handle, const float *segment_count, const D3DTRIPATCH_INFO *patch_info) override { return unsupported("IDirect3DDevice9::DrawTriPatch"); }
HRESULT STDMETHODCALLTYPE DeletePatch(UINT Handle) override { return unsupported("IDirect3DDevice9::DeletePatch"); }
HRESULT STDMETHODCALLTYPE CreateQuery(D3DQUERYTYPE Type, IDirect3DQuery9** ppQuery) override { if(ppQuery)*ppQuery=nullptr; return unsupported("IDirect3DDevice9::CreateQuery"); }
};
