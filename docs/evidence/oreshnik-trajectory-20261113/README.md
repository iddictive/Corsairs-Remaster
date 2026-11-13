# Missile numerical probes

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
