#!/usr/bin/env python3
"""Verify normal-bearing rigging uses scene lighting and authored coverage."""

from pathlib import Path
import sys


root = Path(sys.argv[1]) if len(sys.argv) > 1 else Path(__file__).parent / "engine"
sources = {}
effects = {}
for owner, vertex in (("rope", "ROPEVERTEX"), ("vant", "VANTVERTEX"), ("flag", "FLAGLXVERTEX")):
    source = sources[owner] = (root / f"src/libs/rigging/src/{owner}.cpp").read_text()
    header = (root / f"src/libs/rigging/src/{owner}.h").read_text()
    effects[owner] = (root / f"src/techniques/ship/{owner.title()}.fx").read_text()
    assert f"#define {vertex}_FORMAT (D3DFVF_XYZ | D3DFVF_NORMAL | D3DFVF_TEX1" in header
    assert "CVECTOR pos;\n    CVECTOR norm;\n    float tu, tv;" in header
    assert "riggingUnitNormal(" in source and ".norm =" in source
    assert source.index("RiggingLightingScope lightingScope") < source.index("DrawBuffer")
    assert source.index("D3DRS_TEXTUREFACTOR, 0xffffffff") < source.index("DrawBuffer")
    assert "GetRenderState(D3DRS_AMBIENT" not in source, "ambient must not be baked into texture RGB"
    if owner != "flag":
        assert source.index("SetLightAndFog(true)") < source.index("DrawBuffer")

# Normals follow moving geometry, including the bent strand, shared seam and cloth tip.
assert "riggingUnitNormal(around ^ tangent, pr->pos[i])" in sources["rope"]
assert sources["vant"].count("SetNormals(pv + vlist[vn]->sv);") == 2
assert "v[j].norm = v[j + downOffset].norm = normal;" in sources["vant"]
assert "v[2 * i + side].norm = riggingUnitNormal(across ^ along);" in sources["flag"]
assert "riggingUnitNormal(across ^ along, v[last].norm)" in sources["flag"]

# RGB consumes lit diffuse before the authored opacity factor; coverage stays authored.
for owner, text in effects.items():
    lit = text.split("technique GhostShip", 1)[0]
    for technique in lit.split("technique ")[1:]:
        for state in ("Lighting = true;", "CullMode = none;", "AlphaBlendEnable = true;",
                      "SrcBlend = srcalpha;", "DestBlend = invsrcalpha;",
                      "ColorArg1[0] = texture;", "ColorArg2[0] = current;",
                      "ColorArg1[1] = current;", "ColorArg2[1] = tfactor;",
                      "ColorOp[2] = disable;"):
            assert state in technique, f"{owner}: missing authored lighting/coverage state {state}"
        if "_alpha" in technique.split("{", 1)[0]:
            assert "AlphaTestEnable = false;" in technique and "ZWriteEnable = false;" in technique
            assert "AlphaOp[0] = modulate;" in technique and "AlphaArg2[0] = tfactor;" in technique
        else:
            assert "AlphaTestEnable = true;" in technique and "AlphaOp[0] = SelectArg1;" in technique
        assert "AlphaArg1[0] = texture;" in technique
ghost = effects["flag"].split("technique GhostShip", 1)[1]
for state in ("Lighting = false;", "FogEnable = false;", "DestBlend = one;",
              "TextureFactor = 0x2080ff80;", "AlphaOp[0] = modulate2x;"):
    assert state in ghost, "authored unlit GhostShip recipe changed"

# White reflectance leaves day/night energy with the common renderer; it adds no emission.
scope = (Path(__file__).parent / "rigging_lighting.hpp").read_text()
assert "D3DMATERIAL9 material{};" in scope
assert "material.Diffuse = material.Ambient = {1.f, 1.f, 1.f, 1.f};" in scope
assert "material.Emissive =" not in scope and "material.Specular =" not in scope
assert "device->SetMaterial(&previous);" in scope and "StormMetalSetRiggingDraw(device, previousRole);" in scope
backend = (Path(__file__).parent / "backend.mm").read_text()
sea_lit = backend.split("const bool rawSeaLit=", 1)[1].split(";", 1)[0]
assert "actualLighting" in sea_lit and "fvf&D3DFVF_NORMAL" in sea_lit and "riggingMaterialDraw" not in sea_lit
for predicate in ("worldDraw", "rawSeaShadow", "seaGeometryCandidate", "captureRawShadow"):
    assert f"const bool {predicate}=!riggingMaterialDraw" in backend, "rigging must retain its no-caster role"

print("PASS source contracts: rope/vant/flag normals feed directional scene lighting; authored alpha/fades and GhostShip remain intact; scoped non-emissive material preserves day/night lighting and no-caster role")
