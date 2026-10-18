#pragma once
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstdint>

namespace storm_ship_damage_trace {
using Clock=std::chrono::steady_clock;
inline bool enabled(){static const bool value=[] {const auto*p=std::getenv("STORM_METAL_DAMAGE_TRACE");return p&&p[0]=='1'&&p[1]=='\0';}();return value;}
inline void write(const char*phase,const char*state,uint64_t id,uint64_t owner,int node,double duration=0.){
    const double now=std::chrono::duration<double,std::milli>(Clock::now().time_since_epoch()).count();
    std::fprintf(stderr,"[HullTiming] time_ms=%.3f phase=%s state=%s id=%llu owner=%llu node=%d duration_ms=%.3f\n",
        now,phase,state,(unsigned long long)id,(unsigned long long)owner,node,duration);
}
inline void event(const char*phase,uint64_t owner,int node=-1){if(enabled())write(phase,"event",0,owner,node);}
class Phase {
    const char*phase_;uint64_t owner_,id_=0;int node_;Clock::time_point start_;
public:
    Phase(const char*phase,uint64_t owner,int node=-1):phase_(phase),owner_(owner),node_(node){
        if(enabled()){static std::atomic<uint64_t>next{0};id_=++next;start_=Clock::now();write(phase_,"begin",id_,owner_,node_);}}
    ~Phase(){if(id_)write(phase_,"end",id_,owner_,node_,std::chrono::duration<double,std::milli>(Clock::now()-start_).count());}
    Phase(const Phase&)=delete;Phase&operator=(const Phase&)=delete;
};
}
