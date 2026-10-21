# Nuclear smoke face

The nuclear mushroom uses a geometry-only head field imported from the freely
downloadable [ThreeDee Nova bust](https://app.threedee.design/community/generations/free-3d-bust-of-putin).
The retained head intervals in `experiments/native-metal/phonk_nuclear_head_data.hpp`
are the complete geometry input for rebuilding the effect. No model mesh, skin,
UV, photograph or material is drawn. A 64³ scalar texture shapes the existing
64-step smoke volume; its noise, extinction, light attenuation, scene-depth
clipping and nine-second expiry remain in the same renderer. The face coalesces
after ignition and disperses into the plume. Its horizontal orientation follows
the viewer so it can be read around the blast.

`sources.json` pins the exact native inputs, model identity and baseline revision.
The importer discards the bust's shoulders and all appearance data. To reimport
the exact source GLB:

```sh
python3 tools/prepare_nuclear_head.py model.glb experiments/native-metal/phonk_nuclear_head_data.hpp
```

The original image-derived relief was rejected for weak likeness and a mask-like
surface. The retained geometry candidate improves the proportions and uses only
smoke, but the dense central face can still read as a head in front of the cap.
That stylistic limitation is unresolved; these fixtures do not establish player
acceptance of the likeness or its appearance against a real battle scene.

The PNGs show native 1280×800 Metal output at 2.5, 4 and 8.5 seconds. Expiry at
nine seconds and complete foreground occlusion leave the exact background.
Baseline and candidate logs include thirty warmed GPU samples. Host contention
varied substantially between runs; the retained 4.40/2.82 ms medians do not prove
a speed improvement or unchanged battle FPS. No permanent tests were added.

The frozen `preview.mm.txt` is a disposable reconstruction input, not a CI test.
From the repository root, copy it into task scratch and compile with:

```sh
xcrun clang++ -std=c++17 -O2 -fobjc-arc -I experiments/native-metal \
  -framework Foundation -framework Metal -framework AppKit preview.mm -o preview
./preview output-directory
```

The optional `NUCLEAR_PREVIEW_DISTANCE` environment variable changes camera
distance; the retained comparison uses its default 5.5 blast radii.
