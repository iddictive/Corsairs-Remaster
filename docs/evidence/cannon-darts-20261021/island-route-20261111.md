# Native island and fort projectile route, 2026-11-11

`island-route-replay.py` reuses the native fleet fixture and cached
`sea-fleet-benchmark`. It registers an actual `ISLAND`, Nevis `MODELR`, `AIFort`,
fort model and blots, then fires ordinary balls through the real `AIBalls`
consumer. Fort cannon type -1 avoids firing guns in this collision fixture.
It uses an isolated scratch user-data directory and never launches the game.

The two aimed points are centroids of authored visible Nevis GM triangles:
island (-107.11629, 47.83599, -576.21643), fort (-1073.08736, 27.80720,
-634.23539). The shot starts 2 m before each point along +Z, with Y raised
slightly to account for gravity. The third shot is deliberately far from land,
forts and ships; it remains active at the observation. The fourth hits water.
The 20 mm bounds in the replay check the selected segment's expected hit
coordinates, not a gameplay hull-penetration tolerance.

Run from the repository root:

```sh
~/.codex/bin/codex-run-scratch run --max-bytes 2147483648 --max-files 15000 --seconds 180 --reserve-bytes 10737418240 -- /usr/bin/python3 docs/evidence/cannon-darts-20261021/island-route-replay.py
```

Observed `native_exit=0`: island=1 at (-107.116, 47.837, -576.215), fort=1 at
(-1073.087, 27.807, -634.235), water=1, miss-active=1, unexpected ship=0.
The generated script SHA-256 was
`35b2fb44a7ae8e97b0c50f9ceec3eaa92f9073a177e6ead931c1c4f9ee6c5bad`.
Cached benchmark SHA-256 was
`9af19db386bcd8cf1a32f0b52628bea849e7bdd4c0914ec1c968a59fab20f979`.
Nevis, fort and fort-locators GM SHA-256 respectively:
`90456f323bead05e8675eb206637c15ddf8a40ea1c4e33dfa28c9b73e87c1b95`,
`c69680cd085c1f499985a7e510d47e87c34ff0f3927515581303623434b7900c`,
`ef5094091b45832288fb9d7406c15feaf6b8271f8c14505e10eeda309c335e2a`.

These are four selected segments, not the full island/fort geometry, player's
shot trajectories, atlas aggregation, all ammunition types or gameplay FPS.
The cached diagnostic binary is not proven to represent the current dirty
source; the replay does not establish that a new integrated build behaves so.
Its cached binary and runtime assets are required inputs and are not retained
by this evidence. An initial fort bootstrap lacked `Ship.Cannons.Charge` and
crashed in `FortCannonContext::Parameters::Read`; adding that existing required
fixture attribute produced the successful run above. No product fix is inferred.
