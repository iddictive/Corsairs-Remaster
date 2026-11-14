"""Fade the face correction towards side-facing surface normals."""
from pathlib import Path
here=Path(__file__).resolve().parent
path=here/'cgmodelsnet_face_reprojection.py';source=path.read_text()
needle='fade=np.clip((1-((sc[...,0]-255)/85)**2-((sc[...,1]-222)/95)**2)*3,0,1)'
assert source.count(needle)==1
source=source.replace(needle,needle+'\n    fade*=np.clip((normal[...,2]-.25)/.55,0,1)')
assert source.count('cgmodelsnet-face-refined-atlas.png')==2
source=source.replace('cgmodelsnet-face-refined-atlas.png','cgmodelsnet-face-soft-atlas.png')
assert source.count("f'cgmodelsnet-face-refined-{label}.png'")==1
source=source.replace("f'cgmodelsnet-face-refined-{label}.png'","f'cgmodelsnet-face-soft-{label}.png'")
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
