#!/usr/bin/env python3
"""Native SAVE recovery under the canonical world-damage policy. Never edits input.

Requires the engine's CMZ2 material SAVE reader. Ordinary script state is copied
verbatim; unreachable material publication slots and policy-disabled world damage
are omitted. The output is reparsed and compared with the declared retained state.
"""
from __future__ import annotations

import argparse
import base64
from dataclasses import dataclass, field
import hashlib
import json
from pathlib import Path
import re
import struct
import time
import zlib

PREFIX = b"!CMZ2:"
HEX_PREFIX = b"!CMZ1:"
LIMIT = 0x8000000
MAX_VALUE = 1024 * 1024
MATERIAL = {b"hullmaterial", b"phonkworldmaterial"}
WORLD_POLICY_PATH = Path(__file__).resolve().parents[1] / "experiments/native-metal/world_damage_policy.hpp"


def read_world_policy(path=WORLD_POLICY_PATH):
    """Consume the native owner's declarations; no separately editable policy."""
    raw = path.read_bytes()
    source = raw.decode("utf-8")
    enabled = re.findall(r"inline\s+constexpr\s+bool\s+Enabled\s*=\s*(true|false)\s*;", source)
    names = re.findall(r"inline\s+constexpr\s+std::array<std::string_view,\s*(\d+)>\s+SavedStateAttributes\s*\{([^}]+)\}\s*;", source)
    if len(enabled) != 1 or len(names) != 1:
        raise ValueError("unsupported canonical world-damage policy declarations")
    count, body = names[0]
    attributes = json.loads("[" + body + "]")
    if len(attributes) != int(count) or any(not isinstance(name, str) or not name or not name.isascii() for name in attributes):
        raise ValueError("invalid canonical world-damage attribute names")
    attributes = frozenset(name.encode("ascii").lower() for name in attributes)
    if len(attributes) != int(count):
        raise ValueError("duplicate canonical world-damage attribute names")
    return {"enabled": enabled[0] == "true", "attributes": attributes, "sha256": hashlib.sha256(raw).hexdigest()}


WORLD_POLICY = read_world_policy()


def vd(value):
    if value < 254:
        return bytes((value,))
    return (b"\xfe" + struct.pack("<H", value)) if value <= 65535 else b"\xff" + struct.pack("<I", value)


def wire_string(value):
    return vd(len(value) + 1) + value + b"\0" if value is not None else b"\0"


def unpack(value):
    if value is None or not value.startswith((PREFIX, HEX_PREFIX)):
        return value
    if len(value) < 17 or value[14:15] != b":":
        raise ValueError("invalid material envelope")
    if any(c not in b"0123456789abcdef" for c in value[6:14]):
        raise ValueError("noncanonical material size")
    size = int(value[6:14], 16)
    if not 0 < size <= MAX_VALUE:
        raise ValueError("material value exceeds its bound")
    decoder = zlib.decompressobj()
    if value.startswith(HEX_PREFIX):
        compressed = bytes.fromhex(value[15:].decode("ascii"))
        if compressed.hex().encode() != value[15:]:
            raise ValueError("noncanonical material hex")
    else:
        compressed = base64.b64decode(value[15:], validate=True)
        if base64.b64encode(compressed) != value[15:]:
            raise ValueError("noncanonical base64")
    result = decoder.decompress(compressed, size + 1)
    if len(result) != size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail or b"\0" in result:
        raise ValueError("invalid material compressed bytes")
    return result


def pack(value):
    value = unpack(value)
    if value is None or not 1024 <= len(value) <= MAX_VALUE:
        return value
    candidate = PREFIX + f"{len(value):08x}:".encode() + base64.b64encode(zlib.compress(value, 1))
    return candidate if len(candidate) < len(value) else value


@dataclass
class Node:
    start: int
    code_start: int
    string_start: int
    value: bytes | None
    name: bytes
    children: list[Node] = field(default_factory=list)
    end: int = 0

    def child(self, name):
        return next((v for v in self.children if v.name == name), None)

    def scalar(self, name):
        node = self.child(name)
        return unpack(node.value) if node else None


def revision(node):
    value = node.scalar(b"revision") if node else None
    return int(value) if value and value.isdigit() and int(value) <= 0xffffffffffffffff else 0


def selected(node, names, committed):
    # Match the reader's first-slot-wins tie rule.
    best, latest = None, 0
    for name in names:
        child = node.child(name)
        r = revision(child)
        if 0 < r <= committed and r > latest:
            best, latest = child, r
    return best


