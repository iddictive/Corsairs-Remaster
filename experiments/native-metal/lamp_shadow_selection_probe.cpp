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

static void prime(Selector& selector, const std::vector<Candidate>& left) {
    selector.advance(.25f);
    auto values = selector.select(left);
    assert(weight(values, 1) == 1 && weight(values, 2) == 1);
}

int main() {
    const std::vector<Candidate> left{{1, 1}, {2, 9}, {3, 49}};
    const std::vector<Candidate> right{{2, 1}, {3, 9}, {1, 25}};
    Selector selector;
    prime(selector, left);
    selector.advance(.125f);
    auto crossing = selector.select(right);
    assert(crossing.size() == 3 && weight(crossing, 1) == .5f && weight(crossing, 2) == 1 && weight(crossing, 3) == .5f);
    selector.advance(.0625f);
    auto returnTrip = selector.select(left);
    assert(std::fabs(weight(returnTrip, 1) - .75f) < 1e-6f && std::fabs(weight(returnTrip, 3) - .25f) < 1e-6f);
    auto paused = selector.select(left);
    assert(std::fabs(weight(paused, 1) - .75f) < 1e-6f);
    selector.advance(.25f);
    assert(selector.select({}).empty());
    selector.reset();
    assert(selector.select(left).empty());
    std::vector<Candidate> crowded{{1, 1}, {2, 2}, {3, 3}, {4, 4}, {5, 5}, {6, 6}};
    selector.advance(.25f); selector.select(crowded);
    selector.advance(.05f);
    auto bounded = selector.select({{4, 1}, {5, 2}, {6, 3}, {1, 4}, {2, 5}, {3, 6}});
    assert(bounded.size() <= 4 && weight(bounded, 4) > 0 && weight(bounded, 5) > 0);
}
