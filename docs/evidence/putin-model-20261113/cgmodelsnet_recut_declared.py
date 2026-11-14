"""Map the rebuilt bridge's exhaustive scans to declared ANI actions."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_declared_actions_scan.py';source=path.read_text()
for old,new in [('cgmodelsnet-neck-allframes-','cgmodelsnet-recut-allframes-'),('cgmodelsnet-declared-actions-scan.json','cgmodelsnet-recut-declared-actions.json')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
