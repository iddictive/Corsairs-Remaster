"""Freeze the actual Registry; only expose load action, never rewrite raster math."""
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[2]
SOURCE = ROOT / "experiments/native-metal/point_shadow_registry.hpp"
text = SOURCE.read_text()
original = text.encode()
signature = "const std::array<GpuFace,6>& faces) {"
load = "pass.depthAttachment.loadAction=MTLLoadActionClear;"
assert text.count(signature) == text.count(load) == 1
text = text.replace(signature, "const std::array<GpuFace,6>& faces, MTLLoadAction initialLoad=MTLLoadActionClear) {")
text = text.replace(load, "pass.depthAttachment.loadAction=initialLoad;")
(HERE / "registry_original.hpp").write_bytes(original)
(HERE / "registry_probe.hpp").write_text(text)
(HERE / "header-provenance.json").write_text(json.dumps({
    "source": str(SOURCE.relative_to(ROOT)),
    "source_sha256": hashlib.sha256(original).hexdigest(),
    "probe_sha256": hashlib.sha256(text.encode()).hexdigest(),
    "changes": ["encodeCube accepts load action", "depth attachment uses supplied load action"],
    "shader_bytes_unchanged": True,
}, indent=2) + "\n")
