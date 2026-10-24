#!/usr/bin/env python3
"""ONE task-only bridge TU and isolated link; run only on granted compile lane."""
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys
import time
from build_native_counterfactual import command,safe

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
BUILD=ROOT/"experiments/native-metal/.cache/build"
sys.path.insert(0,str(ROOT/"tools"))
from metal_storage import diagnostic_run

def sha(path):return hashlib.sha256(path.read_bytes()).hexdigest()

def main():
    destination=ROOT/".local-archives/cannon-darts-20261021/angle30-acceptance"
    evidence=HERE/"native-angle30-build"
    assert not destination.exists() and not evidence.exists(),"retain first result"
    destination.mkdir();evidence.mkdir()
    source=HERE/"native-angle-bridge.cpp"
    manifest={"source":str(source.relative_to(ROOT)),"source_sha256":sha(source),"shared_executable_sha256":sha(BUILD/"bin/sea-fleet-benchmark"),"steps":[],"memory_cap_bytes":512*1024**2,"timeout_seconds":120}
    linkargs=command("bin/sea-fleet-benchmark")
    frozen=[Path(a) if Path(a).is_absolute() else BUILD/a for a in linkargs if a.endswith((".a",".o"))]
    manifest["precompile_linked_inputs"]={safe(str(path.resolve())):sha(path) for path in frozen}
    with diagnostic_run(ROOT/"experiments/native-metal/.cache/diagnostics/cannon-darts/angle30-build") as (temporary,budget):
        def execute(args,name):
            started=time.monotonic();peak=0
            with (destination/(name+".log")).open("w") as log:
                process=budget.start(args,cwd=BUILD,env={**os.environ,"TMPDIR":str(temporary)+os.sep},stdout=log,stderr=subprocess.STDOUT)
                while process.poll() is None:
                    rows=subprocess.check_output(["ps","-axo","pgid=,rss="],text=True)
                    rss=sum(int(line.split()[1])*1024 for line in rows.splitlines() if line.split() and int(line.split()[0])==process.pid)
                    peak=max(peak,rss)
                    if rss>512*1024**2 or time.monotonic()-started>120:
                        budget.close();raise RuntimeError("owned compile/link memory or deadline exceeded")
                    time.sleep(.1)
            manifest["steps"].append({"name":name,"argv":[safe(a) for a in args],"returncode":process.returncode,"peak_process_group_rss_bytes":peak,"wall_seconds":time.monotonic()-started})
            (evidence/(name+".log")).write_text(safe((destination/(name+".log")).read_text(errors="replace")))
            (evidence/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
            if process.returncode:raise RuntimeError(name+" failed; stop and retain raw first log")
        args=command("native-engine/storm/libs/sea_ai/CMakeFiles/sea_ai.dir/src/ai_cannon.cpp.o")
        output=destination/"native-angle-bridge.cpp.o"
        for flag,path in (("-o",output),("-MF",temporary/"bridge.d"),("-MT",output)):
            args[args.index(flag)+1]=str(path)
        args[args.index("-c")+1]=str(source)
        args.extend(["-iquote",str(ROOT/"experiments/native-metal/engine/src/libs/sea_ai/src")])
        execute(args,"bridge")
        assert manifest["precompile_linked_inputs"]=={safe(str(path.resolve())):sha(path) for path in frozen}
        assert manifest["shared_executable_sha256"]==sha(BUILD/"bin/sea-fleet-benchmark")
        args=linkargs.copy()
        executable=destination/"sea-fleet-benchmark"
        args[args.index("-o")+1]=str(executable)
        args.insert(args.index("-o"),str(output))
        linked=[Path(a) if Path(a).is_absolute() else BUILD/a for a in args if a.endswith((".a",".o"))]
        manifest["linked_inputs"]={safe(str(path.resolve())):sha(path) for path in linked}
        execute(args,"link")
        assert manifest["linked_inputs"]=={safe(str(path.resolve())):sha(path) for path in linked}
        manifest["executable_sha256"]=sha(executable)
        (evidence/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
        print(json.dumps({"executable":str(executable.relative_to(ROOT)),"sha256":sha(executable),"steps":manifest["steps"]},indent=2))

if __name__=="__main__":main()
