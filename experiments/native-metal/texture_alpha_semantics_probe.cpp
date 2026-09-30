#include "texture_material_compat.hpp"

#include <array>
#include <cstdlib>
#include <iostream>
#include <vector>

using storm_metal::compat::decodeToRgba;

static void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

static std::vector<uint8_t> decode(const std::array<uint8_t, 8> &block) {
    std::vector<uint8_t> rgba;
    require(decodeToRgba(D3DFMT_DXT1, 4, 4, block.data(), block.size(), 8, rgba),
            "DXT1 block decodes");
    return rgba;
}

int main() {
    // c0 > c1 selects DXT1's four-color (opaque) mode. Index 3 remains opaque.
    const auto ground = decode({0xff, 0xff, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff});
    for (size_t i = 3; i < ground.size(); i += 4)
        require(ground[i] == 255, "opaque ground block remains opaque per pixel");

    // c0 <= c1 selects DXT1's three-color-plus-transparent mode. Only authored
    // index 3 texels are cut out; neighboring palette indices remain opaque.
    const auto foliage = decode({0x00, 0x00, 0xff, 0xff, 0x03, 0x00, 0x00, 0x00});
    require(foliage[3] == 0, "authored foliage cutout remains transparent");
    for (size_t i = 7; i < foliage.size(); i += 4)
        require(foliage[i] == 255, "foliage coverage is not inferred texture-wide");

    std::cout << "PASS: DXT1 ground stays opaque and foliage preserves authored per-pixel cutout\n";
}
