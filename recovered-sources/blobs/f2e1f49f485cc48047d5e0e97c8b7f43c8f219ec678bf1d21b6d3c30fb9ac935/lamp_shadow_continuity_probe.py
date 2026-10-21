#!/usr/bin/env python3
"""CPU oracle for the source-owned Metal lamp-shadow selection lifecycle."""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path


FADE_SECONDS = 0.25
MAX_SHADOW_CUBES = 4


@dataclass(frozen=True)
class Lamp:
    id: int
    x: float
    range: float
    color: tuple[float, float, float]


class Selector:
    def __init__(self) -> None:
        self.elapsed = 0.0
        self.weights: dict[int, float] = {}

    def execute(self, elapsed: float) -> None:
        self.elapsed = min(self.elapsed + elapsed, FADE_SECONDS)

    def reset(self) -> None:
        self.elapsed = 0.0
        self.weights.clear()

    def select(self, focus_x: float, lamps: list[Lamp]) -> list[tuple[Lamp, float]]:
        candidates = sorted(
            (
                ((focus_x - lamp.x) ** 2, lamp.id, lamp)
                for lamp in lamps
                if lamp.range > 0
                and any(component > 0 for component in lamp.color)
                and (focus_x - lamp.x) ** 2 < lamp.range**2
            ),
            key=lambda candidate: candidate[:2],
        )
        by_id = {lamp.id: lamp for _, _, lamp in candidates}
        desired = {lamp.id for _, _, lamp in candidates[:2]}
        for lamp_id in desired:
            self.weights.setdefault(lamp_id, 0.0)

        step = self.elapsed / FADE_SECONDS
        self.elapsed = 0.0
        for lamp_id in list(self.weights):
            if lamp_id not in by_id:
                self.weights[lamp_id] = 0.0
            elif lamp_id in desired:
                self.weights[lamp_id] = min(1.0, self.weights[lamp_id] + step)
            else:
                self.weights[lamp_id] = max(0.0, self.weights[lamp_id] - step)
        self.weights = {lamp_id: weight for lamp_id, weight in self.weights.items() if weight > 0.0}

        active = sorted(
            self.weights,
            key=lambda lamp_id: (
                lamp_id not in desired,
                -self.weights[lamp_id],
                lamp_id,
            ),
        )[:MAX_SHADOW_CUBES]
        return [(by_id[lamp_id], self.weights[lamp_id]) for lamp_id in sorted(active)]


def weights(selected: list[tuple[Lamp, float]]) -> dict[int, float]:
    return {lamp.id: weight for lamp, weight in selected}


def prime(selector: Selector, focus_x: float, lamps: list[Lamp]) -> None:
    selector.execute(FADE_SECONDS)
    assert weights(selector.select(focus_x, lamps)) == {1: 1.0, 2: 1.0}


def test_membership_crossfade() -> None:
    lamps = [Lamp(1, 0, 10, (1, 0.4, 0.2)), Lamp(2, 4, 10, (0.2, 1, 0.4)), Lamp(3, 8, 10, (0.4, 0.2, 1))]
    selector = Selector()
    prime(selector, 1, lamps)
    selector.execute(0.125)
    crossed = weights(selector.select(5, lamps))
    assert crossed == {1: 0.5, 2: 1.0, 3: 0.5}
    assert lamps[0].color == (1, 0.4, 0.2) and lamps[0].range == 10
    selector.execute(0.125)
    assert weights(selector.select(5, lamps)) == {2: 1.0, 3: 1.0}


def test_round_trip_and_partitioning() -> None:
    lamps = [Lamp(1, 0, 10, (1, 1, 1)), Lamp(2, 4, 10, (1, 1, 1)), Lamp(3, 8, 10, (1, 1, 1))]
    selector = Selector()
    prime(selector, 1, lamps)
    selector.execute(0.125)
    selector.select(5, lamps)
    selector.execute(0.0625)
    assert weights(selector.select(1, lamps)) == {1: 0.75, 2: 1.0, 3: 0.25}

    whole = Selector()
    split = Selector()
    prime(whole, 1, lamps)
    prime(split, 1, lamps)
    whole.execute(0.2)
    whole_weights = weights(whole.select(5, lamps))
    split.execute(0.1)
    split.select(5, lamps)
    split.execute(0.1)
    split_weights = weights(split.select(5, lamps))
    assert whole_weights == split_weights == {1: 0.2, 2: 1.0, 3: 0.8}


def test_pause_range_disable_delete_and_bound() -> None:
    lamps = [Lamp(1, 0, 10, (1, 1, 1)), Lamp(2, 4, 10, (1, 1, 1)), Lamp(3, 8, 10, (1, 1, 1))]
    selector = Selector()
    prime(selector, 1, lamps)
    selector.execute(0)
    assert weights(selector.select(1, lamps)) == {1: 1.0, 2: 1.0}
    selector.execute(0.125)
    assert 1 not in weights(selector.select(1, [lamps[1], lamps[2]]))
    selector.reset()
    assert selector.select(1, lamps) == []

    crowded = [Lamp(index, float(index * 2), 30, (1, 1, 1)) for index in range(1, 7)]
    selector.execute(FADE_SECONDS)
    selector.select(1, crowded)
    selector.execute(0.05)
    active = selector.select(10, crowded)
    assert len(active) <= MAX_SHADOW_CUBES
    assert {lamp.id for lamp, _ in active}.issuperset({4, 5})


def test_patch_contract() -> None:
    patch = (Path(__file__).with_name("multi-shadow-lamps-v2.patch")).read_text()
    required = (
        "metalShadowElapsed=std::min(metalShadowElapsed+float(delta_time)*.001f,.25f);",
        "const float elapsed=metalShadowElapsed;",
        "metalShadowElapsed=0;",
        "if(metalShadowSelections.size()>4u)metalShadowSelections.resize(4u);",
        "light.Diffuse=source.color;",
        "StormMetalSetPointShadowWeight(device,selected[index].id,selected[index].weight);",
        "ResetMetalShadowSelection();",
    )
    for needle in required:
        assert needle in patch, needle


test_membership_crossfade()
test_round_trip_and_partitioning()
test_pause_range_disable_delete_and_bound()
test_patch_contract()
print("PASS lamp shadow continuity: stable IDs, bounded membership fade, pause/reset, and deterministic timestep")
