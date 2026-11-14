"""Make an unregistered musket GM with stock-style equipment locators."""
import hashlib
import json
import sys
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
sys.path.insert(0,str(root/'tools'))
from patch_metal_portobello_ground import HEADER,LABEL,_name
from prepare_roundhouse import animated_mesh

def label_rows(data):
    h=HEADER.unpack_from(data);strings=data[HEADER.size:HEADER.size+h[2]]
    offset=HEADER.size+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]
    result={}
    for i in range(h[7]):
        row=LABEL.unpack_from(data,offset+i*LABEL.size);name=_name(strings,row[1]).lower()
        assert name not in result
        result[name]=(offset+i*LABEL.size,row)
    return result

source_path=here/'musket-stock-locator-source.json'
expected_stock='cce7d269da5ad1189bd41aa6d0729fc5c5ada24469d8de82623b5f92fb73b7b1'
if not source_path.exists():
    stock=root/'experiments/native-metal/.cache/runtime/RESOURCE/MODELS/Characters/Blaze_8_Mush.gm'
    data=stock.read_bytes();assert hashlib.sha256(data).hexdigest()==expected_stock
    src=label_rows(data);assert src['gun_hand'][1][3:]==src['gun_belt'][1][3:]
    source_path.write_text(json.dumps({'stock_model':'Blaze_8_Mush.gm','sha256':expected_stock,'gun_locator_payload':src['gun_hand'][1][3:]},indent=2)+'\n')
provenance=json.loads(source_path.read_text());assert provenance['sha256']==expected_stock
gun=provenance['gun_locator_payload'];assert len(gun)==24 and gun[16]==65
base=(here/'putin-stockuv-retopology-probe.gm').read_bytes();assert hashlib.sha256(base).hexdigest()=='59f2d10900c0352f8f03f2d56b679208b816c10a1bdd3ddba11cfe66a6c72cd6'
data=bytearray(base);rows=label_rows(base);belt=rows['saber_belt'][1]
for name,payload in [('gun_hand',gun),('gun_belt',gun),('saber_hand',belt[3:])]:
    offset,old=rows[name];LABEL.pack_into(data,offset,*(old[:3]+tuple(payload)))
after=label_rows(data)
assert after['gun_hand'][1][3:]==after['gun_belt'][1][3:]==tuple(gun)
assert after['saber_hand'][1][3:]==after['saber_belt'][1][3:]
assert animated_mesh(base)==animated_mesh(data)
assert data.count(b'Putin.tga\0')==1
out=here/'putin-mush-locator-probe.gm';out.write_bytes(data)
print('musket locator GM',hashlib.sha256(data).hexdigest(),'skin and topology unchanged; gun bone',after['gun_hand'][1][19],'saber bone',after['saber_hand'][1][19])
