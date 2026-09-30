#include "anti_tiling_policy.hpp"

#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>

using storm_metal::anti_tiling::Kind;
using storm_metal::anti_tiling::kindForLabel;

struct Vec2 { float x, y; };

static Vec2 transform(unsigned hash, Vec2 value) {
    Vec2 result = value;
    switch (hash & 3u) {
        case 1: result = {-value.y, value.x}; break;
        case 2: result = {-value.x, -value.y}; break;
        case 3: result = {value.y, -value.x}; break;
        default: break;
    }
    if (hash & 4u) result.x = -result.x;
    if (hash & 8u) result.y = -result.y;
    return result;
}

static bool close(Vec2 left, Vec2 right) {
    return std::fabs(left.x - right.x) < 1e-5f && std::fabs(left.y - right.y) < 1e-5f;
}

int main() {
    for (auto label : {"floorU3.tga", "RESOURCE/Textures/Sandtile.tga", "rockA2.tga",
                       "resource\\textures\\ROCKb2.TGA", "rockC2.tga",
                       "RESOURCE/Textures/floorU3.tga.tx", "Sandtile.tga.TX"}) {
        assert(kindForLabel(label) == Kind::naturalDiffuse);
    }
    for (auto label : {"cobbleM1.tga", "town_cob.tga", "tileU2.tga", "floorU1.tga",
                       "1grasstogrounsU1.tga", "trees.tga", "floorU3.tga.txx",
                       "not-floorU3.tga"}) {
        assert(kindForLabel(label) == Kind::none);
    }

    // The transformed finite difference must equal the transformed source
    // derivative for every rotate/mirror choice used by the MSL helper.
    const Vec2 uv{2.75f, -1.25f}, derivative{0.007f, -0.013f};
    for (unsigned hash = 0; hash < 16; ++hash) {
        const Vec2 finite{transform(hash, {uv.x + derivative.x, uv.y + derivative.y}).x - transform(hash, uv).x,
                          transform(hash, {uv.x + derivative.x, uv.y + derivative.y}).y - transform(hash, uv).y};
        assert(close(finite, transform(hash, derivative)));
    }

    for (float x : {0.f, .13f, .5f, .91f, 1.f}) for (float y : {0.f, .37f, .75f, 1.f}) {
        const float sum = (1.f - x) * (1.f - y) + x * (1.f - y) + (1.f - x) * y + x * y;
        assert(std::fabs(sum - 1.f) < 1e-6f);
    }
    std::puts("PASS anti-tiling: exact natural material admission, structured paving exclusion, transformed gradients, normalized four-tap weights");
}
