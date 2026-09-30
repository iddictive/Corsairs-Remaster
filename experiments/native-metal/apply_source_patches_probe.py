#!/usr/bin/env python3
"""Isolated acceptance for ordered patch-stack migration and failure atomicity."""
import base64
import difflib
import importlib.util
import json
from pathlib import Path
import subprocess
import sys
import tempfile
spec=importlib.util.spec_from_file_location('stack',Path(__file__).with_name('apply_source_patches.py'));m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
def patch(name,before,after,path='a.txt'):
 return {'name':name,'text':''.join(difflib.unified_diff(before.splitlines(True),after.splitlines(True),fromfile='a/'+path,tofile='b/'+path))}
def reject(fn):
 try:fn()
 except m.Conflict:return
 raise AssertionError('invalid input accepted')
root=Path(subprocess.check_output(['mktemp','-d',str(Path(tempfile.gettempdir())/'storm-patch-probe.XXXXXXXXXX')],text=True).strip())
try:
 source=root/'source';source.mkdir();state=root/'state.json';file=source/'a.txt'
 assert m.state_status(source,state,'platform.patch')=='absent'
 malformed=root/'malformed.json';malformed.write_text('{not-json')
 assert m.state_status(source,malformed,'platform.patch')=='malformed'
 base='first\nbase\nlast\n';one='first\nlayer-one\nlast\n';two='first\nlayer-two\nlast\n'
 a=patch('one',base,one);b=patch('two',one,two);file.write_text(base);(source/'unrelated').write_text('preserve')
 assert m.update(source,state,[a,b])==1 and file.read_text()==two
 assert m.update(source,state,[a,b])==0
 # Independent reverse checks would reject a while b overlaps it; whole-stack bootstrap must accept.
 state.unlink();assert m.update(source,state,[a,b])==0 and file.read_text()==two
 # A known untracked leading patch may be migrated only from an existing state.
 file.write_text(base)
 state.unlink()
 legacy_prefix=patch('platform.patch',base,'first\nlegacy-platform\nlast\n')
 m.apply_stack(source,[legacy_prefix])
 legacy_one=patch('one','first\nlegacy-platform\nlast\n',one);legacy_two=patch('two',one,two)
 assert m.update(source,state,[legacy_one,legacy_two])==1 and file.read_text()==two
 assert m.state_status(source,state,'platform.patch')=='legacy'
 current_prefix=patch('platform.patch',base,'first\ncurrent-platform\nlast\n')
 current_one=patch('one','first\ncurrent-platform\nlast\n','first\ncurrent-one\nlast\n')
 current_two=patch('two','first\ncurrent-one\nlast\n','first\ncurrent-two\nlast\n')
 assert m.update(source,state,[current_prefix,current_one,current_two],[legacy_prefix])==1
 assert file.read_text()=='first\ncurrent-two\nlast\n'
 assert m.state_status(source,state,'platform.patch')=='promoted'
 assert subprocess.check_output([sys.executable,Path(__file__).with_name('apply_source_patches.py'),'--source',source,'--state',state,'--state-status','platform.patch'],text=True).strip()=='promoted'
 # The state may commit before its caller advances the fingerprint.  Ordinary
 # stack validation is the only safe continuation in that interrupted window.
 assert m.update(source,state,[current_prefix,current_one,current_two])==0
 migrated_state=state.read_bytes();reject(lambda:m.update(source,state,[current_prefix,current_one,current_two],[legacy_prefix]));assert state.read_bytes()==migrated_state
 wrong_prefix=patch('platform.patch',base,'first\nwrong-platform\nlast\n')
 reject(lambda:m.update(source,state,[current_prefix,current_one,current_two],[wrong_prefix]));assert file.read_text()=='first\ncurrent-two\nlast\n'
 newer='first\nnew-one\nlast\n';final='first\nnew-two\nlast\n';c=patch('one',base,newer);d=patch('two',newer,final)
 assert m.update(source,state,[c,d])==1 and file.read_text()==final
 file.write_text('DRIFT\nnew-two\nlast\n');saved=state.read_bytes();reject(lambda:m.update(source,state,[c,d]));assert file.read_text()=='DRIFT\nnew-two\nlast\n' and state.read_bytes()==saved
 file.write_text(final)
 attack=patch('bad',base,one,'../escape');reject(lambda:m.update(source,state,[attack]));assert not (root/'escape').exists()
 (source/'link').symlink_to(file);bad=patch('bad',base,one,'link');reject(lambda:m.update(source,state,[bad]));assert file.read_text()==final
 # Added files, then removal when migrating to an empty stack.
 add={'name':'add','text':'--- /dev/null\n+++ b/new.txt\n@@ -0,0 +1 @@\n+created\n'}
 assert m.update(source,state,[c,d,add])==1 and (source/'new.txt').read_text()=='created\n'
 assert m.update(source,state,[c,d])==1 and not (source/'new.txt').exists()
 # Fail a multi-file transaction after writing a.txt, before z.txt: old bytes/state restored.
 (source/'z.txt').write_text('old\n');z=patch('z','old\n','new\n','z.txt');oldstate=state.read_bytes();real=m.atomic_write;armed=[True]
 def fail_once(path,data,mode=0o644):
  if path==source.resolve()/'z.txt' and armed[0]:armed[0]=False;raise OSError('simulated write interruption')
  return real(path,data,mode)
 m.atomic_write=fail_once
 try:
  try:m.update(source,state,[a,b,z])
  except OSError:pass
  else:raise AssertionError('expected write failure')
 finally:m.atomic_write=real
 assert file.read_text()==final and (source/'z.txt').read_text()=='old\n' and state.read_bytes()==oldstate
 # Seed a crash journal with one new source file but old state; recovery is exercised by the normal entrypoint.
 before=m.capture(source,['a.txt']);file.write_text(two);after=m.capture(source,['a.txt'])
 pending={'source':str(source.resolve()),'before':before,'after':after,'old_state':base64.b64encode(oldstate).decode(),'new_state':base64.b64encode(b'uncommitted').decode()}
 state.with_name(state.name+'.journal').write_text(json.dumps(pending))
 assert m.update(source,state,[c,d])==0 and file.read_text()==final
 assert (source/'unrelated').read_text()=='preserve'
 print('PASS: cold/warm overlapping stack, known legacy-prefix migration, bootstrap, canonical migration, additions/removal, conflict no-write, traversal/symlink rejection, partial-write rollback, crash recovery, unrelated-file preservation')
finally:
 shutil.rmtree(root)
