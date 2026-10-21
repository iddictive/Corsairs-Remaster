#!/usr/bin/env python3
import json
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import metal_acceptance


class HarnessTest(unittest.TestCase):
    def test_profile_parser(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "profile.log"
            path.write_text("[StormMetal profile] frames=120 inactive=0 wall=8.2ms gpu=4.1ms uploadMB=0 legacy3DDraws=0 legacyShadowDraws=0 readbacks=0 syncs=0 (per-frame averages)\n")
            self.assertEqual(metal_acceptance.profiles(path)[0]["frames"], 120)
            self.assertEqual(metal_acceptance.profiles(path)[0]["uploadMB"], 0)

    def test_synthetic_bundle_is_rejected(self):
        matrix = {"schema":1,"minimum_profile_windows":3,"minimum_profile_frames":360,"default_limits":{"legacy3DDraws":0,"legacyShadowDraws":0,"uploadMB":8,"readbacks":0,"syncs":0,"postfxPasses":0,"wall":9,"gpu":9},"scenes":[]}
        bundle = {"schema":1,"source":"probe","synthetic":True,"build":{"executable_sha256":"x","engine_revision":"x","patch_set_sha256":"x","resource_manifest_sha256":"x","backbuffer":"1x1","device":"x"},"scenes":[]}
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); (root/"matrix.json").write_text(json.dumps(matrix)); (root/"bundle.json").write_text(json.dumps(bundle))
            errors, _ = metal_acceptance.validate(root/"matrix.json", root/"bundle.json")
            self.assertTrue(any("real-game-replay" in error for error in errors))

    def test_real_scene_passes_and_legacy_draw_fails(self):
        matrix = {"schema":1,"minimum_profile_windows":3,"minimum_profile_frames":360,"default_limits":{"legacy3DDraws":0,"legacyShadowDraws":0,"uploadMB":8,"readbacks":0,"syncs":0,"postfxPasses":0,"wall":9,"gpu":9},"scenes":[{"id":"town-day","resources":["alpha-test"],"checks":["alpha_ok"]}]}
        bundle = {"schema":1,"source":"real-game-replay","synthetic":False,"build":{"executable_sha256":"x","engine_revision":"x","patch_set_sha256":"x","resource_manifest_sha256":"x","backbuffer":"1x1","device":"x"},"artifact_sha256":{},"scenes":[{"id":"town-day","synthetic":False,"save_or_route":"real save","save_sha256":"x","interactive":True,"responsive":True,"screenshots":["town.png"],"profile_log":"profile.log","postfx_passes_per_frame":0,"resource_presence":{"alpha-test":True},"visual_checks":{"alpha_ok":True}}]}
        line = "[StormMetal profile] frames=120 inactive=0 wall=8ms gpu=4ms uploadMB=4 legacy3DDraws=0 legacyShadowDraws=0 readbacks=0 syncs=0 (per-frame averages)\n"
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory); (root/"matrix.json").write_text(json.dumps(matrix)); (root/"town.png").write_bytes(b"png"); (root/"profile.log").write_text(line*3)
            bundle["artifact_sha256"] = {name: metal_acceptance.sha256(root/name) for name in ("town.png", "profile.log")}; (root/"bundle.json").write_text(json.dumps(bundle))
            self.assertEqual(metal_acceptance.validate(root/"matrix.json", root/"bundle.json")[0], [])
            (root/"profile.log").write_text(line.replace("legacy3DDraws=0", "legacy3DDraws=1")*3)
            bundle["artifact_sha256"]["profile.log"] = metal_acceptance.sha256(root/"profile.log"); (root/"bundle.json").write_text(json.dumps(bundle))
            errors, _ = metal_acceptance.validate(root/"matrix.json", root/"bundle.json")
            self.assertTrue(any("legacy3DDraws" in error for error in errors))

    def test_scene_cannot_weaken_protected_limits(self):
        matrix = {"schema":1,"minimum_profile_windows":1,"minimum_profile_frames":1,"default_limits":{"legacy3DDraws":0,"legacyShadowDraws":0,"uploadMB":8,"readbacks":0,"syncs":0,"postfxPasses":0,"wall":9,"gpu":9},"scenes":[{"id":"menu","class":"menu","resources":[],"checks":[]}]}
        line = "[StormMetal profile] frames=1 inactive=0 wall=900ms gpu=900ms uploadMB=900 legacy3DDraws=1 legacyShadowDraws=1 readbacks=1 syncs=1 (per-frame averages)\n"
        bundle = {"schema":1,"source":"real-game-replay","synthetic":False,"build":{"executable_sha256":"x","engine_revision":"x","patch_set_sha256":"x","resource_manifest_sha256":"x","backbuffer":"1x1","device":"x"},"artifact_sha256":{},"scenes":[{"id":"menu","synthetic":False,"save_or_route":"menu","save_sha256":"x","interactive":True,"responsive":True,"screenshots":["menu.png"],"profile_log":"profile.log","postfx_passes_per_frame":99,"limits":{"legacy3DDraws":1,"legacyShadowDraws":1,"uploadMB":900,"readbacks":1,"syncs":1,"postfxPasses":99,"wall":900,"gpu":900},"resource_presence":{},"visual_checks":{}}]}
        with tempfile.TemporaryDirectory() as directory:
            root=Path(directory); (root/"matrix.json").write_text(json.dumps(matrix)); (root/"menu.png").write_bytes(b"png"); (root/"profile.log").write_text(line)
            bundle["artifact_sha256"]={name:metal_acceptance.sha256(root/name) for name in ("menu.png","profile.log")}; (root/"bundle.json").write_text(json.dumps(bundle))
            errors,_=metal_acceptance.validate(root/"matrix.json",root/"bundle.json")
            self.assertGreaterEqual(sum("limit override" in error for error in errors), 8)
            self.assertTrue(any("legacy3DDraws worst" in error for error in errors))


if __name__ == "__main__": unittest.main()
