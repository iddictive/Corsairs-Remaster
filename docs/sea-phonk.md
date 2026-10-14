# Phonk Mode

## Contract and owners

At local sea, **Esc → Phonk Mode** sits directly below **Цунами**. The same
button turns it off and resumes play. Map, land and boarding retain their menus.
The player's ship gains roughly twentyfold target speed, rapid acceleration,
inertial drift and lilac illumination. The hull turns while world-space momentum
keeps carrying it sideways; countersteering and recovering traction straighten
the course. NPC ships and ordinary off-mode physics are unchanged.
Grounding, collision, death, stopped/fixed/surrendered ships retain their guards.

`PROGRAM/scripts/sea_phonk.c` owns transient `Sea.Phonk.Active` and one looped
music voice, using the existing music-volume bus. `PROGRAM/sound.c` routes
ordinary playlist requests through that owner while active and retains the latest
requested ordinary track for restoration. Sea teardown/initialization, boarding
and death dispose the mode. No separate setting, permanent perk or save layout
is introduced. Native motion/light are owned by `ship-phonk.patch`, appended
to the existing ordered native patch stack.

The strongest existing facilities are the native transient drift/collision path,
ship point-light rendering, and script music bus. Reuse them; no replacement
physics engine, renderer, playlist scheduler or UI shell is required.

## Music input

The player selected any popular phonk song. This implementation uses
[RAIZHELL — Faster](https://ncs.io/Faster), an official
NCS drift-phonk release. `src/assets/audio/manifest.json` binds its direct
official download, exact bytes and music consumer. `tools/prepare_phonk_music.py`
reuses verified local bytes; canonical staging fetches the input when absent.
The downloaded audio is ignored, while the reproducible recipe is versioned.
The installed app retains the user's downloaded input; the source repository
does not redistribute the recording.

NCS creator-video terms are not a game redistribution licence. A public game
archive containing this recording requires the label's game licence; this
personal installation does not establish that licence. See the
[official usage policy](https://ncs.io/usage-policy). No paid licence is purchased.

## Acceptance

Compile the complete PROGRAM/menu chain against the native script VM, check
on/off plus land/boarding/dead rejection and restored music. Native drift must
show lateral movement after a turn and retain exact ordinary-off behavior.
Canonical `run.sh --stage-only` must consume the final patch and music input.
Installed menu, real ship movement and lilac appearance/music require an actual
sea replay; source/probe/build/staging results remain separate evidence.

Current attempt disposition and installed receipt belong to `runtime.md`.
