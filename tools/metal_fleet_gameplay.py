"""Reviewed Metal fleet/progression layer over the canonical gameplay package."""

import hashlib
import json
from pathlib import Path


MANIFEST = Path(__file__).with_name("gameplay") / "fleet-gameplay.json"
FILES = json.loads(MANIFEST.read_text())["files"]
BASE = {path: spec["base_sha256"] for path, spec in FILES.items()}
UPDATED = {path: spec["updated_sha256"] for path, spec in FILES.items()}
PREVIOUS = {path: set(spec["previous_sha256"]) for path, spec in FILES.items()}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def recognized(relative, data):
    return digest(data) in {BASE[relative], UPDATED[relative]} | PREVIOUS[relative]


def prepare(relative, data):
    spec = FILES[relative]
    if digest(data) == spec["updated_sha256"]:
        return data
    if digest(data) != spec["base_sha256"]:
        raise RuntimeError(f"unrecognized fleet gameplay input: {relative}")
    for edit in spec["edits"]:
        before = edit["before"].replace("\n", "\r\n").encode("utf-8")
        after = edit["after"].replace("\n", "\r\n").encode("utf-8")
        if data.count(before) != 1:
            raise RuntimeError(f"ambiguous fleet gameplay anchor: {relative}")
        data = data.replace(before, after, 1)
    if digest(data) != spec["updated_sha256"]:
        raise RuntimeError(f"unreviewed fleet gameplay output: {relative}")
    return data