def retained(node, state):
    committed = revision(state) if state and state.scalar(b"version") == b"1" and state.scalar(b"model") is not None else 0
    keep = list(node.children)
    if not committed:
        return keep
    if node is state:
        meta = selected(node, (b"metaa", b"metab"), committed)
        if meta:
            keep = [v for v in keep if v.name not in (b"metaa", b"metab") or v is meta]
    elif node.name.startswith(b"n") and node.name[1:].isdigit():
        slot = selected(node, (b"a", b"b"), committed)
        blob = slot.scalar(b"blob") if slot else None
        if blob:
            blob = blob.lower()
            data = node.child(blob)
            if blob not in (b"dataa", b"datab") or not data or slot.scalar(b"payload") is None or slot.scalar(b"payload") != data.scalar(b"id"):
                return keep
        if slot:
            keep = [v for v in keep if v.name not in (b"a", b"b", b"dataa", b"datab") or v is slot or v.name == blob]
    return keep


class Save:
    def __init__(self, file):
        self.file = file
        if len(file) < 48:
            raise ValueError("truncated SAVE")
        self.ext_offset, self.ext_size, raw_size, packed_size = struct.unpack_from("<4I", file, 32)
        if not 0 < raw_size <= 1024 * 1024 * 1024 or not 0 < packed_size <= len(file) - 48:
            raise ValueError("invalid SAVE sizes")
        decoder = zlib.decompressobj()
        self.raw = decoder.decompress(file[48:48 + packed_size], raw_size + 1)
        if len(self.raw) != raw_size or not decoder.eof or decoder.unused_data or decoder.unconsumed_tail:
            raise ValueError("invalid SAVE zlib stream")
        self.packed_size = packed_size
        self.at = 0
        self.roots = []
        self.world_attributes = []
        self.world_count_patches = []
        self.codec = {}
        self.string()  # program
        buckets = {}
        for _ in range(self.vd()):
            value = self.string()
            h = 0
            for c in value:
                if 65 <= c <= 90:
                    c += 32
                if c >= 128:
                    c -= 256
                h = ((h << 4) + c) & 0xffffffff
                g = h & 0xf0000000
                if g:
                    h ^= g >> 24
                    h ^= g
            bucket = h & 511
            index = buckets.get(bucket, 0)
            buckets[bucket] = index + 1
            self.codec[(bucket << 16) | index] = value.lower()
        for _ in range(self.vd()):
            self.string()
        self.variable_count = self.vd()
        for _ in range(self.variable_count):
            self.string()
            kind, count = self.read("<II")
            for _ in range(max(count, 1)):
                if kind in (6, 7):
                    self.take(4)
                elif kind in (9, 12):
                    self.take(8)
                    if kind == 9:
                        self.attr()
                elif kind == 8:
                    self.string()
                elif kind in (10, 11):
                    if self.vd() != 0xffffffff:
                        self.vd()
                        if kind == 11:
                            self.string()
                else:
                    raise ValueError(f"unknown variable kind {kind}")
        if self.at != len(self.raw):
            raise ValueError("unconsumed SAVE data")

    def take(self, size):
        if size < 0 or self.at + size > len(self.raw):
            raise ValueError("truncated SAVE data")
        result = self.raw[self.at:self.at + size]
        self.at += size
        return result

    def read(self, fmt):
        return struct.unpack(fmt, self.take(struct.calcsize(fmt)))

    def vd(self):
        value = self.take(1)[0]
        return self.read("<H")[0] if value == 254 else self.read("<I")[0] if value == 255 else value

    def string(self):
        size = self.vd()
        if not size:
            return None
        result = self.take(size)
        if result[-1] != 0:
            raise ValueError("unterminated SAVE string")
        return result[:-1]

    def attr(self, capture=False):
        start = self.at
        count = self.vd()
        code_start = self.at
        code = self.vd()
        name = self.codec.get(code, b"")
        string_start = self.at
        value = self.string()
        root = not capture and name in MATERIAL
        capture = capture or root
        node = Node(start, code_start, string_start, value if capture else None, name)
        omitted_children = 0
        for _ in range(count):
            child = self.attr(capture)
            if capture:
                node.children.append(child)
            if child.name in WORLD_POLICY["attributes"]:
                omitted_children += 1
        node.end = self.at
        if name in WORLD_POLICY["attributes"]:
            self.world_attributes.append(node)
        if omitted_children and not capture:
            self.world_count_patches.append((start, code_start, vd(count - omitted_children)))
        if root:
            self.roots.append(node)
        return node

    def node_parts(self, node, state, packed, omit_world):
        children = retained(node, state)
        if omit_world:
            children = [child for child in children if child.name not in WORLD_POLICY["attributes"]]
        yield vd(len(children))
        yield self.raw[node.code_start:node.string_start]
        yield wire_string(pack(node.value) if packed else unpack(node.value))
        for child in children:
            yield from self.node_parts(child, state if node is state else None, packed, omit_world)

    def parts(self, packed, omit_world=None):
        if omit_world is None:
            omit_world = not WORLD_POLICY["enabled"]
        operations = [(root.start, root.end, root) for root in self.roots
                      if not (omit_world and root.name in WORLD_POLICY["attributes"])]
        if omit_world:
            operations += [(node.start, node.end, b"") for node in self.world_attributes]
            operations += self.world_count_patches
        at = 0
        for start, end, replacement in sorted(operations, key=lambda op: (op[0], -op[1])):
            if start < at:
                if end > at:
                    raise ValueError("overlapping SAVE transformations")
                continue  # Already transformed by its containing subtree.
            yield memoryview(self.raw)[at:start]
            if isinstance(replacement, Node):
                yield from self.node_parts(replacement, replacement, packed, omit_world)
            else:
                yield replacement
            at = end
        yield memoryview(self.raw)[at:]

    def digest(self, omit_world=None):
        digest = hashlib.sha256()
        for part in self.parts(False, omit_world):
            digest.update(part)
        return digest.hexdigest()

    def compact(self):
        raw = bytearray()
        for part in self.parts(True):
            raw.extend(part)
        if len(raw) > LIMIT:
            raise ValueError(f"compacted SAVE still exceeds native limit: {len(raw)} > {LIMIT}")
        packed = zlib.compress(raw, 9)
        if len(packed) > LIMIT:
            raise ValueError("compressed SAVE exceeds native limit")
        tail_start = 48 + self.packed_size
        if self.ext_offset and not tail_start <= self.ext_offset <= len(self.file):
            raise ValueError("unexpected SAVE extended-data offset")
        header = bytearray(self.file[:48])
        ext = self.ext_offset + len(packed) - self.packed_size if self.ext_offset else 0
        struct.pack_into("<4I", header, 32, ext, self.ext_size, len(raw), len(packed))
        return bytes(header) + packed + self.file[tail_start:]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("--output", required=True, type=Path, help="new file; must not exist")
    args = parser.parse_args()
    if args.output.exists() or args.input.resolve() == args.output.resolve():
        parser.error("output must be a new file distinct from input")
    started = time.monotonic()
    original = Save(args.input.read_bytes())
    baseline = original.digest()
    output = original.compact()
    candidate = Save(output)
    if candidate.digest(omit_world=False) != baseline:
        raise ValueError("authoritative script/material state differs after recovery")
    if not WORLD_POLICY["enabled"] and candidate.world_attributes:
        raise ValueError("disabled world-damage attributes remain after recovery")
    # Preserve metadata/screenshot bytes exactly, including any offset gap.
    if output[48 + candidate.packed_size:] != original.file[48 + original.packed_size:]:
        raise ValueError("extended SAVE data changed")
    with args.output.open("xb") as stream:
        stream.write(output)
    print(json.dumps({"input": str(args.input), "output": str(args.output),
        "input_sha256": hashlib.sha256(original.file).hexdigest(),
        "output_sha256": hashlib.sha256(output).hexdigest(),
        "input_raw_bytes": len(original.raw), "output_raw_bytes": len(candidate.raw),
        "input_file_bytes": len(original.file), "output_file_bytes": len(output),
        "material_states": len(original.roots), "variables": original.variable_count,
        "world_damage_policy": {"path": str(WORLD_POLICY_PATH), "sha256": WORLD_POLICY["sha256"], "enabled": WORLD_POLICY["enabled"], "attributes": sorted(name.decode("ascii") for name in WORLD_POLICY["attributes"])},
        "omitted_world_attributes": len(original.world_attributes) if not WORLD_POLICY["enabled"] else 0,
        "remaining_world_attributes": len(candidate.world_attributes),
        "authoritative_state_sha256": baseline, "seconds": time.monotonic() - started}, indent=2))


if __name__ == "__main__":
    main()
