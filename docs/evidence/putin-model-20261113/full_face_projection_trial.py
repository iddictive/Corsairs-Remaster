"""Project the whole central face including detached eye/nose/mouth triangles."""
import sys
from pathlib import Path

here=Path(__file__).resolve().parent
def run(name,args,replacements):
    path=here/name
    source=path.read_text()
    for old,new in replacements:
        assert source.count(old)==1,old
        source=source.replace(old,new)
    sys.argv=[str(path),*map(str,args)]
    exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})

old='if not (np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))):continue'
new='''skin = np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))
    central = 200 < center[0] < 310 and 145 < center[1] < 320
    if not (skin or central):continue'''
run('separate_face_uv.py',['aligned-width-target.png',here/'no-earring-prototype.gm','width-morph-baseline.png'],[
    (old,new),
    ('if np.any(normals@np.array([-s,0,c])<.55):continue','if not central and np.any(normals@np.array([-s,0,c])<.55):continue'),
    ("blur=np.asarray(Image.fromarray(visible).filter(ImageFilter.GaussianBlur(6)),dtype=float)/255","Image.fromarray(visible).save(here/'full-face-coverage.png')\nblur=np.asarray(Image.fromarray(visible).filter(ImageFilter.GaussianBlur(6)),dtype=float)/255"),
    ('separate-face-prototype.gm','full-face-prototype.gm'),
    ('separate-face-atlas.png','full-face-atlas.png'),
])
for angle,output in [(-.15,'full-face-front.png'),(1,'full-face-profile.png'),(-1,'full-face-other-profile.png')]:
    run('render_officer.py',['officer_5',here/'full-face-atlas.png',angle,here/'full-face-prototype.gm'],[
        ("f'{prefix}-candidate-preview.png'",repr(output)),
        ("f'{prefix}-candidate-angle-{angle:g}.png'",repr(output)),
    ])
