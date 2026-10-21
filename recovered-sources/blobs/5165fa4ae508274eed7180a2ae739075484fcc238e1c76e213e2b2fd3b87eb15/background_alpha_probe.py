#!/usr/bin/env python3
from background_alpha import (
    ANCHOR,
    LEGACY_PATCHED,
    LOADER_ANCHOR,
    LOADER_PATCHED,
    PATCHED,
    prepare,
    prepare_file,
    prepare_loader,
)


source = ("before\n" + ANCHOR + "after\n").encode()
patched, changed = prepare(source)
assert changed and patched.decode() == "before\n" + PATCHED + "after\n"
assert prepare(patched) == (patched, False)
legacy_patched, changed = prepare(LEGACY_PATCHED.encode())
assert changed and legacy_patched == PATCHED.encode()
crlf = source.replace(b"\n", b"\r\n")
crlf_patched, changed = prepare(crlf)
assert changed and b'plan.tech = "LocationModelBlend";\r\n' in crlf_patched
assert crlf_patched.count(b"\r\n") == crlf.count(b"\r\n") + 1

multiple = source + ANCHOR.encode()
multiple_patched, changed = prepare(multiple)
assert changed and multiple_patched.count(PATCHED.encode()) == 2
assert prepare(multiple_patched) == (multiple_patched, False)

assert b'LocationModelBlend' not in source
explicit = b'\tlocations[n].models.always.plan = "plan1";\n\tlocations[n].models.always.plan.tech = "DLightModel";\n\tlocations[n].models.always.plan.level = 9;\n'
assert prepare(explicit) == (explicit, False)
unrelated = b'\tlocations[n].models.always.plan = "plan1";\n\tlocations[n].models.always.plan.level = 12;\n'
assert prepare(unrelated) == (unrelated, False)
puerto_rico, changed = prepare_file("PROGRAM/locations/init/PuertoRico.c", source)
assert changed and PATCHED.encode() in puerto_rico
assert prepare_file("PROGRAM/locations/init/PuertoRico.c", puerto_rico) == (puerto_rico, False)
assert prepare_file("PROGRAM/locations/PuertoRico.c", source) == (source, False)
assert prepare_file("RESOURCE/INI/PuertoRico.c", source) == (source, False)

loader = ("before\n" + LOADER_ANCHOR + "after\n").encode()
loader_patched, changed = prepare_loader(loader)
assert changed and loader_patched.decode() == "before\n" + LOADER_PATCHED + "after\n"
assert prepare_loader(loader_patched) == (loader_patched, False)
loader_crlf = loader.replace(b"\n", b"\r\n")
loader_crlf_patched, changed = prepare_loader(loader_crlf)
assert changed and b'loc.(sat) == "plan1" && level == 9' in loader_crlf_patched
assert loader_crlf_patched.count(b"\r\n") == loader_crlf.count(b"\r\n") + 3
assert prepare_file("PROGRAM/locations/locations_loader.c", loader) == (loader_patched, True)

# The transition only fills an absent technique. Explicit techniques remain the
# canonical owner, and the role guard excludes ordinary opaque ground/models.
assert 'tech == ""' in LOADER_PATCHED
assert 'loc.(sat) == "plan1"' in LOADER_PATCHED
assert "level == 9" in LOADER_PATCHED
print("PASS: init files and serialized old saves route level-9 plan1 town backdrops through LocationModelBlend")
