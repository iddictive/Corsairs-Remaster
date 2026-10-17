#pragma once
#include <array>
#include <vector>
#include "ship_fire_bridge.hpp"
#include <ship_fire_model.hpp>

namespace storm_ship_fire {

inline StormMetalShipFirePoint renderPoint(const Cell& cell, std::size_t index, float seed) {
    return {{cell.x,cell.y,cell.z},cell.radius,cell.intensity(),cell.charred,cell.heat,
            float(index)*17.31f+seed};
}

// Rendering chooses detail; the complete model and saved state stay untouched.
template<class Accept>
inline std::array<std::size_t,MaxPoints> rankedPoints(const Model& model,Accept accept,std::size_t& count) {
    std::array<std::size_t,MaxPoints> indices{};count=0;
    const auto score=[&](std::size_t i) {
        const auto& c=model.cells[i];
        return c.radius*c.radius*(c.intensity()+c.heat*.25f);
    };
    for (std::size_t i=0;i<model.count;++i) {
        if (!accept(model.cells[i])) continue;
        std::size_t place=0;
        while (place<count && score(indices[place])>=score(i)) ++place;
        if (place==MaxPoints) continue;
        const auto end=std::min(count,MaxPoints-1);
        for (std::size_t move=end;move>place;--move) indices[move]=indices[move-1];
        indices[place]=i;count=std::min(MaxPoints,count+1);
    }
    return indices;
}

template<class Accept>
inline std::vector<StormMetalShipFirePoint> hullSurfacePoints(const Model& model,float seed,Accept accept) {
    std::vector<StormMetalShipFirePoint> points;
    // Every occupied hull char field is authoritative. Cloth can contribute
    // warm light in the remaining slots, with its char confined to its sail.
    for (std::size_t i=0;i<model.count;++i) {
        const auto& c=model.cells[i];
        if (!c.available || c.cloth || (c.heat<=.01f && c.charred<=.001f)) continue;
        // Warm virgin wood contributes neither scorch nor fire light. Its
        // residual smoke belongs to volumePoints, not a scoped hull material.
        if (c.intensity()==0.f && c.charred==0.f) continue;
        if (!accept(c)) continue;
        if (points.empty()) points.reserve(MaxPoints);
        if (points.size()<MaxPoints) points.push_back(renderPoint(c,i,seed));
    }
    std::size_t count;
    const auto cloth=rankedPoints(model,[&](const Cell& c){return c.available&&c.cloth&&c.intensity()>.001f&&accept(c);},count);
    for (std::size_t rank=0;rank<count && points.size()<MaxPoints;++rank) {
        if (points.empty()) points.reserve(MaxPoints);
        auto p=renderPoint(model.cells[cloth[rank]],cloth[rank],seed);p.charred=0.f;points.push_back(p);
    }
    return points;
}

template<class Accept>
inline std::vector<StormMetalShipFirePoint> volumePoints(const Model& model,float seed,Accept accept) {
    std::size_t count;
    const auto indices=rankedPoints(model,[&](const Cell& c){return c.available&&(c.heat>.01f||c.intensity()>.001f)&&accept(c);},count);
    std::vector<StormMetalShipFirePoint> points;points.reserve(count);
    for (std::size_t rank=0;rank<count;++rank) points.push_back(renderPoint(model.cells[indices[rank]],indices[rank],seed));
    return points;
}

} // namespace storm_ship_fire
