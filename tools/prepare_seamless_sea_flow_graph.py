#!/usr/bin/env python3
"""Precompute the exact native near-island graph/table for prepared SEA bodies.

Reads only navigation.bodies' frozen GraphPoints INIs. Never starts the game,
changes graph connectivity, or constructs replacement navigation goals.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "experiments/native-metal/.cache/seamless-sea"
CPP = ROOT / "tools/seamless_sea_flow_graph.cpp"
TABLE = ROOT / "experiments/native-metal/engine/src/libs/sea_ai/include/ai_flow_graph_table.h"
VECTOR = ROOT / "experiments/native-metal/engine/src/libs/math/include/c_vector.h"
FLAGS = ["-std=c++20", "-O3", "-DNDEBUG"]


def canonical(value):
    return json.dumps(value, sort_keys=True, separators=(",", ":")).encode("utf-8")


def sha(data):
    return hashlib.sha256(data).hexdigest()


def record(path, base=ROOT):
    data = path.read_bytes()
    return {"path": path.relative_to(base).as_posix(), "sha256": sha(data), "bytes": len(data)}


def atomic_write(path, data):
    temporary = path.with_name(path.name + ".stage")
    try:
        temporary.write_bytes(data)
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def body_inputs(manifest):
    result = {}
    for body, row in sorted(manifest["navigation"]["bodies"].items()):
        path = OUTPUT / row["ini"]
        if not path.resolve().is_relative_to(OUTPUT.resolve()) or path.suffix != ".ini":
            raise ValueError("Prepared graph INI is outside the owning asset output")
        source = next(item for item in row["outputs"] if item["path"] == row["ini"])
        actual = record(path, OUTPUT)
        if actual["path"] != source["path"] or actual["sha256"] != source["sha256"]:
            raise ValueError(f"Frozen navigation INI identity changed: {body}")
        result[body] = {"ini": row["ini"], "sha256": source["sha256"]}
    if not result:
        raise ValueError("Prepare native body navigation before its graph tables")
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compile-only", action="store_true", help="Build the offline producer without generating assets")
    parser.add_argument("--seconds", type=int, default=900, help="Finite total producer deadline (max900)")
    args = parser.parse_args()
    if not 1 <= args.seconds <= 900:
        raise ValueError("Producer deadline must be1..900 seconds")
    deadline = time.monotonic() + args.seconds
    if shutil.disk_usage(OUTPUT).free < 21 * 1024**3:
        raise RuntimeError("Prepared graph assets need1GiB budget above20GiB host reserve")
    producer_dir = OUTPUT / "flow-graphs"
    producer_dir.mkdir(exist_ok=True)
    version = subprocess.check_output(["/usr/bin/c++", "--version"], text=True, timeout=10).splitlines()[0]
    sources = [record(path) for path in (CPP, TABLE, VECTOR)]
    identity = sha(canonical({"sources": sources, "compiler": version, "flags": FLAGS}))
    tool = producer_dir / ("prepare-" + identity[:16])
    command = ["/usr/bin/c++", *FLAGS, str(CPP), "-o", str(tool)]
    if not tool.is_file():
        temporary_tool = tool.with_name(tool.name + ".stage")
        try:
            subprocess.run([*command[:-1], str(temporary_tool)], check=True, timeout=max(.1, deadline-time.monotonic()))
            os.replace(temporary_tool, tool)
        finally:
            temporary_tool.unlink(missing_ok=True)
    compiler = {"identity_sha256": identity, "binary": record(tool, OUTPUT),
                "sources": sources, "compiler": version, "command": command}
    if args.compile_only:
        print(json.dumps(compiler, sort_keys=True))
        return

    manifest_path = OUTPUT / "manifest.json"
    manifest = json.loads(manifest_path.read_bytes())
    inputs = body_inputs(manifest)
    inputs_hash = sha(canonical(inputs))
    evidence_path = OUTPUT / "flow-graphs.json"
    previous = json.loads(evidence_path.read_bytes()) if evidence_path.is_file() else {}
    result = {"version": 1, "generator": record(Path(__file__)), "compiler": compiler,
              "recipe_sha256": manifest["recipe_sha256"], "atlas_frames_sha256": manifest["atlas_frames_sha256"],
              "body_inputs_sha256": inputs_hash, "body_inputs": inputs,
              "runtime_owner": "AIFlowGraph::LoadPrepared", "routing_owner": "native_flow_graph::BuildTable",
              "format": "AIFLOW01 v1 little-endian native32 floats, ordered graph plus exact native routing table",
              "prepare": ["python3", "tools/prepare_seamless_sea_flow_graph.py"], "bodies": {}}
    for body, source in inputs.items():
        ini = OUTPUT / source["ini"]
        output = ini.with_suffix(".aifg")
        old = previous.get("bodies", {}).get(body, {})
        cached = (previous.get("compiler", {}).get("identity_sha256") == identity and old.get("input") == source
                  and output.is_file() and record(output, OUTPUT) == old.get("output"))
        if cached:
            stats = old["preparation"]
        else:
            temporary = output.with_name(output.name + ".stage")
            try:
                stats = json.loads(subprocess.check_output([str(tool), str(ini), str(temporary)], text=True,
                                   timeout=max(.1, deadline-time.monotonic())))
                if stats["bytes"] != temporary.stat().st_size:
                    raise ValueError(f"Prepared graph producer size differs: {body}")
                os.replace(temporary, output)
            finally:
                temporary.unlink(missing_ok=True)
        result["bodies"][body] = {"input": source, "output": record(output, OUTPUT), "preparation": stats}
        print(body, json.dumps(stats, sort_keys=True), flush=True)
    current = json.loads(manifest_path.read_bytes())
    if (body_inputs(current) != inputs or current["recipe_sha256"] != result["recipe_sha256"] or
            current["atlas_frames_sha256"] != result["atlas_frames_sha256"]):
        raise ValueError("Prepared body inputs changed during native graph generation")
    atomic_write(evidence_path, json.dumps(result, ensure_ascii=False, indent=2).encode("utf-8") + b"\n")
    print(json.dumps({"manifest": str(evidence_path.relative_to(ROOT)), "body_inputs_sha256": inputs_hash,
                      "bodies": len(inputs), "bytes": sum(row["output"]["bytes"] for row in result["bodies"].values())}))


if __name__ == "__main__":
    main()
