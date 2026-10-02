#!/usr/bin/env python3
"""Build and deliver the reviewed exact-hash gameplay package to native-metal."""

import argparse
import hashlib
import importlib.util
import json
import subprocess
from pathlib import Path

import patch_gameplay_suite as suite
import metal_graphics_settings as graphics
import metal_menu_branding as menu_branding
import metal_deck_camera as deck_camera
import metal_deck_controls as deck_controls
import patch_custody_life as custody_life
import patch_squad_common_supply as squad_supply
import metal_tradebook as tradebook
import metal_governor_dialog as governor_dialog
import metal_living_caribbean as living_caribbean
import metal_evening_lights as evening_lights
from patch_mod_journal import UPDATED_SHA256
from patch_sea_battle_mode_reset import UPDATED_SHA256 as COMBAT_UPDATED_SHA256
from runtime_script_patch import atomic_write

PROJECT = Path(__file__).resolve().parents[1]
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
BASE = {
    "PROGRAM/Loc_ai/LAi_fightparams.c": "5eed196c397d59475474b0f2c5964402167ba024dd33621cc10c785cd007f553",
    "PROGRAM/Loc_ai/LAi_boarding.c": "656e64ced4c97244dc363bd23a328418269cd7426b6c2015da4ac0a896a275da",
    "PROGRAM/seadogs.c": "50248f0e85736c3e8ff35211db110dbb96fb206a7bad507106c6af89c7d15841",
    "PROGRAM/battle_interface/landinterface.c": "75b1650140beaad2d846b1f62638e9e3af7b46454eb49f19fcd7a52ccc6cefcc",
    "PROGRAM/QuestBook/QuestBook_New.txt": "381bd67437fcabb49abca3df8f5253b3113dec685d628d01933bf5b0610cc349",
    "PROGRAM/quests/quests.c": "4e3bbd97bfad571c631c3dcf17b0e6783279d652c78915b4a6f9149ec4654bdc",
    "PROGRAM/quests/quests_reaction.c": "9aabee25dd86dffe3c3f6f7cfd8b4460b116ee0c2d7a18f53d920b3cd459852b",
    "PROGRAM/locations/locations_loader.c": "113764a8246bcc3987217c76fb1966330b2b6d39a5d724cc09a4556fbc3afd8c",
    "PROGRAM/scripts/Crew.c": "072f41ae931d3c19946dab7aca5da9fa7f85c76e14aeebd8836c102a9230b490",
    "PROGRAM/scripts/custody.c": "b434f09a612bdd154239a7a8afae08e8d213e0982effed45926c8fd903aa4b1f",
    "PROGRAM/sea_ai/AIShip.c": "96b1eaef43f818f1b5d5e25f89c2fae3ef74286d7d37102d65c3ea4057cfe187",
    "PROGRAM/sea_ai/sea.c": "107f47b78d4a1fab38ad51bac1c0349bdafd1343a9ebb9eec20952a1fc213cd1",
    # Installed Metal revision, not the layer input: the Esc exit composes on
    # top of the delivered dialogue-attack revision.
    "PROGRAM/dialog.c": "948e47d62c451f204e9e7bfe41bcc6d235255d2438a3b06ef3ab758170361ee8",
}

# Metal revision each file had before the current gameplay layer was first
# delivered. The baseline copy in .cache/gameplay-baseline is written once and
# never overwritten, so a file delivered a second time keeps its pristine
# baseline here instead of silently accepting an unrelated revision.
BASELINE = {
    "PROGRAM/dialog.c": "1f0f4f918f1f35134478e1ba2c98c607492f045242e8e6c30cccdfb95a6b57ce",
    # Recorded by the 2026-09-18 delivery of the previous gameplay layer. Both paths
    # are delivered a second time now, and their recorded pre-delivery revision is
    # not the current BASE revision, so the tool needs the explicit record.
    "PROGRAM/locations/locations_loader.c": "3000c4aedc6b5f36b881dae6b955e5afd3fb2fcb35954434ff326ebb5445b51c",
    "PROGRAM/quests/quests_reaction.c": "b435b2fbc99346d06a7376b3f02de5fec306fa65da096099a0f092b971414514",
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


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
            and record.get("graphics_layer_sha256") == digest(Path(graphics.__file__).read_bytes())
            and record.get("menu_branding_layer_sha256") == digest(Path(menu_branding.__file__).read_bytes())
            and record.get("external_url_patch_sha256") == digest((METAL / "external-url.patch").read_bytes()))


