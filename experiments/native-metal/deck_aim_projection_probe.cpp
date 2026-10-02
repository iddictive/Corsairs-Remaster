// Called by deck_aim_zoom_probe.py; tests actual renderer methods with a
// deterministic matrix device, without pretending to execute native Metal.
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
struct D3DMATRIX { float _11=0,_22=0,_33=0,_43=0,_34=0; };
constexpr int D3DTS_PROJECTION=1;
#define CHECKD3DERR(x) ((x)!=0)
struct Device { D3DMATRIX projection; int SetTransform(int,const D3DMATRIX *m) { projection=*m; return 0; } };
struct DX9RENDER {
    Device device; Device *d3d9=&device;
    struct { float x=2048.f,y=1285.f; } screen_size;
    float FovMultiplier=1.f, fNearClipPlane=.5f, fFarClipPlane=4000.f, aspectRatio=.75f;
    float Fov=1.f,perspectiveInput=1.f,projectionMagnification=1.f;
    bool bNewFovCalculation=false; int planeUpdates=0;
    bool SetPerspective(float perspective,float fAspectRatio=-1.f,float magnification=1.f);
    void SetNearFarPlane(float fNear,float fFar);
    void FindPlanes(Device *) { ++planeUpdates; }
};
#include "renderer_projection_methods.inc"
bool near(float a,float b,float eps=1e-5f) { return std::abs(a-b)<=eps; }
int main() {
    int configurations=0;
    for (bool policy : {false,true}) for(float multiplier : {.8f,1.f,1.2f}) for(float aspect : {.5f,1285.f/2048.f,1.f}) {
        DX9RENDER r; r.FovMultiplier=multiplier; r.bNewFovCalculation=policy;
        constexpr float raw=1.285f;
        assert(r.SetPerspective(raw,aspect));
        const auto base=r.device.projection;
        const float horizontal=raw*multiplier;
        const float vertical=policy?2.f*std::atan(std::tan(horizontal/2.f)*aspect):horizontal*aspect;
        assert(near(base._11,1.f/std::tan(horizontal/2.f)));
        assert(near(base._22,1.f/std::tan(vertical/2.f)));
        assert(r.SetPerspective(raw,aspect,5.f));
        const auto zoom=r.device.projection;
        assert(near(zoom._11/base._11,5.f) && near(zoom._22/base._22,5.f));
        assert(near(std::tan(horizontal/2.f)/std::tan(r.Fov/2.f),5.f));
        assert(zoom._33==base._33 && zoom._43==base._43 && zoom._34==base._34);
        assert(near(zoom._11/zoom._22,base._11/base._22));
        r.SetNearFarPlane(2.f,8000.f);
        assert(near(r.device.projection._11,zoom._11) && near(r.device.projection._22,zoom._22));
        r.SetNearFarPlane(.5f,4000.f);
        assert(near(r.device.projection._11,zoom._11) && near(r.device.projection._22,zoom._22));
        assert(near(r.device.projection._33,zoom._33) && near(r.device.projection._43,zoom._43));
        assert(near(r.projectionMagnification,5.f) && near(r.perspectiveInput,raw));
        // Ordinary camera/spyglass perspective calls default back to 1x.
        assert(r.SetPerspective(raw,aspect));
        assert(near(r.device.projection._11,base._11) && near(r.device.projection._22,base._22));
        assert(near(r.projectionMagnification,1.f) && near(r.Fov,horizontal));
        r.SetNearFarPlane(2.f,8000.f);
        assert(near(r.device.projection._11,base._11) && near(r.device.projection._22,base._22));
        assert(r.planeUpdates==6);
        ++configurations;
    }
    DX9RENDER r;
    assert(!r.SetPerspective(1.f,-1.f,std::numeric_limits<float>::quiet_NaN()));
    assert(!r.SetPerspective(1.f,-1.f,0.f));
    assert(!r.SetPerspective(1.f,-1.f,-1.f));
    assert(r.planeUpdates==0);
    std::cout << configurations << " actual-renderer projection configurations passed (legacy/new FOV, multiplier, depth reset, ordinary-camera reset)\n";
}
