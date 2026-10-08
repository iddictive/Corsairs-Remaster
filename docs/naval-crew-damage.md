# Naval ammunition crew damage and accuracy

## Status

October 8 crew percentages and ammunition precision multipliers are installed.
The player rejected fixed-person
damage because it does not scale with a ship's crew. Installed-app delivery and action replay are
tracked in `docs/runtime.md`.

## Contract

Naval ammunition removes a percentage of the target's current crew, not a fixed
number of people. These are base percentages before the existing cannon,
airburst falloff, captain, hull-protection and doctor modifiers:

| Ammunition | Base crew loss |
| --- | ---: |
| Round shot | 0.05% |
| Grapeshot | 0.6% |
| Knippels | 0.075% |
| Bombs | 0.125% |
| Airburst | 1.2% |

The percentages are one quarter of the previous numeric coefficients. At 400
crew this preserves that coefficient's loss; smaller crews lose fewer people,
larger crews lose more, with the same relative loss. Repeated hits use the
remaining crew, so casualties diminish. Airburst retains twice grapeshot's
base crew damage and its existing overhead hull-protection bypass. Hull and
cloth damage remain point values.

## Projectile accuracy

The catalogue owns the player's requested gunner-accuracy multipliers: round shot100%,
grapeshot50%, knippels30%, bombs80%, and airburst80% as a bomb-family default.
`NavalAmmo_InitDamage` reconciles these five values on new-game initialization
and normal old-save load, alongside the crew coefficients.

`AIBalls.c::Ball_GetAccuracy` multiplies the existing gunner precision, including
perks, by `(ammoAccuracy / 100)` before converting it to the existing random
scatter factor. Catalogue accuracy is clamped to0–100. Round shot100% preserves
the crew's ordinary precision; knippels30% remove70% of it. Skilled gunners still
scatter knippels widely, and poor gunners retain still wider scatter. This is
a precision multiplier, not a hit probability. Deliberate rake, target choice,
aim point, wind/trajectory and reload keep their existing owners.

The existing `CANNON_GET_FIRE_ACCURACY` query forwards the same live factor to
native `AimLiveSpread` in the unchanged `cannon-rake-spread.patch`. The preview
uses the same direction/elevation/speed bounds as actual firing;
there is no separate ammunition map or new event/save contract. Descriptions
read the catalogue's live `Accuracy` field.

## Owners and saved state

`NavalAmmo_InitDamage` in `PROGRAM/store/initGoods.c` is the only writer of
the five crew coefficients. `Airburst_InitGoods` calls it for both ordinary
initialization and the existing `seadogs.c::OnLoad` catalogue reconciliation.
Old saved absolute coefficients are replaced without resetting goods identity,
prices, cargo, shops or other saved fields.

`AIShip.c::Ship_AmmoCrewLoss` converts the modified percentage into people.
Both `Ship_HullHitEvent` and `AIFort.c` use it before the existing
`Ship_ApplyCrewHitpoints`. Captain defence, doctors, immortality and the minimum
crew floor keep their existing owner. Other crew losses and land weapons remain
absolute and do not use this conversion.

`GoodsDescribe.txt` uses the catalogue's live values and explicitly labels crew
damage as a percentage of current crew. Hull and sail fields remain numeric.

## Acceptance

A 1.25% modified hit removes 1.25 of 100 crew and 12.5 of 1,000 before the
remaining defensive modifiers. Zero crew or nonpositive damage produces no
loss; conversion cannot exceed the whole current crew. A saved old catalogue
must receive all five new values through the load path while retaining ordinary
goods and cargo. Must not reinterpret ram, fire or other absolute crew damage.

The disposable native VM passes old-catalogue save/load reconciliation, the
actual percentage converter and all five accuracy values. At zero captain skill,
scatter factors remain1.2 for unskilled gunners; at1.25 precision the five factors
are0.05/0.575/0.825/0.2/0.2. Out-of-range catalogue accuracy clamps correctly.
Actual installed ship/fort casualty and random-scatter replay remain required;
compilation alone does not accept battle balance.
