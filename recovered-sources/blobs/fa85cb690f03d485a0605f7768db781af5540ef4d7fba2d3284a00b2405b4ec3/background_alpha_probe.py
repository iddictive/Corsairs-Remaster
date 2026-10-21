#!/usr/bin/env python3
from background_alpha import ANCHOR, PATCHED, prepare, prepare_file


source = ("before\n" + PATCHED + "after\n").encode()
patched, changed = prepare(source)
assert changed and patched.decode() == "before\n" + ANCHOR + "after\n"
assert prepare(patched) == (patched, False)
crlf = source.replace(b"\n", b"\r\n")
crlf_patched, changed = prepare(crlf)
assert changed and b'plan.tech = "LocationWindows";\r\n' not in crlf_patched
assert crlf_patched.count(b"\r\n") == crlf.count(b"\r\n") - 1

multiple = source + PATCHED.encode()
multiple_patched, changed = prepare(multiple)
assert changed and multiple_patched.count(ANCHOR.encode()) == 2
assert prepare(multiple_patched) == (multiple_patched, False)

assert b'LocationWindows' in source
explicit = b'\tlocations[n].models.always.plan = "plan1";\n\tlocations[n].models.always.plan.tech = "DLightModel";\n\tlocations[n].models.always.plan.level = 9;\n'
assert prepare(explicit) == (explicit, False)
unrelated = b'\tlocations[n].models.always.plan = "plan1";\n\tlocations[n].models.always.plan.level = 12;\n'
assert prepare(unrelated) == (unrelated, False)
puerto_rico, changed = prepare_file("PROGRAM/locations/init/PuertoRico.c", source)
assert changed and PATCHED.encode() not in puerto_rico
assert prepare_file("PROGRAM/locations/init/PuertoRico.c", puerto_rico) == (puerto_rico, False)
assert prepare_file("PROGRAM/locations/PuertoRico.c", source) == (source, False)
assert prepare_file("RESOURCE/INI/PuertoRico.c", source) == (source, False)
print("PASS: level-9 plan1 backdrops retain authored technique; explicit unrelated techniques remain unchanged")
