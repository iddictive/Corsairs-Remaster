#!/usr/bin/env python3
"""Static contract for authored island vegetation visible from the sea."""
from pathlib import Path

root = Path(__file__).resolve().parent
patch = (root / "sea-island-vegetation.patch").read_text()
grass = (root / ".cache/storm/src/libs/location/src/grass.cpp").read_text()
sea = (root / ".cache/runtime/PROGRAM/sea_ai/sea.c").read_text()

def need(value, message):
    if not value:
        raise AssertionError(message)

need('CreateGrass("resource\\models\\islands\\"+' in sea,
     "sea consumer must load each island's authored .grs distribution")
need('Islands[iIslandIndex].jungle.patch' in sea and 'Islands[iIslandIndex].jungle.texture' in sea,
     "sea vegetation must retain authored distribution and atlas")
need('entid_t eidIsland = core.GetEntityId("ISLAND")' in patch,
     "extended range must be restricted to the sea-island consumer")
need('kSeaIslandVegetationDistance = 3000.0f' in patch,
     "sea island vegetation needs a bounded visible distance")
need('linearDistance > m_fMaxVisibleDist && kLod < 0.7501f' in patch,
     "distant sea vegetation must force the sparsest authored LOD")
need('& 3u) != 0u' in patch,
     "far-field authored blocks need a stable quarter-density budget")
need('StormMetalDrawGrass' in grass and 'metalInstances.push_back' in grass,
     "vegetation must remain Metal-native compact instancing")
need('jungle1.tga' not in patch,
     "fix must not paint vegetation into island terrain")
print("PASS sea island vegetation: authored .grs reaches compact Metal instances to 3000 units; distant population is fixed at authored LOD3")
