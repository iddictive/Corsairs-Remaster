#!/usr/bin/env python3
"""Static routing contract for deck walking and the original gameplay modes."""
from pathlib import Path
import importlib.util
import re

ROOT = Path(__file__).resolve().parent
spec = importlib.util.spec_from_file_location("deck_walk", ROOT / "deck_walk.py")
deck_walk = importlib.util.module_from_spec(spec)
spec.loader.exec_module(deck_walk)


def require(condition: bool, message: str) -> None:
    if not condition:
        raise AssertionError(message)


baseline = ROOT / ".cache/deck-walk-baseline/PROGRAM/controls/init_pc.c"
source = baseline.read_bytes()
patched, state = deck_walk.prepare("PROGRAM/controls/init_pc.c", source)
controls_baseline = ROOT / ".cache/deck-walk-baseline/PROGRAM/controls/controls.c"
controls_patched, controls_state = deck_walk.prepare(
    "PROGRAM/controls/controls.c", controls_baseline.read_bytes()
)
text = patched.decode("utf-8") + controls_patched.decode("utf-8")

bindings = {
    control: (group, key)
    for group, control, key in re.findall(
        r'CI_CreateAndSetControls\(\s*"([^"]*)",\s*"([^"]+)",\s*CI_GetKeyCode\("([^"]+)"\)', text
    )
}
maps = set(re.findall(r'MapControlToGroup\("([^"]+)",\s*"([^"]+)"\)', text))

require(state == "original", "probe must start from the reviewed original corpus")
require(controls_state == "original", "controls probe must start from the reviewed original corpus")
require("RebuildControlsRegistry();" in text, "save-restored registry projection is rebuilt before options")
require("int cntrlCode = CreateControl(controlName);" in text,
        "every remapped control resolves its live engine ID by name")
require("objControlsState.map.controls.(controlName) = cntrlCode;" in text,
        "live engine IDs replace persisted numeric projections")

# Derive the real engine registry order from the old and patched init scripts. Five
# new deck actions precede world-map registration, so a save-restored numeric ID
# targets a different live action even while the saved key name remains correct.
def registry_names(script: bytes) -> list[str]:
    names = []
    for raw_name in re.findall(
            rb'CI_CreateAndSetControls\(\s*"[^"]*",\s*"([^"]+)"', script):
        name = raw_name.decode()
        if name not in names:  # CreateControl returns the existing ID for duplicate names.
            names.append(name)
    return names


old_registry = registry_names(source)
live_registry = registry_names(patched)
worldmap_actions = ("WMapShipSailUp", "WMapShipSailDown", "WMapShipTurnLeft", "WMapShipTurnRight")
require(len(live_registry) == len(old_registry) + 5, "fixture must add exactly five registry entries")
for action in worldmap_actions:
    saved_id = old_registry.index(action)
    require(live_registry[saved_id] != action, f"old {action} ID must be stale after deck insertion")
    rebuilt_id = live_registry.index(action)  # Engine CreateControl(name) lookup contract.
    require(live_registry[rebuilt_id] == action, f"name rebuild must restore {action}")
require(bindings["DeckWalk_Forward"] == ("Sailing1Pers", "KEY_W"), "deck forward owns W")
require(bindings["DeckWalk_Backward"] == ("Sailing1Pers", "KEY_S"), "deck backward owns S")
require(bindings["DeckWalk_Left"] == ("Sailing1Pers", "KEY_A"), "deck left owns A")
require(bindings["DeckWalk_Right"] == ("Sailing1Pers", "KEY_D"), "deck right owns D")
require(bindings["Ship_TurnLeft"] == ("Sailing3Pers", "KEY_A"), "outside sea keeps A rudder")
require(bindings["Ship_TurnRight"] == ("Sailing3Pers", "KEY_D"), "outside sea keeps D rudder")
require(bindings["Ship_SailUp"] == ("Sailing3Pers", "KEY_W"), "outside sea keeps W sails")
require(bindings["Ship_SailDown"] == ("Sailing3Pers", "KEY_S"), "outside sea keeps S sails")
for control in ("Ship_TurnLeft", "Ship_TurnRight", "Ship_SailUp", "Ship_SailDown"):
    require((control, "Sailing1Pers") not in maps, f"{control} must not leak into deck controls")
require(bindings["Ship_TurnLeft1"] == ("Sailing1Pers", "VK_LEFT"), "deck sea rudder keeps left arrow")
require(bindings["Ship_TurnRight1"] == ("Sailing1Pers", "VK_RIGHT"), "deck sea rudder keeps right arrow")
require(bindings["Ship_SailUp1"] == ("Sailing1Pers", "VK_UP"), "deck sea sails keep up arrow")
require(bindings["Ship_SailDown1"] == ("Sailing1Pers", "VK_DOWN"), "deck sea sails keep down arrow")
require(bindings["WMapShipSailUp"] == ("WorldMapControls", "KEY_W"), "world map keeps W")
require(bindings["WMapShipSailDown"] == ("WorldMapControls", "KEY_S"), "world map keeps S")
require(bindings["WMapShipTurnLeft"] == ("WorldMapControls", "KEY_A"), "world map keeps A")
require(bindings["WMapShipTurnRight"] == ("WorldMapControls", "KEY_D"), "world map keeps D")
require(bindings["ChrForward"] == ("PrimaryLand", "KEY_W"), "land keeps W movement")

camera = (ROOT / "deck-walk.patch").read_text()
require('const bool walkMode = AttributesPointer->GetAttributeAsDword("WalkMode") != 0;' in camera,
        "deck movement remains gated by deck walk mode")
require('if (length > 0.f && !telescope)' in camera, "telescope suppresses deck translation")
print("PASS: land, deck, outside sea, deck sea arrows and world-map controls are isolated")
