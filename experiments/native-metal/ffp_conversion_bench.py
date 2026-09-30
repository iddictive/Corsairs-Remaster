#!/usr/bin/env python3
"""Compile the actual FFP conversion loops from two backend snapshots.

CPU-only proxy: this never creates a Metal device, game process or window.
Usage: ffp_conversion_bench.py BEFORE.mm AFTER.mm OUTPUT_DIRECTORY
"""
from pathlib import Path
import subprocess
import sys

before, after, output = map(Path, sys.argv[1:])
output.mkdir(parents=True, exist_ok=True)
root = Path(__file__).resolve().parent
headers = root.parent / 'native-storm/.cache/d3d9/dxvk-native/include/native'

def loop(path):
    source = path.read_text()
    start = source.index('if(!data||!library)return D3DERR_INVALIDCALL;')
    end = source.index(' auto stateResult=applyState', start)
    return source[start:end].replace('!data||!library', '!data')

preamble = r'''
#include <d3d9.h>
#include <simd/simd.h>
#include <vector>
#include <map>
#include <array>
#include <cstring>
#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cmath>
struct Vertex {float p[4],c[4],uv[8],specular[4],cameraNormal[4],cameraPosition[4];};
simd_float4 color(DWORD c){return {(c>>16&255)/255.f,(c>>8&255)/255.f,(c&255)/255.f,(c>>24)/255.f};}
struct State {
 std::array<DWORD,256> rs{};DWORD ts[8][64]{};
 std::map<int,D3DMATRIX> matrices;D3DMATERIAL9 material{};D3DLIGHT9 lights[8]{};bool enabled[8]{};
 DWORD fvf=D3DFVF_XYZ|D3DFVF_NORMAL|D3DFVF_DIFFUSE|D3DFVF_TEX1;
 static UINT count(D3DPRIMITIVETYPE,UINT n){return n*3;}
 State(){for(int i:{D3DTS_WORLD,D3DTS_VIEW}){D3DMATRIX m{};for(int k=0;k<4;k++)m.m[k][k]=1;matrices[i]=m;}material.Diffuse={.8,.7,.6,1};material.Ambient={.1,.1,.1,1};material.Emissive={.01,.02,.03,1};rs[D3DRS_AMBIENT]=0xff405060;for(int i=0;i<3;i++){enabled[i]=true;lights[i].Type=D3DLIGHT_POINT;lights[i].Position={float(i),2,3};lights[i].Range=100;lights[i].Attenuation0=1;lights[i].Attenuation1=.1;lights[i].Diffuse={.2,.3,.4,1};lights[i].Ambient={.1,.1,.1,1};}for(int i=0;i<8;i++)ts[i][D3DTSS_COLOROP]=i?D3DTOP_DISABLE:D3DTOP_MODULATE;}
'''
footer = r'''
struct Input {float p[3],n[3];DWORD color;float uv[2];};
int main(){
 State state;std::vector<Input> vertices(1024);std::vector<uint16_t> indices(12288);
 for(size_t i=0;i<vertices.size();i++)vertices[i]={{float(i%32)*.1f,0,float(i/32)*.1f},{0,1,0},0xff8090a0,{float(i%16)/16,float(i%8)/8}};
 for(size_t i=0;i<indices.size();i++)indices[i]=uint16_t((i*7)%vertices.size());
 for(int scenario=0;scenario<3;scenario++){
  state.rs[D3DRS_LIGHTING]=scenario!=0;state.ts[0][D3DTSS_TEXCOORDINDEX]=scenario==2?D3DTSS_TCI_CAMERASPACEREFLECTIONVECTOR:0;
  double expected=0;for(int variant=0;variant<2;variant++){
   auto call=[&](){return variant?state.after(D3DPT_TRIANGLELIST,4096,(const uint8_t*)vertices.data(),sizeof(Input),indices.data(),D3DFMT_INDEX16):state.before(D3DPT_TRIANGLELIST,4096,(const uint8_t*)vertices.data(),sizeof(Input),indices.data(),D3DFMT_INDEX16);};
   for(int i=0;i<8;i++)call();auto begin=std::chrono::steady_clock::now();double sum=0;for(int i=0;i<160;i++)sum+=call();auto elapsed=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
   std::printf("scenario=%d variant=%s milliseconds=%.3f checksum=%.9f\n",scenario,variant?"after":"before",elapsed,sum);
   if(!variant)expected=sum;else if(std::abs(expected-sum)>1.e-5){std::fprintf(stderr,"FAIL checksum mismatch\n");return 1;}
  }
 }
 std::puts("PASS: same-input conversion outputs; CPU proxy timings, not game FPS");
}
'''
checksum = r'''
double sum=0;for(auto&vertex:vertices){for(float v:vertex.p)sum+=v;for(float v:vertex.c)sum+=v;for(float v:vertex.uv)sum+=v;if(ts[0][D3DTSS_TEXCOORDINDEX]>>16){for(float v:vertex.cameraNormal)sum+=v;for(float v:vertex.cameraPosition)sum+=v;}}return sum;}
'''
source = preamble
for name, path in [('before', before), ('after', after)]:
    source += f'__attribute__((noinline)) double {name}(D3DPRIMITIVETYPE t,UINT n,const uint8_t*data,UINT st,const void*indices,D3DFORMAT fmt,INT base=0){{'
    source += loop(path) + checksum
source += '};\n' + footer
cpp = output / 'ffp_conversion_bench.cpp'
binary = output / 'ffp-conversion-bench'
cpp.write_text(source)
subprocess.run(['clang++', '-O2', '-std=c++20', '-I'+str(headers/'directx'),
    '-I'+str(headers/'windows'), str(cpp), '-o', str(binary)], check=True)
subprocess.run([str(binary)], check=True)
