# WorldMap Forest Layer Leak Repair (2026-10-23)

## User Report Input
- **Artifact**: `user-report.png`
- **SHA-256**: `6852b43a3479c60b8ec8f2386ef24095828cf3628d76af148fdf2e2ecc8651bb`
- **Observed Symptoms**: Global chart view (Puerto Rico — San Martin — Nevis). Floating black unlit tree silhouette in the ocean immediately east of Puerto Rico/Fajardo (and partial silhouette southwest), consistent with a leaked physical forest entity rendered under the global map camera.

## Proven Causal Chain & Source Owners
1. **Creation defaults**:
   - In `experiments/native-metal/engine/src/libs/location/src/grass.cpp:244-247` (`Grass::Init`), the entity unconditionally registers itself in core layers:
     ```cpp
     core.AddToLayer(EXECUTE, GetId(), 1000);
     core.AddToLayer(REALIZE, GetId(), 1000);
     ```
   - Land locations (`Location::LoadGrass`, `location.cpp:845-865`) rely on these default layers (`EXECUTE` / `REALIZE`) throughout location lifetime and destroy the entity upon leaving via `core.EraseEntity(grass)` (`location.cpp:303`).

2. **Sea creation leak**:
   - In `src/gameplay/PROGRAM/sea_ai/sea.c:1789-1791` (`Sea_CreateRegionGrass`), sea grass was created and added to sea layers:
     ```c
     CreateEntity(grass, "Grass");
     LayerAddObject(SEA_EXECUTE, grass, 1000);
     LayerAddObject(SEA_REALIZE, grass, 1000);
     ```
     Because `LayerDelObject(EXECUTE, grass)` and `LayerDelObject(REALIZE, grass)` were missing, the entity retained active membership in both layer pairs simultaneously.

3. **Sea vs WorldMap layer state**:
   - In normal sea mode (`sea.c:278-293`), `EXECUTE` and `REALIZE` are explicitly frozen:
     ```c
     LayerFreeze(REALIZE, true);
     LayerFreeze(EXECUTE, true);
     LayerFreeze(SEA_EXECUTE, false);
     LayerFreeze(SEA_REALIZE, false);
     LayerSetExecute(SEA_EXECUTE);
     LayerSetRealize(SEA_REALIZE);
     ```
     Thus, while membership was duplicate, the base layers were inactive, avoiding double execution during ordinary sailing.
   - When entering the global map with resident sea (`Sea_ResidentSuspend()`, `sea.c:474-479`), sea layers are frozen while `EXECUTE` and `REALIZE` are unfrozen for `worldmap`:
     ```c
     LayerFreeze(SEA_EXECUTE, true);
     LayerFreeze(SEA_REALIZE, true);
     LayerFreeze(EXECUTE, false);
     LayerFreeze(REALIZE, false);
     LayerSetExecute(EXECUTE);
     LayerSetRealize(REALIZE);
     ```
     Because `Grass` retained membership in `EXECUTE` and `REALIZE`, `Grass::Realize` was executed on every WorldMap frame, resolving the resident `ISLAND` entity and dispatching `islandForest->render` into the world map render pass with world map camera and projection matrices.

## Two-Line Product Fix
In `src/gameplay/PROGRAM/sea_ai/sea.c` (`Sea_CreateRegionGrass`), transfer ownership to the sea layers, matching the established layer-transfer pattern used across weather entities (`WhrSky`, `WhrRain`, `WhrLightning`, `WhrSun`):
```c
    CreateEntity(grass, "Grass");
    LayerDelObject(EXECUTE, grass);
    LayerDelObject(REALIZE, grass);
    LayerAddObject(SEA_EXECUTE, grass, 1000);
    LayerAddObject(SEA_REALIZE, grass, 1000);
```

## Acceptance Status
- **Source contract**: Accepted by direct source contract. Land location defaults in `location.cpp` remain unchanged; sea grass ownership is cleanly confined to `SEA_EXECUTE` / `SEA_REALIZE`.
- **Runtime visual verification**: Manual global chart visual acceptance pending player replay (game not launched; zero unmeasured claims).
