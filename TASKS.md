# Active work

## Stable performance in gameplay

Goal: reduce measured stutters while preserving hit behaviour, visual effects,
controls and saves. Final acceptance is the installed game in comparable player
scenes; component timings and CPU samples are not FPS measurements.

- Grapeshot: investigate the 2026-11-05 10:13 sample's projectile/mast
  collision and particle load. Preserve the number and behaviour of shots.
- Flight: the duplicate target prediction was removed and installed at
  `0da6d8a`. Measure the complete AI salvo and flight in the player scene;
  re-entry freeze and water/air state transitions remain unresolved.
- Transitions: investigate cold map-to-sea returns and compare the complete
  transition, including restoration and correctness.
- GPU: assess independent visual particle computation against the existing
  Metal path and measure the complete consumer before changing ownership.
- Extend profiling to other observed heavy scenes; compare frame-time tails,
  not just average FPS, and preserve quality and gameplay.

Related player-reported defects remain open: island polygon flicker, unrealistic
mast and ship motion, and nuclear-wave shape and abrupt attenuation. Mast
gravity cannot be accelerated safely until moving-mast collision is verified.
