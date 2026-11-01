#!/usr/bin/env python3
"""Comprehensive Dev Mode with Hot Module Reloading (HMR) and Engine Recompilation.

Watches:
1. Canonical gameplay sources in src/gameplay/ (scripts, interfaces, INI, dialogues, weather, particles).
   Atomic synchronization to experiments/native-metal/.cache/runtime and
   /Applications/Corsairs Iddictive Remaster.app without restarting the game.
2. Engine C++, Objective-C++, and Metal shader sources in experiments/native-metal/.
   Automatic incremental ninja rebuilds with ms timing, clean diagnostics,
   staged engine update, and candidate installation when the game is idle.
3. Interactive Dev CLI for manual triggers, status inspection, and game supervision.
"""

from __future__ import annotations

import argparse
import datetime
import hashlib
import json
import os
import select
import shutil
import subprocess
import sys
import threading
import time
import types
from pathlib import Path
from typing import Sequence

ROOT = Path(__file__).resolve().parents[1]
TOOLS = ROOT / "tools"
METAL = ROOT / "experiments/native-metal"
DEFAULT_RUNTIME = METAL / ".cache/runtime"
INSTALLED_APP = Path(os.environ.get("CORSAIRS_INSTALLED_APP", "/Applications/Corsairs Iddictive Remaster.app"))
ENGINE_BIN = METAL / ".cache/build/bin/engine-1"
STAGED_ENGINE = METAL / ".cache/CorsairsMetal.app/Contents/MacOS/metal-engine"

# Ensure environment has PIL dummy runtime fallback
DEV_RUNTIME = TOOLS / ".dev_runtime"
if str(DEV_RUNTIME) not in os.environ.get("PYTHONPATH", ""):
    os.environ["PYTHONPATH"] = f"{DEV_RUNTIME}:{os.environ.get('PYTHONPATH', '')}"

if "PIL" not in sys.modules:
    pil = types.ModuleType("PIL")
    pil.Image = pil.ImageChops = pil.ImageDraw = pil.ImageFilter = types.ModuleType("PIL.dummy")
    sys.modules["PIL"] = pil
    sys.modules["PIL.Image"] = pil.Image
    sys.modules["PIL.ImageChops"] = pil.ImageChops
    sys.modules["PIL.ImageDraw"] = pil.ImageDraw
    sys.modules["PIL.ImageFilter"] = pil.ImageFilter

sys.path.insert(0, str(TOOLS))
import gameplay_sources  # noqa: E402
import metal_engine_source  # noqa: E402
from delivery_state import DeliveryState, RECEIPT, atomic_write, transact  # noqa: E402


def timestamp() -> str:
    return datetime.datetime.now().strftime("%H:%M:%S")


def is_game_running() -> bool:
    try:
        ps_proc = subprocess.run(["ps", "-axo", "comm="], capture_output=True, text=True, check=False)
        for line in ps_proc.stdout.splitlines():
            cmd = line.strip()
            if cmd.endswith(("/metal-engine", "/native-engine", "/engine-1", "engine.exe")):
                return True
    except OSError:
        pass

    engines = [str(STAGED_ENGINE), str(ENGINE_BIN)]
    if INSTALLED_APP.is_dir():
        engines.append(str(INSTALLED_APP / "Contents/MacOS/metal-engine"))

    try:
        holders = subprocess.run(
            ["/usr/sbin/lsof", "-t", "--", *engines],
            capture_output=True, text=True, check=False
        )
        if holders.returncode == 0 and holders.stdout.strip():
            return True
    except OSError:
        pass

    return False


