"""Render the same diagnostic after the trial's +0.014 vertical shift."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'sculpture_registration_probe.py';source=path.read_text()
for old,new in [('.010+points[:,0]*.0025,1.385+points[:,2]*.0028', '.010+points[:,0]*.0025,1.399+points[:,2]*.0028'),
                ('sculpture-registered-', 'sculpture-corrected-registration-'),
                ('stock-geometry-', 'stock-corrected-registration-')]:
    assert source.count(old)==1,old
    source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
