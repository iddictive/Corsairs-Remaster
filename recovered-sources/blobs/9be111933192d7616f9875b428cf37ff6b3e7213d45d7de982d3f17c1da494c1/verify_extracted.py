#!/usr/bin/env python3
"""One-off full payload, symlink and native xattr comparison after ditto extraction."""
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import stat
import sys
import unicodedata
import zipfile
import zlib

lib = C.CDLL(None, use_errno=True)
listx = lib.listxattr; listx.restype = C.c_ssize_t
listx.argtypes = [C.c_char_p, C.c_void_p, C.c_size_t, C.c_int]
getx = lib.getxattr; getx.restype = C.c_ssize_t
getx.argtypes = [C.c_char_p, C.c_char_p, C.c_void_p, C.c_size_t, C.c_uint32, C.c_int]

def xattrs(path):
    rawpath = os.fsencode(path); size = listx(rawpath, None, 0, 1)
    if size < 0: raise OSError(C.get_errno(), f'listxattr: {path}')
    buf = C.create_string_buffer(size)
    if size: assert listx(rawpath, buf, size, 1) == size
    result = {}
    for name in buf.raw.split(b'\0'):
        if not name: continue
        n = getx(rawpath, name, None, 0, 0, 1)
        if n < 0: raise OSError(C.get_errno(), f'getxattr: {path}')
        value = C.create_string_buffer(n)
        assert getx(rawpath, name, value, n, 0, 1) == n
        result[name.decode()] = value.raw.hex()
    return result

def verify(archive, inventory, payload, extracted, receipt):
    expected = json.loads(Path(inventory).read_text())['entries']
    normalized = lambda s: unicodedata.normalize('NFC', s)
    with zipfile.ZipFile(archive) as z:
        members = z.infolist()
        by_name = {normalized(i.filename): i for i in members}
        assert len(by_name) == len(expected) == len(members), 'Missing/duplicate member'
        for e in expected:
            i = by_name[e['name']]
            assert (i.file_size, i.CRC) == (e['size'], e['crc']), e['name']
            assert i.external_attr >> 16 == e['mode'], ('mode', e['name'])
            assert not any(p.lower() in {'save','userdata','logs','.git','.codex','profiles','case-inputs'} for p in Path(i.filename).parts), i.filename
        with Path(archive).open('rb') as f:
            f.seek(-min(65557, Path(archive).stat().st_size), 2); tail = f.read()
        assert b'PK\x06\x06' in tail and b'PK\x06\x07' in tail, 'Missing ZIP64 EOCD/locator'
    files = directories = links = attrs = 0
    for e in expected:
        if e['name'].startswith('__MACOSX/'): continue
        rel = e['name'].rstrip('/')
        source, target = Path(payload)/rel, Path(extracted)/rel
        assert source.lstat().st_mode == target.lstat().st_mode, ('filesystem mode', rel)
        if source.is_symlink():
            assert target.is_symlink() and os.readlink(source) == os.readlink(target), ('symlink', rel)
            links += 1
        elif source.is_file():
            crc, n = 0, 0
            with target.open('rb') as f:
                for b in iter(lambda: f.read(1024*1024), b''):
                    crc = zlib.crc32(b, crc); n += len(b)
            assert (n, crc) == (e['size'], e['crc']), ('extracted bytes', rel)
            files += 1
        else: directories += 1
        a, b = xattrs(source), xattrs(target)
        assert a == b, ('xattrs', rel, a, b)
        attrs += len(a)
    h, m = hashlib.sha256(), hashlib.md5()
    with Path(archive).open('rb') as f:
        for b in iter(lambda: f.read(4*1024*1024), b''): h.update(b); m.update(b)
    result = {'archive': str(Path(archive).resolve()), 'bytes': Path(archive).stat().st_size,
              'sha256': h.hexdigest(), 'md5': m.hexdigest(), 'members': len(expected),
              'files': files, 'directories': directories, 'symlinks': links,
              'xattrs': attrs, 'zip64': True, 'payload_crc_modes_links_xattrs': 'pass'}
    Path(receipt).write_text(json.dumps(result, indent=2)); print(json.dumps(result, indent=2))

if __name__ == '__main__': verify(*sys.argv[1:])
