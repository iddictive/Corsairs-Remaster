#!/usr/bin/env python3
"""Deliver complete Git-owned gameplay files and prepared assets."""
import argparse
import hashlib
import json
import subprocess
from contextlib import nullcontext
from pathlib import Path

import changelog_content
import gameplay_sources
import metal_engine_source
import prepare_native_movies
import prepare_puerto_rico_city
import prepare_port_rowboat
from delivery_state import DeliveryState, RECEIPT, player_guard, transact

PROJECT = Path(__file__).resolve().parents[1]
UI_ASSETS = PROJECT / "src/assets/ui"
CHARACTER_ASSETS = PROJECT / "src/assets/characters/red-beard"
RECIPE_ASSETS = PROJECT / "src/assets/quests/great-recipe"
MUSIC_ASSETS = PROJECT / "src/assets/audio"
METAL = PROJECT / "experiments/native-metal"
TARGET = METAL / ".cache/runtime"
INSTALLED_APP = Path("/Applications/Corsairs Iddictive Remaster.app")
ENGINE = METAL / ".cache/CorsairsMetal.app/Contents/MacOS/metal-engine"


def digest(data):
    return hashlib.sha256(data).hexdigest()


def compiler_ready():
    built = METAL / ".cache/build/bin/engine-1"
    return metal_engine_source.ready(built, ENGINE)


def source_changes(target_root, source_set, state=None, managed=None):
    delivery = gameplay_sources.prepare_delivery(target_root, source_set, state)
    if managed is not None:
        managed.update({name: delivery[target_root / name] for name in source_set[0]})
    return {path.relative_to(target_root).as_posix(): (before, after)
            for path, (before, after) in delivery.items()
            if path.is_relative_to(target_root) and path.name != RECEIPT and before != after}


def content_sources(source_set, paths=None):
    sources, inputs = source_set
    if paths is not None:
        return source_set
    # Authored documentation is projected to native text; no script is rewritten.
    generated, snapshots = changelog_content.read()
    if sources.keys() & generated.keys():
        raise RuntimeError("Conflicting generated content source; no files changed")
    return {**sources, **generated}, {**inputs, **snapshots}


def asset_changes(asset_root, state, managed=None, inputs=None, claimed=None):
    manifest = asset_root / "manifest.json"
    raw = manifest.read_bytes()
    record = json.loads(raw)
    if set(record) != {"version", "files"} or record["version"] != 1 or not isinstance(record["files"], dict):
        raise RuntimeError(f"Invalid asset manifest: {manifest}")
    if inputs is not None:
        if manifest in inputs and inputs[manifest] != (raw, raw):
            raise RuntimeError("Concurrent asset manifest; no files changed")
        inputs[manifest] = (raw, raw)
    changes = {}
    for name, spec in record["files"].items():
        if claimed is not None:
            if name in claimed:
                raise RuntimeError("Asset manifests overlap; no files changed")
            claimed.add(name)
        data = None
        for key, checksum in (("source", "source_sha256"), ("prepared", "sha256")):
            path = gameplay_sources.safe_path(asset_root, Path(spec[key]))
            data = path.read_bytes()
            if digest(data) != spec[checksum]:
                raise RuntimeError(f"Unreviewed asset: {name}")
            if inputs is not None:
                if path in inputs and inputs[path] != (data, data):
                    raise RuntimeError("Concurrent asset input; no files changed")
                inputs[path] = (data, data)
        target = state.path(name)
        original = target.read_bytes() if target.exists() else None
        legacy = (spec["original_sha256"],) if "original_sha256" in spec else ()
        state.admit(name, original, data, legacy=legacy, create=True)
        state.track(name, data)
        if managed is not None:
            managed[name] = (original, data)
        if original != data:
            changes[name] = (original, data)
    return changes


