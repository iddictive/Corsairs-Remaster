"""Render worst declared man actions and the worst musket death frame."""
from pathlib import Path
here=Path(__file__).resolve().parent
for filename in ('cgmodelsnet_allframes_extremes.py','cgmodelsnet_allframes_fullbody.py'):
    path=here/filename;source=path.read_text()
    for old,new in [('7236','5320'),('7249','5229'),('animated-cgmodelsnet-allframes-','animated-cgmodelsnet-declared-'),('animated-stock-allframes-','animated-stock-declared-'),('cgmodelsnet-allframes-fullbody-','cgmodelsnet-declared-fullbody-'),('stock-allframes-fullbody-','stock-declared-fullbody-')]:
        source=source.replace(old,new)
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
