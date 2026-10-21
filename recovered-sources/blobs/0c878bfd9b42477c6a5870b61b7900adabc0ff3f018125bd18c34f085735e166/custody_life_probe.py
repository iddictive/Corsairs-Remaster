#!/usr/bin/env python3
"""Compile custody with Metal; probe state in its script VM with world/renderer stubs.

This does not accept real gameplay or touch the running game's scripts/SAVE.
--negative proves that removing transaction cleanup trips the reward assertions.
"""
import argparse
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile

import patch_gameplay_suite as suite
import patch_custody_life as layer

PROJECT = Path(__file__).resolve().parents[1]
RUNTIME = PROJECT / "experiments/native-metal/.cache/runtime"
ENGINE = PROJECT / "experiments/native-metal/.cache/CorsairsMetal.app/Contents/MacOS/metal-engine"

MAIN = r''' 
 InterfaceStates.Launched = false;
 Trace("CUSTODY_MAIN_COMPILED");
 if (!LoadSegment("scripts\custody.c")) { Trace("PROBE_FAILED compile custody"); ExitProgram(); return; }
 if (!LoadSegment("dialogs\russian\Common_Prison.c")) { Trace("PROBE_FAILED compile dialog"); ExitProgram(); return; }
 Trace("CUSTODY_SEGMENTS_COMPILED");
 makeref(pchar, Characters[0]);
 nMainCharacterIndex = 0;
 pchar.id = "ProbeHero"; pchar.index = 0; pchar.model.animation = "man";
 pchar.location = "TestJail";
 pchar.money = 1000;
 pchar.Custody.Active = true; pchar.Custody.Jail = "TestJail";
 pchar.Custody.City = "TestCity"; pchar.Custody.Standing = 50;
 pchar.Custody.DaysRemaining = 12;
 CustodyLife_Ensure();
 pchar.Custody.Life.Admin = 77;
 CustodyLife_Ensure();
 if (sti(pchar.Custody.Life.Admin) != 77) Trace("PROBE_FAILED migration resets state");
 pchar.Custody.Life.Admin = 30;
 LAi_SetHP(pchar, 60, 100);
 CustodyLife_Action("work");
 if (sti(pchar.Custody.DaysRemaining) != 11 || probeDays != 1 || sti(pchar.money) != 1060 || sti(pchar.Custody.Life.Admin) != 42 || sti(pchar.Custody.Standing) != 45) Trace("PROBE_FAILED work");
 pchar.money = 0;
 CustodyLife_Action("food");
 CustodyLife_Action("dice");
 if (probeDays != 1 || sti(pchar.money) != 0) Trace("PROBE_FAILED poor action consumes day");
 CustodyLife_Action("train");
 if (probeExp != 50 || probeDays != 2) Trace("PROBE_FAILED training");
 CustodyLife_Action("quiet");
 if (LAi_GetCharacterHP(pchar) != 77.0 || probeDays != 3) Trace("PROBE_FAILED rest");
 pchar.Custody.Bout.Phase = "fighting";
 CustodyLife_Action("work");
 if (probeDays != 3) Trace("PROBE_FAILED day action during bout");
 DeleteAttribute(pchar, "Custody.Bout");
 pchar.location = "Elsewhere";
 CustodyLife_Action("work");
 if (probeDays != 3) Trace("PROBE_FAILED action outside cell");
 pchar.location = "TestJail";
 pchar.Custody.Life.Admin = 95;
 pchar.Custody.Life.Guards = 60;
 CustodyLife_Action("inform");
 if (sti(pchar.Custody.DaysRemaining) != 0 || sti(pchar.Custody.Life.Admin) != 100 || probeDays != 4) Trace("PROBE_FAILED parole");
 CustodyLife_Action("work");
 if (probeDays != 4) Trace("PROBE_FAILED expired sentence action");
 pchar.Custody.DaysRemaining = 5;
 pchar.Custody.Life.Admin = 20;
 pchar.Custody.Life.Wins = 2;
 pchar.Custody.Bout.Phase = "returning";
 pchar.Custody.Bout.Won = true;
 pchar.Custody.Bout.Blade = "";
 CustodyLife_Returned();
 CustodyLife_Returned();
 if (sti(pchar.money) != 100 || sti(pchar.Custody.Life.Wins) != 3 || probeDays != 5 || CheckAttribute(pchar, "Custody.Bout") || sti(pchar.Custody.DaysRemaining) != 0) Trace("PROBE_FAILED duplicate victory");
 pchar.Custody.DaysRemaining = 4;
 pchar.Custody.Life.Wins = 1;
 pchar.Custody.Bout.Phase = "returning";
 pchar.Custody.Bout.Won = false;
 pchar.Custody.Bout.Blade = "";
 LAi_SetHP(pchar,20,100);
 CustodyLife_Returned();
 if (sti(pchar.Custody.Life.Wins) != 1 || sti(pchar.money) != 100 || LAi_GetCharacterHP(pchar) != 30 || probeDays != 6) Trace("PROBE_FAILED defeat");
 pchar.Custody.Bout.Phase = "returning";
 pchar.Custody.Bout.Blade = "";
 pchar.Custody.Bout.Immortal = true;
 CustodyLife_Returned();
 if (probeDays != 6 || !sti(pchar.chr_ai.immortal)) Trace("PROBE_FAILED cancelled bout or prior immunity");
 if (CustodyLife_Clamp(-1) != 0 || CustodyLife_Clamp(110) != 100 || CustodyLife_Clamp(70) != 70) Trace("PROBE_FAILED clamp");
 Items[0].id = "blade4"; Items[0].groupID = BLADE_ITEM_TYPE; Items[0].price = 10;
 Items[1].id = "blade5"; Items[1].groupID = BLADE_ITEM_TYPE; Items[1].price = 10;
 pchar.items.blade4 = 3; pchar.items.blade5 = 1;
 pchar.equip.blade = "blade4";
 pchar.Custody.Bout.Phase = "returning";
 pchar.Custody.Bout.IssuedBlade = true;
 pchar.Custody.Bout.Blade = "blade5";
 CustodyLife_Returned();
 CustodyLife_Returned();
 if (GetCharacterItem(pchar, "blade4") != 2 || GetCharacterEquipByGroup(pchar, BLADE_ITEM_TYPE) != "blade5" || CheckAttribute(pchar, "chr_ai.immortal")) Trace("PROBE_FAILED weapon escrow or immunity cleanup");
 nLocationsNum = 2;
 Locations[0].id = "FortFrance_fort";
 Locations[0].reload.l1.go = "Town";
 Locations[0].soldiers = true;
 Locations[0].models.always.fortV = "fortVRight";
 Locations[1].id = "TestJail";
 int arena = CustodyLife_Yard();
 if (arena != 2 || nLocationsNum != 3 || CheckAttribute(&Locations[arena], "reload") || !CheckAttribute(&Locations[arena], "DisableOfficers") || Locations[0].reload.l1.go != "Town") Trace("PROBE_FAILED arena isolation");
 if (CustodyLife_Yard() != 2 || nLocationsNum != 3) Trace("PROBE_FAILED duplicate arena");
 pchar.location = "CustodyYard";
 pchar.Custody.Bout.Phase = "fighting";
 pchar.Custody.Bout.Won = true;
 pchar.Custody.Bout.Blade = "";
 if (!CustodyLife_Recover(&Locations[2]) || pchar.Custody.Bout.Phase != "returning" || CheckAttribute(pchar, "Custody.Bout.Won")) Trace("PROBE_FAILED loaded arena recovery");
 pchar.location = "TestJail";
 CustodyLife_Returned();
 if (probeDays != 6 || sti(pchar.money) != 100) Trace("PROBE_FAILED recovery reward");
 LAi_SetHP(pchar, 100, 100);
 LAi_SetCheckMinHP(pchar, 20, true, "CustodyLife_Lost");
 pchar.chr_ai.hp = 0;
 LAi_ProcessCheckMinHP(pchar);
 if (LAi_GetCharacterHP(pchar) != 20 || !sti(pchar.chr_ai.immortal) || CheckAttribute(pchar, "chr_ai.hpchecker")) Trace("PROBE_FAILED lethal blow not intercepted");
 Trace("CUSTODY_STATE_PROBE_FINISHED");
 ExitProgram();
'''

