# Tandem presentation correction — 2026-11-16

Base ed5fd3cd22b0ed2e098385e99934d0cde3604ded. Candidate only, not published,
staged or tested in the installed game.

The prior presentation recursively rendered two complete HE VDB explosions,
with precursor alive until 0.7 s and main starting at 0.22 s. It also changed
the main seed, producing unrelated density/rotation variation. That failed the
requested visible precursor -> main sequence.

The precursor is now an analytic, narrow directional flash, using the existing
incoming visual axis and scene-depth occlusion. It returns before density, smoke,
ejecta and pressure-refraction shader work. Its lifetime is 0.16 simulation seconds;
the main remains at 0.22 seconds, with the same seed. The CPU scissor contains the
flash in every orientation. There is at most one phase draw at a time, one existing
bounded impact slot, and no new event, projectile, timer queue or damage call.
This is stylized game VFX, not a physical penetration or weapon design model.

Authoritative damage, radius, interception, fort handling, sounds and saved-game
IDs are unchanged: phonk_ordnance.cpp is not modified. Profile 7 still resolves to
the established single HE damage event. The small visible pause between phases
is intentional; actual game visual/audio acceptance remains open.

`python3 docs/evidence/tandem-presentation-20261116/check.py` compiles the actual
MSL function body against small C++ vector stand-ins and includes the actual
shared phase header. C++17 -Wall -Wextra -Werror + UBSan PASS: timing, nonoverlap,
NaN/negative phase guards, finite alpha, depth occlusion, incoming-axis rotation,
source branch before volume sampling, unchanged main seed and no damage events.
This is NOT an MSL compiler or native runtime test. The former source fails the
new regression because it has no dedicated precursor shader or short phase.

OPEN: native Objective-C++/Metal compile; script VM/UI tests in combined batch;
ship/fort/water/air impacts, slowmo0.1, FPS/hitch frames that might skip a short
flash, dense salvos, visual readability and audio timing. No Mac access was used.
Gameplay UI changes are developed in a separate coordinated patch and should be
reviewed/integrated together without treating these CPU tests as game acceptance.
