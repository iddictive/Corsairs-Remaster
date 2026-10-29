# Far-water depth contact

The Metal water snapshot uses a depth32float attachment cleared to exactly1.
Treating depth>=.999999 as sky also classified real far geometry as empty.
With runtime near/far .115/32000, a coplanar white receiver at z28000 was
replaced by black water; a nearby change in rounded depth could reject the water
instead. The candidate identifies only the actual clear value as sky. The
MSAA foreground-silhouette fallback remains unchanged.

The archived replay derives its device, sea shaders, geometry and readback from
`experiments/native-metal/depth_water_probe.cpp` at base `e106aac`. It uses
real Metal,128x128, AA off and MSAA4, the runtime projection, a white opaque
receiver and the same-plane Sea2 triangle at z1200/24000/28000/31000. It uses no
game loop, player state or game UI. `observations.json` retains raw pixel values.

For reproduction with a configured native build, build the existing
`depth_water-probe` target so libstorm_metal.a contains the selected shader,
then run the frozen Python source with the repository path as its first
argument and CODEX_SCRATCH_DIR set to an existing disposable directory. The
source reuses compiler/link arguments from the existing target. This is an
archived one-off observation, without a new maintained test target.

The z28000 receiver changes from00000000 to00fdfdfd in both AA modes. Other
sampled distances retain their previous results. The existing native depth-water
fixture passes, including real ten-unit attenuation at z1200, foreground/alpha
rejection, frame invalidation and lighting. The previous subpixel MSAA sail/sky
order oracle also remains exact across four phases in both AA modes.

Cost: one comparison constant changes; no added shader operations, passes,
textures, allocations or CPU work. No FPS improvement is claimed. This proves a
far-depth component defect and repair; binding every reported island disappearance
to it still requires the independent terrain component or a player scene check.

