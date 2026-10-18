#pragma once
#include "geometry_work_budget.hpp"
#include "engine/src/libs/util/include/metal_cpu_parallel.h"

namespace storm_geometry_work {
inline std::atomic<unsigned> parallelGroups{0};
struct ParallelGroup {
    bool acquired=false;
    ParallelGroup(){
        auto count=parallelGroups.load(std::memory_order_relaxed);
        while(count<2)if(parallelGroups.compare_exchange_weak(count,count+1,std::memory_order_relaxed)){
            acquired=true;break;
        }
    }
    ~ParallelGroup(){if(acquired)parallelGroups.fetch_sub(1,std::memory_order_relaxed);}
};
// Only value workers fork numeric work. OS-owned four-chunk dispatch joins
// before returning; every chunk inherits cancellation and the same allowance.
template<class Function>bool parallelFor(size_t count,Function function){
    auto*parent=active;
    if(!parent){for(size_t n=0;n<count;++n)if(!function(n))return false;return true;}
    // Contact and weapon workers share this admission; busy groups fall back
    // to their existing worker, rather than adding more dispatch fan-out.
    ParallelGroup group;
    if(!group.acquired){for(size_t n=0;n<count;++n)if(!function(n))return false;return true;}
    const auto shared=parent->sharedAllowance();const auto cancel=parent->cancellation();
    std::atomic<bool>failed{false};
    MetalCpuParallelFor(count,[&](size_t n)noexcept{
        if(failed.load(std::memory_order_relaxed))return;
        try{Scope scope(shared,cancel);if(!function(n))failed.store(true,std::memory_order_relaxed);}
        catch(...){failed.store(true,std::memory_order_relaxed);}
    });
    return !failed.load(std::memory_order_relaxed);
}
}
