#!/usr/bin/env python3
"""
tools/export_metal_bundle.py

Exports a self-contained, portable Corsairs Metal development and game bundle.
Destination bundle contains:
  - source/: Shallow Git repository snapshot (depth 1) with current commit baseline,
             overlayed working tree source changes, offline inputs (including pinned build definitions),
             and minimal Metal runtime cache.
  - Build.command: Preflights macOS arm64, Xcode CLT, CMake, Ninja, Python 3,
                   then runs source/experiments/native-metal/run.sh --stage-only.
  - Play.command: Launches installed game app if built, or runs full build & launch.
  - README.txt: Russian instructions and prerequisites.
"""

from __future__ import annotations

import argparse
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path


BUILD_COMMAND = """#!/bin/bash
set -euo pipefail

bundle_root="$(cd "$(dirname "$0")" && pwd)"
toolchain_bin="$bundle_root/source/experiments/native-metal/inputs/toolchain/bin"
if [[ -d "$toolchain_bin" ]]; then
    export PATH="$toolchain_bin:$PATH"
fi

run_sh="$bundle_root/source/experiments/native-metal/run.sh"

echo "=== Corsairs Metal: Проверка системных требований ==="

os_name="$(uname -s)"
arch_name="$(uname -m)"

if [[ "$os_name" != "Darwin" ]]; then
    echo "Ошибка: требуется macOS. Текущая ОС: $os_name" >&2
    exit 1
fi

if [[ "$arch_name" != "arm64" ]]; then
    echo "Ошибка: требуется процессор Apple Silicon (arm64). Текущая архитектура: $arch_name" >&2
    exit 1
fi

os_major="$(sw_vers -productVersion | cut -d. -f1)"
if (( os_major < 15 )); then
    echo "Ошибка: требуется macOS 15.0 или новее (бинарный базис CMake/Ninja/SDL). Текущая версия: $(sw_vers -productVersion)" >&2
    exit 1
fi

if ! command -v xcrun >/dev/null 2>&1 || ! xcrun clang --version >/dev/null 2>&1; then
    echo "Ошибка: не найден компилятор clang из Xcode." >&2
    echo "Убедитесь, что Xcode установлен и выбран через xcode-select." >&2
    exit 1
fi

if ! xcrun -f metal >/dev/null 2>&1; then
    echo "Ошибка: не найден компилятор шейдеров metal." >&2
    echo "Требуется полная установка Xcode с macOS SDK." >&2
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    echo "Ошибка: cmake не найден в PATH (включая автономный toolchain)." >&2
    exit 1
fi

if ! command -v ninja >/dev/null 2>&1; then
    echo "Ошибка: ninja не найден в PATH (включая автономный toolchain)." >&2
    exit 1
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "Ошибка: python3 не найден в PATH." >&2
    echo "Требуется установить Python 3.10 или новее." >&2
    exit 1
fi

if ! python3 -c 'import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)' >/dev/null 2>&1; then
    echo "Ошибка: версия python3 устарела ($(python3 -V 2>&1))." >&2
    echo "Требуется установленный Python 3.10+ (системный /usr/bin/python3 в macOS равен 3.9 и не поддерживается)." >&2
    echo "Установите современный Python 3.10+ и убедитесь, что он доступен в PATH." >&2
    exit 1
fi

echo "Системные требования соблюдены. Запуск сборки..."
exec "$run_sh" --stage-only
"""

PLAY_COMMAND = """#!/bin/bash
set -euo pipefail

bundle_root="$(cd "$(dirname "$0")" && pwd)"
toolchain_bin="$bundle_root/source/experiments/native-metal/inputs/toolchain/bin"
if [[ -d "$toolchain_bin" ]]; then
    export PATH="$toolchain_bin:$PATH"
fi

if ! command -v python3 >/dev/null 2>&1; then
    echo "Ошибка: python3 не найден в PATH." >&2
    echo "Требуется установить Python 3.10 или новее." >&2
    exit 1
fi

if ! python3 -c 'import sys; sys.exit(0 if sys.version_info >= (3, 10) else 1)' >/dev/null 2>&1; then
    echo "Ошибка: версия python3 устарела ($(python3 -V 2>&1))." >&2
    echo "Требуется установленный Python 3.10+ (системный /usr/bin/python3 в macOS равен 3.9 и не поддерживается)." >&2
    echo "Установите современный Python 3.10+ и убедитесь, что он доступен в PATH." >&2
    exit 1
fi

run_sh="$bundle_root/source/experiments/native-metal/run.sh"
app_binary="$bundle_root/source/experiments/native-metal/.cache/CorsairsMetal.app/Contents/MacOS/metal-engine"

if [[ ! -x "$app_binary" ]]; then
    echo "Собранное приложение не найдено в кэше, запуск предварительной сборки..."
    "$bundle_root/Build.command"
fi

exec "$run_sh" --launch-installed
"""

