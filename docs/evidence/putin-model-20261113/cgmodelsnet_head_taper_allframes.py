"""Apply the exhaustive scan to the lower-head-weight experiment."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_neck_allframes.py';source=path.read_text()
for old,new in [("root/'src/assets/characters/putin-phonk-candidate/Putin.gm'","here/'cgmodelsnet-head-weight-taper-trial.gm'"),('59f2d10900c0352f8f03f2d56b679208b816c10a1bdd3ddba11cfe66a6c72cd6','91508b2c56546acfaf052b170cfbe391e7bdecd6820a3c22eae78718ff92d99a'),('cgmodelsnet-neck-allframes-','cgmodelsnet-head-taper-allframes-')]:
    assert source.count(old)==1;source=source.replace(old,new)
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
