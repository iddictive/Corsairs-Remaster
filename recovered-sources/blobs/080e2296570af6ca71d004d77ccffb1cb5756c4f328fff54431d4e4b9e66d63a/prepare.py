#!/usr/bin/env python3
"""Domain case over the existing native city probe's CORE/link/run facility.

prepare is CPU/file-only. build and run require the integration owner's slot.
Never copies or reads player SAVE; all mutable game files live below a named run.
"""
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tarfile

SOURCE = Path(__file__).resolve().parent
REPO = SOURCE.parents[2]
NATIVE = REPO / "experiments/native-metal"
STOCK = Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources")
ABI_HEADERS = (
    "engine/src/libs/location/src/location.h",
    "engine/src/libs/location/src/character.h",
    "engine/src/libs/location/src/np_character.h",
    "engine/src/libs/location/src/ai_character.h",
    "engine/src/libs/location/src/location_script_context.hpp",
    "engine/src/libs/ship/include/ship_base.h",
    "engine/src/libs/core/src/core_impl.h",
    "engine/src/libs/core/include/controls.h",
)
sys.path.insert(0, str(REPO / "tools"))
import gameplay_sources
import prepare_puerto_rico_city
import prepare_port_rowboat
from metal_graphics_settings import launch_environment

spec = importlib.util.spec_from_file_location("existing_city_probe", REPO / "docs/evidence/puerto-rico-city/native/run_probe.py")
city = importlib.util.module_from_spec(spec)
spec.loader.exec_module(city)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def save(path, value):
    path.write_text(json.dumps(value, indent=2) + "\n")


def source_identity():
    return {p.name: sha(p) for p in SOURCE.iterdir() if p.is_file() and p.suffix in (".py", ".c", ".cpp", ".steps", ".md")}


def overlay(stock, destination, replacements, prefix=""):
    """Existing city overlay's semantics, with case-insensitive sparse inputs.

    Unchanged stock RESOURCE branches remain read-only references. PROGRAM is
    copied to owned bytes because the VM may resolve/load segments dynamically.
    """
    if prefix in replacements:
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(replacements[prefix])
        return
    changed = {key.split("/")[len(prefix.split("/")) if prefix else 0]
               for key in replacements if not prefix or key.startswith(prefix + "/")}
    if not changed:
        destination.symlink_to(stock, target_is_directory=stock.is_dir())
        return
    destination.mkdir()
    children = {p.name.casefold(): p for p in stock.iterdir()} if stock and stock.is_dir() else {}
    names = {name.casefold(): name for name in changed}
    for key in sorted(children.keys() | names.keys()):
        name = names.get(key) or children[key].name
        relative = prefix + "/" + name if prefix else name
        overlay(children.get(key), destination / name, replacements, relative)


def prepare(run, steps, resume):
    assert not run.exists(), "prior evidence is retained; choose a new --name"
    assert shutil.disk_usage(SOURCE).free > 2 * 1024**3, "private case needs a two-GiB host reserve"
    sources, inputs = gameplay_sources.read()
    city_assets = prepare_puerto_rico_city.prepared()
    boat_assets = prepare_port_rowboat.prepared()
    replacements = {name: row[0] for name, row in sources.items()}
    for name, path in {**city_assets, **boat_assets}.items():
        assert not path.is_symlink()
        replacements[name] = path.read_bytes()
    assert "PROGRAM/SEA_AI/PortTravelRig.c" in replacements
    run.mkdir(parents=True)
    # Stock PROGRAM is immutable private copied input, not a write-through link.
    shutil.copytree(STOCK / "PROGRAM", run / "PROGRAM", symlinks=False)
    for name, data in replacements.items():
        if name.startswith("PROGRAM/"):
            path = run / name
            # Preserve stock casing: fio is case-insensitive; duplicate paths
            # on a case-sensitive host would otherwise shadow canonical inputs.
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_bytes(data)
    main = run / "PROGRAM/seadogs.c"
    encoding = sources["PROGRAM/seadogs.c"][2]
    text = main.read_bytes().decode(encoding)
    assert text.count("void Main()") == 1
    text = text.replace("void Main()", "void FullScene_StockMain()", 1)
    text += '\n#include "fullscene_fixture.c"\n'
    main.write_bytes(text.encode(encoding))
    shutil.copy2(SOURCE / "fixture.c", run / "PROGRAM/fullscene_fixture.c")
    resource = {name.removeprefix("RESOURCE/"): data for name, data in replacements.items() if name.startswith("RESOURCE/")}
    overlay(STOCK / "RESOURCE", run / "RESOURCE", resource)
    for directory in ("SAVE", "PROGRAM_BIN", "userdata/Logs", "frames"):
        (run / directory).mkdir(parents=True)
    shutil.copy2(SOURCE / "probe.cpp", run / "probe.cpp")
    shutil.copy2(steps, run / "journey.steps")
    continuation = None
    if resume:
        previous = SOURCE / "runs" / resume
        checkpoint = previous / "SAVE/fullscene-phase"
        assert checkpoint.is_file() and not checkpoint.is_symlink(), "previous private real-engine checkpoint is absent"
        shutil.copy2(checkpoint, run / "SAVE/fullscene-phase")
        continuation = {"path": str(checkpoint.relative_to(REPO)), "sha256": sha(checkpoint)}
    ini = (REPO / "docs/evidence/puerto-rico-city/native/engine.ini").read_text()
    ini = ini.replace("program_directory=.\nrun=fixture.c", "program_directory=PROGRAM\nprogram_bin_directory=PROGRAM_BIN\nrun=seadogs.c")
    (run / "engine.ini").write_text(ini)
    frozen = {str(p.relative_to(run)): sha(p) for p in (run / "PROGRAM").rglob("*") if p.is_file()}
    frozen.update({"RESOURCE/" + key: hashlib.sha256(value).hexdigest() for key, value in resource.items()})
    frozen.update({name: sha(run / name) for name in ("probe.cpp", "engine.ini", "journey.steps")})
    save(run / "prepared.json", {
        "consumer": "docs/seamless-san-juan.md",
        "source_sha256": source_identity(), "frozen_sha256": frozen,
        "canonical_sha256": {str(p.relative_to(REPO)): sha(p) for p in inputs},
        "native_abi_sha256": {name: sha(NATIVE / name) for name in ABI_HEADERS},
        "generated_sources": {name: {"path": str(path.relative_to(REPO)), "sha256": sha(path)} for name, path in {**city_assets, **boat_assets}.items()},
        "graphics_environment": launch_environment(NATIVE / ".cache/runtime", {}),
        "installed_engine_sha256": sha(STOCK.parent / "MacOS/metal-engine"),
        "player_save": False, "player_control": False,
        "input_bound": "real PROGRAM, canonical complete gameplay projections, generated city and rowboat; no Scene API stubs",
        "acceptance": "UNRUN; successful prepare/build never accepts gameplay",
        "continuation": continuation,
        "disk_lifecycle": "~100MiB maximum per named retained evidence case; no renderer/build output until slot; no bulk app/resources/SAVE clone",
    })
    print(run)


