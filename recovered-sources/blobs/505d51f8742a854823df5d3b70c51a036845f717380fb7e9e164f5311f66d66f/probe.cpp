#include "render_value_types.hpp"
#include <cstdio>
#include <cstddef>
#include <cstdint>
struct Owner {storm_render::Point point; std::uint32_t sentinel;};
int main(){static_assert(sizeof(long)==8&&sizeof(storm_render::Point)==8);Owner old{{0,0},0xa5a5a5a5},fixed=old;std::sscanf("-12,34","%ld,%ld",&old.point.x,&old.point.y);std::sscanf("-12,34","%d,%d",&fixed.point.x,&fixed.point.y);std::printf("old=(%d,%d) sentinel=%08x fixed=(%d,%d) sentinel=%08x\n",old.point.x,old.point.y,old.sentinel,fixed.point.x,fixed.point.y,fixed.sentinel);return old.sentinel==0xa5a5a5a5||fixed.sentinel!=0xa5a5a5a5||fixed.point.x!=-12||fixed.point.y!=34;}
