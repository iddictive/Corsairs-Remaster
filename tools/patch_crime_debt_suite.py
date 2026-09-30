#!/usr/bin/env python3
"""Atomically install or revert the linked patrol, crime, debt, and ambush suite."""

from __future__ import annotations

import argparse
import os
import sys
from collections import defaultdict
from pathlib import Path

from runtime_script_patch import (
    FilePatch,
    PatchSet,
    TARGET_ROOT,
    atomic_write,
    lifecycle_lock,
    require_idle,
    sha256,
    transform,
)

SNAPSHOT_ROOT = TARGET_ROOT / ".codex-crime-debt-suite" / "20260913-v1"


def patch_sets() -> tuple[tuple[str, PatchSet], ...]:
    from patch_crime_reputation import PATCH as crime_patch
    from patch_crew_debt_ledger import FILES as debt_files
    from patch_crew_debt_ledger import SNAPSHOT_ROOT as debt_snapshot
    from patch_dialog_ambush import PATCH as dialog_patch
    from patch_island_patrol_relations import PATCH as island_patch

    return (
        ("island patrol relations", island_patch),
        ("crime reputation", crime_patch),
        ("crew debt ledger", PatchSet(debt_snapshot, debt_files)),
        ("dialog ambush", dialog_patch),
    )


def file_chains() -> dict[str, tuple[FilePatch, ...]]:
    chains: dict[str, list[FilePatch]] = defaultdict(list)
    for _, patch_set in patch_sets():
        for spec in patch_set.files:
            chains[spec.relative_path].append(spec)
    return {path: tuple(specs) for path, specs in chains.items()}


def expected_bytes(source: bytes, specs: tuple[FilePatch, ...]) -> bytes:
    result = source
    for spec in specs:
        if sha256(result) != spec.original_sha256:
            raise RuntimeError(
                f"{spec.relative_path}: patch chain expected {spec.original_sha256}, got {sha256(result)}"
            )
        result = transform(result, spec)
    return result


def classify(root: Path) -> tuple[str, list[tuple[str, str, str]]]:
    rows: list[tuple[str, str, str]] = []
    states: set[str] = set()
    for relative_path, specs in file_chains().items():
        path = root / relative_path
        digest = sha256(path.read_bytes()) if path.is_file() else "-"
        if digest == specs[0].original_sha256:
            state = "original"
        elif digest == specs[-1].patched_sha256:
            state = "patched"
        elif any(digest == spec.patched_sha256 for spec in specs[:-1]):
            state = "intermediate"
        else:
            state = "unsupported"
        rows.append((relative_path, state, digest))
        states.add(state)
    aggregate = states.pop() if len(states) == 1 else "mixed"
    return aggregate, rows


def print_status(root: Path) -> int:
    state, rows = classify(root)
    print(f"target: {root}")
    print(f"state:  {state}")
    for path, item_state, digest in rows:
        print(f"{item_state:12} {digest}  {path}")
    return 0 if state in {"original", "patched"} else 1


def _snapshot(originals: dict[str, bytes], specs_by_path: dict[str, tuple[FilePatch, ...]]) -> None:
    for relative_path, data in originals.items():
        expected = specs_by_path[relative_path][0].original_sha256
        if sha256(data) != expected:
            raise RuntimeError(f"{relative_path}: source changed before snapshot")
        snapshot = SNAPSHOT_ROOT / relative_path
        if snapshot.exists():
            if sha256(snapshot.read_bytes()) != expected:
                raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
            continue
        atomic_write(snapshot, data)
        os.chmod(snapshot, 0o444)