README_TEXT = """Corsairs Metal — Автономный портативный бандл для разработки и игры

1. Требования к системе:
   - Компьютер Mac на базе Apple Silicon (arm64), macOS 15.0 или новее
   - Актуальный Xcode с macOS SDK (требуется для компиляторов clang и metal)
   - Установленный Python 3.10+ в PATH (системный /usr/bin/python3 в macOS равен 3.9 и не поддерживается)

2. Автономный тулчейн и перенос:
   - Утилиты сборки (CMake и Ninja) упакованы внутри бандла (inputs/toolchain).
   - Бандл самодостаточен: исходный код, ресурсы игры (21 ГБ), сохранённые данные
     и оффлайн-зависимости включены в состав.
   - Автоматическая установка системного ПО не производится.
   - Переносите папку Corsairs.bundle целиком, обязательно включая скрытые
     файлы и каталоги (.cache, .git).

3. Сборка и запуск:
   - Для сборки движка и подготовки игровых ресурсов запустите Build.command
     (или выполните в терминале: ./source/experiments/native-metal/run.sh --stage-only).
   - Для запуска игры дважды нажмите Play.command
     (или выполните: ./source/experiments/native-metal/run.sh --launch-installed).
   - Сохранения и данные пользователя сохраняются в source/experiments/native-metal/.cache/userdata
     и runtime/SAVE.
"""


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Export portable complete Corsairs Metal development and game bundle."
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=None,
        help="Destination directory for the exported bundle (default: dist/Corsairs.bundle inside repository).",
    )
    return parser.parse_args()


