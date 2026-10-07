#!/usr/bin/env python3
"""Deliver canonical gameplay sources and the remaining reviewed legacy layers."""

import argparse
import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path

import patch_gameplay_suite as suite
import metal_deck_camera as deck_camera
import metal_deck_controls as deck_controls
import patch_custody_life as custody_life
import metal_pickup_glow as pickup_glow
import patch_squad_common_supply as squad_supply
import metal_governor_dialog as governor_dialog
import metal_living_caribbean as living_caribbean
import gameplay_sources
import metal_fleet_gameplay as fleet_gameplay
import metal_fleet_sea as fleet_sea
import metal_fleet_ui as fleet_ui
import metal_military_integration as military_callbacks
from patch_mod_journal import UPDATED_SHA256
from patch_sea_battle_mode_reset import UPDATED_SHA256 as COMBAT_UPDATED_SHA256
from delivery_state import DeliveryState, RECEIPT, player_guard, transact
from contextlib import nullcontext

PROJECT = Path(__file__).resolve().parents[1]
UI_ASSETS = PROJECT / "src/assets/ui"
# Portable input is a reviewed read-only baseline, never a second game runtime.
INPUTS = PROJECT / "experiments/native-metal/inputs"
if (INPUTS / "manifest.json").is_file():
    suite.NATIVE_ROOT = INPUTS / "gameplay"
    suite.SNAPSHOT_ROOT = INPUTS / "gameplay-originals"
