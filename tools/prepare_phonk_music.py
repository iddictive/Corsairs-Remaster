#!/usr/bin/env python3
"""Prepare the player's official, checksum-pinned music download; no audio in Git."""
import hashlib
import json
from pathlib import Path
import urllib.request

ROOT = Path(__file__).resolve().parents[1] / "src/assets/audio"


def prepare():
    record = json.loads((ROOT / "manifest.json").read_text())
    spec = record["files"]["RESOURCE/Sounds/MUSIC/phonk-drive.mp3"]
    target = ROOT / "phonk-drive.mp3"
    if target.is_file():
        data = target.read_bytes()
    else:
        with urllib.request.urlopen(spec["url"], timeout=30) as response:
            if not response.headers.get("Content-Type", "").startswith("audio/"):
                raise RuntimeError("The official music download did not return audio")
            data = response.read(16 * 1024 * 1024 + 1)
        if len(data) > 16 * 1024 * 1024:
            raise RuntimeError("The official music download exceeded its expected size")
    if hashlib.sha256(data).hexdigest() != spec["sha256"]:
        raise RuntimeError("Phonk track changed; existing music is preserved")
    if not target.exists():
        # Exclusive creation preserves an unknown concurrent local input.
        with target.open("xb") as output:
            output.write(data)
    print("Phonk music: official download verified")


if __name__ == "__main__":
    prepare()
