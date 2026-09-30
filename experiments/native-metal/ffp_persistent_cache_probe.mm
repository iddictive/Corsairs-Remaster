#include "ffp_frame_cache.hpp"

#include <cstdio>
#include <cstdlib>

static void check(bool condition, const char *message)
{
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    FFPFrameCache cache;
    FFPFrameCache::Key key = {11, 1, 22, 1, 0, 28, 0x142, 0, 0, 101, 4, 2, 3, 99};
    FFPFrameCache::Value value{};
    value.vertices.length = 4096;
    value.indices.length = 24;
    value.direct.length = 2048;
    for (uint64_t frame=0; frame<120; ++frame) {
        auto changing=key;changing[13]=1000+frame;
        check(!cache.admit(changing), "never-reused lighting stays in frame arena");
    }
    check(!cache.admit(key) && cache.admit(key), "only an exact repeated state admits persistent allocation");
    cache.observe(11, 1, 22, 1);
    check(cache.canInsert(6168), "bounded immutable lit geometry and shadow fractions fit cache");
    check(cache.insert(key, value), "exact source revision inserts");
    for (unsigned frame = 0; frame < 120; ++frame)
        check(cache.find(key) && cache.find(key)->direct.length == 2048,
              "lit conversion and shadow fractions survive Present");
    auto changedLighting = key;
    changedLighting[13]++;
    check(!cache.find(changedLighting), "changed conversion state misses exact cache key");
    cache.observe(11, 2, 22, 1);
    check(!cache.find(key), "VB write invalidates stale conversion");
    key[1] = 2;
    check(cache.insert(key, value), "new VB revision inserts independently");
    cache.observe(11, 2, 22, 2);
    check(!cache.find(key), "IB write invalidates stale conversion");
    check(!cache.canInsert(FFPFrameCache::byteBudget + 1), "oversized geometry bypasses cache");
    const auto trackedVertexRevisions = cache.vertexRevisions.size();
    const auto trackedIndexRevisions = cache.indexRevisions.size();
    for (std::uint64_t identity = 100; identity < 100100; ++identity)
        cache.observe(identity, 1, identity + 100000, 1);
    check(cache.vertexRevisions.size() == trackedVertexRevisions &&
              cache.indexRevisions.size() == trackedIndexRevisions,
          "uncached identities do not grow revision metadata");
    for (std::uint64_t identity = 100; identity < 100100; ++identity) {
        auto churn = key;
        churn[0] = identity;
        churn[1] = 1;
        churn[2] = identity + 100000;
        churn[3] = 1;
        check(cache.insert(churn, value), "churn identity inserts within released budget");
        cache.observe(identity, 2, identity + 100000, 2);
        check(!cache.find(churn), "churn revision evicts stale conversion");
    }
    check(cache.vertexRevisions.size() == trackedVertexRevisions &&
              cache.indexRevisions.size() == trackedIndexRevisions,
          "admitted identity churn does not grow revision metadata");
    cache.clear();
    auto large = value;
    large.vertices.length = FFPFrameCache::byteBudget / 2;
    large.indices.length = large.direct.length = 0;
    auto cold = key, warm = key, next = key;
    cold[0] = 301; warm[0] = 302; next[0] = 303;
    check(cache.insert(cold, large) && cache.insert(warm, large), "fill budget with two conversions");
    check(cache.find(cold), "reuse promotes geometry in recency order");
    check(cache.insert(next, large), "new live state can replace a full historical cache");
    check(cache.find(cold) && cache.find(next) && !cache.find(warm), "evict only least recently used conversion");
    check(cache.residentBytes == FFPFrameCache::byteBudget &&
              cache.vertexReferences.size() == 2 && cache.vertexRevisions.size() == 2,
          "budget eviction releases stale revision and reference metadata");
    cache.observe(301, key[1] + 1, key[2], key[3]);
    check(!cache.find(cold) && cache.find(next), "source mutation still invalidates admitted LRU state");
    cache.clear();
    check(cache.entries.empty() && cache.residentBytes == 0, "Reset releases persistent geometry");
    std::puts("PASS: lit conversion and shadow fractions survive 120 Presents; state/VB/IB changes miss or invalidate; 512 MiB bound enforced");
}
