#pragma once

#include "engine/src/libs/touch/src/ship_contact.hpp"
#include <memory>
#include <map>
#include <numeric>
#include <cstdint>

namespace storm::ship::contact
{
struct MaterialPiece
{
    detail::Hull hull;
    double lowerY{}, upperY{}; // model-local yaw-only vertical extent
    uint32_t source{};
    uint32_t supportCell{UINT32_MAX};
    double volume{};
};
struct MaterialBounds
{
    double loX{}, hiX{}, loZ{}, hiZ{}, lowerY{}, upperY{}, radius{};
};
struct MaterialNode
{
    MaterialBounds bounds;
    std::size_t left{}, right{}, piece{std::numeric_limits<std::size_t>::max()};
    uint32_t supportCell{UINT32_MAX};
};
struct MaterialSnapshot
{
    std::vector<MaterialPiece> pieces;
    std::vector<MaterialNode> index;
    std::map<uint32_t, MaterialBounds> sourceBounds;
    std::shared_ptr<const MaterialSnapshot> base;
    std::map<uint32_t, std::vector<MaterialPiece>> replaced;
    unsigned overlayDepth{};
    std::shared_ptr<const std::vector<uint8_t>> retainedSupports;
    double authoredVolume{}, retainedVolume{};
    std::shared_ptr<const std::vector<double>> supportVolumes;
};
inline const MaterialSnapshot &materialRoot(const MaterialSnapshot &snapshot)
{
    const MaterialSnapshot *root = &snapshot;
    while (root->base) root = root->base.get();
    return *root;
}
inline const std::vector<MaterialPiece> *materialReplacement(const MaterialSnapshot &snapshot, uint32_t source)
{
    for (const MaterialSnapshot *at = &snapshot; at; at = at->base.get())
        if (const auto found = at->replaced.find(source); found != at->replaced.end()) return &found->second;
    return nullptr;
}
struct MaterialVertex { double x{}, y{}, z{}; };
inline bool appendMaterialPiece(MaterialSnapshot &snapshot, const std::vector<MaterialVertex> &vertices, uint32_t source,
                                uint32_t supportCell = UINT32_MAX)
{
    Body body; body.mass = 1;
    double lower = INFINITY, upper = -INFINITY;
    for (const auto v : vertices)
    {
        if (!std::isfinite(v.x) || !std::isfinite(v.y) || !std::isfinite(v.z)) return false;
        lower = std::min(lower, v.y); upper = std::max(upper, v.y);
        body.hull.push_back({v.x, v.z});
    }
    body.hull = detail::convexHull(std::move(body.hull));
    auto hull = detail::prepare(body);
    if (hull.points.empty() || !(upper > lower)) return false;
    snapshot.pieces.push_back({std::move(hull), lower, upper, source, supportCell});
    return true;
}
// Publication/cache preparation only. Queries below never rebuild this tree
// or truncate a large vessel's complete physical representation.
inline void buildMaterialIndex(MaterialSnapshot &snapshot, void (*checkpoint)() = nullptr)
{
    snapshot.index.clear();
    snapshot.sourceBounds.clear();
    if (snapshot.pieces.empty()) return;
    std::vector<std::size_t> ids(snapshot.pieces.size());
    std::iota(ids.begin(), ids.end(), 0);
    std::vector<MaterialBounds> bounds; bounds.reserve(ids.size());
    for (const auto &piece : snapshot.pieces)
    {
        if (checkpoint) checkpoint();
        MaterialBounds b{INFINITY, -INFINITY, INFINITY, -INFINITY, piece.lowerY, piece.upperY, piece.hull.radius};
        for (const auto p : piece.hull.points)
        { b.loX = std::min(b.loX, p.x); b.hiX = std::max(b.hiX, p.x); b.loZ = std::min(b.loZ, p.z); b.hiZ = std::max(b.hiZ, p.z); }
        bounds.push_back(b);
        auto [found, inserted] = snapshot.sourceBounds.emplace(piece.source, b);
        if (!inserted)
        {
            auto &current = found->second;
            current.loX = std::min(current.loX, b.loX); current.hiX = std::max(current.hiX, b.hiX);
            current.loZ = std::min(current.loZ, b.loZ); current.hiZ = std::max(current.hiZ, b.hiZ);
            current.lowerY = std::min(current.lowerY, b.lowerY); current.upperY = std::max(current.upperY, b.upperY);
            current.radius = std::max(current.radius, b.radius);
        }
    }
    snapshot.index.reserve(ids.size() * 2);
    const auto build = [&](auto &&self, std::size_t begin, std::size_t end) -> std::size_t
    {
        if (checkpoint) checkpoint();
        const auto node = snapshot.index.size(); snapshot.index.push_back({});
        auto b = bounds[ids[begin]];
        uint32_t support = snapshot.pieces[ids[begin]].supportCell;
        for (auto n = begin + 1; n < end; ++n)
        {
            if (checkpoint) checkpoint();
            const auto &c = bounds[ids[n]];
            if (snapshot.pieces[ids[n]].supportCell != support) support = UINT32_MAX;
            b.loX = std::min(b.loX, c.loX); b.hiX = std::max(b.hiX, c.hiX);
            b.loZ = std::min(b.loZ, c.loZ); b.hiZ = std::max(b.hiZ, c.hiZ);
            b.lowerY = std::min(b.lowerY, c.lowerY); b.upperY = std::max(b.upperY, c.upperY);
            b.radius = std::max(b.radius, c.radius);
        }
        snapshot.index[node].bounds = b;
        snapshot.index[node].supportCell = support;
        if (end - begin == 1) snapshot.index[node].piece = ids[begin];
        else
        {
            const double dx = b.hiX - b.loX, dz = b.hiZ - b.loZ, dy = b.upperY - b.lowerY;
            const int axis = dy > dx && dy > dz ? 1 : (dz > dx ? 2 : 0);
            const auto middle = begin + (end - begin) / 2;
            std::nth_element(ids.begin() + begin, ids.begin() + middle, ids.begin() + end,
                [&](auto x, auto y) {
                    const auto &a = bounds[x], &c = bounds[y];
                    return axis == 0 ? a.loX + a.hiX < c.loX + c.hiX :
                           axis == 1 ? a.lowerY + a.upperY < c.lowerY + c.upperY : a.loZ + a.hiZ < c.loZ + c.hiZ;
                });
            const auto left = self(self, begin, middle), right = self(self, middle, end);
            snapshot.index[node].left = left; snapshot.index[node].right = right;
        }
        return node;
    };
    build(build, 0, ids.size());
}
// A contact edit only removes material. Its old source bounds stay conservative,
// so bounded changed-source overlays share the complete worker-built BVH. Eight
// pending edits require a worker rebase before another physical commit.
inline std::shared_ptr<const MaterialSnapshot> applyMaterialReplacements(
    const std::shared_ptr<const MaterialSnapshot> &base, std::map<uint32_t, std::vector<MaterialPiece>> replacements,
    std::shared_ptr<const std::vector<uint8_t>> retainedSupports = {}, double retainedVolume = -1)
{
    if (!base || base->overlayDepth >= 8 || (replacements.empty() && !retainedSupports)) return {};
    const auto &root = materialRoot(*base);
    for (const auto &entry : replacements)
    {
        const auto bounds = root.sourceBounds.find(entry.first);
        if (bounds == root.sourceBounds.end()) return {};
        const auto &b = bounds->second;
        const double tolerance = 1e-5 * std::max(1.0, b.radius);
        for (const auto &piece : entry.second)
        {
            if (piece.source != entry.first || piece.hull.points.empty() ||
                !std::isfinite(piece.lowerY) || !std::isfinite(piece.upperY) || !(piece.upperY > piece.lowerY) ||
                piece.lowerY < b.lowerY - tolerance || piece.upperY > b.upperY + tolerance) return {};
            for (const auto p : piece.hull.points)
                if (!detail::finite(p) || p.x < b.loX - tolerance || p.x > b.hiX + tolerance ||
                    p.z < b.loZ - tolerance || p.z > b.hiZ + tolerance) return {};
        }
    }
    auto next = std::make_shared<MaterialSnapshot>();
    next->base = base; next->overlayDepth = base->overlayDepth + 1; next->replaced = std::move(replacements);
    next->retainedSupports = std::move(retainedSupports);
    next->authoredVolume=base->authoredVolume;next->retainedVolume=retainedVolume>=0?retainedVolume:base->retainedVolume;
    next->supportVolumes=base->supportVolumes;
    return next;
}
inline double retainedMassFraction(const MaterialSnapshot &snapshot)
{
    return snapshot.authoredVolume>0?std::clamp(snapshot.retainedVolume/snapshot.authoredVolume,0.0,1.0):1.0;
}
inline bool materialSupportRetained(const MaterialSnapshot &snapshot, uint32_t support)
{
    if (support == UINT32_MAX) return true;
    for (const MaterialSnapshot *at = &snapshot; at; at = at->base.get())
        if (at->retainedSupports) return support < at->retainedSupports->size() && (*at->retainedSupports)[support] != 0;
    return true;
}
} // namespace storm::ship::contact
