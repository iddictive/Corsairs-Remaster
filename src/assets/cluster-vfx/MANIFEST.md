# Cluster VFX flipbooks

Unity Labs Paris authored these sequences with Houdini and released them under
[CC0 1.0](https://creativecommons.org/publicdomain/zero/1.0/); see Unity's
[original announcement and downloads](https://unity.com/blog/engine-platform/free-vfx-image-sequences-flipbooks).
The original, unmodified archives are in `sources/`; each contains an HDR linear
EXR atlas and an uncompressed TGA atlas. They are not Houdini project files.
The PNGs are lossless 8-bit RGBA conversions of those TGA atlases using macOS
`sips -s format png`. PNG alpha was compared to the original TGA alpha at every
pixel and matched exactly. All atlases were visually inspected.

| PNG | Pixels | Columns × rows | Frames | SHA-256 |
| --- | --- | --- | --- | --- |
| `Explosion00_5x5.png` | 1024 × 1024 | 5 × 5 | 25 | `0ca81733d22e56114e00cc003ce63bf768fe94e52e8e6a2463f2a7bac3c00a9d` |
| `Explosion01_5x5.png` | 1024 × 1024 | 5 × 5 | 25 | `a75a55de571a47fd05e3add72761cf6d51fe3ed8fe8f4a16d5b35679da5357f5` |
| `Explosion02_5x5.png` | 1024 × 1024 | 5 × 5 | 25 | `7bb4f58512a0f96fce87eb48a5f44ef50391c44af2a390cdcdafd052bfb53ecf` |
| `DiscSmoke01_16x4.png` | 2048 × 1024 | 16 × 4 | 64 | `7775673937a2a3354904f52544ddda45b013df6062e57ad9c8759298bcab406c` |
| `WispySmoke02_8x8.png` | 1024 × 1024 | 8 × 8 | 64 | `8833f2a05ffb8bb2214fae15c1e5c4fc9e04b2b5b7c7bfdc7589b72b1a0418e9` |

Source archives and SHA-256:

| Archive | Unity download | SHA-256 |
| --- | --- | --- |
| `sources/Explosion00-flipbooks.zip` | [Explosion00](https://unity3d.com/files/labs/downloads/vfx/assets01/Explosion00/Explosion00-flipbooks.zip) | `5931820e3e8f5062446b059bf4b0489a8fc56df5de35170721fe9cd4139d6d42` |
| `sources/Explosion01-flipbooks.zip` | [Explosion01](https://unity3d.com/files/labs/downloads/vfx/assets01/Explosion01/Explosion01-flipbooks.zip) | `496a212733eb4e93c230f654f1fc1460e8a6389d2f1b13f2b945ec124a375742` |
| `sources/Explosion02-flipbooks.zip` | [Explosion02](https://unity3d.com/files/labs/downloads/vfx/assets01/Explosion02/Explosion02-flipbooks.zip) | `1426be2dcfc9008b8b8f0c7d54a241da3f611725aa3c84fc33f919f56036f538` |
| `sources/DiscSmoke01-flipbooks.zip` | [DiscSmoke01](https://unity3d.com/files/labs/downloads/vfx/assets01/DiscSmoke01/DiscSmoke01-flipbooks.zip) | `e749a28e436c592f35b40a1da47a761602b3323c7d34f72965e6b2b63c7a6723` |
| `sources/WispySmoke02-flipbooks.zip` | [WispySmoke02](https://unity3d.com/files/labs/downloads/vfx/assets01/WispySmoke02/WispySmoke02-flipbooks.zip) | `86607c817f83c4a4942007521f4c251aab144fc0ce51ea4f1002d08f3d0e1c16` |

## Integration notes

Read frames left to right, then top to bottom in image coordinates. For frame
`i`, use `column = i % columns`, `row = i / columns`, and normalized cell UVs;
account for the loader's vertical coordinate convention. The 5×5 atlases have
fractional 204.8-pixel cell steps: do not round the step to 204 or 205. Smoke
cells reach their borders, so bilinear filtering and mipmaps can bleed between
frames; clamp sampling to the current cell or repack with gutters as needed.
DiscSmoke cells are 128 × 256, not square. These are 2D billboards, not volumetric
geometry. EXR is the retained HDR option; PNG has 8-bit channels and cannot
retain EXR's HDR values. Confirm color-space and alpha blending with the actual
Metal loader and render pipeline; those contracts have not been tested here.

The source does not state a frame rate or authored duration. As an initial
playback choice only, 25 fps yields 1.0 s for the 25-frame explosions and
2.56 s for the 64-frame smoke sequences; tune in the consuming scene.
