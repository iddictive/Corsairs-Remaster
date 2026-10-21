"""Replay frozen private script oracle; not run during evidence retention."""
from pathlib import Path
import os, subprocess, hashlib, json
root=Path(__file__).resolve().parent
project=root.parents[2]
engine=root/"bin/metal-engine"
record=json.loads((root/"invocation.json").read_text())
assert hashlib.sha256(engine.read_bytes()).hexdigest()==record["binary_sha256"]
env=os.environ.copy()
env["STORM_USERDATA"]=str(root/"userdata")
env["DYLD_LIBRARY_PATH"]=str(project/"experiments/native-storm/.cache/d3d9/sdl2-fixed/lib")+":/opt/homebrew/lib"
result=subprocess.run([str(engine)],cwd=root,env=env,capture_output=True,timeout=25)
print("exit",result.returncode)
for name in ("compile.log","error.log"):
    for path in (root/"userdata").rglob(name): print(path.read_text(errors="replace"))
raise SystemExit(result.returncode)
