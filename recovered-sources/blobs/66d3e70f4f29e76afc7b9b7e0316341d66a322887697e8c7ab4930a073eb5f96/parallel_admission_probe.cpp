#include "geometry_work_parallel.hpp"
#include <barrier>
#include <future>
#include <cstdio>
#include <stdexcept>
int main(){
    using namespace storm_geometry_work;
    std::barrier start(4);std::atomic<unsigned>live{0},peak{0},calls{0};
    std::vector<std::future<bool>>workers;
    for(int i=0;i<4;++i)workers.push_back(std::async(std::launch::async,[&]{
        Scope scope(allowance());start.arrive_and_wait();
        return parallelFor(64,[&](size_t){const auto n=++live;auto old=peak.load();while(old<n&&!peak.compare_exchange_weak(old,n)){}
            std::this_thread::sleep_for(std::chrono::milliseconds(1));++calls;--live;return true;});
    }));
    for(auto&worker:workers)if(!worker.get())return 1;
    if(calls!=256||live||parallelGroups||peak>10)return 2;
    auto cancel=std::make_shared<std::atomic<bool>>(true);
    {Scope scope(allowance(),cancel);if(parallelFor(8,[](size_t){checkpoint(true);return true;}))return 3;}
    if(parallelGroups)return 4;
    {Scope scope(allowance());if(parallelFor(8,[](size_t n){if(n==0)throw std::runtime_error("probe");return true;}))return 5;}
    if(parallelGroups)return 6;
    std::printf("PARALLEL_ADMISSION callbacks=%u peak_active=%u groups_after=%u canceled_joined=1 exception_joined=1\n",calls.load(),peak.load(),parallelGroups.load());
}
