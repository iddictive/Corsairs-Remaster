# JangaFX Mid-Air Explosion volume

The 19 selected VDBs in `source/` are from JangaFX's
[Mid-Air Explosion](https://jangafx.com/software/embergen/download/free-vdb-animations/)
sequence, distributed by JangaFX under CC0-1.0. The
[original archive](https://www.mediafire.com/file/uwmxisgj9xagrdh/Midair_Explosion_01.rar/file)
contains 126 frames. Source frame numbers, 60 fps source times and hashes are
recorded in `manifest.json`. The sampled frames total 173,838,095 bytes.

`MidairExplosion64.rg8` is an experimental, not yet integrated or visually
accepted, 19-frame package in the existing CIVDB01 format. Each 64³ RG8 frame
encodes density / 2 and flames / 2; texture axes x/y/z sample source x/z/y in
the shared index domain 193×385×385. The source is directional, not spherical.
The package keys map source frames 0–125 to integer keys 0–199; the source
animation length is 125/60 seconds. Its renderer needs its own key-to-time and
channel decoding; it must not be substituted for the ground volume used by
other warheads.

To regenerate with the OpenVDB-linked `tools/convert_cluster_vdb.cpp`
executable, set `CONVERTER` to its absolute path and run from the repository
root:

```sh
SCRATCH="$(mktemp -d)"
export SCRATCH
python3 - <<'PY'
import hashlib, json, os, pathlib, struct, subprocess
root = pathlib.Path('src/assets/cluster-vfx/midair-vdb')
manifest = json.loads((root / 'manifest.json').read_text())
rows = manifest['frames']
scratch = pathlib.Path(os.environ['SCRATCH'])
blocks = []
for row in rows:
    target = scratch / (str(row['source_frame']) + '.rg8')
    subprocess.run([os.environ['CONVERTER'], str(root / row['source']),
                    str(target), 'midair'], check=True)
    blocks.append(target.read_bytes())
package = scratch / manifest['packed_file']
with package.open('wb') as out:
    out.write(b'CIVDB01\0')
    out.write(struct.pack('<I', len(rows)))
    out.write(struct.pack('<' + 'I' * len(rows),
                          *(row['packed_key'] for row in rows)))
    for block in blocks:
        out.write(block)
assert hashlib.sha256(package.read_bytes()).hexdigest() == manifest['packed_sha256']
PY
```

This package is not staged into the game. The synthetic render experiments
have not established the requested blast quality, shock wave, full motion,
or real-game performance.
