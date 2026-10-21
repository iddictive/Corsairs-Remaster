#pragma once

#include "indexed_geometry.hpp"

#include <array>
#include <cstdint>
#include <map>

namespace sm {

// DrawIndexedPrimitive validates the complete source index range before any
// vertex access. Shadow cube faces replay the same immutable buffers many times
// in one frame, so retain that validated span until Present rewinds frame state.
struct IndexedSpanFrameCache {
    using Key = std::array<std::uint64_t, 10>;

    std::map<Key, IndexedSpan> entries;

    const IndexedSpan *find(const Key &key) const
    {
        const auto entry = entries.find(key);
        return entry == entries.end() ? nullptr : &entry->second;
    }

    void insert(const Key &key, IndexedSpan span)
    {
        if (entries.size() < 8192)
            entries.emplace(key, span);
    }

    void clear() { entries.clear(); }
};

} // namespace sm
