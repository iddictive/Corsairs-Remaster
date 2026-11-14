"""Render stock officer poses with the same camera as the graft."""
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
path=here/'cgmodelsnet_fullbody_trial.py';source=path.read_text()
start=source.index('cases=');end=source.index('\nfor label,gm,angle in cases:',start)
stock=root/'src/assets/characters/putin-phonk/original.gm'
assert stock.is_file()
source=source[:start]+f"cases=[('bind-front',{str(stock)!r},-.15),('bind-profile',{str(stock)!r},math.pi/2),('walk','animated-stock-officer-walk.gm',-.15),('8423','animated-stock-officer-8423.gm',-.15),('410','animated-stock-officer-410.gm',-.15),('1985','animated-stock-officer-1985.gm',-.15)]"+source[end:]
for old,new in [('cgmodelsnet-nose-baseline-atlas.png','officer_5-atlas.png'),('cgmodelsnet-fullbody-','stock-officer-fullbody-')]:
    assert source.count(old)>=1
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
