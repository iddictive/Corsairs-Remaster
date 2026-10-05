# Treasurer service availability and sale rules

## Contract and owners

`tools/gameplay/treasurer.c` owns merchant eligibility, chest-sale filtering and
arrival callbacks. `fleet-gameplay.json` pins its composed runtime bytes;
`treasurer-interface.c` consumes the same sale quantity and service functions.
The older fleet audit entries describe previous protection rules.

## Merchant eligibility

Store interiors have location type `shop`, not `store`. The merchant location and
the town door destination must resolve through `FindLocation`: actual game data
contains `FortFrance_Store` versus `FortFrance_store`. A literal comparison and
an invented `store` type both reject the real open shop.

`Common_Store.c` has its nationality-based trade refusal commented out. Treasury
must not introduce that refusal independently. Combat/alarm, blocked doors/night,
death, anger, active NPC service locks and quest dialogue restrictions still apply.

## Sale and arrival

Rarity is descriptive, not a veto. Selected categories, the exclusive base-price
ceiling, retained copies and explicit item locks control ordinary chest stock.
Quest/unique flags and `IsQuestUsedItem` remain protected; personal and officer
inventories are never sale donors.

Automatic service queues on port, town and `shop` arrival, if enabled. It skips
save rehydration, cancels stale arrivals and defers while UI/dialogue is busy.
Only actual proceeds or spending produce the existing text/item-icon notification;
no sound is added.

## Evidence and limits

October 5: native script VM replay uses the real Fort-de-France shop save and the
candidate core. Merchant resolution, door blocking, rare sale and manual/category/
price/quest protections pass with an empty error log. The fixture resolves town
from the saved location attribute because it does not load rendered location entities.
An attempted transaction replay omitted normal stock-module bootstrap and failed;
its inventory/money outcome is not acceptance. Normal scene arrival, transaction
and notification replay remain player-owned. Installation status is in runtime.md.
