# Seamless San Juan port — frozen

The user froze shore/rowboat travel on October16 after rejecting the installed
scene. Full unfinished source, including the two optional-camera crash repairs,
is preserved at Git ref `codex/frozen-shore-travel` (`6dc08e7`). Ordinary town
transitions are restored; the earlier experimental sea-city geometry and
decorative population remain. SAVE reference compatibility is retained separately.
No further port implementation or journey QA is active.

The sections below describe that frozen design, not the active runtime contract.
Geometry placement and protected fort assets remain owned by
`puerto-rico-city.md`; live deck movement by `live-deck-walk.md`; HP, crime and
naval damage keep their existing owners. `../TASKS.md` owns the current handoff.

## Player contract

Use the original San Juan characters, quests, dialogue, inventories, shop hours
and door locks in the sea-region scene. The supported journey is town/pier ↔
controllable rowing launch ↔ the player's actual moving ship/deck, followed by
ordinary cannon combat against town actors, wanted consequences and fort fire.
No visible loading/fader or fleet reset occurs on this journey. Internal asset
preparation, streaming and culling are permitted; they must preserve the state
and continuity the player observes. Service/quest interiors belong to this
journey; other islands, jungles and caves are not implicitly included.

The installed `460039f` city batch is the starting baseline, not acceptance of this
feature. Its SeaTown inhabitants are deliberately decorative. They do not have
canonical character identities, quests, HP, combat groups or dialogue.

## Responsibility and state boundaries

| State | Canonical owner | Required consumer contract |
| --- | --- | --- |
| Region, ships, fort, balls, weather | Existing sea environment | Residency survives hero movement ashore; teardown occurs when leaving the region |
| Hero's current input/camera/navigation mode | Port travel coordinator | Exactly one visible hero, one input owner and one camera writer |
| Town transform | `src/assets/islands/puerto-rico/layout.json` | Models, locators, navigation, actors, collision and lighting use the same placement |
| Land context and collision/navigation | Real native `Location` | Explicit context identity; no first-entity lookup when multiple contexts exist |
| NPCs, quest/dialogue/trade state | Canonical `Characters[]`, original scripts and LAi | Do not substitute decorative citizens or reset generated actors during a boat trip |
| Rowboat, oars, visual transfer | Native transient travel entity | Motion, hands, oarlocks and water stroke share one phase; safe transfer tracks the live ship |
| Persistent travel and land/naval restore | Existing attribute/save coordinator | Restore both prior naval state and logical hero/land context without changing old binary camera layout |
| Human damage and consequences | Existing LAi HP/death and crime scripts | Native geometry selects contact; scripts own damage, attribution, death and wanted state |

The script character object currently doubles as the ship entity reference at
sea and the land Character reference ashore. A retained naval scene therefore
requires explicit entity-role handling. Merely changing `pchar.location` or
creating a Character does not prove that sea message consumers still address
the real ship. Likewise `bSeaActive` currently carries both hero mode and naval
residency; its consumers must be classified before separating those states.

## Existing facilities and rejected shortcuts

Real Location already owns models, navigation and supervisor callbacks on land
layers separate from sea layers. Cabin loading demonstrates retained sea
entities, but freezes maritime simulation, moves shared sea/weather and clears
balls; it is a reuse reference, not the seamless implementation.

`Character::BindLocation` and the existing live deck's ship-relative placement
are reusable. Native Location, camera and loaded-location script lookup must
select explicit contexts. LAi login/release and CharactersGroups currently own
global state; repeatedly calling ordinary load/unload would erase the resident
town and cannot be used unchanged.

The stock boat AI class has no rowing controller. Stock DeckBoat uses a fader and
a stationary location. The clean authored BrigQeen1 boat hull is a candidate,
pending native visual acceptance. Installed action descriptors have no rowing
action. Seated idle alone does not satisfy rowing; compatible authored skeletal
motion, oars, seats and credible hand/water contacts are required. High-deck
ships also require a visible supported climbing/boarding route.

Ordinary balls currently have no land-character collision dispatch. A port-only
adapter must select the nearest solid/contact before causing damage, preserve
existing projectile/debris/special-ammunition behavior and apply bomb blast
occlusion. Canonical HP/death/immortality and quest protection remain in scripts.
Never send a ship entity to CharactersGroups as a land Character. Tactical fort
hostility uses the existing relation publication; a fabricated fort hit would
also damage the fort and apply unrelated consequences.

