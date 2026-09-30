#!/usr/bin/env python3
"""Reject an under-initialized sea-effect postprocess grid.

The renderer's POST_PROCESS_FVF contains four UV sets and the Metal pixel shader
samples stages 0..3.  Every sea-grid vertex therefore has to initialize all four
sets deterministically before the indexed draw.
"""

from __future__ import annotations

import pathlib
import re
import sys


def require(value: bool, message: str) -> None:
    if not value:
        raise SystemExit(f"FAIL: {message}")


def main() -> None:
    if len(sys.argv) != 2:
        raise SystemExit("usage: sea_postprocess_uv_probe.py <storm-source-root>")
    source = pathlib.Path(sys.argv[1]) / "src/libs/renderer/src/s_device.cpp"
    text = source.read_text(encoding="utf-8")
    start = text.index("void DX9RENDER::RunStart()")
    end = text.index("bNeedCopyToScreen = true;", start)
    body = text[start:end]
    for stage in range(1, 4):
        require(
            re.search(rf"qv\[x \+ y \* 32\]\.u{stage}\s*=\s*qv\[x \+ y \* 32\]\.u0\s*;", body)
            is not None,
            f"sea grid initializes u{stage} from authored distorted u0",
        )
        require(
            re.search(rf"qv\[x \+ y \* 32\]\.v{stage}\s*=\s*qv\[x \+ y \* 32\]\.v0\s*;", body)
            is not None,
            f"sea grid initializes v{stage} from authored distorted v0",
        )
    require("DrawIndexedPrimitiveUP(D3DPT_TRIANGLELIST, 0, 32 * 32" in text,
            "probe is bound to the real indexed sea postprocess consumer")
    print("PASS: all four sea postprocess UV sets are deterministic")


if __name__ == "__main__":
    main()
