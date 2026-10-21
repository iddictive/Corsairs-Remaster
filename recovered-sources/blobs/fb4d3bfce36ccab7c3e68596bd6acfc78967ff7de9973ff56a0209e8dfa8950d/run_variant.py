#!/usr/bin/env python3
"""Run the engine-linked hidden fleet diagnostic, never the game executable."""
from __future__ import annotations
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import re
import math
import statistics
import subprocess
import time
import sys
sys.path.insert(0,'/REQUIRED_EXTERNAL_INPUT/Corsairs/tools')
from metal_storage import diagnostic_run, ensure_build_size, ensure_space, maintain

ROOT = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
NATIVE = ROOT / "experiments/native-metal"
APP = Path("/Applications/Corsairs Iddictive Remaster.app")


def distribution(values: list[float]) -> dict[str, float]:
    ordered = sorted(values)
    if not ordered or not all(math.isfinite(x) for x in ordered):
        raise ValueError("missing or invalid timing population")
    return {"median": statistics.median(ordered),
            "p95": ordered[math.ceil(.95 * len(ordered)) - 1],
            "p99": ordered[math.ceil(.99 * len(ordered)) - 1], "max": max(ordered)}


def summarize(run: Path, frames: int, warmup: int) -> dict:
    cpu = list(csv.DictReader((run / "cpu-frames.csv").open()))
    trace = list(csv.DictReader((run / "renderer-frames.csv").open()))
    rendered = {int(row["frame"]): row for row in trace if row["record"] == "frame"}
    commands: dict[int, list[tuple[float, float]]] = {}
    for row in trace:
        if row["record"] != "command":
            continue
        start, end = float(row["gpu_start_s"]), float(row["gpu_end_s"])
        if row["status"] != "4" or start <= 0 or end < start:
            raise ValueError("invalid or failed completed GPU command")
        commands.setdefault(int(row["frame"]), []).append((start, end))
    expected = set(range(frames))
    if len(cpu) != frames or set(rendered) != expected or set(commands) != expected:
        raise ValueError("CPU/render/GPU frame populations do not match")
    warm = list(range(warmup, frames))
    if not warm:
        raise ValueError("no warmed measurement population")
    core_wall = [float(cpu[f]["cpu_wall_ms"]) for f in warm]
    preparation = [float(cpu[f]["cpu_wall_ms"]) - float(rendered[f]["gpu_wait_ms"])
                   - float(rendered[f]["drawable_wait_ms"]) for f in warm]
    completions = [max(end for _, end in commands[f]) for f in warm]
    # All intervals belong to one command queue. Merge before adding; never
    # sum overlapping stages or mistake dispatch time for frame time.
    gpu = []
    for frame in warm:
        merged = []
        for start, end in sorted(commands[frame]):
            if merged and start <= merged[-1][1]:
                merged[-1] = (merged[-1][0], max(merged[-1][1], end))
            else:
                merged.append((start, end))
        gpu.append(sum(end - start for start, end in merged) * 1000)
    coverage = {key: min(int(rendered[f][key]) for f in warm)
                for key in ("sails", "animated", "sea_draws", "sky_current", "shadow_cubes_complete")}
    summary = {"warmup_frames": warmup, "measured_frames": len(warm),
               "percentiles": "nearest rank; median uses middle pair",
               "cpu_core_wall_ms": distribution(core_wall),
               "cpu_core_wall_mean_ms": statistics.mean(core_wall),
               "cpu_preparation_estimate_ms": distribution(preparation),
               "main_completion_cadence_mean_ms": ((completions[-1] - completions[0]) * 1000
                                                    / (len(completions) - 1)
                                                    if len(completions) > 1 else None),
               "gpu_execution_union_ms": distribution(gpu), "required_pass_minima": coverage,
               "buffer_allocations": distribution([float(rendered[f]["buffers"]) for f in warm]),
               "upload_bytes": distribution([float(rendered[f]["upload_bytes"]) for f in warm]),
               "draws": distribution([float(rendered[f]["draws"]) for f in warm]),
               "max_gpu_resource_bytes": max(int(rendered[f]["gpu_memory"]) for f in warm),
               "max_rss_bytes": max(int(cpu[f]["rss_bytes"]) for f in warm),
               "workload_coverage_pass": all(coverage.values()), "performance_acceptance": False}
    (run / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    return summary


def read(path: Path) -> str:
    return path.read_text(encoding="utf-8-sig").replace("\r", "")


def function(text: str, signature: str) -> str:
    if text.count(signature) != 1:
        raise ValueError(f"ambiguous function: {signature}")
    start = text.index(signature)
    opening = text.index("{", start)
    # These selected stock helpers have no brace-containing string literals.
    depth = 1
    end = opening + 1
    while depth and end < len(text):
        depth += (text[end] == "{") - (text[end] == "}")
        end += 1
    if depth:
        raise ValueError(f"incomplete function: {signature}")
    return text[start:end] + "\n"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--frames", type=int, default=8)
    parser.add_argument("--warmup", type=int, default=128)
    parser.add_argument("--width", type=int, default=2560)
    parser.add_argument("--height", type=int, default=1655)
    parser.add_argument("--ships", type=int, default=8, choices=range(1, 17))
    parser.add_argument("--profile", choices=("metal", "cpu"), help="diagnostic trace; timings are not acceptance evidence")
    parser.add_argument("--capture", action="store_true", help="capture one warmed sea frame through MTLCaptureManager")
    args = parser.parse_args()
    # Never compete with a live game or stop it to make room for this workload.
    processes = subprocess.check_output(["ps", "-axo", "pid=,comm="], text=True)
    if any("/metal-engine" in row or row.strip().endswith("/engine-1") for row in processes.splitlines()):
        raise RuntimeError("game process present; fleet GPU measurement deferred")
    binary = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/destruction-scene/candidate-scene-chip-anchor/destruction-benchmark')
    if not binary.is_file():
        raise RuntimeError("build the sea-fleet-benchmark CMake target first")
    ensure_build_size()
    maintain()
    ensure_space(path=NATIVE / ".cache/build")
    source_identity = json.loads(Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/destruction-scene/candidate-scene-chip-anchor/plan.json').read_text())["compiled_source_identity"]
    # CMake's CONFIGURE_DEPENDS glob check always appears in a Ninja dry run;
    # use the real incremental target before measuring, then bind its inputs.
    (NATIVE / ".cache/diagnostics").mkdir(parents=True, exist_ok=True)
    plan=json.loads(Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/destruction-scene/candidate-scene-chip-anchor/plan.json').read_text())
    for name,expected in {**plan['input_sha256'],**plan['linked_input_sha256'],**plan['header_closure']['files'],**plan['header_closure']['installed_dependency_sha256']}.items():
        if hashlib.sha256(Path(name).read_bytes()).hexdigest()!=expected:
            raise RuntimeError('scene packet input changed: '+name)
    resources = APP / "Contents/Resources"
    sources = {
        "ship-catalog": resources / "PROGRAM/ships/ships_init.c",
        "ship-lights": resources / "PROGRAM/SEA_AI/ShipLights.c",
        "day": resources / "PROGRAM/Weather/Init/Day.c",
        "units": resources / "PROGRAM/utils.c",
        "native-declarations": resources / "PROGRAM/seadogs.c",
        "cannon-declarations": resources / "PROGRAM/cannons/cannons.h",
        "fixture": Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/destruction-scene/candidate-scene-chip-anchor/fixture.c'),
    }
    # Resolve all inputs before creating a new run; missing assets must fail.
    texts = {name: read(path) for name, path in sources.items()}
    model = resources / "RESOURCE/MODELS/Ships/Flyingdutchman1/flyingdutchman1.gm"
    if not model.is_file():
        raise RuntimeError("required Flyingdutchman1 asset absent")
    ship=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/destruction-scene/candidate-scene-chip-anchor/ship-type.c').read_text()
    declarations = re.findall(r"^native [^\n]+;", texts["native-declarations"], re.M)
    if len(declarations) != 11:
        raise ValueError("native declaration population changed")
    run = NATIVE / ".cache/diagnostics/sea-fleet-benchmark" / time.strftime("%Y%m%d-%H%M%S")
    with diagnostic_run(run) as (tooltemp, budget):
        return run_workload(args, run, tooltemp, budget, resources, texts, ship, declarations,
                            binary, source_identity, sources, model)


def run_workload(args, run, tooltemp, budget, resources, texts, ship, declarations,
                 binary, source_identity, sources, model):
    (run / "RESOURCE").symlink_to(resources / "RESOURCE", target_is_directory=True)
    fixture = texts["fixture"]
    (run / "fixture.c").write_text(fixture)
    (run / "ship-type.c").write_text(ship)
    (run / "ship-lights.c").write_text(function(texts["ship-lights"], "void InitShipLights()"))
    (run / "day.c").write_text(texts["day"])
    (run / "native-declarations.c").write_text("\n".join(declarations) + "\n")
    (run / "engine.ini").write_text(
        f"full_screen=0\nscreen_x={args.width}\nscreen_y={args.height}\n"
        "program_directory=.\nrun=fixture.c\nmax_fps=0\nvsync=0\n"
        "texture_degradation=0\nUseLargeBackBuffer=1\nNearClipPlane=0.115\nFarClipPlane=32000\n"
        "startFontIniFile=resource\\ini\\fonts.ini\nfont=interface_normal\n"
        "logs=0\n[compatibility]\ntarget_version=teho\n[sea]\nfoamenable=1\n"
        "[script]\ndebuginfo=1\ncodefiles=0\nruntimelog=0\ntracefiles=0\n")
    import metal_graphics_settings as settings
    env = settings.launch_environment(Path.home() / "Library/Application Support/Iddictive Corsairs", dict(os.environ))
    env["TMPDIR"] = str(tooltemp) + os.sep
    env["STORM_METAL_UPSCALING"]="0"
    env.update(STORM_METAL_PROFILE="1", STORM_METAL_MODERN_EFFECTS="1", STORM_QUIET_CLIENT="1")
    env.pop("STORM_METAL_GPU_CAPTURE", None)
    env["STORM_METAL_FRAME_TRACE"] = str(run / "renderer-frames.csv")
    if args.capture:
        env["MTL_CAPTURE_ENABLED"] = "1"
        env["STORM_METAL_GPU_CAPTURE"] = str(run / "sea-frame.gputrace")
    manifest = {"pilot": True, "performance_acceptance": False,
                "binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(),
                "compiled_source_identity": source_identity, "baseline_ref": '87b4acb4e635af2c18526dcce3d433c8424f13d5',
                "inputs": {name: hashlib.sha256(path.read_bytes()).hexdigest() for name, path in sources.items()},
                "model_sha256": hashlib.sha256(model.read_bytes()).hexdigest(),
                "settings": {key: env[key] for key in (*settings.ENV_KEYS.values(), *settings.INTEGER_ENV_KEYS.values(),
                             "STORM_METAL_MODERN_EFFECTS", "STORM_METAL_PROFILE") if key in env},
                "backend_environment": {key: value for key, value in sorted(env.items())
                                        if key.startswith("STORM_METAL_")},
                "resolution": [args.width, args.height], "frames": args.frames, "ships": args.ships,
                "instrumented": True, "destruction_mode": env.get("DESTRUCTION_SCENE_MODE", "baseline"), "diagnostic_plan": '/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/destruction-scene/candidate-scene-chip-anchor/plan.json'}
    (run / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(f"pilot_output={run}", flush=True)
    with (run / "run.log").open("w") as log:
        process = budget.start([str(binary), str(run), str(args.frames), str(args.width), str(args.height)],
                               env=env, stdout=log, stderr=subprocess.STDOUT)
        started = time.monotonic()
        if args.profile:
            with (run / "trace.log").open("w") as trace_log:
                if args.profile == "cpu":
                    # Skip cold scene setup; sample only this owned process.
                    time.sleep(2)
                    trace_args = ["/usr/bin/sample", str(process.pid), "3", "5",
                                  "-file", str(run / "cpu.sample.txt")]
                    trace_timeout = 55
                else:
                    trace_args = ["xcrun", "xctrace", "record", "--template", "Metal System Trace",
                                  "--attach", str(process.pid), "--time-limit", "8s",
                                  "--output", str(run / "profile.trace")]
                    trace_timeout = 35
                trace = budget.start(trace_args, env=env, stdout=trace_log, stderr=subprocess.STDOUT)
                trace_code = budget.wait(trace, trace_timeout)
            if trace_code:
                raise RuntimeError(f"native trace failed: {run / 'trace.log'}")
        code = budget.wait(process, max(0, 90 - (time.monotonic() - started)))
    print(f"pilot_exit={code}", flush=True)
    if code:
        print("\n".join((run / "run.log").read_text().splitlines()[-20:]))
    elif args.frames > args.warmup:
        summary = summarize(run, args.frames, args.warmup)
        summary["instrumented"] = args.profile is not None or args.capture
        (run / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
        print(json.dumps(summary, indent=2))
        if not summary["workload_coverage_pass"]:
            return 76
    return code


if __name__ == "__main__":
    raise SystemExit(main())
