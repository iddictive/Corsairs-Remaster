# Nuclear foam footprint and texture comparison

The installed shader used the positive nuclear crest as the blend mask. The
wave packet also contains negative-height troughs; there the mask became zero
and restored the height-based legacy foam. The candidate carries a separate
continuous nuclear-packet footprint through the sea vertex and uses the
existing four-tap anti-tiling sampler on the nuclear foam texture. Ordinary
tsunami/whirlpool mask input is separate. This does not change wave geometry,
buoyancy or impact damage. Sea vertices grow from 40 to 44 bytes and the
nuclear foam path uses four texture taps instead of one.

The two `*-scene.ppm` frames are the engine-linked synthetic fleet, not the
player's game or a visual acceptance. Both show ships across a daylit sea;
the candidate has sparser, less regularly repeated foreground foam. The
reported in-game stripes have not been independently reproduced or accepted.

The paired 1280×800 run used 8 ships, 600 frames per case, the last 300 for
CPU timing, 16 ms simulation steps, and a synthetic tier-1 wave each frame for
the burst case. No-wave and burst baseline/candidate CPU medians were
8.404/8.529 ms and 8.707/8.574 ms. GPU medians from recorded timestamps were
5.270/5.200 ms and 5.761/5.709 ms respectively; these short sequential samples
do not prove performance at larger yields or during real battle. CSV traces,
library hashes and source fingerprints are retained alongside this note.

For reconstruction, use the retained baseline/candidate source snapshots and
fixture. The driver derives build commands from the local configured Ninja
build and links both variants against the same existing libraries; its hashes
are recorded in the JSON. It needs the installed game resources and the
project's prepared input toolchain. From the repository root, set
`WAVE_EVIDENCE_INPUT` to this directory, `WAVE_RESULT_DIR` to an empty owned
temporary output directory, and run `FLEET_WAVE_MATRIX=1` with `paired.py`
under `codex-run-scratch`. Changed library hashes or toolchain/runtime inputs
make a rerun a new measurement, not a reproduction of identical timings.
