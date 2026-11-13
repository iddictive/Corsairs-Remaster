# Ground Explosion volume sequence

These 19 density/flame pairs are sampled from JangaFX's CC0 Ground Explosion
simulation. `SOURCE_LICENSE.txt` and `SOURCE_README.txt` are from its archive;
`manifest.json` contains the source, selected frames and SHA-256 hashes. The
original selected VDBs are retained in `source/`; `frames/` contains converted
64³ RG8 voxel grids, 524,288 bytes each. Frames 190 and 199 are empty in the
source. Earlier frames are more closely spaced to preserve the initial burst.

Conversion: build `tools/convert_cluster_vdb.cpp` against OpenVDB and invoke it
once per VDB with the desired output path. The converter samples the shared
original index domain (287 × 287 × 319), with x fastest in output, y vertical,
and z horizontal. Output R is density / 2, G is flames / 8; both are UNORM8.
Decode by multiplying by those divisors. The converter refuses values above the
chosen ranges. Indexing one voxel is `2 * ((z * 64 + y) * 64 + x)`.

The time keys use `source_frame / 30` seconds. The source metadata reports a
simulation time of 0.366667 s for frame 10 and 1.03333 s for frame 30; the
keys start at the first exported frame rather than at simulation initialization.
Between neighboring keys, sample their 3D textures at the same normalized
position and linearly interpolate by elapsed time. Use empty density and flame
outside the sequence. The experimental Metal integration selects two 3D
textures and interpolates them per sample. Its scale, lighting, opacity and
delivery still require validation in the consuming game; bounded offscreen
tests do not establish acceptance or real-game frame cost.

64³ and 19 keys are a bounded prototype: they lose fine volume detail and may
show temporal interpolation artifacts. A low-cost CPU projection confirms that
the data changes in three dimensions, not that the finished result is realistic.

`GroundExplosion64.rg8` is the delivery package: eight bytes `CIVDB01\0`, a
little-endian uint32 frame count (19), 19 little-endian uint32 source frame
numbers, and then each corresponding RG8 grid in manifest order. Rebuild it
without modifying source VDBs using:

```sh
python3 - <<'PY'
import json, pathlib, struct
r = pathlib.Path('src/assets/cluster-vfx/ground-vdb')
m = json.loads((r / 'manifest.json').read_text())
with (r / 'GroundExplosion64.rg8').open('wb') as f:
    f.write(b'CIVDB01\0')
    f.write(struct.pack('<I', len(m['frames'])))
    for row in m['frames']:
        f.write(struct.pack('<I', row['source_frame']))
    for row in m['frames']:
        f.write((r / row['texture']).read_bytes())
PY
```

After rebuilding, update and verify both checksums in the parent delivery
manifest. The delivered path is `RESOURCE/Textures/PhonkVFX/GroundExplosion64.rg8`.
