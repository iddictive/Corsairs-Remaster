from pathlib import Path
import sys

root = Path(__file__).resolve().parent
source = (root / ".cache/storm/src/libs/ship/src/ship_lights.cpp").read_text()
patch = (root / "ship-point-shadow.patch").read_text()

def need(value: bool, label: str) -> None:
    if not value:
        raise SystemExit(f"FAIL: {label}")

need("aSelectedLights.insert(aSelectedLights.begin(), SelectedLight{aLights[i].fCurDistance, i});" in source,
     "own active lamps retain the distance-bounded selected-slot path")
need("aLights[i].pObject == pObject && !aLights[i].bCoronaOnly" in source,
     "own-slot admission still excludes corona-only lights")
need("StormMetalSetLightIdentity(pRS->GetD3DDevice(), i + 1, pL->metalId);" in patch,
     "selected D3D slots receive stable ship lamp identities")
need("StormMetalSetLightIdentity(pRS->GetD3DDevice(), unsigned(i), 0);" in patch,
     "disabled slots clear their shadow identity")
print("PASS own ship lamps retain selected D3D slots with stable shadow identities")
