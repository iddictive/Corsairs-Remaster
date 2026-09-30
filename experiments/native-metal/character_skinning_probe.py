#!/usr/bin/env python3
"""Reject character normals that inherit animated bone translation."""
from pathlib import Path
import math

patch = Path(__file__).with_name("character-skinning-normals.patch").read_text()
assert "XMVector3TransformNormal" in patch
assert "mtx.matrix[0] * n.x + mtx.matrix[4] * n.y + mtx.matrix[8] * n.z" in patch


def point_transform(matrix, vector):
    x, y, z = vector
    return (
        matrix[0] * x + matrix[4] * y + matrix[8] * z + matrix[12],
        matrix[1] * x + matrix[5] * y + matrix[9] * z + matrix[13],
        matrix[2] * x + matrix[6] * y + matrix[10] * z + matrix[14],
    )


def direction_transform(matrix, vector):
    x, y, z = vector
    return (
        matrix[0] * x + matrix[4] * y + matrix[8] * z,
        matrix[1] * x + matrix[5] * y + matrix[9] * z,
        matrix[2] * x + matrix[6] * y + matrix[10] * z,
    )


def normalized(vector):
    length = math.sqrt(sum(component * component for component in vector))
    return tuple(component / length for component in vector)


# Representative shoulder bone: 90 degree turn and a large animated offset.
# The offset must move the vertex but must not change its light-facing direction.
bone = [0.0] * 16
bone[0], bone[1], bone[4], bone[5], bone[10], bone[15] = 0, 1, -1, 0, 1, 1
bone[12], bone[13], bone[14] = 7, 3, -2
source_normal = (1, 0, 0)
expected = (0, 1, 0)
fixed = normalized(direction_transform(bone, source_normal))
broken = normalized(point_transform(bone, source_normal))
assert all(abs(a - b) < 1e-6 for a, b in zip(fixed, expected))
assert sum(abs(a - b) for a, b in zip(broken, expected)) > 0.5

# Nearest negative: changing translation moves an animated point while leaving
# the normal and therefore diffuse lighting invariant.
moved = bone.copy()
moved[12], moved[13], moved[14] = -11, 8, 5
assert point_transform(moved, (0, 0, 0)) != point_transform(bone, (0, 0, 0))
assert direction_transform(moved, source_normal) == direction_transform(bone, source_normal)
print("PASS: skinned character normals ignore bone translation; positions still follow it")
