#pragma once
#include "core.h"
#include "weather_base.h"
#include "wdm_render_object.h"
#include "wdm_objects.h"
#include <array>
#include <algorithm>

// Reuse the map's world transform, translucent water technique and render list.
// Islands, ships, icons and labels are drawn afterwards; this never owns input.
class WdmWeatherRegions final : public WdmRenderObject
{
    struct Vertex { CVECTOR pos; uint32_t color; };
    static constexpr int cells=48;
    std::array<Vertex,cells*cells*6> vertices{};
    std::array<uint32_t,(cells+1)*(cells+1)> colors{};
public:
    void LRender(VDX9RENDER *rs) override
    {
        const auto id=core.GetEntityId("weather");
        if (!id || !rs || !wdmObjects) return;
        auto *weather=static_cast<WEATHER_BASE *>(core.GetEntityPointer(id));
        if (!weather) return;
        const float dx=wdmObjects->worldSizeX/cells, dz=wdmObjects->worldSizeZ/cells;
        const float x0=-wdmObjects->worldSizeX*.5f, z0=-wdmObjects->worldSizeZ*.5f;
        for (int z=0;z<=cells;++z) for (int x=0;x<=cells;++x) {
            float rain=0.f, storm=0.f, coverage=0.f;
            weather->GetWeatherAt(x0+x*dx,z0+z*dz,rain,storm,coverage);
            const float wet=std::clamp((rain-.03f)/.97f,0.f,1.f);
            const uint32_t alpha=uint32_t(255.f*std::clamp(.10f*wet+.14f*storm,0.f,.24f));
            // Blue-grey showers deepen toward storm cores; no icons or labels.
            const uint32_t red=uint32_t(143.f-43.f*storm), green=uint32_t(166.f-49.f*storm);
            const uint32_t blue=uint32_t(178.f-42.f*storm);
            colors[z*(cells+1)+x]=(alpha<<24)|(red<<16)|(green<<8)|blue;
        }
        int count=0;
        for (int z=0;z<cells;++z) for (int x=0;x<cells;++x) {
            const auto point=[&](int ix,int iz) {
                return Vertex{CVECTOR(x0+ix*dx,.30f,z0+iz*dz),colors[iz*(cells+1)+ix]};
            };
            const Vertex a=point(x,z), b=point(x+1,z), c=point(x+1,z+1), d=point(x,z+1);
            vertices[count++]=a; vertices[count++]=c; vertices[count++]=b;
            vertices[count++]=a; vertices[count++]=d; vertices[count++]=c;
        }
        const CMatrix identity;
        rs->TextureSet(0,-1); rs->TextureSet(1,-1);
        rs->SetTransform(D3DTS_WORLD,identity);
        rs->DrawPrimitiveUP(D3DPT_TRIANGLELIST,D3DFVF_XYZ|D3DFVF_DIFFUSE,cells*cells*2,
            vertices.data(),sizeof(Vertex),"WdmPortDangerZone");
    }
};
