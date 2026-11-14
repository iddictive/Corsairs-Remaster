# Missile numerical probes

## Apex and cluster comparison, 2026-11-14

For commit `193596f` versus the updated flight header, the same 9000 m,
12-shot cluster probe (24 blocks, flat water) gave impact speeds of
2615–7227 versus 1612–4291 m/s and flight times of 0.497–1.938 versus
0.712–2.528 s. The new arc therefore slows the blocks substantially; this is
an arcade-model numerical comparison, not visual or gameplay acceptance.
Reproduce with `probe.py`; its generated vector stub also needs `operator|`
and `operator-=` when compiled against the current headers. Use the original
headers from `193596f` for the baseline.

For the updated default profile, 500 m range produced an apex of 1204 m,
741 m initial retreat and 10.4 s duration; 8000 m produced 9033 m apex,
2051 m retreat and 14.8 s duration. Sampling the 2400 m flight at half a
second before/at/after apex gave 1068/1157/1235 m/s. A 648-case sweep across
distance, elevation and settings found finite endpoints and monotonic arc
inversion (maximum fractional residual 1.3e-7). These are profile-only
calculations, without seeker corrections, rendered visuals or frame timing.

The probes compile the current flight header and extract the current ordnance
calculations and hull-aim helper; they do not launch the game. Run in this
order with an owned temporary directory in `PROBE_DIR`:

```sh
PROBE_DIR=<owned-temporary-directory> python3 docs/evidence/oreshnik-trajectory-20261113/probe.py
PROBE_DIR=<owned-temporary-directory> python3 docs/evidence/oreshnik-trajectory-20261113/hull_probe.py
PROBE_DIR=<owned-temporary-directory> python3 docs/evidence/oreshnik-trajectory-20261113/selection_probe.py
PROBE_DIR=<owned-temporary-directory> python3 docs/evidence/oreshnik-trajectory-20261113/control_probe.py
```

The first probe supplies a minimal vector stub used by the next two. On Apple
clang 21, using 12 shot IDs, owner 7, 24 blocks and 9000 m range, releases were
3553–5822 m above flat water. Block time to impact was 0.480–1.952 s, impact
speed 2448–8274 m/s and the first-to-last impact spread 0.614–1.094 s per shot.
Before changing descent variation the same cases gave 0.643–1.528 s, 3228–5994
m/s and 0.194–0.385 s respectively. All blocks still separate at the same
carrier event; flight times and impact positions differ.

The hull probe uses a synthetic box collision mesh and tests four headings,
three approach directions and stationary/translated targets. It is not a test
of actual ship models, waves, future ship rotation or gameplay. The selection
probe checks HE for each direct/random type; the control probe checks the
angular limiter, invalid rays and the fire-release latch. Visual appearance,
complete native integration, input timing and frame performance require game
runtime verification.

After enlarging the Oreshnik footprint, the same 12-shot calculation yielded
0.467–1.942 s to impact, 2451–8282 m/s at impact and 0.636–1.147 s between
first and last impact. A 20 m grid over a 1000 m circle, using the unjittered
760 m golden-angle pattern and 330 m radius per impact, gave approximately
89%, 91% and 95% nominal coverage for 20, 24 and 30 impacts respectively.
This is a planar geometric estimate: damage falloff, random scale, terrain,
ship motion and actual frame cost are not represented.