def plan(target_root=None):
    if target_root is None:
        target_root = TARGET
    source_state = suite.classify(SOURCE)[0]
    if source_state not in {"patched", "upgrade"}:
        raise RuntimeError("archived native-storm baseline is not a reviewed gameplay package")
    outputs = suite._build_outputs(suite._read_originals(SOURCE, source_state))
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
    changes = {}
    # Compare all script consumers and interface INIs, not renderer materials.
    for directory in ("PROGRAM", "RESOURCE/INI"):
        for source in (SOURCE / directory).rglob("*"):
            if not source.is_file() or source.name == ".DS_Store":
                continue
            relative = source.relative_to(SOURCE).as_posix()
            target = target_root / relative
            if source.is_symlink() or target.is_symlink():
                raise RuntimeError(f"refusing linked gameplay file: {relative}")
            if not target.is_file():
                raise RuntimeError(f"missing Metal gameplay file: {relative}")
            incoming = outputs.get(relative, source.read_bytes())
            current = target.read_bytes()
            if relative == governor_dialog.PATH:
                reviewed = governor_dialog.prepare(incoming)
                if current not in (incoming, reviewed) and current not in living_caribbean.UPDATED.values():
                    raise RuntimeError(f"unrecognized governor dialogue revision: {relative}")
                if current != reviewed:
                    changes[relative] = (current, reviewed)
                continue
            if relative in living_caribbean.PREPARERS:
                reviewed = living_caribbean.prepare(relative, incoming)
                recognized = {digest(incoming), digest(reviewed), BASE.get(relative)}
                recognized.update(living_caribbean.PREVIOUS.get(relative, ()))
                if digest(current) not in recognized:
                    raise RuntimeError(f"unrecognized living Caribbean revision: {relative}")
                if current != reviewed:
                    changes[relative] = (current, reviewed)
                continue
            if relative in evening_lights.PREPARERS:
                reviewed = evening_lights.prepare(relative, incoming)
                if current not in (incoming, reviewed):
                    raise RuntimeError(f"unrecognized evening lights revision: {relative}")
                if current != reviewed:
                    changes[relative] = (current, reviewed)
                continue
            if relative in menu_branding.FILES:
                canonical_current, _ = menu_branding.strip(relative, current)
                if canonical_current != incoming:
                    raise RuntimeError(f"unrecognized menu branding source revision: {relative}")
                reviewed, _ = menu_branding.prepare(relative, incoming)
                if reviewed != current:
                    changes[relative] = (current, reviewed)
                continue
            if relative in tradebook.BASE:
                reviewed = tradebook.prepare(relative, incoming)
                if current not in (incoming, reviewed) and digest(current) != tradebook.PREVIOUS[relative]:
                    raise RuntimeError(f"unrecognized Metal trade journal revision: {relative}")
                if current != reviewed:
                    changes[relative] = (current, reviewed)
                continue
            if relative in deck_controls.PATHS:
                canonical_current = deck_controls.strip(relative, current)
                if canonical_current != incoming:
                    raise RuntimeError(f"unrecognized Metal deck-control source revision: {relative}")
                reviewed = deck_controls.prepare(relative, incoming)
                if reviewed != current:
                    changes[relative] = (current, reviewed)
                continue
            if relative == deck_camera.PATH:
                if deck_camera.strip(current) != incoming:
                    raise RuntimeError(f"unrecognized deck camera source revision: {relative}")
                reviewed = deck_camera.prepare(incoming)
                if reviewed != current:
                    changes[relative] = (current, reviewed)
                continue
            if relative in custody_life.BASE:
                if digest(current) not in {custody_life.INSTALLED[relative], custody_life.UPDATED[relative]}:
                    raise RuntimeError(f"unrecognized custody revision: {relative}")
                if current != incoming:
                    changes[relative] = (current, incoming)
                continue
            if relative in squad_supply.BASE:
                if digest(incoming) != squad_supply.UPDATED[relative]:
                    raise RuntimeError(f"unreviewed squad-supply output: {relative}")
                if digest(current) not in {
                    squad_supply.BASE[relative], squad_supply.UPDATED[relative], '3a2756ae55d9db463b59a500b297f37eb9d70efff5f008bf08608bb8290eb78d'
                }:
                    raise RuntimeError(f"unrecognized squad-supply revision: {relative}")
                if current != incoming:
                    changes[relative] = (current, incoming)
                continue
            if relative in {item.relative_path for item in graphics.FILES}:
                canonical_current, _ = graphics.strip(relative, current)
                if canonical_current != incoming:
                    raise RuntimeError(f"unrecognized graphics source revision: {relative}")
                reviewed, _ = graphics.prepare(relative, incoming)
                if reviewed != current:
                    changes[relative] = (current, reviewed)
                continue
            if incoming == current:
                continue
            reviewed, backdrop_changed = background_alpha.prepare_file(relative, incoming)
            if backdrop_changed:
                canonical_current, current_backdrop = background_alpha.strip_file(relative, current)
                canonical_incoming, incoming_backdrop = background_alpha.strip_file(relative, reviewed)
                if (relative in BASE and current_backdrop and incoming_backdrop
                        and digest(canonical_current) == BASE[relative]
                        and digest(canonical_incoming) == expected[relative]):
                    changes[relative] = (current, reviewed)
                    continue
                if reviewed != current:
                    raise RuntimeError(f"unrecognized Metal backdrop revision: {relative}")
                continue
            if relative in deck.PATCHES:
                reviewed_current, _ = deck.prepare(relative, current)
                reviewed_incoming, _ = deck.prepare(relative, incoming)
                canonical_current, _ = deck.settings_visibility.strip(
                    relative, reviewed_current
                )
                canonical_incoming, _ = deck.settings_visibility.strip(
                    relative, reviewed_incoming
                )
                if canonical_current != canonical_incoming:
                    raise RuntimeError(f"unrecognized deck gameplay revision: {relative}")
                changes[relative] = (current, incoming)
                continue
            if relative not in BASE:
                raise RuntimeError(f"unreviewed Metal difference: {relative}")
            if digest(incoming) != expected[relative] or digest(current) != BASE[relative]:
                raise RuntimeError(f"unrecognized gameplay revision: {relative}")
            changes[relative] = (current, incoming)
    return changes


