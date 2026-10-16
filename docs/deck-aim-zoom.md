# Deck aiming zoom

This is a separate camera/input feature from the aiming-volume contact overlay.
It applies only to the first-person shooting view on deck.

## Controls

- A short right-button click smoothly switches 1× → 5× or zoomed → 1×
- Holding the right button for at least 180 ms smoothly zooms in
- Starting a hold at 5× smoothly zooms out instead
- Releasing a hold fixes the current intermediate magnification; it does not trigger a click
- A short click from a fixed intermediate level returns to 1×
- Clicking again during an endpoint transition reverses its destination

A full held transition takes 1.2 seconds after the hold threshold. A full click
transition takes 0.4 seconds. Both use smoothstep easing in log magnification;
partial transitions are shorter. Mouse-wheel bindings are unchanged.

Zoom resets on leaving the shooting view, camera reselection, telescope use,
control locking, focus loss and pause. A pause revision lets the camera detect
an interruption even if both sea callbacks were frozen. Resuming with the button
held requires release before another zoom action. Transient zoom is not saved;
the existing binary save layout and script base perspective are unchanged.
An automatic neighboring-island handoff retains the same deck camera and held
zoom; rebinding its replacement ship does not count as a camera reselection.

The equipment-dependent spyglass limit is intentionally separate work. The
existing Ctrl spyglass controls and non-aiming right-button actions are preserved.
The spyglass now shares the raised aiming eye: at least the default standing
height plus 0.5 m above the deck path. This clears low eye locators and rails;
saved eye height, spyglass magnification and the third-person orbit are unchanged.
The height correction is a source candidate until staged and replayed on a ship.

## Projection and ownership

The renderer derives the original horizontal and vertical projection from the
base FOV, aspect policy and fov_multiplier, then multiplies both projection axes
by the same magnification. This produces actual, undistorted 5× zoom under both
legacy and new FOV policies. Near/far-plane updates retain the original input
and magnification. Ordinary camera/perspective calls default to 1×.

The aiming aperture continues to read the current projection matrix: a fixed
screen-space region therefore narrows in angle as zoom increases. There is no
second independently calculated aiming camera. Projection restoration is
limited to the active deck camera and its last effective renderer FOV.

## Source and checks

The tracked `engine/src/libs/sea_cameras/src` files own the zoom state machine.
`src/gameplay/PROGRAM/controls/init_pc.c` and `controls.c` own the fixed,
group-locked DeckAimZoom binding on fresh and restored profiles. Ordinary
source delivery preserves unknown runtime edits through its receipt.

Run the portable source probe:

```sh
python3 experiments/native-metal/deck_aim_zoom_probe.py \
  --engine-source experiments/native-metal/engine \
  --gameplay-source src/gameplay
```

The portable probe checks click/hold timing, interruptions, frame independence,
invalid/hitch deltas, bounds, camera projection ownership, both FOV policies,
non-default multipliers, near/far round-trips and fresh/restored fixed bindings.
It executes the actual renderer projection functions against a matrix
stub, not a native Metal device.

Current check status: zoom timing and camera lifecycle pass. The projection
fixture still models the old renderer facade and fails to compile against the
tracked `storm_render`/`renderDevice` methods; native renderer maintenance owns
that fixture update. The direct startup/restored binding check passes separately,
including rejection when saved mappings override the fixed binding. The complete
portable command remains unresolved until the fixture is updated.

Native macOS acceptance still requires a replay: short clicks, a half-held zoom,
a full zoom and outward hold, aim exit/reentry, Ctrl spyglass, menu/pause, focus
loss while held, load/reload, different display aspect ratios, and aiming-volume
alignment with a ship and with the sea at 1× and 5×. Portable tests do not prove
rendered behavior or an app build.
