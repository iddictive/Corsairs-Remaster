"""Render the stock officer at the same sampled man.an frames for comparison."""
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
path=here/'animated_face_render_trial.py';source=path.read_text()
stock=root/'src/assets/characters/putin-phonk/original.gm'
assert stock.is_file()
needle='src=renderer.read_text()'
assert source.count(needle)==1
source=source.replace(needle,needle+"\n    src=src.replace(\"original_tx = (base / f'Textures/Characters/{textures[model]}.tga.tx').read_bytes()\",\"original_tx = (root / 'src/assets/characters/putin-phonk/original.tga.tx').read_bytes()\")")
for old,new in [('nose-subdivision-prototype.gm',str(stock)),('identity-geometry-face-atlas.png','officer_5-atlas.png'),('animated-face-','animated-stock-officer-'),("[('walk',10),('run',163),('fight-stand',8532)]","[('walk',10),('run',163),('fight-stand',8532),('8423',8423),('410',410),('1985',1985)]")]:
    assert source.count(old)>=1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
