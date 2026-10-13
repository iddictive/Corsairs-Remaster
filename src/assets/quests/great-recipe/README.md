# Great Recipe item locators

`prepare.py` builds three additive GM files from the retained stock
`hold_l.original.gm` dummy mesh and the exact transforms in `layout.json`.
The layout records source hashes for Hold, Deck2 and Camp_Deck. Existing string
offsets, materials and geometry remain intact; only the label set is replaced.

`manifest.json` binds each prepared file to its exact runtime destination.
`tools/sync_metal_gameplay.py` owns delivery. The cloned quest rooms load these
through `models.always.greatRecipeItems`, allowing the native location owner to
register actual `item` entry events. Script attribute aliases alone cannot do so.

The source/format check verifies copied matrices and unchanged mesh payload.
Actual floor placement, visibility and pickup/use remain player replay checks.
