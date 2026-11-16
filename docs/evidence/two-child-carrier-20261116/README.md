# Tandem cluster-parity regression probes

Run from repository root: `python3 docs/evidence/two-child-carrier-20261116/check.py`.

The two tandem children reuse the existing cluster drag/gravity evaluator. Manual RMB consumption and automatic release distance use the same payload-carrier policy. They have no powered endpoint continuation, seeker, motor jet or carrier trail. Auto release applies the same bounded initial correction as cluster, without lateral scatter; it never steers after release.

The second body's initial state is a bounded phase offset along that same ballistic curve (at most 0.18 simulation seconds and approximately 12 world units, reduced near launch/obstacles). Collision checking reduces the offset; fully blocked rear placement falls back to zero separation rather than spawning through geometry. Thus a visible interval is not guaranteed at that safety fallback or in one long frame.

The probe compiles the actual spawn helper with C++17/UBSan and checks phase parity, finite values and source guards. Its trajectory formula fixture is not the full production collision scheduler. Native arm64 engine build passed on the final source. Full in-game manual/automatic release, rendered impacts and audio acceptance remain separate checks. Existing AA targeting and damage multipliers are unchanged.
