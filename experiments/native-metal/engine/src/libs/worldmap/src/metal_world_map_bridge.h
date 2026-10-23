#pragma once
#include "dx9render.h"
struct WdmMetalScreenRect { float x,y,w,h,angle,z,rhw; uint32_t color; float uv0[4],uv1[4]; float uv1Rotation; };
struct WdmMetalQuad3D { CVECTOR center,axisX,axisY; float halfWidth,halfHeight; uint32_t color; float uv[4]; };
struct WdmMetalDangerDisc { CVECTOR center; float radius; uint32_t centerColor, edgeColor; uint32_t segments; };
struct WdmMetalShipWake { CVECTOR origin; const void *centerline; uint32_t points; float y,baseV,alpha,speedScale; };
// Implemented by the Metal renderer owner. Each call uploads compact records;
// vertex expansion and coordinate generation happen in the Metal shader path.
extern "C" bool StormMetalDrawWdmScreenRects(void*, const WdmMetalScreenRect*, uint32_t, const char*);
extern "C" bool StormMetalDrawWdmQuad(void*, const WdmMetalQuad3D&, const char*);
extern "C" bool StormMetalDrawWdmDangerDisc(void*, const WdmMetalDangerDisc&, const char*);
extern "C" bool StormMetalDrawWdmShipWake(void*, const WdmMetalShipWake&, const char*);
extern "C" unsigned StormMetalBeginWdmModel(void*, unsigned);
extern "C" void StormMetalEndWdmModel(void*, unsigned);
#ifdef STORM_METAL_RENDERER_CONSUMERS
class WdmMetalModelScope {
public:
    explicit WdmMetalModelScope(VDX9RENDER *rs, unsigned role = 1): device(rs ? rs->GetNativeDevice() : nullptr), previous(device ? StormMetalBeginWdmModel(device, role) : 0) {}
    ~WdmMetalModelScope(){if(device)StormMetalEndWdmModel(device,previous);}
private:void *device;unsigned previous;
};
#endif
// The Metal backend consumes compact records but not the fixed-function stage
// state, so each submission first applies the technique pass the legacy draw
// ran before it submitted geometry. The backend only receives the device.
inline bool WdmMetalDrawScreenRects(VDX9RENDER *rs,const WdmMetalScreenRect *rects,uint32_t count,const char *technique){if(!rs||!technique||!rs->TechniqueExecuteStart(technique))return false;bool ok=true;do{ok=StormMetalDrawWdmScreenRects(rs->GetNativeDevice(),rects,count,technique);}while(ok&&rs->TechniqueExecuteNext());return ok;}
inline bool WdmMetalDrawQuad3D(VDX9RENDER *rs,const WdmMetalQuad3D &quad,const char *technique){if(!rs||!technique||!rs->TechniqueExecuteStart(technique))return false;bool ok=true;do{ok=StormMetalDrawWdmQuad(rs->GetNativeDevice(),quad,technique);}while(ok&&rs->TechniqueExecuteNext());return ok;}
inline bool WdmMetalDrawDangerDisc(VDX9RENDER *rs,const WdmMetalDangerDisc &disc,const char *technique){if(!rs||!technique||!rs->TechniqueExecuteStart(technique))return false;bool ok=true;do{ok=StormMetalDrawWdmDangerDisc(rs->GetNativeDevice(),disc,technique);}while(ok&&rs->TechniqueExecuteNext());return ok;}
inline bool WdmMetalDrawShipWake(VDX9RENDER *rs,const WdmMetalShipWake &wake,const char *technique){if(!rs||!technique||!rs->TechniqueExecuteStart(technique))return false;bool ok=true;do{ok=StormMetalDrawWdmShipWake(rs->GetNativeDevice(),wake,technique);}while(ok&&rs->TechniqueExecuteNext());return ok;}
