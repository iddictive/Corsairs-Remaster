
#define STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
#include "ship_auxiliary_detail.h"
#include <cassert>
struct Backend {bool visible=true;};
extern "C" bool StormMetalSetShipAuxiliaryDetail(void*p,bool value){if(!p)return true;auto&b=*static_cast<Backend*>(p);bool prior=b.visible;b.visible=value;return prior;}
void early(Backend*b){storm::ship::AuxiliaryDetailScope a(b,false);assert(!b->visible);{storm::ship::AuxiliaryDetailScope c(b,true);assert(b->visible);}assert(!b->visible);return;}
int main(){Backend b;for(int i=0;i<1000;i++){early(&b);assert(b.visible);}storm::ship::AuxiliaryDetailScope absent(nullptr,false);}
