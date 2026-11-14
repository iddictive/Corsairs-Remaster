"""Attach a uniformly colored stock musket mesh for offline pose diagnostics.

Replays blade.cpp's local-X reflection and GM gun locator, then skins the
combined mesh through the same active musket bind. This is not a game asset.
"""
import hashlib
import struct
import sys
from pathlib import Path
import numpy as np
from PIL import Image
here=Path(__file__).resolve().parent;root=here.parents[2];sys.path.insert(0,str(root/'tools'))
from patch_metal_portobello_ground import parse_gm,HEADER,LABEL,_name

base=(here/'putin-mush-locator-probe.gm').read_bytes();h=list(HEADER.unpack_from(base))
strings=base[72:72+h[2]];labeloff=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]
labels={}
for i in range(h[7]):
    row=LABEL.unpack_from(base,labeloff+i*LABEL.size);labels[_name(strings,row[1]).lower()]=row
label=labels['gun_hand'];assert label[19]==65
matrix=np.asarray(label[3:19],dtype=float).reshape(4,4)
obj=labeloff+108*h[7];tri=obj+104*h[8];off=tri+6*h[9]
faces=np.frombuffer(base,'<u2',h[9]*3,tri).reshape(-1,3).copy();kind,size=struct.unpack_from('<2i',base,off);assert kind==4
start=off+8;rows=[base[start+i*44:start+(i+1)*44] for i in range(size//44)];old_vertices=len(rows)
weapon=here/'mushket-source.gm';weapon_bytes=weapon.read_bytes()
assert hashlib.sha256(weapon_bytes).hexdigest()=='968f0cd0a07430d0409f613e25965748720fd1bf1768f8ab6f8c3156e6e48c29'
g=parse_gm(weapon_bytes);newfaces=[]
for item in g['objects']:
    buf=g['buffers'][item[7]];first=len(rows)
    for i in range(item[10]):
        offset=buf['payload_offset']+(item[11]+i)*buf['stride'];raw=struct.unpack_from('<3f3fI',g['data'],offset)
        # In blade.cpp scl negates the weapon's local X. Global reflected X
        # is undone here because the portrait CPU helper also undoes it.
        local=np.array([-raw[0],raw[1],raw[2],1.]);p=local@matrix
        n=np.array([-raw[3],raw[4],raw[5]])@matrix[:3,:3];n/=max(np.linalg.norm(n),1e-12)
        rows.append(struct.pack('<3ffI3fI2f',*p[:3],1.,65|(65<<8),*n,0xffffffff,1568/2048,224/2048))
    for face in g['triangles'][item[9]:item[9]+item[8]]:
        assert max(face)<item[10];newfaces.append(tuple(first+v for v in face))
assert len(rows)<65536 and len(newfaces)==1880
outfaces=np.vstack((faces,np.asarray(newfaces,dtype='<u2'))).astype('<u2')
prefix=bytearray(base[:tri]);h[9]=len(outfaces);HEADER.pack_into(prefix,0,*h)
fields=list(struct.unpack_from('<3i4f6i8i4ii',prefix,obj));fields[8]=len(outfaces);fields[10]=len(rows);struct.pack_into('<3i4f6i8i4ii',prefix,obj,*fields)
(here/'musket-overlay-diagnostic.gm').write_bytes(prefix+outfaces.tobytes()+struct.pack('<2i',4,len(rows)*44)+b''.join(rows)+base[start+size:])
# Uniform color makes the extra mesh inspectable and does not pretend to render
# its real material; region is unused by candidate vertices.
dt=np.dtype({'names':['uv'],'formats':[('<f4',2)],'offsets':[36],'itemsize':44});uv=np.frombuffer(base,dt,size//44,start)['uv']%1
assert not np.any((uv[:,0]>=1536/2048)&(uv[:,0]<1600/2048)&(uv[:,1]>=192/2048)&(uv[:,1]<256/2048))
atlas=np.array(Image.open(here/'cgmodelsnet-stockuv-scarf-atlas.png').convert('RGB'));atlas[192:256,1536:1600]=[132,96,53]
Image.fromarray(atlas).save(here/'musket-overlay-diagnostic-atlas.png')
print('diagnostic musket vertices',len(rows)-old_vertices,'triangles',len(newfaces),'source',weapon)
