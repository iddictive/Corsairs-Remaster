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
import metal_fleet_gameplay as fleet_gameplay
import metal_fleet_sea as fleet_sea
import metal_fleet_ui as fleet_ui
import metal_military_integration as military_callbacks
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


# Independently delivered cannon aiming; this package does not replace it.
PRESERVED_RUNTIME = {
    "PROGRAM/sea_ai/AICannon.c": "b013ee3296b62e0b71c26f5a22b8b2b0c7601ba30b4e281a4162246783b00229",
}


# Exact main revisions accepted for this gameplay review upgrade.
PREVIOUS = {
    "PROGRAM/Loc_ai/LAi_fightparams.c": "0e5c671e97b2575586ee87379db027c0393689f1e60b242b5c9d215a7cf37f78",
    "PROGRAM/quests/quests.c": "9dddd1e68b627ba53451df039c17a224c70119d4d21381eb82c3dcc356f0ec3c",
    "PROGRAM/scripts/Crew.c": "07c1df47aac70ed0c0bc3cf4b01a6bd70564c3cd5e45b629e063d4cfabdae8d2",
    "PROGRAM/scripts/CompanionTravel.c": "311926e964fa10bc1cf121a162ac22f9d1139c2e3f387738bf7a253044fb645e",
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def retain_sea_speed(current, reviewed):
    # This separately delivered sailing function is outside the fleet owner.
    # Admit it only when removing that exact delta restores a reviewed bridge.
    def split(data):
        text = data.decode("utf-8").replace("\r\n", "\n")
        start = text.index("float Sea_ApplyMaxSpeedZ(")
        end = text.index("// <<<--- ZhilyaevDm", start)
        return text[:start], text[start:end], text[end:]
    before, retained, after = split(current)
    _, canonical, _ = split(reviewed)
    if digest(retained.encode()) != "2df44847ed8efdc2e64c4acc6cdebb9650e7f6ba4b4b5cd5c1e5d512fd03c224":
        return current, reviewed
    canonical_current = (before + canonical + after).replace("\n", "\r\n").encode()
    before, _, after = split(reviewed)
    preserved = (before + retained + after).replace("\n", "\r\n").encode()
    return canonical_current, preserved


def script_bytes(relative, data, *representations):
    # The VM treats LF and CRLF identically; compare the exact reviewed CRLF
    # representation without accepting any other script change.
    if relative.endswith(".c"):
        normalized_lf = data.replace(b"\r\n", b"\n")
        for representation in representations:
            if normalized_lf == representation.replace(b"\r\n", b"\n"):
                return representation
        known = set()
        owners = (globals(), vars(suite), vars(living_caribbean), vars(fleet_gameplay),
                  vars(fleet_sea), vars(fleet_ui), vars(military_callbacks), vars(graphics),
                  vars(menu_branding), vars(deck_camera), vars(deck_controls), vars(tradebook),
                  vars(governor_dialog), vars(squad_supply), vars(custody_life))
        for owner in owners:
            for value in owner.values():
                if not isinstance(value, dict) or relative not in value:
                    continue
                hashes = value[relative]
                if isinstance(hashes, str):
                    known.add(hashes)
                elif isinstance(hashes, (tuple, list, set)):
                    known.update(item for item in hashes if isinstance(item, str))
        if digest(data) in known:
            return data
        normalized = data.replace(b"\r\n", b"\n").replace(b"\n", b"\r\n")
        if digest(normalized) in known:
            return normalized
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
            and record.get("sea_contact_patch_sha256") == digest((METAL / "sea-contact-activity.patch").read_bytes())
            and record.get("graphics_layer_sha256") == digest(Path(graphics.__file__).read_bytes())
            and record.get("menu_branding_layer_sha256") == digest(Path(menu_branding.__file__).read_bytes())
            and record.get("external_url_patch_sha256") == digest((METAL / "external-url.patch").read_bytes()))


