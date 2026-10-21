#!/usr/bin/env python3
"""Compile the backend's actual conversion key; no Metal device or game process."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
backend = (root / "backend.mm").read_text()
start = backend.index("auto conversionSignature=[&]()")
end = backend.index("return h;};", start) + len("return h;};")
signature = backend[start:end]
source = r'''
#include <d3d9.h>
#include <simd/simd.h>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
static void check(bool ok,const char*message){if(!ok){fprintf(stderr,"FAIL: %s\n",message);exit(1);}}
int main(){
 bool actualLighting=true,dynamicDraw=true,bakedRelight=true,prepassLit=true,modernLit=true;
 simd_float4x4 worldMatrix=matrix_identity_float4x4;
 simd_float4 materialDiffuse={1,1,1,1},materialAmbient={.2f,.3f,.4f,1},materialEmissive={0,0,0,1},sceneAmbient={.1f,.1f,.1f,1};
 D3DLIGHT9 lights[8]{};bool enabled[8]={true,true};
 lights[0].Type=D3DLIGHT_DIRECTIONAL;lights[0].Direction={0,-1,0};lights[0].Diffuse={.6f,.5f,.4f,1};
 lights[1].Type=D3DLIGHT_POINT;lights[1].Position={2,3,4};lights[1].Range=30;lights[1].Attenuation0=1;
 std::array<DWORD,256>rs{};
 struct {bool sceneLampCatalog=true,indoor=false,locationActive=true;unsigned scope=1;struct {unsigned sunValid=3;uint64_t pointId=23,lightIds[8]={0,23};} prepass;}landShadow;
''' + signature + r'''
 const auto initial=conversionSignature();
 for(int frame=0;frame<120;++frame){
  worldMatrix.columns[3]={float(frame)*.125f,.001f*frame,-float(frame)*.3f,1};
  lights[0].Position={worldMatrix.columns[3].x,worldMatrix.columns[3].y,worldMatrix.columns[3].z};
  lights[0].Specular={float(frame),0,0,1};lights[0].Range=float(frame);
  lights[1].Diffuse.r=float(frame);landShadow.prepass.lightIds[1]=frame+100;landShadow.prepass.pointId=frame+100;
  check(conversionSignature()==initial,"camera origin and ignored scene catalog slots must reuse lit conversion");
 }
 lights[0].Direction.x=.1f;check(conversionSignature()!=initial,"sun direction change invalidates lit conversion");lights[0].Direction.x=0;
 lights[0].Diffuse.r=.7f;check(conversionSignature()!=initial,"sun color change invalidates lit conversion");lights[0].Diffuse.r=.6f;
 worldMatrix.columns[0].x=.8f;check(conversionSignature()!=initial,"world normal basis change invalidates lit conversion");worldMatrix.columns[0].x=1;
 materialDiffuse.w=.5f;check(conversionSignature()!=initial,"material alpha change invalidates conversion");materialDiffuse.w=1;
 landShadow.prepass.sunValid=0;const auto night=conversionSignature();lights[0].Direction.x=.5f;
 check(conversionSignature()==night,"skipped sun does not invalidate nighttime conversion");
 landShadow.prepass.sunValid=3;landShadow.sceneLampCatalog=false;const auto point=conversionSignature();
 worldMatrix.columns[3].x+=1;check(conversionSignature()!=point,"CPU point lighting still invalidates on world position");worldMatrix.columns[3].x-=1;
 lights[1].Position.x+=1;check(conversionSignature()!=point,"CPU point lighting still invalidates on light position");lights[1].Position.x-=1;
 landShadow.prepass.pointId+=1;check(conversionSignature()!=point,"selected CPU point contribution identity invalidates");
 puts("PASS: actual FFP key reuses 120 camera-relative sun/catalog frames; consumed light, transform, alpha and shadow changes invalidate");
}
'''
headers = root.parent / "native-storm/.cache/d3d9/dxvk-native/include/native"
work = Path(sys.argv[1])
cpp, binary = work / "probe.cpp", work / "probe"
cpp.write_text(source)
subprocess.run(["clang++", "-std=c++20", "-O2", "-I" + str(headers / "directx"),
                "-I" + str(headers / "windows"), str(cpp), "-o", str(binary)], check=True)
subprocess.run([str(binary)], check=True)
