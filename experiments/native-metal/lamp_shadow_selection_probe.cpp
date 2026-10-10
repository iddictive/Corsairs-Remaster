#include "metal_shadow_selection.hpp"
#include <cassert>
#include <cmath>
#include <vector>

using metal_shadow::Candidate;
using metal_shadow::Selector;
using metal_shadow::Selection;

static float weight(const std::vector<Selection>& values, uint64_t id) {
    for (const auto& value : values) if (value.id == id) return value.weight;
    return 0;
}

int main() {
    const std::vector<Candidate> left{{1, 1}, {2, 9}, {3, 49}};
    const std::vector<Candidate> right{{2, 1}, {3, 9}, {1, 25}};
    Selector selector;
    auto original = selector.select(left);
    assert(original.size() == 3);
    auto crossing = selector.select(right);
    assert(crossing.size() == 3 && weight(crossing, 1) == 1 && weight(crossing, 2) == 1 && weight(crossing, 3) == 1);
    auto returnTrip = selector.select(left);
    assert(weight(returnTrip, 1) == 1 && weight(returnTrip, 3) == 1);
    auto paused = selector.select(left);
    assert(weight(paused, 1) == 1);
    assert(selector.select({}).empty());
    selector.reset();
    assert(selector.select(left).size() == 3);
    selector.reset();
    std::vector<Candidate> crowded;
    for (unsigned id = 1; id <= 12; ++id) crowded.push_back({id, float(id)});
    auto bounded = selector.select(crowded);
    assert(bounded.size() == metal_shadow::Capacity);
    for (auto& value : crowded) value.distanceSquared = 13 - value.id;
    auto reordered = selector.select(crowded);
    for (auto value : bounded) assert(weight(reordered, value.id) == 1);
    // Only a lamp whose influence left the view (or was deleted) frees a slot.
    crowded.erase(crowded.begin());
    auto replaced = selector.select(crowded);
    assert(replaced.size() == metal_shadow::Capacity && weight(replaced, 1) == 0 && weight(replaced, 12) == 1);
    selector.reset();
    auto fresh = selector.select(crowded);
    assert(weight(fresh, 12) == 1 && weight(fresh, 2) == 0);

    const float k = std::sqrt(.5f);
    const metal_shadow::Plane view[]{{k,0,k,0},{-k,0,k,0},{0,k,k,0},{0,-k,k,0}};
    // Lamp outside player Range still illuminates a visible wall at z=20.
    const float visibleLamp[]{0,0,22};
    assert(metal_shadow::overlapsView(view, visibleLamp, 5));
    const float outsideLamp[]{40,0,10};
    assert(!metal_shadow::overlapsView(view, outsideLamp, 5));
    const float reachingLamp[]{12,0,10};
    assert(metal_shadow::overlapsView(view, reachingLamp, 5));
}