def plan(target_root=None, paths=None, source_set=None, state=None, managed=None, inputs=None):
    target_root = TARGET if target_root is None else target_root
    source_set = content_sources(gameplay_sources.read(paths) if source_set is None else source_set, paths)
    app = target_root.parent.parent if target_root.name == "Resources" and target_root.parent.name == "Contents" else None
    state = DeliveryState(target_root, app) if state is None else state
    if state.resources != target_root or state.app != app:
        raise RuntimeError("Gameplay receipt belongs to another runtime")
    changes = source_changes(target_root, source_set, state, managed)
    if inputs is not None:
        inputs.update(source_set[1])
    if paths is not None:
        return changes
    variants = gameplay_sources.read_variants()
    for name in variants[0]:
        target = state.path(name)
        if not target.is_file():
            raise RuntimeError(f"Missing gameplay variant: {name}")
        before = target.read_bytes()
        after, identity, snapshots = gameplay_sources.variant_source(name, before, state, variants)
        legacy = variants[0][name]["sources"][identity]
        encoding = variants[0][name]["encoding"]
        admitted = before
        if name not in state.files:
            normalized = gameplay_sources.runtime_bytes(name, gameplay_sources.source_bytes(name, before, encoding), encoding)
            if digest(normalized) in legacy:
                admitted = normalized
        equivalent = gameplay_sources.source_bytes(name, before, encoding) == gameplay_sources.source_bytes(name, after, encoding)
        state.admit(name, admitted, after, legacy, development=equivalent)
        state.track(name, after, owner=state.files.get(name, {}).get("owner", "stage"), source=identity)
        if managed is not None:
            managed[name] = (before, after)
        if inputs is not None:
            inputs.update(snapshots)
        if before != after:
            changes[name] = (before, after)
    asset_targets = set()
    for asset_root in (UI_ASSETS, CHARACTER_ASSETS, RECIPE_ASSETS, MUSIC_ASSETS):
        changes.update(asset_changes(asset_root, state, managed, inputs, asset_targets))
    city = prepare_puerto_rico_city
    recipe_inputs = {path: path.read_bytes() for path in (
        city.LAYOUT, city.MANIFEST, Path(city.__file__),
        Path(city.__file__).with_name("puerto_rico_city_blocks.py"))}
    city_specs = json.loads(recipe_inputs[city.MANIFEST])["files"]
    rowboat = prepare_port_rowboat
    recipe_inputs.update({path: path.read_bytes() for path in (
        rowboat.LAYOUT, rowboat.MANIFEST, Path(rowboat.__file__))})
    rowboat_specs = json.loads(recipe_inputs[rowboat.MANIFEST])["files"]
    if inputs is not None:
        inputs.update({path: (raw, raw) for path, raw in recipe_inputs.items()})
    for owner, prepared, specs in (
        ("src/assets/video/manifest.json", prepare_native_movies.prepared(), {}),
        ("src/assets/islands/puerto-rico/manifest.json", city.prepared(), city_specs),
        ("src/assets/port-rowboat/manifest.json", rowboat.prepared(), rowboat_specs),
    ):
        for relative, source in prepared.items():
            target = state.path(relative)
            previous = target.read_bytes() if target.is_file() else None
            incoming = source.read_bytes()
            spec = specs.get(relative, {})
            if spec and digest(incoming) != spec["sha256"]:
                raise RuntimeError(f"Concurrent prepared asset change: {relative}")
            legacy = (spec["original_sha256"],) if "original_sha256" in spec else ()
            state.admit(relative, previous, incoming, legacy=legacy, create=True)
            state.track(relative, incoming, source=owner)
            if managed is not None:
                managed[relative] = (previous, incoming)
            if inputs is not None:
                inputs[source] = (incoming, incoming)
            if previous != incoming:
                changes[relative] = (previous, incoming)
    gameplay_sources.verify_layout_assets(target_root, inputs)
    return changes