STUBS = [
 ('quests/quests.c','WaitDate','probeDays = probeDays + day;'),
 ('battle_interface/reload_tables.c','RecalculateJumpTable',''),
 ('weather/WhrWeather.c','Whr_UpdateWeather',''),
 ('scripts/custody.c','Custody_ApplyLocks','probeLocks = probeLocks + 1;'),
 ('scripts/custody.c','Custody_UpdateJournal',''),
 ('scripts/custody.c','CustodyLife_RemoveActors',''),
 ('scripts/custody.c','Custody_RehydrateJail',''),
 ('characters/characterUtilite.c','RemoveCharacterEquip','DeleteAttribute(chref, "equip." + groupID);'),
 ('characters/characterUtilite.c','AddMoneyToCharacter','_refCharacter.money = sti(_refCharacter.money) + _Money; return sti(_refCharacter.money);'),
 ('characters/RPGUtilite.c','AddCharacterExpToSkill','probeExp = probeExp + sti(_addValue);'),
 ('utils.c','Log_Info',''),
]


def replace_body(root, path, name, new):
    p = root / "PROGRAM" / path
    source = p.read_text()
    match = re.search(r'\b(?:void|int|bool|string|float) ' + name + r'\([^)]*\)(?:\s|//[^\n]*\n)*\{', source, re.I)
    if not match:
        raise ValueError(f"missing fixture boundary {path}:{name}")
    start = match.end()
    end, depth = start, 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    p.write_text(source[:start] + "\n" + new + "\n" + source[end-1:])


