#!/usr/bin/env python3
"""Evaluate actual native position CSV against independent closed-form physics."""
import csv
import json
import math
from pathlib import Path
import sys

HERE=Path(__file__).resolve().parent

def evaluate(path):
    result=json.loads((Path(path).parent/"result.json").read_text())
    assert result["returncode"]==0 and "validation_error" not in result
    counts=result["native_counters"]
    assert counts["RangeBad"]==0 and counts["AngleFires"]==6 and counts["AngleBad"]==0 and counts["AngleLoaded"]==1
    rows=list(csv.DictReader(Path(path).open()))
    assert rows,"missing actual native trajectory rows"
    cases=[]
    for degrees in (20,30,40,60,80):
        beta=math.atan(math.tan(math.radians(degrees))/.5773502692)
        selected=[{key:float(value) for key,value in row.items() if key!="event"} for row in rows if abs(math.atan2(float(row["sin_beta"]),float(row["cos_beta"]))-beta)<1e-5]
        assert selected,f"missing native {degrees}degree shot"
        first=min(selected,key=lambda r:r["time"])
        last=max(selected,key=lambda r:r["time"])
        velocity=first["speed"];height=first["height_scale"]
        # Independent oracle: fixed vertical frame, constant9.81m/s² gravity.
        vx=velocity*math.cos(beta);vy=height*velocity*math.sin(beta)
        peak=first["y0"]+vy*vy/(2*9.81*height)
        actual_peak=max(row["y"] for row in selected)
        actual_range=math.hypot(last["x"]-last["x0"],last["z"]-last["z0"])
        times=sorted({row["time"] for row in selected})
        dt=min(b-a for a,b in zip(times,times[1:]) if b>a)
        residual=0.
        for row in selected:
            expected_y=row["y0"]+vy*row["time"]-.5*9.81*height*row["time"]**2
            horizontal=math.hypot(row["x"]-row["x0"],row["z"]-row["z0"])
            residual=max(residual,abs(row["y"]-expected_y),abs(horizontal-vx*row["time"]))
            assert abs(row["raw_angle"])<1e-6
        assert residual<.02,(degrees,"native position residual",residual)
        assert abs(actual_peak-peak)<.02,(degrees,"native peak",actual_peak,peak)
        cases.append({"visible_launch_degrees":degrees,"raw_launch_degrees":math.degrees(beta),"native_samples":len(selected),"native_peak_y_m":actual_peak,"independent_peak_y_m":peak,"max_native_position_residual_m":residual,"native_last_pre_water_range_m":actual_range,"native_sea_contact_range_interval_m":[actual_range,actual_range+vx*dt],"actual_physical_sample_step_seconds":dt,"height_scale":height,"raw_angle":first["raw_angle"]})
    assert cases[1]["native_last_pre_water_range_m"]>max(case["native_last_pre_water_range_m"] for index,case in enumerate(cases) if index!=1)
    return {"accepted":True,"claim_ceiling":"Native AIBalls samples and sea-contact range bracket after direct AICannon queue; no manualcontroller/UI acceptance. Native peaks, range ordering and full flight independently checked, not math substituted for native.","cases":cases}

if __name__=="__main__":
    source=Path(sys.argv[1])
    result=evaluate(source)
    (source.parent/"angle-verdict.json").write_text(json.dumps(result,indent=2)+"\n")
    print(json.dumps(result,indent=2))