def check_active_game() -> None:
    proc = subprocess.run(
        ["ps", "-axo", "comm"],
        capture_output=True,
        text=True,
    )
    if proc.returncode != 0:
        sys.exit("Error: Failed to inspect running processes via ps. Cannot safely verify game status.")

    pattern = re.compile(r"(engine\.exe|/(native|metal)-engine|/engine-1)$")
    for line in proc.stdout.splitlines():
        line = line.strip()
        if pattern.search(line):
            sys.exit(
                f"Error: Active game or engine process detected ({line}). "
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


def validate_inputs(repo_root: Path) -> Path:
    inputs_dir = repo_root / "experiments" / "native-metal" / "inputs"
    manifest_path = inputs_dir / "manifest.json"
    prep_script = repo_root / "tools" / "prepare_metal_inputs.py"

    if not inputs_dir.is_dir():
        sys.exit(
            f"Error: Required inputs directory not found: {inputs_dir}. "
            "Run tools/prepare_metal_inputs.py first."
        )

    if not manifest_path.is_file():
        sys.exit(
            f"Error: Required inputs manifest marker not found: {manifest_path}. "
            "Run tools/prepare_metal_inputs.py first."
        )

    if not prep_script.is_file():
        sys.exit("Error: Missing tools/prepare_metal_inputs.py to verify inputs.")

    print("Verifying complete inputs via tools/prepare_metal_inputs.py --check...")
    res = subprocess.run(
        [sys.executable, str(prep_script), "--check"],
        cwd=repo_root,
        capture_output=True,
        text=True,
    )
    if res.returncode != 0:
        err_msg = res.stderr.strip() or res.stdout.strip()
        sys.exit(f"Error: Inputs verification failed: {err_msg}")

    return inputs_dir


def validate_symlinks(path: Path, subtree_root: Path) -> None:
    subtree_root_res = subtree_root.resolve()
    if path.is_symlink():
        target = path.resolve()
        if not target.is_relative_to(subtree_root_res):
            sys.exit(f"Error: Symlink {path} points outside copied subtree: {target}")
    elif path.is_dir():
        for root, dirs, files in os.walk(path, followlinks=False):
            for item in dirs + files:
                item_path = Path(root) / item
                if item_path.is_symlink():
                    target = item_path.resolve()
                    if not target.is_relative_to(subtree_root_res):
                        sys.exit(
                            f"Error: Symlink {item_path} points outside copied subtree: {target}"
                        )


def handle_symlink(src: Path, dst: Path, subtree_root: Path) -> None:
    resolved_target = src.resolve()
    subtree_root_res = subtree_root.resolve()

    if not resolved_target.is_relative_to(subtree_root_res):
        sys.exit(
            f"Error: Symlink {src} points outside copied subtree ({resolved_target}). Rejecting for portability."
        )

    link_target = os.readlink(src)
    target_path = Path(link_target)

    if target_path.is_absolute():
        link_to_write = os.path.relpath(resolved_target, start=src.parent.resolve())
    else:
        link_to_write = link_target

    if dst.exists() or dst.is_symlink():
        dst.unlink()
    os.symlink(link_to_write, dst)


def copy_entry(src: Path, dst: Path, subtree_root: Path) -> None:
    validate_symlinks(src, subtree_root)
    dst.parent.mkdir(parents=True, exist_ok=True)

    if src.is_symlink():
        handle_symlink(src, dst, subtree_root)
        return

    if src.is_dir():
        res = subprocess.run(
            ["/bin/cp", "-cR", str(src), str(dst)],
            capture_output=True,
            text=True,
        )
        if res.returncode != 0:
            if not dst.exists():
                shutil.copytree(src, dst, symlinks=True)
            else:
                sys.exit(
                    f"Error: Clone failed for {src} -> {dst} leaving partial target. "
                    "Preserving partial destination per cleanup policy."
                )
    else:
        res = subprocess.run(
            ["/bin/cp", "-c", str(src), str(dst)],
            capture_output=True,
            text=True,
        )
        if res.returncode != 0:
            if not dst.exists() and not dst.is_symlink():
                shutil.copy2(src, dst, follow_symlinks=False)
            else:
                sys.exit(
                    f"Error: Clone failed for {src} -> {dst} leaving partial target. "
                    "Preserving partial destination per cleanup policy."
                )


def export_source(repo_root: Path, dest_source: Path) -> None:
    print(f"Cloning shallow repository snapshot to {dest_source}...")
    subprocess.run(
        [
            "git",
            "clone",
            "--config",
            "core.logAllRefUpdates=false",
            "--no-local",
            "--depth",
            "1",
            "--single-branch",
            "--no-tags",
            str(repo_root),
            str(dest_source),
        ],
        check=True,
        capture_output=True,
        text=True,
    )

    subprocess.run(
        ["git", "-C", str(dest_source), "remote", "remove", "origin"],
        check=True,
        capture_output=True,
        text=True,
    )

    raw_files = subprocess.check_output(
        ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
        cwd=repo_root,
    )
    file_list = [p for p in raw_files.decode("utf-8").split(chr(0)) if p]

    print(f"Overlaying {len(file_list)} working tree source files...")
    for rel_str in file_list:
        rel_path = Path(rel_str)
        src_file = repo_root / rel_path
        dst_file = dest_source / rel_path

        if src_file.is_symlink():
            validate_symlinks(src_file, repo_root)
            dst_file.parent.mkdir(parents=True, exist_ok=True)
            handle_symlink(src_file, dst_file, repo_root)
        elif src_file.exists():
            dst_file.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(src_file, dst_file, follow_symlinks=False)
            shutil.copymode(src_file, dst_file, follow_symlinks=False)
        else:
            if dst_file.exists() or dst_file.is_symlink():
                dst_file.unlink()


def export_inputs(repo_root: Path, dest_source: Path) -> None:
    src_inputs = repo_root / "experiments" / "native-metal" / "inputs"
    dst_inputs = dest_source / "experiments" / "native-metal" / "inputs"
    print(f"Copying inputs to {dst_inputs}...")
    copy_entry(src_inputs, dst_inputs, subtree_root=src_inputs)


def export_minimal_cache(repo_root: Path, dest_source: Path) -> None:
    src_cache = repo_root / "experiments" / "native-metal" / ".cache"
    dst_cache = dest_source / "experiments" / "native-metal" / ".cache"
    dst_cache.mkdir(parents=True, exist_ok=True)

    items = [
        ("runtime", True),
        ("userdata", False),
        ("material-originals", False),
        ("gameplay-baseline", False),
        ("portobello-ground-original.gm", False),
        ("portobello-town-original.gm", False),
    ]

    for name, required in items:
        item_src = src_cache / name
        item_dst = dst_cache / name
        if item_src.exists():
            print(f"Copying .cache/{name}...")
            copy_entry(item_src, item_dst, subtree_root=src_cache)
        elif required:
            sys.exit(f"Error: Required .cache item missing: {item_src}")


def write_root_artifacts(dest_bundle: Path) -> None:
    build_cmd = dest_bundle / "Build.command"
    build_cmd.write_text(BUILD_COMMAND, encoding="utf-8")
    build_cmd.chmod(0o755)

    play_cmd = dest_bundle / "Play.command"
    play_cmd.write_text(PLAY_COMMAND, encoding="utf-8")
    play_cmd.chmod(0o755)

    readme = dest_bundle / "README.txt"
    readme.write_text(README_TEXT, encoding="utf-8")


def main() -> None:
    args = parse_args()
    repo_root = Path(__file__).resolve().parent.parent

    check_active_game()

    if args.output is not None:
        dest_bundle = args.output
        if not dest_bundle.is_absolute():
            dest_bundle = (Path.cwd() / dest_bundle).resolve()
    else:
        dest_bundle = repo_root / "dist" / "Corsairs.bundle"

    validate_destination(dest_bundle, repo_root)
    validate_inputs(repo_root)

    print(f"Exporting Metal bundle to: {dest_bundle}")
    dest_bundle.mkdir(parents=True, exist_ok=False)

    try:
        dest_source = dest_bundle / "source"
        export_source(repo_root, dest_source)
        export_inputs(repo_root, dest_source)
        export_minimal_cache(repo_root, dest_source)
        write_root_artifacts(dest_bundle)
        print("Metal bundle export completed successfully.")
    except Exception as e:
        sys.exit(f"Export failed: {e}. Preserving partial destination for inspection.")


if __name__ == "__main__":
    main()
