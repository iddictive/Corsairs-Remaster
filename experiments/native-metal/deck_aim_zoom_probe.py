#!/usr/bin/env python3
"""Portable zoom/input regression checks against the tracked engine.

python3 deck_aim_zoom_probe.py --engine-source engine \
    --gameplay-source ../../src/gameplay
No native renderer or gameplay execution is claimed by this probe.
"""
import argparse
import os
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parent


def function(source, name, prefix='void DECK_CAMERA::'):
    start = source.index(prefix + name + '(')
    opening = source.index('{', start)
    level = 1
    end = opening + 1
    while level:
        level += (source[end] == '{') - (source[end] == '}')
        end += 1
    return source[start:end]


def verify_gameplay_bindings(gameplay_source):
    binding = 'CI_CreateAndSetControls("Sailing1Pers","DeckAimZoom",CI_GetKeyCode("VK_RBUTTON"),0,false);'
    for relative in ('PROGRAM/controls/init_pc.c', 'PROGRAM/controls/controls.c'):
        source = (gameplay_source / relative).read_text()
        compact = re.sub(r'\s+', '', source)
        assert compact.count(binding) == 1, f'{relative}: fixed zoom binding missing or duplicated'
        if relative.endswith('/controls.c'):
            restore = compact.index('CI_CreateAndSetControls(grName,ctrlName,keyCode,state,true);')
            fixed = compact.index(binding)
            activate = compact.index('RunControlsContainers();', restore)
            assert restore < fixed < activate, 'fixed zoom binding must override saved mappings before activation'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine-source', type=Path, default=ROOT / 'engine')
    parser.add_argument('--gameplay-source', type=Path)
    args = parser.parse_args()
    camera = args.engine_source.resolve() / 'src/libs/sea_cameras/src'
    if not (camera / 'deck_aim_zoom.h').is_file():
        parser.error('pass a complete engine source containing deck_aim_zoom.h')
    source = (camera / 'deck_camera.cpp').read_text()
    with tempfile.TemporaryDirectory(prefix='deck-aim-zoom-') as folder:
        folder = Path(folder)
        compiler = os.environ.get('CXX', 'c++')
        flags = [compiler, '-std=c++20', '-Wall', '-Wextra', '-Werror', '-pedantic', '-I' + str(camera)]
        subprocess.run(flags + [str(ROOT / 'deck_aim_zoom_probe.cpp'), '-o', str(folder / 'zoom')], check=True)
        subprocess.run([str(folder / 'zoom')], check=True)
        lifecycle = folder / 'lifecycle.cpp'
        lifecycle.write_text(r'''
#include "deck_aim_zoom.h"
#include <cassert>
#include <chrono>
#include <cstdint>
#include <iostream>
#include <map>
#include <string>
struct CVECTOR { float x=2.f,y=3.f,z=4.f; };
enum { CST_INACTIVE, CST_ACTIVE, CST_ACTIVATED, SEA_EXECUTE };
struct CONTROL_STATE { int state=0; };
struct Controls {
    bool locked=false, down=false;
    bool IsControlLocked(const char *) const { return locked; }
    void GetControlState(const char *, CONTROL_STATE &s) { s.state=down?CST_ACTIVE:CST_INACTIVE; }
};
struct Core {
    struct Controls *Controls=nullptr;
    bool frozen=false; float timeScale=1.f; uint64_t pauseRevision=0;
    float GetTimeScale() const { return timeScale; }
    uint64_t GetScenePauseRevision() const { return pauseRevision; }
    bool IsLayerFrozen(int) const { return frozen; }
    unsigned GetRDeltaTime() const { return 16; }
} core;
struct Attributes {
    std::map<std::string,unsigned> values{{"WalkMode",1},{"ThirdPerson",0},{"TelescopeActive",0}};
    unsigned GetAttributeAsDword(const char *name) { return values[name]; }
};
struct Renderer {
    float fov=1.285f; CVECTOR pos,angles; int writes=0;
    void GetCamera(CVECTOR &p,CVECTOR &a,float &f) { p=pos; a=angles; f=fov; }
    void SetCamera(CVECTOR p,CVECTOR a,float f) { pos=p; angles=a; fov=f; ++writes; }
};
struct DECK_CAMERA {
    deck_aim::Zoom aimZoom;
    std::chrono::steady_clock::time_point zoomLastUpdate{};
    uint64_t zoomPauseRevision=0; float zoomRenderedPerspective=0.f;
    Renderer *RenderService=nullptr; Attributes *AttributesPointer=nullptr;
    bool on=true,active=true,zoomWindowFocused=true;
    bool isOn() const { return on; } bool isActive() const { return active; }
    float GetPerspective() const { return 1.285f; }
    void ResetAimZoomProjection(); void UpdateAimZoom();
};
''' + function(source, 'ResetAimZoomProjection') + '\n' + function(source, 'UpdateAimZoom') + r'''
void zoomIn(DECK_CAMERA &c,Renderer &r) {
    c.aimZoom.reset(); c.aimZoom.update(0,false); c.aimZoom.update(0,true);
    c.aimZoom.update(.05f,false); for(int i=0;i<8;++i)c.aimZoom.update(.1f,false);
    r.fov=c.aimZoom.perspective(c.GetPerspective()); c.zoomRenderedPerspective=r.fov; r.writes=0;
}
int main() {
    Controls input; core.Controls=&input; Renderer renderer; Attributes attr;
    DECK_CAMERA c; c.RenderService=&renderer; c.AttributesPointer=&attr;
    zoomIn(c,renderer); c.ResetAimZoomProjection();
    assert(renderer.writes==1 && renderer.fov==c.GetPerspective());
    assert(renderer.pos.x==2.f && renderer.angles.z==4.f && c.aimZoom.magnification()==1.f);
    zoomIn(c,renderer); renderer.fov=.8f; c.ResetAimZoomProjection();
    assert(renderer.writes==0 && renderer.fov==.8f && c.aimZoom.magnification()==1.f);
    zoomIn(c,renderer); c.on=false; c.ResetAimZoomProjection(); assert(renderer.writes==0); c.on=true;
    zoomIn(c,renderer); c.active=false; c.ResetAimZoomProjection(); assert(renderer.writes==0); c.active=true;
    for(const auto &name : {"ThirdPerson","TelescopeActive"}) {
        zoomIn(c,renderer); attr.values[name]=1; c.UpdateAimZoom();
        assert(c.aimZoom.magnification()==1.f); attr.values[name]=0;
    }
    zoomIn(c,renderer); attr.values["WalkMode"]=0; c.UpdateAimZoom(); assert(c.aimZoom.magnification()==1.f); attr.values["WalkMode"]=1;
    zoomIn(c,renderer); core.frozen=true; c.UpdateAimZoom(); assert(c.aimZoom.magnification()==1.f); core.frozen=false;
    zoomIn(c,renderer); input.locked=true; input.down=true; c.UpdateAimZoom();
    input.locked=false; c.UpdateAimZoom(); input.down=false; c.UpdateAimZoom();
    assert(c.aimZoom.magnification()==1.f);
    zoomIn(c,renderer); c.zoomWindowFocused=false; c.UpdateAimZoom(); assert(c.aimZoom.magnification()==1.f); c.zoomWindowFocused=true;
    zoomIn(c,renderer); core.timeScale=0.f; c.UpdateAimZoom(); assert(c.aimZoom.magnification()==1.f); core.timeScale=1.f;
    zoomIn(c,renderer); ++core.pauseRevision; input.down=true; c.UpdateAimZoom();
    input.down=false; c.UpdateAimZoom(); assert(c.aimZoom.magnification()==1.f);
    zoomIn(c,renderer); core.Controls=nullptr; c.UpdateAimZoom(); assert(c.aimZoom.magnification()==1.f);
    std::cout << "Camera mode/control/focus/pause gates and projection ownership checks passed\n";
}
''')
        subprocess.run(flags + [str(lifecycle), '-o', str(folder / 'lifecycle')], check=True)
        subprocess.run([str(folder / 'lifecycle')], check=True)
        renderer_source = (args.engine_source / 'src/libs/renderer/src/s_device.cpp').read_text()
        (folder / 'renderer_projection_methods.inc').write_text(
            function(renderer_source, 'SetPerspective', 'bool DX9RENDER::') + '\n' +
            function(renderer_source, 'SetNearFarPlane', 'void DX9RENDER::'))
        subprocess.run(flags + ['-I' + str(folder), str(ROOT / 'deck_aim_projection_probe.cpp'),
                                '-o', str(folder / 'projection')], check=True)
        subprocess.run([str(folder / 'projection')], check=True)
    if args.gameplay_source:
        verify_gameplay_bindings(args.gameplay_source)
        print('Direct startup/restored fixed zoom bindings passed')
    else:
        print('Script bindings not checked: pass --gameplay-source src/gameplay')


if __name__ == '__main__':
    main()
