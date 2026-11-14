# Aerial Explosion: bounded source evaluation

JangaFX distributes [Aerial Explosion](https://jangafx.com/software/embergen/download/free-vdb-animations/)
under CC0. The [ZIP archive](https://www.mediafire.com/file/toxcf3jmeuu4ub1/AerialExplosionVDB.zip/file)
was queried with HTTP byte ranges; the 6.6 GB archive was not downloaded.
`source-reference.mp4` is the provider's included animation. Only source VDB
frames 0, 10, 25, 40 and 50 were extracted; `source-bounds.txt` records their
metadata. Their original index domain is 511×511×703. Density touches source
z=0 at frame 40 and remains there at 50; frame 40 also reaches y=510. These
observations show a source-domain boundary, not by themselves how much visible
clipping it causes.

The source hashes (SHA-256), for recovering these exact inputs, are:

```text
0000 058a002b188162e175365ee5add234316536b88d1fe3bb5148b3c8044d09f9ca
0010 afaa66b183e231f54b891e61b2840cdfa10e7e3164d8c1ac799372106e908ae1
0025 142b8f8cf2ff08528b90c56796f2ab1afb3c9e7a825f9e0123728468df2d48fc
0040 d422b9a2f0d562489af78b3b57a2f24e2341622fd2893922665b17cd2802cb0b
0050 f9643e093afb68c2edf0b2592079f130824c77d634929eceb689c8101798f197
```

`four-frame-probe.rg8` is a disposable-quality 64³ RG8 probe, not a runtime
asset: density is divided by 2 and flames by 16; frames 0, 10, 25 and 50 are
held across 19 slots to exercise the existing loader. It does not represent
continuous source motion, and omits most of the 120-frame sequence.
`front.png`, `side.png` and `above.png` show the same early candidate
from three synthetic camera directions at increased distance. The prototype
is not accepted: source-relative position and brightness differ from the
provider's reference, and complete propagation around the detonation point
and the late boundary have not been established. Neither random seed selection
nor the land/water distinction was modified.

At 2056×1290, camera (0,60,180), input radius 42 and eight overlapping
airborne impacts, the source renderer's GPU medians at 0.10/0.25/0.50 s were
4.160/3.171/2.942 ms; this four-frame probe gave 3.248/2.944/2.736 ms.
Each median is calculated from the three values in the CSV after a warmup;
the fixture prints the minimum rather than the median. Both runs used a
synthetic sky/sea, with no ships or game process. The different sequences,
four held keyframes, and unaccepted image mean this is a cost observation,
not an equivalent-quality performance result.

Reproduction: `extract-range.py.txt` accepts an output directory followed by
the exact archive member names `AerialExplosion/AerialExplosionVDB/AerialExplosion_NNNN.vdb`.
`converter-probe.cpp.txt` is compiled against OpenVDB 14 and receives
`input.vdb output.rg8 midair`. To reproduce the probe package, write
`CIVDB01\\0`, uint32 count 19, uint32 keys [0,10,25,50,51..64,199], then
the 64³ RG8 frame belonging to the greatest extracted source key at or below
each key. Compile a copied fixture and matching copied header with Apple
clang 21, C++20, `-O2 -fobjc-arc -framework Metal -framework AppKit
-I experiments/native-metal`, then run with an output directory and `single 6`.
Relative asset paths assume the repository root. No game build, install or
process access was performed.
