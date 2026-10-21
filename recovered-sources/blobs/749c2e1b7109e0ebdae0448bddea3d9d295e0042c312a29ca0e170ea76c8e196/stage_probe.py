#!/usr/bin/env python3
"""Exercise relocated public launcher staging only, with prior state and stale-owner negative."""
from pathlib import Path
import hashlib
import json
import os
import shutil
import subprocess
import sys

def probe(work, frozen_app, extracted_app):
    work, frozen_app, extracted_app = map(lambda p: Path(p).resolve(), (work, frozen_app, extracted_app))
    profile = work/'isolated-prior-profile'; profile.mkdir()
    before = {}
    for folder in ('SAVE', 'userdata', 'logs'):
        (profile/folder).mkdir()
        canary = profile/folder/'archive-preservation-canary'
        canary.write_bytes(b'Prior user state: retain these exact bytes.\n')
        before[str(canary)] = hashlib.sha256(canary.read_bytes()).hexdigest()
    old_resources = frozen_app/'Contents/Resources'
    for name in ('PROGRAM', 'RESOURCE'): (profile/name).symlink_to(old_resources/name)
    for name in ('engine.ini','options','project.df'): shutil.copy2(old_resources/name,profile/name)
    result = subprocess.run([str(extracted_app/'Contents/MacOS/launch'),'--stage-only','--user-dir',str(profile)],capture_output=True,text=True)
    assert result.returncode == 0,(result.stdout,result.stderr)
    for path, checksum in before.items(): assert hashlib.sha256(Path(path).read_bytes()).hexdigest() == checksum
    for name in ('PROGRAM','RESOURCE'): assert (profile/name).resolve() == extracted_app/'Contents/Resources'/name
    assert not (profile/'logs/launch.log').exists(),'Engine launch log unexpectedly created'
    negative = work/'stale-owner-profile'; negative.mkdir(); (negative/'PROGRAM').mkdir()
    rejection = subprocess.run([str(extracted_app/'Contents/MacOS/launch'),'--stage-only','--user-dir',str(negative)],capture_output=True,text=True)
    assert rejection.returncode != 0 and 'Refusing unknown stale owner' in rejection.stderr,(rejection.returncode,rejection.stdout,rejection.stderr)
    receipt = {'relocated_launcher':str(extracted_app/'Contents/MacOS/launch'),
               'stage_only_exit':result.returncode,'stage_stdout':result.stdout,
               'prior_save_userdata_logs':'preserved','relocated_resource_links':'pass',
               'stale_non_symlink_owner':'rejected','negative_exit':rejection.returncode,
               'negative_stderr':rejection.stderr,'engine_launched':False}
    (work/'stage-receipt.json').write_text(json.dumps(receipt,indent=2));print(json.dumps(receipt,indent=2))

if __name__ == '__main__': probe(*sys.argv[1:])
