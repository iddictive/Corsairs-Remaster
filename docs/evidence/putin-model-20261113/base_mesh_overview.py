"""Compare existing character head geometry without opening the game."""
import sys
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(root / 'tools'))
from prepare_roundhouse import animated_mesh
from render_unarmed_atlas import panel

folder = root / 'experiments/native-metal/.cache/runtime/RESOURCE/MODELS/Characters'
names = [f'barmen_{i}' for i in range(28)] + [f'officer_{i}' for i in range(1, 29)] + ['Researcher', 'QuestMan1', 'BigPirate', 'capitan']
canvas = Image.new('RGB', (200 * 8, 240 * ((len(names)+7)//8)))
for i, name in enumerate(names):
    path = folder / (name + '.gm')
    try:
        vertices, faces = animated_mesh(path.read_bytes())
        points = [v[0] for v in vertices]
        im = panel(points, faces, 0, (200, 240), name, (0, 1.45, 500))
        canvas.paste(im, ((i % 8)*200, (i // 8)*240))
    except (OSError, ValueError, IndexError) as e:
        print(name, str(e))
canvas.save(Path(__file__).with_name('base-mesh-overview.png'))
