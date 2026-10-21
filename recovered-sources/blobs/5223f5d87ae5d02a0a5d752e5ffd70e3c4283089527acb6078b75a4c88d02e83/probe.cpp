#include <initializer_list>

#include <cassert>
#include <cmath>
#include <limits>
constexpr float PI=3.14159265358979323846f;
struct CVECTOR{float x=0,y=0,z=0;};
CVECTOR operator-(CVECTOR a,CVECTOR b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
float operator~(CVECTOR a){return a.x*a.x+a.y*a.y+a.z*a.z;}
struct Character{bool main=false;unsigned GetAttributeAsDword(const char*,unsigned)const{return main;}};
struct Render{CVECTOR camera{},angles{};float fov=PI/2;struct Point{int x=2560,y=1655;}screen;float reflectionProjection=1;unsigned targetViewport=256;
void GetCamera(CVECTOR&p,CVECTOR&a,float&f){p=camera;a=angles;f=fov;} Point GetScreenSize(){return screen;}};
struct SHIP{Render*pRS;Character*character;struct {CVECTOR vBoxSize;}State;CVECTOR position;Character*GetACharacter(){return character;}CVECTOR GetPos(){return position;}bool IsAuxiliaryDetailVisible();};
bool SHIP::IsAuxiliaryDetailVisible()
{
    const auto *character = GetACharacter();
    if (!pRS || !character)
        return true;

    CVECTOR camera, angles;
    float fov;
    pRS->GetCamera(camera, angles, fov);
    const auto screen = pRS->GetScreenSize();
    const auto delta = GetPos() - camera;
    const float radius = .5f * std::sqrt(~State.vBoxSize);
    const float distance = std::sqrt(~delta);
    const float lens = std::tan(fov * .5f);
    if (screen.x <= 0 || !std::isfinite(radius) || radius <= 0.f ||
        !std::isfinite(distance) || distance <= radius ||
        !std::isfinite(lens) || lens <= 0.f || fov >= PI)
        return true;

    // GetCamera retains the effective main-camera lens during reflection cube
    // VIEW/PROJECTION overrides. Use the near side of the hull's enclosing
    // sphere, so any close or zoomed view restores detail conservatively.
    const float pixels = screen.x * radius / (lens * (distance - radius));
    return pixels >= 64.f;
}

int main(){Render r;Character ch;SHIP s{&r,&ch,{{10,5,40}},{0,0,1000}};
assert(!s.IsAuxiliaryDetailVisible()); // small distant hull still has its ordinary draw
s.position.z=100;assert(s.IsAuxiliaryDetailVisible());
s.position.z=1000;s.State.vBoxSize={50,20,180};assert(s.IsAuxiliaryDetailVisible());
s.State.vBoxSize={10,5,40};r.fov=.15f;assert(s.IsAuxiliaryDetailVisible());
r.fov=PI/2;assert(!s.IsAuxiliaryDetailVisible());
for(float yaw:{-3.f,-1.f,0.f,1.f,3.f}){r.angles.y=yaw;assert(!s.IsAuxiliaryDetailVisible());}
// Reflection target/view/projection overrides do not enter this owner.
r.reflectionProjection=100;r.targetViewport=16;assert(!s.IsAuxiliaryDetailVisible());
ch.main=true;assert(!s.IsAuxiliaryDetailVisible());ch.main=false;
s.position.z=1;assert(s.IsAuxiliaryDetailVisible());s.position.z=1000;
r.screen.x=0;assert(s.IsAuxiliaryDetailVisible());r.screen.x=2560;
r.fov=0;assert(s.IsAuxiliaryDetailVisible());r.fov=std::numeric_limits<float>::quiet_NaN();assert(s.IsAuxiliaryDetailVisible());r.fov=PI/2;
s.State.vBoxSize={0,0,0};assert(s.IsAuxiliaryDetailVisible());s.State.vBoxSize={10,5,40};
// No cached projection or admission survives zoom/scene/mode changes.
for(int i=0;i<1000;i++){r.fov=.15f;assert(s.IsAuxiliaryDetailVisible());r.fov=PI/2;assert(!s.IsAuxiliaryDetailVisible());}
}
