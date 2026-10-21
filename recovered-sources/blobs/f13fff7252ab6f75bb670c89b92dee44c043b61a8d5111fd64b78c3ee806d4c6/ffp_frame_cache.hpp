#pragma once
#include "frame_arena.hpp"
#include <array>
#include <map>
// Immutable converted slices live only until the owning device rewinds its arena.
// Key equality is exact; resource identities never reuse an address as identity.
struct FFPFrameCache {
 using Key=std::array<uint64_t,13>;
 struct Value {FrameArena::Slice vertices,indices;};
 std::map<Key,Value> entries;
 const Value* find(const Key&key)const{auto i=entries.find(key);return i==entries.end()?nullptr:&i->second;}
 void insert(const Key&key,Value value){if(entries.size()<8192)entries.emplace(key,value);}
 void clear(){entries.clear();}
};
