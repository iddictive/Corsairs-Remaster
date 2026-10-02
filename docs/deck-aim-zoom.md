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

The equipment-dependent spyglass limit is intentionally separate work. The
existing Ctrl spyglass controls and non-aiming right-button actions are preserved.

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

`experiments/native-metal/deck-aim-zoom.patch` follows `deck-eye-height.patch` in
the ordered source stack. `tools/metal_deck_controls.py` installs the fixed,
group-locked DeckAimZoom binding on fresh profiles and restored profiles. Its
exact-hash migration accepts both the previous reviewed layer and this one;
unknown script revisions still fail closed.

After applying the canonical source stack:

```sh
python3 experiments/native-metal/deck_aim_zoom_probe.py \
  --engine-source experiments/native-metal/.cache/storm \
  --gameplay-source experiments/native-metal/inputs/gameplay
```

The portable probe covers click/hold timing, interruptions, frame independence,
invalid/hitch deltas, bounds, camera projection ownership, both FOV policies,
non-default multipliers, near/far round-trips and exact-hash script migration.
It executes the actual patched renderer projection functions against a matrix
stub, not a native Metal device.

Native macOS acceptance still requires a replay: short clicks, a half-held zoom,
a full zoom and outward hold, aim exit/reentry, Ctrl spyglass, menu/pause, focus
loss while held, load/reload, different display aspect ratios, and aiming-volume
alignment with a ship and with the sea at 1× and 5×. Portable tests do not prove
rendered behavior or an app build.
