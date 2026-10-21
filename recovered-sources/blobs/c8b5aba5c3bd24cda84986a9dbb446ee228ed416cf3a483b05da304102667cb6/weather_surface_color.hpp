#pragma once
#include <algorithm>
#include <array>
#include <cmath>
namespace storm_weather_surface {
using Color=std::array<float,3>;
inline float luminance(const Color&c){return c[0]*.2126f+c[1]*.7152f+c[2]*.0722f;}
inline float saturation(const Color&c){return *std::max_element(c.begin(),c.end())-*std::min_element(c.begin(),c.end());}
inline Color mix(const Color&a,const Color&b,float t){t=std::clamp(t,0.f,1.f);return {a[0]+(b[0]-a[0])*t,a[1]+(b[1]-a[1])*t,a[2]+(b[2]-a[2])*t};}
inline Color scale(const Color&c,float v){return {c[0]*v,c[1]*v,c[2]*v};}
struct Palette{Color water;Color foam;};
// Linear RGB. Fog is the smoothed visible horizon proxy; ambient is the same
// smoothed scene illumination consumed by land and the modern sea material.
inline Palette derive(const Color&authoredWater,const Color&fog,const Color&ambient){
 const float ambientY=std::clamp(luminance(ambient),.025f,1.f),fogSat=saturation(fog);
 const float overcast=std::clamp((.16f-fogSat)*5.f,0.f,1.f),fogY=luminance(fog);
 const Color neutralFog{fogY,fogY,fogY},horizon=mix(fog,neutralFog,.18f+.42f*overcast);
 Color water=scale(mix(authoredWater,horizon,.30f+.30f*overcast),.38f+.62f*ambientY);
 const float foamY=std::clamp(.28f+.58f*std::sqrt(ambientY)+.12f*luminance(water),.32f,.92f);
 Color foam=scale(mix(horizon,neutralFog,.72f),foamY/std::max(luminance(mix(horizon,neutralFog,.72f)),.025f));
 for(float&channel:foam)channel=std::clamp(channel,0.f,1.f);
 return {water,foam};
}
}
