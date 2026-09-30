#include "weather_visual_temporal.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static void need(bool value,const char*message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}

int main(){using namespace storm_weather_visual;
 State a,b;a.seed(0,0xff000000,0xff000000,0xff000000,6.20f,0,0,5.9f,-.2f,6.2f);b=a;
 for(int i=0;i<60;i++)a.update(.8f,0xffffffff,0xffffffff,0xffffffff,.08f,18,5000,6.1f,.8f,.08f,1.f/30.f);
 for(int i=0;i<240;i++)b.update(.8f,0xffffffff,0xffffffff,0xffffffff,.08f,18,5000,6.1f,.8f,.08f,1.f/120.f);
 need(std::abs(a.fogDensity-b.fogDensity)<1e-4f&&std::abs(a.windSpeed-b.windSpeed)<1e-4f&&std::abs(a.rainIntensity-b.rainIntensity)<.5f&&a.fogColor==b.fogColor&&a.ambientColor==b.ambientColor&&a.sunColor==b.sunColor&&a.timeOfDay==b.timeOfDay&&std::abs(a.sunHeight-b.sunHeight)<1e-4f&&std::abs(a.sunAzimuth-b.sunAzimuth)<1e-4f,"30/120 Hz invariance");
 need(a.windAngle>6.20f&&a.windAngle<6.37f,"wind takes shortest angular path across wrap");
 State jump;jump.seed(.1f,0xff202020,0xff202020,0xff202020,0,2,0,6.f,-.1f,0);jump.update(.9f,0xffffffff,0xffffffff,0xffffffff,3.f,20,5000,6.1f,.7f,3.f,1.f/60.f);need(jump.fogDensity<.14f&&jump.rainIntensity<250.f&&jump.sunHeight<-.05f,"no one-frame preset jump");
 const float before=jump.rainIntensity;jump.update(.1f,0xff202020,0xff202020,0xff202020,0,2,0,6.2f,-.2f,0,1.f/60.f);need(jump.rainIntensity<before&&jump.rainIntensity>0,"rapid reversal continues current state");
 State hitch;hitch.seed(0,0,0,0,0,0,0,23.9f,0,6.2f);State capped=hitch;hitch.update(1,0xffffffff,0xffffffff,0xffffffff,1,20,5000,24.1f,1,0,5);capped.update(1,0xffffffff,0xffffffff,0xffffffff,1,20,5000,24.1f,1,0,.1f);need(std::abs(hitch.fogDensity-capped.fogDensity)<1e-6f&&std::abs(hitch.sunHeight-capped.sunHeight)<1e-6f&&hitch.timeOfDay>.09f&&hitch.timeOfDay<.11f,"pause/hitch clamp and day wrap");
 std::puts("PASS shared weather visual temporal state including time and sun geometry");
}
