// CPU-only actual index topology probe. No Metal, app or player-state access.
#include "ordered_indices.hpp"
#include <chrono>
#include <cstdio>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace ordered_indices;
static void need(bool pass,const char* message) {if(!pass)throw std::runtime_error(message);}
template<class T> static T read(std::ifstream& in) {T v{};in.read(reinterpret_cast<char*>(&v),sizeof(v));need(bool(in),"fixture read");return v;}
static double now() {return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now().time_since_epoch()).count();}
static void exact(std::span<const Source> spans,const Indices& packed) {
    size_t at=0;
    for(size_t packet=0;packet<spans.size();++packet) {
        const auto& s=spans[packet];
        for(uint32_t i=0;i<s.count;++i) {
            uint32_t original=0;std::memcpy(&original,s.bytes.data()+s.offset+size_t(i)*s.width,s.width);
            need(at<packed.size() && (packed[at]>>20)==packet && (packed[at]&0xfffffu)==original,"original triangle/packet order");++at;
        }
    }
    need(at==packed.size(),"packed count");
}
int main(int argc,char** argv) {try {
    need(argc==2,"usage: cache_probe index-fixture.bin");std::ifstream in(argv[1],std::ios::binary);
    char magic[8];in.read(magic,8);need(std::memcmp(magic,"OBIX0001",8)==0,"fixture magic");
    const auto buffers=read<uint32_t>(in);need(buffers<1024,"bounded source population");
    std::map<uint64_t,std::vector<uint8_t>> bytes;
    for(uint32_t i=0;i<buffers;++i) {auto id=read<uint64_t>(in),size=read<uint64_t>(in);need(size<64u*1024u*1024u,"bounded source bytes");auto& b=bytes[id];b.resize(size);in.read(reinterpret_cast<char*>(b.data()),size);need(bool(in),"source bytes");}
    const auto count=read<uint32_t>(in);need(count==12,"capture run count");std::vector<std::vector<Source>> runs(count);
    for(auto& run:runs) {auto size=read<uint32_t>(in);need(size<=4096,"packet capacity");for(uint32_t i=0;i<size;++i) {Source s{};s.identity=read<uint64_t>(in);s.revision=read<uint64_t>(in);s.offset=read<uint64_t>(in);s.count=read<uint32_t>(in);s.width=read<uint32_t>(in);s.bytes=bytes.at(s.identity);run.push_back(s);}}
    Cache cache;size_t references=0,draws=0;double cold=0,oracle=0;
    for(const auto& run:runs) {double begin=now();auto r=cache.get(run);cold+=now()-begin;need(bool(r.indices) && r.reject==Reject::none,"capture accepted");begin=now();exact(run,*r.indices);oracle+=now()-begin;references+=r.indices->size();draws+=run.size();}
    need(draws==770,"complete eligible draws");
    size_t rebuilt=0;double beginWarm=now();
    for(unsigned frame=0;frame<1000;++frame)for(const auto& run:runs) {auto r=cache.get(run);need(bool(r.indices),"warm accepted");rebuilt+=r.built;}
    double warmPerFrame=(now()-beginWarm)/1000.;need(rebuilt==0,"warm frame has no topology rebuild or index upload");
    // Prior valid state stays alive across changed source and order keys.
    const auto original=cache.get(runs[0]).indices;auto changed=runs[0];
    changed[0].revision++;auto revision=cache.get(changed);need(revision.built && revision.indices!=original,"revision invalidation");exact(changed,*revision.indices);
    changed=runs[0];std::swap(changed[0],changed[1]);auto order=cache.get(changed);need(order.built,"original sequence is key");exact(changed,*order.indices);
    changed=runs[0];changed[0].count-=3;auto resized=cache.get(changed);need(resized.built && resized.indices->size()+3==original->size(),"count invalidation");exact(changed,*resized.indices);
    changed=runs[0];changed[0].identity+=1000000;need(cache.get(changed).built,"same-address new identity invalidation");
    need(cache.get(runs[0]).indices==original,"prior valid topology preserved");
    Source invalid=runs[0][0];invalid.offset=invalid.bytes.size();need(cache.get(std::span(&invalid,1)).reject==Reject::range,"bad range rejected before lookup");
    invalid=runs[0][0];invalid.width=3;need(cache.get(std::span(&invalid,1)).reject==Reject::format,"unsupported width");
    uint32_t high=1u<<20;invalid={9999999,1,0,3,4,{reinterpret_cast<const uint8_t*>(&high),4}};
    uint32_t tooHigh[3]={0,1,1u<<20};invalid.bytes={reinterpret_cast<const uint8_t*>(tooHigh),sizeof(tooHigh)};need(cache.get(std::span(&invalid,1)).reject==Reject::index,"packing overflow fallback");
    std::vector<Source> tooMany(4097,runs[0][0]);need(cache.get(tooMany).reject==Reject::packets,"packet overflow fallback");
    Cache small(64);need(small.get(runs[0]).reject==Reject::capacity,"oversized batch fallback");
    // Eviction retains externally held immutable data; GPU adapter must retain
    // that version through frame completion rather than mutate it in place.
    auto smallRun=std::vector<Source>{runs[0][0]};Cache one(size_t(smallRun[0].count)*4);auto held=one.get(smallRun).indices;smallRun[0].revision++;need(one.get(smallRun).built && one.size()==1,"bounded eviction");exact(std::span(&runs[0][0],1),*held);
    std::printf("{\"status\":\"CPU cache and every original reference pass; no GPU\",\"runs\":%u,\"draws\":%zu,\"references\":%zu,\"cold_build_ms\":%.6f,\"separate_full_oracle_ms\":%.6f,\"warm_lookup_per_frame_ms\":%.6f,\"warm_frames\":1000,\"warm_rebuilds\":%zu,\"cache_bytes_after_negative_cases\":%zu,\"cache_limit_bytes\":33554432}\n",count,draws,references,cold,oracle,warmPerFrame,rebuilt,cache.bytes());
    return 0;
}catch(const std::exception& e) {std::fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
