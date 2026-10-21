#!/usr/bin/env python3
"""Capture a hash-bound native build for engine-linked acceptance fixtures."""
from __future__ import annotations

import argparse
from datetime import date
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import subprocess
import tarfile

from metal_storage import compress_copy, ensure_evidence_space, file_digest


def reject_duplicate(repo: Path, output: Path, native_hashes: dict,
                     header_hashes: dict, vendor_hashes: dict) -> None:
    """One frozen native payload can serve multiple named fixture consumers."""
    roots = {repo / ".local-archives", repo / "experiments/native-metal/.cache/diagnostics",
             output.parent}
    seen = set()
    for root in sorted(roots):
        for directory, children, names in os.walk(root, followlinks=False):
            folder = Path(directory)
            children[:] = [name for name in children if name != ".git"]
            if (folder / ".git").exists():
                children[:] = []
                continue
            manifest = folder / "manifest.json"
            if "manifest.json" not in names or manifest in seen or folder == output:
                continue
            seen.add(manifest)
            try:
                previous = json.loads(manifest.read_text())
                if (previous.get("header_sources") != header_hashes
                        or previous.get("vendor_sources") != vendor_hashes):
                    continue
                retained = previous["retained"]
                payload = {name: record["sha256"] for name, record in retained.items()
                           if name == "engine-unsigned" or name.startswith("libraries/")}
                if payload != native_hashes:
                    continue
                if any(not (folder / name).is_file() or (folder / name).is_symlink()
                       or file_digest(folder / name).hex() != digest
                       for name, digest in native_hashes.items()):
                    continue
            except (OSError, ValueError, KeyError, TypeError, AttributeError):
                continue  # Foreign or incomplete evidence is never overwritten.
            raise RuntimeError(f"Native build already captured at {folder}; reuse its artifacts "
                               "and retain only distinct target metadata, not another payload")


def capture_destination(repo: Path, output: Path) -> Path:
    """One retained build slot; extra captures live only in bounded scratch."""
    canonical = repo / "experiments/native-metal/.cache/diagnostics/retained-native-build"
    if output == canonical:
        return output
    scratch = os.environ.get("CODEX_SCRATCH_DIR")
    if (scratch and not Path(scratch).resolve().is_relative_to(repo)
            and output != Path(scratch).resolve() and output.is_relative_to(Path(scratch).resolve())):
        return output
    raise RuntimeError(f"Persistent capture has one slot: {canonical}; "
                       "reuse it or use CODEX_SCRATCH_DIR from the bounded scratch producer")


