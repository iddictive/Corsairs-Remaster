# Combat debug controls

The existing debug menu has three independent, OFF-by-default player options:
infinite energy; instant player-ship cannon recharge; unlimited player ship
ammunition. Missing old-save attributes mean OFF. Existing presets and Phonk
retain their distinct policies; NPC energy/ammo/recharge are unchanged.

Ship toggles are deferred until the queued salvo finishes. The controller
unloads under the old policy, applies saved pending attributes through one
script event, and recharges under the new policy. The menu shows `после залпа`;
a second toggle cancels the pending change. Ordinary reload/cancel and sea
teardown keep the old policy until that transition, preventing virtual rounds
from being refunded into physical cargo. No new native save fields or message
signatures were introduced. Zero recharge has an explicit finite progress guard.

The retained actual-script VM probe covers 53 branches with RunCheats=0 and
script_errors=0. The native component uses extracted current methods and
substituted dependencies: normal/zero timers, physical and virtual queued fire,
cancellation, ordinary reload and empty-controller restore pass; interrupted
virtual rounds change stock by zero. This is component evidence, not live sea
combat. The existing VM probe's after-load callback reports Invalid function;
that route was not counted as a passing save/load replay. Ordinary old-save
Sea_Load recreates the cannon handler through the existing environment owner.

```sh
export CODEX_SCRATCH_DIR=$(mktemp -d /tmp/corsairs-cheats-check.XXXXXX)
python3 docs/evidence/debug-combat-cheats/probe.py
clang++ -std=c++20 -O2 "$CODEX_SCRATCH_DIR/native.cpp" -o "$CODEX_SCRATCH_DIR/native-probe"
"$CODEX_SCRATCH_DIR/native-probe"
experiments/native-metal/.cache/build/bin/script-vm-probe "$CODEX_SCRATCH_DIR" RunCheats 0
```

`native.cpp` retains the executed component. `menu.json` records group counts
6,12,5,12,12,3,6,10; the 12-row capacity is preserved.
