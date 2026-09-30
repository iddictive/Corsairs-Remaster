#include "indexed_span_frame_cache.hpp"

#include <cstdio>
#include <cstdlib>

static void check(bool ok, const char *message)
{
    if (!ok) {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    const std::uint16_t indices[] = {2, 3, 4, 4, 3, 5};
    sm::IndexedSpanFrameCache cache;
    unsigned scans = 0;

    auto resolve = [&](sm::IndexedSpanFrameCache::Key key, int base = -2,
                       unsigned minimum = 2, unsigned declared = 4,
                       unsigned available = 4) -> const sm::IndexedSpan * {
        cache.observe(key);
        if (const auto *hit = cache.find(key))
            return hit;
        sm::IndexedSpan span;
        ++scans;
        if (!sm::indexedSpan(indices, false, 6, base, minimum, declared, available, span))
            return nullptr;
        cache.insert(key, span);
        return cache.find(key);
    };

    // Identity, revisions and exact draw bounds are supplied by the backend.
    sm::IndexedSpanFrameCache::Key key = {11, 4, 21, 7, 0, 6, std::uint64_t(-2), 2, 4, 4};
    for (unsigned face = 0; face < 48; ++face) {
        const auto *span = resolve(key);
        check(span && span->first == 2 && span->last == 5,
              "same immutable draw retains its validated span");
    }
    check(scans == 1, "eight six-face lamps scan the index range once");
    for (unsigned frame = 1; frame < 120; ++frame)
        for (unsigned face = 0; face < 48; ++face)
            check(resolve(key), "immutable draw remains valid across Present");
    check(scans == 1, "120 frames retain the validated immutable span");

    auto changed = key;
    ++changed[1];
    check(resolve(changed), "IB revision change remains valid");
    check(scans == 2, "IB revision change misses the cache");
    changed = key;
    ++changed[4];
    check(resolve(changed), "index start change remains independently keyed");
    check(scans == 3, "index start change misses the cache");
    changed = key;
    ++changed[5];
    check(resolve(changed), "reference count change remains independently keyed");
    check(scans == 4, "reference count change misses the cache");
    changed = key;
    --changed[9];
    check(!resolve(changed, -2, 2, 4, 3), "smaller available VB range is rejected");
    check(scans == 5, "available VB range change misses the cache");

    changed = key;
    --changed[7];
    check(!resolve(changed, -2, 1, 4, 4), "index below declared minimum is rejected");
    check(!resolve(changed, -2, 1, 4, 4), "invalid range is not cached");
    check(scans == 7, "invalid ranges are revalidated on every submission");

    const auto trackedIndexRevisions = cache.indexRevisions.size();
    const auto trackedVertexRevisions = cache.vertexRevisions.size();
    for (std::uint64_t identity = 100; identity < 100100; ++identity) {
        auto unseen = key;
        unseen[0] = identity;
        unseen[2] = identity + 100000;
        cache.observe(unseen);
    }
    check(cache.indexRevisions.size() == trackedIndexRevisions &&
              cache.vertexRevisions.size() == trackedVertexRevisions,
          "uncached identities do not grow revision metadata");
    for (std::uint64_t identity = 100; identity < 100100; ++identity) {
        auto churn = key;
        churn[0] = identity;
        churn[1] = 1;
        churn[2] = identity + 100000;
        churn[3] = 1;
        cache.insert(churn, {2, 5});
        auto revised = churn;
        revised[1] = 2;
        revised[3] = 2;
        cache.observe(revised);
        check(!cache.find(churn), "churn revision evicts stale span");
    }
    check(cache.indexRevisions.size() == trackedIndexRevisions &&
              cache.vertexRevisions.size() == trackedVertexRevisions,
          "admitted identity churn does not grow revision metadata");

    cache.clear();
    check(resolve(key), "Reset revalidates the immutable draw");
    check(scans == 8, "Reset ends span reuse");

    std::puts("PASS: 5760 repeated shadow submissions scan once; revisions/bounds miss; invalid ranges reject; Reset revalidates");
}
