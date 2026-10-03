# Screenshot save packs

## Delivered pack

The October 3 portfolio capture pack is retained under
`experiments/native-metal/.cache/screenshot-save-pack-2026-10-03`.
`pack/Снять-скриншоты.md` maps six independently loadable saves to the missing
fleet, treasury and progression pictures. `Corsairs-Portfolio-Test-Saves.zip`
contains the guide, manifest and `SAVE/Портфолио`, including profile options.
The same profile is installed in the public app's external SAVE directory.
Select **Load → Портфолио**; select **Кампот** again to resume the player's game.

| Save | Prepared state | Capture |
| --- | --- | --- |
| 01 Treasurer / fleet | Four real ships and the existing appointed treasurer; distinct cargo targets, a cash reserve, automatic services off | Flagship and companion purchase tables |
| 02 Treasurer / loot | Existing flagship chest, one retained copy, price limit 750, armor excluded, emerald keep lock | Sale rules, eligible quantities and protected rows |
| 03 Shared chest / officers | One enabled officer's ammunition and small medicine moved into the same real chest; quantities and weight conserved | Officer inventory before and after opening the shared chest |
| 04 Captain progression | Original wounded rank-18 save; no seeded HP or rank gain | Character sheet after the ordinary installed-game HP migration |
| 05 Fleet rest | Existing brothel scene and four crew morale values set to 55/60/65/70 | Paid service and the resulting fleet morale |
| 06 Sea battle | Existing four-ship sea scene; native sea serialization unchanged | Fleet in action and direct hull aiming at 2× |

Captured pictures must show the actual result. A configured order table does not
prove automatic purchases, and a battle picture does not prove ammunition fallback.
The prepared morale and resupply deficits are test setup, not player progression.

## Format and preservation

The installed app resolves PROGRAM/RESOURCE through its bundle and keeps writable
state under `~/Library/Application Support/Iddictive Corsairs`. Saves are neither
installed engine resources nor source inputs for canonical staging.

`libs/core/src/compiler.cpp::SaveState/LoadState` owns the native file format:
a 32-byte build identifier, four little-endian 32-bit envelope fields, compressed
script variables, then compressed description/thumbnail data. Attribute names
use the existing SCodec table; native entity IDs are 64 bits and strings UTF-8.

The preparation adapted only the decoding/object routines from upstream
[gamesave-convert](https://github.com/storm-devs/storm-engine/tree/a68f43df197a50c5177f40209abc109f01f4b114/tools/gamesave-convert),
checked against the installed engine's serializer. Its old CP1251/version
conversion and sea-state conversion were not run. macOS native `struct 'l'`
cannot represent the engine's four-byte script integer; decoding uses `<i`.
The upstream sources and bounded preparation scripts are retained as ignored
provenance, not as a new maintained game-save editor.

Only named variable blocks are replaced. Every touched object first passes a
byte-identical decode/encode check against its original bytes; the resulting
save passes a full semantic roundtrip. Adding attribute names appends codec
entries without renumbering existing names. Item definitions, quests and native
`oSeaSave` state are preserved; no whole save or sea-state conversion is applied.

The dedicated profile name is stored inside PlayerProfile as well as the folder,
so subsequent quick/automatic saves stay in the capture profile. Stale profile-list
entries are removed; the normal load UI rediscovers folders through FillProfileList.
Copying a save into another folder alone does not isolate subsequent saves.
Delivery refuses an existing destination profile. Player configuration and
original SAVE files are never overwritten; hashes are checked before and after.

## Evidence boundary

All six saves pass the installed engine's actual LoadEngineState, with the
expected profile, character location and rank, and an empty error log. An
unchanged original save is the control. The temporary OnLoad callback observes
rehydrated variables instead of creating world entities or rendering a scene;
this proves native file compatibility, not interactive gameplay.

The treasury sample checks real chest stock, keep protection, ordinary mineral
eligibility and rare-gun refusal; the supply sample checks the enabled officer's
deficit; purchase targets and all four morale values are checked independently.
A corrupted compressed payload is rejected. `native-load-verification.json`,
`original-hashes.json` and `delivery.json` retain the evidence; the source profile's
205 recorded files remain unchanged. The ZIP passes its CRC check and installed
copies match the six manifest hashes.

The headless harness must initialize InterfaceStates.Launched before invoking
LoadEngineState. Script attribute branches are bound with makearef, not an
address expression such as `&Locations[index].box1`. These were harness-input
corrections; installed PROGRAM and the player's game were not changed.

The remaining accepting step is the player's ordinary load of each scene and
the indicated menu/action, followed by screenshots. No full scene replay or
automatic-service gameplay acceptance is inferred from the file checks.
