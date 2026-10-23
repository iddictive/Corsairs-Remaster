"""Replay the actual map camera bodies without launching the game.

Run from the repository root with TASK_SCRATCH_DIR set to an empty directory.
The renderer, controls and player pose are bounded fixtures; camera methods are
compiled directly from the selected Git revision or working source.
"""
import json
import os
from pathlib import Path
import subprocess

root = Path.cwd()
scratch = Path(os.environ['TASK_SCRATCH_DIR'])
prefix = 'experiments/native-metal/engine/src/libs/worldmap/src/'
baseline = 'adbc57a3c0235273a5961cce8531c5c709088329'
prelude = r'''
#include <cmath>
#include <cstdio>
#include <chrono>
struct CVECTOR {
 float x=0,y=0,z=0;
 CVECTOR()=default; CVECTOR(float a,float b,float c):x(a),y(b),z(c){}
};
namespace storm_map_player { constexpr float minHeight=50,maxHeight=300,mapViewPitch=1; }
struct VDX9RENDER { CVECTOR position,angle; void SetCamera(CVECTOR p,CVECTOR a){position=p;angle=a;} };
struct WdmPlayerShip { struct { CVECTOR position; CVECTOR Pos(){return position;} } mtx;
 float yaw=0; float GetAy(){return yaw;} };
struct Objects { void* camera=nullptr; WdmPlayerShip* playerShip; float worldSizeX=2000,worldSizeZ=2000; } objects;
Objects* wdmObjects=&objects;
struct SeaMapTransition { inline static bool active=false;
 static bool OverrideMapCamera(){return false;} static bool Active(){return active;}
 static float MapViewPitch(float destination){return destination;} };
'''
driver = r'''
struct Camera:WdmCamera {
 bool free=false;
 void CtrlProcess(float) override {}
 float MoveLeftRight(float) override{return 0;}
 float MoveUpDown(float) override{return 0;}
 float RotLeftRight(float) override{return 0;}
 float ZoomInOut(float) override{return 0;}
 bool CurrentFreeMode() override{return free;}
 bool GetHightHeight(float&) override{return false;}
};
int main(){
 WdmPlayerShip ship; objects.playerShip=&ship; VDX9RENDER renderer;
 for(int free=0;free<2;free++){
  Camera camera; camera.Init(.6f,130); ship.yaw=0;
  ship.mtx.position=CVECTOR(10,0,20); camera.free=free;
  SeaMapTransition::active=false; camera.Move(.016f,&renderer);
  // Reuse the resident map after the player moved and changed heading at sea.
  ship.yaw=2; ship.mtx.position=CVECTOR(900,0,-700);
  camera.Init(.6f,130); SeaMapTransition::active=true; camera.Move(0,&renderer);
  const float yawError=std::remainder(renderer.angle.y-2.6f,6.283185307f);
  const float anchorX=renderer.position.x+std::sin(renderer.angle.y)*130/std::tan(renderer.angle.x);
  const float anchorZ=renderer.position.z+std::cos(renderer.angle.y)*130/std::tan(renderer.angle.x);
  std::printf("{\"prior_free\":%d,\"yaw_error\":%.9g,\"anchor_error\":%.9g}\n",free,yawError,
    std::hypot(anchorX-900,anchorZ+700));
  // The first ordinary frame must retain the cinematic's final heading even
  // when the player's saved free-camera toggle becomes active again.
  camera.free=true; SeaMapTransition::active=false; camera.Move(.016f,&renderer);
  std::printf("{\"released_free\":%d,\"yaw_error\":%.9g}\n",free,
    std::remainder(renderer.angle.y-2.6f,6.283185307f));
  camera.Move(.016f,&renderer); camera.free=false; camera.Move(.016f,&renderer);
  std::printf("{\"free_to_follow\":%d,\"yaw_error\":%.9g}\n",free,
    std::remainder(renderer.angle.y-2.6f,6.283185307f));
 }
 // A newly constructed ordinary map is the independent compatibility oracle.
 Camera normal; normal.Init(.6f,130); ship.yaw=2; SeaMapTransition::active=false;
 normal.Move(.016f,&renderer);
 std::printf("{\"ordinary_yaw_error\":%.9g}\n",std::remainder(renderer.angle.y-2.6f,6.283185307f));
 Camera locked; locked.Init(.6f,130); locked.lock=true; locked.Move(.016f,&renderer);
 std::printf("{\"locked_yaw_error\":%.9g}\n",renderer.angle.y);
 auto start=std::chrono::steady_clock::now();
 for(int i=0;i<1000000;i++) normal.Move(.016f,&renderer);
 std::printf("{\"million_move_seconds\":%.9g,\"retained_yaw\":%.9g}\n",
   std::chrono::duration<double>(std::chrono::steady_clock::now()-start).count(),renderer.angle.y);
}
'''
results = {}
for mode in ('baseline', 'current'):
    def read(name):
        path = prefix + name
        return (subprocess.check_output(['git', 'show', baseline + ':' + path], text=True)
                if mode == 'baseline' else (root / path).read_text())
    header = '\n'.join(line for line in read('wdm_camera.h').splitlines()
                       if not line.startswith(('#include', '#pragma')))
    source = '\n'.join(line for line in read('wdm_camera.cpp').splitlines()
                       if not line.startswith('#include'))
    unit = scratch / (mode + '.cpp')
    binary = scratch / mode
    unit.write_text(prelude + header + source + driver)
    subprocess.run(['clang++', '-std=c++20', '-O2', '-include', 'cstdint', str(unit), '-o', str(binary)], check=True)
    output = subprocess.check_output([str(binary)], text=True)
    results[mode] = [json.loads(row) for row in output.splitlines()]
print(json.dumps(results, indent=2))
