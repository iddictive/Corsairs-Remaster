"""Stage only manifest-approved textures; retain originals for an exact bypass."""
import hashlib
import json
import shutil
import sys
from pathlib import Path

root = Path(__file__).resolve().parent
runtime, backup = map(Path, sys.argv[1:3])
enabled = sys.argv[3] == "1"
manifest = json.loads((root / "manifest.json").read_text())
allowed = {"Sandtile.tga.tx", "pierWood1.tga.tx", "tileU2.tga.tx", "deckPlanksU1.tga.tx"}
assert {row["target"] for row in manifest} == allowed
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
backup.mkdir(parents=True, exist_ok=True)
state_path = backup / "state.json"
state = json.loads(state_path.read_text()) if state_path.exists() else {}
for row in manifest:
    name = row["target"]
    source = root / "prepared" / name
    target = runtime / "RESOURCE" / "Textures" / name
    original = backup / name
    assert source.is_file() and digest(source) == row["output_sha256"], name
    assert target.is_file(), target
    current = digest(target)
    if name not in state:
        assert not original.exists(), f"Unregistered backup: {original}"
        shutil.copy2(target, original)
        state[name] = {"original": current, "installed": current}
        state_path.write_text(json.dumps(state, indent=2) + "\n")
    assert digest(original) == state[name]["original"], f"Changed backup: {name}"
    assert current in (state[name]["original"], state[name]["installed"]), f"Texture changed outside material staging: {name}"
    selected = source if enabled else original
    if current != digest(selected):
        shutil.copy2(selected, target)
    state[name]["installed"] = digest(selected)
    state_path.write_text(json.dumps(state, indent=2) + "\n")
print("Materials: " + ("prepared sand + wood + paving" if enabled else "original textures"))
