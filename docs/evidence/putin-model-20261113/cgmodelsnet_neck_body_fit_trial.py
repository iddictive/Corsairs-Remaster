"""Widen body transition and fit two bones against the original motion."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_body_follow_trial.py';source=path.read_text()
source=source.replace('import numpy as np','import numpy as np\nfrom fit_skin_trial import SkinFit')
source=source.replace('-.060','-.120').replace('+.060','+.120').replace('/.060','/.120')
source=source.replace('for i in ids:\n','fitter=SkinFit();cache={}\nfor i in ids:\n')
old='''        # Preserve the larger original influence within the two-weight format.
        a=a if w>=.5 else b;b=16;w=1-t
        v['b'][i]=(int(v['b'][i])&0xffff0000)|a|(b<<8)'''
new='''        row=struct.unpack_from('<3ffI3fI2f',data,off+8+int(i)*44)
        target=list(row);target[3]=1.;target[4]=16|(16<<8)
        key=(tuple(row[:3]),row[3],row[4],round(float(t),10))
        if key not in cache:cache[key]=fitter.fit(row,target,float(t),p[i],((a,b),w))
        (a,b),w=cache[key]
        v['b'][i]=(int(v['b'][i])&0xffff0000)|a|(b<<8)'''
assert source.count(old)==1;source=source.replace(old,new)
source=source.replace("print('body/bridge rows changed'","fitter.report()\nprint('body/bridge rows changed'")
source=source.replace('cgmodelsnet-neck-body-follow-trial.gm','cgmodelsnet-neck-body-fit-trial.gm')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
