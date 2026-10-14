#include "../src/wdm_label_layout.h"

#include <iostream>
#include <vector>

namespace
{

bool Intersects(const wdm::labels::Rect &a, const wdm::labels::Rect &b)
{
    return a.right > b.left && b.right > a.left && a.bottom > b.top && b.bottom > a.top;
}

bool IntersectsWithPadding(const wdm::labels::Rect &a, const wdm::labels::Rect &b, float padding)
{
    return a.right + padding > b.left && b.right + padding > a.left && a.bottom + padding > b.top &&
           b.bottom + padding > a.top;
}

bool Check(bool condition, const char *message)
{
    if (!condition)
        std::cerr << message << '\n';
    return condition;
}

} // namespace

int main()
{
    using namespace wdm::labels;
    bool passed = true;

    std::vector<LayoutItem> weighted{{0, 10, {40, 40, 80, 60}, {}}, {1, 100, {40, 40, 80, 60}, {}}};
    ResolveCollisions(weighted, 200, 120);
    passed &= Check(weighted[1].placed.left == 40.0f && weighted[1].placed.top == 40.0f,
                    "higher-priority label moved from its preferred position");
    passed &= Check(!Intersects(weighted[0].placed, weighted[1].placed), "two movable labels still overlap");

    std::vector<LayoutItem> near{{0, 100, {20, 20, 70, 40}, {}}, {1, 10, {72, 20, 122, 40}, {}}};
    ResolveCollisions(near, 200, 120, 4.0f);
    passed &= Check(!IntersectsWithPadding(near[0].placed, near[1].placed, 4.0f),
                    "layout did not preserve requested padding");

    std::vector<LayoutItem> first{{0, 50, {20, 20, 70, 40}, {}}, {1, 50, {20, 20, 70, 40}, {}}};
    auto second = first;
    ResolveCollisions(first, 160, 100);
    ResolveCollisions(second, 160, 100);
    for (size_t i = 0; i < first.size(); ++i)
    {
        passed &= Check(first[i].placed.left == second[i].placed.left && first[i].placed.top == second[i].placed.top &&
                            first[i].placed.right == second[i].placed.right &&
                            first[i].placed.bottom == second[i].placed.bottom,
                        "equal-weight layout is not deterministic");
    }

    std::vector<LayoutItem> dense;
    for (int32_t i = 0; i < 20; ++i)
        dense.push_back({i, static_cast<uint32_t>(i), {0, 0, 90, 40}, {}});
    ResolveCollisions(dense, 100, 50);
    passed &= Check(dense.size() == 20, "dense layout dropped a label");
    for (const LayoutItem &label : dense)
        passed &= Check(label.placed.right > label.placed.left && label.placed.bottom > label.placed.top,
                        "dense layout produced an invalid rectangle");

    return passed ? 0 : 1;
}
