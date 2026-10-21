#include "lighting_scene.hpp"
#include <cstdio>
#include <cstdlib>
static void need(bool v,const char*m){if(!v){std::fprintf(stderr,"FAIL: %s\n",m);std::exit(1);}}
int main(){storm_metal::LightingScene s;auto first=s.revision();s.setDomain(storm_metal::LightingDomain::Interior);s.setAmbient(0xff203040u);storm_metal::SceneLight sun{};sun.type=3;sun.enabled=true;sun.direction={0,-.8f,.2f,0};sun.diffuse={1,.8f,.6f,1};s.setLight(0,sun);need(s.revision()==first+3,"shared revision");s.beginFrame();need(s.frameRevision()==s.revision(),"frozen frame");s.setLight(0,sun);need(s.frameRevision()==s.revision(),"stable publication");s.setEnabled(0,false);need(s.revision()==s.frameRevision()+1,"day/night invalidation");std::puts("PASS unified lighting scene revision and immutable frame snapshot");}