def delivery_content(target_root, sources=None):
    source_set = gameplay_sources.read()
    managed = {}
    app = target_root.parent.parent if target_root.name == "Resources" and target_root.parent.name == "Contents" else None
    state = DeliveryState(target_root, app)
    if plan(target_root, source_set=source_set, managed=managed, state=state):
        raise RuntimeError("Gameplay is not staged; run sync_metal_gameplay.py apply")
    if sources is not None:
        sources.update({name: row["source"] for name, row in state.files.items()
                        if "source" in row and name in managed})
    return {name: after for name, (_, after) in managed.items()} | {
        name: row[0] for name, row in source_set[0].items()}


def apply(changes, paths=None, source_set=None):
    guard = player_guard(INSTALLED_APP) if INSTALLED_APP.is_dir() else nullcontext()
    with guard:
        apply_locked(changes, paths, source_set)


def apply_locked(changes, paths=None, source_set=None):
    if not compiler_ready():
        raise RuntimeError("build and stage Metal with the current engine sources before applying gameplay")
    app_resources = INSTALLED_APP / "Contents/Resources"
    source_set = gameplay_sources.read(paths) if source_set is None else source_set
    runtime_state = DeliveryState(TARGET)
    app_state = DeliveryState(app_resources, INSTALLED_APP) if INSTALLED_APP.is_dir() else None
    runtime_managed, app_managed, variant_inputs = {}, {}, {}
    if plan(paths=paths, source_set=source_set, state=runtime_state, managed=runtime_managed, inputs=variant_inputs) != changes:
        raise RuntimeError("Concurrent gameplay plan change; no files changed")
    if app_state is not None:
        plan(app_resources, paths, source_set, app_state, app_managed, variant_inputs)
    engines = [ENGINE]
    if INSTALLED_APP.is_dir():
        engines.append(INSTALLED_APP / "Contents/MacOS/metal-engine")
    holders = subprocess.run(["/usr/sbin/lsof", "-t", "--", *map(str, engines)],
                             capture_output=True, text=True, check=False)
    if holders.returncode != 1 or holders.stdout or holders.stderr:
        raise RuntimeError("close the Metal game before applying gameplay")
    batch = {**source_set[1], **variant_inputs}
    for relative, (previous, incoming) in runtime_managed.items():
        path = TARGET / relative
        batch.update(gameplay_sources.backup_change(previous, incoming))
        batch[path] = (previous, incoming)
    for relative, (previous, incoming) in app_managed.items():
        batch.update(gameplay_sources.backup_change(previous, incoming))
        batch[app_resources / relative] = (previous, incoming)
    # plan already admitted canonical files against the loaded receipt. Reuse
    # that proposal instead of re-admitting old bytes against future hashes.
    batch[runtime_state.receipt] = runtime_state.change()
    if app_state is not None:
        batch[app_state.receipt] = app_state.change()

    def verify():
        if plan(paths=paths, source_set=source_set):
            raise RuntimeError("post-apply gameplay comparison failed")
        if app_state is not None and plan(app_resources, paths, source_set):
            raise RuntimeError("post-apply installed gameplay comparison failed")

    transact(batch, INSTALLED_APP if app_state is not None else None, verify)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "apply"), default="check", nargs="?")
    parser.add_argument("--path", action="append", help="Deliver only this registered canonical gameplay source (repeatable).")
    args = parser.parse_args()
    try:
        source_set = gameplay_sources.read(args.path)
        changes = plan(paths=args.path, source_set=source_set)
        ready = compiler_ready()
        print(f"Metal gameplay: {len(changes)} files pending; compiler {'ready' if ready else 'build/stage pending'}")
        for path in sorted(changes):
            print(path)
        if INSTALLED_APP.is_dir():
            installed_changes = plan(INSTALLED_APP / "Contents/Resources", args.path, source_set)
            print(f"Installed Metal gameplay: {len(installed_changes)} files pending")
            for path in sorted(installed_changes):
                print(path)
        if args.action == "apply":
            apply(changes, args.path, source_set)
            print("Applied to native-metal. SAVE, config and renderer resources unchanged.")
        return 0
    except (RuntimeError, OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"error: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
