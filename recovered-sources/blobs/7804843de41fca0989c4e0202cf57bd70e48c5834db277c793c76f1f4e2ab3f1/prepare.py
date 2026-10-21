"""Prepare a source-only sparse ICB candidate and reuse the retained 88-case oracle.

Never compiles, runs Metal, edits production source, or launches the game.
"""
from pathlib import Path
import ast
import difflib
import hashlib
import json
import struct

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = ROOT / "experiments/native-metal/point_shadow_registry.hpp"
CAPTURE = ROOT / "experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-001520/captured-bounds-fit"
ORACLE = ROOT / ".local-archives/metal-renderer-evidence/ship-local-cache-fit/probe.mm"
SKIN_FIT = ROOT / ".local-archives/metal-renderer-evidence/compute-skin-cache-fit/inspect.py"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def replace_once(text, old, new):
    assert text.count(old) == 1, old
    return text.replace(old, new)


original = SOURCE.read_text()
candidate = replace_once(original,
    "[blit resetCommandsInBuffer:frame->icb withRange:NSMakeRange(0,draws.size()*6)];",
    "// Reset all previous optimized ranges before caster-count-dependent face boundaries move.\n"
    "        [blit resetCommandsInBuffer:frame->icb withRange:NSMakeRange(0,MaxCasters*6)];")
candidate = replace_once(candidate,
    "++stats.gpuCullDispatches;stats.candidateCommands=uint32_t(draws.size()*6);",
    "++stats.gpuCullDispatches;stats.candidateCommands=uint32_t(draws.size()*6);\n"
    "        // Metal requires execution to match an entire optimized range. Each\n"
    "        // face is executed separately, so optimize those same disjoint ranges.\n"
    "        auto optimize=[command blitCommandEncoder];\n"
    "        for(unsigned face=0;face<6;++face)\n"
    "            [optimize optimizeIndirectCommandBuffer:frame->icb withRange:NSMakeRange(face*draws.size(),draws.size())];\n"
    "        [optimize endEncoding];")
assert original.split('shaderSource=R"MSL(', 1)[1] == candidate.split('shaderSource=R"MSL(', 1)[1]
(HERE / "point_shadow_registry.hpp").write_text(candidate)
(HERE / "candidate.patch").write_text("".join(difflib.unified_diff(original.splitlines(True), candidate.splitlines(True),
    fromfile="a/experiments/native-metal/point_shadow_registry.hpp", tofile="b/experiments/native-metal/point_shadow_registry.hpp")))

# Test-only adapter: the retained oracle already compares fresh/split depth over
# 88 cases. Fresh bypasses optimization; split paths use the candidate operation.
# This preserves the canonical shader and has no production API consequence.
probe_header = replace_once(candidate, "const std::array<GpuFace,6>& faces) {",
    "const std::array<GpuFace,6>& faces, MTLLoadAction initialLoad=MTLLoadActionClear, bool optimizeCommands=true) {")
probe_header = replace_once(probe_header, "pass.depthAttachment.loadAction=MTLLoadActionClear;",
    "pass.depthAttachment.loadAction=initialLoad;")
probe_header = replace_once(probe_header, "auto optimize=[command blitCommandEncoder];",
    "if(optimizeCommands){auto optimize=[command blitCommandEncoder];")
probe_header = replace_once(probe_header, "[optimize endEncoding];", "[optimize endEncoding];}")
(HERE / "registry_probe.hpp").write_text(probe_header)
probe = ORACLE.read_text()
probe = replace_once(probe, "bool empty,MTLLoadAction load){", "bool empty,MTLLoadAction load,bool optimize=true){")
probe = replace_once(probe, "reg.encodeCube(cb,cube,faces,load)", "reg.encodeCube(cb,cube,faces,load,optimize)")
probe = replace_once(probe, "render(fresh,cb,reference,faces,all.empty(),MTLLoadActionClear);",
    "render(fresh,cb,reference,faces,all.empty(),MTLLoadActionClear,false);")
probe = replace_once(probe, "render(fresh,cb,reference,faces,rows.empty(),MTLLoadActionClear);",
    "render(fresh,cb,reference,faces,rows.empty(),MTLLoadActionClear,false);")
(HERE / "probe.mm").write_text(probe)