SOURCE = suite.NATIVE_ROOT
METAL = PROJECT / "experiments/native-metal"
TARGET = METAL / ".cache/runtime"
INSTALLED_APP = Path("/Applications/Corsairs Iddictive Remaster.app")
ENGINE = METAL / ".cache/CorsairsMetal.app/Contents/MacOS/metal-engine"
PATCH = PROJECT / "experiments/native-storm/compiler-extern.patch"
DECK_PATCHES = (PATCH.with_name("deck-walk.patch"),)
ENGINE_DECK_PATCH = METAL / "deck-walk.patch"
BACKGROUND_ALPHA = METAL / "background_alpha.py"
# Independently delivered cannon aiming; this package does not replace it.
PRESERVED_RUNTIME = {
    "PROGRAM/sea_ai/AICannon.c": "b013ee3296b62e0b71c26f5a22b8b2b0c7601ba30b4e281a4162246783b00229",
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def script_bytes(relative, data, *representations):
    # The VM treats LF and CRLF identically; compare the exact reviewed CRLF
    # representation without accepting any other script change.
    if relative.endswith(".c"):
        normalized_lf = data.replace(b"\r\n", b"\n")
        for representation in representations:
            if normalized_lf == representation.replace(b"\r\n", b"\n"):
                return representation
    return data


def deck_package():
    spec = importlib.util.spec_from_file_location("native_deck_walk", PATCH.with_name("deck_walk.py"))
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def background_alpha_package():
    spec = importlib.util.spec_from_file_location("metal_background_alpha", BACKGROUND_ALPHA)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def compiler_ready():
    receipt = METAL / ".cache/gameplay-compiler.json"
    if not receipt.is_file() or not ENGINE.is_file():
        return False
    record = json.loads(receipt.read_text())
    built = METAL / ".cache/build/bin/engine-1"
    return (record.get("patch_sha256") == digest(PATCH.read_bytes())
            and built.is_file()
            and record.get("engine_sha256") == digest(built.read_bytes())
            and record["engine_sha256"] == digest(ENGINE.read_bytes())
            and record.get("gameplay_patches") == {
                patch.name: digest(patch.read_bytes()) for patch in DECK_PATCHES
            }
            and record.get("engine_deck_patch_sha256") == digest(ENGINE_DECK_PATCH.read_bytes())
            and record.get("sea_surrender_patch_sha256") == digest((METAL / "sea-surrender-relations.patch").read_bytes())
            and record.get("cannon_loaded_patch_sha256") == digest((METAL / "cannon-loaded-count.patch").read_bytes())
            and record.get("mast_repair_patch_sha256") == digest((METAL / "mast-repair.patch").read_bytes())
            and record.get("sea_contact_patch_sha256") == digest((METAL / "sea-contact-activity.patch").read_bytes())
            and record.get("pickup_patch_sha256") == digest((METAL / "pickup-glow.patch").read_bytes())
            and record.get("external_url_patch_sha256") == digest((METAL / "external-url.patch").read_bytes()))


# Compose after the existing gameplay owners, notably the AIShip fleet layer.
FLEET_FINAL_BASE = {**fleet_sea.BASE_HASHES, **fleet_ui.BASES}
FLEET_FINAL_SHA = {'PROGRAM/worldmap/worldmap_reload.c': '82fa26766d7706d01b21716e52381115edf556764be7caf611779b06c44b3776',
 'PROGRAM/sea_ai/sea.c': '9937ba2362e2b55ca9b13adcb60dc0b4310c5a29470a1e58abda1a38a52a1a09',
 'PROGRAM/sea_ai/AIFantom.c': '94064aa548f1d3e41033c94ebe63dc823c664c1943852eadabff3d3ed6a75c0b',
 'PROGRAM/sea_ai/AIShip.c': '3f42a3c1c00db690ed91e83e78113a8ed4a12ed01e4e9a02ccf02e3842f78427',
 'PROGRAM/interface/map.c': 'd342e2cc54a3272c5b13998210fff578046b5f8f4db62e569e564af3457802cf',
 'PROGRAM/battle_interface/WmInterface.c': '4088a04e7f29758167938ec95530199bfd998cb7b37f01c67b939f4d973aac78',
 'PROGRAM/battle_interface/loginterface.c': '88a187aec46bf600ff6c239eb2d358b8155775a006f70d085917fa7bc79a849c',
 'RESOURCE/INI/interfaces/map.ini': '0edb623bf4ce8f01d815a80d62c66748eb7520ad133917f927d83b90c43cfafd'}


def prepare_fleet_final(relative, incoming):
    if relative in living_caribbean.PREPARERS:
        incoming = living_caribbean.prepare(relative, incoming)
    if relative in fleet_gameplay.FILES:
        incoming = fleet_gameplay.prepare(relative, incoming)
    if digest(incoming) != FLEET_FINAL_BASE[relative]:
        raise RuntimeError(f"unrecognized fleet bridge input: {relative}")
    result = fleet_ui.prepare(relative, fleet_sea.prepare(relative, incoming))
    if digest(result) != FLEET_FINAL_SHA[relative]:
        raise RuntimeError(f"unreviewed fleet bridge output: {relative}")
    return result


def source_changes(target_root, source_set, state=None, managed=None):
    delivery = gameplay_sources.prepare_delivery(target_root, source_set, state)
    if managed is not None:
        managed.update({name: delivery[target_root / name] for name in source_set[0]})
    return {path.relative_to(target_root).as_posix(): (before, after)
            for path, (before, after) in delivery.items()
            if path.is_relative_to(target_root) and path.name != RECEIPT and before != after}


def legacy_admission():
    record = json.loads((PROJECT / "tools/gameplay/delivery-bootstrap.json").read_bytes())
    if (not isinstance(record, dict) or set(record) != {"version", "files"}
            or type(record["version"]) is not int or record["version"] != 1
            or not isinstance(record["files"], dict)):
        raise RuntimeError("Invalid legacy gameplay admission")
    for name, hashes in record["files"].items():
        if not isinstance(hashes, list) or any(not isinstance(sha, str) or len(sha) != 64
                or any(c not in "0123456789abcdef" for c in sha) for sha in hashes):
            raise RuntimeError(f"Invalid legacy gameplay admission: {name}")
    return record["files"]


def ui_asset_changes(target_root, state, managed=None, inputs=None):
    manifest = UI_ASSETS / "manifest.json"
    raw = manifest.read_bytes()
    record = json.loads(raw)
    if set(record) != {"version", "files"} or record["version"] != 1 or not isinstance(record["files"], dict):
        raise RuntimeError("Invalid UI asset manifest")
    if inputs is not None:
        if manifest in inputs and inputs[manifest] != (raw, raw):
            raise RuntimeError("Concurrent UI asset manifest; no files changed")
        inputs[manifest] = (raw, raw)
    changes = {}
    for name, spec in record["files"].items():
        data = None
        for key, checksum in (("source", "source_sha256"), ("prepared", "sha256")):
            path = gameplay_sources.safe_path(UI_ASSETS, Path(spec[key]))
            data = path.read_bytes()
            if digest(data) != spec[checksum]:
                raise RuntimeError(f"Unreviewed UI asset: {name}")
            if inputs is not None:
                if path in inputs and inputs[path] != (data, data):
                    raise RuntimeError("Concurrent UI asset input; no files changed")
                inputs[path] = (data, data)
        target = state.path(name)
        original = target.read_bytes() if target.exists() else None
        state.admit(name, original, data, create=True)
        state.track(name, data)
        if managed is not None:
            managed[name] = (original, data)
        if original != data:
            changes[name] = (original, data)
    return changes


def plan(target_root=None, paths=None, source_set=None, state=None, managed=None, inputs=None):
    if target_root is None:
        target_root = TARGET
    source_set = gameplay_sources.read(paths) if source_set is None else source_set
    app = target_root.parent.parent if target_root.name == "Resources" and target_root.parent.name == "Contents" else None
    state = DeliveryState(target_root, app) if state is None else state
    if state.resources != target_root or state.app != app:
        raise RuntimeError("Gameplay receipt belongs to another runtime")
    canonical = source_changes(target_root, source_set, state, managed)
    if paths is not None:
        return canonical
    assets = ui_asset_changes(target_root, state, managed, inputs)
    living_caribbean.verify_fort_layout_assets(target_root)
    living_caribbean.verify_land_layout_assets(target_root)
    # Regenerated suite consumers come from exact-hash originals, not the
    # mutable carrier copy. Each provider and cumulative output is checked by
    # _build_outputs; unrelated incoming scripts still pass admission below.
    outputs = suite._build_outputs(suite._read_originals(SOURCE, "upgrade"))
    suite._verify_cross_feature_contracts(outputs)
    outputs = custody_life.transform(outputs)
    squad_inputs = {
        relative: outputs.get(relative, (SOURCE / relative).read_bytes())
        for relative in squad_supply.BASE
    }
    outputs.update(squad_supply.transform(squad_inputs))
    expected = {**suite.COMPOSITE_SHA256, **UPDATED_SHA256, **COMBAT_UPDATED_SHA256,
                **squad_supply.UPDATED}
    deck = deck_package()
    background_alpha = background_alpha_package()
    for relative in deck.PATCHES:
        incoming = (SOURCE / relative).read_bytes()
        reviewed, _ = deck.prepare(relative, incoming)
        if reviewed != incoming:
            raise RuntimeError("install the current deck-walk package in native-storm before Metal sync")
    bootstrap = legacy_admission()
    variants = gameplay_sources.read_variants()
    changes = {}
    # Compare all script consumers and interface INIs, not renderer materials.
    for directory in ("PROGRAM", "RESOURCE/INI"):
        for source in (SOURCE / directory).rglob("*"):
            if not source.is_file() or source.name == ".DS_Store":
                continue
            relative = source.relative_to(SOURCE).as_posix()
            if relative in source_set[0]:
                continue
            target = target_root / relative
            if source.is_symlink() or target.is_symlink():
                raise RuntimeError(f"refusing linked gameplay file: {relative}")
            if not target.is_file():
                raise RuntimeError(f"missing Metal gameplay file: {relative}")
            incoming = outputs.get(relative, source.read_bytes())
            original = target.read_bytes()
            preserved = original.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n") if relative.endswith(".c") else original
            if digest(preserved) == PRESERVED_RUNTIME.get(relative):
                continue
            variant = gameplay_sources.variant_source(relative, original, state, variants)
            if variant is not None:
                reviewed, identity, variant_inputs = variant
                if inputs is not None:
                    for path, pair in variant_inputs.items():
                        if path in inputs and inputs[path] != pair:
                            raise RuntimeError("Concurrent gameplay variant input; no files changed")
                        inputs[path] = pair
            elif relative in FLEET_FINAL_BASE:
                reviewed = prepare_fleet_final(relative, incoming)
            elif relative in fleet_gameplay.FILES:
                if relative in living_caribbean.PREPARERS:
                    incoming = living_caribbean.prepare(relative, incoming)
                incoming, _ = background_alpha.prepare_file(relative, incoming)
                reviewed = fleet_gameplay.prepare(relative, incoming)
            elif relative == governor_dialog.PATH:
                reviewed = governor_dialog.prepare(incoming)
            elif relative in living_caribbean.PREPARERS:
                reviewed = living_caribbean.prepare(relative, incoming)
            elif relative in deck_controls.PATHS:
                reviewed = deck_controls.prepare(relative, incoming)
            elif relative == deck_camera.PATH:
                reviewed = deck_camera.prepare(incoming)
            elif relative == pickup_glow.RELATIVE_PATH:
                reviewed = pickup_glow.prepare(incoming)
            elif relative in custody_life.BASE:
                reviewed = incoming
            elif relative in squad_supply.BASE:
                if digest(incoming) != squad_supply.UPDATED[relative]:
                    raise RuntimeError(f"unreviewed squad-supply output: {relative}")
                reviewed = incoming
            else:
                if (relative not in bootstrap
                        and relative not in deck.PATCHES and relative not in military_callbacks.HOOKS):
                    # No composer owns this file. Preserve runtime edits.
                    continue
                reviewed, _ = background_alpha.prepare_file(relative, incoming)
                if relative in expected and digest(incoming) != expected[relative]:
                    raise RuntimeError(f"unreviewed gameplay output: {relative}")
            if relative in military_callbacks.HOOKS:
                reviewed = (military_callbacks.transform(relative, reviewed) if variant is not None
                            else military_callbacks.prepare(relative, reviewed))
            if script_bytes(relative, original, reviewed) == reviewed:
                reviewed = original
            admitted = original if relative in state.files else script_bytes(relative, original, reviewed)
            if relative not in state.files and relative.endswith(".c"):
                normalized = admitted.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
                if digest(normalized) in bootstrap.get(relative, ()):
                    admitted = normalized
            state.admit(relative, admitted, reviewed, bootstrap.get(relative, ()))
            state.track(relative, reviewed, owner=state.files.get(relative, {}).get("owner", "stage"),
                        source=identity if variant is not None else None)
            if managed is not None:
                managed[relative] = (original, reviewed)
            if original != reviewed:
                changes[relative] = (original, reviewed)
    changes.update(canonical)
    changes.update(assets)
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
        raise RuntimeError("build and stage Metal with the current compiler, deck, sea-surrender and contact-activity patches before applying gameplay")
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
        if args.action == "apply":
            apply(changes, args.path, source_set)
            print("Applied to native-metal. SAVE, config and renderer resources unchanged.")
        return 0
    except (RuntimeError, OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"error: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
