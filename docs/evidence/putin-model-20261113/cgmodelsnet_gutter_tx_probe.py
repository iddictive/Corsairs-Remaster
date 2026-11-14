"""Encode the gutter candidate and render the same mip diagnostics."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_tx_format_probe.py';source=path.read_text()
changes=[('cgmodelsnet-neck-retopology-fit.gm','cgmodelsnet-body-scarf-gutter.gm'),('cgmodelsnet-nose-baseline-atlas.png','cgmodelsnet-body-scarf-gutter-atlas.png'),('e73916910134f6d9182f837fb46bc22b64ab17c98bf3dd85fb5897554fda9f05','473d4ea6ae65a6ec4553b7e2812f57a9249e50638c71589b750f3cfd50def7ab'),('4b8c225f922a04d279c1e8477ef4fd20d9b68651bd167110982e924d68ede776','cb6cca652223ee6f1fdce531af5ef43e0ab9c6a5d090288dfa62723f5ef40686'),('putin-format-probe.gm','putin-gutter-probe.gm'),('putin-format-probe.tga.tx','putin-gutter-probe.tga.tx')]
for old,new in changes:
    assert source.count(old)==1,old;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_tx_mip_render_probe.py';source=path.read_text()
assert source.count("source=path.read_text()") == 1
for old,new in [('putin-format-probe.tga.tx','putin-gutter-probe.tga.tx'),('putin-format-mip-','putin-gutter-mip-')]:
    assert source.count(old)>=1;source=source.replace(old,new)
source=source.replace("source=path.read_text()","source=path.read_text();source=source.replace('putin-format-probe.gm','putin-gutter-probe.gm');source=source.replace('putin-format-probe.tga.tx','putin-gutter-probe.tga.tx')")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
