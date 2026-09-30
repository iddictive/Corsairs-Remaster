#include "frame_limiter.hpp"

#include <cassert>
#include <chrono>
#include <cstdio>

int main()
{
    using Limiter = storm::frame::Limiter;
    using namespace std::chrono;

    Limiter limiter120(120);
    assert(duration_cast<nanoseconds>(limiter120.Interval()).count() == 8333333);
    const auto origin = Limiter::TimePoint{};
    assert(limiter120.NextFrame(origin) == origin);
    auto target = origin;
    for (int frame = 1; frame <= 120; ++frame)
    {
        target = limiter120.NextFrame(target);
        assert(target > origin);
    }
    assert(duration_cast<nanoseconds>(target - origin).count() == 999999960);

    // A late frame is returned immediately and advances beyond the missed
    // deadlines in one calculation instead of spinning through catch-up work.
    Limiter stalled(120);
    stalled.NextFrame(origin);
    const auto late = origin + milliseconds(50);
    assert(stalled.NextFrame(late) == late);
    const auto recovery = stalled.NextFrame(late);
    assert(recovery > late);
    assert(recovery - late <= stalled.Interval());

    Limiter unlimited(0);
    assert(unlimited.NextFrame(late) == late);
    assert(unlimited.Interval() == Limiter::Duration::zero());

    std::puts("PASS: 120 FPS interval is 8.333333 ms; late frames skip debt without catch-up spin");
}
