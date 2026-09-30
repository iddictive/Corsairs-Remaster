#include "dynamic_sail_native.hpp"

#include <cstdlib>
#include <iostream>

using storm::metal::sail::DrawContract;
using storm::metal::sail::eligible;

static void require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        std::exit(1);
    }
}

int main() {
    DrawContract sail{0x312u, 48u, 3u, true, true, true, true, true,
                      false, false, true, false, true, true, true, true};
    require(eligible(sail), "authored dynamic SAILVERTEX must use native Metal");
    require(storm::metal::sail::kNormalOffset == 12u &&
                storm::metal::sail::kUv0Offset == 24u &&
                storm::metal::sail::kUv1Offset == 32u &&
                storm::metal::sail::kUv2Offset == 40u &&
                storm::metal::sail::kUv2Offset + 8u == storm::metal::sail::kSailStride,
            "XYZ|NORMAL|TEX3 offsets must match SAILVERTEX");

    storm::metal::sail::IndexedBinding binding{};
    require(storm::metal::sail::indexedBinding(
                96u, 48u, 221, 7u, 153u, 96u + (221u + 160u) * 48u,
                54u, 2u, 288u, 2048u, binding),
            "nonzero sail base/minimum/start must produce a bounded native binding");
    require(binding.vertexBufferOffset == 96u + 221u * 48u,
            "D3D base vertex must be folded into the Metal vertex byte offset");
    require(binding.indexBufferOffset == 54u * 2u,
            "D3D start index must remain measured in indices");
    require(binding.metalBaseVertex == 0,
            "Metal must not apply the D3D sail base a second time");

    auto badBinding = binding;
    require(!storm::metal::sail::indexedBinding(
                96u, 48u, 221, 7u, 153u, 96u + (221u + 159u) * 48u,
                54u, 2u, 288u, 2048u, badBinding),
            "declared vertex window beyond the buffer must fail closed");
    require(!storm::metal::sail::indexedBinding(
                0u, 48u, 0, 0u, 16u, 16u * 48u,
                1000u, 2u, 30u, 2048u, badBinding),
            "index start/count beyond the buffer must fail closed");

    // The compatibility expander deliberately drops its optional compact span
    // when unique vertices >= draw references. Native indexed submission must
    // remain eligible because it consumes the original index bytes, not that
    // nullable optimization object (the real crash regression).
    require(eligible(sail), "unique vertices == draw count must stay native");

    auto nullIndices = sail;
    nullIndices.hasIndexData = false;
    require(!eligible(nullIndices), "null index bytes must fail closed");

    auto outOfRangeIndices = sail;
    outOfRangeIndices.indexRangeValid = false;
    require(!eligible(outOfRangeIndices), "out-of-range index bytes must fail closed");

    auto staticSail = sail;
    staticSail.dynamicSource = false;
    require(!eligible(staticSail), "contract must identify the traced dynamic owner");

    auto terrain = sail;
    terrain.fvf = 0x152u;
    terrain.stride = 36u;
    terrain.textureCoordinates = 1u;
    require(!eligible(terrain), "ordinary location geometry must not be reclassified");

    auto malformed = sail;
    malformed.stride = 40u;
    require(!eligible(malformed), "mismatched sail layout must fail closed");

    auto generated = sail;
    generated.generatedCoordinates = true;
    require(!eligible(generated), "generated coordinates require their existing path");

    auto locked = sail;
    locked.vertexLocked = true;
    require(!eligible(locked), "locked update buffers cannot be submitted");

    std::cout << "PASS: exact dynamic XYZ|NORMAL|TEX3 sail stream is native-Metal eligible\n";
}
