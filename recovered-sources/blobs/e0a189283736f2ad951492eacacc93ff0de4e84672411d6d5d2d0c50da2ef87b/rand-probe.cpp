
#include <cstdlib>
#include <iostream>
#include "metal_cpu_parallel.h"
int main() {
    constexpr size_t count=1000000, jobs=4;
    srand(3127);
    for(size_t i=0;i<jobs*count;++i) (void)rand();
    const int serial=rand();
    srand(3127);
    MetalCpuParallelFor(jobs, [](size_t) noexcept {
        for(size_t i=0;i<count;++i) (void)rand();
    });
    const int parallel=rand();
    std::cout<<"identical seed and 4000000 rand calls: serial next="<<serial<<" parallel next="<<parallel<<"\n";
    return serial==parallel?0:1;
}
