#include "point_shadow_refresh.hpp"
#include <cstdio>
#include <cstdlib>
namespace {void need(bool ok,const char*message){if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}}
int main(){storm_metal::PointShadowRefresh refresh;const float stable[]={10,2,-4};
 need(!refresh.canReuse(1,7,stable,12,false),"incomplete cube cannot be reused");refresh.completed(1,7,stable,12);
 need(refresh.canReuse(2,7,stable,12,true),"first stable frame reuses cube");need(refresh.canReuse(3,7,stable,12,true),"second stable frame reuses cube");need(!refresh.canReuse(4,7,stable,12,true),"third frame refreshes all faces");
 const float moved[]={10.1f,2,-4};need(!refresh.canReuse(2,7,moved,12,true),"moved lamp refreshes");need(!refresh.canReuse(2,8,stable,12,true),"new lamp owner refreshes");need(!refresh.canReuse(2,7,stable,14,true),"changed range refreshes");
 std::puts("PASS: stable point shadow traversal is amortized 3:1; changes refresh immediately");}
