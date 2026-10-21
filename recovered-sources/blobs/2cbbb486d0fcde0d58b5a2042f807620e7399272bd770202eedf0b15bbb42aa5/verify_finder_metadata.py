import errno
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile
from unittest.mock import patch
import zipfile

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from rewrite_metal_zip64 import rewrite
from verify_metal_archive import xattrs
import metal_storage

work = Path(tempfile.mkdtemp(prefix='corsairs-package-metadata-')).resolve()
(ROOT / '.local-archives/reconstruction-20261016/finder-metadata-scratch.json').write_text(json.dumps({'scratch': str(work)}))
payload = work / 'payload'
payload.mkdir()
(payload / 'file').write_bytes(b'game payload\n' * 100)
(payload / 'file').chmod(0o755)
subprocess.run(['/usr/bin/xattr', '-w', 'com.apple.test', 'preserved metadata', str(payload / 'file')], check=True)
(payload / 'link').symlink_to('file')
(payload / '.DS_Store').write_bytes(b'volatile directory state')
native, public, inventory = [work / n for n in ('native.zip', 'public.zip', 'inventory.json')]
subprocess.run(['/usr/bin/ditto', '-c', '-k', '--sequesterRsrc', str(payload), str(native)], check=True)
(payload / '.DS_Store').unlink()
rewrite(native, public, inventory, payload)
extracted = work / 'extracted'
subprocess.run(['/usr/bin/ditto', '-x', '-k', str(public), str(extracted)], check=True)
with zipfile.ZipFile(public) as z:
    assert not any('.DS_Store' in i.filename for i in z.infolist())
    assert z.testzip() is None
assert (extracted / 'file').read_bytes() == (payload / 'file').read_bytes()
assert (extracted / 'file').stat().st_mode == (payload / 'file').stat().st_mode
assert os.readlink(extracted / 'link') == 'file'
assert xattrs(extracted / 'file') == xattrs(payload / 'file')

# Preserve the root identity, as Finder does when recreating a directory file.
def late_file(path, name):
    for child in path.iterdir():
        child.unlink()
    (path / name).write_text('new state')
    raise OSError(errno.ENOTEMPTY, 'injected final-rmdir race')

owned = work / 'owned'
owned.mkdir()
with patch.object(metal_storage.shutil, 'rmtree', side_effect=lambda p: late_file(p, '.DS_Store')):
    metal_storage._remove_owned(owned, metal_storage._identity(owned))
assert not owned.exists()
owned.mkdir()
with patch.object(metal_storage.shutil, 'rmtree', side_effect=lambda p: late_file(p, 'unknown.txt')):
    try:
        metal_storage._remove_owned(owned, metal_storage._identity(owned))
    except OSError:
        pass
    else:
        raise AssertionError('Unknown newly created file must be preserved')
assert (owned / 'unknown.txt').read_text() == 'new state'
result = {'status': 'passed', 'scratch': str(work), 'finder_metadata_excluded': True,
          'removed_source_metadata_allowed': True, 'native_bytes_modes_links_xattrs': 'pass',
          'late_finder_cleanup': 'pass', 'unknown_new_file_preserved': True}
(ROOT / '.local-archives/reconstruction-20261016/finder-metadata-result.json').write_text(json.dumps(result, indent=2) + '\n')
print(json.dumps(result))
