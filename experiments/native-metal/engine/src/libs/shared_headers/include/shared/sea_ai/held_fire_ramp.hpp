#pragma once

#include <algorithm>

// Integrated volleys per second: 2 + 5.8*t until ten seconds, then 60.
// Fractional work stays in debt so a low frame rate does not drop launches.
inline void AdvanceHeldFireRamp(double &seconds, double &debt, double delta)
{
    delta = std::max(0.0, delta);
    const double before = std::min(seconds, 10.0);
    const double after = std::min(seconds + delta, 10.0);
    const double tail = std::max(0.0, seconds + delta - 10.0) - std::max(0.0, seconds - 10.0);
    debt += 2.0 * delta + 2.9 * (after * after - before * before) + 58.0 * tail;
    seconds += delta;
}
