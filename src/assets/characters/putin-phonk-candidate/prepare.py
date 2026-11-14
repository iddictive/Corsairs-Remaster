#!/usr/bin/env python3
"""Reproduce the isolated, unregistered candidate; never touches the game."""
import argparse
import hashlib
import io
import json
import struct
import sys
from pathlib import Path
from PIL import Image

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[3]
sys.path.insert(0,str(ROOT/'tools'))
from patch_metal_portobello_ground import HEADER,LABEL,_name

def sha(data):return hashlib.sha256(data).hexdigest()
def labels(data):
    h=HEADER.unpack_from(data);strings=data[72:72+h[2]]
    offset=72+h[2]+4*h[3]+4*h[4]+56*h[5]+80*h[6]
    result={}
    for i in range(h[7]):
        row=LABEL.unpack_from(data,offset+i*LABEL.size);name=_name(strings,row[1]).lower()
        assert name not in result;result[name]=(offset+i*LABEL.size,row)
    return result

def main():
    parser=argparse.ArgumentParser();parser.add_argument('--write',action='store_true',help='rewrite only files in this candidate directory')
    args=parser.parse_args()
    model=(HERE/'model-source.gm').read_bytes();atlas=(HERE/'atlas-source.png').read_bytes();locators=(HERE/'musket-locator-source.json').read_bytes()
    assert sha(model)=='667f956c2e0f20e3270b8aaca2f5cf85a0b09b7bc211a10c8cf5fbd63c04e3aa'
    assert sha(atlas)=='890846414f7c0b83c5e3cfeb25b569e902bac6e9634dac7bf7fb832d137366f5'
    assert sha(locators)=='a1d02b3cbedeeb1e4acc3ba7c7450d6cef611dc12c02e567c53e976a899fe0cf'
    old,new=b'mr05x.tga\0',b'Putin.tga\0';assert len(old)==len(new) and model.count(old)==1
    normal=model.replace(old,new);assert normal.count(new)==1
    musket=bytearray(normal);all_labels=labels(normal);gun=json.loads(locators)['gun_locator_payload']
    assert len(gun)==24 and gun[16]==65
    for name,payload in [('gun_hand',gun),('gun_belt',gun),('saber_hand',all_labels['saber_belt'][1][3:])]:
        offset,row=all_labels[name];LABEL.pack_into(musket,offset,*(row[:3]+tuple(payload)))
    image=Image.open(io.BytesIO(atlas)).convert('RGBA');assert image.size==(2048,2048)
    chunks=[]
    for _ in range(6):
        output=io.BytesIO();image.save(output,format='DDS',pixel_format='DXT1');dds=output.getvalue();assert dds[84:88]==b'DXT1'
        chunk=dds[128:];assert len(chunk)==((image.width+3)//4)*((image.height+3)//4)*8
        chunks.append(chunk);image=image.resize((image.width//2,image.height//2),Image.Resampling.LANCZOS)
    texture=struct.pack('<6I',0,2048,2048,len(chunks),int.from_bytes(b'DXT1','little'),len(chunks[0]))+b''.join(chunks)
    expected={'Putin.gm':'59f2d10900c0352f8f03f2d56b679208b816c10a1bdd3ddba11cfe66a6c72cd6','Putin_mush.gm':'6a911be56a356c0dc75583162e67e46153a65a5b87661557f4f9272e5dc67d5e','Putin.tga.tx':'8bb3d9115c95011b3cef188e584c8e61cb88190a74c4cc2b1dd2315ff977a728'}
    for name,data in [('Putin.gm',normal),('Putin_mush.gm',bytes(musket)),('Putin.tga.tx',texture)]:
        assert sha(data)==expected[name],f'Generated hash mismatch: {name}'
        if args.write:(HERE/name).write_bytes(data)
        else:assert (HERE/name).read_bytes()==data,f'Prepared file differs: {name}'
        print(name,sha(data),len(data))

if __name__=='__main__':main()
