"""Freeze retained captured index spans for the CPU-only ordered cache probe."""
from pathlib import Path
import contextlib, hashlib, io, json, runpy, struct

HERE=Path(__file__).resolve().parent
with contextlib.redirect_stdout(io.StringIO()):
    fit=runpy.run_path(str(HERE.parent/'ordered-material-batch-fit/analyze.py'))
runs=[row[0] for row in fit['runs']]
ids=sorted({r['address'][2] for run in runs for r in run})
inputs={}
with (HERE/'index-fixture.bin').open('wb') as out:
    out.write(b'OBIX0001');out.write(struct.pack('<I',len(ids)))
    for resource in ids:
        path=fit['geo']['CAPTURE']/f'buf{resource}.bin';data=path.read_bytes()
        inputs[str(path)]=hashlib.sha256(data).hexdigest()
        out.write(struct.pack('<QQ',resource,len(data)));out.write(data)
    out.write(struct.pack('<I',len(runs)))
    for run in runs:
        out.write(struct.pack('<I',len(run)))
        for r in run:
            a=r['address']
            out.write(struct.pack('<QQQII',a[2],1,a[3],a[4],4 if a[5]=='UInt32' else 2))
receipt={'kind':'CPU-only captured ordered topology fixture; GPU unexecuted',
         'source_sha256':inputs,'runs':len(runs),'draws':sum(map(len,runs)),
         'fixture_sha256':hashlib.sha256((HERE/'index-fixture.bin').read_bytes()).hexdigest(),
         'fixture_bytes':(HERE/'index-fixture.bin').stat().st_size}
(HERE/'index-fixture.json').write_text(json.dumps(receipt,indent=2)+'\n')
print(json.dumps({k:v for k,v in receipt.items() if k!='source_sha256'}))
