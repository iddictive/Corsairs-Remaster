#include "weather_surface_color.hpp"
#include "particles_shaders.hpp"
#include <cmath>
#include <cstring>
#include <cstdio>
#include <cstdlib>
using namespace storm_weather_surface;
static void need(bool v,const char*m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
static float distance(const Color&a,const Color&b){float s=0;for(int i=0;i<3;i++)s+=(a[i]-b[i])*(a[i]-b[i]);return std::sqrt(s);}
int main(){
 const Color authored{.04f,.22f,.39f};
 const auto clear=derive(authored,{.43f,.61f,.78f},{.62f,.60f,.55f});
 const Color sunsetFog{.76f,.39f,.27f};
 const auto sunset=derive(authored,sunsetFog,{.46f,.30f,.24f});
 const auto overcast=derive(authored,{.48f,.50f,.52f},{.34f,.35f,.36f});
 const auto storm=derive(authored,{.25f,.27f,.29f},{.14f,.15f,.16f});
 // Actual open-sea night preset and midnight weather, including a raised moon.
 const Color nightAuthored{5.f/255.f,10.f/255.f,20.f/255.f};
 const Color nightFog{2.f/255.f,2.f/255.f,2.f/255.f};
 const Color nightAmbient{28.f/255.f,28.f/255.f,35.f/255.f};
 const auto night=derive(nightAuthored,nightFog,nightAmbient);
 need(saturation(overcast.water)<saturation(clear.water),"overcast water loses clear-sky blue saturation");
 need(luminance(clear.water)<luminance(Color{.43f,.61f,.78f})&&
      luminance(sunset.water)<luminance(sunsetFog)&&
      luminance(overcast.water)<luminance(Color{.48f,.50f,.52f})&&
      luminance(storm.water)<luminance(Color{.25f,.27f,.29f}),
      "clear/sunset/overcast/storm water body remains darker than the visible horizon");
 need(reflectionCeiling(Color{.43f,.61f,.78f})<luminance(Color{.43f,.61f,.78f})&&
      reflectionCeiling(sunsetFog)<luminance(sunsetFog)&&
      reflectionCeiling(Color{.48f,.50f,.52f})<luminance(Color{.48f,.50f,.52f})&&
      reflectionCeiling(Color{.25f,.27f,.29f})<luminance(Color{.25f,.27f,.29f}),
      "broad environment reflection remains below the visible horizon in all weather fixtures");
 need(sunset.water[0]>sunset.water[2],"sunset water follows the warm visible horizon instead of turning gray-blue");
 need(distance(sunset.water,sunsetFog)<distance(authored,sunsetFog),"sunset water moves toward the visible horizon palette");
 need(saturation(storm.foam)<.10f,"storm foam cannot become emissive cyan");
 need(luminance(clear.foam)>luminance(clear.water)&&luminance(storm.foam)>luminance(storm.water),"foam remains readable above water");
 need(luminance(storm.foam)<.72f,"storm foam follows available sky light");
 need(distance(overcast.water,Color{.48f,.50f,.52f})<distance(authored,Color{.48f,.50f,.52f}),"water moves toward visible horizon palette");
 need(distance(storm.water,storm.foam)>.15f,"storm water and foam retain separation");
 need(luminance(night.water)>0.f&&luminance(night.water)<luminance(nightFog),
      "midnight water body stays darker than its actual horizon; specular moonlight is separate");
 need(night.water[2]>night.water[1]&&night.water[1]>night.water[0],
      "low-light response preserves the authored cool water ordering");
 const auto unlit=derive(authored,{0,0,0},{0,0,0});
 need(luminance(unlit.water)==0.f&&luminance(unlit.foam)==0.f,
      "water pigment and foam cannot emit light without illumination");
 need(luminance(night.foam)<luminance(clear.foam)*.5f,
      "night foam cannot retain daytime illumination");
 need(isWaterEffectTexture("RESOURCE\\Textures\\WATERSPLASH.TGA")&&
      isWaterEffectTexture("water1.tga")&&isWaterEffectTexture("sparcle2.TgA"),
      "known spray and foam atlases are classified independent of path/case");
 need(!isWaterEffectTexture("fire.tga")&&!isWaterEffectTexture("rain.tga")&&
      !isWaterEffectTexture("smoke_watersplash.tga.bak")&&
      !isWaterEffectTexture("smokewatersplash.tga"),
      "ordinary particles and near-name suffix/prefix textures remain outside weather tinting");
 need(std::strstr(seaShaderSource,"seafoam_modern_fs")&&
      std::strstr(seaShaderSource,"float4(foam.rgb*tint,o.t3.z)"),
      "weather-aware sea foam preserves authored alpha");
 need(std::strstr(seaShaderSource,"weatherBoundedReflection")&&
      std::strstr(particlesShaderSource,"particle_water_fs")&&
      std::strstr(particlesShaderSource,"particle_modern_fs"),
      "bounded sea reflection and isolated generic/water particle entries are present");
 std::puts("PASS weather surface palette: clear/sunset/overcast/storm coherence, bounded foam, exact water-particle scope");
}
