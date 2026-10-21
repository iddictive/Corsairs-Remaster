#pragma once
#include <array>
#include <cstdint>
#include <vector>

// Shared integer planar adapter; Clipper stays private to geometry.
namespace storm_world_cut {
using PlanarPoint=std::array<double,2>;
using PlanarRing=std::vector<PlanarPoint>;
using PlanarPolygon=std::vector<PlanarRing>;
bool unitePlanar(const std::vector<PlanarRing>&,std::vector<PlanarPolygon>&);
bool differencePlanar(const std::vector<PlanarRing>&,const std::vector<PlanarRing>&,std::vector<PlanarPolygon>&);
bool integerTriangulation(const PlanarPolygon&,std::vector<uint32_t>&);
}
