#!/usr/bin/env python3
"""CPU oracle for Storm's rebased VIEW/WORLD shadow-space contract."""

from __future__ import annotations

import math


def add(a, b):
    return tuple(x + y for x, y in zip(a, b))


def sub(a, b):
    return tuple(x - y for x, y in zip(a, b))


def dot(a, b):
    return sum(x * y for x, y in zip(a, b))


def cross(a, b):
    return (
        a[1] * b[2] - a[2] * b[1],
        a[2] * b[0] - a[0] * b[2],
        a[0] * b[1] - a[1] * b[0],
    )


def scale(v, s):
    return tuple(x * s for x in v)


def unit(v):
    length = math.sqrt(dot(v, v))
    return scale(v, 1.0 / length)


def basis(forward, up):
    z = unit(forward)
    x = unit(cross(up, z))
    return x, cross(z, x), z


def view_point(point, eye, forward, up):
    """Equivalent to Storm lookAt and its column-vector Metal transform."""
    x, y, z = basis(forward, up)
    relative = sub(point, eye)
    return dot(x, relative), dot(y, relative), dot(z, relative)


def wrapper_view_point(point, eye, forward, up):
    """Emulate DX9RENDER::SetTransform(VIEW): strip VIEW translation and rebase WORLD."""
    x, y, z = basis(forward, up)
    rebased_world = sub(point, eye)  # vWordRelationPos becomes -eye for this VIEW
    return dot(x, rebased_world), dot(y, rebased_world), dot(z, rebased_world)


def close(a, b, epsilon=1e-5):
    return all(abs(x - y) < epsilon for x, y in zip(a, b))


def main():
    obj = (111.0, 4.0, -37.0)
    lamp = (105.0, 9.0, -31.0)
    forward = unit(sub(obj, lamp))
    up = (0.0, 1.0, 0.0)

    expected = view_point(obj, lamp, forward, up)
    assert close(wrapper_view_point(obj, lamp, forward, up), expected), (
        "absolute light VIEW must survive Storm's VIEW/WORLD rebasing"
    )

    # Two third-person camera yaw positions on the same orbit. Storm publishes
    # worldOrigin=-cameraPosition, so rotating the camera changes this value.
    target = (110.0, 4.0, -36.0)
    camera_origins = []
    for yaw in (0.0, math.pi * 0.5):
        camera = add(target, (8.0 * math.sin(yaw), 3.0, 8.0 * math.cos(yaw)))
        camera_origins.append(scale(camera, -1.0))
    corrected_samples = []
    broken_casters = []
    for origin in camera_origins:
        # Engine receiver WORLD is already camera-relative: Wrel = Wabs + origin.
        receiver_relative = add(obj, origin)
        recovered_absolute = sub(receiver_relative, origin)
        corrected_samples.append(view_point(recovered_absolute, lamp, forward, up))

        # Reproduces the rejected implementation: beginPass first adds worldOrigin
        # to an already absolute lamp, then SetTransform(VIEW) treats it as a new eye.
        broken_eye = add(lamp, origin)
        broken_casters.append(wrapper_view_point(obj, broken_eye, forward, up))

    assert close(corrected_samples[0], expected)
    assert close(corrected_samples[1], expected), (
        "P*Vabsolute*T(-frameOrigin)*Wrelative must be camera invariant"
    )
    assert not close(broken_casters[0], broken_casters[1]), (
        "negative case failed: pre-rebasing the light must move its shadow with camera"
    )

    # Point-light lookup in the receiver shader shares camera-relative WORLD.
    for origin in camera_origins:
        receiver_relative = add(obj, origin)
        lamp_relative = add(lamp, origin)
        assert close(sub(receiver_relative, lamp_relative), sub(obj, lamp))

    print("PASS: absolute shadow pass and single receiver rebase are camera invariant")
    print("PASS: camera-pre-rebased light reproduces moving-shadow regression")


if __name__ == "__main__":
    main()
