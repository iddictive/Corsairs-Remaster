#!/usr/bin/env python3
# One-off evidence reproduction. No project, app, GPU or player-state edits.
import json,pathlib,subprocess,tempfile,tarfile,hashlib,resource
here=pathlib.Path(__file__).resolve().parent
work=pathlib.Path(tempfile.mkdtemp(prefix="corsairs-short-dose-replay-"))
with tarfile.open(here/"frozen-headers.tar.gz") as archive:archive.extractall(work,filter="data")
old="/REQUIRED_EXTERNAL_INPUT/scratch"
original_work="/REQUIRED_EXTERNAL_INPUT/scratch"
source=(here/"short_cpu.cpp").read_text().replace(old,str(work/"headers"))
(work/"short_cpu.cpp").write_text(source)
(work/"read_input.hpp").write_bytes((here/"read_input.hpp").read_bytes())
command=[x.replace(old,str(work/"headers")).replace(original_work,str(work)) for x in json.loads((here/"compile-command.json").read_text())]
resource.setrlimit(resource.RLIMIT_CPU,(120,120))
subprocess.run(command,cwd=work,check=True,timeout=115)
input_path=here.parent/"capture-ship/first-rejected-node-0.input.bin"
assert hashlib.sha256(input_path.read_bytes()).hexdigest()=="f0cd84f0f22da84089e2f42838b25f9288570759883f40d8b72fcc3213f171ca"
asset=here.parent.parent/"held-fire-first/assets/ships/Galeon_l1/Galeon_l1.gm"
assert hashlib.sha256(asset.read_bytes()).hexdigest()=="4aa21bab75c2604bbafe5166d77489c15a3665b28b282b123f82af2c432b9402"
run=subprocess.run([str(work/"short-probe"),str(input_path),str(asset)],cwd=work,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=90)
(work/"run.log").write_bytes(run.stdout)
print(run.stdout.decode());print("Reproduction intermediates retained at",work)
raise SystemExit(run.returncode)
