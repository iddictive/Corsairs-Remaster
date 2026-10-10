# Lighting of surfaces without vertex normals

## Contract and owners

`backend.mm` owns light application for fixed-function material draws. A world
material with `Lighting = true` and no FVF normal receives incident light after
its texture combiners and before fog. This makes texture-only and texture-factor
recipes obey the same lighting contract as diffuse-color recipes. RGB is lit;
texture coverage, alpha, topology and vertex stride stay unchanged.

Outdoors, the incident term is the existing completed sky packet's upward
irradiance plus enabled directional light energy filtered by the same solar
spectrum/transmission packet used by normal-bearing models and water. The old
`D3DRS_AMBIENT` does not contribute there. Interiors, disabled sky and an unavailable
packet retain authored ambient as their fallback. `Lighting = false` remains the
explicit contract for emitters and authored unlit effects; transformed screen
geometry is excluded.

Night navigation uses a 15% diffuse exposure increase in the common GPU sky
projection packet (`sky_surface_solar`), blended out by daylight/twilight. This
multiplies existing sky energy rather than adding a minimum or restoring legacy
ambient. Normal-bearing models, vegetation and normal-less surfaces share it;
visible sky/fog radiance and emissive effects are not brightened by this control.

Missing normals permit a common upward-facing response, not directional surface
shading or per-fragment local-light shadows. This repair does not claim those
capabilities, a global illumination solution, or acceptance of the sea/horizon.

`normal-less-surface-lighting.patch` binds the existing material state for ropes,
vants, flags, butterflies, flies, blood, blots and sinking foam. Ambient-to-texture-
factor copies in the corresponding producers are removed, leaving white RGB and
their existing opacity. The ordered stack in `build.sh` owns source delivery;
`tools/delivery_state.py` owns the newly admitted technique resources in both the
cache runtime and installed application. Their original installed bytes were
compared with source before initial admission (only line endings differed).

## Rejected repairs and decisive evidence

Rope, vant and flag vertices contain position plus UV, with neither normal nor
diffuse color. Their recipes use texture × `TextureFactor`; Butterfly selects
texture RGB directly. Changing a compatibility vertex's color cannot repair any
of these outputs. Earlier outdoor-unlit vertex modulation also added solar
transmission as if it were energy: transmission can be one at night. That branch
and its sky-dependent conversion-cache invalidation are removed.

An isolated Metal draw with a white texture, authored ambient 0.5 and a sky packet
of RGB 0.02/0.03/0.04 reproduced the bypass: texture-factor output was 128/128/128.
The repaired material outputs 5/8/10 for both texture-factor and texture-only
recipes. A day packet produces 102/128/153; switching back produces 5/8/10 without
stale cached colors. Authored fallback returns 128/128/128, an unlit emitter stays
255/255/255, and texture alpha 128 stays 128. All eight draws use compact GPU
decoding with zero compatibility CPU vertex conversions. This is GPU component
evidence, not a player-scene replay.

The night exposure packet was also dispatched on Metal against its previous
projection: at 19:36 and midnight all four diffuse SH coefficients increase by
exactly 1.15; noon and 06:09 stay at 1.0 and the solar packet remains identical.
Existing D3D9 material/specular/spot and menu compact-decoding probes pass.
The older `lighting-probe` still rejects its unrelated modern night-floor
expectation (expects 48, gets 25); compiling the pre-repair HEAD reproduces the
same values and failure. Its expectation is not changed to disguise that gap.

## Rope cross-section geometry

The original rope frame used `z / length` for its vertical cosine, with the wrong
sign. Along the X axis a complete ring collapsed to a line; diagonal rings were
not perpendicular to their endpoints' tangent. The corrected coefficient is
`-sqrt(x*x + z*z) / length`. Horizontal X/Z, diagonal, downward and near-vertical
directions preserve unit radius and tangent orthogonality to within 1e-6. The
existing near-vertical branch remains intact. Vant vertex/index layout is
unchanged; no screenshot alone proves that its authored locator arrangement is
corrupt. Player visual acceptance of the assembled rigging remains pending.