def run_engine(root):
    env = os.environ.copy()
    env["STORM_USERDATA"] = str(root / "userdata")
    env["DYLD_LIBRARY_PATH"] = str(PROJECT / "experiments/native-storm/.cache/d3d9/sdl2-fixed/lib") + ":/opt/homebrew/lib"
    result = subprocess.run([str(ENGINE)], cwd=root, env=env, capture_output=True, timeout=30)
    logs = {name: next(root.rglob(name)).read_text(errors="replace") for name in ("compile.log", "error.log")}
    if result.returncode or logs["error.log"]:
        raise RuntimeError(f"Metal script probe failed: {result.returncode}\n{logs}")
    return logs["compile.log"]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--negative", action="store_true")
    args = parser.parse_args()
    state = suite.classify(suite.NATIVE_ROOT)[0]
    original = suite._build_outputs(suite._read_originals(suite.NATIVE_ROOT, state))
    outputs = layer.transform(original)
    assert all(outputs[p] == value for p, value in original.items() if p not in layer.BASE)
    for path in layer.BASE:
        bad = dict(original)
        bad[path] += b"unknown revision"
        try:
            layer.transform(bad)
        except ValueError:
            pass
        else:
            raise AssertionError(f"accepted unknown input: {path}")
    root = Path(tempfile.mkdtemp(prefix="corsairs-custody-probe."))
    try:
        shutil.copytree(RUNTIME / "PROGRAM", root / "PROGRAM")
        (root / "RESOURCE").symlink_to(RUNTIME / "RESOURCE", target_is_directory=True)
        for path in layer.BASE:
            (root / path).write_bytes(outputs[path])
        (root / "engine.ini").write_text("full_screen = 0\nwindow_borders = 1\nscreen_x = 320\nscreen_y = 240\nrun_in_background = 1\nSteam = 0\nprogram_directory = PROGRAM\nprogram_bin_directory = PROGRAM_BIN\nrun = seadogs.c\nmax_fps = 30\n[script]\nruntimelog = 1\ntracefiles = 1\n[compatibility]\ntarget_version = teho\n")
        replace_body(root, "seadogs.c", "Main", r'''
 InterfaceStates.Launched = false;
 if (!LoadSegment("scripts\custody.c")) Trace("PROBE_FAILED custody compile");
 if (!LoadSegment("dialogs\russian\Common_Prison.c")) Trace("PROBE_FAILED dialog compile");
 Trace("CUSTODY_EXACT_COMPILE_FINISHED");
 ExitProgram();
''')
        compiled = run_engine(root)
        assert "CUSTODY_EXACT_COMPILE_FINISHED" in compiled and "PROBE_FAILED" not in compiled, compiled
        for path, name, replacement in STUBS:
            replace_body(root, path, name, replacement)
        replace_body(root, "seadogs.c", "Main", MAIN)
        entry = root / "PROGRAM/seadogs.c"
        entry.write_text("int probeDays;\nint probeLocks;\nint probeExp;\nextern int CustodyLife_Clamp(int value);\nextern int CustodyLife_Yard();\nextern bool CustodyLife_Recover(aref loc);\n" + entry.read_text())
        if args.negative:
            changed = root / "PROGRAM/scripts/custody.c"
            value = changed.read_text()
            anchor = '\tDeleteAttribute(pchar, "Custody.Bout");\n\tif (scored)'
            assert value.count(anchor) == 1
            changed.write_text(value.replace(anchor, "\tif (scored)", 1))
        observed = run_engine(root)
        assert "CUSTODY_STATE_PROBE_FINISHED" in observed, observed
        if args.negative:
            assert "PROBE_FAILED duplicate victory" in observed, observed
            print("PASS: missing bout cleanup rejected by reward/day assertions")
        else:
            assert "PROBE_FAILED" not in observed, observed
            print("PASS: exact script compilation; state migration, day costs, funds, parole, reward idempotence, defeat, cancellation, weapon quantity, immunity cleanup, isolated arena")
    finally:
        subprocess.run(["/REQUIRED_EXTERNAL_INPUT/.codex/bin/codex-delete-temp", str(root.resolve())], check=True)


if __name__ == "__main__":
    main()