# Compose after the existing gameplay owners, notably the AIShip fleet layer.
FLEET_FINAL_BASE = {**fleet_sea.BASE_HASHES, **fleet_ui.BASES}
FLEET_FINAL_SHA = {'PROGRAM/worldmap/worldmap_reload.c': 'e3ea4c8b5999ff3d7ffdf9129b4b67f5d7ce0bc99540c2e0e7376b83b076b2dc',
 'PROGRAM/sea_ai/sea.c': 'ace3ef44a63c688ac5f0639a735505d9ec1c8fbd2a0628db6e9baf0685edfcd9',
 'PROGRAM/sea_ai/AIFantom.c': '94064aa548f1d3e41033c94ebe63dc823c664c1943852eadabff3d3ed6a75c0b',
 'PROGRAM/sea_ai/AIShip.c': '3f42a3c1c00db690ed91e83e78113a8ed4a12ed01e4e9a02ccf02e3842f78427',
 'PROGRAM/interface/map.c': '4aac6bd149f573d4425be7d524648998a70e28106838ec2f330426b8b7bc10be',
 'PROGRAM/battle_interface/WmInterface.c': '4088a04e7f29758167938ec95530199bfd998cb7b37f01c67b939f4d973aac78',
 'PROGRAM/battle_interface/loginterface.c': '88a187aec46bf600ff6c239eb2d358b8155775a006f70d085917fa7bc79a849c',
 'RESOURCE/INI/interfaces/map.ini': '0edb623bf4ce8f01d815a80d62c66748eb7520ad133917f927d83b90c43cfafd'}


FLEET_FINAL_PREVIOUS = {'PROGRAM/sea_ai/sea.c': {'22cbf6b06c96e6518093e5e717c48973684d8006b19fe9e52fe862e658425ab1',
                          '7b4c7a20efa8d2b18954a47e134e7cc3701b72d241d6b38fcb4a30db63ad976a',
                          'ace3ef44a63c688ac5f0639a735505d9ec1c8fbd2a0628db6e9baf0685edfcd9',
                          'd0178cf32d2a148df0f444dcd751d497071489c847f70e033f976350f00a629c',
                          'ef3137c2627cd023b5e03e0714b57fae9a62aeb0dbcb0746fb1101985d835a6c'},
 'PROGRAM/interface/map.c': {'8728d28c489c98d6bee5b01698f14c7e78f192f12e2359e3829a5a176c0d910c'},
 'PROGRAM/battle_interface/WmInterface.c': {'4e9a00718859264aa67c528bd73a54215c9972c39b94b7ce4219e026104f8ae1'},
 'PROGRAM/battle_interface/loginterface.c': {'7c29df890c9db731ea72eddbf0d27ee07314b6b41643f36965f967efb67a90e0'},
 'RESOURCE/INI/interfaces/map.ini': {'fc8394f323f4ff94ad125e7c3b10c3219df957a7fab2c0ec545720e2b087e122'}}


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


