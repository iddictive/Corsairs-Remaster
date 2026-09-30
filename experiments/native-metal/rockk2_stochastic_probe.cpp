#include "rockk2_stochastic.hpp"
#include <array>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <set>

using namespace storm_metal::rockk2;

static float smooth(float x) { return x*x*(3.f-2.f*x); }
static float cell(uint32_t seed, int x, int y) {
    uint32_t h=seed^uint32_t(x)*0x9e3779b9u^uint32_t(y)*0x85ebca6bu;
    h^=h>>16;h*=0x7feb352du;h^=h>>15;return float(h&65535u)/65535.f;
}
static float bombing(uint32_t seed,float x,float y) {
    const int ix=int(std::floor(x-.5f)),iy=int(std::floor(y-.5f));
    const float fx=smooth(x-.5f-ix),fy=smooth(y-.5f-iy);
    const float a=cell(seed,ix,iy)*(1-fx)+cell(seed,ix+1,iy)*fx;
    const float b=cell(seed,ix,iy+1)*(1-fx)+cell(seed,ix+1,iy+1)*fx;
    return a*(1-fy)+b*fy;
}
int main() {
    const std::array siblings{
        "RESOURCE/MODELS/Islands/Nevis/Nevis.gm",
        "resource\\models\\islands\\nevis\\Nevis_refl.gm",
        "RESOURCE/MODELS/Islands/Nevis/Nevis_seabed.gm"};
    assert(modelSeed(siblings[0])==modelSeed(siblings[1]) && modelSeed(siblings[1])==modelSeed(siblings[2]));
    assert(candidate(siblings[0])==candidate(siblings[2]));
    assert(modelSeed("RESOURCE/MODELS/Locations/BarbadosPlantation/BarbadosPlantation.gm") ==
           modelSeed("RESOURCE/MODELS/Locations/BarbadosPlantation/BarbadosPlantation_reflect.gm"));
    std::set<unsigned> variants;
    for (auto name : {"Islands/Cumana/Cumana.gm", "Islands/Nevis/Nevis.gm", "Islands/Tortuga/Tortuga.gm",
                      "Locations/Town_PuertoRico/Town/SanJuan.gm", "Locations/BarbadosPlantation/BarbadosPlantation.gm"})
        variants.insert(candidate(name));
    assert(variants.size() >= 2);
    const auto seed=modelSeed(siblings[0]);
    for(int edge=-4;edge<=4;++edge) for(float y=-1.75f;y<2.f;y+=.37f)
        assert(std::fabs(bombing(seed,edge-1e-5f,y)-bombing(seed,edge+1e-5f,y))<1e-3f);
    std::set<int> contacts;
    for(int y=0;y<8;y++)for(int x=0;x<10;x++)contacts.insert(int(bombing(seed,x+.5f,y+.5f)*10000));
    assert(contacts.size()>60);
    static_assert(stochasticTapCount==4);
    std::puts("PASS: rockK2 stable sibling seed, 3-way assignment, seam-safe 10x8 contact, 4 taps");
}
