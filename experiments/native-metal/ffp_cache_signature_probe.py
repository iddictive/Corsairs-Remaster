#!/usr/bin/env python3
"""Compile the backend's actual conversion key; no Metal device or game process."""
from pathlib import Path
import subprocess
import sys

root = Path(__file__).resolve().parent
backend = (root / "backend.mm").read_text()
start = backend.index("std::array<uint64_t,8> conversionParts{};")
end = backend.index("return h;};", start) + len("return h;};")
signature = backend[start:end]
stabilizer = backend[backend.index("simd_float3 stableSunDirection("):backend.index("\nvoid caps(")]
source = r'''
#include <d3d9.h>
#include <simd/simd.h>
#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
static void check(bool ok,const char*message){if(!ok){fprintf(stderr,"FAIL: %s\n",message);exit(1);}}
''' + stabilizer + r'''
int main(){
 struct {bool enabled=true;}profile;
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
 landShadow.sceneLampCatalog=true;
 constexpr double angularCell=6.2831853071795864769/32768.;
 auto ray=[](double azimuth,double elevation){return simd_make_float3(float(std::cos(elevation)*std::cos(azimuth)),float(std::sin(elevation)),float(std::cos(elevation)*std::sin(azimuth)));};
 auto assignSun=[&](simd_float3 input){const auto value=stableSunDirection(input);lights[0].Direction={value.x,value.y,value.z};};
 assignSun(ray(2000*angularCell,-3000*angularCell));const auto stableKey=conversionSignature();
 for(int frame=0;frame<120;++frame){assignSun(ray((2000+.25*frame/120.)*angularCell,-3000*angularCell));check(conversionSignature()==stableKey,"subtexel sun evolution reuses cached geometry for 120 frames");}
 assignSun(ray(2002*angularCell,-3000*angularCell));check(conversionSignature()!=stableKey,"meaningful sun-angle change invalidates cached geometry");
 double maximumDisplacement=0;unsigned evolution=0;uint64_t prior=0;
 for(int elevation=-64;elevation<=64;++elevation)for(int azimuth=-128;azimuth<=128;++azimuth){
  const auto original=ray(azimuth*.02454567,elevation*.024531),snapped=stableSunDirection(original),twice=stableSunDirection(snapped);
  check(simd_all(snapped==twice),"sun snapping is idempotent through SetLight/GetLight and the shadow bridge");
  maximumDisplacement=std::max(maximumDisplacement,double(simd_distance(original,snapped))*160.);
 }
 check(maximumDisplacement<.5*32./512.,"sun angular error remains below half the finest shadow texel across 160 world units");
 for(int frame=0;frame<1000;++frame){assignSun(ray(frame*.001,-.5));const auto key=conversionSignature();evolution+=key!=prior;prior=key;}
 check(evolution>990,"time-of-day angle evolution is preserved instead of freezing sunlight");
 check(simd_all(stableSunDirection(simd_make_float3(0.f))==simd_make_float3(0.f)),"zero sun direction is not synthesized");
 std::printf("sun maximum shadow displacement %.6f world units (< 0.03125)\n",maximumDisplacement);
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
