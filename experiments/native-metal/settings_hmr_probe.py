#!/usr/bin/env python3
"""Verify native-metal settings HMR ownership and refusal boundaries."""

from __future__ import annotations

import hashlib
import shutil
import tempfile
from pathlib import Path

import settings_hmr as hmr

INPUTS = hmr.PROJECT / "experiments/native-metal/inputs"
BASELINE = (
    INPUTS / "gameplay"
    if (INPUTS / "manifest.json").is_file()
    else hmr.PROJECT / "experiments/native-storm/.cache/runtime"
)
COLD_STRINGTABLE = "RESOURCE/INI/texts/russian/common.ini"


def digest(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def copy_fixture(root: Path) -> None:
    for relative in hmr.HOT_PATHS + (COLD_STRINGTABLE,):
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
    source_set = hmr.gameplay_sources.read(hmr.HOT_PATHS)
    expected_hashes = {
        relative: hashlib.sha256(source_set[0][relative][0]).hexdigest()
        for relative in hmr.HOT_PATHS
    }

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-") as name:
        root = Path(name).resolve()
        copy_fixture(root)
        before = snapshot(root)
        changed = hmr.sync_once(root)
        after = snapshot(root)
        assert set(changed) == set(hmr.HOT_PATHS), changed
        assert set(after) == set(before) | {hmr.RECEIPT}
        assert after["SAVE/untouched.txt"] == before["SAVE/untouched.txt"]
        assert after["engine.ini"] == before["engine.ini"]
        assert after[COLD_STRINGTABLE] == before[COLD_STRINGTABLE]
        for relative in hmr.HOT_PATHS:
            assert digest(root / relative) == expected_hashes[relative]
        assert not any(path.name.startswith(".option_") for path in root.rglob("*"))
        assert hmr.sync_once(root) == ()
        assert snapshot(root) == after

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-missing-") as name:
        root = Path(name).resolve()
        copy_fixture(root)
        (root / hmr.HOT_PATHS[0]).unlink()
        before = snapshot(root)
        try:
            hmr.sync_once(root)
        except RuntimeError as error:
            assert "settings HMR target is missing" in str(error)
        else:
            raise AssertionError("missing target was accepted")
        assert snapshot(root) == before

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-refuse-") as name:
        root = Path(name).resolve()
        copy_fixture(root)
        damaged = root / hmr.HOT_PATHS[1]
        damaged.write_bytes(damaged.read_bytes() + b"\nunknown runtime edit\n")
        before = snapshot(root)
        try:
            hmr.sync_once(root)
        except (RuntimeError, ValueError) as error:
            assert "Unknown delivered-file edit" in str(error) or "unsupported source hash" in str(error) or "Unrecognized legacy delivery" in str(error)
        else:
            raise AssertionError("unknown runtime revision was accepted")
        assert snapshot(root) == before

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-atomic-") as name:
        root = Path(name).resolve()
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

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-verify-fail-") as name:
        root = Path(name).resolve()
        p1 = root / "PROGRAM/interface/option_sl.c"
        p1.parent.mkdir(parents=True)
        p1.write_bytes(b"old_target")
        p_src = root / "PROGRAM/interface/source.c"
        p_src.write_bytes(b"source_input")
        changes = {
            p_src: (b"source_input", b"source_input"),
            p1: (b"old_target", b"new_target"),
        }

        def tamper_source_on_write(path: Path, data: bytes) -> None:
            hmr.atomic_write(path, data)
            p_src.write_bytes(b"tampered_source")

        try:
            hmr.transact(changes, writer=tamper_source_on_write)
        except RuntimeError as error:
            assert "Delivery verification failed" in str(error)
        else:
            raise AssertionError("verification failure was ignored")
        assert p1.read_bytes() == b"old_target"
        assert p_src.read_bytes() == b"tampered_source"

    with tempfile.TemporaryDirectory(prefix="corsairs-settings-hmr-watch-") as name:
        root = Path(name).resolve()
        copy_fixture(root)
        source_root = root / "sources"
        for relative in hmr.HOT_PATHS:
            target = source_root / relative
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(hmr.gameplay_sources.ROOT / relative, target)
        manifest = source_root / "manifest.json"
        manifest.write_bytes((hmr.gameplay_sources.ROOT / "manifest.json").read_bytes())
        receipt = root / hmr.RECEIPT
        receipt.write_text('{"version": 1, "files": {}}\n', encoding="utf-8")

        base_sig = hmr.watch_signature(root, source_root=source_root)

        # 1) Hot source file mutation triggers signature invalidation independently
        hot_src = source_root / hmr.HOT_PATHS[0]
        hot_src.write_bytes(hot_src.read_bytes() + b"\n// hot edit\n")
        source_sig = hmr.watch_signature(root, source_root=source_root)
        assert source_sig != base_sig

        # 2) Receipt mutation triggers signature invalidation independently
        receipt.write_text('{"version": 1, "files": {"repaired": true}}\n', encoding="utf-8")
        receipt_sig = hmr.watch_signature(root, source_root=source_root)
        assert receipt_sig != source_sig

        # 3) Manifest mutation triggers signature invalidation independently
        manifest.write_bytes(manifest.read_bytes() + b"\n")
        manifest_sig = hmr.watch_signature(root, source_root=source_root)
        assert manifest_sig != receipt_sig

    print("settings HMR probe: ok")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
