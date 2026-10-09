#pragma once
#include <cstdint>
#include <vector>

namespace storm_metal {
// Source noise fields are embedded in the engine, so old installations gain
// volumetric skies through the ordinary engine update without new runtime files.
bool loadCloudNoise(std::vector<uint8_t>&shape,std::vector<uint8_t>&detail,std::vector<uint8_t>&weather);
}
