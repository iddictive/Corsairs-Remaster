"""Encode and inspect mips of the original-body-UV scarf experiment."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_gutter_tx_probe.py';source=path.read_text()
changes=[('cgmodelsnet-body-scarf-gutter.gm','cgmodelsnet-refined-rebridge-prototype.gm'),('cgmodelsnet-body-scarf-gutter-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('473d4ea6ae65a6ec4553b7e2812f57a9249e50638c71589b750f3cfd50def7ab','31e5b20a635ae45d40be8c5a8778fb81a49d00859b5851877a9241a5573ea739'),('cb6cca652223ee6f1fdce531af5ef43e0ab9c6a5d090288dfa62723f5ef40686','890846414f7c0b83c5e3cfeb25b569e902bac6e9634dac7bf7fb832d137366f5'),('putin-gutter-probe','putin-stockuv-probe'),('putin-gutter-mip-','putin-stockuv-mip-')]
for old,new in changes:
    assert source.count(old)>=1,old;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
