# Nuclear smoke face

The facial variant is disabled in the current shader. These retained sources
and frames document the earlier variant; the live source uses the ordinary
procedural mushroom.

The recorded nuclear mushroom uses an authored all-smoke sprite during its readable
middle interval, with the existing procedural ignition, plume, ground ring and
nine-second expiry. The frontal face shares the cap's billowing smoke material.
It is a vertical world-space card facing the viewer horizontally, with animated
noise displacement and scene-depth occlusion. Overhead views retain the volume.
The sprite fades in over 0.35–1.35 seconds and back into the plume over 5.5–8.5.

The canonical source is `experiments/native-metal/assets/nuclear-smoke.png`, one
512×512 RGBA PNG. Its generated header embeds the same PNG bytes in the engine;
installed and offline builds need no extra resource file or working-directory
path. Cocoa decodes it once to a premultiplied RGBA8 Metal texture. Regenerate:

```sh
python3 tools/prepare_nuclear_smoke.py experiments/native-metal/assets/nuclear-smoke.png experiments/native-metal/phonk_nuclear_smoke_data.hpp
```

`sprite-sources.json` pins the final native inputs and source asset. The
`sprite-age-*.png` frames pass the material/likeness review at native size: no
eyeballs, skin layer, isolated bust or pasted portrait. The source art and native
frames were reviewed separately. These are standalone renderer fixtures; they
do not establish battle composition, continuous motion or player acceptance.

The PNGs show native 1280×800 Metal output at 2.5, 4 and 8.5 seconds. Expiry at
nine seconds and complete foreground occlusion leave the exact background.
`sprite-baseline.txt` and `sprite-candidate.txt` are consecutive native runs of
the previous installed field and final sprite, with thirty warmed samples each:
2.40/1.49 ms median on Apple M3 Max. This bounded fixture does not prove battle
FPS. No permanent tests or runtime dependency were added.

The earlier geometry approach and importer are recoverable from commit
`5bc53877f21653263b72517ba55a1469990eccf5`. It used the freely downloadable
[ThreeDee Nova bust](https://app.threedee.design/community/generations/free-3d-bust-of-putin).
Its head field was rejected because it read as a dense head in front of the cap.
The original `sources.json`, `age-*.png`, `baseline.png` and logs retain that
comparison's provenance; they describe the retired candidate, not the current
renderer. Its unused data and importer were removed from the active source.

The frozen `preview.mm.txt` is a disposable reconstruction input, not a CI test.
From the repository root, copy it into task scratch and compile with:

```sh
xcrun clang++ -std=c++17 -O2 -fobjc-arc -I experiments/native-metal \
  -framework Foundation -framework Metal -framework AppKit preview.mm -o preview
./preview output-directory
```

The optional `NUCLEAR_PREVIEW_DISTANCE` environment variable changes camera
distance; the retained comparison uses its default 5.5 blast radii.
