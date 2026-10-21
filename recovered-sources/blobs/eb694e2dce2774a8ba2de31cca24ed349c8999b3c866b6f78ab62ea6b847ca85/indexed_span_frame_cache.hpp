#pragma once

#include "indexed_geometry.hpp"

#include <array>
#include <cstdint>
#include <map>

namespace sm {

// DrawIndexedPrimitive validates the complete source index range before any
// vertex access. Identity/revision keys remain valid across Present, avoiding
// the same full scan every frame as well as across shadow cube faces.
struct IndexedSpanFrameCache {
    using Key = std::array<std::uint64_t, 10>;

    std::map<Key, IndexedSpan> entries;
    std::map<std::uint64_t, std::uint64_t> indexRevisions, vertexRevisions;

    void observe(const Key &key)
    {
        const bool indexChanged = indexRevisions.contains(key[0]) && indexRevisions[key[0]] != key[1];
        const bool vertexChanged = vertexRevisions.contains(key[2]) && vertexRevisions[key[2]] != key[3];
        if (indexChanged || vertexChanged) {
            for (auto entry = entries.begin(); entry != entries.end();) {
                if ((indexChanged && entry->first[0] == key[0]) || (vertexChanged && entry->first[2] == key[2]))
                    entry = entries.erase(entry);
                else
                    ++entry;
            }
        }
        indexRevisions[key[0]] = key[1];
        vertexRevisions[key[2]] = key[3];
    }

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

    void clear() { entries.clear(); indexRevisions.clear(); vertexRevisions.clear(); }
};

} // namespace sm
