"""Prepare shader inputs, including one CPU-only C++ emitter compile/execution.

Generated files stay below this private diagnostic owner. The baseline is the
exact Device material-library expression. Candidate functions are derived from
the actual raw-hull entries by asserted, single substitutions. The emitter
includes only the actual cloud_visibility_msl.hpp to let C++ evaluate its macro-
composed string. No Metal compilation, GPU, game runtime or app access occurs.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = next(p for p in HERE.parents if (p / "experiments/native-metal/backend.mm").is_file())
SRC = ROOT / "experiments/native-metal"
parser = argparse.ArgumentParser()
parser.add_argument("--out", type=Path, default=HERE / "generated")
args = parser.parse_args()
out = args.out.resolve()
assert out.is_relative_to(HERE) and out != HERE, "output needs its own directory under this diagnostic owner"
inputs = {}
cpu_evaluation = {}


def read(name):
    data = (SRC / name).read_bytes()
    digest = hashlib.sha256(data).hexdigest()
    assert name not in inputs or inputs[name] == digest, ("source changed during extraction", name)
    inputs[name] = digest
    return data.decode()


def raw(name, symbol):
    text = read(name)
    matches = list(re.finditer(r"\b" + re.escape(symbol) + r'\s*=\s*R"MSL\((.*?)\)MSL"', text, re.S))
    assert len(matches) == 1, (name, symbol, len(matches))
    # Only an entire single-literal initializer is supported here. In particular,
    # reject an adjacent macro/string/operator instead of accepting its prefix.
    assert text[matches[0].end():].lstrip().startswith(";"), ("not a standalone raw initializer", name, symbol)
    return matches[0][1]


def cloud_visibility():
    name = "cloud_visibility_msl.hpp"
    text = read(name)
    cpp_only = re.sub(r'R"MSL\(.*?\)MSL"', '""', text, flags=re.S)
    assert not re.search(r"^\s*#\s*include\b", cpp_only, re.M), "cloud emitter dependency closure changed"
    pieces = re.findall(r'R"MSL\((.*?)\)MSL"', text, re.S)
    assert len(pieces) == 2, "review cloud initializer composition before changing this emitter"
    emitter = '''#include <cstdio>
#include <cstring>
#include "cloud_visibility_msl.hpp"
int main(){const char* value=storm_metal::cloudVisibilityMSL;
const size_t size=std::strlen(value);return std::fwrite(value,1,size,stdout)==size?0:1;}
'''
    out.mkdir(parents=True, exist_ok=True)
    source = out / "emit-cloud-visibility.cpp"
    executable = out / "emit-cloud-visibility"
    source.write_text(emitter)
    command = ["xcrun", "--sdk", "macosx", "clang++", "-std=c++20", "-I", str(SRC), str(source), "-o", str(executable)]
    compilation = subprocess.run(command, capture_output=True)
    (out / "emit-cloud-visibility-compile.stdout").write_bytes(compilation.stdout)
    (out / "emit-cloud-visibility-compile.stderr").write_bytes(compilation.stderr)
    compilation.check_returncode()
    emitted = subprocess.run([str(executable)], capture_output=True, check=True)
    assert not emitted.stderr, "CPU emitter unexpectedly wrote stderr"
    value = emitted.stdout.decode()
    assert value.startswith(pieces[0]) and value.endswith(pieces[1]), "partial cloud initializer output"
    assert len(value) > sum(map(len, pieces)), "cloud macro expansion missing"
    read(name)  # The compiler must have consumed the same recorded header bytes.
    (out / "cloud-visibility.metal").write_bytes(emitted.stdout)
    version = subprocess.run(["xcrun", "--sdk", "macosx", "clang++", "--version"], capture_output=True, check=True)
    cpu_evaluation[name] = {"command": command, "compiler_version": version.stdout.decode(),
                            "emitter_sha256": hashlib.sha256(emitter.encode()).hexdigest(),
                            "emitter_binary_sha256": hashlib.sha256(executable.read_bytes()).hexdigest(),
                            "output_sha256": hashlib.sha256(emitted.stdout).hexdigest(),
                            "contract": "C++ compiler evaluates complete actual header; no MSL compiler or GPU"}
    return value


def once(text, old, new):
    assert text.count(old) == 1, (old, text.count(old))
    return text.replace(old, new)


backend = read("backend.mm")
symbols = {
    "storm_metal::solarShapesMSL": raw("solar_shapes_msl.hpp", "solarShapesMSL"),
    "storm_metal::ffpSpecializationMSL": raw("ffp_specialization.hpp", "ffpSpecializationMSL"),
    "storm_metal::shipFireCommonMSL": raw("ship_fire_shaders.hpp", "shipFireCommonMSL"),
    "antiTilingMSL": raw("anti_tiling_msl.hpp", "antiTilingMSL"),
    "storm_metal::cloudVisibilityMSL": cloud_visibility(),
    "storm_metal::skyFogMSL": raw("sky_fog.hpp", "skyFogMSL"),
    "storm_metal::skyIrradianceMSL": raw("sky_irradiance.hpp", "skyIrradianceMSL"),
    "storm_metal::point_shadow::SamplingMSL": raw("point_shadow_registry.hpp", "SamplingMSL"),
    "landShaderSource": raw("land_shaders.hpp", "landShaderSource"),
    "postprocessShaderSource": raw("postprocess_shaders.hpp", "postprocessShaderSource"),
    "LandShadow::materialSource": raw("land_shadow.hpp", "materialSource"),
}


def evaluate(expression):
    token = re.compile(r'\s*(?:\+\s*)?(?:R"MSL\((.*?)\)MSL"|std::string\((\w+(?:::\w+)*)\)|(\w+(?:::\w+)*))', re.S)
    parts, pos = [], 0
    while pos < len(expression):
        m = token.match(expression, pos)
        assert m, expression[pos:pos + 100]
        parts.append(m[1] if m[1] is not None else symbols[m[2] or m[3]])
        pos = m.end()
    return "".join(parts)


expr = re.search(r'const std::string shaders=(.*?\)MSL");', backend, re.S)
assert expr
symbols["shaders"] = evaluate(expr[1])
expr = re.search(r'materialShaderSource=(.*?);const auto&source=materialShaderSource;', backend)
assert expr, "Device material-library composition changed"
baseline = evaluate(expr[1])


def function(text, signature):
    start = text.index(signature)
    brace = text.index("{", start)
    level, end = 1, brace + 1
    while level:
        level += (text[end] == "{") - (text[end] == "}")
        end += 1
    return text[start:end]


vertex = function(baseline, "vertex O vs_landraw(")
vertex = once(vertex, "vs_landraw(", "vs_landraw_instances(")
vertex = once(vertex, "uint i [[vertex_id]]", "uint i [[vertex_id]],uint instanceIndex [[instance_id]]")
vertex = once(vertex, "constant RawLandU&r [[buffer(3)]]", "constant RawLandU*rr [[buffer(3)]]")
vertex = once(vertex, "constant LandSampling&s [[buffer(4)]]", "constant LandSampling*ss [[buffer(4)]]")
vertex = once(vertex, ") {", ") {\n constant RawLandU&r=rr[instanceIndex];constant LandSampling&s=ss[instanceIndex];")
vertex = once(vertex, "O o;o.p=u.m*", "O o;o.instanceIndex=instanceIndex;o.p=r.m*")
fragment = function(baseline, "fragment float4 fs_landlit_skyfog(")
fragment = once(fragment, "fs_landlit_skyfog(", "fs_landlit_skyfog_instances(")
fragment = once(fragment, "constant LandSampling&s [[buffer(2)]]", "constant LandSampling*ss [[buffer(2)]]")
fragment = once(fragment, ") {return ", ") {constant LandSampling&s=ss[v.instanceIndex];return ")
candidate = once(baseline, "struct O { ", "struct O { uint instanceIndex [[flat]]; ") + "\n" + vertex + "\n" + fragment

# CPU ABI comes verbatim from its current owner, not a second maintained model.
cpu = []
for name in ("Stage", "Uniform", "RawLandUniform"):
    matches = list(re.finditer(r"^struct " + name + r" \{simd_[^\n]*\};", backend, re.M))
    assert len(matches) == 1, ("unique CPU SIMD declaration required", name, len(matches))
    cpu.append(matches[0][0])
shadow = read("land_shadow.hpp")
for name in ("PointLight", "Sampling"):
    m = re.search(r"^\s*struct " + name + r" \{[^\n]*\};", shadow, re.M)
    assert m, name
    cpu.append(m[0])
header = "#pragma once\n" + "\n".join(cpu) + "\nstatic_assert(sizeof(Uniform)==1328);\nstatic_assert(sizeof(Sampling)==3488);\n"
provenance = {"inputs_sha256": inputs,
              "cpu_evaluation": cpu_evaluation,
              "diagnostic_sources_sha256": {name: hashlib.sha256((HERE / name).read_bytes()).hexdigest()
                                            for name in ("prepare-probe.py", "probe.mm")},
              "baseline_msl_sha256": hashlib.sha256(baseline.encode()).hexdigest(),
              "candidate_msl_sha256": hashlib.sha256(candidate.encode()).hexdigest(),
              "generated_header_sha256": hashlib.sha256(header.encode()).hexdigest(),
              "scope": "synthetic raw-hull shading; no reordering, mesh merging, batch admission, production delivery or FPS acceptance",
              "derivation": "asserted single replacements of actual vs_landraw and fs_landlit_skyfog; per-instance RawLandU and LandSampling; shared U and per-view SkyFogDraw"}
out.mkdir(parents=True, exist_ok=True)
(out / "baseline.metal").write_text(baseline)
(out / "candidate.metal").write_text(candidate)
(out / "generated.hpp").write_text(header)
(out / "provenance.json").write_text(json.dumps(provenance, indent=2) + "\n")
print(json.dumps({"output": str(out), **provenance}, indent=2))
