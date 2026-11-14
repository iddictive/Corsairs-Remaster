"""Replay the established bridge, body-edge and visual checks for fitted weights."""
from pathlib import Path
here=Path(__file__).resolve().parent
for name in ['cgmodelsnet_neck_body_follow_check.py','cgmodelsnet_neck_body_follow_edge_check.py']:
    path=here/name;source=path.read_text()
    assert 'cgmodelsnet-neck-body-follow-' in source
    source=source.replace('cgmodelsnet-neck-body-follow-','cgmodelsnet-neck-body-fit-')
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
