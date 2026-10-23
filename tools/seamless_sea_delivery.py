"""Join prepared sea resources into the ordinary atomic engine installation.

Generators retain inactive inputs. Installation explicitly activates the joined
layout only alongside its engine, scripts, native depth and route tables.
"""
from __future__ import annotations

import json
from pathlib import Path

from delivery_state import digest, safe_path
import gameplay_sources
import metal_engine_source
from prepare_seamless_sea_assets import OUTPUT, OWNER


SCRIPTS = tuple("PROGRAM/" + name for name in (
    "battle_interface/BattleInterface.c", "scripts/companions.c",
    "scripts/islandships.c", "sea_ai/AIFantom.c", "sea_ai/AIFort.c",
    "sea_ai/AISea.c", "sea_ai/AIShip.c", "sea_ai/direct_sailing.c",
    "sea_ai/sea.c", "sea_ai/sea_traffic.c", "worldmap/worldmap.c",
    "worldmap/worldmap_coords.c", "worldmap/worldmap_encgen.c",
    "worldmap/worldmap_events.c", "worldmap/worldmap_globals.c",
    "worldmap/worldmap_reload.c",
))


def prepare(state, engine: Path):
    if not metal_engine_source.ready(engine, engine):
        raise RuntimeError("Seamless sea needs the completed canonical engine build")
    inputs = {}

    def read(root, name):
        path = safe_path(root, Path(name))
        data = path.read_bytes()
        inputs[path] = (data, data)
        return data

    raw = read(OUTPUT, "manifest.json")
    bodies = json.loads(raw)
    chart_raw = read(OUTPUT, "chart/manifest.json")
    graph_raw = read(OUTPUT, "flow-graphs.json")
    chart, graphs = json.loads(chart_raw), json.loads(graph_raw)
    if (digest(read(OWNER, "recipe.json")) != bodies["recipe_sha256"] or
            digest(read(OWNER, "manifest.json")) != bodies["inventory_sha256"]):
        raise RuntimeError("Prepared sea no longer matches the authored recipe")
    joined = bodies["runtime_layout"]
    if (joined["chart_manifest_sha256"] != digest(chart_raw) or
            joined["flow_graph_manifest_sha256"] != digest(graph_raw)):
        raise RuntimeError("Prepared sea consumers changed; rejoin the runtime layout first")
    for consumer in (chart, graphs, bodies["navigation"]):
        if any(consumer[key] != bodies[key] for key in ("recipe_sha256", "atlas_frames_sha256")):
            raise RuntimeError("Prepared sea consumers have different coordinate frames")

    assets = {name: row for name, row in bodies["files"].items()
              if name.startswith("RESOURCE/MODELS/SeamlessSea/")}
    assets.update({name: row for name, row in chart["assets"].items() if name.startswith("RESOURCE/")})
    assets[chart["files"]["model"]["path"]] = chart["files"]["model"]
    navigation = bodies["navigation"]
    patch = navigation["global"]["patch"]
    assets[patch["path"]] = patch
    for body in navigation["bodies"].values():
        assets.update({row["path"]: row for row in body["outputs"]})
    for body in graphs["bodies"].values():
        row = body["output"]
        assets[row["path"]] = row
    layout = joined["input"]
    assets[layout] = bodies["files"][layout]

    # Textures are existing native resources, not generated atlas variants.
    # Admit their exact installed bytes without replacing player modifications.
    for name, row in bodies["texture_inputs"].items():
        data = read(state.resources, name)
        if digest(data) != row["sha256"]:
            raise RuntimeError(f"Installed atlas texture differs from its prepared source: {name}")

    changes = dict(inputs)
    for name, row in sorted(assets.items()):
        data = read(OUTPUT, name)
        if digest(data) != row["sha256"]:
            raise RuntimeError(f"Prepared atlas asset changed: {name}")
        if name == layout:
            marker = b"runtime_activation=0\n"
            if data.count(marker) != 1:
                raise RuntimeError("Prepared atlas activation is not explicit and inactive")
            data = data.replace(marker, b"runtime_activation=1\n")
        target = state.path(name)
        before = target.read_bytes() if target.exists() else None
        legacy = (navigation["stock_navigation"]["sha256"],) if name == patch["path"] else ()
        state.admit(name, before, data, legacy, create=True)
        state.track(name, data)
        changes.update(gameplay_sources.backup_change(before, data))
        changes[target] = (before, data)
    source_set = gameplay_sources.read(SCRIPTS)
    for name, (incoming, legacy, _) in source_set[0].items():
        target = state.path(name)
        before = target.read_bytes() if target.exists() else None
        previous = state.files.get(name)
        if previous is None or (before is not None and digest(before) == previous["sha256"]):
            continue
        # The isolated played build restored these known pre-atlas scripts,
        # while its receipt retained the canonical staged projection. Resolve
        # that exact predecessor pair; never admit arbitrary installed edits.
        staged = read(metal_engine_source.METAL / ".cache/runtime", name)
        if (previous["owner"] != "stage" or previous.get("source") is not None or
                (before is not None and digest(before) not in legacy) or digest(staged) != previous["sha256"]):
            raise RuntimeError(f"Unrecognized installed script change: {name}")
        changes.update(gameplay_sources.backup_change(staged, incoming))
        if before is None:
            del state.files[name]
        else:
            state.track(name, before)
    changes.update(inputs)
    changes.update(gameplay_sources.prepare_delivery(state.resources, source_set, state=state))
    return changes
