#!/usr/bin/env python3
"""Rewrite native ditto local ZIP records using automatic ZIP64, retaining AppleDouble."""
import ctypes as C
import hashlib
import json
import os
import stat
from pathlib import Path
import sys
import unicodedata
import zlib

lib = C.CDLL('/usr/lib/libarchive.2.dylib')
P, I, S, Z = C.c_void_p, C.c_int, C.c_char_p, C.c_size_t
def bind(name, result, *args):
    fn = getattr(lib, name); fn.restype = result; fn.argtypes = args
    return fn
new_r = bind('archive_read_new', P)
new_w = bind('archive_write_new', P)
error = bind('archive_error_string', S, P)
version = bind('archive_version_string', S)
support = bind('archive_read_support_format_zip_streamable', I, P)
option_r = bind('archive_read_set_format_option', I, P, S, S, S)
open_r = bind('archive_read_open_fd', I, P, I, Z)
next_header = bind('archive_read_next_header', I, P, C.POINTER(P))
read_data = bind('archive_read_data', C.c_ssize_t, P, P, Z)
name = bind('archive_entry_pathname_utf8', S, P)
size = bind('archive_entry_size', C.c_int64, P)
mode = bind('archive_entry_mode', C.c_uint, P)
symlink = bind('archive_entry_symlink_utf8', S, P)
set_mode = bind('archive_entry_set_mode', None, P, C.c_uint)
set_symlink = bind('archive_entry_set_symlink_utf8', None, P, S)
format_w = bind('archive_write_set_format_zip', I, P)
option_w = bind('archive_write_set_format_option', I, P, S, S, S)
open_w = bind('archive_write_open_filename', I, P, S)
header_w = bind('archive_write_header', I, P, P)
data_w = bind('archive_write_data', C.c_ssize_t, P, P, Z)
finish_w = bind('archive_write_finish_entry', I, P)
close_w = bind('archive_write_close', I, P)
free_w = bind('archive_write_free', I, P)
free_r = bind('archive_read_free', I, P)

def check(result, archive, operation):
    if result < 0:
        raise RuntimeError(f'{operation}: {(error(archive) or b"unknown").decode()} ({result})')
    return result

def rewrite(source, destination, inventory, source_root):
    r, w = new_r(), new_w()
    check(support(r), r, 'ZIP streaming reader')
    check(option_r(r, b'zip', b'mac-ext', None), r, 'Retain explicit AppleDouble')
    fd = os.open(source, os.O_RDONLY)
    check(open_r(r, fd, 1024 * 1024), r, 'Open source')
    check(format_w(w), w, 'ZIP writer')
    check(option_w(w, b'zip', b'compression-level', b'6'), w, 'Deflate level')
    # Leave ZIP64 automatic. Forced local ZIP64 breaks native ditto for small entries.
    check(open_w(w, os.fsencode(destination)), w, 'Open destination')
    buf = C.create_string_buffer(1024 * 1024)
    entries = []; entry = P()
    while True:
        status = next_header(r, C.byref(entry))
        if status == 1: break
        check(status, r, 'Read header')
        pathname = name(entry).decode('utf-8')
        is_link = False
        if not pathname.startswith('__MACOSX/'):
            disk_path = Path(source_root) / pathname.rstrip('/')
            disk_stat = disk_path.lstat()
            set_mode(entry, disk_stat.st_mode)
            is_link = stat.S_ISLNK(disk_stat.st_mode)
            if is_link: set_symlink(entry, os.fsencode(os.readlink(disk_path)))
        check(header_w(w, entry), w, f'Write {pathname}')
        crc, byte_count = 0, 0
        while True:
            n = check(read_data(r, buf, len(buf)), r, f'Read {pathname}')
            if not n: break
            raw = buf.raw[:n]; crc = zlib.crc32(raw, crc); byte_count += n
            if not is_link:
                written = check(data_w(w, buf, n), w, f'Write data {pathname}')
                if written != n: raise RuntimeError(f'Short write: {pathname}')
        check(finish_w(w), w, f'Finish {pathname}')
        entries.append({'name': unicodedata.normalize('NFC', pathname), 'size': byte_count,
                        'crc': crc, 'mode': mode(entry),
                        'symlink': (symlink(entry) or b'').decode('utf-8')})
        if len(entries) % 5000 == 0: print(f'{len(entries)} entries rewritten', flush=True)
    check(close_w(w), w, 'Close archive')
    check(free_w(w), w, 'Free writer'); check(free_r(r), r, 'Free reader'); os.close(fd)
    Path(inventory).write_text(json.dumps({'libarchive': version().decode(), 'entries': entries}, ensure_ascii=False))
    print(f'Complete: {len(entries)} entries; {Path(destination).stat().st_size} bytes', flush=True)

if __name__ == '__main__':
    rewrite(*sys.argv[1:])
