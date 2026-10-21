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
