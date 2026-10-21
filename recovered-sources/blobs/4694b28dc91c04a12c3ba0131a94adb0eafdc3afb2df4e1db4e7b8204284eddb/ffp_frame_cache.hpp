#pragma once
#include "frame_arena.hpp"
#include <array>
#include <map>
// Immutable converted slices use dedicated shared Metal buffers and remain valid
// across Present. Exact identities/revisions reject stale source data.
struct FFPFrameCache {
 using Key=std::array<uint64_t,13>;
 struct Value {FrameArena::Slice vertices,indices;};
 static constexpr uint64_t byteBudget=512ull*1024*1024;
 std::map<Key,Value> entries;
 std::map<uint64_t,uint64_t> vertexRevisions,indexRevisions;
 uint64_t residentBytes=0;
 static uint64_t bytes(const Value&value){return value.vertices.length+value.indices.length;}
 void observe(uint64_t vertexIdentity,uint64_t vertexRevision,uint64_t indexIdentity,uint64_t indexRevision){
  const bool vertexChanged=vertexRevisions.contains(vertexIdentity)&&vertexRevisions[vertexIdentity]!=vertexRevision;
  const bool indexChanged=indexRevisions.contains(indexIdentity)&&indexRevisions[indexIdentity]!=indexRevision;
  if(vertexChanged||indexChanged)for(auto i=entries.begin();i!=entries.end();){
   if((vertexChanged&&i->first[0]==vertexIdentity)||(indexChanged&&i->first[2]==indexIdentity)){residentBytes-=bytes(i->second);i=entries.erase(i);}else ++i;
  }
  if(vertexChanged||indexChanged){vertexRevisions.clear();indexRevisions.clear();for(const auto&entry:entries){vertexRevisions[entry.first[0]]=entry.first[1];indexRevisions[entry.first[2]]=entry.first[3];}}
 }
 const Value* find(const Key&key)const{auto i=entries.find(key);return i==entries.end()?nullptr:&i->second;}
 bool canInsert(uint64_t length)const{return length<=byteBudget-residentBytes&&entries.size()<8192;}
 bool insert(const Key&key,Value value){const uint64_t length=bytes(value);if(!canInsert(length))return false;auto [i,added]=entries.emplace(key,value);if(added){residentBytes+=length;vertexRevisions[key[0]]=key[1];indexRevisions[key[2]]=key[3];}return added;}
 void clear(){entries.clear();vertexRevisions.clear();indexRevisions.clear();residentBytes=0;}
};
