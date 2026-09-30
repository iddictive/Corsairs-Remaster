// Compile against the snapshot after timing.patch is applied; no wall-clock sleeps.
#include "timer.h"
#include <cassert>
#include <cmath>
#include <cstdio>
int main() {
    TIMER timer;
    const auto origin=timer.Current;
    double elapsed=0;
    unsigned legacy=0;
    for(int frame=1;frame<=120;++frame) {
        const auto timestamp=origin+std::chrono::nanoseconds((1000000000LL*frame)/120);
        legacy+=timer.Run(timestamp);
        const float dt=timer.GetVisualDeltaTime();
        assert(std::abs(dt-1000.f/120.f)<0.0001f);
        elapsed+=dt;
    }
    assert(std::abs(elapsed-1000.0)<0.001);
    assert(legacy==960); // Legacy gameplay cadence intentionally unchanged.
    timer.visualDeltaTime*=0.f; // Core timeScale=0 pause.
    assert(timer.GetVisualDeltaTime()==0.f);
    timer.SetDelta(20);
    assert(timer.GetVisualDeltaTime()==20.f); // Fixed step overrides scale, as entity dt does.
    assert(timer.GetDeltaTime()==20);
    timer.SetDelta(-1);
    assert(timer.GetVisualDeltaTime()==0.f);
    timer.Run(origin+std::chrono::milliseconds(1010));
    timer.visualDeltaTime*=0.2f;
    assert(std::abs(timer.GetVisualDeltaTime()-2.f)<0.0001f);
    timer.Run(origin+std::chrono::milliseconds(2010));
    assert(timer.GetVisualDeltaTime()==100.f); // Existing long-frame cap retained.
    printf("PASS: 120 frames = %.6f visual ms; legacy = %u ms; pause/fixed/scale/cap\n",elapsed,legacy);
}
