#!/usr/bin/env python3
"""Prepare one retained native range fixture from current source and GM evidence."""
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT / "tools"))
from run_sea_benchmark import function

def read(path):
    data = path.read_bytes()
    try:
        return data.decode("utf-8-sig").replace("\r", "")
    except UnicodeDecodeError:
        return data.decode("cp1251").replace("\r", "")

def main():
    program = ROOT / "src/gameplay/PROGRAM"
    cannon_path = Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/PROGRAM/CANNONS/cannons_init.c")
    paths = {"balls":program/"sea_ai/AIBalls.c", "goods":program/"store/initGoods.c", "perks":program/"interface/perks/perks.c", "cannons":cannon_path, "calibration":HERE/"calibration.json"}
    texts = {key:read(path) for key,path in paths.items()}
    calibration = json.loads(texts["calibration"])
    model = Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/MODELS/Ships/Manowar1/Manowar1.gm")
    assert hashlib.sha256(model.read_bytes()).hexdigest() == calibration["model_sha256"]
    selected = [case for case in calibration["cases"] if case["common_ai_bort_delta_m"] == 0]
    assert len(selected) == 2 and len(calibration["guns"]) == 51
    source = """#define GOOD_BALLS 0
#define GOOD_KNIPPELS 2
#define GOOD_DARTS 52
#define GOODS_QUANTITY 53
#define TRADE_TYPE_AMMUNITION 4
object Goods[53], Cannon[16], RangeGun[51];
int iArcadeSails=0;
ref GetGoodByType(int index) { return &Goods[index]; }
ref GetCannonByType(int index) { return &Cannon[index]; }
"""
    for key, signature in (("perks","bool CheckCharacterPerk(ref chref, string perkName)"),("goods","void Darts_InitGoods()"),("balls","float Ball_GetHeightMultiply(aref aCharacter)"),("balls","float Ball_GetAccuracy(aref aCharacter)"),("cannons","void InitBaseCannons_CalcSpeedV0(ref rCannon, float fFireRange)")):
        source += function(texts[key], signature) + "\n"
    launch = function(texts["balls"], "void Ball_AddBall(")
    assert launch.count("\tfloat fCannons = ") == 1 and launch.count("\tfloat fK = ") == 1
    launch = launch[:launch.index("\tfloat fCannons = ")] + launch[launch.index("\tfloat fK = "):]
    assert launch.count('\tAIBalls.Event = "";') == 1
    launch = launch[:launch.index('\tAIBalls.Event = "";')] + '\tAIBalls.Event="@dart:52";\n\tAIBalls.Type="Balls";\n\tEntityUpdate(1);\n\tAIBalls.Add="";\n}\n'
    source += "// Source launch transform and scatter retained verbatim; misfire, muzzle FX/audio omitted.\n" + launch + "\nvoid InitRangeSource()\n{\n    Darts_InitGoods();\n"
    for index, fire_range in ((14,800),(15,1600)):
        block = texts["cannons"].split(f"makeref(rCannon,Cannon[CANNON_TYPE_CANNON_LBS{42 if index == 14 else 48}]);",1)[1].split("makeref(",1)[0]
        for field in ("caliber","FireAngMax","TimeSpeedMultiply","SizeMultiply","HeightMultiply"):
            values = re.findall(r"rCannon\." + field + r"\s*=\s*([^;]+);",block)
            assert len(values)==1
            source += f"    Cannon[{index}].{field}={values[0].strip()};\n"
        source += f"    InitBaseCannons_CalcSpeedV0(&Cannon[{index}],{fire_range}.0);\n"
    for index, point in enumerate(calibration["guns"]):
        for axis,value in zip("xyz",point):
            source += f"    RangeGun[{index}].{axis}={value:.9f};\n"
    source += "}\n"
    (HERE/"native-range-source.c").write_text(source)
    fixture = (HERE/"native-dart-contract.c").read_text().split('    CreateEntity(&BenchAI, "sea_ai");',1)[0]
    start = fixture.index("object BenchSeaSave, BenchCameraData;")
    end = fixture.index("object Sail, Rope, Flag",start)
    fixture = fixture[:start] + "int RangeHits0=0;\nint RangeHits1=0;\nint RangeBad=0;\n" + fixture[end:]
    fixture = fixture.replace('#include "day.c"','#include "day.c"\n#include "native-range-source.c"')
    for axis, replacement in (("x", f"        Fleet[i].ship.pos.x=-3000.0-i*100.0;\n        if (i==0) Fleet[i].ship.pos.x={selected[0]['distance_all_guns_reachable_at_target_band_ceiling_m']:.9f};\n        if (i==1) Fleet[i].ship.pos.x={selected[1]['distance_all_guns_reachable_at_target_band_ceiling_m']:.9f};\n        if (i==2 || i==3) Fleet[i].ship.pos.x=0.0;"),("z", "        Fleet[i].ship.pos.z=0.0;\n        if (i==1 || i==3) Fleet[i].ship.pos.z=200.0;")):
        pattern = r"        Fleet\[i\]\.ship\.pos\." + axis + r" = [^;]+;"
        assert len(re.findall(pattern,fixture)) == 1
        fixture = re.sub(pattern,lambda _:replacement,fixture)
    fixture = fixture.replace("Fleet[i].ship.ang.y = 0.1;", "Fleet[i].ship.ang.y = 0.0;")
    fixture = fixture.replace("Fleet[i].ship.speed.z = 4.0;", "Fleet[i].ship.speed.z = 0.0;")
    fixture = fixture.replace("        Fleet[i].index = i;", "        Fleet[i].index = i;\n        if (i==2) Fleet[i].index=1000;\n        if (i==3) Fleet[i].index=1001;")
    fixture += (HERE/"native-range-tail.c").read_text()
    (HERE/"native-range-contract.c").write_text(fixture)
    manifest = {"inputs":{key:{"path":str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),"sha256":hashlib.sha256(path.read_bytes()).hexdigest()} for key,path in paths.items()},"model_sha256":calibration["model_sha256"],"ranges_m":[case['distance_all_guns_reachable_at_target_band_ceiling_m'] for case in selected],"shots_per_range":1020,"volleys_per_range":20,"acceptance_hits_inclusive":[663,765],"script_source_boundary":"Three full ammo helpers and perk lookup; Ball_AddBall launch transform+dispersion verbatim, omitting misfire branch and postlaunch muzzle smoke/audio; four-lane dart identity assigned as current source. Aim uses AICannon-equivalent low ballistic root. No AI controller/reload timing acceptance."}
    (HERE/"native-range-inputs.json").write_text(json.dumps(manifest,indent=2)+"\n")
    print("range fixture prepared; no native launch")

if __name__ == "__main__":
    main()
