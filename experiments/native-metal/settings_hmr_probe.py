#!/usr/bin/env python3
"""Verify native-metal settings HMR ownership and refusal boundaries."""

from __future__ import annotations

import hashlib
import shutil
import tempfile
from pathlib import Path

import settings_hmr as hmr


BASELINE = hmr.PROJECT / "experiments/native-storm/.cache/runtime"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy_fixture(root: Path) -> None:
    for relative in hmr.HOT_PATHS:
        target = root / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(BASELINE / relative, target)
    (root / "SAVE").mkdir()
    (root / "SAVE/untouched.txt").write_text("keep", encoding="utf-8")
    (root / "engine.ini").write_text("untouched\n", encoding="utf-8")


def snapshot(root: Path) -> dict[str, bytes]:
    return {
        str(path.relative_to(root)): path.read_bytes()
        for path in root.rglob("*")
        if path.is_file()
    }


def main() -> int:
    module = hmr.load_generator()
    specs = hmr.reviewed_specs(module)
    assert set(specs) == set(hmr.HOT_PATHS)

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-") as name:
        root = Path(name)
        copy_fixture(root)
        before = snapshot(root)
        changed = hmr.sync_once(root)
        after = snapshot(root)
        assert set(changed) == set(hmr.HOT_PATHS), changed
        assert set(after) == set(before)
        assert after["SAVE/untouched.txt"] == before["SAVE/untouched.txt"]
        assert after["engine.ini"] == before["engine.ini"]
        for relative in hmr.HOT_PATHS:
            assert digest(root / relative) == specs[relative].patched_sha256
        assert not any(path.name.startswith(".option_") for path in root.rglob("*"))
        assert hmr.sync_once(root) == ()

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-refuse-") as name:
        root = Path(name)
        copy_fixture(root)
        damaged = root / hmr.HOT_PATHS[1]
        damaged.write_bytes(damaged.read_bytes() + b"\nunknown runtime edit\n")
        before = snapshot(root)
        try:
            hmr.sync_once(root)
        except ValueError as error:
            assert "unsupported source hash" in str(error)
        else:
            raise AssertionError("unknown runtime revision was accepted")
        assert snapshot(root) == before

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-atomic-") as name:
        root = Path(name)
        copy_fixture(root)
        before = snapshot(root)
        calls = 0

        def fail_second_write(path: Path, data: bytes) -> None:
            nonlocal calls
            calls += 1
            if calls == 2:
                raise OSError("injected atomic replacement failure")
            hmr.atomic_write(path, data)

        try:
            hmr.sync_once(root, writer=fail_second_write)
        except OSError as error:
            assert "injected atomic replacement failure" in str(error)
        else:
            raise AssertionError("injected writer failure was ignored")
        assert snapshot(root) == before

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-watch-") as name:
        root = Path(name)
        copy_fixture(root)
        generator = root / "generator.py"
        generator.write_bytes(hmr.GENERATOR.read_bytes())
        signature = hmr.watch_signature(root, generator)
        generator.write_bytes(generator.read_bytes() + b"\n# probe change\n")
        assert hmr.watch_signature(root, generator) != signature

    print("settings HMR probe: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
