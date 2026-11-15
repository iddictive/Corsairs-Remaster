#!/usr/bin/env python3
"""Compile the actual zero-tick firing guard; no renderer or gameplay stand-ins beyond its boundary."""
from pathlib import Path
import os
import subprocess
import tempfile
root = Path(__file__).resolve().parents[3]
source = (root / 'experiments/native-metal/engine/src/libs/ship/src/phonk_ordnance.cpp').read_text()
start = source.index('    if (!deltaTime) {', source.index('void SHIP::ExecutePhonkOrdnance('))
end = source.index('\n    if (active)', start + 1)
block = source[start:end]
cpp = r'''
#include <cassert>
#include <cstdint>
#include <iostream>
struct State { bool charging=true,burstArmed=true,triggerHeld=false; float charge=.5f; double burstSeconds=.2,burstDebt=.75; };
struct Attr { int clears=0; Attr* VerifyAttributeClass(const char*){return this;} void SetAttributeUseDword(const char*,unsigned){++clears;} };
struct Sea { Attr* AttributesPointer; };
struct Core { float ms=.4f; float GetVisualDeltaTime(){return ms;} } core;
bool tick(State &state,uint32_t deltaTime,Sea *pSea,bool active) {
 bool advanced=false;
 auto body=[&](){
''' + block + r'''
 advanced=true;
 };body();return advanced;
}
int main(){
 Attr attr; Sea sea{&attr}; State state;
 for(int n=0;n<120;++n){assert(!tick(state,0,&sea,true));assert(state.burstArmed && state.burstDebt==.75 && !state.triggerHeld && state.charging);}
 assert(attr.clears==0);assert(tick(state,1,&sea,true));
 core.ms=0;assert(!tick(state,0,&sea,true));
 assert(!state.charging && state.charge==0 && !state.burstArmed && state.burstDebt==0 && state.burstSeconds==0 && state.triggerHeld && attr.clears==1);
 State inactive;assert(!tick(inactive,0,&sea,false));assert(attr.clears==1 && inactive.triggerHeld);
 std::cout<<"PASS: rounded-zero frames retain trigger/burst; true pause cancels; no zero-tick launch/damage; active/inactive mode\n";
}
'''
with tempfile.TemporaryDirectory(prefix='corsairs-firing-') as directory:
    path = Path(directory) / 'probe.cpp'; path.write_text(cpp)
    output = path.with_suffix('')
    subprocess.run([os.environ.get('CXX','c++'),'-std=c++17','-Wall','-Wextra','-Werror',str(path),'-o',str(output)],check=True)
    subprocess.run([str(output)],check=True)
