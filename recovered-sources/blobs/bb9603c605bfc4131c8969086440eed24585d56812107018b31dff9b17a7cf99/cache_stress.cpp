#include "indexed_span_frame_cache.hpp"
#include <cstdio>
#include <cstdlib>
#include <random>
#include <set>

using Cache = sm::IndexedSpanFrameCache;
static void require(bool ok, const char *what, unsigned step) {
    if (!ok) { std::fprintf(stderr,"FAIL step=%u %s\n",step,what); std::abort(); }
}
static void invariant(const Cache &c, unsigned step) {
    require(c.entries.size() == c.recency.size(), "entries/recency count", step);
    require(c.entries.size() <= 8192, "entry bound", step);
    std::set<Cache::Key> recent;
    for (auto i=c.recency.begin(); i!=c.recency.end(); ++i) {
        require(recent.insert(*i).second,"duplicate recency key",step);
        auto e=c.entries.find(*i);
        require(e!=c.entries.end(),"stale recency key",step);
        require(e->second.recent==i,"wrong recency iterator",step);
    }
    std::size_t bytes=0;
    for (const auto &[key,e]:c.entries) {
        bytes += e.topology.capacity()*sizeof(std::uint32_t);
        bytes += e.boneBoxes.capacity()*sizeof(storm_metal::animated_bounds::BoneBox);
        for (unsigned slot : {0u,2u}) {
            const auto &owners=slot==0?c.indexSpans:c.vertexSpans;
            const auto &revisions=slot==0?c.indexRevisions:c.vertexRevisions;
            auto o=owners.find(key[slot]); auto r=revisions.find(key[slot]);
            require(o!=owners.end(),"missing owner",step);
            require(r!=revisions.end()&&r->second==key[slot+1],"wrong revision",step);
            require(std::count(o->second.begin(),o->second.end(),key)==1,"owner membership",step);
        }
    }
    for (unsigned slot : {0u,2u}) {
        const auto &owners=slot==0?c.indexSpans:c.vertexSpans;
        const auto &revisions=slot==0?c.indexRevisions:c.vertexRevisions;
        require(owners.size()==revisions.size(),"revision owner count",step);
        for (const auto &[id,keys]:owners) {
            require(!keys.empty(),"empty owner",step);
            for (const auto &key:keys)
                require(key[slot]==id&&c.entries.contains(key),"stale owner key",step);
        }
    }
    require(bytes==c.topologyBytes,"wrong payload byte count",step);
    require(bytes<=Cache::topologyBudget,"payload budget",step);
}
int main(int argc, char **argv) {
    unsigned steps=argc>1?std::strtoul(argv[1],nullptr,10):300000;
    Cache cache;
    std::mt19937_64 rng(0xA63B31D4);
    std::uint64_t ir[192]{},vr[192]{};
    for (unsigned step=0;step<steps;++step) {
        auto i=rng()%192,v=rng()%192;
        if (rng()%19==0) ++ir[i];
        if (rng()%23==0) ++vr[v];
        Cache::Key key={i,ir[i],v,vr[v],rng()%48,4096,0,0,4096,4096};
        cache.observe(key);
        if (!cache.find(key)) cache.insert(key,{0,4095});
        if (rng()%3==0) {
            const auto *topology=cache.unique(key,[](std::uint64_t k){return std::uint32_t(k);});
            require(topology&&topology->size()==4096,"topology result",step);
            if (rng()%2==0) {
                unsigned bones=1+rng()%256;
                auto boxes=cache.animatedBoxes(key,rng()%3,44,bones,[bones](std::uint32_t k){
                    storm_metal::animated_bounds::Vertex v{};
                    v.position[0]=float(k); v.weight=1;
                    v.packedBones=k%bones;
                    return v;
                });
                require(boxes&&!boxes->empty(),"box result",step);
                require(topology->size()==4096,"topology pointer survives box budget",step);
            }
        }
        if (step%100==0) invariant(cache,step);
    }
    invariant(cache,steps);
    // Pure non-topology cap pressure, then intersecting identity invalidations.
    cache.clear();
    for(unsigned step=0;step<40000;++step) {
        Cache::Key key={step%127,0,step%131,0,step,3,0,0,3,3};
        cache.observe(key);cache.insert(key,{0,2});
        if(step%500==0) invariant(cache,step);
    }
    for (unsigned id=0;id<192;++id) {
        Cache::Key key={id,1,id,1,0,3,0,0,3,3};cache.observe(key);
        invariant(cache,id);
    }
    cache.clear();invariant(cache,steps);
    std::printf("PASS %u random draws + 40000 LRU draws + 192 crossed revision invalidations\n",steps);
    return 0;
}
