from pathlib import Path
import sys, struct, zlib, json, hashlib

REPO = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(REPO / "tools"))
import compact_native_save as codec

Save, vd, wire_string, unpack = codec.Save, codec.vd, codec.wire_string, codec.unpack
assert not codec.WORLD_POLICY["enabled"]

def n(name, value=None, children=()):
    return name, value, list(children)

wc = n("PhonkWorldCuts", children=[n("count", b"500")])
wm = n("PhonkWorldMaterial", children=[n("payload", b"world geometry")])
worlds = n("Islands", children=[
    n("i0", children=[n("name", b"PuertoRico"), wc, wm]),
    n("i1", children=[n("name", b"SaintMartin"), n("PHONKWORLDCUTS", children=[n("count", b"3")]), n("phonkworldmaterial", children=[n("payload", b"other geometry")])]),
    n("i2", children=[n("name", b"untouched"), n("PhonkWorldCutsExtra", b"keep"), n("note", b"PhonkWorldMaterial")]),
    n("i3", children=[n("quest", children=[wc, n("progress", b"5")])]),
])
payload = b"abcd" * 1024
bslot = n("b", children=[n("revision", b"2"), n("name", b"hull"), n("blob", b"dataB"), n("payload", b"new-id"), n("tail", b"0")])
data = n("dataB", children=[n("id", b"new-id"), n("bytes", b"4096"), n("chunks", b"1"), n("p0", payload)])
meta = n("metaB", children=[n("revision", b"2"), n("remaining", b"0.7")])
fields = [n("version", b"1"), n("model", b"ship"), n("revision", b"2")]
hull = n("HullMaterial", children=fields + [n("metaA", children=[n("revision", b"1")]), meta, n("n0", children=[n("a", children=[n("revision", b"1")]), bslot, n("dataA", children=[n("id", b"old-id")]), data, wc])])
original = n("", children=[worlds, n("Ship", children=[hull]), n("quest", b"retained"), n("money", b"1932923")])

# Explicit expected retained tree; this oracle does not call the transform's filter.
expected_worlds = n("Islands", children=[
    n("i0", children=[n("name", b"PuertoRico")]),
    n("i1", children=[n("name", b"SaintMartin")]),
    n("i2", children=[n("name", b"untouched"), n("PhonkWorldCutsExtra", b"keep"), n("note", b"PhonkWorldMaterial")]),
    n("i3", children=[n("quest", children=[n("progress", b"5")])]),
])
expected_hull = n("HullMaterial", children=fields + [meta, n("n0", children=[bslot, data])])
expected = n("", children=[expected_worlds, n("Ship", children=[expected_hull]), n("quest", b"retained"), n("money", b"1932923")])

names = []
def collect(node):
    if node[0] not in names: names.append(node[0])
    for child in node[2]: collect(child)
collect(original)
codes, buckets = {}, {}
for name in names:
    h = 0
    for c in name.lower().encode():
        h = ((h << 4) + c) & 0xffffffff
        g = h & 0xf0000000
        if g: h ^= g >> 24; h ^= g
    bucket = h & 511
    index = buckets.get(bucket, 0)
    buckets[bucket] = index + 1
    codes[name] = (bucket << 16) | index

def attr(node):
    return vd(len(node[2])) + vd(codes[node[0]]) + wire_string(node[1]) + b"".join(attr(c) for c in node[2])

prefix = wire_string(b"PROGRAM") + vd(len(names)) + b"".join(wire_string(s.encode()) for s in names) + vd(1) + wire_string(b"fixture.c") + vd(2) + wire_string(b"TestObject") + struct.pack("<IIQ", 9, 1, 0)
suffix = wire_string(b"QuestInteger") + struct.pack("<IIi", 6, 1, 7)
def file(raw):
    packed = zlib.compress(raw, 9)
    return b"fixture".ljust(32, b"\0") + struct.pack("<4I", 0, 0, len(raw), len(packed)) + packed

raw = prefix + attr(original) + suffix
s = Save(file(raw))
assert len(s.world_attributes) == 6
out = s.compact()
c = Save(out)
assert len(c.world_attributes) == 0
expected_raw = prefix + attr(expected) + suffix
assert b"".join(c.parts(False, omit_world=False)) == expected_raw
assert s.digest() == c.digest(omit_world=False)
assert s.digest(omit_world=False) != s.digest()
assert unpack(c.roots[0].child(b"n0").child(b"datab").child(b"p0").value) == payload

# A policy flip is read from declarations, never from a second product flag.
path = Path(__file__).resolve().parent / "save-world-enabled-policy.hpp"
source = codec.WORLD_POLICY_PATH.read_text()
assert source.count("inline constexpr bool Enabled = false;") == 1
path.write_text(source.replace("inline constexpr bool Enabled = false;", "inline constexpr bool Enabled = true;"))
enabled_policy = codec.read_world_policy(path)
assert enabled_policy["enabled"]
previous = codec.WORLD_POLICY
try:
    codec.WORLD_POLICY = enabled_policy
    preserved = Save(Save(file(raw)).compact())
    assert len(preserved.world_attributes) == 6
    assert preserved.digest(omit_world=False) == s.digest(omit_world=False)
finally:
    codec.WORLD_POLICY = previous

root = Path(__file__).resolve().parent
(root / "save-world-synthetic-input.bin").write_bytes(file(raw))
(root / "save-world-synthetic-output.bin").write_bytes(out)
result = {"omitted_reserved_attributes": 6, "remaining": 0, "all_island_entries": 4, "case_insensitive": True,
          "nested_reserved_world_state_omitted": True, "hull_payload_bytes_preserved": 4096,
          "similar_names_and_value_literals_preserved": True, "ordinary_quest_money_int_preserved": True,
          "enabled_policy_retains_world_state": True, "explicit_independent_expected_raw_sha256": hashlib.sha256(expected_raw).hexdigest(),
          "retained_state_sha256": c.digest(omit_world=False), "input_raw": len(raw), "output_raw": len(c.raw)}
(root / "save-world-synthetic.json").write_text(json.dumps(result, indent=2))
print(json.dumps(result, indent=2))
