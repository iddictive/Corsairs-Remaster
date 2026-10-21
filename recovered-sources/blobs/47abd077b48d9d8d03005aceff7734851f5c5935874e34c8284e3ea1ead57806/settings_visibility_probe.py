#!/usr/bin/env python3
"""Falsify the exact-source and visibility invariants of settings_visibility."""

from __future__ import annotations

from settings_visibility import DEFAULT_SOURCE, FILES, prepare


def main() -> int:
    for spec in FILES:
        source = (DEFAULT_SOURCE / spec.relative_path).read_bytes()
        output, _ = prepare(spec.relative_path, source)
        if output == source:
            raise RuntimeError(f"{spec.relative_path}: fixture was already patched")

        damaged = source.replace(b"PrimaryLand", b"PrimaryLxnd", 1)
        try:
            prepare(spec.relative_path, damaged)
        except RuntimeError as error:
            if "unsupported source hash" not in str(error):
                raise
        else:
            raise RuntimeError(f"{spec.relative_path}: altered input was accepted")

    print("PASS: reviewed hotkeys become visible; altered inputs and duplicate labels are rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
