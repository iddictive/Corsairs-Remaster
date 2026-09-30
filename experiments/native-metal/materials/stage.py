"""Stage only manifest-approved textures; retain originals for an exact bypass."""
import hashlib
import json
import shutil
import sys
from pathlib import Path

root = Path(__file__).resolve().parent
runtime, backup = map(Path, sys.argv[1:3])
enabled = sys.argv[3] == "1"
delivery = json.loads((root / "staging.json").read_text())
manifest = delivery["assets"]
targets = [row["target"] for row in manifest]
assert len(targets) == len(set(targets))
assert all(row["semantic"].startswith(("ground/", "foliage/alpha-tested")) for row in manifest)
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
backup.mkdir(parents=True, exist_ok=True)
state_path = backup / "state.json"
state = json.loads(state_path.read_text()) if state_path.exists() else {}
# Wood replacements were retired. Restore captured originals where the old
# staging path touched them, and reject any unexplained change to board/planks.
for preserved in delivery["preserve_original"]:
    name = preserved["target"]
    target = runtime / "RESOURCE" / "Textures" / name
    assert target.is_file(), name
    current = digest(target)
    if name not in state:
        assert current == preserved["sha256"], f"Preserved material changed: {name}"
        continue
    original = backup / name
    assert original.is_file(), name
    assert state[name]["original"] == preserved["sha256"], f"Unexpected preserved baseline: {name}"
    assert current in (state[name]["original"], state[name]["installed"]), f"Texture changed outside material staging: {name}"
    assert digest(original) == state[name]["original"], f"Changed backup: {name}"
    if current != state[name]["original"]:
        shutil.copy2(original, target)
    state[name]["installed"] = state[name]["original"]
    state_path.write_text(json.dumps(state, indent=2) + "\n")
for row in manifest:
    name = row["target"]
    source = root / row["prepared"]
    assert source.resolve().is_relative_to(root.resolve()), name
    target = runtime / "RESOURCE" / "Textures" / name
    original = backup / name
    assert source.is_file() and digest(source) == row["sha256"], name
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
print("Materials: " + (f"{len(manifest)} hash-verified ground/stone/foliage assets; original wood preserved" if enabled else "original textures"))
