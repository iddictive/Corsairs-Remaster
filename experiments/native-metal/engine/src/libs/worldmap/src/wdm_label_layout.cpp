#include "wdm_label_layout.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace wdm::labels
{
namespace
{

enum class Direction
{
    None,
    Up,
    Right,
    Down,
    Left
};

bool Intersects(const Rect &a, const Rect &b, float padding)
{
    return a.right + padding > b.left && b.right + padding > a.left && a.bottom + padding > b.top &&
           b.bottom + padding > a.top;
}

float OverlapArea(const Rect &a, const Rect &b)
{
    const float width = std::max(0.0f, std::min(a.right, b.right) - std::max(a.left, b.left));
    const float height = std::max(0.0f, std::min(a.bottom, b.bottom) - std::max(a.top, b.top));
    return width * height;
}

Rect Translate(const Rect &rect, float x, float y)
{
    return {rect.left + x, rect.top + y, rect.right + x, rect.bottom + y};
}

Rect ClampToViewport(Rect rect, float viewportWidth, float viewportHeight)
{
    const float width = rect.right - rect.left;
    const float height = rect.bottom - rect.top;
    if (width <= viewportWidth)
    {
        const float x = std::clamp(rect.left, 0.0f, viewportWidth - width) - rect.left;
        rect = Translate(rect, x, 0.0f);
    }
    if (height <= viewportHeight)
    {
        const float y = std::clamp(rect.top, 0.0f, viewportHeight - height) - rect.top;
        rect = Translate(rect, 0.0f, y);
    }
    return rect;
}

Rect Push(const Rect &preferred, const std::vector<Rect> &accepted, Direction direction, float padding)
{
    Rect candidate = preferred;
    for (size_t pass = 0; pass <= accepted.size(); ++pass)
    {
        bool moved = false;
        for (const Rect &obstacle : accepted)
        {
            if (!Intersects(candidate, obstacle, padding))
                continue;

            switch (direction)
            {
            case Direction::Up:
                candidate = Translate(candidate, 0.0f, obstacle.top - padding - candidate.bottom);
                break;
            case Direction::Right:
                candidate = Translate(candidate, obstacle.right + padding - candidate.left, 0.0f);
                break;
            case Direction::Down:
                candidate = Translate(candidate, 0.0f, obstacle.bottom + padding - candidate.top);
                break;
            case Direction::Left:
                candidate = Translate(candidate, obstacle.left - padding - candidate.right, 0.0f);
                break;
            case Direction::None:
                return candidate;
            }
            moved = true;
        }
        if (!moved)
            break;
    }
    return candidate;
}

float DisplacementSquared(const Rect &a, const Rect &b)
{
    const float x = a.left - b.left;
    const float y = a.top - b.top;
    return x * x + y * y;
}

} // namespace

void ResolveCollisions(std::vector<LayoutItem> &items, float viewportWidth, float viewportHeight, float padding)
{
    std::vector<LayoutItem *> priority;
    priority.reserve(items.size());
    for (LayoutItem &item : items)
    {
        item.placed = item.preferred;
        priority.push_back(&item);
    }
    std::stable_sort(priority.begin(), priority.end(), [](const LayoutItem *a, const LayoutItem *b) {
        if (a->weight != b->weight)
            return a->weight > b->weight;
        return a->index < b->index;
    });

    std::vector<Rect> accepted;
    accepted.reserve(items.size());
    constexpr std::array directions{Direction::None, Direction::Up, Direction::Right, Direction::Down,
                                    Direction::Left};

    for (LayoutItem *item : priority)
    {
        Rect best = ClampToViewport(item->preferred, viewportWidth, viewportHeight);
        float bestOverlap = std::numeric_limits<float>::max();
        float bestDisplacement = std::numeric_limits<float>::max();

        for (Direction direction : directions)
        {
            Rect candidate = Push(item->preferred, accepted, direction, padding);
            candidate = ClampToViewport(candidate, viewportWidth, viewportHeight);
            float overlap = 0.0f;
            for (const Rect &obstacle : accepted)
            {
                const Rect paddedObstacle{obstacle.left - padding, obstacle.top - padding, obstacle.right + padding,
                                          obstacle.bottom + padding};
                overlap += OverlapArea(candidate, paddedObstacle);
            }
            const float displacement = DisplacementSquared(candidate, item->preferred);
            if (overlap < bestOverlap || (overlap == bestOverlap && displacement < bestDisplacement))
            {
                best = candidate;
                bestOverlap = overlap;
                bestDisplacement = displacement;
            }
        }

        item->placed = best;
        accepted.push_back(best);
    }
}

} // namespace wdm::labels
