#pragma once

#include "indexed_geometry.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <list>
#include <map>
#include <utility>
#include <vector>

namespace sm {

// DrawIndexedPrimitive validates the complete source index range before any
// vertex access. Identity/revision keys remain valid across Present, avoiding
// the same full scan every frame as well as across shadow cube faces.
//
// A revision change must drop every span admitted for that buffer. Each identity
// keeps its own span list, so rebuilding one buffer costs the spans of that
// buffer instead of a walk over the whole cache plus a rebuild of the revision
// metadata; a sail or foam scene reaches the 8192-span cap and paid that walk
// once per rebuilt buffer per frame.
struct IndexedSpanFrameCache {
    using Key = std::array<std::uint64_t, 10>;
    using IdentitySpans = std::map<std::uint64_t, std::vector<Key>>;

    struct Entry { IndexedSpan span; std::list<Key>::iterator recent; };
    std::map<Key, Entry> entries;
    std::list<Key> recency;
    std::map<std::uint64_t, std::uint64_t> indexRevisions, vertexRevisions;
    IdentitySpans indexSpans, vertexSpans;

    void observe(const Key &key)
    {
        const auto index = indexRevisions.find(key[0]);
        if (index != indexRevisions.end() && index->second != key[1])
            evict(0, key[0]);
        const auto vertex = vertexRevisions.find(key[2]);
        if (vertex != vertexRevisions.end() && vertex->second != key[3])
            evict(2, key[2]);
    }

    const IndexedSpan *find(const Key &key)
    {
        const auto entry = entries.find(key);
        if (entry == entries.end())
            return nullptr;
        recency.splice(recency.end(), recency, entry->second.recent);
        return &entry->second.span;
    }

    void insert(const Key &key, IndexedSpan span)
    {
        if (entries.contains(key))
            return;
        // Old locations cannot permanently prevent later spans from caching.
        // Keep the established bound and replace only the least-recent hit.
        if (entries.size() >= 8192)
            erase(entries.find(recency.front()));
        recency.push_back(key);
        entries.emplace(key, Entry{span, std::prev(recency.end())});
        indexRevisions[key[0]] = key[1];
        vertexRevisions[key[2]] = key[3];
        indexSpans[key[0]].push_back(key);
        vertexSpans[key[2]].push_back(key);
    }

    void clear()
    {
        entries.clear();
        recency.clear();
        indexRevisions.clear();
        vertexRevisions.clear();
        indexSpans.clear();
        vertexSpans.clear();
    }

  private:
    void unlink(IdentitySpans &owners,
                std::map<std::uint64_t, std::uint64_t> &revisions,
                const Key &key, unsigned slot)
    {
        const auto owner = owners.find(key[slot]);
        if (owner == owners.end())
            return;
        auto &keys = owner->second;
        keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
        if (keys.empty()) {
            revisions.erase(owner->first);
            owners.erase(owner);
        }
    }

    void erase(std::map<Key, Entry>::iterator entry)
    {
        const Key key = entry->first;
        recency.erase(entry->second.recent);
        entries.erase(entry);
        unlink(indexSpans, indexRevisions, key, 0);
        unlink(vertexSpans, vertexRevisions, key, 2);
    }

    // Slot 0 owns the index identity, slot 2 the vertex identity. Removing a span
    // through one owner unlinks it from the other and drops the peer revision
    // once that buffer has no admitted span left.
    void evict(unsigned slot, std::uint64_t identity)
    {
        IdentitySpans &owners = slot == 0 ? indexSpans : vertexSpans;
        std::map<std::uint64_t, std::uint64_t> &ownerRevisions = slot == 0 ? indexRevisions : vertexRevisions;
        const auto owner = owners.find(identity);
        if (owner == owners.end()) {
            ownerRevisions.erase(identity);
            return;
        }
        // Detach this owner's list before exact erasure mutates both lists.
        auto keys = std::move(owner->second);
        owners.erase(owner);
        ownerRevisions.erase(identity);
        for (const Key &key : keys)
            erase(entries.find(key));
    }
};

} // namespace sm