def capture(repo: Path, output: Path, target: str, max_bytes: int,
            retain_for: str, release_after: str, expires: str) -> dict:
    if not retain_for.strip() or not release_after.strip():
        raise RuntimeError("A retained capture needs a concrete consumer and release condition")
    if date.fromisoformat(expires) < date.today():
        raise RuntimeError("Capture retention expiry is already past")
    repo, output = repo.resolve(), output.resolve()
    capture_destination(repo, output)
    metal = repo / "experiments/native-metal"
    build = metal / ".cache/build"
    spec = importlib.util.spec_from_file_location("native_build_identity", repo / "tools/metal_engine_source.py")
    identity = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(identity)
    receipt = json.loads(identity.RECEIPT.read_text())
    engine = build / "bin/engine-1"
    if not identity.ready(engine, engine):
        raise RuntimeError("Native inputs or engine differ from the canonical build receipt; build first")

    cache = build / "CMakeCache.txt"
    ninja = next(line.split("=", 1)[1] for line in cache.read_text().splitlines()
                 if line.startswith("CMAKE_MAKE_PROGRAM:FILEPATH="))
    commands = subprocess.check_output([ninja, "-C", str(build), "-t", "commands", target], text=True)
    # Link-target flags are not an ABI contract for classes from its libraries.
    # Retain Ninja's actual per-translation-unit definitions and include paths.
    compilation = subprocess.check_output(
        [ninja, "-C", str(build), "-t", "compdb-targets", target], text=True)
    if not isinstance(json.loads(compilation), list):
        raise RuntimeError("Ninja did not return a compilation database")
    headers = {p for p in identity.source_files()
               if p.suffix.lower() in {".h", ".hpp", ".rh", ".inl", ".inc", ".ipp", ".tcc"}}
    # Vendor headers are actual compiler inputs, including earcut and Clipper.
    headers.update(p for p in (metal / "third_party").rglob("*")
                   if p.is_file() and p.suffix.lower() in {".h", ".hpp", ".inl", ".inc", ".ipp", ".tcc"})
    libraries = sorted(build.rglob("*.a"))
    if not libraries:
        raise RuntimeError("Canonical build contains no static libraries")
    sources = {"engine-unsigned": engine, "CMakeCache.txt": cache,
               "build.log": metal / ".cache/build.log", "configure.log": metal / ".cache/configure.log"}
    sources.update({"libraries/" + p.relative_to(build).as_posix(): p for p in libraries})
    vendor_inputs = [p for p in (metal / "third_party").rglob("*") if p.is_file()
                     and (p.suffix.lower() in {".h", ".hpp", ".inl", ".inc", ".ipp", ".tcc", ".cpp", ".c", ".cc", ".cxx", ".mm", ".cmake"}
                          or p.name == "CMakeLists.txt")]
    # Canonical build.sh writes this receipt only after Ninja has finished.
    # A dry run cannot prove freshness here: CMake's unconditional VERIFY_GLOBS
    # schedules manifest regeneration even immediately after a successful build.
    recorded_at = identity.RECEIPT.stat().st_mtime_ns
    for path in [*headers, *sources.values()]:
        if path.is_symlink() or not path.is_file():
            raise RuntimeError(f"Missing or linked capture input: {path}")
    for path in [*headers, *sources.values(), *vendor_inputs]:
        if path.stat().st_mtime_ns > recorded_at:
            raise RuntimeError(f"Build input/artifact changed after the canonical receipt: {path}; build first")
    header_hashes = {p.relative_to(repo).as_posix(): identity.digest(p) for p in sorted(headers)}
    vendor_hashes = {p.relative_to(repo).as_posix(): identity.digest(p) for p in sorted(vendor_inputs)}
    source_hashes = {name: identity.digest(p) for name, p in sources.items()}

    manifest_path = output / "manifest.json"
    if manifest_path.is_file():
        previous = json.loads(manifest_path.read_text())
        if "compile-commands.json" not in previous["retained"]:
            raise RuntimeError("Existing capture lacks production compiler flags; preserve it for owner review")
        if (previous["build_receipt"] != receipt or previous["header_sources"] != header_hashes
                or previous["vendor_sources"] != vendor_hashes):
            raise RuntimeError("Existing capture belongs to different native inputs; resolve its consumer/release condition first")
        if previous["retained"]["producer-commands.txt"]["sha256"] != hashlib.sha256(commands.encode()).hexdigest():
            raise RuntimeError("Fixture producer commands changed; resolve the retained capture's consumer first")
        if previous["retained"]["compile-commands.json"]["sha256"] != hashlib.sha256(compilation.encode()).hexdigest():
            raise RuntimeError("Production compiler flags changed; resolve the retained capture's consumer first")
        for name, record in previous["retained"].items():
            if identity.digest(output / name) != record["sha256"]:
                raise RuntimeError(f"Existing capture changed: {name}")
        for name, digest in source_hashes.items():
            if previous["retained"][name]["sha256"] != digest:
                raise RuntimeError(f"Current build artifact differs from capture: {name}")
        return {"capture": str(output), "reused": True, "build_receipt": receipt}
    if output.exists():
        raise RuntimeError("Capture output already exists without a complete manifest; preserve it for owner review")
    reject_duplicate(repo, output,
                     {name: digest for name, digest in source_hashes.items()
                      if name == "engine-unsigned" or name.startswith("libraries/")},
                     header_hashes, vendor_hashes)
    estimate = (sum(p.stat().st_size for p in sources.values())
                + 2 * sum(p.stat().st_size for p in headers)
                + len(headers) * 2048 + len(commands.encode()) + len(compilation.encode()) + 1024 ** 2)
    if max_bytes <= 0 or estimate > max_bytes:
        raise RuntimeError(f"Capture requires at most {estimate} bytes; admitted bound is {max_bytes}")
    ensure_evidence_space(estimate, output)
    output.mkdir(parents=True)
    retained = {}
    for name, path in sources.items():
        destination = output / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        compress_copy(path, destination)
        copied = identity.digest(destination)
        if copied != source_hashes[name]:
            raise RuntimeError(f"Build artifact changed while copying: {path}")
        retained[name] = {"sha256": copied, "bytes": destination.stat().st_size}
    archive = output / "headers.tar.gz"
    with tarfile.open(archive, "w:gz") as packed:
        for path in sorted(headers):
            packed.add(path, arcname=path.relative_to(repo).as_posix(), recursive=False)
    with tarfile.open(archive) as packed:
        for member in packed.getmembers():
            if hashlib.sha256(packed.extractfile(member).read()).hexdigest() != header_hashes[member.name]:
                raise RuntimeError(f"Header changed while archiving: {member.name}")
    (output / "producer-commands.txt").write_text(commands)
    (output / "compile-commands.json").write_text(compilation)
    for path in [archive, output / "producer-commands.txt", output / "compile-commands.json"]:
        retained[path.name] = {"sha256": identity.digest(path), "bytes": path.stat().st_size}
    if identity.fingerprint() != receipt["source_sha256"]:
        raise RuntimeError("Native sources changed during capture; incomplete output is preserved")
    for name, path in sources.items():
        if identity.digest(path) != source_hashes[name]:
            raise RuntimeError(f"Native build changed during capture: {path}")
    for name, digest in header_hashes.items():
        if identity.digest(repo / name) != digest:
            raise RuntimeError(f"Native header changed during capture: {name}")
    for name, digest in vendor_hashes.items():
        if identity.digest(repo / name) != digest:
            raise RuntimeError(f"Vendor input changed during capture: {name}")
    manifest = {"source_commit": subprocess.check_output(["git", "-C", str(repo), "rev-parse", "HEAD"], text=True).strip(),
                "build_receipt": receipt, "producer_target": target,
                "retention": {"consumer": retain_for, "release_after": release_after,
                              "expires": expires},
                "header_sources": header_hashes, "vendor_sources": vendor_hashes, "retained": retained,
                "portable_inputs_manifest_sha256": (identity.digest(metal / "inputs/manifest.json")
                                                     if (metal / "inputs/manifest.json").is_file() else None),
                "scope": "Coherent engine-linked fixture inputs; no staging, player replay or portable-dependency export"}
    manifest_path.write_text(json.dumps(manifest, indent=2) + "\n")
    return {"capture": str(output), "reused": False, "libraries": len(libraries),
            "headers": len(headers), "build_receipt": receipt}


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--repo", type=Path, default=Path(__file__).resolve().parents[1])
    parser.add_argument("--output", type=Path,
                        help="Defaults to the sole retained-build slot; alternative must be inside CODEX_SCRATCH_DIR")
    parser.add_argument("--target", default="sea-fleet-benchmark")
    parser.add_argument("--max-bytes", type=int, required=True)
    parser.add_argument("--retain-for", required=True,
                        help="Specific consumer property requiring frozen binaries instead of Git/rebuild")
    parser.add_argument("--release-after", required=True,
                        help="Observable condition ending this capture's retention")
    parser.add_argument("--expires", required=True, help="Finite retention review date, YYYY-MM-DD")
    args = parser.parse_args()
    output = args.output or args.repo / "experiments/native-metal/.cache/diagnostics/retained-native-build"
    try:
        print(json.dumps(capture(args.repo, output, args.target, args.max_bytes,
                                 args.retain_for, args.release_after, args.expires), indent=2))
    except (OSError, RuntimeError, ValueError, KeyError, subprocess.CalledProcessError) as error:
        parser.exit(1, f"Build capture failed: {error}\n")
