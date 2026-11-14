"""Repeat the scarf test using an uncontaminated stock fabric sample."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_scarf_surface_trial.py';source=path.read_text()
changes=[('stock[710:745,665:800]','stock[716:736,665:735]'),
         ('fabric.shape==(35,135,3)','fabric.shape==(20,70,3)'),
         ('cgmodelsnet-scarf-surface-', 'cgmodelsnet-scarf-clean-')]
for old,new in changes:
    assert source.count(old)>0,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
