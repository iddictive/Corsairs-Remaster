"""Compare narrower rear scarf masks on the actual current GM and posed copies."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_rear_collar_trial.py'
for label,top in [('20','.020'),('30','.030')]:
    source=path.read_text()
    changes=[('cgmodelsnet-neck-retopology-fit.gm','cgmodelsnet-stockuv-retopology.gm'),
        ('cgmodelsnet-nose-baseline-atlas.png','cgmodelsnet-stockuv-scarf-atlas.png'),
        ("field.min()>.052","field.min()>"+str(float(top)+.002)),
        ('(.050-f)/.015',f'({top}-f)/.010'),
        ("cgmodelsnet-rear-collar-",f"cgmodelsnet-rear-collar-narrow-{label}-"),
        ('animated-cgmodelsnet-neck-retopology-fit-8423.gm','animated-cgmodelsnet-decimated-8423.gm'),
        ("('1985','animated-cgmodelsnet-neck-retopology-fit-1985.gm'", "('5320','animated-cgmodelsnet-decimated-5320.gm'"),
        ('animated-cgmodelsnet-neck-retopology-fit-run.gm','animated-cgmodelsnet-decimated-run.gm')]
    for old,new in changes:
        assert source.count(old)>=1,(old,source.count(old));source=source.replace(old,new)
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
