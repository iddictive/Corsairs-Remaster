"""Compare raising the same posterior contour and render against the control."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_rear_lower_trial.py';source=path.read_text()
assert source.count('result[:,1]-=amount*fade')==1
source=source.replace('result[:,1]-=amount*fade','result[:,1]+=amount*fade')
source=source.replace('cgmodelsnet-rear-lower-','cgmodelsnet-rear-raise-')
source=source.replace("for strength in ['3','6']:","for strength in ['3']:")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
path=here/'cgmodelsnet_rear_lower_profiles.py';source=path.read_text()
source=source.replace('cgmodelsnet-rear-lower-', 'cgmodelsnet-rear-raise-')
source=source.replace(",('6','animated-cgmodelsnet-rear-raise-6-8423.gm')",'')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
