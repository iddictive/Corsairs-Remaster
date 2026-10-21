#!/usr/bin/env python3
"""Run deck HUD state transitions in the isolated Metal script VM."""
from pathlib import Path
import shutil
import subprocess
import tempfile

import custody_life_probe as vm
import metal_deck_camera as layer

FIXTURE = r'''
void DeckCamera_Probe()
{
 makeref(pchar, Characters[0]); nMainCharacterIndex = 0;
 LAi_SetHP(pchar, 100, 100);
 makearef(Crosshair, SeaCameras.Crosshair);
 locCameraEnableFree = false;
 bCanSwitchCameras = true;
 SeaCameras.Camera = "SeaShipCamera";
 SeaCameras_Switch();
 if (SeaCameras.Camera != "SeaDeckCamera" || sti(SeaDeckCamera.ThirdPerson) != 1 || !sti(Crosshair.OutsideCamera)) Trace("PROBE_FAILED third person reticle");
 SetEventHandler(TELESCOPE_ACTIVE, "SeaCameras_TelescopeActive", 0);
 Event(TELESCOPE_ACTIVE, "l", 1);
 if (bCanSwitchCameras || !sti(Crosshair.OutsideCamera)) Trace("PROBE_FAILED telescope enter");
 Event(TELESCOPE_ACTIVE, "l", 0);
 if (!bCanSwitchCameras || !sti(Crosshair.OutsideCamera)) Trace("PROBE_FAILED telescope third person return");
 SeaCameras_Switch();
 if (SeaCameras.Camera != "SeaDeckCamera" || sti(SeaDeckCamera.ThirdPerson) != 0 || sti(Crosshair.OutsideCamera)) Trace("PROBE_FAILED first person reticle");
 Event(TELESCOPE_ACTIVE, "l", 1);
 Event(TELESCOPE_ACTIVE, "l", 0);
 if (!bCanSwitchCameras || sti(Crosshair.OutsideCamera)) Trace("PROBE_FAILED telescope first person return");
 SeaCameras_Switch();
 if (SeaCameras.Camera != "SeaShipCamera" || !sti(Crosshair.OutsideCamera)) Trace("PROBE_FAILED outside reticle");
 Trace("DECK_CAMERA_PROBE_FINISHED");
 ExitProgram();
}
'''


def main():
    current = (vm.RUNTIME / layer.PATH).read_bytes()
    source = layer.strip(current)
    updated = layer.prepare(source)
    assert layer.prepare(updated) == updated and layer.strip(updated) == source
    try:
        layer.prepare(source + b"unreviewed")
    except RuntimeError:
        pass
    else:
        raise AssertionError("unknown camera script accepted")
    root = Path(tempfile.mkdtemp(prefix="corsairs-deck-camera-probe."))
    try:
        shutil.copytree(vm.RUNTIME / "PROGRAM", root / "PROGRAM")
        (root / "RESOURCE").symlink_to(vm.RUNTIME / "RESOURCE", target_is_directory=True)
        (root / layer.PATH).write_bytes(updated)
        (root / "engine.ini").write_text("full_screen = 0\nwindow_borders = 1\nscreen_x = 320\nscreen_y = 240\nrun_in_background = 1\nSteam = 0\nprogram_directory = PROGRAM\nprogram_bin_directory = PROGRAM_BIN\nrun = seadogs.c\nmax_fps = 30\n[script]\nruntimelog = 1\ntracefiles = 1\n[compatibility]\ntarget_version = teho\n")
        # Keep the real state machine; replace only the engine camera-entity send.
        vm.replace_body(root, "sea_ai/AICameras.c", "SeaCameras_UpdateCamera", "return;")
        sea = root / "PROGRAM/sea_ai/sea.c"
        sea.write_text(sea.read_text() + FIXTURE)
        vm.replace_body(root, "seadogs.c", "Main", r'''
 InterfaceStates.Launched = false;
 DeckCamera_Probe();
''')
        log = vm.run_engine(root)
        assert "DECK_CAMERA_PROBE_FINISHED" in log and "PROBE_FAILED" not in log, log
        print("PASS: real VM deck third/first/outside reticle and telescope return transitions")
    finally:
        subprocess.run(["/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp", str(root.resolve())], check=True)


if __name__ == "__main__":
    main()
