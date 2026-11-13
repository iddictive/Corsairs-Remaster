#pragma once

#include "c_vector.h"
#include <algorithm>

namespace deck_fall
{
enum class Phase { aboard, falling, water };
struct State
{
    Phase phase = Phase::aboard;
    CVECTOR position{}, velocity{};
    float yaw = 0.f;
};

inline void detach(State &state, const CVECTOR &position, const CVECTOR &velocity, float yaw)
{
    state = {};
    state.phase = Phase::falling;
    state.position = position;
    state.velocity = velocity;
    state.yaw = yaw;
}

template<class Sweep, class Wave>
void fall(State &state, float dt, Sweep sweep, Wave wave)
{
    if (state.phase != Phase::falling || !(dt > 0.f)) return;
    while (dt > 0.f)
    {
        const float step = std::min(dt, .025f);
        dt -= step;
        state.velocity.y -= 9.81f * step;
        const CVECTOR delta = state.velocity * step;
        float fraction = 1.f;
        CVECTOR normal{};
        if (sweep(state.position, delta, fraction, normal))
        {
            fraction = std::clamp(fraction, 0.f, 1.f);
            state.position += delta * std::max(0.f, fraction - .001f);
            const float inward = state.velocity | normal;
            if (inward < 0.f) state.velocity -= normal * inward;
            const CVECTOR slide = state.velocity * (step * (1.f - fraction));
            state.position += normal * .002f;
            float slideFraction = 1.f;
            CVECTOR slideNormal{};
            if (sweep(state.position, slide, slideFraction, slideNormal))
            {
                state.position += slide * std::max(0.f, std::clamp(slideFraction, 0.f, 1.f) - .001f);
                const float intoSlide = state.velocity | slideNormal;
                if (intoSlide < 0.f) state.velocity -= slideNormal * intoSlide;
            }
            else state.position += slide;
        }
        else state.position += delta;
        const float level = wave(state.position.x, state.position.z);
        if (state.position.y <= level)
        {
            state.position.y = level;
            state.velocity = CVECTOR(0.f);
            state.phase = Phase::water;
            return;
        }
    }
}
}
