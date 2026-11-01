import sys, types
if "PIL" not in sys.modules:
    pil = types.ModuleType("PIL")
    pil.Image = pil.ImageChops = pil.ImageDraw = pil.ImageFilter = types.ModuleType("PIL.dummy")
    sys.modules["PIL"] = pil
    sys.modules["PIL.Image"] = pil.Image
    sys.modules["PIL.ImageChops"] = pil.ImageChops
    sys.modules["PIL.ImageDraw"] = pil.ImageDraw
    sys.modules["PIL.ImageFilter"] = pil.ImageFilter
