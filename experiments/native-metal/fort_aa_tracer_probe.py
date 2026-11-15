#!/usr/bin/env python3
"""CPU-only Fort AA tracer geometry check; no Metal/runtime acceptance.

CXX selects the compiler. SANITIZERS=address,undefined optionally enables
sanitizers; runner-specific ASAN_OPTIONS remain the caller's responsibility.
"""
from pathlib import Path
import os
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
s=(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp').read_text()
h=(root/'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.hpp').read_text()
struct=h[h.index('struct FortAaShell\n'):h.index('struct OrdnanceState\n')]
func=s[s.index('CVECTOR fortAaShellPosition('):s.index('constexpr float defenseTimerEpsilon')]
start=s.index('    for (const auto &shell : phonkOrdnance.fortAaShells)',s.index('void SHIP::DrawPhonkOrdnanceForOwner'))
block=s[start:s.index('    // Render-only history:',start)]
cpp='''#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <vector>
#include <cassert>
#include <iostream>
#include "c_vector.h"
using entid_t=uint64_t;
'''+struct+func+'''
struct Vertex { CVECTOR position; uint32_t color; };
struct State { std::array<FortAaShell,32> fortAaShells{}; };
struct Core { float remainder; float GetScaledDeltaRemainder(){return remainder;} };
std::vector<Vertex> draw(State &phonkOrdnance, CVECTOR camera, float remainder) {
 Core core{remainder};
 const auto aaShellCount=std::count_if(phonkOrdnance.fortAaShells.begin(), phonkOrdnance.fortAaShells.end(),[](const auto&s){return s.active;});
 std::vector<Vertex> vertices(aaShellCount*12); unsigned at=0;
'''+block+'''
 vertices.resize(at); return vertices;
}
int main(){
 State state; auto &s=state.fortAaShells[0];
 s.active=true; s.launch={0.f,30.f,0.f}; s.velocity={900.f,100.f,0.f}; s.age=.25f;
 auto a=draw(state,{0.f,40.f,-20.f},0); auto b=draw(state,{0.f,40.f,-20.f},.5f);
 assert(a.size()==12 && b.size()==12);
 // Half a simulation millisecond is a visible, continuous .45m displacement.
 assert(std::abs((b[2].position.x-a[2].position.x)-.45f)<.01f);
 assert(s.age==.25f); auto pause=draw(state,{0.f,40.f,-20.f},0);
 for(unsigned i=0;i<a.size();++i) assert(~(a[i].position-pause[i].position)==0);
 assert((a[0].color>>24)==0 && (a[2].color>>24)>0);
 const auto head=fortAaShellPosition(s,s.age);
 const auto tail=fortAaShellPosition(s,s.age-.012f);
 const auto mid=(a[2].position+a[5].position)*.5f;
 assert(~(mid-head)<1e-8f); assert((head-tail).GetLength()>10.f);
 // View aligned with vertical flight must still produce finite nonzero width.
 s.velocity={0.f,900.f,0.f}; auto vertical=draw(state,{0.f,1000.f,0.f},0);
 assert(vertical.size()==12 && ~(vertical[2].position-vertical[5].position)>.1f);
 for(const auto &v:vertical) assert(std::isfinite(v.position.x)&&std::isfinite(v.position.y)&&std::isfinite(v.position.z));
 for(auto &shell:state.fortAaShells) shell=s;
 assert(draw(state,{20.f,50.f,-20.f},.5f).size()==32*12);
 for(auto &shell:state.fortAaShells) shell.active=false;
 assert(draw(state,{0.f,0.f,0.f},0).empty());
 s.active=true;s.age=0;assert(draw(state,{0.f,0.f,0.f},0).empty());
 std::cout<<"PASS: actual CVECTOR + extracted Fort AA render block; 32-slot capacity, fractional motion, pause, taper, vertical camera, launch, retirement\\n";
}
'''
with tempfile.TemporaryDirectory(prefix="fort-aa-tracer-") as directory:
    out = Path(directory)
    (out / "probe.cpp").write_text(cpp)
    compiler = shlex.split(os.environ.get("CXX", "c++"))
    flags = ["-std=c++17", "-Wall", "-Wextra", "-Werror"]
    if sanitizers := os.environ.get("SANITIZERS"):
        flags += ["-fsanitize=" + sanitizers, "-fno-omit-frame-pointer"]
    includes = root / "experiments/native-metal/engine/src/libs/math/include"
    subprocess.run(compiler + flags + ["-I", str(includes), str(out / "probe.cpp"),
                   "-o", str(out / "probe")], check=True)
    subprocess.run([str(out / "probe")], check=True)
