"""Create a separate delivery-shaped trial without changing registered assets."""
import hashlib
import json
import shutil
from pathlib import Path
here=Path(__file__).resolve().parent;root=here.parents[2]
old=root/'src/assets/characters/putin-phonk-candidate'
dest=root/'src/assets/characters/putin-phonk-recut-candidate'
assert not dest.exists(),f'Refusing to overwrite existing {dest}'
model=(here/'cgmodelsnet-recut-winding-trial.gm').read_bytes()
assert hashlib.sha256(model).hexdigest()=='4cc955a1e9b91ed811aac69192f14fabad50a9ad7f42ac1fd2e05dba656f4359'
dest.mkdir()
(dest/'model-source.gm').write_bytes(model)
for name in ['atlas-source.png','musket-locator-source.json']:
    shutil.copyfile(old/name,dest/name)
source=(old/'prepare.py').read_text()
changes={'667f956c2e0f20e3270b8aaca2f5cf85a0b09b7bc211a10c8cf5fbd63c04e3aa':'4cc955a1e9b91ed811aac69192f14fabad50a9ad7f42ac1fd2e05dba656f4359',
'59f2d10900c0352f8f03f2d56b679208b816c10a1bdd3ddba11cfe66a6c72cd6':'91ebfa4ede221009948c492c70f26786ea346cc167f856033ae36ceb07547f47',
'6a911be56a356c0dc75583162e67e46153a65a5b87661557f4f9272e5dc67d5e':'0fa26be494fa0d62054c61e93b122ddef5d9828f3eb25c344dea958ade2a4f12'}
for a,b in changes.items():
    assert source.count(a)==1;source=source.replace(a,b)
(dest/'prepare.py').write_text(source)
manifest=json.loads((old/'manifest.json').read_text())
for item in manifest['files'].values():
    for key in ['source_sha256','sha256']:
        if item[key] in changes:item[key]=changes[item[key]]
    item['consumer']=item['consumer'].replace('Unregistered','Unregistered recut trial')
(dest/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Prepared independent source set:',dest)
