#pragma once

#include <cstdint>
#include <vector>

namespace wdm::labels
{

struct Rect
{
    float left;
    float top;
    float right;
    float bottom;
};

struct LayoutItem
{
    int32_t index;
    uint32_t weight;
    Rect preferred;
    Rect placed;
};

void ResolveCollisions(std::vector<LayoutItem> &items, float viewportWidth, float viewportHeight,
                       float padding = 4.0f);

} // namespace wdm::labels
