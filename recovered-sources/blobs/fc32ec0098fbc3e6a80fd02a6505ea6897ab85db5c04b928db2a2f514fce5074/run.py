"""Bound the cost of in-process diagnostics on one frozen linked fleet binary."""
from pathlib import Path
import csv
import hashlib
import json
import os
import shutil
import statistics
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools"))
from metal_storage import diagnostic_run, ensure_space
from run_sea_benchmark import summarize

BASE = ROOT / "experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark"
SOURCE = BASE / "20261015-105609"
OWNER = BASE / "20261015-001520/ablation"
BINARY = OWNER / "sea-fleet-ablation"
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
receipt = json.loads((OWNER / "build-receipt.json").read_text())
assert sha(BINARY) == receipt["binary_sha256"]
original = json.loads((SOURCE / "manifest.json").read_text())
ensure_space(required_bytes=64 * 1024 * 1024, path=BASE)
stamp = time.strftime("%Y%m%d-%H%M%S")
results = []
for index, enabled in enumerate(("1", "0", "1")):
    processes = subprocess.check_output(["ps", "-axo", "pid=,comm="], text=True)
    assert not any("/metal-engine" in row or row.strip().endswith("/engine-1") for row in processes.splitlines())
    run = BASE / f"{stamp}-profile-cost-{index}-{enabled}"
    with diagnostic_run(run) as (scratch, budget):
        names = ("fixture.c", "ship-type.c", "ship-lights.c", "day.c", "native-declarations.c", "engine.ini")
        for name in names:
            shutil.copyfile(SOURCE / name, run / name)
        (run / "RESOURCE").symlink_to((SOURCE / "RESOURCE").resolve(), target_is_directory=True)
        env = {k: v for k, v in os.environ.items() if not k.startswith("STORM_")}
        env.update(original["backend_environment"])
        env.update(STORM_METAL_PROFILE=enabled, STORM_METAL_SHIP_LOCAL_SHADOWS="0",
                   STORM_METAL_CONSERVATIVE_ANIMATED_BOUNDS="1", STORM_METAL_BENCHMARK_EXCLUDE="",
                   STORM_QUIET_CLIENT="1", STORM_METAL_FRAME_TRACE=str(run / "renderer-frames.csv"),
                   TMPDIR=str(scratch) + os.sep)
        env.pop("STORM_METAL_GPU_CAPTURE", None)
        manifest = {"binary_sha256": sha(BINARY), "frozen_build_receipt": str(OWNER / "build-receipt.json"),
                    "profile_enabled": enabled, "frames": 384, "warmup": 128,
                    "inputs_sha256": {name: sha(run / name) for name in names},
                    "backend_environment": {k: v for k, v in env.items() if k.startswith("STORM_")},
                    "not_proven": ["current installed-engine cadence", "GPU cost with diagnostics disabled", "120FPS"]}
        (run / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
        with (run / "run.log").open("w") as log:
            process = budget.start([str(BINARY), str(run), "384", "2560", "1655"], env=env, stdout=log, stderr=log)
            code = budget.wait(process, 90)
        assert code == 0, (run, code)
        cpu = list(csv.DictReader((run / "cpu-frames.csv").open()))
        assert len(cpu) == 384
        values = [float(row["cpu_wall_ms"]) for row in cpu[128:]]
        result = {"run": str(run.relative_to(ROOT)), "profile_enabled": enabled,
                  "core_mean_ms": statistics.mean(values), "core_median_ms": statistics.median(values),
                  "scene_sha256": sha(run / "scene.ppm")}
        if enabled == "1":
            summary = summarize(run, 384, 128)
            assert summary["required_pass_minima"] == {"sails": 152, "animated": 311, "sea_draws": 1, "sky_current": 1, "shadow_cubes_complete": 8}
            result["completion_mean_ms"] = summary["main_completion_cadence_mean_ms"]
        results.append(result)
        (Path(__file__).parent / "result.json").write_text(json.dumps(results, indent=2) + "\n")
        print(json.dumps(result), flush=True)
