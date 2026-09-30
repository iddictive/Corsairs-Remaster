#!/usr/bin/env python3
"""Stage or remove the three deterministic rockK2 aliases; never rewrites models."""
import hashlib
import json
import shutil
import sys
from pathlib import Path

root = Path(__file__).resolve().parent
runtime = Path(sys.argv[1]).resolve()
enabled = sys.argv[2] == "1"
manifest = json.loads((root / "staging.json").read_text())
aliases = manifest["aliases"]
assert len(aliases) == 3
assert len({len(row["target"]) for row in aliases} | {len(manifest["semantic_material"])}) == 1
textures = runtime / "RESOURCE" / "Textures"
assert textures.is_dir()
for row in aliases:
    source = (root / row["prepared"]).resolve()
    assert source.is_relative_to(root) and source.is_file()
    assert hashlib.sha256(source.read_bytes()).hexdigest() == row["sha256"]
    target = textures / row["target"]
    if enabled:
        if not target.exists() or hashlib.sha256(target.read_bytes()).hexdigest() != row["sha256"]:
            shutil.copy2(source, target)
    elif target.exists():
        target.unlink()
print("rockK2 aliases: " + ("A/B/C staged" if enabled else "removed"))
