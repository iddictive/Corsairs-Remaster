"""Experimental side texture projection; preserves skinning, not acceptance."""
import sys
from pathlib import Path

here = Path(__file__).resolve().parent
path = here / 'separate_face_uv.py'
source = path.read_text()
changes = [
    ("assert np.all((uv[:,0]>=0)&(uv[:,0]<=1)&(uv[:,1]>=-1)&(uv[:,1]<=0))", "assert np.isfinite(uv).all()"),
    ('angle=-.15; c,s=', 'angle=1.; c,s='),
    ("for i,v in enumerate(raw):struct.pack_into('<2f',v,36,uv[i,0]*.5,(uv[i,1]+1)*.5)", '# retain existing UVs'),
    ('if not (np.all(tex[:,0]>.64) and np.all((tex[:,1]+1>.33)&(tex[:,1]+1<.72))):continue', 'if not (1.60 < p[f,1].mean() < 1.73):continue'),
    ('if not (185<center[0]<325 and 145<center[1]<325 and center[2]>.045):continue', 'if not (235<center[0]<415 and 175<center[1]<340 and center[2]>.015):continue'),
    ('if np.any(normals@np.array([-s,0,c])<.55):continue', 'if np.mean(normals@np.array([-s,0,c]))<.50 or np.mean(normals[:,2])>.65:continue'),
    ("struct.pack_into('<2f',v,36,.5+x/2048,y/2048)", "struct.pack_into('<2f',v,36,.5+x/2048,.25+y/2048)"),
    ("atlas=Image.new('RGB',(2048,2048))", "atlas=Image.open(here/'identity-geometry-face-atlas.png').convert('RGB')"),
    ("base=Image.open(here/'officer_5-imagegen.png').convert('RGB').resize((1024,1024),Image.Resampling.LANCZOS)", "base=atlas.copy()"),
    ('((u%1)*1024)', '((u%1)*2048)'),
    ('((t%1)*1024)', '((t%1)*2048)'),
    ('0,1023)', '0,2047)'),
    ("atlas.paste(base,(0,0)); atlas.paste(Image.fromarray(face),(1024,0))", "atlas.paste(Image.fromarray(face),(1024,512))"),
    ('separate-face-prototype.gm', 'profile-projection-prototype.gm'),
    ('separate-face-atlas.png', 'profile-projection-atlas.png'),
]
for old, new in changes:
    expected = 2 if old == '0,1023)' else 1
    assert source.count(old) == expected, (old, source.count(old))
    source = source.replace(old, new)
sys.argv = [str(path), 'profile-refinement-target.png', str(here/'identity-geometry-face-prototype.gm'), 'identity-geometry-face-profile.png']
exec(compile(source, str(path), 'exec'), {'__file__': str(path), '__name__': '__main__'})
