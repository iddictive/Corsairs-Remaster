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
    cache.clear();
    check(cache.entries.empty() && cache.residentBytes == 0, "Reset releases persistent geometry");
    std::puts("PASS: static conversion survives 120 Presents; VB/IB revisions and Reset invalidate; 512 MiB bound enforced");
}
