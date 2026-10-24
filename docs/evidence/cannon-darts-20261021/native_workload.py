#!/usr/bin/env python3
"""Retained 1024-projectile native baseline/candidate fixture; no builds or player."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "tools"))
from metal_storage import diagnostic_run
from metal_graphics_settings import launch_environment
from run_sea_benchmark import summarize
from read_native_range_state import read as read_native_counters
from read_native_range_state import read_scalars

def main():
    label = sys.argv[1]
    final30_label=label.removesuffix("-declarations").removesuffix("-locals")
    final30=final30_label in ("angle30-angle","angle30-range","angle30-functional")
    if label not in ("baseline", "baseline-retry", "candidate", "dart-contract", "dart-contract-retry", "range-contract", "dart-contract-overlay", "range-contract-overlay", "perf-old-world", "perf-current-world") and not final30:
        raise ValueError("baseline, baseline-retry, candidate, dart-contract or range-contract required")
    profile = label.removesuffix("-retry")
    profile = profile.removesuffix("-overlay")
    if final30: profile="dart-contract" if final30_label=="angle30-functional" else "range-contract"
    if label in ("perf-old-world","perf-current-world"):
        profile="candidate"
    frames = 512 if profile == "range-contract" else 160
    binary = ROOT / "experiments/native-metal/.cache/build/bin/sea-fleet-benchmark"
    if final30: binary=ROOT/".local-archives/cannon-darts-20261021/angle30-acceptance/sea-fleet-benchmark"
    if label in ("perf-old-world","perf-current-world"):
        binary=ROOT/".local-archives/cannon-darts-20261021/counterfactual-2tu"/("sea-fleet-benchmark" if label=="perf-old-world" else "sea-fleet-benchmark.current-world")
    run = ROOT / "experiments/native-metal/.cache/diagnostics/cannon-darts" / (label + "-" + time.strftime("%Y%m%d-%H%M%S"))
    inputs = HERE / "native-fixture"
    retained = HERE / ("native-" + label)
    if retained.exists():
        raise ValueError("retain first result; choose another explicit evidence owner before another run")
    with diagnostic_run(run) as (tooltemp, budget):
        for source in inputs.iterdir():
            if source.is_file():
                shutil.copyfile(source, run / source.name)
        resources = Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE")
        overrides = {}
        if profile in ("dart-contract", "range-contract"):
            fixture=HERE/("native-"+final30_label+".c") if final30 else HERE / ("native-" + ("dart" if profile == "dart-contract" else "range") + "-contract.c")
            shutil.copyfile(fixture, run / "fixture.c")
            if final30:
                for name in ("native-angle30-source.c","native-angle30-legacy-balls.hex"):
                    shutil.copyfile(HERE/name,run/name)
            elif profile == "range-contract":
                shutil.copyfile(HERE / "native-range-source.c", run / "native-range-source.c")
            overrides = {"textures/balls/dart.tga.tx": ROOT / "src/assets/ui/darts-projectile.tga.tx", "techniques/ai/balls.fx": ROOT / "experiments/native-metal/engine/src/techniques/ai/balls.fx"}
            overlay(resources, run / "resource-overlay", overrides)
            (run / "RESOURCE").symlink_to(run / "resource-overlay", target_is_directory=True)
        else:
            (run / "RESOURCE").symlink_to(resources, target_is_directory=True)
        env = launch_environment(Path.home() / "Library/Application Support/Iddictive Corsairs", dict(os.environ))
        env.update(TMPDIR=str(tooltemp) + os.sep, STORM_METAL_PROFILE="1", STORM_METAL_MODERN_EFFECTS="1", STORM_QUIET_CLIENT="1", STORM_METAL_FRAME_TRACE=str(run / "renderer-frames.csv"))
        env.pop("STORM_METAL_GPU_CAPTURE", None)
        env.pop("STORM_METAL_CONTACT_DIAGNOSTIC", None)
        manifest = {"binary_sha256": hashlib.sha256(binary.read_bytes()).hexdigest(), "fixture_inputs": {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs.iterdir() if p.is_file()}, "frames":frames,"resolution":[640,480],"projectiles":1024,"ordinary_visible_lane":512,"ordinary_callback_lane":512,"final_callback_expectation":81408,"timeout_seconds":90,"claim_ceiling":"hidden native ordinary-ball full-path workload; not installed gameplay/hit acceptance"}
        if profile in ("dart-contract", "range-contract"):
            manifest.update(projectiles="four sequential BSP shots + three final sprite shots", fixture_inputs={**manifest["fixture_inputs"], "fixture.c": hashlib.sha256((run / "fixture.c").read_bytes()).hexdigest()}, overlay_inputs={name: {"path": str(path.relative_to(ROOT)), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()} for name,path in overrides.items()}, expected_final_ships=7, claim_ceiling="hidden native BSP identity, in-memory sea codec, projectile metadata and sprite/tracer output; not installed impact-camera or hit-rate acceptance")
            for key in ("ordinary_visible_lane", "ordinary_callback_lane", "final_callback_expectation"):
                manifest.pop(key)
            if profile == "range-contract":
                manifest.update(projectiles=2040, range_contract=json.loads((HERE / ("native-angle30-inputs.json" if final30 else "native-range-inputs.json")).read_text()), claim_ceiling="Native BSP range hit fractions each inside65-75% for1020 simultaneous manual-profile shots; exact counters exported; stationary receivers, no manualcontroller/UI/installed combat acceptance")
                source_name="native-angle30-source.c" if final30 else "native-range-source.c"
                manifest["fixture_inputs"][source_name] = hashlib.sha256((run / source_name).read_bytes()).hexdigest()
            if final30:
                manifest["final30_inputs"]=json.loads((HERE/"native-angle30-inputs.json").read_text())
                manifest["fixture_inputs"]["native-angle30-legacy-balls.hex"]=hashlib.sha256((run/"native-angle30-legacy-balls.hex").read_bytes()).hexdigest()
                if final30_label=="angle30-angle":
                    manifest.update(projectiles=6,claim_ceiling="Actual native AICannon raw-angle queue and80degree queued codec; AIBalls position/peak/range snapshots. Direct queue does not accept manualcontroller/UI.")
        (run / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        started = time.monotonic()
        with (run / "run.log").open("w") as log:
            process = budget.start([str(binary), str(run), str(frames), "640", "480"], env=env, stdout=log, stderr=subprocess.STDOUT)
            try:
                returncode = process.wait(timeout=90)
            except subprocess.TimeoutExpired:
                budget.close()
                raise
        result = {"returncode": returncode, "wall_seconds":time.monotonic()-started,"run":str(run.relative_to(ROOT)),"run_path_basis":"repository-relative"}
        snapshot = run / ("dart-state.bin" if profile == "dart-contract" else "range-state.bin")
        if profile in ("dart-contract", "range-contract") and snapshot.is_file():
            try:
                result["native_counters"] = read_native_counters(snapshot, profile == "dart-contract")
                if final30_label=="angle30-angle":
                    result["native_counters"].update(read_scalars(snapshot,("AngleFires","AngleBad","AngleLoaded")))
            except Exception as error:
                result["validation_error"] = "native counter decode: " + str(error)
        if returncode == 0:
            try:
                if final30 and any(marker in (run/"run.log").read_text() for marker in ("COMPILE ERROR","RUNTIME ERROR")):
                    raise RuntimeError("native script error; preserve raw log before another run")
                if profile in ("dart-contract", "range-contract") and "ships=7 hidden=1" not in (run / "run.log").read_text():
                    raise RuntimeError("final native fixture predicate was not acknowledged by seven-ship result")
                summary = summarize(run, frames, 32)
                result.update(measured_frames=summary["measured_frames"],warmup_frames=summary["warmup_frames"],cpu_wall_mean_ms=summary["cpu_core_wall_mean_ms"],cpu_wall_median_ms=summary["cpu_core_wall_ms"]["median"],cpu_wall_p95_ms=summary["cpu_core_wall_ms"]["p95"],max_rss_bytes=summary["max_rss_bytes"])
            except Exception as error:
                result["validation_error"] = str(error)
        (run / "result.json").write_text(json.dumps(result,indent=2)+"\n")
        retained.mkdir()
        for name in ("manifest.json","result.json","summary.json","run.log","cpu-frames.csv","renderer-frames.csv","scene.ppm","range-state.bin","dart-state.bin","fixture.c","native-range-source.c","native-angle30-source.c","native-angle30-legacy-balls.hex","angle-trajectory.csv"):
            if (run/name).is_file():
                shutil.copyfile(run/name,retained/name)
        print(json.dumps(result,indent=2))
        if returncode:
            raise SystemExit(returncode)
        if "validation_error" in result:
            raise RuntimeError(result["validation_error"])

def overlay(source, destination, overrides, materialize=False):
    """Link unchanged resource subtrees; only descend through two overridden paths."""
    destination.mkdir()
    children = {p.name.casefold():p for p in source.iterdir()} if source and source.is_dir() else {}
    names = set(children) | {key.split("/",1)[0] for key in overrides}
    for name in sorted(names):
        child = children.get(name)
        target = destination / (child.name if child else name)
        if name in overrides:
            target.symlink_to(overrides[name].resolve())
        else:
            nested = {key.split("/",1)[1]:value for key,value in overrides.items() if key.startswith(name + "/")}
            if nested or (materialize and child and child.is_dir()):
                overlay(child, target, nested, materialize or name == "techniques")
            else:
                target.symlink_to(child.resolve(), target_is_directory=child.is_dir())

if __name__ == "__main__":
    main()
