#!/usr/bin/env python3
"""Export and verify one public ZIP64 release; discard all packaging scratch."""
from __future__ import annotations

import argparse
import json
import os
from pathlib import Path
import shutil
import subprocess

from export_metal_app import (
    DEFAULT_APP_NAME, check_active_game, estimate_app_bytes, export_bundle,
    validate_inputs,
)
from metal_storage import ensure_space, scratch_output
from rewrite_metal_zip64 import rewrite
from verify_metal_archive import verify


ROOT = Path(__file__).resolve().parents[1]


def package(output: Path, changelog: Path, installation_text: Path) -> None:
    output = output.absolute()
    dist = ROOT / "dist"
    receipt = output.with_suffix(".receipt.json")
    if not output.resolve().is_relative_to(dist.resolve()) or output.suffix != ".zip":
        raise ValueError("Release output must be a new .zip inside repository dist")
    if output.exists() or output.is_symlink() or receipt.exists() or receipt.is_symlink():
        raise FileExistsError("Archive or receipt already exists; choose a new output")
    for source in (changelog, installation_text):
        if not source.is_file():
            raise ValueError(f"Missing release text: {source.name}")
    check_active_game(ROOT)
    inputs = validate_inputs(ROOT)
    payload_bound = (estimate_app_bytes(inputs) + changelog.stat().st_size
                     + installation_text.stat().st_size)
    # App, uncompressed metadata ZIP, public ZIP and extracted app coexist.
    # Never budget APFS clone savings; 1 GiB covers ZIP metadata/staging slack.
    reserve = 4 * payload_bound + 1024 ** 3
    ensure_space(reserve, path=dist)
    output.parent.mkdir(parents=True, exist_ok=True)
    state = {"status": "failed", "archive": output.name,
             "required_workspace_bytes": reserve, "phase": "export"}
    # Receipts survive failed scratch cleanup; never record machine paths or
    # captured launcher output. Exclusive creation preserves prior evidence.
    with receipt.open("x", encoding="utf-8") as report:
        try:
            with scratch_output(dist, ".metal-release-") as work:
                payload = work / "payload"
                payload.mkdir()
                export_bundle(payload / DEFAULT_APP_NAME, ROOT)
                shutil.copy2(changelog, payload / "Что изменилось.md")
                shutil.copy2(installation_text, payload / "Установка.txt")
                native_zip = work / "native-metadata.zip"
                archive = work / output.name
                inventory = work / "zip-inventory.json"
                state["phase"] = "native-metadata"
                subprocess.run([
                    "/usr/bin/ditto", "-c", "-k", "--sequesterRsrc",
                    "--zlibCompressionLevel", "0", str(payload), str(native_zip),
                ], check=True)
                state["phase"] = "zip64"
                rewrite(native_zip, archive, inventory, payload)
                state["phase"] = "native-extraction"
                extracted = work / "extracted"
                subprocess.run([
                    "/usr/bin/ditto", "-x", "-k", str(archive), str(extracted),
                ], check=True)
                state["phase"] = "archive-verification"
                verified = work / "verified.json"
                verify(archive, inventory, payload, extracted, verified)
                state.update(json.loads(verified.read_text(encoding="utf-8")))
                app = extracted / DEFAULT_APP_NAME
                state["phase"] = "signature"
                subprocess.run([
                    "/usr/bin/codesign", "--verify", "--deep", "--strict", str(app),
                ], check=True)
                state["phase"] = "relocated-stage"
                subprocess.run([
                    str(app / "Contents/MacOS/launch"), "--stage-only",
                    "--user-dir", str(work / "isolated-profile"),
                ], check=True)
                inventory_data = json.loads(inventory.read_text(encoding="utf-8"))
                state["libarchive"] = inventory_data["libarchive"]
                state["phase"] = "publish-local"
                # Hard linking is exclusive and does not create another ZIP copy.
                # Both paths are on dist's filesystem; an existing output is safe.
                os.link(archive, output)
                state.update(status="complete", phase="complete")
        except BaseException as error:
            state["error_type"] = type(error).__name__
            raise
        finally:
            json.dump(state, report, ensure_ascii=False, indent=2)
            report.write("\n")
    print(f"Verified release: {output}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, required=True,
                        help="New public .zip inside repository dist")
    parser.add_argument("--changelog", type=Path, default=ROOT / "docs/changelog-ru.md")
    parser.add_argument("--installation-text", type=Path, default=ROOT / "docs/install-ru.txt",
                        help="Player installation text (default: docs/install-ru.txt)")
    args = parser.parse_args()
    try:
        package(args.output, args.changelog, args.installation_text)
    except (OSError, RuntimeError, ValueError, AssertionError, subprocess.SubprocessError) as error:
        parser.exit(1, f"Release failed: {type(error).__name__}; see the output receipt if created.\n")


if __name__ == "__main__":
    main()