def plan(target_root=None):
    if target_root is None:
        target_root = TARGET
    living_caribbean.verify_fort_layout_assets(target_root)
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
    changes = {}
    originals = {}
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
            originals[relative] = target.read_bytes()
            current = military_callbacks.strip(relative, script_bytes(relative, originals[relative], incoming))
            if digest(current) == PRESERVED_RUNTIME.get(relative):
                continue
            if digest(current) == military_callbacks.RETAINED_BASES.get(relative):
                # A pinned independently delivered revision keeps its existing
                # behaviour; only this package's callback layer is upgraded.
                continue
            if relative in FLEET_FINAL_BASE:
                reviewed = prepare_fleet_final(relative, incoming)
                if relative == "PROGRAM/sea_ai/sea.c":
                    current, reviewed = retain_sea_speed(current, reviewed)
                if digest(current) not in ({FLEET_FINAL_BASE[relative], FLEET_FINAL_SHA[relative]} | FLEET_FINAL_PREVIOUS.get(relative, set())):
                    raise RuntimeError(f"unrecognized installed fleet bridge: {relative}")
                if current != reviewed:
                    changes[relative] = (current, reviewed)
                continue
            if relative in fleet_gameplay.FILES:
                if relative in living_caribbean.PREPARERS:
                    incoming = living_caribbean.prepare(relative, incoming)
                incoming, _ = background_alpha.prepare_file(relative, incoming)
                reviewed = fleet_gameplay.prepare(relative, incoming)
                if not fleet_gameplay.recognized(relative, current):
                    raise RuntimeError(f"unrecognized fleet gameplay revision: {relative}")
                if current != reviewed:
                    changes[relative] = (current, reviewed)
                continue
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
                reviewed = deck_controls.prepare(relative, incoming)
                current = script_bytes(relative, current, incoming, reviewed)
                canonical_current = deck_controls.strip(relative, current)
                if canonical_current != incoming:
                    raise RuntimeError(f"unrecognized Metal deck-control source revision: {relative}")
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
                if digest(current) not in {custody_life.INSTALLED[relative], custody_life.UPDATED[relative]} | custody_life.PREVIOUS.get(relative, set()):
                    raise RuntimeError(f"unrecognized custody revision: {relative}")
                if current != incoming:
                    changes[relative] = (current, incoming)
                continue
            if relative in squad_supply.BASE:
                if digest(incoming) != squad_supply.UPDATED[relative]:
                    raise RuntimeError(f"unreviewed squad-supply output: {relative}")
                if digest(current) not in {
                    squad_supply.BASE[relative], squad_supply.UPDATED[relative], '3a2756ae55d9db463b59a500b297f37eb9d70efff5f008bf08608bb8290eb78d'
                } | squad_supply.PREVIOUS.get(relative, set()):
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
            if relative not in BASE and relative not in PREVIOUS:
                # No composer owns this delta. Preserve existing runtime edits
                # instead of replacing them with the historical carrier copy.
                continue
            if digest(incoming) != expected[relative] or digest(current) not in {BASE.get(relative), PREVIOUS.get(relative)}:
                raise RuntimeError(f"unrecognized gameplay revision: {relative}")
            changes[relative] = (current, incoming)
    # The owning layers validate their canonical predecessors above; military
    # callbacks then compose once on those exact final bytes. Original bytes
    # remain the backup/delivery owner, including an already-installed adapter.
    for relative in military_callbacks.HOOKS:
        target = target_root / relative
        current = target.read_bytes()
        if relative in originals and current != originals[relative]:
            raise RuntimeError(f"concurrent gameplay change: {relative}")
        originals[relative] = current
        incoming = changes[relative][1] if relative in changes else military_callbacks.strip(relative, script_bytes(relative, current))
        reviewed = military_callbacks.prepare(relative, incoming)
        if reviewed != current:
            changes[relative] = (current, reviewed)
        else:
            changes.pop(relative, None)
    return {relative: (originals[relative], incoming)
            for relative, (_, incoming) in changes.items()}


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
        raise RuntimeError("build and stage Metal with the current compiler, deck, sea-surrender and contact-activity patches before applying gameplay")
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
                backup_matches = digest(baseline) in {BASELINE.get(relative), BASE.get(relative), PREVIOUS.get(relative), suite.base_hashes().get(relative)}
                if relative in FLEET_FINAL_BASE:
                    backup_matches = digest(baseline) in ({FLEET_FINAL_BASE[relative], FLEET_FINAL_SHA[relative]} | FLEET_FINAL_PREVIOUS.get(relative, set())) or backup_matches
                if relative in military_callbacks.HOOKS:
                    backup_matches = digest(baseline) in {
                        military_callbacks.BASES[relative], military_callbacks.UPDATED[relative],
                        military_callbacks.RETAINED_BASES.get(relative), military_callbacks.RETAINED_UPDATED.get(relative),
                    } | military_callbacks.PREVIOUS.get(relative, set()) | military_callbacks.RETAINED_PREVIOUS.get(relative, set()) or backup_matches
                if relative == governor_dialog.PATH:
                    backup_matches = digest(baseline) == governor_dialog.BASE
                if relative in living_caribbean.PREPARERS:
                    backup_matches = digest(baseline) in {BASE.get(relative), living_caribbean.PREPARERS[relative][0]} | living_caribbean.PREVIOUS.get(relative, set())
                if relative in evening_lights.PREPARERS:
                    backup_matches = digest(baseline) == evening_lights.PREPARERS[relative][0]
                if relative in fleet_gameplay.FILES:
                    backup_matches = fleet_gameplay.recognized(relative, baseline) or backup_matches
                if relative in tradebook.BASE:
                    backup_matches = digest(baseline) == tradebook.BASE[relative]
                if relative in custody_life.BASELINE:
                    backup_matches = digest(baseline) == custody_life.BASELINE[relative]
                if relative in squad_supply.BASE:
                    backup_matches = digest(baseline) in {squad_supply.BASE[relative]} | squad_supply.PREVIOUS.get(relative, set())
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
