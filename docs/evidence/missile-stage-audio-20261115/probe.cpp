#include "audio_voice_policy.h"
#include <cassert>
#include <cstdio>
#include <set>
#include <string_view>
using namespace storm::metal::audio;
int main() {
 const char* stages[]={"phonk_missile_heavy","phonk_missile_launch_kinzhal","phonk_missile_launch_oreshnik","phonk_missile_launch_shtorm","phonk_missile_launch_kalibr","phonk_missile_interceptor","phonk_missile_interceptor_patriot","phonk_missile_cruise","phonk_missile_flyby","phonk_missile_sonic","phonk_missile_impact"};
 std::set<std::string_view> buses;
 for(auto a:stages){assert(IsClusteredImpactAlias(a)); buses.insert(ClusteredVoiceKey(a));}
 assert(buses.size()==5);
 size_t total=0; for(auto key:buses) total+=MaxClusteredVoices(key);
 assert(total==25); // 6 launch + 4 cruise + 3 flyby + 4 sonic + 8 impacts
 assert(ClusteredVoiceKey("phonk_missile_impact")!=ClusteredVoiceKey("phonk_missile_sonic"));
 assert(ClusteredVoiceKey("debris_splash")=="ball_splash");
 assert(!IsClusteredImpactAlias("ship_onfire"));
 assert(!IsClusteredImpactAlias("chargeballs"));
 assert(MaxClusteredVoices("ball_splash")==8);
 float min=1,max=0;
 for(unsigned i=0;i<100000;i++){auto g=MissileEventGain(i); assert(g>=.9f && g<=1.f); if(g<min)min=g;if(g>max)max=g;assert(g==MissileEventGain(i));}
 assert(max-min>.09f);
 assert(ImpactMixGain(2)==.5f);
 assert(SpatialPolicyForAlias("phonk_missile_sonic").linearFalloff);
 puts("PASS: 11 aliases, 5 independent stage buses, 25-voice bound, legacy policies, 100000 deterministic bounded gains");
}
