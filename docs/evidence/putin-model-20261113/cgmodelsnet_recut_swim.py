"""Check the sourced swimming action against the exhaustive man-bank scan."""
import hashlib
import json
import re
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
definition=(here/'man-swim-source.ani').read_bytes()
assert hashlib.sha256(definition).hexdigest()=='e6f610358fa34cec6483c04bb345c701e25f68b1c6ef980d975be62ca143c1c9'
text=definition.decode('cp1251',errors='replace');assert re.search(r'(?im)^\s*animation\s*=\s*man\.an\s*$',text)
parts=re.split(r'(?m)^\s*\[([^\]]+)\]',text);body=next(b for n,b in zip(parts[1::2],parts[2::2]) if n.lower()=='swim')
a=re.search(r'(?im)^\s*start_time\s*=\s*(\d+)',body);b=re.search(r'(?im)^\s*end_time\s*=\s*(\d+)',body)
lo,hi=sorted((int(a.group(1)),int(b.group(1))))
rows=json.loads((here/'cgmodelsnet-recut-allframes-man.json').read_text())['rows'];worst=max(rows[lo:hi+1],key=lambda row:row[1])
print('swim frames',lo,hi,'worst',worst,flush=True)
path=here/'animated_face_render_trial.py';source=path.read_text()
for old,new in [('nose-subdivision-prototype.gm','cgmodelsnet-recut-trial.gm'),('identity-geometry-face-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),('animated-face-','animated-cgmodelsnet-recut-swim-'),("[('walk',10),('run',163),('fight-stand',8532)]",f"[('start',{lo}),('worst',{worst[0]}),('end',{hi})]")]:
    assert source.count(old)>=1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
