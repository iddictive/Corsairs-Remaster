// Private CPU prototype: ordered source indices, no vertices/material changes.
#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <limits>
#include <map>
#include <memory>
#include <span>
#include <vector>

namespace ordered_indices {
struct Source {
    uint64_t identity, revision, offset;
    uint32_t count, width;
    std::span<const uint8_t> bytes;
};
using Key = std::vector<std::array<uint64_t, 5>>;
using Indices = std::vector<uint32_t>;
enum class Reject { none, empty, packets, format, range, capacity, index };
struct Result { std::shared_ptr<const Indices> indices; Reject reject{}; bool built=false; };

// The caller owns immutable source identities/revisions. GPU residency/lifetime
// is deliberately absent; an eventual adapter must own it in its frame ring.
class Cache {
    struct Entry { std::shared_ptr<const Indices> indices; uint64_t used; };
    std::map<Key, Entry> entries;
    size_t retained=0, limit;
    uint64_t clock=0;
public:
    explicit Cache(size_t bytes=32u*1024u*1024u):limit(bytes) {}
    size_t bytes() const { return retained; }
    size_t size() const { return entries.size(); }
    Result get(std::span<const Source> sources) {
        if(sources.empty()) return {{},Reject::empty};
        if(sources.size()>4096) return {{},Reject::packets};
        Key key; key.reserve(sources.size()); size_t count=0;
        for(const auto& s:sources) {
            if(!s.identity || (s.width!=2 && s.width!=4) || s.count%3) return {{},Reject::format};
            if(s.offset>s.bytes.size() || uint64_t(s.count)*s.width>s.bytes.size()-s.offset) return {{},Reject::range};
            if(s.count>(limit/sizeof(uint32_t))-count) return {{},Reject::capacity};
            count+=s.count;
            // Packet boundaries and original order are represented by the key
            // sequence, including repeated spans. Base/VB live in the table.
            key.push_back({s.identity,s.revision,s.offset,s.count,s.width});
        }
        auto hit=entries.find(key);
        if(hit!=entries.end()) {hit->second.used=++clock;return {hit->second.indices};}
        auto data=std::make_shared<Indices>(count); size_t offset=0;
        for(size_t packet=0;packet<sources.size();++packet) {
            const auto& s=sources[packet];
            const uint32_t prefix=uint32_t(packet)<<20; const auto* source=s.bytes.data()+s.offset;
            auto* destination=data->data()+offset;
            // UInt16 necessarily fits20 bits. A fixed-width loop lets Clang
            // widen and pack the original indices without per-index growth.
            if(s.width==2) for(uint32_t i=0;i<s.count;++i) {
                uint16_t index;std::memcpy(&index,source+size_t(i)*2,2);destination[i]=prefix|index;
            } else {
                uint32_t maximum=0;
                for(uint32_t i=0;i<s.count;++i) {
                    uint32_t index;std::memcpy(&index,source+size_t(i)*4,4);maximum=std::max(maximum,index);destination[i]=prefix|index;
                }
                if(maximum>0xfffffu) return {{},Reject::index};
            }
            offset+=s.count;
        }
        const size_t bytes=data->size()*sizeof(uint32_t);
        while(retained>limit-bytes && !entries.empty()) {
            auto oldest=std::min_element(entries.begin(),entries.end(),[](const auto& a,const auto& b){return a.second.used<b.second.used;});
            retained-=oldest->second.indices->size()*sizeof(uint32_t);entries.erase(oldest);
        }
        entries.emplace(std::move(key),Entry{data,++clock});retained+=bytes;
        return {data,Reject::none,true};
    }
};
}
