# Deck heel and falls — 2026-11-05

This evidence covers the native deck-camera and ambient-sailor owners without
launching or operating the game. [replay.py](replay.py) builds private copies of
the changed native modules, runs the supplied fixtures, and compiles the current
gameplay script chain. It does not modify the shared build or installed app.

```sh
deck_scratch="$(mktemp -d /tmp/corsairs-deck-replay.XXXXXX)"
TASK_SCRATCH_DIR="$deck_scratch" python3 docs/evidence/deck-heel-20261105/replay.py
/Users/md/.codex/bin/codex-delete-temp "$deck_scratch"
```

The [recorded output](result.txt) was produced on an Apple M5 Max with Apple
clang 21, at 16 ms simulated steps. The native fixtures use the real `MODELR`
and Brig1 geometry, with a flat test sea and test ship/control objects. The deck
owner moves 0.186 m locally over 1.92 s at a 0.30 rad heel, stays aboard at that
heel, detaches at 1.22 rad, continues after a `CSaveLoad` round trip, and posts
one water event. It also checks landing on the stabilised deck and character
replacement without a death event. The NPC fixture exercises the real `ShipWalk`
owner and a loaded low-detail sailor model. Its incidental `npc_40_frame_ms`
line is an upright, post-recovery observation; use the separate performance
fixture for the comparative results below.

The comparative fixture measures the same `ShipWalk` update, position and
separation loop with and without the new step. It uses real Brig1 collision
geometry, but does not render or animate 40/200 sailor models. Values are CPU
milliseconds per simulated frame; mode 0 is upright, mode 1 oscillates through
ordinary heel, and mode 2 is critical heel.

| NPCs | Mode | Baseline p95 / p99 | Changed p95 / p99 |
| ---: | --- | ---: | ---: |
| 40 | upright | 0.0012 / 0.0013 | 0.0014 / 0.0015 |
| 40 | ordinary | 0.0012 / 0.0012 | 0.0097 / 0.0103 |
| 40 | critical | 0.0013 / 0.0013 | 0.0219 / 0.0241 |
| 200 | upright | 0.0262 / 0.0325 | 0.0272 / 0.0332 |
| 200 | ordinary | 0.0258 / 0.0294 | 0.0661 / 0.0701 |
| 200 | critical | 0.0250 / 0.0253 | 0.1209 / 0.1375 |

The script fixture executes the current `DeckFall_Water` and `GameOverE`
bodies with bounded substitutes for quest/death side effects. It checks ordinary
death, the GreatRecipe claim, the GhostCapt branch, scene exit, and duplicate
water events. The separate full-chain compile passes without script errors.
These fixtures do not prove actual quest transitions, a full game save/restart,
rendered animation, or FPS in a battle; those require the consuming game runtime.

The replay uses the prepared `.cache/build` and `.cache/runtime` inputs selected
by the existing native workflow. The tested Brig1 hull hash is
`0dec3e566390bd410486861ad58844b7589f34684fe87520f1568b5e9a93eaec`;
the corresponding frozen hull is under
`experiments/native-metal/inputs/native/puerto-rico-city-originals/RESOURCE/MODELS/Ships/Brig1/Brig1.gm`.
The tested Brig1 path, low-detail sailor and animation are sourced by the
existing preparation workflow from the installed Corsairs app; their SHA-256
values were respectively `aeb8748bc2d4c1c34fbfb0066649d129c0db779046a1b0d6375c28dadac50137`,
`65ed0c8a651d470d64a055d4569cf24a01b49846d42218ee4048165581b3c891`, and
`a4c11f27af2c9d353c222e51f7229d70a3ed31a015ad4d7aa1cae416d0c708ec`.
Those installed assets are an external input and must still be present to replay.
