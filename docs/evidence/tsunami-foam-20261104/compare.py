#!/usr/bin/env python3
"""Render saved shader snapshots; arguments: probe executable, PENA TX, output directory."""
from pathlib import Path
import subprocess
import sys

import numpy as np
from PIL import Image

here = Path(__file__).resolve().parent
probe, texture, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
for amplitude in (14, 28, 56):
    for version in ('baseline', 'sine-115', 'sine-180'):
        shader = (here / (version + '.metal')).read_text()
        original = 'float4(1,1,tm,28)'
        assert shader.count(original) == 1
        shader = shader.replace(original, f'float4(1,1,tm,{amplitude})')
        path = output / f'{version}-{amplitude}.metal'
        path.write_text(shader)
        for time in (0, 20):
            image = output / f'{version}-{amplitude}-{time}.png'
            run = subprocess.run([str(probe), str(path), str(texture), str(image), str(time)],
                                 check=True, capture_output=True, text=True)
            values = np.asarray(Image.open(image).convert('L'), dtype=np.float64) / 255
            centered = values - values.mean()
            shift = round((amplitude * 18 / 28) / 180 * 1024)
            def correlation(axis):
                left = centered[:, :-shift] if axis == 0 else centered[:-shift, :]
                right = centered[:, shift:] if axis == 0 else centered[shift:, :]
                return np.corrcoef(left.ravel(), right.ravel())[0, 1]
            detail = (np.abs(np.diff(values, axis=0)).mean() +
                      np.abs(np.diff(values, axis=1)).mean()) / 2
            print(f'{version} amp={amplitude} time={time} mean={values.mean():.4f} '
                  f'std={values.std():.4f} neighbor_diff={detail:.4f} tile_shift_px={shift} '
                  f'corr_x={correlation(0):.4f} corr_y={correlation(1):.4f} '
                  + run.stdout.strip(), flush=True)