## Acceptance and delivery

Use a seeded existing Puerto Rico sea state with another active ship. Complete
deck → launch → rowing → pier → original NPC dialogue/quest/trade → guard combat
→ launch → the same live ship → naval attack on real guards and fort retaliation.
Observe the complete scene, continuous camera/hero ownership and retained fleet.
Then restore stable travel states through normal save/load, including old sea
and old San Juan land saves. Verify pause/interface resume, protected and
wall-occluded targets, locked/night doors, unsafe boarding, unrelated ordinary
sailing/deck/boarding and intentional region departure.

Source, script VM, asset and native component checks prove only their stated
properties. Shared build/staging/app delivery remains serialized by the native
integration owner. The installed-game journey is unresolved until replayed.

## Implemented source boundaries, pending scene replay

`PROGRAM/sea_ai/PortRegion.c` owns region preparation, hero mode, input/camera
handoff and supported reload interception. `LocPrepareResident` prepares original
Location/LAi state without taking ownership of the hero or naval environment;
`LocActivateResident`, `LocSuspendResident` and `LocRetireResident` separate entry,
departure and actual teardown. `PortSave.c` retains the existing naval save blob
and logical land/boat state. Saved-population reconstruction borrows the normal
load flag so dead NPCs are not resurrected by deferred preparation. That marker
is consumed after successful preparation, not kept as a permanent generation ban.

Native `PortEntityRoles` changes only the canonical script object's EID when
switching its ship/Character role; assigning an object would copy its attributes
and is therefore unsuitable. Recognized ship messages reaching the land hero
are forwarded to the retained ship with the same canonical attributes. Script
land-target admissions use `Character::ResolveScriptTarget`: live Character and,
when a source is supplied, the same real Location. A SHIP is never coerced to a
Character. Actor-specific script callbacks borrow their real Location reference
and capture state; guarded restoration does not change camera or active context.

`PORT_ROWBOAT` owns transient hero/oars/bench and receiver-relative ladder motion.
Its message operations configure, begin, request exit, cancel, release the visual
hero and restore. Travel descriptors are ordinary saved attributes; the existing
binary deck-camera layout is unchanged. Authored rig/action and oar phase inputs
come from `src/assets/port-rowboat/layout.json` through
`tools/prepare_port_rowboat.py`. Unsupported rigs or unsafe contacts reject the
transfer while retaining the prior state.

The visible San Juan static geometry remains the sea island's live MODEL
receivers. Resident Location reuses those receivers for collision and shadows;
duplicating its static town meshes would cause overlap and stale uncut walls.
Location still owns transformed locators, PTC navigation, grass and animated
props. Artillery contact dispatch stays port-only; original LAi/crime scripts
own HP, death, quest protection and fort hostility, while transient gore retains
the searchable canonical corpse. Deferred-erased contexts and actors are excluded
from role, target and gore lookups. A resident hero ashore/in a boat is separate
from the ship's native sinking lifecycle; ordinary aboard death is preserved.

Known remaining scene boundaries: authored PTC support is not regenerated after
arbitrary floor destruction; global land fauna creation is skipped during hidden
preparation. Full travel geometry, rowing contacts, lighting, original dialogue/
trade/quests and real naval retaliation still require the integrated native run.

The retained private accepting case is
`.local-archives/seamless-port/fullscene-20261016/README.md`: one real PROGRAM,
generated city/rowing inputs, independent userdata and four continued journey
phases. CPU parsing/syntax is preparation evidence only. Native context component
inputs and results are retained at
`.local-archives/seamless-port/native-context-20261016/receipt.json`.
SAVE compatibility evidence and old-writer seeds are retained at
`.local-archives/seamless-port/save-reference-remap-20261016/evidence.json`;
the compiler remap remains a candidate until its actual decoder replay passes.
The exact source integration handoff (owned patch, hashes and final helper syntax)
is retained at `.local-archives/seamless-port/integration-20261016/`; it excludes
the independently owned laser header and is not permission to stage an untested
port into the player application.
