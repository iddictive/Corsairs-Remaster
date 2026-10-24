#!/usr/bin/env python3
"""Two serial old-AIBalls/factory compilations and one isolated diagnostic link."""
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import time

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
BUILD=ROOT/"experiments/native-metal/.cache/build"
SOURCE=ROOT/"experiments/native-metal/engine/src/libs/sea_ai/src"
ARCHIVE=ROOT/".local-archives/cannon-darts-20261021"
sys.path.insert(0,str(ROOT/"tools"))
from metal_storage import diagnostic_run

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def safe(value): return value.replace(str(ROOT),"<repo>").replace(str(Path.home()),"<home>")
def command(target):
    lines=subprocess.check_output(["ninja","-C",str(BUILD),"-t","commands",target],text=True).splitlines()
    matching=[line for line in lines if target in line]
    assert matching
    args=shlex.split(matching[-1])
    if "&&" in args: args=args[args.index("&&")+1:len(args)-2]
    assert args[0]=="/usr/bin/c++"
    return args

def main():
    destination=ARCHIVE/"counterfactual-2tu"
    evidence=HERE/"native-counterfactual-build"
    assert not destination.exists() and not evidence.exists()
    destination.mkdir();evidence.mkdir()
    paired=destination/"sea-fleet-benchmark.current-world"
    shutil.copy2(BUILD/"bin/sea-fleet-benchmark",paired)
    frozen=ARCHIVE/"baseline"
    original=json.loads((frozen/"inputs.json").read_text())
    for relative,row in original.items():
        assert sha(frozen/Path(relative).name)==row["sha256"]
    retained_sources={"old-ai_balls.cpp":frozen/"ai_balls.cpp","old-ai_balls.h":frozen/"ai_balls.h","factory-sea_ai.cpp":SOURCE/"sea_ai.cpp","factory-sea_ai.h":SOURCE/"sea_ai.h"}
    for name,path in retained_sources.items(): shutil.copyfile(path,destination/name)
    source_hashes={str(p.relative_to(ROOT)):sha(p) for p in (SOURCE/"ai_balls.h",SOURCE/"sea_ai.cpp",SOURCE/"sea_ai.h")}
    manifest={"source_hashes":source_hashes,"frozen_sources":{name:sha(destination/name) for name in retained_sources},"paired_current_executable":str(paired.relative_to(ROOT)),"paired_current_sha256":sha(paired),"compile_memory_cap_bytes":2*1024**3,"compile_timeout_seconds":120,"scope":"old AIBalls cpp/header and current matching factory; current remaining world archives","vfs_contract":"https://llvm.org/docs/doxygen/VirtualFileSystem_8h_source.html","steps":[]}
    run=ROOT/"experiments/native-metal/.cache/diagnostics/cannon-darts/counterfactual-build"
    with diagnostic_run(run) as (temporary,budget):
        sourcecopy=temporary/"source";sourcecopy.mkdir()
        shutil.copyfile(frozen/"ai_balls.cpp",sourcecopy/"ai_balls.cpp")
        vfs=temporary/"vfs.json"
        vfs.write_text(json.dumps({"version":0,"use-external-names":False,"roots":[{"type":"file","name":str(SOURCE/"ai_balls.h"),"external-contents":str(destination/"old-ai_balls.h")}]},indent=2))
        def execute(args,name,timeout=120):
            started=time.monotonic();peak=0
            logfile=destination/(name+".log")
            with logfile.open("w") as log:
                process=budget.start(args,cwd=BUILD,env={**os.environ,"TMPDIR":str(temporary)+os.sep},stdout=log,stderr=subprocess.STDOUT)
                while process.poll() is None:
                    rows=subprocess.check_output(["ps","-axo","pgid=,rss="],text=True)
                    rss=sum(int(line.split()[1])*1024 for line in rows.splitlines() if line.split() and int(line.split()[0])==process.pid)
                    peak=max(peak,rss)
                    if rss>2*1024**3 or time.monotonic()-started>timeout:
                        budget.close();raise RuntimeError("owned compile/link memory or deadline exceeded")
                    time.sleep(.1)
                result=process.returncode
            manifest["steps"].append({"name":name,"argv":[safe(a) for a in args],"returncode":result,"peak_process_group_rss_bytes":peak,"wall_seconds":time.monotonic()-started})
            (evidence/(name+".log")).write_text(safe(logfile.read_text(errors="replace")))
            (evidence/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
            if result: raise RuntimeError(name+" failed; raw first log retained")
        objects=[]
        for name in ("ai_balls","sea_ai"):
            target=f"native-engine/storm/libs/sea_ai/CMakeFiles/sea_ai.dir/src/{name}.cpp.o"
            args=command(target);output=destination/(name+".cpp.o")
            for flag,path in (("-o",output),("-MF",temporary/(name+".d")),("-MT",output)):
                args[args.index(flag)+1]=str(path)
            if name=="ai_balls":args[args.index("-c")+1]=str(sourcecopy/"ai_balls.cpp")
            args.extend(["-iquote",str(SOURCE),"-ivfsoverlay",str(vfs)])
            execute(args,name);objects.append(output)
        assert source_hashes=={str(p.relative_to(ROOT)):sha(p) for p in (SOURCE/"ai_balls.h",SOURCE/"sea_ai.cpp",SOURCE/"sea_ai.h")}
        print("header_freeze_done=1",flush=True)
        library=destination/"libsea_ai.a"
        shutil.copyfile(BUILD/"lib/libsea_ai.a",library)
        members=subprocess.check_output(["/usr/bin/ar","-t",str(library)],text=True).splitlines()
        assert members.count("ai_balls.cpp.o")==1 and members.count("sea_ai.cpp.o")==1
        execute(["/usr/bin/ar","-d",str(library),"ai_balls.cpp.o","sea_ai.cpp.o"],"archive-remove")
        execute(["/usr/bin/ar","-r",str(library),*[str(p) for p in objects]],"archive-insert")
        execute(["/usr/bin/ranlib",str(library)],"archive-index")
        args=command("bin/sea-fleet-benchmark")
        executable=destination/"sea-fleet-benchmark"
        args[args.index("-o")+1]=str(executable)
        substitutions=0
        for index,arg in enumerate(args):
            if Path(arg).name=="libsea_ai.a":args[index]=str(library);substitutions+=1
        assert substitutions>=1
        execute(args,"link")
        manifest.update(executable=str(executable.relative_to(ROOT)),executable_sha256=sha(executable),archive_sha256=sha(library),archive_replacements=substitutions)
        (destination/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
        (evidence/"manifest.json").write_text(json.dumps(manifest,indent=2)+"\n")
        print(json.dumps({key:manifest[key] for key in ("executable","executable_sha256","archive_replacements")}),flush=True)

if __name__=="__main__":main()
