# Ship wake ribbons

## Status

October15: long-wake index overflow reproduced in the actual reservation/draw
methods; a bounded local-index correction is source verified. Native staging
and the reported player's moving scene remain pending.

## Contract and owners

Wake triangles connect adjacent samples of one water-contact interval. A long
fast wake retains its full authored lifetime, width, texture, alpha and wave
projection; it must not fold distant samples into a straight connector.

- `engine/src/libs/ship/src/track.cpp`: both wake grids, shared buffers and
  draws. `track.h` owns retained rows and contact segment identity.
- `PROGRAM/sea_ai/AIShip.c::Ship_SetTrackSettings`: one-metre spacing,
  12 columns for the ship trail and four for the keel trail.
- `PROGRAM/ships/ships_init.c`: authored lifetimes, up to18.5 seconds.
- `renderer/src/s_device.cpp::DrawBuffer`: its `minv` is the native base vertex,
  not an index-buffer offset; the backend validates the local vertex count.
- Flight gating and landing interval semantics remain in `sea-phonk.md`.
  Texture/lighting black-splash investigation is separate.

## Cause and correction

The old grid writes absolute vertex numbers into `uint16_t`. Twelve columns
admit only5461 complete rows: at5462 rows, actual draws contain15 triangles
joining nonadjacent history. Four columns have the same defect past16384 rows.
The wrap can connect samples over5461 metres apart along the recorded path.

Both wake types now reuse one bounded local index grid and draw it with the
existing base-vertex facility. Adjacent batches share one row, preserving every
quad exactly once. Water-contact segments never share triangles. Full vertex
history, width/UV/alpha generation and texture/shader state are unchanged; the
correction neither shortens the wake nor changes its sampling density.

## Evidence and limits

Disposable ASan/UBSan probes execute the actual `Reserve1`, `Reserve2` and
`Realize` bodies with a renderer-buffer adapter. Both real column counts
(four/twelve) cover2/100/5461/5462/
9523/16385 rows, continuous tracks, separated contact segments and isolated
single rows. Before the correction, the first overflowing12-column draw has
maximum row separation5461; afterward every triangle spans exactly one adjacent
pair of the same segment. Exact ordered global indices match the intended
grid, with full coverage and unchanged winding; declared vertex/index spans
stay within their buffers. The native Ninja-flags syntax check passes; these
component checks do not prove the player's rendered scene.

The reported screenshot displays498.4 knots. Sustained500 knots with the
stock maximum lifetime stays below the16-bit limit; a recent1000-knot boost
can leave enough surviving samples to overflow. The player recalls Shift but
is unsure, so the screenshot alone does not prove the precise speed history,
ship type or runtime row count. Do not promote the component reproduction to
player visual acceptance or close an independently reported black splash.

The original4112×2580 frame is retained in the existing private renderer-evidence
owner at `.local-archives/metal-renderer-evidence/wake-index-20261015/reported.png`,
SHA-256 `d67a2a1b6635f3b0aa9332892a9bc9f93412b5b5a1e2d46bc151c20ef7360a8d`.
Required player replay: long boosted curved wake, return to500, then an ordinary
short wake and a flight/landing gap. No game launch/control is authorized here.
