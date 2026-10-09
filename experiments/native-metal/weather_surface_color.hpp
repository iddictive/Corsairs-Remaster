#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <string_view>
namespace storm_weather_surface {
using Color=std::array<float,3>;
inline float luminance(const Color&c){return c[0]*.2126f+c[1]*.7152f+c[2]*.0722f;}
inline float saturation(const Color&c){return *std::max_element(c.begin(),c.end())-*std::min_element(c.begin(),c.end());}
inline Color mix(const Color&a,const Color&b,float t){t=std::clamp(t,0.f,1.f);return {a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t,a[2]+(b[2]-a[2])*t};}
inline Color scale(const Color&c,float v){return {c[0]*v,c[1]*v,c[2]*v};}
inline float reflectionCeiling(const Color&fog){return std::max(luminance(fog)*.98f,.06f);}
struct Palette{Color water;Color foam;};
// Normalized renderer color space. Fog is the smoothed visible horizon proxy;
// ambient is the same smoothed scene illumination consumed by land and water.
// The modern sea vertex converts Palette::water to linear light once per vertex;
// foam remains a display-space tint for the existing UNorm texture paths.
inline Color visibleHorizon(const Color&fog,float overcast){
 const float fogY=luminance(fog);const Color neutralFog{fogY,fogY,fogY};
 return mix(fog,neutralFog,.18f+.42f*overcast);
}
inline Color deriveFoam(const Color&fog,const Color&ambient){
 const float ambientY=std::clamp(luminance(ambient),0.f,1.f),fogSat=saturation(fog);
 const float overcast=std::clamp((.16f-fogSat)*5.f,0.f,1.f),fogY=luminance(fog);
 const Color neutralFog{fogY,fogY,fogY},horizon=visibleHorizon(fog,overcast);
 // Foam reflects incident light; the daylight response must not become an
 // emission floor when the same material is drawn under moonlight.
 const float foamY=std::min(std::clamp(.30f+.60f*std::sqrt(ambientY),.32f,.92f),2.f*ambientY);
 Color foam=scale(mix(horizon,neutralFog,.72f),foamY/std::max(luminance(mix(horizon,neutralFog,.72f)),.025f));
 for(float&channel:foam)channel=std::clamp(channel,0.f,1.f);
 return foam;
}
inline Palette derive(const Color&authoredWater,const Color&fog,const Color&ambient){
 const float ambientY=std::clamp(luminance(ambient),0.f,1.f),fogSat=saturation(fog);
 const float overcast=std::clamp((.16f-fogSat)*5.f,0.f,1.f);
 const Color horizon=visibleHorizon(fog,overcast);
 // Warm horizons need more influence than clear blue daylight; otherwise the
 // fixed blue pigment turns sunset water gray while the sky is orange.
 const float warmHorizon=std::clamp((fog[0]-fog[2])*3.f,0.f,1.f);
 // Retain the daylight pigment response. Below it, remove the artificial
 // readability lift and converge to zero with the actual ambient light.
 const float illumination=(.38f+.62f*ambientY)*std::min(1.f,ambientY/.20f);
 Color water=scale(mix(authoredWater,horizon,.30f+.30f*overcast+.25f*warmHorizon),
                   illumination);
 return {water,deriveFoam(fog,ambient)};
}
inline bool equalAsciiInsensitive(char a,char b){
 if(a>='a'&&a<='z')a=char(a-'a'+'A');
 if(b>='a'&&b<='z')b=char(b-'a'+'A');
 return a==b;
}
inline bool equalAsciiInsensitive(std::string_view value,std::string_view expected){
 if(value.size()!=expected.size())return false;
 for(size_t i=0;i<expected.size();++i)if(!equalAsciiInsensitive(value[i],expected[i]))return false;
 return true;
}
// These are particle-atlas owners for spray/foam. Technique identity is too
// broad because smoke, fire, debris and rain share the particle shaders.
inline bool isWaterEffectTexture(std::string_view name){
 const auto separator=name.find_last_of("/\\");
 if(separator!=std::string_view::npos)name.remove_prefix(separator+1);
 return equalAsciiInsensitive(name,"watersplash.tga")||
        equalAsciiInsensitive(name,"watersplash2.tga")||
        equalAsciiInsensitive(name,"water1.tga")||
        equalAsciiInsensitive(name,"sparcle2.tga");
}
}
