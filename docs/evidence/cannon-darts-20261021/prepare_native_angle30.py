#!/usr/bin/env python3
"""Prepare final30 task fixtures only; never compile, launch or replace old inputs."""
import hashlib
import json
import math
from pathlib import Path
import re
import struct
import sys

ROOT=Path(__file__).resolve().parents[3]
HERE=Path(__file__).resolve().parent
sys.path.insert(0,str(ROOT/"tools"))
from run_sea_benchmark import function
from prepare_native_range import read

def main():
    program=ROOT/"src/gameplay/PROGRAM"
    paths={"balls":program/"sea_ai/AIBalls.c","goods":program/"store/initGoods.c","perks":program/"interface/perks/perks.c","controller":program/"sea_ai/AICannon.c","ships":program/"sea_ai/AIShip.c","cannons":Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/PROGRAM/CANNONS/cannons_init.c"),"calibration":HERE/"calibration.json"}
    texts={key:read(path) for key,path in paths.items()}
    calibration=json.loads(texts["calibration"])
    model=Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/RESOURCE/MODELS/Ships/Manowar1/Manowar1.gm")
    assert hashlib.sha256(model.read_bytes()).hexdigest()==calibration["model_sha256"]
    source='''#define GOOD_BALLS 0
#define GOOD_KNIPPELS 2
#define GOOD_DARTS 52
#define GOODS_QUANTITY 53
#define TRADE_TYPE_AMMUNITION 4
#define CANNON_TYPE_NONECANNON -1
object Goods[53], Cannon[16];
int iArcadeSails=0;
ref GetGoodByType(int index) { return &Goods[index]; }
ref GetCannonByType(int index) { return &Cannon[index]; }
ref GetCharacter(int index) { return &Fleet[index]; }
'''
    for key,signature in (("perks","bool CheckCharacterPerk(ref chref, string perkName)"),("goods","void Darts_InitGoods()"),("balls","float Ball_GetHeightMultiply(aref aCharacter)"),("balls","float Ball_GetAccuracy(aref aCharacter)"),("ships","float AIShip_isPerksUse(string sPerk, float fOff, float fOn)"),("controller","void Cannon_RecalculateParameters(int iCharacterIndex)"),("cannons","void InitBaseCannons_CalcSpeedV0(ref rCannon, float fFireRange)")):
        body=function(texts[key],signature)
        if key=="controller":
            # Storm #define stores scalar constants, not C identifier aliases.
            assert body.count("Characters[iCharacterIndex]")==1
            body=body.replace("Characters[iCharacterIndex]","Fleet[iCharacterIndex]")
        source+=body+"\n"
    launch=function(texts["balls"],"void Ball_AddBall(")
    assert launch.count("\tfloat fCannons = ")==1 and launch.count("\tfloat fK = ")==1
    launch=launch[:launch.index("\tfloat fCannons = ")]+launch[launch.index("\tfloat fK = "):]
    assert launch.count('\tAIBalls.Event = "";')==1
    launch=launch[:launch.index('\tAIBalls.Event = "";')]+'\tAIBalls.Event="@dart:52";\n\tAIBalls.Type="Balls";\n\tEntityUpdate(1);\n\tAIBalls.Add="";\n}\n'
    source+='// Actual source scatter/transform; misfire and postlaunch FX omitted.\n'+launch+'\nvoid InitRangeSource()\n{\n    Darts_InitGoods();\n'
    for index,fire_range in ((14,800),(15,1600)):
        block=texts["cannons"].split(f"makeref(rCannon,Cannon[CANNON_TYPE_CANNON_LBS{42 if index==14 else 48}]);",1)[1].split("makeref(",1)[0]
        for field in ("caliber","FireAngMax","FireAngMin","TimeSpeedMultiply","SizeMultiply","HeightMultiply"):
            values=re.findall(r"rCannon\."+field+r"\s*=\s*([^;]+);",block);assert len(values)==1
            source+=f"    Cannon[{index}].{field}={values[0].strip()};\n"
        source+=f"    InitBaseCannons_CalcSpeedV0(&Cannon[{index}],{fire_range}.0);\n"
    source+='}\n'
    (HERE/"native-angle30-source.c").write_text(source)
    # Lowest real muzzle reaching the upper target band, with 2m finite margin.
    ranges=[]
    for maximum in (2116.,4232.):
        limits=[]
        for ordinal,(x,y,z) in enumerate(calibration["guns"]):
            targetz=-35.790977478+ordinal*(69.490001679/50.)
            limits.append(x+math.sqrt(maximum**2-2*maximum*(9.245-y)/.5773502692-(targetz-z)**2))
        ranges.append(min(limits)-2.)
    for mode in ("angle","range"):
        fixture=(HERE/"native-dart-contract.c").read_text().split('    CreateEntity(&BenchAI, "sea_ai");',1)[0]
        start=fixture.index("object BenchSeaSave, BenchCameraData;");end=fixture.index("object Sail, Rope, Flag",start)
        fixture=fixture[:start]+'''object BenchSeaSave, BenchCameraData;
int nMainCharacterIndex=999;
int RangeHits0=0, RangeHits1=0, RangeBad=0;
int AngleMode=0, AngleFires=0, AngleBad=0, AngleLoaded=0;
'''+fixture[end:]
        fixture=fixture.replace('native void BenchmarkSeed(int seed);','''native void BenchmarkSeed(int seed);
native int DartNativeQueue(aref actor, int gunOrdinal, float velocity, float targetX, float targetY, float targetZ, float rawBeta, int codec);
native void DartNativeSample();
native void DartNativeLegacy();
#libriary "DartNativeAcceptanceLibrary"''')
        fixture=fixture.replace('#include "day.c"','#include "day.c"\n#include "native-angle30-source.c"')
        fixture=fixture.replace("Fleet[i].ship.ang.y = 0.1;","Fleet[i].ship.ang.y = 0.0;").replace("Fleet[i].ship.speed.z = 4.0;","Fleet[i].ship.speed.z = 0.0;")
        fixture=fixture.replace("        Fleet[i].index = i;","        Fleet[i].index = i;\n        if (i==2) Fleet[i].index=1000;\n        if (i==3) Fleet[i].index=1001;")
        if mode=="range":
            fixture=re.sub(r"        Fleet\[i\]\.ship\.pos\.x = [^;]+;",lambda _:f"        Fleet[i].ship.pos.x=-3000.0-i*100.0;\n        if (i==0) Fleet[i].ship.pos.x={ranges[0]:.9f};\n        if (i==1) Fleet[i].ship.pos.x={ranges[1]:.9f};\n        if (i==2 || i==3) Fleet[i].ship.pos.x=0.0;",fixture)
            fixture=re.sub(r"        Fleet\[i\]\.ship\.pos\.z = [^;]+;",lambda _:"        Fleet[i].ship.pos.z=0.0;\n        if (i==1 || i==3) Fleet[i].ship.pos.z=200.0;",fixture)
        else:
            fixture=re.sub(r"        Fleet\[i\]\.ship\.pos\.x = [^;]+;",lambda m:m[0]+"\n        if (i==2) Fleet[i].ship.pos.x=-6000.0;",fixture)
            fixture=re.sub(r"        Fleet\[i\]\.ship\.pos\.z = [^;]+;",lambda m:m[0]+"\n        if (i==2) Fleet[i].ship.pos.z=0.0;",fixture)
        setup=(HERE/"native-range-tail.c").read_text().split('    SetEventHandler("frame"',1)[0]
        fixture+=setup+'''    SetEventHandler("frame","FinalFrame",0);
    SetEventHandler("CannLoad","FinalLoad",0);
    SetEventHandler("CannGRTime","FinalReload",0);
    SetEventHandler("CannFire","FinalFire",0);
    SetEventHandler("SeaLoad_GetPointer","FinalPointer",0);
    SetEventHandler("Shp_HullHit","FinalHull",0);
    SetEventHandler("ShpMastDamage","FinalNoDamage",0);
    SetEventHandler("ShpHullDamage","FinalNoDamage",0);
    BenchmarkReady=1;
}
int FinalLoad() { return 1; }
float FinalReload() { return 1.0; }
float FinalNoDamage() { return 0.0; }
ref FinalPointer() { string kind=GetEventData(); int index=GetEventData(); return &BenchSeaSave; }
void FinalProfile(int actor,int kind)
{
    Fleet[actor].Ship.Cannons.Type=kind+14;
    Fleet[actor].Ship.Cannons.Charge.Type=52;
    Fleet[actor].TmpSkill.Accuracy=1.0;
    Fleet[actor].TmpSkill.Cannons=1.0;
    Fleet[actor].perks.list.GunProfessional=1;
    Fleet[actor].perks.list.LongRangeShoot=1;
    Fleet[actor].TmpPerks.LongRangeShoot=1;
    Cannon_RecalculateParameters(actor);
    if (sti(Fleet[actor].Ship.Cannons.FixedBallisticFrame)!=1 || abs(stf(Fleet[actor].Ship.Cannons.BallHeightMultiply)-0.5773502692)>0.00001 || abs(stf(Fleet[actor].Ship.Cannons.BallTimeSpeedMultiply)-2.0)>0.00001) RangeBad++;
}
void FinalFire()
{
    aref actor=GetEventData();
    float x=GetEventData();
    float y=GetEventData();
    float z=GetEventData();
    float velocity=GetEventData();
    float yaw=GetEventData();
    float beta=GetEventData();
    float bort=GetEventData();
    float limit=GetEventData();
    float raw=GetEventData();
    if (AngleMode) {
        AngleFires++;
        float degrees=20.0;
        if (AngleFires==2) degrees=30.0;
        if (AngleFires==3) degrees=40.0;
        if (AngleFires==4) degrees=60.0;
        if (AngleFires>=5) degrees=80.0;
        float expected=atan(tan(Degree2Radian(degrees))/0.5773502692);
        if (abs(beta-expected)>0.00001 || abs(raw)>0.000001) AngleBad++;
        if (AngleFires==6 && sti(AIBalls.TargetValid)==0) AngleLoaded=1;
        AIBalls.Type="Balls"; AIBalls.Event="@dart:52";
        AIBalls.CharacterIndex=actor.index;
        AIBalls.x=x; AIBalls.y=y; AIBalls.z=z;
        AIBalls.SpdV0=velocity; AIBalls.Dir=yaw; AIBalls.Ang=beta;
        AIBalls.RawAng=raw;
        AIBalls.HeightMultiply=Ball_GetHeightMultiply(actor);
        AIBalls.SizeMultiply=1.0; AIBalls.TimeSpeedMultiply=2.0;
        AIBalls.CannonType=actor.Ship.Cannons.Type;
        AIBalls.MaxFireDistance=10000.0;
        AIBalls.Add=1;
    } else Ball_AddBall(actor,x,y,z,velocity,yaw,beta,bort,limit,raw);
}
void FinalHull()
{
    aref nativeShip=GetEventData(); int owner=GetEventData(); int receiver=GetEventData();
    if (sti(AIBalls.CurrentBallType)!=52) RangeBad++;
    if (owner==1000 && receiver==0) RangeHits0++;
    else if (owner==1001 && receiver==1) RangeHits1++;
    else RangeBad++;
}
'''
        fixture+=(HERE/("native-angle30-"+mode+"-tail.c")).read_text()
        (HERE/("native-angle30-"+mode+".c")).write_text(fixture)
    # Independent prior-format seed: all fields in the frozen pre-edit Save order.
    prior=ROOT/".local-archives/cannon-darts-20261021/baseline/ai_balls.cpp"
    oldsave=function(read(prior),"void BALL_PARAMS::Save(CSaveLoad *pSL)")
    currentsave=function(read(ROOT/"experiments/native-metal/engine/src/libs/sea_ai/src/ai_balls.cpp"),"void BALL_PARAMS::Save(CSaveLoad *pSL)")
    assert oldsave==currentsave
    shot=struct.pack("<6fQI i 11f I",-210.,5.,45.,-210.,5.,45.,0,0,999,0.,200.,1.,0.,0.,1.,.4,1.,1.,2000.,0.,0)
    payload=struct.pack("<III",0,0,1)+shot+struct.pack("<I",0)
    (HERE/"native-angle30-legacy-balls.hex").write_text(f"{len(payload):08x}"+payload.hex())
    functional=(HERE/"native-dart-contract.c").read_text()
    functional=functional.replace('native void BenchmarkSeed(int seed);','''native void BenchmarkSeed(int seed);
native int DartNativeQueue(aref actor, int gunOrdinal, float velocity, float targetX, float targetY, float targetZ, float rawBeta, int codec);
native void DartNativeSample();
native void DartNativeLegacy();
#libriary "DartNativeAcceptanceLibrary"''')
    assert functional.count('DartContractFire("Balls", "");')==1
    functional=functional.replace('DartContractFire("Balls", "");','AIBalls.clear=1;\n        DartNativeLegacy();')
    (HERE/"native-angle30-functional.c").write_text(functional)
    manifest={"inputs":{key:{"path":str(path.relative_to(ROOT)) if path.is_relative_to(ROOT) else str(path),"sha256":hashlib.sha256(path.read_bytes()).hexdigest()} for key,path in paths.items()},"model_sha256":calibration["model_sha256"],"ranges_m":ranges,"nominal_master_ranges_m":[2116,4232],"shots_per_range":1020,"acceptance_hits_inclusive":[663,765],"frames":512,"delta_ms":16,"time_scale":1,"angle_degrees":[20,30,40,60,80],"queue_codec_degrees":80,"legacy_seed":{"frozen_save_source_sha256":hashlib.sha256(prior.read_bytes()).hexdigest(),"payload_sha256":hashlib.sha256(payload).hexdigest(),"bytes":len(payload),"origin":"Independently authored prior-format ordinary shot; frozen old and current Save functions byte-identical. Not a player save or historical native save capture."},"claim_ceiling":"Actual AICannon queue/codec, AIBalls kernel and native BSP. Direct queue does not exercise manualcontroller BuildManualAimSolution, reticle UI or installed combat. Range uses actual51GM labels and native low ballistic root, current source launch scatter; no old13.9degree cap."}
    (HERE/"native-angle30-inputs.json").write_text(json.dumps(manifest,indent=2)+"\n")
    print("final30 fixtures prepared offline; no compiler/GPU")

if __name__=="__main__":main()
