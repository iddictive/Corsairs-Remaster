"""Test a wider scarf transition over the original body surface."""
from pathlib import Path

here = Path(__file__).resolve().parent
path = here / 'cgmodelsnet_body_scarf_bake.py'
source = path.read_text()
old = "select=np.all(faces<2106,axis=1)&(center[:,1]>1.55)&(center[:,1]<1.62)&(abs(center[:,0]-.01)<.1)&(center[:,2]>-.1)&(center[:,2]<.16)&(color[:,0]>color[:,2]*1.3)&(color[:,0]>55)"
new = """field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
select=np.all(faces<2106,axis=1)&(field[faces].max(1)>-.055)&(center[:,1]>1.46)&(abs(center[:,0]-.01)<.12)&(abs(center[:,2])<.16)"""
assert source.count(old) == 1
source = source.replace(old, new)
old = 't=np.clip((height+.020)/.012,0,1)'
assert source.count(old) == 1
source = source.replace(old, 't=np.clip((height+.055)/.025,0,1)')
source = source.replace('cgmodelsnet-body-scarf-bake', 'cgmodelsnet-body-scarf-deep')
exec(compile(source, str(path), 'exec'), {'__file__': str(path), '__name__': '__main__'})
