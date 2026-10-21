#include "weather_surface_color.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>
using namespace storm_weather_surface;
static void need(bool v,const char*m){if(!v){std::fprintf(stderr,"FAIL %s\n",m);std::exit(1);}}
static float distance(const Color&a,const Color&b){float s=0;for(int i=0;i<3;i++)s+=(a[i]-b[i])*(a[i]-b[i]);return std::sqrt(s);}
int main(){
 const Color authored{.04f,.22f,.39f};
 const auto clear=derive(authored,{.43f,.61f,.78f},{.62f,.60f,.55f});
 const auto overcast=derive(authored,{.48f,.50f,.52f},{.34f,.35f,.36f});
 const auto storm=derive(authored,{.25f,.27f,.29f},{.14f,.15f,.16f});
 need(saturation(overcast.water)<saturation(clear.water),"overcast water loses clear-sky blue saturation");
 need(saturation(storm.foam)<.10f,"storm foam cannot become emissive cyan");
 need(luminance(clear.foam)>luminance(clear.water)&&luminance(storm.foam)>luminance(storm.water),"foam remains readable above water");
 need(luminance(storm.foam)<.72f,"storm foam follows available sky light");
 need(distance(overcast.water,Color{.48f,.50f,.52f})<distance(authored,Color{.48f,.50f,.52f}),"water moves toward visible horizon palette");
 need(distance(storm.water,storm.foam)>.15f,"storm water and foam retain separation");
 std::puts("PASS weather surface palette: clear/overcast/storm coherence, bounded foam, storm readability");
}
