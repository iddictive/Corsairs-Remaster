# Puerto Rico seabed and reactive-jet collision, 2026-11-13

Scope: source and staged-resource inspection only; no game, installation, or save was run or modified. No reported trajectory or height was supplied. The checked-out engine revision was `599e7b71a0d5bacca301a5bdfdf79defcb8f92ca`; this does not establish the installed engine's identity or loaded scene state.

The staged `RESOURCE/MODELS/Islands/PuertoRico/PuertoRico_seabed.gm` has SHA-256 `1b418ee3a94f784beaf2492297e651a2daac4f6434132859d575110e5f132944`, matching the Puerto source layout and seamless manifest. Its two objects have flags 3103, admitting both visible and collision-enabled geometry. Across all 1,900 render triangles, local Y ranges from -35.0058365 to +0.1658148 m. Nineteen triangles have a vertex above zero, none have all three above zero, and none have a vertex above +1 m. The highest is object 0, local face 54, vertices `(203.31366,-35.00573,1688.26489)`, `(255.09352,-35.00573,1675.20166)`, `(247.48183,0.165815,1660.23047)`; this is an authored face, not evidence of a displaced collider. Object 1's 82 faces have no positive vertices, providing an adjacent negative. The original primary island's seabed child matrix in the manifest is identity. This bounds this resource in its authored frame, not every possible transformed or damaged scene.

The installed app supplies the canonical game resource; at inspection its file matched the staged file and the hash in `src/assets/islands/puerto-rico/layout.json`. The app is available from the project's download. The runtime cache is disposable. Reproduce read-only from the repo root (Python 3; existing GM parser and vertex reader); the hash check rejects a different asset version:

```sh
PYTHONDONTWRITEBYTECODE=1 python3 - <<'PY'
import hashlib, pathlib, sys
sys.path.insert(0, 'tools')
from patch_metal_portobello_ground import parse_gm
from prepare_puerto_rico_city import vertices
p = pathlib.Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/MODELS/Islands/PuertoRico/PuertoRico_seabed.gm')
d = p.read_bytes()
assert hashlib.sha256(d).hexdigest() == '1b418ee3a94f784beaf2492297e651a2daac4f6434132859d575110e5f132944', 'Different seabed resource; do not compare results'
g = parse_gm(d)
print('sha256', hashlib.sha256(d).hexdigest())
for i, obj in enumerate(g['objects']):
    ts = [tuple(tuple(v[:3]) for v in t) for t in vertices(g, obj)]
    ys = [v[1] for t in ts for v in t]
    high = max(enumerate(ts), key=lambda x: max(v[1] for v in x[1]))
    print(i, 'flags', obj[2], 'faces', len(ts), 'y', min(ys), max(ys),
          'any-positive', sum(any(v[1] > 0 for v in t) for t in ts),
          'all-positive', sum(all(v[1] > 0 for v in t) for t in ts),
          'above-one', sum(any(v[1] > 1 for v in t) for t in ts), 'highest', high)
PY
```

Source path: `ISLAND::Mount` unlinks the seabed into a model with its parent's global matrix; `NODER::Update` composes the node local and model matrices, and rendering uses that same node global matrix. In the `STORM_METAL_SHIP_HULL_DAMAGE` path, `SHIP::PreparePhonkJetWorld` includes the seabed through `WorldModels`, preserves its current model matrix and calls `seabed_collision::clip`; uncut mesh capture reads original enabled object triangles, applies each node's `glob_mtx`, and emits world-space triangles. `TriangleSink` rejects degenerate faces; `ReactiveWorld::SetStaticTriangles` builds a Jolt mesh at zero translation and identity rotation, so no second placement is applied. World-cut nodes use a different native clip branch; capture failures fall back to native model clip. Region translation/rotation moves the model and saved authored matrix; the rigid region pose has no Y displacement. Water height for the reactive solver is separately sampled by `ShipRocking` and is not asserted constant at zero.

Result: no proven abnormal above-water collider or placement mismatch from these inputs. This is not a Jolt contact replay or live-scene observation: active region selection, node flags, damage/world cuts, transient matrix, actual water height, installed binary, ship hull, and the reported trajectory remain unverified. The legacy resource mapping references the original Puerto seabed separately; do not assume that every atlas or loaded runtime variant is represented by this staged source file.
