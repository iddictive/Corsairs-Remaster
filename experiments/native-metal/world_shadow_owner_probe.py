#!/usr/bin/env python3
"""Source contract companion for the behavioral Metal registry probe."""
from pathlib import Path
root=Path(__file__).resolve().parent
b=(root/'backend.mm').read_text(); h=(root/'land_shadow.hpp').read_text(); p=(root/'world_shadow_registry_probe.cpp').read_text()
checks={
 'shared receiver':'const bool receiverEligible=worldDraw&&landShadow.prepass.finished',
 'semantic depth coverage':'const bool shadowDepthCoverage=rs[D3DRS_ZWRITEENABLE]',
 'sea registry':'const bool seaTraversalCandidate=worldDraw&&!landShadow.locationActive',
 'order-independent sea capture':'const bool traversalCapture=seaTraversalCandidate;',
 'texture-selected alpha':'const bool selectedTextureAlpha=',
 'unconditional present resolve':'Present(const RECT*,const RECT*,HWND,const RGNDATA*)override {resolveTraversalShadow();',
 'material-independent overlay':'texture color cancels from direct/total',
 'resident raw shadow span':'const bool rawOutdoorShadow=outdoorLocationShadow&&boundIndexed&&vb&&ib',
 'raw shadow pipeline':'land_raw_caster',
 'strict native shadow contract':'STORM_METAL_REQUIRE_NATIVE_SHADOW',
 'raw shadow coverage counter':'rawShadowDraws',
 'legacy shadow coverage counter':'legacyShadowDraws',
}
for name,token in checks.items():
 text=h if name in ('material-independent overlay','raw shadow pipeline') else b
 if token not in text: raise SystemExit(f'FAIL {name}')
for token in ('blended Z-writing MODELR caster','multistage/lightmapped MODELR caster','unconditional scene-boundary registry resolve'):
 if token not in p: raise SystemExit(f'FAIL behavioral case {token}')
print('world shadow owner source contract: PASS')