def sign_installed_app():
    # Only resource scripts changed; reseal the outer bundle and preserve the
    # already-signed nested executables/frameworks.
    for arguments in (("--force", "-s", "-"), ("--verify", "--deep", "--strict")):
        result = subprocess.run(["codesign", *arguments, str(INSTALLED_APP)],
                                capture_output=True, text=True, check=False)
        if result.returncode:
            detail = result.stderr.strip() or result.stdout.strip()
            raise RuntimeError(f"installed app codesign failed: {detail}")


def apply(changes):
    if not compiler_ready():
        raise RuntimeError("build and stage Metal with the current compiler and deck gameplay patches before applying gameplay")
    app_resources = INSTALLED_APP / "Contents/Resources"
    app_changes = plan(app_resources) if INSTALLED_APP.is_dir() else {}
    engines = [ENGINE]
    if INSTALLED_APP.is_dir():
        engines.append(INSTALLED_APP / "Contents/MacOS/metal-engine")
    holders = subprocess.run(["/usr/sbin/lsof", "-t", "--", *map(str, engines)],
                             capture_output=True, text=True, check=False)
    if holders.returncode != 1 or holders.stdout or holders.stderr:
        raise RuntimeError("close the Metal game before applying gameplay")
    written = []
    app_written = []
    try:
        for relative, (previous, incoming) in changes.items():
            path = TARGET / relative
            if path.read_bytes() != previous:
                raise RuntimeError(f"concurrent Metal change: {relative}")
            backup = METAL / ".cache/gameplay-baseline" / relative
            if backup.exists() and backup.read_bytes() != previous:
                baseline = backup.read_bytes()
                deck = deck_package()
                backup_matches = digest(baseline) == BASELINE.get(relative)
                if relative == governor_dialog.PATH:
                    backup_matches = digest(baseline) == governor_dialog.BASE
                if relative in living_caribbean.PREPARERS:
                    backup_matches = digest(baseline) == BASE.get(relative, living_caribbean.PREPARERS[relative][0])
                if relative in evening_lights.PREPARERS:
                    backup_matches = digest(baseline) == evening_lights.PREPARERS[relative][0]
                if relative in tradebook.BASE:
                    backup_matches = digest(baseline) == tradebook.BASE[relative]
                if relative in custody_life.BASELINE:
                    backup_matches = digest(baseline) == custody_life.BASELINE[relative]
                if relative in squad_supply.BASE:
                    backup_matches = digest(baseline) == squad_supply.BASE[relative]
                if relative in menu_branding.FILES:
                    backup_matches = digest(baseline) == menu_branding.BASE[relative]
                graphics_spec = next((item for item in graphics.FILES if item.relative_path == relative), None)
                if graphics_spec:
                    backup_matches = digest(baseline) == graphics_spec.original_sha256
                if relative in deck.PATCHES:
                    prepared_baseline, _ = deck.prepare(relative, baseline)
                    shared_incoming = incoming
                    if relative == deck_camera.PATH:
                        shared_incoming = deck_camera.strip(shared_incoming)
                    if relative in deck_controls.PATHS:
                        shared_incoming = deck_controls.strip(relative, shared_incoming)
                    prepared_incoming, _ = deck.prepare(relative, shared_incoming)
                    canonical_baseline, _ = deck.settings_visibility.strip(relative, prepared_baseline)
                    canonical_incoming, _ = deck.settings_visibility.strip(relative, prepared_incoming)
                    backup_matches = canonical_baseline == canonical_incoming
                if not backup_matches:
                    raise RuntimeError(f"unexpected backup revision: {relative}")
            if not backup.exists():
                atomic_write(backup, previous)
            atomic_write(path, incoming)
            written.append((path, previous, incoming))
        if plan():
            raise RuntimeError("post-apply gameplay comparison failed")
        for relative, (previous, incoming) in app_changes.items():
            app_target = app_resources / relative
            if app_target.read_bytes() != previous:
                raise RuntimeError(f"concurrent installed gameplay change: {relative}")
            atomic_write(app_target, incoming)
            app_written.append((app_target, previous, incoming))
        if app_written:
            if plan(app_resources):
                raise RuntimeError("post-apply installed gameplay comparison failed")
            sign_installed_app()
    except BaseException as error:
        failures = []
        for path, previous, incoming in reversed(written + app_written):
            try:
                if path.read_bytes() != incoming:
                    raise RuntimeError(f"concurrent change prevents rollback: {path}")
                atomic_write(path, previous)
            except (OSError, RuntimeError) as rollback_error:
                failures.append(str(rollback_error))
        if app_written and not failures:
            try:
                sign_installed_app()
            except (OSError, RuntimeError) as rollback_error:
                failures.append(str(rollback_error))
        if failures:
            raise RuntimeError(f"{error}; rollback incomplete: {'; '.join(failures)}") from error
        raise


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("check", "apply"), default="check", nargs="?")
    args = parser.parse_args()
    try:
        changes = plan()
        ready = compiler_ready()
        print(f"Metal gameplay: {len(changes)} files pending; compiler {'ready' if ready else 'build/stage pending'}")
        for path in sorted(changes):
            print(path)
        if args.action == "apply":
            apply(changes)
            print("Applied to native-metal. SAVE, config and renderer resources unchanged.")
        return 0
    except (RuntimeError, OSError, ValueError) as error:
        print(f"error: {error}")
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
