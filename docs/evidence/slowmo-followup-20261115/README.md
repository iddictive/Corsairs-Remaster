# Slow-motion follow-up: zero-ms camera frames and inverted evasion basis

Base: 136a7c9bd79b5efd21f6d07287742926d0e54ae9.

## Confirmed findings

1. The previous render-age helper alone was insufficient: SHIP_CUT_FRAGMENTS
   skips all execute work when rounded delta is zero. ExecutePhonkOrdnance
   also returns on zero, so missile camera attributes remain at the previous
   fractional sample while missile bodies advance their fractional render age.
   At 0.1 scale, 8-ms frames become repeating zero/one-ms simulation ticks.
   The patch refreshes the existing late camera publisher on zero-ms execute
   ticks, without running projectile integration, collision or effect updates.
   This fixes a confirmed relative-camera vibration path. It does not prove
   all independent-camera projectile vibration is eliminated.

2. evadeInterceptor constructed sideways = forward x worldUp, then
   up = forward x sideways. The latter points DOWN for ordinary flight.
   Kinzhal positive vertical evasion therefore dove while the clearance guard
   only checked negative vertical requests. Correcting the second cross product
   makes vertical requests agree with the existing sea-clearance guard.
   Kalibr explicitly sets displacement.y, so that branch is unchanged.

## Checks

`python3 docs/evidence/slowmo-followup-20261115/probe.py`

Compiles the actual stage guard and evasion basis extracted from production,
with the real CVECTOR, C++20 -Wall -Wextra -Werror. PASS: zero-ms camera refresh,
no simulation on zero, positive-time normal path, irrelevant-stage skip,
correct up sign and orthogonality during level/climbing/diving flight. Mutation
back to the old evasion basis fails the test as expected.

## Remaining limits

This is not a full Metal build or in-game acceptance. The existing clearance
estimate only samples four future points; it is not a hard continuous altitude
floor. No artificial height clamp or guaranteed-hit logic is added: either
could break terminal attack, collision and intended interception outcomes.
A stronger evasion envelope needs trajectory replay and terminal hit/miss tests.
Visual acceptance remains user-owned: slow motion with both chase and external
camera, pause/resume, repeated evasions over water and terminal ship approach.
