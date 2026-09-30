#include "gpu_skinning_bridge.hpp"

#include <array>
#include <bit>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <cstdio>

using storm::metal::skinning::AnimatedVertex;
using storm::metal::skinning::Matrix4x4;

struct OutVertex { float p[3], n[3]; std::int32_t color; float uv[2]; };

static OutVertex shaderOracle(const AnimatedVertex &v, const Matrix4x4 *p)
{
    const auto a = v.packedBones & 0xffu, b = (v.packedBones >> 8) & 0xffu;
    const float weights[2] = {v.weight, 1.0f - v.weight};
    const Matrix4x4 *m[2] = {&p[a], &p[b]};
    float blend[16]{};
    for (int i = 0; i != 16; ++i) blend[i] = m[0]->elements[i] * weights[0] + m[1]->elements[i] * weights[1];
    blend[0] = -blend[0]; blend[4] = -blend[4]; blend[8] = -blend[8]; blend[12] = -blend[12];
    OutVertex o{};
    for (int c = 0; c != 3; ++c) {
        o.p[c] = v.position[0] * blend[c] + v.position[1] * blend[4 + c] +
                 v.position[2] * blend[8 + c] + blend[12 + c];
        o.n[c] = v.normal[0] * blend[c] + v.normal[1] * blend[4 + c] + v.normal[2] * blend[8 + c];
    }
    o.color = v.color; o.uv[0] = v.uv[0]; o.uv[1] = v.uv[1];
    return o;
}

static std::array<std::uint8_t, 64> raster(const std::array<OutVertex, 3> &v)
{
    std::array<std::uint8_t, 64> pixels{};
    auto edge=[](float ax,float ay,float bx,float by,float x,float y){return (x-ax)*(by-ay)-(y-ay)*(bx-ax);};
    for(int y=0;y<8;++y) for(int x=0;x<8;++x) {
        const float px=x-3.5f, py=y-2.5f;
        const float e0=edge(v[0].p[0],v[0].p[1],v[1].p[0],v[1].p[1],px,py);
        const float e1=edge(v[1].p[0],v[1].p[1],v[2].p[0],v[2].p[1],px,py);
        const float e2=edge(v[2].p[0],v[2].p[1],v[0].p[0],v[0].p[1],px,py);
        pixels[y*8+x]=(e0<=0&&e1<=0&&e2<=0)||(e0>=0&&e1>=0&&e2>=0) ? 0xff : 0;
    }
    return pixels;
}

static std::uint64_t hash(const std::array<std::uint8_t,64>& p)
{
    std::uint64_t h=1469598103934665603ull;
    for(auto b:p){h^=b;h*=1099511628211ull;} return h;
}

struct Fake { int begins=0, binds=0, ends=0; };
static bool accept(void *,const AnimatedVertex *v,std::uint32_t n,std::uint32_t bones){
    for(std::uint32_t i=0;i<n;++i) if((v[i].packedBones&0xffu)>=bones||((v[i].packedBones>>8)&0xffu)>=bones)return false;
    return true;
}
static bool begin(void *c, void *, const Matrix4x4 *, std::uint32_t n){++static_cast<Fake*>(c)->begins;return n==2;}
static void *bind(void *c,const AnimatedVertex*,std::uint32_t,std::uint32_t,std::uint32_t){++static_cast<Fake*>(c)->binds;return c;}
static void end(void *c){++static_cast<Fake*>(c)->ends;}

int main()
{
    Matrix4x4 pose[2]{};
    for(auto &m:pose) m.elements[0]=m.elements[5]=m.elements[10]=m.elements[15]=1;
    pose[0].elements[12]=2; pose[0].elements[13]=1;
    pose[1].elements[12]=-2; pose[1].elements[13]=3;
    const std::array<AnimatedVertex,3> source={{
        {{0,0,0},1.0f,0xdead0100u,{1,0,0},std::int32_t(0xff102030u),{0,0}},
        {{1,0,0},0.0f,0xbeef0100u,{0,1,0},std::int32_t(0xff405060u),{1,0}},
        {{0,1,0},0.5f,0xabcd0100u,{1,1,0},std::int32_t(0xff708090u),{0,1}}
    }};
    const std::array<OutVertex,3> golden={{
        {{-2,1,0},{-1,0,0},std::int32_t(0xff102030u),{0,0}},
        {{1,3,0},{0,1,0},std::int32_t(0xff405060u),{1,0}},
        {{0,3,0},{-1,1,0},std::int32_t(0xff708090u),{0,1}}
    }};
    std::array<OutVertex,3> actual{};
    for(size_t i=0;i<source.size();++i){
        actual[i]=shaderOracle(source[i],pose);
        for(int c=0;c<3;++c){assert(actual[i].p[c]==golden[i].p[c]);assert(actual[i].n[c]==golden[i].n[c]);}
        assert(actual[i].color==golden[i].color);
        assert(std::bit_cast<std::uint32_t>(actual[i].uv[0])==std::bit_cast<std::uint32_t>(golden[i].uv[0]));
        assert(std::bit_cast<std::uint32_t>(actual[i].uv[1])==std::bit_cast<std::uint32_t>(golden[i].uv[1]));
    }
    const auto actualPixels=raster(actual), goldenPixels=raster(golden);
    assert(actualPixels==goldenPixels);
    constexpr std::uint64_t expectedMaskHash=8188581879532667129ull;
    const auto measured=hash(goldenPixels);
    if(expectedMaskHash==0){std::printf("mask-hash=%llu\n",(unsigned long long)measured);return 2;}
    assert(measured==expectedMaskHash);
    auto poseB=pose;poseB[0].elements[12]=4;poseB[1].elements[12]=-4;
    const auto moved=shaderOracle(source[0],poseB);assert(moved.p[0]!=actual[0].p[0]);
    assert(std::memcmp(moved.n,actual[0].n,sizeof(moved.n))==0);
    Fake fake; const storm::metal::skinning::Provider provider{&fake,accept,begin,bind,end};
    storm::metal::skinning::registerProvider(&provider);
    assert(storm::metal::skinning::acceptVertices(source.data(),3,2));
    auto invalid=source;invalid[0].packedBones=0x00000200u;
    assert(!storm::metal::skinning::acceptVertices(invalid.data(),3,2));
    assert(storm::metal::skinning::beginPose(nullptr,pose,2));
    assert(!storm::metal::skinning::beginPose(nullptr,pose,2));
    assert(storm::metal::skinning::bindVertices(source.data(),0,3,3)==&fake);
    storm::metal::skinning::endPose();
    assert(fake.begins==1&&fake.binds==1&&fake.ends==1);
    std::puts("PASS: AVERTEX0 pose bytes, pixel coverage, translation-invariant normals and bridge lifecycle match");
}
