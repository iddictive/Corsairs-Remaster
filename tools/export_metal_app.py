#!/usr/bin/env python3
"""
tools/export_metal_app.py

Exports a fully standalone, public macOS application bundle:
  dist/Corsairs Iddictive Remaster.app (bundle ID: us.iddictive.corsairs)

The public app bundle is completely independent of Xcode, Homebrew, repository,
and user Python installations. It contains:
  - Native Metal engine binary (metal-engine) with @executable_path/../Frameworks rpath.
  - Pinned SDL2 runtime dylib in Contents/Frameworks.
  - Bundled, isolated Python 3.14 framework with standard library (system libs only).
  - Runtime PROGRAM and RESOURCE trees (21GB) in Contents/Resources.
  - Baseline gameplay configs (engine.ini with msaa=0, options, project.df).
  - Public launcher script and canonical graphics settings adapter.
  - Zero player saves, userdata, diagnostic logs, or repository source code.

Player state is managed at runtime in:
  ~/Library/Application Support/Iddictive Corsairs
with symlinked resource roots and separate mutable configuration/saves.
"""

from __future__ import annotations

import argparse
import os
import plistlib
import re
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Any


DEFAULT_APP_NAME = "Corsairs Iddictive Remaster.app"
BUNDLE_ID = "us.iddictive.corsairs"
DISPLAY_NAME = "Corsairs Iddictive Remaster"
FORBIDDEN_PREFIXES = ("/opt/homebrew", "/usr/local", "/Users/")


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export standalone public Corsairs Iddictive Remaster.app."
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help=f"Destination .app path (default: dist/{DEFAULT_APP_NAME} in repository).",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Validate inputs and packaging prerequisites without exporting.",
    )
    return parser.parse_args()