class GameplayHMR:
    """Watches canonical gameplay sources in src/gameplay/ and hot-syncs them."""

    def __init__(self, runtime: Path, app: Path = INSTALLED_APP):
        self.runtime = runtime.resolve()
        self.app = app.resolve()
        self.app_resources = self.app / "Contents/Resources" if self.app.is_dir() else None
        self.source_root = gameplay_sources.ROOT
        self.manifest_path = self.source_root / "manifest.json"
        self._last_signatures: dict[Path, tuple[int, int]] = {}
        self.sync_stats = {
            "total_syncs": 0,
            "files_synced": 0,
            "last_synced": "None",
            "last_error": None
        }

    def scan_files(self) -> dict[Path, tuple[int, int]]:
        files = {}
        if self.manifest_path.is_file():
            stat = self.manifest_path.stat()
            files[self.manifest_path] = (stat.st_mtime_ns, stat.st_size)

        var_manifest = self.source_root / "variants.json"
        if var_manifest.is_file():
            stat = var_manifest.stat()
            files[var_manifest] = (stat.st_mtime_ns, stat.st_size)

        try:
            source_set = gameplay_sources.read()
            for relative in source_set[0]:
                src_path = self.source_root / relative
                if src_path.is_file():
                    stat = src_path.stat()
                    files[src_path] = (stat.st_mtime_ns, stat.st_size)
        except Exception:
            for p in self.source_root.rglob("*"):
                if p.is_file() and not p.name.startswith("."):
                    stat = p.stat()
                    files[p] = (stat.st_mtime_ns, stat.st_size)

        return files

    def has_changes(self) -> bool:
        current = self.scan_files()
        if not self._last_signatures:
            self._last_signatures = current
            return False
        if current != self._last_signatures:
            self._last_signatures = current
            return True
        return False

    def sync(self, paths: Sequence[str] | None = None) -> list[str]:
        source_set = gameplay_sources.read(paths)
        runtime_state = DeliveryState(self.runtime)
        runtime_changes = gameplay_sources.prepare_delivery(
            self.runtime, source_set, state=runtime_state, development=False
        )

        app_state = DeliveryState(self.app_resources, self.app) if (self.app_resources and self.app_resources.is_dir()) else None
        app_changes = gameplay_sources.prepare_delivery(
            self.app_resources, source_set, state=app_state, development=False
        ) if app_state is not None else {}

        changed_relatives = set()
        for p, (before, after) in runtime_changes.items():
            if p.is_relative_to(self.runtime) and p.name != RECEIPT and before != after:
                changed_relatives.add(p.relative_to(self.runtime).as_posix())

        if app_state is not None:
            for p, (before, after) in app_changes.items():
                if p.is_relative_to(self.app_resources) and p.name != RECEIPT and before != after:
                    changed_relatives.add(p.relative_to(self.app_resources).as_posix())

        batch = dict(runtime_changes)
        batch.update(app_changes)

        if any(before != after for before, after in batch.values()):
            transact(batch, app=None, writer=atomic_write)
            changed_list = sorted(changed_relatives)
            self.sync_stats["total_syncs"] += 1
            self.sync_stats["files_synced"] += len(changed_list)
            self.sync_stats["last_synced"] = f"{len(changed_list)} files at {timestamp()}"
            self.sync_stats["last_error"] = None
            return changed_list

        return []


