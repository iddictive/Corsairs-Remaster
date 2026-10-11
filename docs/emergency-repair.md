# Emergency hull and sail repair

## Contract and owners

LightRepair is free emergency stabilization, not a replacement for material
repairs. It runs for60 seconds at0.5 percentage points per second, up to a
20–40% condition ceiling based on effective Repair skill, with240 seconds
between activations. InstantRepair consumes the existing planks/sailcloth
allocation, runs for30 seconds at2 percentage points per second and stops at75%
condition, with360 seconds between activations. These are repair limits, not
guaranteed gains under incoming damage, missing materials or broken sail geometry.
Neither ability restores fallen masts or crew.

`src/gameplay/PROGRAM/battle_interface/utils.c` owns repair ticks and the shared
`BI_SlowRepairLimit` used by both flagship and companion menu eligibility.
`PROGRAM/_mod_on_off.h` owns InstantRepairRATE; `interface/perks/perks_init.c`
owns cooldowns and active durations. The existing perk state and officer lookup
remain authoritative. An expired or inactive ability must stop before damage,
material withdrawal or boarding deferral. Existing sea, companion and death
guards, fractional material accounting and sail-hole repair are preserved.
The two AbilityDescribe bodies disclose the same rates, ceilings and timers.

The previous free ceiling was25–70%, above the paid65% ceiling; its menu repeated
that formula. The fast repair queue also lacked an activity guard, while the
slow queue could perform one final expired tick. Their correction belongs to
the existing event/perk lifecycle; no separate repair state or timer is needed.

## Delivery and evidence boundary

The canonical files are complete editable sources admitted from the installed
composed baseline, not regenerated from a raw historical game tree. The normal
gameplay receipt delivers them with the already installed native engine and
preserves SAVE/configuration. A saved character's existing perk tree and active
timer are not reset; new activation reads the current perk definitions.

Disposable actual-script VM checks cover the balance limits, event lifetime,
material shortage and inactive/boarding/sea/companion/death boundaries. They
instrument external services and cannot prove rendered sail restoration or the
player's battle balance. Full startup compilation checks shared menu consumers;
installed activation, officer inheritance and expiration under a live battle
remain player replay. Current delivery disposition belongs to `runtime.md`.
