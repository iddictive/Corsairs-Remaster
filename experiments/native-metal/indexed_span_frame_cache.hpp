#pragma once

#include "indexed_geometry.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <map>
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

    std::map<Key, IndexedSpan> entries;
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

    const IndexedSpan *find(const Key &key) const
    {
        const auto entry = entries.find(key);
        return entry == entries.end() ? nullptr : &entry->second;
    }

    void insert(const Key &key, IndexedSpan span)
    {
        if (entries.size() >= 8192)
            return;
        if (!entries.emplace(key, span).second)
            return;
        indexRevisions[key[0]] = key[1];
        vertexRevisions[key[2]] = key[3];
        indexSpans[key[0]].push_back(key);
        vertexSpans[key[2]].push_back(key);
    }

    void clear()
    {
        entries.clear();
        indexRevisions.clear();
        vertexRevisions.clear();
        indexSpans.clear();
        vertexSpans.clear();
    }

  private:
    // Slot 0 owns the index identity, slot 2 the vertex identity. Removing a span
    // through one owner unlinks it from the other and drops the peer revision
    // once that buffer has no admitted span left.
    void evict(unsigned slot, std::uint64_t identity)
    {
        IdentitySpans &owners = slot == 0 ? indexSpans : vertexSpans;
        IdentitySpans &peers = slot == 0 ? vertexSpans : indexSpans;
        std::map<std::uint64_t, std::uint64_t> &ownerRevisions = slot == 0 ? indexRevisions : vertexRevisions;
        std::map<std::uint64_t, std::uint64_t> &peerRevisions = slot == 0 ? vertexRevisions : indexRevisions;
        const unsigned peerSlot = slot == 0 ? 2 : 0;
        const auto owner = owners.find(identity);
        if (owner == owners.end()) {
            ownerRevisions.erase(identity);
            return;
        }
        for (const Key &key : owner->second) {
            entries.erase(key);
            const auto peer = peers.find(key[peerSlot]);
            if (peer == peers.end())
                continue;
            auto &keys = peer->second;
            keys.erase(std::remove(keys.begin(), keys.end(), key), keys.end());
            if (keys.empty()) {
                peerRevisions.erase(peer->first);
                peers.erase(peer);
            }
        }
        owners.erase(owner);
        ownerRevisions.erase(identity);
    }
};

} // namespace sm