class EngineBuilder:
    """Watches native Metal engine, renderer, and shader sources and rebuilds incrementally."""

    def __init__(self, metal_root: Path = METAL):
        self.metal_root = metal_root.resolve()
        self.build_dir = self.metal_root / ".cache/build"
        self.staged_engine = STAGED_ENGINE
        self.engine_bin = ENGINE_BIN
        self._last_signatures: dict[Path, tuple[int, int]] = {}
        self.build_stats = {
            "rebuild_count": 0,
            "last_status": "Idle",
            "last_duration_ms": 0.0,
            "last_fingerprint": "",
            "last_error": None
        }

    def scan_files(self) -> dict[Path, tuple[int, int]]:
        files = {}
        try:
            for p in metal_engine_source.source_files():
                if p.is_file():
                    stat = p.stat()
                    files[p] = (stat.st_mtime_ns, stat.st_size)
        except Exception:
            pass
        return files

    def has_changes(self) -> bool:
        current = self.scan_files()
        if not self._last_signatures:
            self._last_signatures = current
            return False
        if current != self._last_signatures:
            self._last_signatures = current
            return True
        return False

    def build(self, jobs: int = 4) -> tuple[bool, float, str]:
        t0 = time.perf_counter()
        self.build_stats["last_status"] = "Building..."

        toolchain_bin = self.metal_root / "inputs/toolchain/bin"
        env = dict(os.environ)
        if toolchain_bin.is_dir():
            env["PATH"] = f"{toolchain_bin}:{env.get('PATH', '')}"

        if not (self.build_dir / "build.ninja").is_file():
            conf_proc = subprocess.run([str(self.metal_root / "build.sh")], env=env, capture_output=True, text=True, check=False)
            dt = (time.perf_counter() - t0) * 1000.0
            if conf_proc.returncode != 0:
                err_text = conf_proc.stderr or conf_proc.stdout or "Configure failed"
                self.build_stats["last_status"] = "Configure FAILED"
                self.build_stats["last_error"] = err_text
                return False, dt, err_text
            self.build_stats["last_status"] = "SUCCESS"
            self.build_stats["last_duration_ms"] = dt
            self.build_stats["rebuild_count"] += 1
            return True, dt, ""

        source_id = metal_engine_source.fingerprint()

        cmd = ["cmake", "--build", str(self.build_dir), "--target", "engine", "-j", str(jobs)]
        proc = subprocess.run(cmd, env=env, capture_output=True, text=True, check=False)
        dt = (time.perf_counter() - t0) * 1000.0

        if proc.returncode != 0:
            diag_lines = []
            output = (proc.stdout or "") + "\n" + (proc.stderr or "")
            for line in output.splitlines():
                if any(k in line for k in ("error:", "FAILED:", "undefined", "Undefined", "ld:", "fatal error:")):
                    diag_lines.append(line)
            diag_text = "\n".join(diag_lines) if diag_lines else output.strip()[-1000:]
            self.build_stats["last_status"] = "FAILED"
            self.build_stats["last_error"] = diag_text
            return False, dt, diag_text

        # Record candidate identity and update stage
        try:
            metal_engine_source.record(self.engine_bin, source_id)
            self.staged_engine.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(self.engine_bin, self.staged_engine)
        except Exception as error:
            self.build_stats["last_status"] = f"Record failed: {error}"
            self.build_stats["last_error"] = str(error)
            return False, dt, str(error)

        self.build_stats["last_status"] = "SUCCESS"
        self.build_stats["last_duration_ms"] = dt
        self.build_stats["last_fingerprint"] = source_id[:16]
        self.build_stats["rebuild_count"] += 1
        self.build_stats["last_error"] = None
        return True, dt, ""

    def install_candidate(self) -> tuple[bool, str]:
        if is_game_running():
            return False, "Cannot install candidate: game is running. Close the game or press 'k'."
        if not self.engine_bin.is_file():
            return False, "Candidate binary missing; rebuild first."
        if not INSTALLED_APP.is_dir():
            return True, "Installed app missing; staged app is ready."

        cmd = [str(self.metal_root / "install-engine.sh")]
        env = dict(os.environ)
        if str(DEV_RUNTIME) not in env.get("PYTHONPATH", ""):
            env["PYTHONPATH"] = f"{DEV_RUNTIME}:{env.get('PYTHONPATH', '')}"

        proc = subprocess.run(cmd, env=env, capture_output=True, text=True, check=False)
        if proc.returncode != 0:
            return False, (proc.stderr or proc.stdout).strip()
        return True, proc.stdout.strip()


