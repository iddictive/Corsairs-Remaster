# Island flicker investigation

The player reports polygon flicker on islands even without explosions. The cause
is unresolved; this replay does not establish that the game is fixed or that
its full scene is stable.

`replay.py` uses the real `CreateGeometry` and `GEOM::Draw` with a capture
service, then submits the captured Nevis terrain triangles, normals, vertex
colours, UVs and material groups to the actual Metal fixed-function renderer.
The four diffuse TX textures use all eight authored mip levels, linear filtering
and wrap; Nevis's actual rockK2 alias is rockA2 and enables the existing
stochastic sampling. The 256x256 fixture uses near 0.115, far 32000, camera
position (0, 200, -2600) and an independently drawn nearer red occluder.
Repeated fixed-camera output is identical; reversing triangle order changes
three pixels, and translating the camera by 0.01 changes 158. These observations
cannot reproduce or rule out the reported flicker.

The capture service substitutes for the normal geometry service and only the
single-texture stage is reproduced. The replay omits NODER/ISLAND traversal,
seabed, weather, dynamic lighting and shadows, water, sky, postprocessing,
scene MSAA, real player camera and installed game. Its elapsed time includes
CPU copies, readback and synchronisation and is not a performance baseline.
A single isolated view cannot generalise to all islands or camera distances.

Run from the repository root with an empty scratch directory, the configured
native build/runtime, and Python with Pillow:

```
python3 docs/evidence/island-flicker-20261105/replay.py <empty-scratch-directory>
```

The exact output is in [result.txt](result.txt). At source revision
`2529d304972e12adda20e8fff1e59030f9e7a9ee` the renderer archive SHA-256 was
`9b6629c8dce5a536100580fa91d4a8ef0541fe3eef5a476a66290b09766f3baf`.
Runtime asset hashes, relative to `experiments/native-metal/.cache/runtime/`:

```
90456f323bead05e8675eb206637c15ddf8a40ea1c4e33dfa28c9b73e87c1b95  RESOURCE/MODELS/Islands/Nevis/Nevis.gm
c2f815a7d39561fd7fec9cc5d778332dc6d82b40f4c1d3ec2fa91cc97b34d4bd  RESOURCE/Textures/jungle_gray.tga.tx
3363912678fa6fd37c4ebc8da654218c8cdb4de91a40e73c0cb22730113437cd  RESOURCE/Textures/rockA2.tga.tx
b34bcdda13bc6a8eeb514f3f33ff24998799ce7ef2e1cc1bd6a3240ca6092d8c  RESOURCE/Textures/rockK4.tga.tx
1f37a991166d28646cbb7a7ed1b2f59999d2532c940627554ccada665f5e10a9  RESOURCE/Textures/beach1.tga.tx
```

Next evidence: capture consecutive full-scene frames during observed flicker,
with camera/view/projection, resident regions, island and seabed transforms,
immersion and distance blend, technique and material/texture draw identities,
depth state, and weather/lighting/shadow state. Compare terrain before
postprocessing with the final frame and isolate the changing input. The user
owns gameplay operation; a source or offscreen replay is not live acceptance.
