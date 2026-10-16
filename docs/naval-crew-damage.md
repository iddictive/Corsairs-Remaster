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
grapeshot50%, knippels35%, bombs80%, and airburst80% as a bomb-family default.
`NavalAmmo_InitDamage` reconciles these five values on new-game initialization
and normal old-save load, alongside the crew coefficients.

`AIBalls.c::Ball_GetAccuracy` multiplies the existing gunner precision, including
perks, by `(ammoAccuracy / 100)` before converting it to the existing random
scatter factor. Catalogue accuracy is clamped to0–100. Round shot100% preserves
the crew's ordinary precision; knippels35% remove65% of it. Skilled gunners still
scatter knippels widely, and poor gunners retain still wider scatter. This is
a precision multiplier, not a hit probability. Deliberate rake, target choice,
aim point, wind/trajectory and reload keep their existing owners.

The October 11 candidate raises knippels from30% to35%: at maximum1.25
gunner precision in simulation mode, scatter falls from0.825 to0.7625 (7.58%).
At zero skill it stays1.2; no other ammunition's accuracy or trajectory changes.

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
were0.05/0.575/0.825/0.2/0.2 for the earlier30% knippel value. The October 11
production-function VM repeats the zero/full-skill cases and two serializer
rounds at35% knippel accuracy:0.7625 scatter, with every other factor unchanged.
Out-of-range catalogue accuracy clamps correctly.
Actual installed ship/fort casualty and random-scatter replay remain required;
compilation alone does not accept battle balance.

## Deck crew combat visuals

The October16 mod adds spatial reactions to the existing sea ammunition and
successful native hull cuts. `AIShip.c::Ship_HullHitEvent` sends one visual
impact after the normal damage calculation; `Sailors` owns temporary escape and
death, and `crew_gore` snapshots the actual posed sailor mesh. Ordinary casualties
usually retain one complete corpse, with one anatomical separation in20% of cases;
laser casualties separate once into two complementary parts. Launch retains25%
of the caller impulse with no second upward kick. A native successful cut sends its local plane once, never on
save restoration. Existing fire response, deck support and ammunition casualty
calculations retain their owners. No new numeric crew loss or SAVE field.

Ordinary hit presentation receives the actual float casualty fraction after
defence, doctors and the crew floor. Visible deaths scale to the live low-poly
crew, carrying only a transient fractional remainder; a tiny hit cannot kill all
nearby visible sailors. Positive actual loss can emit blood once from the nearest
affected retained-deck sailor even without a whole visual death. Represented deaths
already emit their blood and do not receive a duplicate wound burst. This latest
source candidate has no tests, build or player replay by the owner's instruction.

The explicit follow-up adds abundant blood spray and blood coating the struck
ship. Reuse `location/src/blood`'s real mesh projection and `blood.tga` in an
optional ship-attached mode. Stains store receiver-local geometry, follow motion,
and reproject after hull cuts; land blood keeps its existing mode. They must
stay on retained wood, never bridge a removed hull section or paint sails/sea.
The finite per-ship stain pool expires with its sea scene; no save migration.

Acceptance: a close admitted crew hit produces blood and proportionate bodies; nearby
survivors escape along retained deck links, then resume work. Bomb/airburst
reach exceeds round shot; a laser cuts sailors at its actual cut plane. A hit
well below deck, a distant sailor, paused gameplay and an immune ship must not
produce spurious deaths. Fragment count, texture ownership and expiry remain
bounded across repeated broadsides and scene removal. Ordinary unhit sailors,
firefighting and cannon work must remain functional.

Source and native syntax checks pass for the crew, fragment renderer and blood
adapter. The actual script function passes the existing native VM for all five
ammunition profiles and zero-damage/immunity/sea/reload/boarding/interface guards.
An extracted actual-method CPU replay passes deck impact, true3D below-hull and
distant negatives, scaled laser planes, outward graph escape, disabled/fire route
rejection and no teleport. These component checks do not accept visible gore.

`CrewGore` caps the scene at96 fragments, retains its own texture references and
expires pieces after9–13 seconds (water shortens that lifetime). Attached Blood
uses at most128 stains/2048 triangles per ship; each lasts180 game-seconds with
a25-second fade. It follows the live wreck until the receiver disappears.
Ordinary cannon-hole rendering still has a legacy BSP/mesh mismatch; that
existing collision owner can admit blood over a cannon hole. Laser cuts expose
their current retained geometry to projection and trigger reprojection.

Canonical combined delivery includes the crew source and successful-cut hook;
accepting battle replay remains pending. The exact installed runtime verdict is
owned by `runtime.md`. No game launch/control or player SAVE change occurs.
