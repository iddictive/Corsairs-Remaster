#!/usr/bin/env python3
"""Read only three known scalar records from the native fixture VM snapshot."""
import json
from pathlib import Path
import struct
import sys
import zlib

def payload(path):
    data = Path(path).read_bytes()
    # core/src/compiler.h EXTDATA_HEADER: char[32], uint32 offset, uint32 size.
    # COMPILER::SaveState then writes raw bytes, packed bytes, zlib payload.
    raw_size, packed_size = struct.unpack_from("<II", data, 40)
    assert len(data) == 48 + packed_size
    raw = zlib.decompress(data[48:])
    assert len(raw) == raw_size and raw_size <= 16 * 1024 * 1024
    return raw

def read(path, dart=False):
    names = ("BenchFrames", "BenchBad", "BenchOrdinaryHits", "BenchDartHits", "BenchAirburstHits", "BenchLoadedDartHits", "BenchExplosions", "BenchFreshMetadata", "BenchLoadedMetadata", "BenchPhonkChecks") if dart else ("BenchFrames", "RangeHits0", "RangeHits1", "RangeBad")
    result=read_scalars(path,names)
    if not dart:
        result["hits_fraction"] = [result["RangeHits0"] / 1020, result["RangeHits1"] / 1020]
    return result

def read_scalars(path,names):
    raw = payload(path)
    result = {}
    for name in names:
        encoded = name.encode() + b"\0"
        # SaveString's compact one-byte size + name + S_TOKEN_TYPE VAR_INTEGER=6
        # + Number_of_elements=1. Refuse ambiguous/changed source formats.
        prefix = bytes([len(encoded)]) + encoded + struct.pack("<II", 6, 1)
        assert raw.count(prefix) == 1, f"missing or ambiguous native scalar: {name}"
        result[name] = struct.unpack_from("<i", raw, raw.index(prefix)+len(prefix))[0]
    return result

def attributes(path, balls_only=False):
    """Decode only known fixture objects needed to distinguish setup/flight errors."""
    raw = payload(path)
    offset = 0
    def vdword():
        nonlocal offset
        first = raw[offset]
        offset += 1
        if first < 254:
            return first
        fmt = "<H" if first == 254 else "<I"
        value = struct.unpack_from(fmt, raw, offset)[0]
        offset += struct.calcsize(fmt)
        return value
    def string():
        nonlocal offset
        size = vdword()
        data = raw[offset:offset+size]
        offset += size
        return data[:-1].decode("utf-8", "replace") if size else None
    def hashvalue(text):
        value = 0
        for byte in text.lower().encode():
            value = ((value << 4) + byte) & 0xffffffff
            high = value & 0xf0000000
            if high:
                value ^= high >> 24
                value ^= high
        return value
    string()  # ProgramDirectory
    names, buckets = {}, {}
    for _ in range(vdword()):
        name = string()
        bucket = hashvalue(name) & 511
        rank = buckets.get(bucket, 0)
        names[(bucket << 16) | rank] = name
        buckets[bucket] = rank + 1
    def attribute():
        count, code, value = vdword(), vdword(), string()
        children = {}
        for _ in range(count):
            name, child = attribute()
            children[name] = child
        return names.get(code, str(code)), children if children else value
    def objects(name, count):
        nonlocal offset
        encoded = name.encode() + b"\0"
        prefix = bytes([len(encoded)]) + encoded + struct.pack("<II", 9, count)
        assert raw.count(prefix) == 1, name
        offset = raw.index(prefix) + len(prefix)
        result = []
        for _ in range(count):
            offset += 8  # native entid_t
            result.append(attribute()[1])
        return result
    if balls_only:
        return {"AIBalls":objects("AIBalls",1)[0]}
    fleet = objects("Fleet", 16)[:4]
    for row in fleet:
        ship = row.get("ship", {})
        row["ship"] = {key:ship[key] for key in ("pos","ang","speed","cannons") if key in ship}
        for key in tuple(row):
            if key not in ("index","maincharacter","ship","tmpskill","tmpperks","perks"):
                row.pop(key)
    return {"AIBalls":objects("AIBalls",1)[0],"Cannon14_15":objects("Cannon",16)[14:16],"Goods52":objects("Goods",53)[52],"RangeGun0":objects("RangeGun",51)[0],"Fleet0_3":fleet}

if __name__ == "__main__":
    print(json.dumps(attributes(sys.argv[1],"--balls" in sys.argv[2:]) if any(option in sys.argv[2:] for option in ("--attributes","--balls")) else read(sys.argv[1], "--dart" in sys.argv[2:]), indent=2))
