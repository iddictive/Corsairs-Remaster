"""Source-scale frames shared by offline sea, chart and navigation preparation.

The old chart's logical-region labels are not a rigid atlas: Cuba has two
origins and Mainland has seven. Keep each physical body's authored metre and
bind only its primary origin to the old chart. Derive every secondary origin
from the proven local-to-body correspondence. The chart is a projection of
this atlas, never a second definition of its coastline.

This module prepares data only. It does not activate the world or change SAVE.
At runtime a saved rigid atlas-to-sea frame can keep a legacy scene's local
coordinates unchanged, including a rotated Cuba2 scene.
"""
from __future__ import annotations

import math


MAP_METRES = 50.0


def point(matrix, x, z):
    a, b, c, d, tx, tz = matrix
    return [a*x + b*z + tx, c*x + d*z + tz]


def compose(outer, inner):
    a, b, c, d, tx, tz = outer
    e, f, g, h, ux, uz = inner
    return [a*e+b*g, a*f+b*h, c*e+d*g, c*f+d*h,
            a*ux+b*uz+tx, c*ux+d*uz+tz]


def inverse(matrix):
    a, b, c, d, tx, tz = matrix
    det = a*d-b*c
    if not all(math.isfinite(v) for v in matrix) or det <= 0.0:
        raise ValueError("Atlas frame is not a finite, orientation-preserving transform")
    return [d/det, -b/det, -c/det, a/det,
            (b*tz-d*tx)/det, (c*tx-a*tz)/det]


def layout(recipe, inventory):
    """Return one physical atlas and both directions of every consumer frame."""
    chart = [1/MAP_METRES, 0.0, 0.0, 1/MAP_METRES, 0.0, 0.0]
    bodies, regions = {}, {}
    for name, body in recipe["bodies"].items():
        primary = body["source_region"]
        anchor = inventory["regions"][primary]["map_anchor_xz"]
        if not anchor or not all(math.isfinite(v) for v in anchor):
            raise ValueError(f"Atlas body has no finite primary chart origin: {name}")
        to_atlas = [1.0, 0.0, 0.0, 1.0, anchor[0]*MAP_METRES, anchor[1]*MAP_METRES]
        bodies[name] = {"source_region": primary, "body_to_atlas": to_atlas,
                        "atlas_to_body": inverse(to_atlas)}
        for region in body["logical_regions"]:
            if region in regions:
                raise ValueError(f"Logical region has two physical owners: {region}")
            to_body = body["local_to_body"][region]
            a, b, c, d = to_body[:4]
            if max(abs(a*a+c*c-1), abs(b*b+d*d-1), abs(a*b+c*d)) > 2e-6:
                raise ValueError(f"Authored scene scale cannot change in atlas placement: {region}")
            local_atlas = compose(to_atlas, to_body)
            local_map = compose(chart, local_atlas)
            regions[region] = {
                "body": name, "local_to_body": list(to_body), "body_to_local": inverse(to_body),
                "local_to_atlas": local_atlas, "atlas_to_local": inverse(local_atlas),
                "local_to_map": local_map, "map_to_local": inverse(local_map),
                "map_origin": point(local_map, 0.0, 0.0),
                "legacy_map_origin": inventory["regions"][region]["map_anchor_xz"],
            }
    if set(regions) != set(inventory["regions"]):
        raise ValueError("Atlas placement does not cover the authored logical regions")
    return {"version": 2, "metres_per_map_unit": MAP_METRES,
            "placement": "Authored metre; primary chart anchor; shared-region origins derived from native body correspondence",
            "atlas_to_map": chart, "map_to_atlas": inverse(chart),
            "bodies": bodies, "regions": regions}


def scene_frame(atlas_layout, region, legacy_offset=(0.0, 0.0)):
    """Keep a loaded legacy region at its old sea pose without moving actors.

    The offset is zero for an ordinary pre-atlas SAVE. Version-1 resident saves
    supply their selected region's old translation. This frame is saved once;
    entering another region must not choose a new frame or teleport the world.
    """
    translation = [1.0, 0.0, 0.0, 1.0, *legacy_offset]
    atlas_to_sea = compose(translation, atlas_layout["regions"][region]["atlas_to_local"])
    return {"atlas_to_sea": atlas_to_sea, "sea_to_atlas": inverse(atlas_to_sea)}


def ini(atlas_layout, recipe_sha256, inventory_sha256, model_trees):
    """Native INIFILE input; all matrices use x'=a*x+b*z+tx, z'=c*x+d*z+tz."""
    rows = ["[Atlas]", "version=2", "runtime_activation=0",
            f"recipe_sha256={recipe_sha256}", f"inventory_sha256={inventory_sha256}",
            f"metres_per_map_unit={MAP_METRES:g}", f"bodies={len(atlas_layout['bodies'])}",
            f"regions={len(atlas_layout['regions'])}"]
    for index, (name, body) in enumerate(atlas_layout["bodies"].items()):
        rows += ["", f"[Body{index}]", f"id={name}", f"source_region={body['source_region']}"]
        for role in ("terrain", "reflection"):
            rows.append(role+"="+model_trees[name][role]["path"])
        for key in ("body_to_atlas", "atlas_to_body"):
            rows.append(key+"="+",".join(format(v, ".17g") for v in body[key]))
    for index, (name, region) in enumerate(atlas_layout["regions"].items()):
        rows += ["", f"[Region{index}]", f"id={name}", f"body={region['body']}"]
        for key in ("local_to_body", "body_to_local", "local_to_atlas", "atlas_to_local",
                    "local_to_map", "map_to_local", "map_origin", "legacy_map_origin"):
            rows.append(key+"="+",".join(format(v, ".17g") for v in region[key]))
    return ("\n".join(rows)+"\n").encode("ascii")
