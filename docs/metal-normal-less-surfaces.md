# Lighting of surfaces without vertex normals

## Contract and owners

`backend.mm` owns light application for fixed-function material draws. A world
material with `Lighting = true` and no FVF normal receives incident light after
its texture combiners and before fog. This makes texture-only and texture-factor
recipes obey the same lighting contract as diffuse-color recipes. RGB is lit;
texture coverage, alpha, topology and vertex stride stay unchanged.

Ropes, vants and flags now provide geometry normals and leave this fallback.
Their native streams are position/normal/UV (32 bytes); the existing raw FFP GPU
decoder owns directional and sky lighting. RGB is texture × lit diffuse × the
authored texture factor, while the original alpha recipes remain intact.

## Thin rigging coverage

The October15 enlarged player reference exposes dotted ropes and stair steps.
The matching native fleet capture had antialiasing disabled. Enabling the
existing FXAA softens edges but leaves missing rope samples. Geometry inspection
finds continuous five-sided tubes, continuous longitudinal UVs and culling off;
`sail_rope.tga.tx` is fully opaque at all five authored mip levels. The Vant
ratline quad is separately alpha textured and must not be made opaque. Missing
subpixel tube coverage is therefore a demonstrated problem beyond texture alpha.

`scene_msaa.hpp` owns private main-scene multisample color/depth attachments.
The existing antialiasing setting enables MSAA4 plus the existing FXAA; Off
allocates neither. Public scene surfaces stay single-sample: encoder ends resolve
color and minimum depth before water snapshots, shadows, postprocessing,
MetalFX, readback or presentation. Stored samples survive ordinary encoder
restarts; external color writers invalidate only sampled color so per-sample
depth remains intact. Native and each MetalFX flight keep separate attachments,
so rotating scaler slots do not allocate textures every frame. Target/size changes and graphics resets recreate resources.
The existing aim identity MRT remains single-sample and invalidates both sampled
attachments before subsequent scene draws; this preserves aim compatibility,
but that capture scope does not gain geometric MSAA. Reflections, shadow maps
and HUD retain their existing sample counts.

The native136-frame fleet comparison at2560x1655 visibly improves thin ropes
with4 samples;2 samples still leave conspicuous dotted spans. GPU medians after
frame48 were13.681ms for FXAA,17.289ms for MSAA4+FXAA,14.908ms for MSAA2 alone
and15.907ms for MSAA4 alone. These short serial cases establish the quality/cost
tradeoff, not sustained FPS acceptance. Sources, full frames, crops, logs and
private link receipts are retained in ignored
`.local-archives/metal-renderer-evidence/rigging-alias-fit`.

The Options checkbox label now names MSAA4+FXAA. Its nine single-control Metal
siblings drop `individualpos=1`: native checkbox input otherwise accepts label
hover while only the15px icon can toggle. Persistence and flag64 stay unchanged;
no additional settings record or user configuration rewrite is introduced.

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

The former rope, vant and flag vertices contained position plus UV, with neither
normal nor diffuse color. Their recipes used texture × `TextureFactor`; Butterfly selects
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

## Directional rigging lighting

The noon deck report reopened the upward-light fallback: a vertical or downward
rigging face received the same direct solar energy as an upward face, while the
normal-bearing mast and crew responded to their surface direction. Darkening
texture RGB would retain that mismatch and alter authored reflectance.

`normal-less-surface-lighting.patch` adds normals at the existing geometry writers.
Rope normals use the current bent center tangent and circumferential tangent,
oriented outward. Vant plane/beam/strand normals refresh at initialization and
locator movement, including the duplicated strand seam. Flag normals follow both
deformed cloth edges and the triangular tip; alternating index winding must not
cancel adjacent normals. Positions, UVs, indices, culling, depth writes, alpha
tests, fades and the unlit `GhostShip` recipe are preserved. This does not add
cloth transmission or change two-sided topology.

`rigging_lighting.hpp` owns one scoped white diffuse/ambient material with zero
emissive/specular energy and restores its predecessor. It also marks a producer
draw independently of aiming. `backend.mm` excludes that role only from shadow
registry/caster admission, preserving the former no-caster participation instead
of enrolling newly normal-bearing rigging in additional cube/traversal work.
The ordinary hull predicates, raw lighting decoder and aim MRT remain intact.
Town and ship flags share the same material scope. `ship/Flag.fx` is admitted by
`tools/delivery_state.py` against its exact installed predecessor; normalized
installed text matched the original technique before admission.

CPU checks of the actual normal loops cover tilted/bent strands, moved locators,
twisted cloth/tips, seam agreement and finite zero-area output.
Degenerate normals use a finite unit fallback before the GPU normalizes them;
cloth tip fallback retains its adjacent normal. All new RGB chains terminate
stage2, so an inherited material stage cannot change flag coverage or color.
Scoped material checks cover inherited emissive material, nesting and exception cleanup. The
folded patch applies, reverses and reapplies byte-exactly over its predecessor;
alpha/depth/culling and unlit-emitter recipes compare unchanged. These are source
and CPU component checks. GPU directional readback and the matched installed
deck/flag replay remain unresolved; compiler evidence is recorded below.

Patch generation must use its exact ordered insertion point, reconstructed from
the recorded stack. Reversing only this patch in a fully composed source leaves
later fire headers in the baseline and produced an invalid rope include context.
The canonical build rejected that candidate before compilation and left the
source cache unchanged. The corrected patch preserves the later header insertion
and passes all 92 current ordered patches forward/reverse/forward in a task-local
copy using the canonical patch applicator.

The subsequent real compile exposed another boundary: `CVECTOR` in `c_vector.h`
and `Vector` in `math3d/vector.h` are different types. The initial CPU fixture's
alias hid it; the normal helper now accepts/returns the actual `CVECTOR`. The
geometry fixture uses that real header, and all three actual Rope/Flag/Vant
translation units pass syntax checks with their current Ninja compiler flags.
Those checks remove only object/dependency outputs and do not mutate the shared
build cache. The canonical incremental retry then compiled/linked the complete
engine successfully with two compiler workers (64 steps, 24.35s), consuming the
corrected ordered patch and typed helper unchanged. GPU directional readback and
the matched installed deck/flag replay remain unresolved; no app activation is
performed by this source milestone.
