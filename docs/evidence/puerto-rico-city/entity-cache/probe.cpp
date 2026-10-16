// Task-local CPU evidence, not a new permanent test target.
// Uses the actual EntityContainerCache implementation unchanged. EntityManager
// CalculateEntityId packs milliseconds in the high word and the slot below;
// reverse retirement order followed by NewLifecycle's LIFO queues can recycle
// slot11 before slot10. Both recreated IDs can receive the same millisecond.
#include <memory>
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>
#include "entity_container_cache.h"

static entid_t id(unsigned milliseconds, unsigned slot)
{
    return entid_t(milliseconds) << 32 | slot;
}
static void dump(const char *phase, const std::vector<entid_t> &ids)
{
    std::printf("%s count=%zu sorted=%d ids=", phase, ids.size(), int(std::is_sorted(ids.begin(), ids.end())));
    for (auto value : ids) std::printf("%llu,", static_cast<unsigned long long>(value));
    std::puts("");
}
int main(int argc, char **argv)
{
    const bool expectBug = argc == 2 && !std::strcmp(argv[1], "--expect-bug");
    if (argc > 2 || (argc == 2 && !expectBug)) return 64;
    constexpr hash_t hash = 0x1234;
    EntityContainerCache warm;
    const std::vector<entid_t> prior = {id(100, 10), id(100, 11)};
    for (const auto value : prior) warm.Add(hash, value);
    // Remove11 then10: the manager's deleted queue pops10 then11, placing11
    // at the top of its free queue. Creation consequently yields11 then10.
    warm.UpdateErase(hash, prior[1]);
    warm.UpdateErase(hash, prior[0]);
    if (!warm.Contains(hash) || !warm.Get(hash).empty()) return 2;
    const std::vector<entid_t> active = {id(101, 11), id(101, 10)};
    EntityContainerCache fresh;
    for (const auto value : active)
    {
        warm.UpdateAdd(hash, value);
        fresh.Add(hash, value);
    }
    dump("reactivated_warm", warm.Get(hash));
    dump("reactivated_fresh", fresh.Get(hash));
    const bool orderMismatch = warm.Get(hash) != fresh.Get(hash);
    const bool identical = !orderMismatch && std::is_sorted(warm.Get(hash).begin(), warm.Get(hash).end());
    for (const auto value : active)
    {
        warm.UpdateErase(hash, value);
        fresh.UpdateErase(hash, value);
    }
    dump("retired_warm", warm.Get(hash));
    dump("retired_fresh", fresh.Get(hash));
    const bool stale = !warm.Get(hash).empty() && fresh.Get(hash).empty();
    const bool clean = warm.Get(hash).empty() && fresh.Get(hash).empty();

    // Nearest normal case: decreasing reused slots, but distinct timestamps.
    EntityContainerCache normal;
    normal.Add(hash, id(99, 1)); normal.UpdateErase(hash, id(99, 1));
    const std::vector<entid_t> ordered = {id(101, 11), id(102, 10)};
    for (const auto value : ordered) normal.UpdateAdd(hash, value);
    dump("distinct_tick_warm", normal.Get(hash));
    for (const auto value : ordered) normal.UpdateErase(hash, value);
    dump("distinct_tick_retired", normal.Get(hash));
    const bool allowed = normal.Get(hash).empty();

    EntityContainerCache isolation;
    constexpr hash_t otherHash = 0x5678, uncachedHash = 0x9abc;
    isolation.Add(otherHash, id(101, 77));
    const auto otherBefore = isolation.Get(otherHash);
    isolation.UpdateAdd(uncachedHash, id(101, 11));
    isolation.UpdateErase(uncachedHash, id(101, 11));
    const bool uncachedNoOp = !isolation.Contains(uncachedHash);
    isolation.Add(hash, id(99, 1)); isolation.UpdateErase(hash, id(99, 1));
    for (const auto value : active) isolation.UpdateAdd(hash, value);
    for (const auto value : active) isolation.UpdateErase(hash, value);
    const bool otherUnchanged = isolation.Contains(otherHash) && isolation.Get(otherHash) == otherBefore;
    std::printf("bug_reproduced=%d ordered_equivalence=%s retirement_empty=%s nearest_normal=%s uncached_noop=%s other_class=%s\n",
                int(orderMismatch && stale), identical ? "PASS" : "FAIL", clean ? "PASS" : "FAIL",
                allowed ? "PASS" : "FAIL", uncachedNoOp ? "PASS" : "FAIL", otherUnchanged ? "PASS" : "FAIL");
    const bool behavior = expectBug ? orderMismatch && stale : identical && clean;
    return behavior && allowed && uncachedNoOp && otherUnchanged ? 0 : 3;
}
