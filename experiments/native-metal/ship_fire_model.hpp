#pragma once

#include <algorithm>
#include <limits>
#include <vector>
#include <cmath>
#include <cstddef>
#include <cstdint>

// Ship-local, bounded thermal cells. Geometry, SAVE attributes and damage
// admission belong to SHIP; drawing never advances this model.
namespace storm_ship_fire {
constexpr std::size_t MaxPoints = 48; // fixed rendering ABI, not a simulation capacity
constexpr std::size_t InvalidCell = std::numeric_limits<std::size_t>::max();
constexpr float StepSeconds = .25f;
inline float unit(float x) { return std::isfinite(x) ? std::clamp(x, 0.f, 1.f) : 0.f; }

struct Cell {
    float x = 0.f, y = 0.f, z = 0.f, radius = 2.f;
    float heat = 0.f, fuel = 1.f, charred = 0.f;
    float burned = 0.f; // unconsumed cloth damage; checkpointed with the cell
    int32_t owner = -1;
    bool cloth = false, available = true;
    float intensity() const { return available ? unit((heat - .32f) / .62f) * std::sqrt(unit(fuel)) : 0.f; }
};

struct Context {
    float crewRatio = 1.f, morale = 1.f, repair = .5f;
    float rain = 0.f, windX = 0.f, windZ = 0.f;
    bool priority = false, enabled = true, dead = false;
};

struct Output {
    float hullFraction = 0.f, crewFraction = 0.f, duty = 0.f;
    float intensity = 0.f, heat = 0.f;
    uint32_t active = 0;
    int32_t owner = -1;
};

class Model {
public:
    std::vector<Cell> cells = std::vector<Cell>(MaxPoints);
    std::size_t count = 0;
    float seconds = 0.f;

    // SHIP sizes this once from its preserved hull budget and cloth keys.
    // No scratch allocation occurs in the fixed-step hot path.
    void setCapacity(std::size_t capacity) {
        capacity=std::max(capacity,count);
        cells.resize(capacity);flame.resize(capacity);incoming.resize(capacity);
    }

    std::size_t add(float x, float y, float z, float radius, bool cloth = false) {
        if (count == cells.size()) return InvalidCell;
        auto &c = cells[count];
        c = {}; c.x = x; c.y = y; c.z = z;
        c.radius = std::clamp(radius, .8f, 8.f); c.cloth = cloth;
        return count++;
    }

    std::size_t nearest(float x, float y, float z, bool cloth = false) const {
        std::size_t best = InvalidCell; float closest = 1.e30f;
        for (std::size_t i = 0; i < count; ++i) {
            const auto &c = cells[i];
            if (c.cloth != cloth || !c.available) continue;
            const float dx = x-c.x, dy = y-c.y, dz = z-c.z;
            const float d = dx*dx + dy*dy + dz*dz;
            if (d < closest) { closest = d; best = i; }
        }
        return best;
    }

    void ignite(std::size_t i, float energy, int32_t owner) {
        if (i >= count || !cells[i].available) return;
        auto &c = cells[i];
        // Repeated shells at the same place fill a finite heat budget, never
        // create another independent damage timer or replenish consumed wood.
        c.heat = std::min(1.25f, c.heat + unit(energy) * (1.f - c.heat/1.5f));
        if (owner >= 0 || c.owner < 0) c.owner = owner;
    }

    void restore(std::size_t i, float heat, float fuel, float charred, float burned, int32_t owner) {
        if (i >= count) return;
        auto &c = cells[i];
        c.heat = std::isfinite(heat) ? std::clamp(heat, 0.f, 1.25f) : 0.f;
        c.fuel = unit(fuel); c.charred = unit(charred); c.burned = unit(burned); c.owner = owner;
    }

