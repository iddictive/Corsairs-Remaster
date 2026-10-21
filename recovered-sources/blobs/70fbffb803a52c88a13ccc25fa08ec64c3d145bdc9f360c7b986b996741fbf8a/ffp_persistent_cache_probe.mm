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
    FFPFrameCache::Key key = {11, 1, 22, 1, 0, 28, 0x142, 0, 0, 101, 4, 2, 3};
    FFPFrameCache::Value value{};
    value.vertices.length = 4096;
    value.indices.length = 24;
    cache.observe(11, 1, 22, 1);
    check(cache.canInsert(4120), "bounded immutable geometry fits cache");
    check(cache.insert(key, value), "exact source revision inserts");
    for (unsigned frame = 0; frame < 120; ++frame)
        check(cache.find(key), "static conversion survives Present");
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
    check(cache.entries.empty() && cache.residentBytes == 0, "Reset releases persistent geometry");
    std::puts("PASS: static conversion survives 120 Presents; VB/IB revisions and Reset invalidate; 512 MiB bound enforced");
}