def check_active_game(repo_root: Path) -> None:
    """Refuse export if a game process is actively writing to the source cache."""
    proc = subprocess.run(
        ["ps", "-axo", "pid,comm"],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        sys.exit("Error: Failed to inspect running processes via ps.")

    pattern = re.compile(r"(engine\.exe|/(native|metal)-engine|/engine-1)$")
    cache_root_str = str((repo_root / "experiments/native-metal/.cache").resolve())

    for line in proc.stdout.splitlines():
        line = line.strip()
        if not line:
            continue
        parts = line.split(None, 1)
        if len(parts) != 2:
            continue
        pid_str, comm = parts
        try:
            pid = int(pid_str)
        except ValueError:
            continue

        if pattern.search(comm):
            args_proc = subprocess.run(
                ["ps", "-p", str(pid), "-o", "args="],
                capture_output=True,
                text=True,
            )
            full_cmd = args_proc.stdout.strip()
            if cache_root_str in full_cmd:
                sys.exit(
                    f"Error: Active game or engine process detected running from source cache (PID {pid}: {full_cmd}). "
                    "Close the running game before exporting."
                )

            # Check cwd of process
            cwd_proc = subprocess.run(
                ["lsof", "-a", "-p", str(pid), "-d", "cwd", "-Fn"],
                capture_output=True,
                text=True,
            )
            if cwd_proc.returncode == 0:
                for cwd_line in cwd_proc.stdout.splitlines():
                    if cwd_line.startswith("n") and cache_root_str in cwd_line[1:]:
                        sys.exit(
                            f"Error: Active game or engine process detected running with cwd in source cache (PID {pid}). "
                            "Close the running game before exporting."
                        )


def validate_destination(dest: Path, repo_root: Path) -> None:
    dest_res = dest.resolve()
    repo_res = repo_root.resolve()

    if dest.exists():
        sys.exit(f"Error: Destination already exists: {dest}. Refusing to overwrite.")

    if dest_res == repo_res or dest_res in repo_res.parents:
        sys.exit(f"Error: Destination cannot be repository root or an ancestor: {dest}")

    if any(part in (".cache", "inputs") for part in dest_res.parts):
        sys.exit(f"Error: Destination cannot be inside .cache or inputs directory: {dest}")

    if dest_res.is_relative_to(repo_res):
        rel = dest_res.relative_to(repo_res)
        forbidden_roots = {"experiments", "tools", "docs", "game", ".git", "source"}
        if rel.parts and rel.parts[0] in forbidden_roots:
            sys.exit(
                f"Error: Destination cannot be inside repository source tree ({rel.parts[0]}): {dest}"
            )


def read_macho_minos(binary_path: Path) -> tuple[float, float] | None:
    """Extract Mach-O (minos, sdk) version from LC_BUILD_VERSION or LC_VERSION_MIN_MACOSX."""
    try:
        ot = subprocess.check_output(["otool", "-l", str(binary_path)], text=True)
        m = re.search(
            r"cmd\s+LC_BUILD_VERSION\s+cmdsize\s+\d+\s+platform\s+\d+\s+minos\s+(\d+(?:\.\d+)?)\s+sdk\s+(\d+(?:\.\d+)?)",
            ot,
        )
        if m:
            return float(m.group(1)), float(m.group(2))
        m2 = re.search(
            r"cmd\s+LC_VERSION_MIN_MACOSX\s+cmdsize\s+\d+\s+version\s+(\d+(?:\.\d+)?)\s+sdk\s+(\d+(?:\.\d+)?)",
            ot,
        )
        if m2:
            return float(m2.group(1)), float(m2.group(2))
    except Exception:
        pass
    return None


def find_python_framework() -> Path:
    candidates = [
        Path("/opt/homebrew/opt/python@3.14/Frameworks/Python.framework/Versions/3.14"),
        Path("/opt/homebrew/Frameworks/Python.framework/Versions/3.14"),
        Path("/Library/Frameworks/Python.framework/Versions/3.14"),
    ]
    for c in candidates:
        if (c / "Python").is_file() and (c / "bin/python3.14").is_file():
            return c
    sys.exit(
        "Error: Python 3.14 framework with 'Python' binary not found. "
        "Checked: " + ", ".join(str(c) for c in candidates)
    )


def validate_inputs(repo_root: Path) -> dict[str, Any]:
    metal_cache = repo_root / "experiments/native-metal/.cache"
    inputs_native = repo_root / "experiments/native-metal/inputs/native"
    gameplay_inputs = repo_root / "experiments/native-metal/inputs/gameplay"

    # Engine must be staged candidate - no build/bin fallback for public app
    engine_bin = metal_cache / "CorsairsMetal.app/Contents/MacOS/metal-engine"
    if not engine_bin.is_file() or not os.access(engine_bin, os.X_OK):
        sys.exit(
            f"Error: Staged metal-engine executable missing: {engine_bin}. "
            "Run experiments/native-metal/run.sh --stage-only first."
        )

    # Check Mach-O minos floor
    minos_info = read_macho_minos(engine_bin)
    if minos_info:
        minos, sdk = minos_info
        print(f"Preflight: Engine Mach-O minos={minos}, sdk={sdk}")
        if minos > 15.0:
            print(f"Notice: Observed engine minos {minos} exceeds target floor 15.0.")

    sdl_dylib = inputs_native / "d3d9/sdl2-fixed/lib/libSDL2-2.0.0.dylib"
    if not sdl_dylib.is_file():
        sys.exit(f"Error: Required SDL2 dylib missing: {sdl_dylib}")

    program_dir = metal_cache / "runtime/PROGRAM"
    resource_dir = metal_cache / "runtime/RESOURCE"
    if not program_dir.is_dir() or not resource_dir.is_dir():
        sys.exit(
            "Error: Runtime PROGRAM or RESOURCE directory missing in cache. "
            "Run experiments/native-metal/run.sh --stage-only first."
        )

    from sync_metal_gameplay import delivery_content
    gameplay_origins = {}
    try:
        gameplay_content = delivery_content(metal_cache / "runtime", gameplay_origins)
    except (RuntimeError, OSError, ValueError) as error:
        sys.exit(f"Error: {error}")

    # Captured neutral baseline gameplay inputs required - no runtime-config fallback
    engine_ini = gameplay_inputs / "engine.ini"
    options = gameplay_inputs / "options"
    project_df = gameplay_inputs / "project.df"
    if not engine_ini.is_file():
        sys.exit(f"Error: Required baseline engine.ini missing in {gameplay_inputs}.")
    if not options.is_file():
        sys.exit(f"Error: Required baseline options missing in {gameplay_inputs}.")
    if not project_df.is_file():
        sys.exit(f"Error: Required baseline project.df missing in {gameplay_inputs}.")

    icns = metal_cache / "CorsairsMetal.app/Contents/Resources/Corsairs.icns"
    if not icns.is_file():
        icns = inputs_native / "Corsairs.icns"
    if not icns.is_file():
        sys.exit(f"Error: Missing Corsairs.icns: {icns}")

    shared_headers = metal_cache / "CorsairsMetal.app/Contents/Resources/resource/shared"
    if not (shared_headers / "messages.h").is_file():
        sys.exit(f"Error: Built script messages.h missing: {shared_headers}; stage the engine first.")

    graphics_script = repo_root / "tools/metal_graphics_settings.py"
    if not graphics_script.is_file():
        sys.exit(f"Error: Missing canonical settings adapter: {graphics_script}")

    launcher_script = repo_root / "experiments/native-metal/public_launcher.py"
    if not launcher_script.is_file():
        sys.exit(f"Error: Missing public launcher coordinator: {launcher_script}")

    py_framework = find_python_framework()

    return {
        "engine_bin": engine_bin,
        "minos_info": minos_info,
        "sdl_dylib": sdl_dylib,
        "program_dir": program_dir,
        "resource_dir": resource_dir,
        "engine_ini": engine_ini,
        "options": options,
        "project_df": project_df,
        "icns": icns,
        "shared_headers": shared_headers,
        "graphics_script": graphics_script,
        "launcher_script": launcher_script,
        "py_framework": py_framework,
        "gameplay_content": gameplay_content,
        "gameplay_origins": gameplay_origins,
    }


def copy_or_clone(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    if src.is_dir():
        res = subprocess.run(["/bin/cp", "-cR", str(src), str(dst)], capture_output=True, text=True)
        if res.returncode != 0:
            if dst.exists():
                sys.exit(
                    f"Error: Clone failed for {src} -> {dst} leaving partial target. "
                    "Preserving partial destination per cleanup policy."
                )
            shutil.copytree(src, dst, symlinks=True)
    else:
        res = subprocess.run(["/bin/cp", "-c", str(src), str(dst)], capture_output=True, text=True)
        if res.returncode != 0:
            if dst.exists():
                sys.exit(
                    f"Error: Clone failed for {src} -> {dst} leaving partial target. "
                    "Preserving partial destination per cleanup policy."
                )
            shutil.copy2(src, dst)


def strip_external_rpaths(binary_path: Path) -> None:
    """Remove any LC_RPATH starting with Homebrew, local, or user directories."""
    ot_load = subprocess.check_output(["otool", "-l", str(binary_path)], text=True)
    rpath_pattern = re.compile(r"cmd\s+LC_RPATH\s+cmdsize\s+\d+\s+path\s+(.*?)\s+\(offset")
    for match in rpath_pattern.finditer(ot_load):
        rp = match.group(1).strip()
        if any(rp.startswith(pfx) for pfx in FORBIDDEN_PREFIXES):
            subprocess.check_call(["install_name_tool", "-delete_rpath", rp, str(binary_path)])


def package_python_framework(py_src_version_dir: Path, frameworks_dir: Path) -> None:
    print("Packaging isolated Python 3.14 framework...")
    py_dst_version_dir = frameworks_dir / "Python.framework/Versions/3.14"
    py_dst_version_dir.mkdir(parents=True, exist_ok=True)

    # 1. Copy Python dylib and strip external rpaths
    py_dst_dylib = py_dst_version_dir / "Python"
    shutil.copy2(py_src_version_dir / "Python", py_dst_dylib)
    subprocess.check_call([
        "install_name_tool",
        "-id",
        "@rpath/Python.framework/Versions/3.14/Python",
        str(py_dst_dylib),
    ])
    strip_external_rpaths(py_dst_dylib)

    # 2. Copy python executable: use real binary from Python.app to avoid stub posix_spawn
    (py_dst_version_dir / "bin").mkdir(parents=True, exist_ok=True)
    bin_dst = py_dst_version_dir / "bin/python3.14"
    app_bin = py_src_version_dir / "Resources/Python.app/Contents/MacOS/Python"
    if app_bin.is_file():
        shutil.copy2(app_bin, bin_dst)
    else:
        shutil.copy2(py_src_version_dir / "bin/python3.14", bin_dst)

    # Re-link Python dylib reference to bundle relative
    otool_out = subprocess.check_output(["otool", "-L", str(bin_dst)], text=True)
    for line in otool_out.splitlines()[1:]:
        dep = line.strip().split(" (")[0]
        if "Python.framework" in dep and dep != "@executable_path/../Python":
            subprocess.check_call([
                "install_name_tool",
                "-change",
                dep,
                "@executable_path/../Python",
                str(bin_dst),
            ])

    # Ensure rpath and strip external rpaths for executable
    subprocess.run(
        ["install_name_tool", "-add_rpath", "@executable_path/../..", str(bin_dst)],
        capture_output=True,
    )
    strip_external_rpaths(bin_dst)

    sym_py3 = py_dst_version_dir / "bin/python3"
    if sym_py3.exists() or sym_py3.is_symlink():
        sym_py3.unlink()
    sym_py3.symlink_to("python3.14")

    # 3. Copy standard library, skipping heavy external, test, or precompiled bytecode modules
    skip_parts = {
        "site-packages",
        "test",
        "idlelib",
        "tkinter",
        "turtledemo",
        "ensurepip",
        "pydoc_data",
        "config-3.14-darwin",
        "__pycache__",
    }
    src_lib = py_src_version_dir / "lib/python3.14"
    dst_lib = py_dst_version_dir / "lib/python3.14"

    def filter_lib(dirpath: str, names: list[str]) -> list[str]:
        ignored = []
        for name in names:
            if name in skip_parts or name.endswith((".pyc", ".a")):
                ignored.append(name)
        return ignored

    shutil.copytree(src_lib, dst_lib, symlinks=False, ignore=filter_lib)

    # 4. Clean dynload: classify each .so by binary dependency closure (system-only required)
    dynload_dir = dst_lib / "lib-dynload"
    if dynload_dir.is_dir():
        for so_file in dynload_dir.glob("*.so"):
            ot = subprocess.check_output(["otool", "-L", str(so_file)], text=True)
            has_external = False
            for line in ot.splitlines()[1:]:
                dep = line.strip().split(" (")[0]
                if not (dep.startswith("/usr/lib/") or dep.startswith("/System/Library/")):
                    has_external = True
                    break
            if has_external:
                so_file.unlink()
            else:
                strip_external_rpaths(so_file)
                subprocess.check_call(["codesign", "-s", "-", "-f", str(so_file)])

    # 5. Create framework symlinks
    fw_root = frameworks_dir / "Python.framework"
    (fw_root / "Versions/Current").symlink_to("3.14")
    for link_name in ("Python", "bin", "lib"):
        link_target = Path("Versions/Current") / link_name
        link_path = fw_root / link_name
        if link_path.exists() or link_path.is_symlink():
            link_path.unlink()
        link_path.symlink_to(str(link_target))

    # 6. Ad-hoc sign binaries
    subprocess.check_call(["codesign", "-s", "-", "-f", str(py_dst_dylib)])
    subprocess.check_call(["codesign", "-s", "-", "-f", str(bin_dst)])


def package_engine_and_sdl(
    inputs: dict[str, Any],
    macos_dir: Path,
    frameworks_dir: Path,
    repo_root: Path,
) -> None:
    print("Packaging native engine and SDL2...")
    engine_dst = macos_dir / "metal-engine"
    shutil.copy2(inputs["engine_bin"], engine_dst)

    # Re-wire rpath to bundle Frameworks and sign copied engine binary
    strip_external_rpaths(engine_dst)
    subprocess.check_call([
        "install_name_tool",
        "-add_rpath",
        "@executable_path/../Frameworks",
        str(engine_dst),
    ])
    subprocess.check_call(["codesign", "-s", "-", "-f", str(engine_dst)])

    # Check for unstripped originating machine paths in copied engine AFTER rpath rewriting
    engine_bytes = engine_dst.read_bytes()
    repo_bytes = str(repo_root.resolve()).encode("utf-8")
    user_md = b"/Users/"
    if user_md in engine_bytes or repo_bytes in engine_bytes:
        sys.exit(
            "Error: Compiled metal-engine contains local checkout or user-home machine paths after rpath rewriting. "
            "Rebuild engine with -ffile-prefix-map before exporting public bundle."
        )

    # SDL2 dylib
    sdl_dst = frameworks_dir / "libSDL2-2.0.0.dylib"
    shutil.copy2(inputs["sdl_dylib"], sdl_dst)
    strip_external_rpaths(sdl_dst)
    for link_name in ("libSDL2.dylib", "libSDL2-2.0.dylib"):
        link_path = frameworks_dir / link_name
        if link_path.exists() or link_path.is_symlink():
            link_path.unlink()
        link_path.symlink_to("libSDL2-2.0.0.dylib")

    subprocess.check_call(["codesign", "-s", "-", "-f", str(sdl_dst)])


def package_resources(
    inputs: dict[str, Any],
    resources_dir: Path,
) -> None:
    print("Packaging application resources and baseline configs...")
    resources_dir.mkdir(parents=True, exist_ok=True)

    copy_or_clone(inputs["program_dir"], resources_dir / "PROGRAM")
    copy_or_clone(inputs["resource_dir"], resources_dir / "RESOURCE")

    # Baseline engine.ini: ensure msaa=0
    engine_ini_text = inputs["engine_ini"].read_text(encoding="utf-8-sig")
    engine_ini_text = re.sub(r"(?m)^msaa\s*=.*$", "msaa = 0", engine_ini_text)
    (resources_dir / "engine.ini").write_text(engine_ini_text, encoding="utf-8")

    # Baseline options and project.df
    if inputs["options"].is_file():
        shutil.copy2(inputs["options"], resources_dir / "options")
    if inputs["project_df"].is_file():
        shutil.copy2(inputs["project_df"], resources_dir / "project.df")

    # Icon
    if inputs["icns"].is_file():
        shutil.copy2(inputs["icns"], resources_dir / "Corsairs.icns")

    # Shared headers if present
    if inputs["shared_headers"].is_dir():
        copy_or_clone(inputs["shared_headers"], resources_dir / "resource/shared")

    # Settings and launcher coordinator scripts
    shutil.copy2(inputs["graphics_script"], resources_dir / "metal_graphics_settings.py")
    shutil.copy2(inputs["launcher_script"], resources_dir / "public_launcher.py")


def write_plist_and_launcher(contents_dir: Path, minos_version: str = "15.0") -> None:
    print(f"Writing Info.plist (minos={minos_version}) and launch script...")
    plist_data = {
        "CFBundleDevelopmentRegion": "en",
        "CFBundleDisplayName": DISPLAY_NAME,
        "CFBundleExecutable": "launch",
        "CFBundleIconFile": "Corsairs.icns",
        "CFBundleIdentifier": BUNDLE_ID,
        "CFBundleInfoDictionaryVersion": "6.0",
        "CFBundleName": DISPLAY_NAME,
        "CFBundlePackageType": "APPL",
        "CFBundleVersion": "1",
        "LSMinimumSystemVersion": minos_version,
        "NSHighResolutionCapable": True,
    }
    plist_path = contents_dir / "Info.plist"
    plist_path.write_bytes(plistlib.dumps(plist_data))

    source = Path(__file__).resolve().parents[1] / 'experiments/native-metal/public_native_launcher.c'
    subprocess.check_call(['xcrun', 'clang', '-arch', 'arm64',
                           '-mmacosx-version-min=' + minos_version,
                           '-O2', str(source), '-o', str(contents_dir / 'MacOS/launch')])


def sign_application(bundle_dir: Path, gameplay_content: dict[str, bytes], gameplay_origins: dict[str, str]) -> None:
    """Seal nested code before the app, avoiding TCC's unsigned-bundle synthesis."""
    framework = bundle_dir / 'Contents/Frameworks/Python.framework'
    version = framework / 'Versions/3.14'
    resources = version / 'Resources'
    resources.mkdir(exist_ok=True)
    (resources / 'Info.plist').write_bytes(plistlib.dumps({
        'CFBundleIdentifier': 'us.iddictive.corsairs.python',
        'CFBundleExecutable': 'Python', 'CFBundlePackageType': 'FMWK',
        'CFBundleVersion': '3.14', 'CFBundleName': 'Python'}))
    resource_link = framework / 'Resources'
    if not resource_link.exists() and not resource_link.is_symlink():
        resource_link.symlink_to('Versions/Current/Resources')
    for path, identifier in ((version / 'bin/python3.14', 'us.iddictive.corsairs.python-bin'),
                             (framework, 'us.iddictive.corsairs.python'),
                             (bundle_dir / 'Contents/MacOS/metal-engine', BUNDLE_ID + '.engine')):
        subprocess.check_call(['codesign', '--force', '--sign', '-', '--identifier', identifier, str(path)])
    from delivery_state import record_export
    record_export(bundle_dir, gameplay_content, gameplay_origins)
    subprocess.check_call(['codesign', '--force', '--sign', '-', '--identifier', BUNDLE_ID, str(bundle_dir)])
    subprocess.check_call(['codesign', '--verify', '--deep', '--strict', str(bundle_dir)])


def verify_dependencies(bundle_dir: Path) -> None:
    print("Verifying binary dependencies via otool...")
    count = 0
    macho_magics = (
        bytes([0xcf, 0xfa, 0xed, 0xfe]),
        bytes([0xfe, 0xed, 0xfa, 0xcf]),
        bytes([0xca, 0xfe, 0xba, 0xbe]),
        bytes([0xbe, 0xba, 0xfe, 0xca]),
        bytes([0xca, 0xfe, 0xba, 0xbf]),
    )

    for root, _, files in os.walk(bundle_dir):
        for name in files:
            path = Path(root) / name
            try:
                with path.open("rb") as stream:
                    header = stream.read(4)
            except Exception:
                continue
            if header not in macho_magics:
                continue

            count += 1
            # Check linked libraries
            ot = subprocess.check_output(["otool", "-L", str(path)], text=True)
            for line in ot.splitlines()[1:]:
                dep = line.strip().split(" (")[0]
                if not dep.startswith(("/usr/lib/", "/System/Library/",
                                       "@rpath/", "@executable_path/", "@loader_path/")):
                    sys.exit(f"Error: Non-bundle dependency in {path}: {dep}")

            # Check rpaths
            ot_load = subprocess.check_output(["otool", "-l", str(path)], text=True)
            rpath_pattern = re.compile(r"cmd\s+LC_RPATH\s+cmdsize\s+\d+\s+path\s+(.*?)\s+\(offset")
            for match in rpath_pattern.finditer(ot_load):
                rp = match.group(1).strip()
                for forbidden in FORBIDDEN_PREFIXES:
                    if rp.startswith(forbidden):
                        sys.exit(
                            f"Error: Illegal LC_RPATH in {path}: {rp}. "
                            "Packaging failed standalone isolation requirements."
                        )

    print(f"Verified {count} Mach-O binaries: system and bundle internal references only.")


def export_bundle(dest_app: Path, repo_root: Path) -> None:
    check_active_game(repo_root)
    validate_destination(dest_app, repo_root)
    inputs = validate_inputs(repo_root)

    minos_str = "15.0"
    if inputs.get("minos_info"):
        minos_val = inputs["minos_info"][0]
        minos_str = f"{minos_val:.1f}"

    print(f"Exporting standalone application bundle to: {dest_app}")
    contents_dir = dest_app / "Contents"
    macos_dir = contents_dir / "MacOS"
    resources_dir = contents_dir / "Resources"
    frameworks_dir = contents_dir / "Frameworks"

    macos_dir.mkdir(parents=True, exist_ok=False)
    resources_dir.mkdir(parents=True, exist_ok=False)
    frameworks_dir.mkdir(parents=True, exist_ok=False)

    package_python_framework(inputs["py_framework"], frameworks_dir)
    package_engine_and_sdl(inputs, macos_dir, frameworks_dir, repo_root)
    package_resources(inputs, resources_dir)
    write_plist_and_launcher(contents_dir, minos_version=minos_str)
    # Fresh output only: Finder metadata and host bytecode are not release inputs.
    for directory, _, files in os.walk(dest_app):
        for name in files:
            if name == ".DS_Store" or name.endswith(".pyc"):
                (Path(directory) / name).unlink()
    sign_application(dest_app, inputs['gameplay_content'], inputs['gameplay_origins'])
    verify_dependencies(dest_app)
    print("Standalone application bundle export completed successfully.")


def main() -> None:
    args = parse_args()
    repo_root = Path(__file__).resolve().parent.parent

    if args.check:
        check_active_game(repo_root)
        validate_inputs(repo_root)
        print("All export inputs and prerequisites verified successfully.")
        return

    dest = args.output
    if dest is None:
        dest = repo_root / "dist" / DEFAULT_APP_NAME
    elif not dest.is_absolute():
        dest = (Path.cwd() / dest).resolve()

    export_bundle(dest, repo_root)


if __name__ == "__main__":
    main()
