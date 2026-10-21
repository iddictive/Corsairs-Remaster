"""Private authored-ambient correction; no canonical source or sky producer edit."""
from pathlib import Path
import hashlib
import json
import difflib

ROOT = Path(__file__).resolve().parents[3]
NATIVE = ROOT / 'experiments/native-metal'
OUT = Path(__file__).resolve().parent / 'ambient-candidate'
backend = (NATIVE / 'backend.mm').read_text()
shadow = (NATIVE / 'land_shadow.hpp').read_text()
assert hashlib.sha256(backend.encode()).hexdigest() == '4397960abc0e8e6fb6091bd8172da0c667d66959fc2a943f46274506d893c3c9'
changes = [
 ('materialDiffuse,materialAmbient,materialEmissive,materialSpecular,sceneAmbient,cameraPosition',
  'materialDiffuse,materialAmbient,materialEmissive,materialSpecular,materialSkyAmbient,sceneAmbient,cameraPosition', 2),
 ('raw.materialAmbient=rawOutdoor?materialDiffuse:materialAmbient;',
  'raw.materialAmbient=rawOutdoor?materialDiffuse:materialAmbient;raw.materialSkyAmbient=raw.materialAmbient;', 1),
 ('rawfb.materialAmbient=materialAmbient;',
  'rawfb.materialAmbient=materialAmbient;rawfb.materialSkyAmbient=rawfb.materialAmbient;', 1),
 ('+diffuse.rgb*skyIrradiance(normal,r.skyLight)',
  '+(r.ambientFromColor?authored.rgb:r.materialSkyAmbient.rgb)*skyIrradiance(normal,r.skyLight)', 2),
 ('if(authoredOutdoorAlbedo)ambient=diffuse;if(outdoorLighting)',
  'if(authoredOutdoorAlbedo)ambient=diffuse;const auto skyAmbient=ambient;if(outdoorLighting)', 1),
 ('+diffuse.xyz*storm_metal::skyIrradiance(normal,skyLight.data())',
  '+skyAmbient.xyz*storm_metal::skyIrradiance(normal,skyLight.data())', 1),
]
result = backend
for old, new, count in changes:
 assert result.count(old) == count, (old, result.count(old))
 result = result.replace(old, new)
old = '+diffuse.rgb*skyIrradiance(normal,r.skyLight)'
new = '+ambient.rgb*skyIrradiance(normal,r.skyLight)'
assert shadow.count(old) == 4
shadow_result = shadow.replace(old, new)
OUT.mkdir(exist_ok=True)
receipt = {'scope': 'Resolved authored ambient multiplies lit SH; original coefficient survives legacy ambient floor. Direct, unlit, sky, exposure and textures unchanged.',
           'canonical_source_sha256': {}, 'candidate_sha256': {}}
for name, original, candidate in [('backend.mm', backend, result), ('land_shadow.hpp', shadow, shadow_result)]:
 (OUT / name).write_text(candidate)
 (OUT / (name+'.patch')).write_text(''.join(difflib.unified_diff(original.splitlines(True), candidate.splitlines(True), fromfile='canonical/'+name, tofile='candidate/'+name)))
 receipt['canonical_source_sha256'][name] = hashlib.sha256(original.encode()).hexdigest()
 receipt['candidate_sha256'][name] = hashlib.sha256(candidate.encode()).hexdigest()
(OUT / 'prepare-receipt.json').write_text(json.dumps(receipt, indent=2)+'\n')
print(json.dumps(receipt, indent=2))