def apply_suite(root: Path) -> str:
    if root.resolve() != TARGET_ROOT.resolve():
        raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")
    state, _ = classify(root)
    if state == "patched":
        return "already patched"
    if state != "original":
        raise RuntimeError(f"refusing apply: aggregate state is {state}")

    chains = file_chains()
    originals = {path: (root / path).read_bytes() for path in chains}
    patched = {path: expected_bytes(originals[path], specs) for path, specs in chains.items()}

    with lifecycle_lock():
        require_idle()
        for relative_path, data in originals.items():
            if (root / relative_path).read_bytes() != data:
                raise RuntimeError(f"{relative_path}: source changed during suite preflight")
        _snapshot(originals, chains)
        written: list[str] = []
        try:
            for relative_path, data in patched.items():
                atomic_write(root / relative_path, data)
                written.append(relative_path)
            final, _ = classify(root)
            if final != "patched":
                raise RuntimeError(f"post-apply state is {final}")
        except Exception:
            for relative_path in reversed(written):
                atomic_write(root / relative_path, originals[relative_path])
            raise
    return "patched"


def revert_suite(root: Path) -> str:
    if root.resolve() != TARGET_ROOT.resolve():
        raise RuntimeError(f"mutation target is hardcoded to {TARGET_ROOT}")
    state, _ = classify(root)
    if state == "original":
        return "already original"
    if state != "patched":
        raise RuntimeError(f"refusing revert: aggregate state is {state}")

    chains = file_chains()
    originals: dict[str, bytes] = {}
    for relative_path, specs in chains.items():
        snapshot = SNAPSHOT_ROOT / relative_path
        if not snapshot.is_file():
            raise RuntimeError(f"snapshot is missing: {snapshot}")
        data = snapshot.read_bytes()
        if sha256(data) != specs[0].original_sha256:
            raise RuntimeError(f"snapshot hash mismatch: {snapshot}")
        originals[relative_path] = data
    patched = {path: (root / path).read_bytes() for path in chains}

    with lifecycle_lock():
        require_idle()
        for relative_path, data in patched.items():
            if (root / relative_path).read_bytes() != data:
                raise RuntimeError(f"{relative_path}: source changed during suite preflight")
        written: list[str] = []
        try:
            for relative_path, data in originals.items():
                atomic_write(root / relative_path, data)
                written.append(relative_path)
            final, _ = classify(root)
            if final != "original":
                raise RuntimeError(f"post-revert state is {final}")
        except Exception:
            for relative_path in reversed(written):
                atomic_write(root / relative_path, patched[relative_path])
            raise
    return "original"


def verify(root: Path) -> int:
    chains = file_chains()
    for relative_path, specs in chains.items():
        path = root / relative_path
        if not path.is_file():
            raise RuntimeError(f"missing file: {path}")
        digest = sha256(path.read_bytes())
        if digest == specs[0].original_sha256:
            final = expected_bytes(path.read_bytes(), specs)
        elif digest == specs[-1].patched_sha256:
            final = path.read_bytes()
        else:
            raise RuntimeError(f"{relative_path}: unsupported suite state {digest}")
        if sha256(final) != specs[-1].patched_sha256:
            raise RuntimeError(f"{relative_path}: final chain hash mismatch")

    from patch_crime_reputation import verify as verify_crime
    from patch_crew_debt_ledger import verify_static
    from patch_dialog_ambush import verify as verify_dialog
    from patch_crew_debt_ledger import FILES as debt_files

    if verify_crime(root) != 0:
        return 1
    verify_static(PatchSet(SNAPSHOT_ROOT / "verify-debt", debt_files))
    if verify_dialog(root) != 0:
        return 1
    print("PASS: atomic suite chain, crime, debt, and dialog fixtures")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("status", "verify", "apply", "revert"))
    parser.add_argument("--target", type=Path, default=TARGET_ROOT)
    args = parser.parse_args()
    root = args.target.resolve()
    try:
        if args.action == "status":
            return print_status(root)
        if args.action == "verify":
            return verify(root)
        result = apply_suite(root) if args.action == "apply" else revert_suite(root)
        print(result)
        return 0
    except RuntimeError as error:
        print(f"error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