def check(run):
    state = json.loads((run / "prepared.json").read_text())
    assert state["source_sha256"] == source_identity(), "fixture changed; prepare a fresh named case"
    for name, expected in state["frozen_sha256"].items():
        assert sha(run / name) == expected, "private input changed: " + name
    for name, expected in state["canonical_sha256"].items():
        assert sha(REPO / name) == expected, "canonical source changed: " + name
    for name, expected in state["native_abi_sha256"].items():
        assert sha(NATIVE / name) == expected, "native ABI changed: " + name
    for name, row in state["generated_sources"].items():
        assert sha(REPO / row["path"]) == row["sha256"], "generated source changed: " + name
    return state


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument("action", choices=("prepare", "check", "parse", "build", "run"))
    p.add_argument("--name", default="candidate-1")
    p.add_argument("--closure", type=Path)
    p.add_argument("--steps", default="journey.steps", help="one authored domain .steps input in this directory")
    p.add_argument("--resume-from", help="prior private named run, never a player SAVE path")
    args = p.parse_args()
    assert re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,63}", args.name)
    run = SOURCE / "runs" / args.name
    if args.action == "prepare":
        assert Path(args.steps).name == args.steps and args.steps.endswith(".steps")
        if args.resume_from: assert re.fullmatch(r"[A-Za-z0-9][A-Za-z0-9_.-]{0,63}", args.resume_from)
        prepare(run, SOURCE / args.steps, args.resume_from)
        return
    state = check(run)
    if args.action == "check":
        print("frozen private PROGRAM/generated assets verified; gameplay remains UNRUN")
        return
    if args.action == "parse":
        destination = run / "cpu-parse"
        assert not destination.exists(), "preserve parser evidence; prepare a fresh case"
        shutil.copytree(run / "PROGRAM", destination)
        fixture = destination / "fullscene_fixture.c"
        text = fixture.read_text()
        assert text.count("void Main()") == 1
        fixture.write_text(text.replace("void Main()", "void FullScene_Bootstrap()", 1))
        (destination / "RESOURCE").symlink_to(run / "RESOURCE", target_is_directory=True)
        (destination / "cases.c").write_text("int FullChain() { return 0; }\n")
        (destination / "engine.ini").write_text("program_directory=.\nrun=driver.c\n[compatibility]\ntarget_version=teho\n[script]\nruntimelog=1\n")
        binary = REPO / ".local-archives/metal-renderer-evidence/phonk-current-20261016/installed-script-chain-80ea20/script-vm-probe"
        command = [str(binary), str(destination), "FullChain", "0", "FullChain", "seadogs.c"]
        result = subprocess.run(command, cwd=destination, capture_output=True, text=True, timeout=45)
        (destination / "stdout.log").write_text(result.stdout)
        (destination / "stderr.log").write_text(result.stderr)
        errors = destination / ".vm-userdata/Logs/error.log"
        save(destination / "command.json", {"argv": command, "binary_sha256": sha(binary), "exit": result.returncode,
                                             "scope": "CPU PROGRAM parse only; no bootstrap/scene APIs executed"})
        print(result.stdout[-2500:])
        print(result.stderr[-2500:])
        if errors.exists(): print(errors.read_text(errors="replace")[-5000:])
        raise SystemExit(result.returncode)
    # Reuse the existing immutable closure/link facility. No shared library is built.
    city.SOURCE = SOURCE
    city.INPUTS = ()
    city.check_prepared = check
    if args.action == "build":
        assert args.closure, "integration owner's current ABI-matched retained closure is required"
        with tarfile.open(args.closure / "headers.tar.xz") as headers:
            for name, expected in state["native_abi_sha256"].items():
                member = headers.extractfile("experiments/native-metal/" + name)
                assert member and hashlib.sha256(member.read()).hexdigest() == expected, "retained ABI mismatch: " + name
        city.compile_probe(run, args.closure)
    else:
        linked = json.loads((run / "linked.json").read_text())
        assert sha(run / "native-probe") == linked["binary_sha256"]
        city.player_idle()
        os.environ.update(state["graphics_environment"])
        city.execute([str(run / "native-probe"), str(run), "journey.steps"], run, "native-run", run, gpu=True)


if __name__ == "__main__":
    main()