# Reuse the existing cull translation without importing its full mesh analysis.
module = ast.parse(SKIN_FIT.read_text())
functions = [node for node in module.body if isinstance(node, ast.FunctionDef) and node.name in ("f32", "survives")]
assert len(functions) == 2
namespace = {"struct": struct}
exec(compile(ast.Module(body=functions, type_ignores=[]), str(SKIN_FIT), "exec"), namespace)
survives = namespace["survives"]
inputs, cubes = {}, []
for cube in range(8):
    blobs = {}
    for kind in ("draws", "bounds", "faces", "resources"):
        path = CAPTURE / f"cube{cube}-{kind}.bin"
        blobs[kind] = path.read_bytes()
        inputs[str(path.relative_to(ROOT))] = sha(blobs[kind])
    draw_count = 0
    for i in range(len(blobs["draws"]) // 160):
        count = struct.unpack_from("<I", blobs["draws"], i * 160 + 24)[0]
        if not count:
            break
        assert struct.unpack_from("<I", blobs["draws"], i * 160 + 16)[0] == 0
        draw_count += 1
    assert draw_count == 701
    faces = []
    for face in range(6):
        values = struct.unpack_from("<20f", blobs["faces"], face * 80)
        live, skinned = [], 0
        for i in range(draw_count):
            bounds = struct.unpack_from("<8f", blobs["bounds"], i * 32)
            visible = survives(bounds, values)
            assert visible == survives(bounds, values, True)
            if visible:
                live.append(i)
                skinned += bool(struct.unpack_from("<I", blobs["draws"], i * 160 + 60)[0])
        live_set = set(live)
        runs, length = [], 0
        for i in range(draw_count):
            if i not in live_set:
                length += 1
            elif length:
                runs.append(length)
                length = 0
        if length:
            runs.append(length)
        faces.append({"face": face, "live": len(live), "skinned": skinned, "rigid": len(live)-skinned,
            "empty": draw_count-len(live), "empty_runs": len(runs), "longest_empty_run": max(runs, default=0)})
    cubes.append({"cube": cube, "casters": draw_count, "faces": faces})

live = sum(face["live"] for cube in cubes for face in cube["faces"])
assert live == 1210
receipt = {
    "disposition": "SOURCE_ONLY_PENDING_ROOT_COMPILE_GPU_DEPTH_AND_WHOLE_FRAME_TIMING",
    "production_source": str(SOURCE.relative_to(ROOT)), "production_sha256": sha(original.encode()),
    "candidate_sha256": sha(candidate.encode()), "candidate_patch_sha256": sha((HERE / "candidate.patch").read_bytes()),
    "oracle_original_sha256": sha(ORACLE.read_bytes()), "oracle_adapted_sha256": sha(probe.encode()),
    "oracle_adapter_sha256": sha(probe_header.encode()),
    "abi": "Unchanged GpuDraw160/GpuBounds32/GpuFace80; shader source and Resources argument IDs byte-identical",
    "lifecycle": "Existing private ICB per cube/framePool24; completion handler remains sole reuse owner; full12288-slot reset removes prior optimized ranges before variable-count face boundaries move",
    "resources": "Existing compute useResource Write and render residency Read declarations unchanged; direct blit operation names its resource",
    "oracle": "Existing8cubes*11cases depth compositor; fresh bypasses optimize, split paths use optimize; synthetic alpha remains structural only; does not prove framePool slot reuse or actual captured material parity",
    "compile": "xcrun clang++ -std=c++20 -fobjc-arc -O2 -framework Metal -framework Foundation .local-archives/metal-renderer-evidence/sparse-shadow-command-fit/probe.mm -o .local-archives/metal-renderer-evidence/sparse-shadow-command-fit/probe",
    "run": ".local-archives/metal-renderer-evidence/sparse-shadow-command-fit/probe --capture experiments/native-metal/.cache/diagnostics/sea-fleet-benchmark/20261015-001520/captured-bounds-fit --synthetic-alpha",
    "captured_slots": 33648, "captured_live": live, "captured_empty": 33648-live,
    "empty_fraction": (33648-live)/33648, "cubes": cubes, "inputs_sha256": inputs,
    "reuse_decision": "Use Apple built-in optimize first: no prefix-sum/atomic compaction shader, count readback, execution-range buffer, new resource lifetime or draw-order policy",
    "counterargument": "8 extra blit encoders/48 optimize calls plus full-capacity reset (98304 slots vs33648) may cost more than cheap no-op walking; no speedup is proven",
    "required_next_evidence": ["root compile", "88-case depth parity", "24-slot reuse with shrinking/growing caster counts", "matched full-quality whole-frame GPU/CPU cadence A/B"],
    "apple_contracts": ["https://developer.apple.com/documentation/metal/mtlblitcommandencoder/optimizeindirectcommandbuffer:withrange:", "https://developer.apple.com/documentation/metal/encoding-indirect-command-buffers-on-the-gpu"],
}
(HERE / "result.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps({key: receipt[key] for key in ("disposition", "candidate_sha256", "captured_live", "captured_empty", "empty_fraction")}, indent=2))
