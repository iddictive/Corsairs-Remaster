#!/usr/bin/env python3
"""Falsify the exact-source and visibility invariants of settings_visibility."""

from __future__ import annotations

import importlib.util
import sys

from settings_visibility import DEFAULT_SOURCE, FILES, prepare, strip


DECK_WALK = DEFAULT_SOURCE.parents[1] / "deck_walk.py"


def load_deck_walk():
    spec = importlib.util.spec_from_file_location("settings_probe_deck_walk", DECK_WALK)
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


def main() -> int:
    deck_walk = load_deck_walk()
    for spec in FILES:
        source = (DEFAULT_SOURCE / spec.relative_path).read_bytes()
        canonical, source_state = strip(spec.relative_path, source)
        output, _ = prepare(spec.relative_path, canonical)
        if output == canonical:
            raise RuntimeError(f"{spec.relative_path}: settings layer added no delta")
        if source_state == "patched" and output != source:
            raise RuntimeError(f"{spec.relative_path}: installed layer is not reproducible")

        damaged = source.replace(b"PrimaryLand", b"PrimaryLxnd", 1)
        try:
            prepare(spec.relative_path, damaged)
        except RuntimeError as error:
            if "unsupported source hash" not in str(error):
                raise
        else:
            raise RuntimeError(f"{spec.relative_path}: altered input was accepted")

        round_trip, state = strip(spec.relative_path, output)
        if state != "patched" or round_trip != canonical:
            raise RuntimeError(f"{spec.relative_path}: exact layer did not round-trip")
        composed, deck_state = deck_walk.prepare(spec.relative_path, output)
        if composed != output or deck_state != "installed + settings":
            raise RuntimeError(f"{spec.relative_path}: deck/settings composition failed")

    print("PASS: settings layer composes with deck controls; altered inputs are rejected")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
