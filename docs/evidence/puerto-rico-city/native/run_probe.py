#!/usr/bin/env python3
"""Private native evidence for docs/puerto-rico-city.md; no app/build/SAVE mutation.

prepare snapshots generated resources over read-only installed stock; build
compiles this probe and links existing coherent libraries; run uses a hidden
independent CORE. Wait for the integration owner's library/GPU release first.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shlex
import shutil
import subprocess
import sys
import tarfile
import time

SOURCE = Path(__file__).resolve().parent
REPO = SOURCE.parents[3]
NATIVE = REPO / "experiments/native-metal"
BUILD = NATIVE / ".cache/build"
OUTPUT = REPO / ".local-archives/metal-renderer-evidence/puerto-rico-city-20261016"
STOCK = Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE")
GENERATED = NATIVE / ".cache/puerto-rico-city/RESOURCE"
INPUTS = (
    "engine/src/libs/location/src/sea_town.cpp",
    "engine/src/libs/location/src/location.h",
    "engine/src/libs/location/src/location.cpp",
    "engine/src/libs/location/src/character.h",
    "engine/src/libs/location/include/ptc_data.h",
    "engine/src/libs/location/src/ptc_data.cpp",
)


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path: Path, value: object) -> None:
    path.write_text(json.dumps(value, indent=2) + "\n")


def source_identity() -> dict[str, str]:
    return {str(p.relative_to(REPO)): sha(p) for p in SOURCE.iterdir() if p.is_file()}


def player_idle() -> None:
    rows = subprocess.check_output(["ps", "-axo", "pid=,comm="], text=True).splitlines()
    assert not any("/metal-engine" in row or row.strip().endswith("/engine-1") for row in rows), \
        "player runtime present; leave it untouched and defer the private GPU probe"


def overlay(stock: Path | None, generated: Path | None, destination: Path, allowed: set[Path]) -> None:
    """Materialize only changed branches; never create a write-through file."""
    if generated is None or not generated.exists():
        assert stock and stock.exists()
        destination.symlink_to(stock, target_is_directory=stock.is_dir())
        return
    assert not generated.is_symlink(), f"generated input must be owned bytes: {generated}"
    if generated.is_file():
        shutil.copy2(generated, destination)
        return
    destination.mkdir()
    old = {p.name.casefold(): p for p in stock.iterdir()} if stock and stock.is_dir() else {}
    new = {p.name.casefold(): p for p in generated.iterdir()
           if p in allowed or any(p in value.parents for value in allowed)}
    for key in sorted(old.keys() | new.keys()):
        child = new.get(key) or old[key]
        overlay(old.get(key), new.get(key), destination / child.name, allowed)


def prepare(run: Path) -> None:
    assert STOCK.is_dir() and GENERATED.is_dir(), "installed stock/generated city resources missing"
    config = GENERATED / "INI/sea_towns/puerto_rico.ini"
    assert config.is_file(), f"generated SeaTown config pending: {config}"
    assert not run.exists(), f"preserve prior evidence; choose a new --name: {run}"
    sys.path.insert(0, str(REPO / "tools"))
    from prepare_puerto_rico_city import prepared
    from metal_graphics_settings import launch_environment
    active = set(prepared().values())
    generated = {str(p.relative_to(GENERATED)): sha(p) for p in active}
    assert generated and shutil.disk_usage(REPO).free > 1024**3, "one GiB private probe reserve unavailable"
    run.mkdir(parents=True)
    for name in ("fixture.c", "engine.ini", "probe.cpp"):
        shutil.copy2(SOURCE / name, run / name)
    overlay(STOCK, GENERATED, run / "RESOURCE", active)
    state = {
        "consumer": "docs/puerto-rico-city.md",
        "source_sha256": source_identity(),
        "native_sha256": {p: sha(NATIVE / p) for p in INPUTS},
        "generated_sha256": generated,
        "graphics_environment": launch_environment(NATIVE / ".cache/runtime", {}),
        "installed_stock": str(STOCK),
        "installed_engine_sha256": sha(STOCK.parents[1] / "MacOS/metal-engine"),
        "source_template": "phonk-world-async-20261016/build.py and probe.cpp",
        "player_control": False,
        "player_save": False,
        "acceptance": "private native component evidence; installed player acceptance unresolved",
    }
    save(run / "prepared.json", state)
    print("prepared", run)


def check_prepared(run: Path) -> dict:
    state = json.loads((run / "prepared.json").read_text())
    assert state["source_sha256"] == source_identity(), "fixture source changed; prepare a new named run"
    assert state["native_sha256"] == {p: sha(NATIVE / p) for p in INPUTS}, "native candidate changed"
    for relative, expected in state["generated_sha256"].items():
        path = run / "RESOURCE" / relative
        assert not path.is_symlink() and sha(path) == expected, f"snapshot changed: {relative}"
    return state


def execute(command: list[str], run: Path, name: str, cwd: Path, *, gpu: bool = False) -> None:
    save(run / (name + "-command.json"), command)
    with (run / (name + ".log")).open("w") as log:
        process = subprocess.Popen(command, cwd=cwd, stdout=log, stderr=subprocess.STDOUT)
        deadline = time.monotonic() + 55 if gpu else None
        try:
            while process.poll() is None:
                if gpu:
                    player_idle()
                    assert time.monotonic() < deadline, "private GPU slot exceeded55 seconds"
                try:
                    process.wait(timeout=0.5)
                except subprocess.TimeoutExpired:
                    pass
        except BaseException:
            process.terminate()
            process.wait(timeout=10)
            raise
    print(name, "exit", process.returncode, flush=True)
    if process.returncode:
        print((run / (name + ".log")).read_text()[-6000:])
        raise SystemExit(process.returncode)


def compile_probe(run: Path, closure: Path) -> None:
    check_prepared(run)
    closure = closure.resolve()
    receipt = json.loads((closure / "manifest.json").read_text())
    assert receipt["build_receipt"]["version"] == 2, "coherent build receipt required"
    for relative, row in receipt["retained"].items():
        assert sha(closure / relative) == row["sha256"], "retained build closure changed"
    before = {p.removeprefix("libraries/"): row["sha256"] for p, row in receipt["retained"].items()
              if p.startswith("libraries/") and p.endswith(".a")}
    assert before, "retained native archives absent"
    # This is already a retained immutable closure, shared by the native
    # evidence consumers. Refer to it instead of copying it for each camera run.
    libraries = closure / "libraries"
    headers = run / "headers"
    headers.mkdir()
    with tarfile.open(closure / "headers.tar.xz") as archive:
        archive.extractall(headers, filter="data")
    frozen = headers / "experiments/native-metal"
    # The retained header archive omits this resource header. It has no local
    # edit; bind its exact committed bytes rather than falling back to live includes.
    resource_header = "engine/src/libs/util/include/platform/winuser.rh"
    committed = subprocess.check_output(["git", "show", "HEAD:experiments/native-metal/" + resource_header], cwd=REPO)
    assert (NATIVE / resource_header).read_bytes() == committed, "resource header has an unreviewed edit"
    (frozen / resource_header).write_bytes(committed)
    save(run / "supplemental-header.json", {resource_header: hashlib.sha256(committed).hexdigest()})
    for relative in INPUTS:
        if Path(relative).suffix == ".h":
            assert sha(frozen / relative) == sha(NATIVE / relative), "city header ABI differs from closure"
    save(run / "closure.json", receipt)
    ninja = NATIVE / "inputs/toolchain/bin/ninja"
    lines = subprocess.check_output([str(ninja), "-C", str(BUILD), "-t", "commands", "sea-fleet-benchmark"], text=True).splitlines()
    compiles = [line for line in lines if " -c " in line and "/sea_fleet_benchmark.cpp" in line]
    links = [line for line in lines if " -o bin/sea-fleet-benchmark " in line]
    assert len(compiles) == len(links) == 1, "existing benchmark command template changed"
    compile = shlex.split(compiles[0])
    for flag in ("-MD", "-MT", "-MF"):
        if flag in compile:
            index = compile.index(flag)
            del compile[index:index + (1 if flag == "-MD" else 2)]
    source = str(NATIVE / "sea_fleet_benchmark.cpp")
    original_object = "native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o"
    assert compile.count(source) == compile.count(original_object) == 1
    compile[compile.index(source)] = str(run / "probe.cpp")
    compile[compile.index(original_object)] = str(run / "probe.o")
    for index, value in enumerate(compile):
        if value.startswith("-I" + str(NATIVE)):
            old = Path(value[2:]).resolve()
            if old == NATIVE or old.is_relative_to(NATIVE / "engine/src"):
                compile[index] = "-I" + str(frozen / old.relative_to(NATIVE))
    if "-v" in compile:
        compile.remove("-v")
    compile += ["-UNDEBUG", "-I" + str(frozen / "engine/src/libs/location/src")]
    link = shlex.split(links[0])
    assert link[:3] == [":", "&&", compile[0]] and link[-2:] == ["&&", ":"]
    link = link[2:-2]
    assert link.count(original_object) == 1
    link[link.index(original_object)] = str(run / "probe.o")
    link[link.index("bin/sea-fleet-benchmark")] = str(run / "native-probe")
    if "-v" in link:
        link.remove("-v")
    for index, value in enumerate(link):
        path = Path(value)
        if value.endswith(".a"):
            relative = path.relative_to(BUILD) if path.is_absolute() and path.is_relative_to(BUILD) else path
            if relative.as_posix() in before:
                link[index] = str(libraries / relative)
            else:
                raise RuntimeError(f"Library outside retained closure: {value}")
    execute(compile, run, "compile", BUILD)
    execute(link, run, "link", BUILD)
    save(run / "linked.json", {"libraries_sha256": before,
                               "retained_libraries": str(libraries.relative_to(REPO)),
                               "binary_sha256": sha(run / "native-probe")})


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("action", choices=("prepare", "build", "run"))
    parser.add_argument("--name", default="candidate-native", help="new name preserves prior evidence")
    parser.add_argument("--closure", type=Path, help="integration owner's retained ABI-matched build closure")
    args = parser.parse_args()
    assert re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,63}", args.name), "invalid local run name"
    run = OUTPUT / args.name
    if args.action == "prepare":
        prepare(run)
    elif args.action == "build":
        assert args.closure, "build requires --closure from the integration owner"
        compile_probe(run, args.closure)
    else:
        state = check_prepared(run)
        linked = json.loads((run / "linked.json").read_text())
        assert sha(run / "native-probe") == linked["binary_sha256"], "private binary changed"
        player_idle()
        os.environ.update(state["graphics_environment"])
        execute([str(run / "native-probe"), str(run)], run, "native-run", run, gpu=True)


if __name__ == "__main__":
    main()
