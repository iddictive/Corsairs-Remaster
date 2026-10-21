#pragma once
#include "indexed_span_frame_cache.hpp"
#include <cstdio>
#include <cstdlib>
#include <set>
#include <thread>

// Read-only diagnostics. Never skip an erase, repair a link, or clear a cache.
namespace hull_cache_diag {
inline void require(bool value,const char*phase,const char*invariant) {
    if(value)return;
    std::fprintf(stderr,"HULL_CACHE_INVARIANT phase=%s invariant=%s\n",phase,invariant);
    std::fflush(stderr);std::abort();
}
inline void check(const sm::IndexedSpanFrameCache&cache,const char*phase) {
    require(cache.entries.size()==cache.recency.size(),phase,"entry-recency-count");
    require(cache.entries.size()<=8192,phase,"entry-budget");
    std::set<sm::IndexedSpanFrameCache::Key> seen;
    std::size_t bytes=0;
    for(auto recent=cache.recency.begin();recent!=cache.recency.end();++recent){
        require(seen.insert(*recent).second,phase,"recency-unique");
        const auto entry=cache.entries.find(*recent);
        require(entry!=cache.entries.end(),phase,"recency-entry");
        require(entry->second.recent==recent,phase,"exact-recency-iterator");
    }
    for(const auto&[key,entry]:cache.entries){
        bytes+=entry.topology.capacity()*sizeof(std::uint32_t);
        bytes+=entry.boneBoxes.capacity()*sizeof(storm_metal::animated_bounds::BoneBox);
        const auto owner=[&](const auto&spans,const auto&revisions,unsigned slot){
            const auto found=spans.find(key[slot]);const auto revision=revisions.find(key[slot]);
            require(found!=spans.end()&&revision!=revisions.end(),phase,"entry-owner");
            require(revision->second==key[slot+1],phase,"owner-revision");
            require(std::count(found->second.begin(),found->second.end(),key)==1,phase,"owner-membership");
        };
        owner(cache.indexSpans,cache.indexRevisions,0);owner(cache.vertexSpans,cache.vertexRevisions,2);
    }
    const auto owners=[&](const auto&spans,const auto&revisions,unsigned slot){
        require(spans.size()==revisions.size(),phase,"owner-revision-count");
        std::size_t population=0;
        for(const auto&[id,keys]:spans){
            require(!keys.empty()&&revisions.count(id)==1,phase,"nonempty-owner");
            for(const auto&key:keys){
                require(key[slot]==id&&cache.entries.count(key)==1,phase,"owner-entry");++population;
            }
        }
        require(population==cache.entries.size(),phase,"owner-population");
    };
    owners(cache.indexSpans,cache.indexRevisions,0);owners(cache.vertexSpans,cache.vertexRevisions,2);
    require(bytes==cache.topologyBytes&&bytes<=cache.topologyBudget,phase,"payload-accounting");
}
struct Scope {
    const sm::IndexedSpanFrameCache&cache;
    bool full=false;
    inline static thread_local bool inside=false;
    inline static thread_local std::uint64_t draws=0;
    inline static const std::thread::id firstThread=std::this_thread::get_id();
    explicit Scope(const sm::IndexedSpanFrameCache&value):cache(value){
        require(std::this_thread::get_id()==firstThread,"draw-enter","single-thread");
        require(!inside,"draw-enter","nonrecursive");inside=true;
        full=(++draws%128==0)||std::getenv("HULL_DIAG_EVERY_DRAW");
        if(full)check(cache,"draw-enter");
    }
    ~Scope(){if(full)check(cache,"draw-leave");inside=false;}
};
}
