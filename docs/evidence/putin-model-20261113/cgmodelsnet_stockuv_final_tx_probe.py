"""Encode and inspect mips after applying the contour and weight changes."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_stockuv_tx_probe.py';source=path.read_text()
for old,new in [('cgmodelsnet-refined-rebridge-prototype.gm','cgmodelsnet-stockuv-retopology.gm'),('31e5b20a635ae45d40be8c5a8778fb81a49d00859b5851877a9241a5573ea739','667f956c2e0f20e3270b8aaca2f5cf85a0b09b7bc211a10c8cf5fbd63c04e3aa'),('putin-stockuv-probe','putin-stockuv-retopology-probe'),('putin-stockuv-mip-','putin-stockuv-retopology-mip-')]:
    assert source.count(old)>=1,old;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
