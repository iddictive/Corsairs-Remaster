# Bounded storm escape

## Contract

An ordinary storm permits world-map escape within120 real seconds, even while
regional weather continues. The existing60 simulated-second delay remains the
earlier path: about10–12 real seconds at the storm's maximum6x time scale.
Returning to the map must not immediately readmit the ship to the same continuous
storm region. Leaving that region or its natural clearing rearms storm contact.
Authored tornado/tsunami contacts, enemy/fort proximity, quest restrictions and
ordinary fleet departure validation retain their independent admission rules.

## Owners and state

`src/gameplay/PROGRAM/sea_ai/sea.c::Sea_StormEscapeFrame` owns the real-time
ceiling and map rearming; `Sea_MapLoad` records escape only after departure
validation. `AIShip.c::Ship_CheckMainCharacter` records whether weather alone
owns the map lock. `worldmap.c::wdmEvent_StormEntry` skips only an escaped native
regional contact, retaining the stock callback for authored hazards.
`worldMap.stormEscape` and the elapsed sea counter use the existing script save
codec. New sea creation resets the counter; a restored sea retains its saved
counter. Native WEATHER query6 provides uncapped monotonic elapsed seconds and
rebases on binding; it changes neither weather generation nor regional lifetime.

## Evidence and rejected approaches

The installed storm callback reproduces immediate readmission of an escaped ship
(`ReentryCases=201`, zero VM errors). The candidate's actual callback and frame
body pass119.9/120-second boundaries, a30-second hitch, same-region suppression,
dry rearming, later storm entry, and combat/quest/tornado/inactive-sea negatives
(`EscapeCases=0`, zero VM errors). The canonical native build passes; the complete
installed PROGRAM with exactly the three candidate script overlays compiles in
the real VM (`CompileStormProgram=0`, zero errors). The extracted native clock
clause reports0.255025 seconds against0.255015 wall seconds and rebases to zero.

Waiting for meteorological clearing was rejected by the player's explicit
two-minute ceiling. The engine's `iRealDeltaTime` is capped at100ms per frame and
cannot enforce that ceiling below10 FPS. Script RDTSC truncates a platform
frequency counter to32 bits without a script frequency contract. A timed grace
period on the map could expire inside the same storm and recreate the loop.

The isolated real SAVE codec passes four consecutive restore rounds
(`SaveSeed=0`, `RestoredEscape=0` ×4, zero script errors), retaining earned escape
and elapsed sea time through the120-second boundary, dry rearming and a later
native storm contact. The earlier driver omitted `OnLoad`; the compiler invokes
that callback on restore without a guard. Adding the fixture's empty callback
repairs its invalid-function error without changing the frozen product functions.
The larger incomplete driver is also rejected as acceptance evidence.

## Remaining player evidence

Canonical stage-only delivery passes with the installed hash recorded in
`docs/runtime.md`; actual sea→map→leave-region replay remains a separate check.
Normal player save/load during storm and after escape remains pending; the codec
fixture does not recreate the complete player scene.
