from pathlib import Path
import hashlib,json,shlex,shutil,subprocess,difflib

HERE=Path(__file__).resolve().parent
ROOT=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
BASE=ROOT/'.local-archives/metal-renderer-evidence/phonk-current-20261016/integrated-sea-laser'
CLOSURE=BASE/'coherent-8d1d27'
PREVIOUS=BASE/'held-fire-second/ship'
PRODUCER='/REQUIRED_EXTERNAL_INPUT/Corsairs'
OLD='/REQUIRED_EXTERNAL_INPUT/scratch'
NATIVE='experiments/native-metal'
SOURCE=HERE/'headers'/NATIVE/'engine/src/libs/ship/src/ship_phonk_laser.cpp'
sha=lambda p:hashlib.sha256(p.read_bytes()).hexdigest()

def run(name,argv):
    (HERE/(name+'-command.json')).write_text(json.dumps(argv,indent=2)+'\n')
    print(name,flush=True)
    with (HERE/(name+'.log')).open('w') as output:
        p=subprocess.run(argv,cwd=HERE,stdout=output,stderr=subprocess.STDOUT)
    if p.returncode:
        print((HERE/(name+'.log')).read_text()[-5000:]);raise SystemExit(p.returncode)

def remap(a):
    a=a.replace(OLD+'/headers',str(HERE/'headers'))
    for directory in [NATIVE+'/engine',NATIVE+'/.cache/build']:
        a=a.replace(PRODUCER+'/'+directory,str(HERE/'headers'/directory))
    if a=='-I'+PRODUCER+'/'+NATIVE:
        a='-I'+str(HERE/'headers'/NATIVE)
    return a

line=next(x for x in (CLOSURE/'producer-all-commands.txt').read_text().splitlines() if 'ship_phonk_laser.cpp.o' in x and ' -c ' in x)
original=shlex.split(line);args=[];n=0
while n<len(original):
    a=original[n]
    if a in ['-MT','-MF','-o','-c']:n+=2;continue
    if a in ['-MD','-v']:n+=1;continue
    args.append(remap(a));n+=1
args+=['-o',str(HERE/'ship_phonk_laser.cpp.o'),'-c',str(SOURCE)]
assert '-DSTORM_METAL_SHIP_FIRE=1' in args
run('ship-compile',args)

fixture=HERE/'ship';fixture.mkdir(exist_ok=True)
for p in PREVIOUS.rglob('*'):
    if p.is_file() and not p.is_symlink() and p.suffix in ['.cpp','.hpp','.c','.ini']:
        q=fixture/p.relative_to(PREVIOUS);q.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(p,q)
driver=json.loads((PREVIOUS/'compile-command.json').read_text())
driver=[remap(a).replace(OLD+'/second/ship',str(fixture)) for a in driver]
run('driver-compile',driver)

originalArchive=CLOSURE/'libraries/lib/libship.a'
privateArchive=HERE/'libship.a';shutil.copy2(originalArchive,privateArchive)
members=subprocess.check_output(['/usr/bin/ar','t',str(originalArchive)],text=True).splitlines()
assert members.count('ship_phonk_laser.cpp.o')==1
run('archive-replace',['/usr/bin/ar','-rs',str(privateArchive),str(HERE/'ship_phonk_laser.cpp.o')])
changed=[]
for member in members:
    before=subprocess.check_output(['/usr/bin/ar','p',str(originalArchive),member])
    after=subprocess.check_output(['/usr/bin/ar','p',str(privateArchive),member])
    if before!=after:changed.append(member)
assert [m for m in changed if m!='__.SYMDEF']==['ship_phonk_laser.cpp.o'],changed

link=json.loads((PREVIOUS/'link-command.json').read_text())
link=[a.replace(OLD+'/second/ship',str(fixture)).replace(str(originalArchive),str(privateArchive)) for a in link]
assert any(str(privateArchive)==a for a in link)
run('link',link)

diff=''.join(difflib.unified_diff((HERE/'original.cpp').read_text().splitlines(True),SOURCE.read_text().splitlines(True),fromfile='cf75576/ship_phonk_laser.cpp',tofile='diagnostic/ship_phonk_laser.cpp'))
(HERE/'diagnostic.patch').write_text(diff)
identity=json.loads((HERE/'identity.json').read_text())
identity.update({'diagnostic_cpp_sha256':sha(SOURCE),'diagnostic_header_sha256':sha(SOURCE.parent/'laser_diag.hpp'),'diagnostic_diff_sha256':sha(HERE/'diagnostic.patch'),'binary_sha256':sha(fixture/'replay'),'original_archive_sha256':sha(originalArchive),'private_archive_sha256':sha(privateArchive),'changed_archive_members':changed,'fixture_source_sha256':{p.name:sha(p) for p in fixture.iterdir() if p.suffix in ['.cpp','.hpp','.c','.ini']},'capture_limit_bytes':960<<20,'log_limit_bytes':4<<20,'gpu_started':False})
(HERE/'ready.json').write_text(json.dumps(identity,indent=2)+'\n')
print('READY',identity['binary_sha256'],flush=True)
