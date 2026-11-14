"""Bake the scarf in continuous cylindrical coordinates, not per-face coordinates."""
from pathlib import Path

here = Path(__file__).resolve().parent
path = here/'cgmodelsnet_body_scarf_bake.py'
source = path.read_text()
old = "select=np.all(faces<2106,axis=1)&(center[:,1]>1.55)&(center[:,1]<1.62)&(abs(center[:,0]-.01)<.1)&(center[:,2]>-.1)&(center[:,2]<.16)&(color[:,0]>color[:,2]*1.3)&(color[:,0]>55)"
new = """field=p[:,1]+.25*(p[:,2]-.02)-.30*abs(p[:,0]-.01)-1.595
select=np.all(faces<2106,axis=1)&(field[faces].max(1)>-.055)&(center[:,1]>1.46)&(abs(center[:,0]-.01)<.12)&(abs(center[:,2])<.16)"""
assert source.count(old)==1
source=source.replace(old,new)
old='t=np.clip((height+.020)/.012,0,1)'
assert source.count(old)==1
source=source.replace(old,'t=np.clip((height+.055)/.025,0,1)')
old='pattern=fabric[(yy//2)%20,(xx//2)%70]'
new="""world=weights@p[f]
    theta=np.mod(np.arctan2(world[...,2],world[...,0]-.01)+np.pi,2*np.pi)/(2*np.pi)
    mirror=np.mod(theta*8*69,2*69)
    fx=np.minimum(mirror,2*69-mirror)
    fy=np.clip((height+.055)*19/.055,0,19)
    xlo=np.floor(fx).astype(int);xhi=np.minimum(xlo+1,69);ylo=np.floor(fy).astype(int);yhi=np.minimum(ylo+1,19)
    dx=(fx-xlo)[...,None];dy=(fy-ylo)[...,None]
    pattern=(1-dy)*((1-dx)*fabric[ylo,xlo]+dx*fabric[ylo,xhi])+dy*((1-dx)*fabric[yhi,xlo]+dx*fabric[yhi,xhi])"""
assert source.count(old)==1
source=source.replace(old,new)
source=source.replace('cgmodelsnet-body-scarf-bake','cgmodelsnet-body-scarf-worlduv')
exec(compile(source,str(path),'exec'),{'__file__':str(path),'__name__':'__main__'})
