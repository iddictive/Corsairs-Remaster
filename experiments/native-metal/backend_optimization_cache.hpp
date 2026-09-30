#pragma once
// CPU-only cache: never owns Metal objects, GPU addresses, or frame allocations.
#include <array>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace storm_metal {
namespace backend_optimization {

// Vertex/index identity + revision and the complete indexed draw addressing.
// WORLD, VIEW and the camera origin are deliberately absent: values are LOCAL.
struct BoundsKey {
    std::uint64_t vertexIdentity=0, vertexRevision=0;
    std::uint64_t indexIdentity=0, indexRevision=0;
    std::uint64_t vertexOffset=0, indexOffset=0, indexCount=0;
    std::uint64_t stride=0, baseVertex=0, indexWidth=0, fvf=0;

    std::array<std::uint64_t, 11> words() const noexcept {
        return {{vertexIdentity, vertexRevision, indexIdentity, indexRevision,
                 vertexOffset, indexOffset, indexCount, stride, baseVertex,
                 indexWidth, fvf}};
    }
    bool operator==(const BoundsKey& other) const noexcept {
        return words()==other.words();
    }
    std::uint64_t hash() const noexcept {
        std::uint64_t h=0xcbf29ce484222325ULL;
        for (const auto word:words()) {
            h^=word;
            h*=0x100000001b3ULL;
            h^=h>>32;
        }
        return h;
    }
};

// Four candidates per set; every hit compares the FULL key, never just a hash.
// On conflict only the oldest entry in that set is replaced. No whole-cache
// flush, unbounded growth, per-draw heap allocation, or resource retention.
template<class Value, std::size_t SetCount=1024, std::size_t Ways=4>
class BoundsCache {
    static_assert(SetCount && (SetCount & (SetCount-1))==0,
                  "SetCount must be a power of two");
    static_assert(Ways>0, "A cache needs at least one way");
    struct Entry {
        BoundsKey key{};
        Value value{};
        std::uint64_t used=0; // zero means empty
    };
    std::array<std::array<Entry,Ways>,SetCount> entries_{};
    std::uint64_t clock_=0;

    std::uint64_t tick() noexcept {
        if (clock_==std::numeric_limits<std::uint64_t>::max()) clear();
        return ++clock_;
    }
public:
    static constexpr std::size_t capacity=SetCount*Ways;
    void clear() noexcept {
        for (auto& set:entries_) for (auto& entry:set) entry.used=0;
        clock_=0;
    }
    const Value* find(const BoundsKey& key) noexcept {
        const auto now=tick();
        auto& set=entries_[key.hash() & (SetCount-1)];
        for (auto& entry:set) {
            if (entry.used && entry.key==key) {
                entry.used=now;
                return &entry.value;
            }
        }
        return nullptr;
    }
    void insert(const BoundsKey& key,const Value& value) noexcept {
        const auto now=tick();
        auto& set=entries_[key.hash() & (SetCount-1)];
        Entry* victim=&set[0];
        for (auto& entry:set) {
            if (entry.used && entry.key==key) {
                entry.value=value;
                entry.used=now;
                return;
            }
            if (entry.used<victim->used) victim=&entry;
        }
        victim->key=key;
        victim->value=value;
        victim->used=now;
    }
};

struct Statistics {
    std::uint64_t frames=0, boundsHits=0, boundsMisses=0, boundsBypassed=0;
    std::uint64_t boundsSkipped=0, indicesScanned=0, indicesAvoided=0;
    std::uint64_t viewHits=0, viewMisses=0;
    std::uint64_t samplingShared=0, samplingFallbacks=0, samplingBytesSaved=0;
    double boundsMilliseconds=0;
};

} // namespace backend_optimization
} // namespace storm_metal
