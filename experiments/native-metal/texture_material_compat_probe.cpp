#include "texture_material_compat.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using storm_metal::compat::decodeToRgba;

struct TxHeader { int32_t flags, width, height, mips, format, mipSize; };
static void require(bool value, const std::string &message) { if (!value) throw std::runtime_error(message); }

static std::vector<uint8_t> decode(const fs::path &path, D3DFORMAT expected) {
    std::ifstream input(path, std::ios::binary); require(bool(input), "missing real asset: " + path.string());
    TxHeader header{}; input.read(reinterpret_cast<char *>(&header), sizeof(header));
    require(header.format == int32_t(expected), "unexpected format: " + path.string());
    std::vector<uint8_t> payload(header.mipSize); input.read(reinterpret_cast<char *>(payload.data()), payload.size());
    require(size_t(input.gcount()) == payload.size(), "truncated mip: " + path.string());
    const unsigned pitch = expected == D3DFMT_DXT1 ? std::max(1, (header.width + 3) / 4) * 8 :
                           (expected == D3DFMT_DXT3 || expected == D3DFMT_DXT5) ? std::max(1, (header.width + 3) / 4) * 16 :
                           expected == D3DFMT_A8R8G8B8 ? header.width * 4 : header.width * 2;
    std::vector<uint8_t> rgba;
    require(decodeToRgba(expected, header.width, header.height, payload.data(), payload.size(), pitch, rgba),
            "decode failed: " + path.string());
    require(rgba.size() == size_t(header.width) * header.height * 4, "wrong decoded extent");
    return rgba;
}

static std::pair<size_t, size_t> alphaRange(const std::vector<uint8_t> &rgba) {
    size_t transparent = 0, partial = 0;
    for (size_t i = 3; i < rgba.size(); i += 4) { transparent += rgba[i] == 0; partial += rgba[i] > 0 && rgba[i] < 255; }
    return {transparent, partial};
}

int main(int argc, char **argv) {
    require(argc == 2, "usage: texture-material-compat-probe RESOURCE/Textures");
    const fs::path root = argv[1];
    struct Asset { const char *name; D3DFORMAT format; bool needsTransparency; };
    const Asset assets[] = {
        {"leafPalms.tga.tx", D3DFMT_A1R5G5B5, true},
        {"sailLSCU1.tga.tx", D3DFMT_DXT3, true},
        {"back_plants.tga.tx", D3DFMT_DXT5, true},
        {"Back_G.tga.tx", D3DFMT_DXT1, true},
        {"deckPlanksU1.tga.tx", D3DFMT_DXT1, false},
        {"treePalms.tga.tx", D3DFMT_A8R8G8B8, true},
        {"Text_sel.tga.tx", D3DFMT_A4R4G4B4, true},
    };
    for (const auto &asset : assets) {
        auto rgba = decode(root / asset.name, asset.format); auto [transparent, partial] = alphaRange(rgba);
        if (asset.needsTransparency) require(transparent || partial, std::string("authored alpha lost: ") + asset.name);
        else require(!transparent && !partial, std::string("opaque DXT1 changed alpha class: ") + asset.name);
        std::cout << asset.name << " transparent=" << transparent << " partial=" << partial << '\n';
    }
    using namespace storm_metal::compat;
    require(alphaCompare(D3DCMP_GREATER, 161, 160) && !alphaCompare(D3DCMP_GREATER, 160, 160), "alpha-test edge");
    Float4 src{.2f,.4f,.6f,.25f},dst{.8f,.6f,.4f,.75f};auto mod=textureOp(D3DTOP_MODULATE,src,dst,{},{},{},{},{});
    require(mod.a>.187f&&mod.a<.188f,"stage alpha modulation");require(!stageEnabled(D3DTOP_DISABLE)&&stageEnabled(D3DTOP_MODULATE),"disabled stage termination contract");
    require(blendFactor(D3DBLEND_SRCALPHA,src,dst).r==.25f&&blendFactor(D3DBLEND_INVDESTALPHA,src,dst).r==.25f,"alpha blend factors");
    require(blendFactor(D3DBLEND_SRCCOLOR,src,dst).b==.6f&&blendFactor(D3DBLEND_INVDESTCOLOR,src,dst).r>.199f,"color blend factors");
    require(blendFactor(D3DBLEND_SRCALPHASAT,src,dst).r==.25f&&blendFactor(D3DBLEND_SRCALPHASAT,src,dst).a==1,"source-alpha saturation");
    require(supportsTextureOp(D3DTOP_BLENDTEXTUREALPHA)&&!supportsTextureOp(D3DTOP_BUMPENVMAP),"explicit supported stage domain");
    std::cout << "PASS: real DXT1/3/5 and A8/A4/A1 assets preserve authored alpha; alpha-test/blend/stage contract\n";
}