class DevModeSupervisor:
    """Coordinates watchers, interactive CLI, and optional game supervision."""

    def __init__(self, runtime: Path = DEFAULT_RUNTIME, app: Path = INSTALLED_APP, interval: float = 0.25):
        self.runtime = runtime.resolve()
        self.app = app.resolve()
        self.interval = interval
        self.gameplay_hmr = GameplayHMR(self.runtime, self.app)
        self.engine_builder = EngineBuilder()
        self.running = False
        self.game_process: subprocess.Popen | None = None

    def launch_game(self) -> None:
        if is_game_running():
            print(f"[{timestamp()}] Game is already running.", flush=True)
            return
        cmd = [str(METAL / "run.sh"), "--launch-installed"]
        print(f"[{timestamp()}] Launching game: {' '.join(cmd)}", flush=True)
        try:
            self.game_process = subprocess.Popen(cmd)
        except Exception as err:
            print(f"[{timestamp()}] Failed to launch game: {err}", file=sys.stderr, flush=True)

    def close_game(self) -> None:
        if self.game_process and self.game_process.poll() is None:
            self.game_process.terminate()
            try:
                self.game_process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.game_process.kill()
        subprocess.run(["pkill", "-f", "(metal-engine|native-engine|engine-1)"], check=False)
        print(f"[{timestamp()}] Game processes closed.", flush=True)

    def print_status(self) -> None:
        gp_count = len(self.gameplay_hmr.scan_files())
        eng_count = len(self.engine_builder.scan_files())
        game_status = "RUNNING" if is_game_running() else "STOPPED"
        print("\n" + "=" * 65)
        print(" Corsairs Dev Mode (HMR & Live Engine Recompile)")
        print("=" * 65)
        print(f" Status:             Game [{game_status}] | Watcher [ACTIVE]")
        print(f" Gameplay HMR:       {gp_count} files watched | Syncs: {self.gameplay_hmr.sync_stats['total_syncs']} ({self.gameplay_hmr.sync_stats['last_synced']})")
        print(f" Engine Recompile:   {eng_count} files watched | Builds: {self.engine_builder.build_stats['rebuild_count']} (Last: {self.engine_builder.build_stats['last_status']})")
        print(f" Staged Engine:      {STAGED_ENGINE}")
        print(f" Installed App:      {INSTALLED_APP}")
        print("-" * 65)
        print(" Commands: [r]esync/rebuild  [s]ync gameplay  [b]uild engine")
        print("           [i]nstall engine  [l]aunch game    [k]ill game  [q]uit")
        print("=" * 65 + "\n", flush=True)

    def tick(self) -> None:
        if self.gameplay_hmr.has_changes():
            try:
                changed = self.gameplay_hmr.sync()
                if changed:
                    print(f"[{timestamp()}] [Gameplay HMR] Synced {len(changed)} file(s):", flush=True)
                    for item in changed:
                        print(f"    -> {item}", flush=True)
            except Exception as error:
                print(f"[{timestamp()}] [Gameplay HMR] Sync refused: {error}", file=sys.stderr, flush=True)

        if self.engine_builder.has_changes():
            print(f"[{timestamp()}] [Engine Rebuild] Engine source change detected. Rebuilding...", flush=True)
            ok, duration, diag = self.engine_builder.build()
            if ok:
                print(f"[{timestamp()}] [Engine Rebuild] SUCCESS in {duration:.1f}ms (candidate staged).", flush=True)
                if not is_game_running():
                    installed_ok, msg = self.engine_builder.install_candidate()
                    if installed_ok:
                        print(f"[{timestamp()}] [Engine Rebuild] Candidate installed into /Applications app.", flush=True)
                    else:
                        print(f"[{timestamp()}] [Engine Rebuild] Install notice: {msg}", flush=True)
                else:
                    print(f"[{timestamp()}] [Engine Rebuild] Game running; candidate install ready upon game exit or 'i'.", flush=True)
            else:
                print(f"[{timestamp()}] [Engine Rebuild] FAILED in {duration:.1f}ms:\n{diag}", file=sys.stderr, flush=True)

    def handle_command(self, cmd: str) -> bool:
        cmd = cmd.strip().lower()
        if not cmd:
            return True
        if cmd == "q":
            return False
        elif cmd == "r":
            print(f"[{timestamp()}] Forcing gameplay sync and engine check...", flush=True)
            try:
                changed = self.gameplay_hmr.sync()
                print(f"[{timestamp()}] Gameplay sync: {len(changed)} updated.")
            except Exception as e:
                print(f"[{timestamp()}] Gameplay sync error: {e}", file=sys.stderr)
            ok, dt, diag = self.engine_builder.build()
            print(f"[{timestamp()}] Engine build: {'OK' if ok else 'FAILED'} ({dt:.1f}ms)")
            if not ok:
                print(diag, file=sys.stderr)
        elif cmd == "s":
            print(f"[{timestamp()}] Syncing gameplay sources...", flush=True)
            try:
                changed = self.gameplay_hmr.sync()
                print(f"[{timestamp()}] Gameplay sync: {len(changed)} files updated.")
            except Exception as e:
                print(f"[{timestamp()}] Gameplay sync error: {e}", file=sys.stderr)
        elif cmd == "b":
            print(f"[{timestamp()}] Rebuilding engine...", flush=True)
            ok, dt, diag = self.engine_builder.build()
            print(f"[{timestamp()}] Engine build: {'OK' if ok else 'FAILED'} ({dt:.1f}ms)")
            if not ok:
                print(diag, file=sys.stderr)
        elif cmd == "i":
            print(f"[{timestamp()}] Installing engine candidate...", flush=True)
            ok, msg = self.engine_builder.install_candidate()
            print(f"[{timestamp()}] Result: {'SUCCESS' if ok else 'FAILED'} - {msg}")
        elif cmd == "l":
            self.launch_game()
        elif cmd == "k":
            self.close_game()
        elif cmd == "c" or cmd in ("h", "help", "?"):
            self.print_status()
        else:
            print(f"Unknown command: '{cmd}'. Press 'h' for help.", flush=True)
        return True

    def run(self, launch_game: bool = False) -> int:
        self.running = True
        self.print_status()

        if launch_game:
            self.launch_game()

        self.gameplay_hmr.scan_files()
        self.engine_builder.scan_files()

        try:
            while self.running:
                self.tick()

                # Poll stdin if available
                if sys.stdin in select.select([sys.stdin], [], [], 0)[0]:
                    line = sys.stdin.readline()
                    if not line:
                        break
                    if not self.handle_command(line):
                        break

                time.sleep(self.interval)
        except KeyboardInterrupt:
            print(f"\n[{timestamp()}] Dev Mode interrupted by user.", flush=True)
        finally:
            self.running = False
            print(f"[{timestamp()}] Dev Mode stopped.", flush=True)

        return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("runtime", nargs="?", type=Path, default=DEFAULT_RUNTIME, help="Target runtime directory")
    parser.add_argument("--interval", type=float, default=0.25, help="Polling interval in seconds")
    parser.add_argument("--sync-only", action="store_true", help="Sync once and exit")
    parser.add_argument("--build-only", action="store_true", help="Build engine once and exit")
    parser.add_argument("--launch", action="store_true", help="Launch the game alongside dev mode")
    parser.add_argument("--jobs", "-j", type=int, default=4, help="Compiler build jobs")
    args = parser.parse_args()

    if args.sync_only:
        hmr = GameplayHMR(args.runtime)
        changed = hmr.sync()
        if changed:
            print(f"[{timestamp()}] Synced {len(changed)} file(s): " + ", ".join(changed))
        else:
            print(f"[{timestamp()}] Reviewed runtime and app are up to date.")
        return 0

    if args.build_only:
        builder = EngineBuilder()
        ok, dt, diag = builder.build(jobs=args.jobs)
        if ok:
            print(f"[{timestamp()}] Engine rebuild SUCCESS in {dt:.1f}ms.")
            if not is_game_running():
                ins_ok, msg = builder.install_candidate()
                print(f"[{timestamp()}] Install: {'SUCCESS' if ins_ok else msg}")
            return 0
        else:
            print(f"[{timestamp()}] Engine rebuild FAILED in {dt:.1f}ms:\n{diag}", file=sys.stderr)
            return 1

    supervisor = DevModeSupervisor(runtime=args.runtime, interval=args.interval)
    return supervisor.run(launch_game=args.launch)


if __name__ == "__main__":
    raise SystemExit(main())

