#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include "weather_visual_temporal.hpp"
#include "weather_engine.hpp"
enum {whf_wind_angle,whf_wind_speed,whf_fog_density,whf_time_counter};
enum {whc_fog_color,whc_sun_ambient,whc_sun_color};
float WeatherRainIntensity(float rain){return std::clamp((rain-.03f)/.97f,0.f,1.f);}
struct WEATHER {
 bool weatherState=true,weatherPaletteValid=true;
 struct Engine {storm_weather::Conditions c; storm_weather::Conditions local(){return c;}} weatherEngine;
 float fFloats[4]{},visualRainTarget=0;
 uint32_t dwColors[3]{},weatherPalette[24][3]{};
 void ApplyWeatherConditions();
};
void WEATHER::ApplyWeatherConditions()
{
    if (!weatherState) return;
    const auto c=weatherEngine.local();
    fFloats[whf_wind_angle]=c.angle; fFloats[whf_wind_speed]=c.speed;
    fFloats[whf_fog_density]=.000005f+c.rain*.00004f;
    visualRainTarget=WeatherRainIntensity(c.rain)*5000.f;
    const float hour=storm_weather_visual::wrapDayHour(fFloats[whf_time_counter]);
    const int first=static_cast<int>(hour), next=(first+1)%24;
    const float fraction=hour-first;
    const auto color=[this,fraction,first,next](unsigned channel,uint32_t fallback) {
        const uint32_t start=weatherPaletteValid?weatherPalette[first][channel]:fallback;
        const uint32_t end=weatherPaletteValid?weatherPalette[next][channel]:start;
        uint32_t out=0;
        for(unsigned shift=0;shift<24;shift+=8) {
            float low=float((start>>shift)&255u), high=float((end>>shift)&255u);
            out|=uint32_t(low+(high-low)*fraction)<<shift;
        } return out;
    };
    dwColors[whc_fog_color]=color(0,dwColors[whc_fog_color]);
    // Palette is solar time, not a second clear-weather owner. Overcast air
    // loses its blue tint and dims with the same parcel driving the sky/rain.
    const float cloudiness=std::clamp((c.coverage-.4f)/.55f,0.f,1.f);
    const float overcast=weatherPaletteValid?cloudiness*cloudiness*(3.f-2.f*cloudiness):0.f;
    const uint32_t fog=dwColors[whc_fog_color];
    const float red=float((fog>>16)&255u), green=float((fog>>8)&255u), blue=float(fog&255u);
    const float grey=(.2126f*red+.7152f*green+.0722f*blue)*(1.f-.22f*c.rain-.10f*c.storm);
    uint32_t weatherFog=fog&0xff000000u;
    for (unsigned shift=0;shift<24;shift+=8) {
        const float channel=float((fog>>shift)&255u);
        weatherFog|=uint32_t(channel+(grey-channel)*overcast)<<shift;
    }
    dwColors[whc_fog_color]=weatherFog;
    dwColors[whc_sun_ambient]=color(1,dwColors[whc_sun_ambient]);
    dwColors[whc_sun_color]=color(2,dwColors[whc_sun_color]);
}

void need(bool v,const char* text){if(!v){fprintf(stderr,"FAIL %s\n",text);exit(1);}}
int main(){
 WEATHER w; w.fFloats[whf_time_counter]=12.5f;
 for(auto& hour:w.weatherPalette){hour[0]=0xff80a0c0;hour[1]=0xff403020;hour[2]=0xfff0e0d0;}
 w.weatherEngine.c={.2f,0,10,0,0};w.ApplyWeatherConditions();
 need(w.dwColors[0]==0x0080a0c0,"clear sky retains exact authored fog");
 w.weatherEngine.c={1,0,10,0,0};w.ApplyWeatherConditions();
 uint32_t grey=w.dwColors[0];need(((grey>>16)&255)==((grey>>8)&255)&&((grey>>8)&255)==(grey&255),"overcast horizon loses blue cast");
 w.weatherEngine.c={1,1,10,0,1};w.ApplyWeatherConditions();
 need((w.dwColors[0]&255)<(grey&255),"rain/storm horizon darkens");
 need(w.dwColors[1]==0x00403020&&w.dwColors[2]==0x00f0e0d0,"ambient/sun unchanged");
 need((w.dwColors[0]>>24)==0,"fog alpha preserved");
 w.weatherPaletteValid=false;w.dwColors[0]=0xff80a0c0;for(int i=0;i<100;i++)w.ApplyWeatherConditions();
 need(w.dwColors[0]==0x0080a0c0,"missing palette cannot compound color");
 w.weatherState=false;w.dwColors[0]=0xff010203;w.ApplyWeatherConditions();need(w.dwColors[0]==0xff010203,"unbound weather preserved");
 puts("PASS actual ApplyWeatherConditions: clear/overcast/storm, palette absence, unbound weather");
}
