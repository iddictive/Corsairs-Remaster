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
allowed = {"Sandtile.tga.tx", "pierWood1.tga.tx", "tileU2.tga.tx", "deckPlanksU1.tga.tx", "treePalms.tga.tx", "trees.tga.tx", "townK2.tga.tx"}
assert {row["target"] for row in manifest} == allowed
digest = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
backup.mkdir(parents=True, exist_ok=True)
state_path = backup / "state.json"
state = json.loads(state_path.read_text()) if state_path.exists() else {}
for row in manifest:
    name = row["target"]
    source = root / row.get("prepared_path", "prepared/" + name)
    assert source.resolve().is_relative_to(root.resolve()), name
    target = runtime / "RESOURCE" / "Textures" / name
    original = backup / name
    assert source.is_file() and digest(source) == row["output_sha256"], name
    created = row.get("created_target", False)
    if created:
        current = digest(target) if target.is_file() else None
        expected = row["output_sha256"]
        assert current in (None, expected), f"Texture changed outside material staging: {name}"
        if enabled and current != expected:
            shutil.copy2(source, target)
            current = expected
        elif not enabled and target.exists():
            target.unlink()
            current = None
        state[name] = {"original": None, "installed": current}
    else:
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

    redirect = row.get("model_redirect")
    if redirect:
        model = runtime / redirect["path"]
        model_key = "model:" + redirect["path"]
        model_backup = backup / "models" / Path(redirect["path"]).relative_to("RESOURCE")
        assert model.is_file(), model
        if model_key not in state:
            model_backup.parent.mkdir(parents=True, exist_ok=True)
            assert not model_backup.exists(), f"Unregistered backup: {model_backup}"
            shutil.copy2(model, model_backup)
            state[model_key] = {"original": digest(model), "installed": digest(model)}
            state_path.write_text(json.dumps(state, indent=2) + "\n")
        assert digest(model_backup) == state[model_key]["original"], f"Changed backup: {model}"
        current_model = model.read_bytes()
        original_model = model_backup.read_bytes()
        old = redirect["from"].encode("ascii") + b"\0"
        new = redirect["to"].encode("ascii") + b"\0"
        assert len(old) == len(new), "GM redirect must preserve string-table offsets"
        assert original_model.count(old) == redirect["expected_occurrences"]
        redirected_model = original_model.replace(old, new)
        assert redirected_model.count(new) == redirect["expected_occurrences"]
        allowed_models = {original_model, redirected_model}
        assert current_model in allowed_models, f"Model changed outside material staging: {model}"
        selected_model = redirected_model if enabled else original_model
        if current_model != selected_model:
            model.write_bytes(selected_model)
        state[model_key]["installed"] = digest(model)
        state_path.write_text(json.dumps(state, indent=2) + "\n")

    preserved = row.get("shared_source_preserved")
    if preserved:
        shared = runtime / preserved["path"]
        assert digest(shared) == preserved["sha256"], f"Shared source changed: {shared}"

print("Materials: " + ("prepared sand + wood + paving + foliage + isolated San Juan ground" if enabled else "original textures and San Juan material binding"))
