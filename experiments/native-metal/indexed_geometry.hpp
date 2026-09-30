#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <limits>
#include <algorithm>
namespace sm {
struct IndexedSpan {uint32_t first=0,last=0;uint32_t vertices()const{return last-first+1;}};
inline uint32_t readIndex(const void*bytes,bool wide,size_t index){
 if(wide){uint32_t value;std::memcpy(&value,static_cast<const uint8_t*>(bytes)+index*4,4);return value;}
 uint16_t value;std::memcpy(&value,static_cast<const uint8_t*>(bytes)+index*2,2);return value;
}
// All source indices are relative to the signed base. Validate before forming
// any vertex pointer; declarations may cover a larger range than actually used.
inline bool indexedSpan(const void*indices,bool wide,uint32_t count,int32_t base,
                        uint32_t minimum,uint32_t declaredVertices,size_t availableVertices,IndexedSpan&span){
 if(!indices||!count||!declaredVertices)return false;
 uint64_t declaredEnd=uint64_t(minimum)+declaredVertices;
 int64_t firstVertex=int64_t(base)+minimum,endVertex=int64_t(base)+int64_t(declaredEnd);
 if(declaredEnd>uint64_t(UINT32_MAX)+1||firstVertex<0||endVertex<firstVertex||uint64_t(endVertex)>availableVertices)return false;
 span.first=UINT32_MAX;span.last=0;
 for(uint32_t i=0;i<count;i++){uint32_t value=readIndex(indices,wide,i);if(value<minimum||uint64_t(value)>=declaredEnd)return false;span.first=std::min(span.first,value);span.last=std::max(span.last,value);}
 return uint64_t(span.last)-span.first+1<=UINT32_MAX;
}
}
