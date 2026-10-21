#!/usr/bin/env python3
"""Prepare source and exact native compile/link argv; never compile or run anything."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shlex

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
NATIVE=ROOT/'experiments/native-metal'
BUILD=NATIVE/'.cache/build'

def replace_once(text,before,after):
    assert text.count(before)==1,(before,text.count(before))
    return text.replace(before,after)

def block(ninja,needle):
    matches=[m for m in re.finditer(r'^build ([^\n]+)\n((?:  [^\n]*\n)*)',ninja,re.M)
             if needle in m[1].split(':',1)[0]]
    assert len(matches)==1,(needle,len(matches))
    values=dict(re.findall(r'^  ([A-Z_]+) = (.*)$',matches[0][2],re.M))
    return matches[0][1].split(':',1)[0],values

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--sanitize',action='store_true')
    parser.add_argument('--allocator',choices=('mimalloc','system'),default='mimalloc')
    args=parser.parse_args()
    assert not args.sanitize or args.allocator=='system','ASan mode must omit the benchmark mimalloc new/delete override'
    tag='asan-system' if args.sanitize else args.allocator
    out=HERE/tag
    out.mkdir(exist_ok=True)
    ninja=(BUILD/'build.ninja').read_text()
    driver=(NATIVE/'sea_fleet_benchmark.cpp').read_text()
    driver=replace_once(driver,'class SeaBenchmarkLibrary', '#include "hull_exercise.hpp"\n\nclass SeaBenchmarkLibrary')
    if args.allocator=='system':
        driver=replace_once(driver,'#include <mimalloc-new-delete.h>','// Diagnostic uses the sanitizer/system C++ allocator.')
    driver=replace_once(driver,'        const auto begin = std::chrono::steady_clock::now();',
        '        hull_exercise::beforeFrame(frame);\n        const auto begin = std::chrono::steady_clock::now();')
    driver=replace_once(driver,'        const auto end = std::chrono::steady_clock::now();',
        '        hull_exercise::afterFrame(frame);\n        const auto end = std::chrono::steady_clock::now();')
    driver=replace_once(driver,'    core_internal.EraseEntities();','    hull_exercise::finish();\n    core_internal.EraseEntities();')
    (out/'driver.cpp').write_text(driver)
    backend=(NATIVE/'backend.mm').read_text()
    backend=replace_once(backend,'#include "indexed_span_frame_cache.hpp"','#include "indexed_span_frame_cache.hpp"\n#include "cache_check.hpp"')
    signature='storm_render::Result DrawIndexedPrimitive(storm_render::Primitive t,int32_t b,uint32_t minimum,uint32_t vertexCount,uint32_t start,uint32_t n)override{'
    backend=replace_once(backend,signature,signature+'\n  hull_cache_diag::Scope diagnosticScope(indexedSpanCache);')
    # Check the complete owner topology immediately before revision invalidation.
    before='if(cacheable)indexedSpanCache.observe(spanKey);'
    after='''if(cacheable){const auto ir=indexedSpanCache.indexRevisions.find(spanKey[0]);const auto vr=indexedSpanCache.vertexRevisions.find(spanKey[2]);if((ir!=indexedSpanCache.indexRevisions.end()&&ir->second!=spanKey[1])||(vr!=indexedSpanCache.vertexRevisions.end()&&vr->second!=spanKey[3]))hull_cache_diag::check(indexedSpanCache,"before-revision-evict");indexedSpanCache.observe(spanKey);}'''
    backend=replace_once(backend,before,after)
    backend+='\nextern "C" void StormMetalHullCacheCheck(void*p,const char*phase){hull_cache_diag::require(p!=nullptr,phase,"device");auto*d=static_cast<Device*>(static_cast<storm_render::Device*>(p));hull_cache_diag::check(d->indexedSpanCache,phase);}\n'
    (out/'backend.mm').write_text(backend)
    sources=[
        ('driver', 'native-engine/CMakeFiles/sea-fleet-benchmark.dir/__/sea_fleet_benchmark.cpp.o',out/'driver.cpp',None),
        ('backend','CMakeFiles/storm_metal.dir/backend.mm.o',out/'backend.mm','libstorm_metal.a'),
        ('geom_static','native-engine/storm/libs/geometry/CMakeFiles/geometry.dir/src/geom_static.cpp.o',NATIVE/'engine/src/libs/geometry/src/geom_static.cpp','lib/libgeometry.a'),
        ('s_device','native-engine/storm/libs/renderer/CMakeFiles/renderer.dir/src/s_device.cpp.o',NATIVE/'engine/src/libs/renderer/src/s_device.cpp','lib/librenderer.a'),
        ('t_carcass','native-engine/storm/libs/sea_foam/CMakeFiles/sea_foam.dir/src/t_carcass.cpp.o',NATIVE/'engine/src/libs/sea_foam/src/t_carcass.cpp','lib/libsea_foam.a')]
    compile_commands=[];archives=[]
    for name,object_name,source,archive in sources:
        _,values=block(ninja,object_name)
        obj=out/(name+('.mm.o' if name=='backend' else '.cpp.o'))
        flags=shlex.split(values['DEFINES'])+shlex.split(values['INCLUDES'])+shlex.split(values['FLAGS'])
        flags+=['-I'+str(HERE),'-I'+str(NATIVE/'engine/src/libs/sea_foam/src')]
        if args.sanitize:flags+=['-O1','-g','-fsanitize=address,undefined','-fno-omit-frame-pointer']
        command=['/usr/bin/c++',*flags,'-o',str(obj),'-c',str(source)]
        compile_commands.append(command)
        if archive:archives.append({'original':str(BUILD/archive),'replacement':str(out/Path(archive).name),
            'remove_member':Path(object_name).name,'append_object':str(obj)})
    _,link=block(ninja,'bin/sea-fleet-benchmark')
    argv=['/usr/bin/c++',*shlex.split(link['FLAGS']),*shlex.split(link['LINK_FLAGS']),str(out/'driver.cpp.o'),
          '-o',str(out/'hull-cache-benchmark'),*shlex.split(link.get('LINK_PATH','')),*shlex.split(link['LINK_LIBRARIES'])]
    replacements={a['original']:a['replacement'] for a in archives}
    for a in archives:replacements[str(Path(a['original']).relative_to(BUILD))]=a['replacement']
    argv=[replacements.get(word,word) for word in argv]
    if args.sanitize:argv+=['-fsanitize=address,undefined']
    inputs=[NATIVE/'sea_fleet_benchmark.cpp',NATIVE/'backend.mm',NATIVE/'indexed_span_frame_cache.hpp',
            NATIVE/'engine/src/libs/geometry/src/geom_static.cpp',NATIVE/'engine/src/libs/renderer/src/s_device.cpp',
            NATIVE/'engine/src/libs/sea_foam/src/t_carcass.cpp',NATIVE/'resources.hpp',
            HERE/'hull_exercise.hpp',HERE/'cache_check.hpp',out/'driver.cpp',out/'backend.mm']
    manifest={'cwd':str(BUILD),'allocator':args.allocator,'sanitize':args.sanitize,
        'source_sha256':{str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs},
        'compile_commands':compile_commands,'archives':archives,'link_command':argv,
        'archive_procedure':'Copy each original archive to replacement, require ar -t reports exactly one remove_member, ar -d replacement remove_member, ar -q replacement append_object, ranlib replacement. Do not edit originals.',
        'execution_owner':'root only; hidden fleet benchmark; no original app or SAVE'}
    (out/'plan.json').write_text(json.dumps(manifest,indent=2)+'\n')
    runner=(ROOT/'tools/run_sea_benchmark.py').read_text()
    runner=replace_once(runner,'ROOT = Path(__file__).resolve().parents[1]',
        'ROOT = Path('+repr(str(ROOT))+')\nimport sys\nsys.path.insert(0,str(ROOT / "tools"))')
    # metal_storage is imported before ROOT in the canonical runner.
    runner=replace_once(runner,'from metal_storage import diagnostic_run, ensure_build_size, ensure_space, maintain',
        'import sys\nsys.path.insert(0,'+repr(str(ROOT/'tools'))+')\nfrom metal_storage import diagnostic_run, ensure_build_size, ensure_space, maintain')
    runner=replace_once(runner,'binary = NATIVE / ".cache/build/bin/sea-fleet-benchmark"',
        'binary = Path('+repr(str(out/'hull-cache-benchmark'))+')')
    beginning='    with (NATIVE / ".cache/diagnostics/sea-fleet-build.log").open("w") as build_log:'
    ending='    if metal_engine_source.fingerprint() != source_identity:'
    assert runner.count(beginning)==runner.count(ending)==1
    start=runner.index(beginning);end=runner.index(ending,start)
    runner=runner[:start]+'''    plan=json.loads(Path('''+repr(str(out/'plan.json'))+''').read_text())
    for name, expected in plan['source_sha256'].items():
        if hashlib.sha256((ROOT/name).read_bytes()).hexdigest()!=expected:
            raise RuntimeError('diagnostic source changed since plan: '+name)
'''+runner[end:]
    runner=replace_once(runner,'"instrumented": args.profile is not None or args.capture}',
        '"instrumented": True, "hull_diag_mode": env.get("HULL_DIAG_MODE", "cuts"), "diagnostic_plan": '+repr(str(out/'plan.json'))+'}')
    (out/'run_variant.py').write_text(runner)
    print(out/'plan.json')

if __name__=='__main__':main()
