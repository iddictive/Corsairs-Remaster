#!/usr/bin/env python3
from background_alpha import ANCHOR, PATCHED, prepare


source = ("before\n" + ANCHOR + "after\n").encode()
patched, changed = prepare(source)
assert changed and patched.decode() == "before\n" + PATCHED + "after\n"
assert prepare(patched) == (patched, False)
crlf = source.replace(b"\n", b"\r\n")
crlf_patched, changed = prepare(crlf)
assert changed and b'plan.tech = "LocationWindows";\r\n' in crlf_patched
assert crlf_patched.count(b"\r\n") == crlf.count(b"\r\n") + 1

nearby = source + ANCHOR.encode()
try:
    prepare(nearby)
except ValueError:
    pass
else:
    raise AssertionError("ambiguous backdrop definitions must be rejected")

assert b'LocationWindows' not in source
print("PASS: one St Martin backdrop gets alpha blending; unrelated/ambiguous input is rejected")
