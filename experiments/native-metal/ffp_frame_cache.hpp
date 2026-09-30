#pragma once
#include "frame_arena.hpp"
#include <array>
#include <list>
#include <map>
#include <set>
// Immutable converted slices use dedicated shared Metal buffers and remain valid
// across Present. Exact identities/revisions reject stale source data.
struct FFPFrameCache {
 using Key=std::array<uint64_t,14>;
 struct Value {FrameArena::Slice vertices,indices,direct;std::array<float,3> localMin{},localMax{};};
 static constexpr uint64_t byteBudget=512ull*1024*1024;
 struct Entry {Value value;std::list<Key>::iterator recent;};
 std::map<Key,Entry> entries;
 std::list<Key> recency;
 std::set<Key> pending;
 std::map<uint64_t,uint64_t> vertexRevisions,indexRevisions;
 std::map<uint64_t,size_t> vertexReferences,indexReferences;
 uint64_t residentBytes=0;
 static uint64_t bytes(const Value&value){return value.vertices.length+value.indices.length+value.direct.length;}
 void erase(std::map<Key,Entry>::iterator i){
  residentBytes-=bytes(i->second.value);recency.erase(i->second.recent);
  if(!--vertexReferences[i->first[0]]){vertexReferences.erase(i->first[0]);vertexRevisions.erase(i->first[0]);}
  if(!--indexReferences[i->first[2]]){indexReferences.erase(i->first[2]);indexRevisions.erase(i->first[2]);}
  entries.erase(i);
 }
 void observe(uint64_t vertexIdentity,uint64_t vertexRevision,uint64_t indexIdentity,uint64_t indexRevision){
  const bool vertexChanged=vertexRevisions.contains(vertexIdentity)&&vertexRevisions[vertexIdentity]!=vertexRevision;
  const bool indexChanged=indexRevisions.contains(indexIdentity)&&indexRevisions[indexIdentity]!=indexRevision;
  if(vertexChanged||indexChanged)for(auto i=entries.begin();i!=entries.end();){
   if((vertexChanged&&i->first[0]==vertexIdentity)||(indexChanged&&i->first[2]==indexIdentity)){auto stale=i++;erase(stale);}else ++i;
  }
 }
 const Value* find(const Key&key){auto i=entries.find(key);if(i==entries.end())return nullptr;recency.splice(recency.end(),recency,i->second.recent);return &i->second.value;}
 // A first-seen conversion uses the frame arena. Allocate dedicated buffers
 // only after the exact state repeats; moving lights need not create hundreds
 // of persistent buffers for signatures that will never be reused.
 bool admit(const Key&key){if(pending.contains(key))return true;if(pending.size()>=8192)pending.clear();pending.insert(key);return false;}
 // Time-varying lighting and earlier locations must not permanently occupy the
 // budget. Encoded commands retain their immutable Metal buffers after eviction.
 bool canInsert(uint64_t length){if(length>byteBudget)return false;while(length>byteBudget-residentBytes||entries.size()>=8192)erase(entries.find(recency.front()));return true;}
 bool insert(const Key&key,Value value){if(entries.contains(key))return false;const uint64_t length=bytes(value);if(!canInsert(length))return false;recency.push_back(key);entries.emplace(key,Entry{value,std::prev(recency.end())});pending.erase(key);residentBytes+=length;vertexRevisions[key[0]]=key[1];indexRevisions[key[2]]=key[3];++vertexReferences[key[0]];++indexReferences[key[2]];return true;}
 void clear(){entries.clear();recency.clear();pending.clear();vertexRevisions.clear();indexRevisions.clear();vertexReferences.clear();indexReferences.clear();residentBytes=0;}
};
