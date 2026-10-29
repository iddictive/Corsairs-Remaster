# Falling-sail transfer component evidence

`SAIL::DoNoRopeSailToNewHost` reassigns rope-released cloth to a detached mast.
The existing hard-point conversion establishes the coordinate contract: changing
the host must preserve the cloth's world position. The old code converted the
attachments but retained the live `SailPnt` controls in the previous frame.
It also traversed the source group forward while transfers swap-removed entries.

The retained [raw results](observations.json) and the two frozen probe sources
below were recovered from the task's already completed disposable runs. They
are component evidence, not an installed-game replay or a permanent test suite.
No additional validation was run while archiving them.

## Inputs and procedure

Baseline: commit `b9bb2082bc346fb10c24ec2df0bd52abb4ec8a6e`,
`experiments/native-metal/engine/src/libs/rigging/src/sail.cpp` Git blob
`d7950b1121d865d80db31d4011cb975d08e326b4`.
Candidate: the same file, blob `ab5c237134271687dba493415e9f1b2c03af431d`.
The native math headers are unchanged from the baseline commit.

Both probes ran from the repository root with Python 3.12.14 and Apple clang
21.0.0 (`clang-2100.3.34.2`), target `arm64-apple-darwin27.2.0`.
The disposable producer supplied `CODEX_SCRATCH_DIR`; each probe compiled one
small executable, joined it, and released its intermediate files. These `.txt`
files preserve the Python sources, rather than registering new test commands:

- [Reparent probe](reparent-probe.py.txt) extracts the actual transfer block and
  compiles it against the native `CMatrix`. Compile flags are `-std=c++20 -O2`,
  `-Wno-implicit-const-int-float-conversion`, and the repository's native/math
  include directories. It seeds 20 controls at `(0.7*i, 5-0.4*i, 0.2*i)`;
  square sails consume 20 and triangular sails consume 15. Mode 0 uses identity
  matrices. Mode 1 uses old rotation/translation
  `(0.3,-0.4,0.7)/(32,18,-11)` and new
  `(-0.6,0.8,-0.2)/(-15,3,21)`. Mode 2 gives both hosts the old transform.
  The oracle compares each resulting world point with its pre-transfer world
  point, accepting a maximum error of `1e-4` world units.
- [Membership probe](membership-probe.py.txt) extracts the actual iteration
  header and swap-removal statements. Compile flags are `-std=c++20 -O2`.
  It enumerates every eligible-subset mask for group sizes 0 through 8: 511
  scenarios. The oracle requires each eligible identity to move and every other
  identity to remain. The baseline run counted missed eligible identities;
  the candidate run additionally checked the exact survivor set.

To reproduce, select the stated source version, set `CODEX_SCRATCH_DIR` to a
disposable directory, and run the corresponding frozen Python source from the
repository root. The membership source retains the stricter candidate oracle.

## Result and limits

The worst reparent discontinuity falls from `55.70219` to `8.996945e-06` world
units. Identity and same-transform controls pass. The baseline membership probe
fails 369 scenarios; the candidate passes all 511, including empty, all-eligible,
none-eligible and mixed groups.

The change adds at most 20 rigid point conversions per rope-released sail at
the detach event. Traversal stays linear in the source group. It adds no
per-frame work, allocation or GPU work; no timing or FPS improvement is claimed.
Rolled-cloth precedence and the existing free-cloth relaxation are unchanged.
Rendered falling-cloth appearance requires separate native scene acceptance.
