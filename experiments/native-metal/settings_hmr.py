#!/usr/bin/env python3
"""Hot-sync the reviewed native-metal settings UI into a running game.

The settings screen and its save/load helper are dynamic Storm script segments.
Closing the settings screen unloads them, and opening it again loads these files
from the runtime.  This tool therefore updates only those reviewed hot files;
engine, renderer, string-table and gameplay changes still use normal staging.
"""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import sys
import time
from pathlib import Path
from types import ModuleType
from typing import Callable


ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[1]
TOOLS = PROJECT / "tools"
GENERATOR = TOOLS / "metal_graphics_settings.py"
DEFAULT_RUNTIME = ROOT / ".cache/runtime"
HOT_PATHS = (
    "PROGRAM/interface/option_sl.c",
    "PROGRAM/interface/option_screen.c",
    "RESOURCE/INI/interfaces/option_screen.ini",
)

sys.path.insert(0, str(TOOLS))
from runtime_script_patch import atomic_write  # noqa: E402


def sha256(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def load_generator(path: Path = GENERATOR) -> ModuleType:
    source = path.read_bytes()
    module_name = f"metal_graphics_settings_hmr_{sha256(source)[:16]}"
    spec = importlib.util.spec_from_file_location(module_name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"cannot load settings generator: {path}")
    module = importlib.util.module_from_spec(spec)
    sys.modules[module_name] = module
    try:
        spec.loader.exec_module(module)
    except Exception:
        sys.modules.pop(module_name, None)
        raise
    return module


def reviewed_specs(module: ModuleType) -> dict[str, object]:
    specs: dict[str, object] = {}
    for item in module.FILES:
        relative = item.relative_path
        if relative in HOT_PATHS:
            if relative in specs:
                raise RuntimeError(f"duplicate reviewed settings file: {relative}")
            specs[relative] = item
    missing = set(HOT_PATHS) - set(specs)
    if missing:
        raise RuntimeError(f"settings generator does not own hot file(s): {sorted(missing)}")
    return specs


def sync_once(
    runtime: Path,
    *,
    generator_path: Path = GENERATOR,
    writer: Callable[[Path, bytes], None] = atomic_write,
) -> tuple[str, ...]:
    """Verify all hot inputs, then replace changed files atomically.

    No write occurs until every current runtime file is accepted by the reviewed
    generator and every generated output matches its pinned final hash.
    """

    runtime = runtime.resolve()
    module = load_generator(generator_path)
    specs = reviewed_specs(module)
    before: dict[str, bytes] = {}
    generated: dict[str, bytes] = {}

    for relative in HOT_PATHS:
        target = runtime / relative
        if not target.is_file():
            raise RuntimeError(f"settings HMR target is missing: {target}")
        source = target.read_bytes()
        output, _state = module.prepare(relative, source)
        expected = specs[relative].patched_sha256
        if not expected or sha256(output) != expected:
            raise RuntimeError(f"{relative}: generated output is not the reviewed final hash")
        before[relative] = source
        generated[relative] = output

    # Refuse a concurrent runtime edit between verification and replacement.
    for relative in HOT_PATHS:
        if (runtime / relative).read_bytes() != before[relative]:
            raise RuntimeError(f"{relative}: changed during settings HMR preflight")

    changed = tuple(relative for relative in HOT_PATHS if generated[relative] != before[relative])
    written: list[str] = []
    try:
        for relative in changed:
            target = runtime / relative
            if target.read_bytes() != before[relative]:
                raise RuntimeError(f"{relative}: changed during settings HMR replacement")
            writer(target, generated[relative])
            written.append(relative)
        for relative in HOT_PATHS:
            if (runtime / relative).read_bytes() != generated[relative]:
                raise RuntimeError(f"{relative}: settings HMR verification failed")
    except Exception as error:
        failures = []
        for relative in reversed(written):
            target = runtime / relative
            try:
                if target.read_bytes() != generated[relative]:
                    raise RuntimeError(f"{relative}: concurrent change prevents settings HMR rollback")
                writer(target, before[relative])
            except Exception as rollback_error:
                failures.append(str(rollback_error))
        if failures:
            raise RuntimeError(f"{error}; settings HMR rollback incomplete: {'; '.join(failures)}") from error
        raise
    return changed


def watch_signature(runtime: Path, generator_path: Path = GENERATOR) -> str:
    digest = hashlib.sha256()
    paths = (generator_path,) + tuple(runtime / relative for relative in HOT_PATHS)
    for path in paths:
        digest.update(str(path).encode("utf-8"))
        try:
            digest.update(path.read_bytes())
        except FileNotFoundError:
            digest.update(b"<missing>")
    return digest.hexdigest()


def watch(runtime: Path, interval: float) -> None:
    last_attempt = ""
    print(f"Settings HMR: watching {GENERATOR}", flush=True)
    print("Close and reopen Settings after each synced change.", flush=True)
    while True:
        signature = watch_signature(runtime)
        if signature != last_attempt:
            last_attempt = signature
            try:
                changed = sync_once(runtime)
                if changed:
                    print("Settings HMR: synced " + ", ".join(changed), flush=True)
                else:
                    print("Settings HMR: reviewed runtime is current", flush=True)
                # Recompute after our own writes so they do not trigger a second pass.
                last_attempt = watch_signature(runtime)
            except Exception as error:
                print(f"Settings HMR: refused: {error}", file=sys.stderr, flush=True)
        time.sleep(interval)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", nargs="?", type=Path, default=DEFAULT_RUNTIME)
    parser.add_argument("--once", action="store_true", help="sync once and exit")
    parser.add_argument("--interval", type=float, default=0.25, help="watch interval in seconds")
    args = parser.parse_args()
    if args.interval <= 0:
        parser.error("--interval must be greater than zero")
    try:
        if args.once:
            changed = sync_once(args.runtime)
            if changed:
                print("Settings HMR: synced " + ", ".join(changed))
            else:
                print("Settings HMR: reviewed runtime is current")
            return 0
        watch(args.runtime.resolve(), args.interval)
    except KeyboardInterrupt:
        print("Settings HMR: stopped")
    except Exception as error:
        print(f"Settings HMR: refused: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
