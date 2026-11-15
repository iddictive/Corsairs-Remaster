# Slow-motion missile camera clock mismatch

## Confirmed defect

At time scale 0.1, CoreImpl retains fractional milliseconds while simulation
advances integer milliseconds. Missile body rendering already samples
`age + GetScaledDeltaRemainder() * .001`, but both regular and late missile-view
producers sampled `age` for camera position and direction. SEA_CAMERAS consumes
those attributes directly. The rendered missile and camera therefore followed
different points on the same trajectory. This is a confirmed contributor to
slow-motion stepping in missile-follow view, not a diagnosis of every projectile
or a claim about Metal rendering performance.

## Fix and risk

Share the existing body render-age calculation with both missile camera owners.
Simulation ages, flightAge (also a steering threshold), collision, guidance,
damage, ballistic formulas and lifetime are unchanged. The presentation-only
sample is still capped at missile duration. Risk: camera framing moves by at
most one fractional simulation millisecond compared with its prior sample.

## Regression probe

From repository root:

```sh
S=experiments/native-metal/engine/src/libs
c++ -std=c++20 -Wall -Wextra -Werror -I"$S/ship/src" -I"$S/math/include" \
  docs/evidence/slowmo-missile-camera-20261115/probe.cpp -o /tmp/corsairs-camera-probe
/tmp/corsairs-camera-probe
```

The probe compiles the actual production header and CVECTOR, using 120 frames
of 8 ms real time scaled to 0.1 and a manual missile at 2000 world units/sec.
Observed before: camera/body error up to 1.6001 units; 23 repeated camera samples.
After: error 0, repeated samples 0. Repeated presentation sampling, zero
remainder (normal speed/pause), and duration cap also pass. This focused CPU
probe is not a full engine build or in-game visual acceptance.

## Visual acceptance (user-owned)

Rebuild on the supported Apple Silicon/macOS/Metal toolchain. Follow a missile
at normal speed and 0.1 speed; compare body stability in chase view, pause/resume,
cluster transition, and impact. Also observe from independent camera: any
remaining jerks there require separate diagnosis. Linux executor cannot run
that Metal acceptance.
