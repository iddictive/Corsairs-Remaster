"""Use the retained exhaustive bridge scan on the rebuilt diagnostic GM."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_allframes.py';source=path.read_text()
for old,new in [("(root/'src/assets/characters/putin-phonk-candidate/Putin.gm').read_bytes()","(here/'cgmodelsnet-recut-trial.gm').read_bytes()"),('59f2d10900c0352f8f03f2d56b679208b816c10a1bdd3ddba11cfe66a6c72cd6','99e823418f9ab542725dc349fdc54ffcabb4fc7644e45d2beccb25d82616bd37'),('[-694:]','[-344:]'),('(indices[edges]>=15016)&(indices[edges]<15623)','(indices[edges]>=16012)&(indices[edges]!=16270)'),('len(edges)==695','len(edges)==346'),('cgmodelsnet-neck-allframes-','cgmodelsnet-recut-allframes-')]:
    assert source.count(old)==1,(old,source.count(old));source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