    Output step(const Context &ctx, float dt = StepSeconds) {
        Output out;
        if (!ctx.enabled || dt <= 0.f || !std::isfinite(dt)) return out;
        dt = std::min(dt, StepSeconds); seconds += dt;
        std::fill(flame.begin(),flame.end(),0.f);
        std::fill(incoming.begin(),incoming.end(),0.f);
        float woodArea = 0.f, woodExposure = 0.f, allArea = 0.f, demand = 0.f;
        for (std::size_t i = 0; i < count; ++i) {
            const auto &c = cells[i];
            if (!c.available) continue;
            flame[i] = c.intensity();
            const float area = c.radius*c.radius;
            allArea += area; demand += area*flame[i];
            if (!c.cloth) { woodArea += area; woodExposure += area*flame[i]; }
            if (flame[i] > out.intensity) { out.intensity = flame[i]; out.owner = c.owner; }
            out.heat = std::max(out.heat, c.heat);
            if (flame[i] > .025f) ++out.active;
        }
        const float coverage = allArea > 0.f ? demand/allArea : 0.f;
        const float crew = std::clamp(ctx.crewRatio, 0.f, 1.25f);
        const float quality = crew*(.45f+.55f*unit(ctx.morale))*(.55f+.45f*unit(ctx.repair));
        const float work = quality*(ctx.priority ? 2.5f : 1.f)/(1.f+coverage*6.f);
        out.duty = ctx.dead || out.active == 0 ? 0.f :
            std::min(ctx.priority ? .48f : .20f, (.025f+coverage*.65f)*(ctx.priority ? 2.4f : 1.f));
        const float wind = unit(std::sqrt(ctx.windX*ctx.windX+ctx.windZ*ctx.windZ)/16.f);
        const float rain = unit(ctx.rain);
        // A single previous-state pass makes spread independent of cell order.
        for (std::size_t i = 0; i < count; ++i) {
            if (flame[i] <= 0.f) continue;
            const auto &a = cells[i];
            for (std::size_t j = 0; j < count; ++j) {
                const auto &b = cells[j];
                if (i == j || !b.available || b.fuel <= .005f) continue;
                const float dx=b.x-a.x, dy=b.y-a.y, dz=b.z-a.z;
                const float vertical = b.cloth && dy > 0.f ? .16f : .8f;
                const float distance = std::sqrt(dx*dx+dz*dz+dy*dy*vertical);
                const float reach = a.radius+b.radius+1.5f;
                if (distance >= reach || dy > 18.f) continue;
                const float downwind = unit(.5f+(dx*ctx.windX+dz*ctx.windZ)/(std::max(1.f,distance)*32.f));
                const float transfer = .042f*flame[i]*(1.f-distance/reach)*(.55f+downwind)*(1.f-.8f*rain);
                incoming[j] += transfer;
                if (transfer > .002f && b.heat < .32f) cells[j].owner = a.owner;
            }
        }
        for (std::size_t i = 0; i < count; ++i) {
            auto &c = cells[i];
            if (!c.available) { c.heat = 0.f; continue; }
            // Cloth burns faster, but rain reaches it and repair crews cool it.
            const float cooling = .010f+rain*(c.cloth ? .23f : .16f)+(ctx.dead ? 0.f : .075f*work);
            const float heating = flame[i]*(.028f+.018f*wind)+std::min(.08f,incoming[i]);
            c.heat = std::clamp(c.heat + dt*(heating-cooling), 0.f, 1.25f);
            const float burned = std::min(c.fuel, dt*flame[i]*(c.cloth ? .0032f : .0013f));
            c.fuel -= burned; c.charred = unit(c.charred+burned*3.3f);
            if (c.cloth) c.burned += burned;
            if (c.fuel <= .005f) c.heat = std::max(0.f,c.heat-dt*.10f);
        }
        if (!ctx.dead && woodArea > 0.f) {
            // Crew are partitioned across deck areas; flames cannot each charge
            // the entire crew. Existing script defences and the crew floor apply.
            const float exposure = woodExposure/woodArea;
            out.hullFraction = dt*(.00065f*exposure+.00006f*out.intensity);
            out.crewFraction = dt*.0010f*exposure*(1.f+out.duty);
        }
        return out;
    }
private:
    std::vector<float> flame = std::vector<float>(MaxPoints);
    std::vector<float> incoming = std::vector<float>(MaxPoints);
};
} // namespace storm_ship_fire
