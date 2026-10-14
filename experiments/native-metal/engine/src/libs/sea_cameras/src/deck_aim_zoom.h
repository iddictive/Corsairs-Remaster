#pragma once

#include <algorithm>
#include <cmath>

// Transient deck-aim presentation. Magnification multiplies projection scale,
// never the FOV angle; no changes to the saved camera or script perspective.
namespace deck_aim
{
class Zoom
{
  public:
    static constexpr float maximum = 5.f;
    static constexpr float holdDelay = .18f;
    static constexpr float fullTravelTime = 1.2f;
    static constexpr float clickTravelTime = .4f;

    void reset()
    {
        *this = Zoom{};
    }

    float magnification() const { return std::exp(logZoom); }

    float perspective(float base) const
    {
        if (!std::isfinite(base) || base <= 0.f || base >= 3.14159265f)
            return base;
        return 2.f * std::atan(std::tan(base * .5f) / magnification());
    }

    void update(float seconds, bool down)
    {
        // A mode/focus/control interruption requires a fresh press. In
        // particular, releasing the button over a menu is never a click.
        if (needRelease)
        {
            if (!down) needRelease = false;
            return;
        }
        const float realElapsed = std::isfinite(seconds) ? std::max(seconds, 0.f) : 0.f;
        const float dt = std::min(realElapsed, .1f);
        if (down && !pressed)
        {
            pressed = true;
            held = false;
            pressTime = 0.f;
            // Rapid clicks reverse an in-progress endpoint transition.
            tapTarget = targetLog > .0001f ? 0.f : maxLog();
            holdTarget = logZoom >= maxLog() - .0001f ? 0.f : maxLog();
            startLog = logZoom;
            targetLog = logZoom;
            travelTime = 0.f;
            return;
        }
        if (down)
        {
            if (held)
                advance(dt);
            else
            {
                const float untilHold = std::max(0.f, holdDelay - pressTime);
                pressTime = std::min(holdDelay, pressTime + realElapsed);
                if (pressTime >= holdDelay)
                {
                    held = true;
                    begin(holdTarget, fullTravelTime);
                    advance(std::min(dt, std::max(0.f, realElapsed - untilHold)));
                }
            }
            return;
        }
        if (pressed)
        {
            pressed = false;
            // Input is sampled per frame: a release can cross the hold
            // threshold even if the previous down sample was just below it.
            if (!held && pressTime + realElapsed >= holdDelay)
            {
                held = true;
                begin(holdTarget, fullTravelTime);
                advance(std::min(dt, pressTime + realElapsed - holdDelay));
            }
            if (held)
            {
                // Hold release fixes the exact current view, without inertia
                // and without a second, short-click endpoint transition.
                targetLog = startLog = logZoom;
                travelTime = 0.f;
                return;
            }
            begin(tapTarget, clickTravelTime);
        }
        advance(dt);
    }

  private:
    static float maxLog() { return std::log(maximum); }
    void begin(float target, float fullDuration)
    {
        startLog = logZoom;
        targetLog = target;
        elapsed = 0.f;
        travelTime = fullDuration * std::abs(targetLog - startLog) / maxLog();
    }
    void advance(float dt)
    {
        if (travelTime <= 0.f) return;
        elapsed = std::min(travelTime, elapsed + dt);
        const float t = elapsed / travelTime;
        const float eased = t * t * (3.f - 2.f * t);
        logZoom = std::clamp(startLog + (targetLog - startLog) * eased, 0.f, maxLog());
        if (elapsed >= travelTime)
        {
            logZoom = targetLog;
            travelTime = 0.f;
        }
    }
    float logZoom = 0.f, startLog = 0.f, targetLog = 0.f;
    float tapTarget = 0.f, holdTarget = 0.f;
    float pressTime = 0.f, elapsed = 0.f, travelTime = 0.f;
    bool needRelease = true, pressed = false, held = false;
};
} // namespace deck_aim
