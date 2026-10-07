# Sailing propulsion floor

The player's requested minimum is one quarter of the ideal hull speed when all
counted masts stand and sails are raised. It is a propulsion target, not an
instantaneous velocity guarantee: acceleration, collisions and grounding still
control actual movement. Partial mast loss retains the standing fraction; full
dismasting, lowered sails, stop, death, fixed position, surrender and NPC drift
disable assistance. The current native count includes numbered mast nodes below
`TOPMAST_BEGIN`, including the bowsprit; this remains the existing contract.

`src/gameplay/PROGRAM/sea_ai/AIShip.c:Ship_UpdateParameters` owns the ideal hull
speed supplied to the engine. `sailing-propulsion-floor.patch` owns native
`SHIP::CalculateNewSpeedVector`; the battle navigator reads actual velocity and
applies the existing HUD speed scaler. Native physics uses arcade speed units:
arcade mode multiplies propulsion by 2.5 and the HUD divides by 2.5.

The old script stored an unscaled ideal beside scaled `Ship.MaxSpeedZ`. A decoded
player save had arcade mode enabled, four intact masts, no blocking flags and
ideal speed 8.6057501. Its native target floor was 2.1514375, only 0.860575 on the
HUD. The intended arcade target is 5.3785938, displayed as 2.1514375.
One local coefficient now scales both the ideal and MaxSpeedZ on every parameter
update; mode changes replace the derived ideal rather than multiplying old state.
The normal-mode result and turn-rate scaling are unchanged.

A disposable fixture executes the changed script blocks in the real script VM:
normal → arcade → normal passes with zero script errors. Another fixture compiles
the exact installed native target method under ASan/UBSan, using explicit
attribute/force adapters. Both modes produce the same HUD quarter; partial and
full mast loss, blocking flags, lower sails, faster stock propulsion and a mast
count exceeding the vector pass. These probes establish target math and guards,
not live acceleration or HUD acceptance. Installed delivery and player replay
remain separate states in `docs/runtime.md`.
