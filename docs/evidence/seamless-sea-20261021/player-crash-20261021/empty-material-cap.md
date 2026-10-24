# Empty material group in a secondary fragment cut

The supplied macOS report records a crash at 2026-10-21 20:59:40 -0400,
thread 12, `SHIP_CUT_FRAGMENTS::PollDamage` worker → `prepareFragment` →
`splitSurface` → `solidSectionCaps + 6872`, reading address `0x18`.

Reported Mach-O UUID `8C5B5F82-F1B4-3AAE-BAEF-E4852E087832` matches the
retained ammunition cut (raw engine SHA-256
`fabd945a78aa5fb7ac8a5d2e8c9e4adfb7caf9fd6451cb1729865fc2989ad5ad`).
It differs from the subsequently installed seamless-sea UUID
`2C54DFA4-503D-3E2E-8A57-ABE5F498B0A6`.

At unslid PC `0x1001164f4`, disassembly is `ldr w26, [x8, #0x18]`.
The report records `x8 = 0`. The preceding instructions select the uncharred
color branch and load the first material group's triangle-vector storage.
`Vertex::color` has offset `0x18`: this is the dereference in
`solid.begin()->second.front()[0].color`, not the sky work on thread 0.

The shared header's pre-fix SHA-256 is
`4672f1596bce18372cbcff44a3eaaf345ae6198f26b62a6c90d932ca927a3b8b`
in both the retained crashing cut and the seamless-sea source.
`subtractVolume` deliberately retains empty material keys; a later key can
still contain the solid's geometry. The correction selects the first nonempty
group for the cap's material and color. A mesh with no triangles produces no
caps. Geometry clipping, triangulation and worker scheduling are unchanged.

No new game, VM, GPU or benchmark replay was run: the user stopped those checks.
The report, matching binary disassembly and source establish the invalid read;
player-scene acceptance remains separate.
