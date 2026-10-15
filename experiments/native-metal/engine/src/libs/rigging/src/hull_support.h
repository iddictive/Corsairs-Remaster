#pragma once
#include "model.h"
#include "ship_base.h"

// Rig points on surviving mast/yard nodes belong to their attachment owner.
// Only a locator on the cut hull itself loses support through hull halfspaces.
class RiggingHullSupport
{
    const SHIP_BASE *ship;
    MODEL *model;
    NODE *hull;
  public:
    struct Stamp
    {
        unsigned revision = 0;
        const MODEL *model = nullptr;
        const NODE *hull = nullptr;
        uint64_t generation = 0;
    };
    RiggingHullSupport(const SHIP_BASE *owner, MODEL *host)
        : ship(owner), model(host), hull(host ? host->GetNode(0) : nullptr) {}
    bool NeedsCheck(Stamp &stamp) const
    {
        const unsigned revision = ship ? ship->GetHullContourRevision() : 0;
        if (!revision || !model || !hull) { stamp = {}; return false; }
        uint64_t generation = 0;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
        if (hull->geo) generation = hull->geo->GeometryGeneration();
#endif
        if (stamp.revision == revision && stamp.model == model &&
            stamp.hull == hull && stamp.generation == generation) return false;
        stamp = {revision, model, hull, generation};
        return true;
    }
    bool Retains(CMatrix *host, const CVECTOR &point) const
    {
        if (!ship || !model || !hull || host != &hull->glob_mtx) return true;
        CVECTOR local;
        model->mtx.MulToInv(*host * point, local);
        return ship->IsHullPositionRetained(local);
    }
};
