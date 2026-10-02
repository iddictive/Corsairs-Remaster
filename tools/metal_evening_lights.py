#!/usr/bin/env python3
"""Exact-hash Metal evening lamp ownership.

The engine sun window is 05:30-19:00 (sunset ~18:22), so the 19:00 and 20:00
presets have no direct sun. Without lamps those towns render ambient-only.
This suite flips only the lamp flag for those two hours; Night stays false so
gameplay (animals, ambience, menu) is unchanged.
"""

from __future__ import annotations

import hashlib


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


EVENING_PATH = "PROGRAM/weather/Init/Evening.c"
EVENING_BASE = "c0f5e6d814cc5232e94efd92cb76c5c32429f9dd799b9cc44baf0690d8898e10"

_LIGHTS_OFF = "Weathers[n].Lights = false;".encode("utf-8")
_LIGHTS_ON = "Weathers[n].Lights = true;".encode("utf-8")


def prepare_evening(data: bytes) -> bytes:
    count = data.count(_LIGHTS_OFF)
    if count != 2:
        raise RuntimeError(f"expected the 19h/20h lamp flags, found {count}")
    return data.replace(_LIGHTS_OFF, _LIGHTS_ON)


PREPARERS = {
    EVENING_PATH: (EVENING_BASE, prepare_evening),
}

UPDATED = {
    EVENING_PATH: "092440b04e38a7c6974181764e3c9560989f22476172f678dc79765c4ecb9d69",
}


def prepare(relative: str, data: bytes) -> bytes:
    if relative not in PREPARERS:
        return data
    base_hash, fn = PREPARERS[relative]
    curr_hash = digest(data)
    if relative in UPDATED and curr_hash == UPDATED[relative]:
        return data
    if curr_hash != base_hash:
        raise RuntimeError(f"unrecognized base for {relative}: {curr_hash} != {base_hash}")
    result = fn(data)
    if digest(result) != UPDATED[relative]:
        raise RuntimeError(f"unreviewed evening lights output: {relative}")
    return result
