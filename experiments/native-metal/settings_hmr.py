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
import sys
import time
from pathlib import Path
from typing import Callable

ROOT = Path(__file__).resolve().parent
PROJECT = ROOT.parents[1]
TOOLS = PROJECT / "tools"
DEFAULT_RUNTIME = ROOT / ".cache/runtime"
HOT_PATHS = (
    "PROGRAM/interface/option_sl.c",
    "PROGRAM/interface/option_screen.c",
    "RESOURCE/INI/interfaces/option_screen.ini",
)

sys.path.insert(0, str(TOOLS))
from delivery_state import DeliveryState, RECEIPT, atomic_write, transact  # noqa: E402
import gameplay_sources  # noqa: E402


def sync_once(
    runtime: Path,
    *,
    writer: Callable[[Path, bytes], None] = atomic_write,
) -> tuple[str, ...]:
    """Verify hot inputs against canonical sources, then replace changed files atomically.

    Consumes the shared canonical delivery projection and state transaction without
    imposing player-idle guards or app bundle sealing.
    """
    runtime = runtime.resolve()
    for relative in HOT_PATHS:
        target = runtime / relative
        if not target.is_file():
            raise RuntimeError(f"settings HMR target is missing: {target}")

    source_set = gameplay_sources.read(HOT_PATHS)
    state = DeliveryState(runtime)
    changes = gameplay_sources.prepare_delivery(runtime, source_set, state=state, development=False)

    changed = tuple(
        relative for relative in HOT_PATHS
        if (runtime / relative) in changes and changes[runtime / relative][0] != changes[runtime / relative][1]
    )

    if any(before != after for before, after in changes.values()):
        transact(changes, app=None, writer=writer)

    return changed


def watch_signature(
    runtime: Path,
    manifest_path: Path | None = None,
    source_root: Path | None = None,
) -> str:
    digest = hashlib.sha256()
    sources = source_root if source_root is not None else gameplay_sources.ROOT
    manifest = manifest_path if manifest_path is not None else sources / "manifest.json"
    receipt = runtime / RECEIPT
    paths = (manifest, receipt) + tuple(sources / relative for relative in HOT_PATHS) + tuple(runtime / relative for relative in HOT_PATHS)
    for path in paths:
        digest.update(str(path).encode("utf-8"))
        try:
            digest.update(path.read_bytes())
        except FileNotFoundError:
            digest.update(b"<missing>")
    return digest.hexdigest()


def watch(runtime: Path, interval: float) -> None:
    last_attempt = ""
    print(f"Settings HMR: watching canonical sources, {gameplay_sources.ROOT / 'manifest.json'} and {runtime / RECEIPT}", flush=True)
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
