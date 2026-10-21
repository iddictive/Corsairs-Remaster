#pragma once
#import <Metal/Metal.h>
#include <CommonCrypto/CommonDigest.h>
#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <string>
#include <vector>

// Experimental content-matched index replacement. The canonical renderer does
// not include this file. Only the frozen diagnostic backend consumes it.
struct ReflectionLod {
 struct Cooked {uint32_t sourceCount=0;std::vector<uint32_t> indices;};
 struct Selection {id<MTLBuffer> buffer=nil;NSUInteger count=0;};
 using Key=std::array<uint64_t,13>;
 std::map<std::string,Cooked> cooked;
 std::map<Key,Selection> cache;
 bool loaded=false,enabled=false;
 size_t bytes=0;
 uint64_t candidates=0,reduced=0,sourceReferences=0,submittedReferences=0;
 void clear(){cache.clear();bytes=0;}
 void load(){
  loaded=true;const char*path=std::getenv("STORM_METAL_REFLECTION_LOD_DATA");if(!path)return;
  std::ifstream f(path,std::ios::binary);char magic[8];uint32_t count=0;
  if(!f.read(magic,8)||std::memcmp(magic,"RLOD0001",8)||!f.read((char*)&count,4)||count>512)return;
  size_t total=0;
  for(uint32_t i=0;i<count;i++){
   char hash[64];uint32_t oldCount=0,newCount=0;
   if(!f.read(hash,64)||!f.read((char*)&oldCount,4)||!f.read((char*)&newCount,4)||newCount>oldCount||!newCount||newCount%3)return;
   total+=size_t(newCount)*4;if(total>32*1024*1024)return;
   Cooked row;row.sourceCount=oldCount;row.indices.resize(newCount);
   if(!f.read((char*)row.indices.data(),newCount*4))return;
   cooked.emplace(std::string(hash,64),std::move(row));
  }
  enabled=true;
 }
 Selection select(id<MTLDevice>device,const Key&key,const uint8_t*vertices,size_t stride,const void*indices,bool wide,uint32_t count,int32_t base){
  if(!loaded)load();if(!enabled)return {};++candidates;
  auto found=cache.find(key);if(found!=cache.end())return account(found->second,count);
  if(cache.size()>=2048)return {};
  std::vector<uint32_t> order(count);
  for(uint32_t i=0;i<count;i++){if(wide)std::memcpy(&order[i],(const uint8_t*)indices+i*4,4);else{uint16_t v;std::memcpy(&v,(const uint8_t*)indices+i*2,2);order[i]=v;}}
  auto unique=order;std::sort(unique.begin(),unique.end());unique.erase(std::unique(unique.begin(),unique.end()),unique.end());
  CC_SHA256_CTX context;CC_SHA256_Init(&context);
  for(auto index:unique)CC_SHA256_Update(&context,vertices+(int64_t(index)+base)*stride,CC_LONG(stride));
  for(auto index:order){uint32_t compact=uint32_t(std::lower_bound(unique.begin(),unique.end(),index)-unique.begin());CC_SHA256_Update(&context,&compact,4);}
  unsigned char digest[32];CC_SHA256_Final(digest,&context);char hex[65];for(unsigned i=0;i<32;i++)std::snprintf(hex+i*2,3,"%02x",digest[i]);
  Selection result;auto match=cooked.find(std::string(hex,64));
  if(match!=cooked.end()&&match->second.sourceCount==count&&match->second.indices.size()<count){
   std::vector<uint32_t> replacement;replacement.reserve(match->second.indices.size());
   for(auto index:match->second.indices){if(index>=unique.size())return {};replacement.push_back(unique[index]);}
   if(bytes+replacement.size()*4<=32*1024*1024){
    result.buffer=[device newBufferWithBytes:replacement.data() length:replacement.size()*4 options:MTLResourceStorageModeShared];
    if(result.buffer){result.count=replacement.size();bytes+=replacement.size()*4;}
   }
  }
  cache.emplace(key,result);return account(result,count);
 }
 Selection account(Selection result,uint32_t count){if(result.buffer){++reduced;sourceReferences+=count;submittedReferences+=result.count;}return result;}
};
