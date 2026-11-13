#pragma once

#include "reactive_body.hpp"
#include "ship_hull_contact_geometry.hpp"

namespace roundhouse
{
using ReactiveParts = std::vector<std::vector<ReactivePoint>>;

// Support planes enclose the retained material; lower oblique planes form
// the bilges while the top plane remains flat. Damage retains the original
// material; collision and buoyancy use the coarse physical hull.
inline ReactiveParts ReactiveOuterHull(const ReactiveParts &parts)
{
    constexpr size_t sides = 10;
    constexpr size_t planes = sides * 2 + 2;
    constexpr double pi = 3.14159265358979323846;
    struct Normal { double x,y,z; } n[planes];
    for (size_t i = 0; i < sides; ++i) {
        const double a = 2. * pi * i / sides;
        n[i] = {std::cos(a), 0., std::sin(a)};
        n[i+sides] = {std::cos(a), -1., std::sin(a)};
    }
    n[2*sides] = {0.,1.,0.}; n[2*sides+1] = {0.,-1.,0.};
    double support[planes];
    std::fill(std::begin(support), std::end(support), -INFINITY);
    for (const auto &part : parts) for (const auto &p : part)
    {
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z)) return {};
        for (size_t i = 0; i < planes; ++i)
            support[i] = std::max(support[i], p.x*n[i].x + p.y*n[i].y + p.z*n[i].z);
    }
    if (!std::isfinite(support[0])) return {};
    const auto cross = [](Normal a, Normal b) { return Normal{a.y*b.z-a.z*b.y,a.z*b.x-a.x*b.z,a.x*b.y-a.y*b.x}; };
    const auto dot = [](Normal a, Normal b) { return a.x*b.x+a.y*b.y+a.z*b.z; };
    std::vector<ReactivePoint> hull;
    for (size_t i = 0; i < planes; ++i) for (size_t j = i+1; j < planes; ++j)
        for (size_t k = j+1; k < planes; ++k)
    {
        const auto a=cross(n[j],n[k]), b=cross(n[k],n[i]), c=cross(n[i],n[j]);
        const double det=dot(n[i],a);
        if (std::abs(det)<1e-9) continue;
        Normal p{(support[i]*a.x+support[j]*b.x+support[k]*c.x)/det,
                 (support[i]*a.y+support[j]*b.y+support[k]*c.y)/det,
                 (support[i]*a.z+support[j]*b.z+support[k]*c.z)/det};
        bool inside=true;
        for(size_t q=0;q<planes && inside;++q) inside=dot(n[q],p)<=support[q]+1e-8;
        if (inside) hull.push_back({p.x,p.y,p.z});
    }
    if (hull.size()<4) return {};
    return {std::move(hull)};
}

inline std::vector<ReactivePoint> ReactivePrismVertices(const storm::ship::contact::MaterialPiece &piece)
{
    std::vector<ReactivePoint> vertices;
    for (const auto &p : piece.hull.points)
    {
        vertices.push_back({p.x, piece.lowerY, p.z});
        vertices.push_back({p.x, piece.upperY, p.z});
    }
    return vertices;
}

struct ReactiveCollisionGroup
{
    std::vector<ReactivePoint> vertices;
    // Keys bind this derived collider to its canonical current material.
    std::vector<storm::ship::material::convex::Key> members;
};

inline double ReactivePrismDistance(const storm::ship::contact::MaterialPiece &prism,
                                   const ReactivePoint &point)
{
    bool positive = false, negative = false;
    double planarDistance2 = INFINITY;
    const auto &ring = prism.hull.points;
    for (size_t n = 0; n < ring.size(); ++n)
    {
        const auto a = ring[n], b = ring[(n + 1) % ring.size()];
        const double x = b.x-a.x, z = b.z-a.z;
        const double cross = x*(point.z-a.z)-z*(point.x-a.x);
        positive |= cross > 0.; negative |= cross < 0.;
        const double t = std::clamp(((point.x-a.x)*x+(point.z-a.z)*z)/(x*x+z*z), 0., 1.);
        const double dx = point.x-a.x-t*x, dz = point.z-a.z-t*z;
        planarDistance2 = std::min(planarDistance2, dx*dx+dz*dz);
    }
    if (!positive || !negative) planarDistance2 = 0.;
    const double y = std::max({prism.lowerY-point.y, 0., point.y-prism.upperY});
    return std::sqrt(planarDistance2+y*y);
}

inline std::vector<ReactiveCollisionGroup> ReactiveCollisionGroups(
    const storm::ship::material::convex::ContactSnapshot &snapshot,
    const storm::ship::material::ContactTransform &transform)
{
    using namespace storm::ship::material;
    if (!snapshot.cells || !snapshot.cells->convex) return {};
    const auto &source = *snapshot.cells->convex;
    const auto retained = [&](const convex::Piece &piece) {
        return !snapshot.retainedSupports || piece.supportCell == storm_ship_structure::NoCell ||
            (piece.supportCell < snapshot.retainedSupports->size() && snapshot.retainedSupports->at(piece.supportCell));
    };
    // A cut can leave locally connected wood around an opening. Keep the
    // original colliders throughout every touched support cell, including
    // source pieces entirely removed by that cut.
    std::set<uint32_t> cutSupports;
    for (const auto &[id, boundary] : snapshot.cells->changed)
    {
        if (id < source.source->pieces.size() && source.source->pieces[id])
            for (const auto &piece : *source.source->pieces[id]) cutSupports.insert(piece.supportCell);
        if (id < source.cells.size() && source.cells[id] && source.cells[id]->pieces)
            for (const auto &piece : *source.cells[id]->pieces) cutSupports.insert(piece.supportCell);
    }
    // Reuse the native positive-contact graph and component traversal. The
    // local projection only removes graph edges; it never joins material.
    auto local = source;
    for (size_t id = 0; id < source.cells.size(); ++id)
    {
        const auto &cell = source.cells[id];
        if (!cell || !cell->pieces) continue;
        auto copy = std::make_shared<convex::Cell>(*cell);
        copy->edges.resize(cell->pieces->size());
        for (size_t n = 0; n < cell->pieces->size(); ++n)
        {
            const auto &piece = cell->pieces->at(n);
            if (!retained(piece) || piece.supportCell == storm_ship_structure::NoCell ||
                cutSupports.contains(piece.supportCell))
            {
                copy->edges[n].clear();
                continue;
            }
            std::erase_if(copy->edges[n], [&](convex::Key key) {
                if (key.cell >= source.cells.size() || !source.cells[key.cell] ||
                    !source.cells[key.cell]->pieces || key.piece >= source.cells[key.cell]->pieces->size()) return true;
                const auto &other = source.cells[key.cell]->pieces->at(key.piece);
                return !retained(other) || other.supportCell != piece.supportCell ||
                    other.authoredOwner != piece.authoredOwner;
            });
        }
        local.cells[id] = std::move(copy);
    }
    convex::Budget budget;
    if (!convex::components(local, budget)) return {};
    std::vector<ReactiveCollisionGroup> result;
    for (const auto &component : local.components)
    {
        std::vector<storm::ship::contact::MaterialPiece> prisms;
        ReactiveParts vertices;
        std::vector<convex::Key> keys;
        std::map<convex::Key, size_t> positions;
        double maximumCoordinate = 1.;
        for (auto key : component.pieces)
        {
            const auto &piece = source.cells[key.cell]->pieces->at(key.piece);
            if (!retained(piece)) continue;
            storm::ship::contact::MaterialSnapshot prism;
            if (!appendContactPiece(prism, piece, key.cell, transform)) return {};
            auto points = ReactivePrismVertices(prism.pieces.front());
            for (const auto &p : points) maximumCoordinate = std::max({maximumCoordinate,std::abs(p.x),std::abs(p.y),std::abs(p.z)});
            positions.emplace(key, keys.size()); keys.push_back(key);
            prisms.push_back(std::move(prism.pieces.front()));
            vertices.push_back(std::move(points));
        }
        std::vector<size_t> anchors(keys.size());
        std::iota(anchors.begin(), anchors.end(), 0);
        const auto volume = [&](size_t n) {
            const auto &p = prisms[n]; double area2 = 0.;
            for (size_t i=0;i<p.hull.points.size();++i) { const auto a=p.hull.points[i],b=p.hull.points[(i+1)%p.hull.points.size()]; area2+=a.x*b.z-a.z*b.x; }
            return std::abs(area2)*(p.upperY-p.lowerY);
        };
        std::stable_sort(anchors.begin(), anchors.end(), [&](size_t a,size_t b) { return volume(a)>volume(b); });
        std::vector<uint8_t> assigned(keys.size());
        // Distance to a convex set is convex. If every retained vertex lies
        // inside this anchor's 2cm offset, its entire convex hull does too.
        // Leave headroom for float collider construction; input stays double.
        const double cover = std::max(0., .02-32*std::numeric_limits<float>::epsilon()*maximumCoordinate);
        for (auto anchor : anchors)
        {
            if (assigned[anchor]) continue;
            std::vector<size_t> members{anchor}; assigned[anchor]=1;
            ReactiveCollisionGroup group;
            for (size_t at=0;at<members.size();++at)
            {
                const auto member=members[at];
                const auto key=keys[member];
                group.vertices.insert(group.vertices.end(),vertices[member].begin(),vertices[member].end());
                group.members.push_back(key);
                for (auto edge : local.cells[key.cell]->edges[key.piece])
                {
                    const auto found=positions.find(edge);
                    if (found==positions.end() || assigned[found->second]) continue;
                    const auto n=found->second;
                    if (!std::all_of(vertices[n].begin(),vertices[n].end(),[&](const ReactivePoint &p) { return ReactivePrismDistance(prisms[anchor],p)<=cover; })) continue;
                    assigned[n]=1; members.push_back(n);
                }
            }
            result.push_back(std::move(group));
        }
    }
    return result;
}

inline ReactiveParts ReactiveCollisionParts(const storm::ship::material::convex::ContactSnapshot &snapshot,
                                           const storm::ship::material::ContactTransform &transform)
{
    auto groups = ReactiveCollisionGroups(snapshot, transform);
    ReactiveParts parts;
    parts.reserve(groups.size());
    for (auto &group : groups) parts.push_back(std::move(group.vertices));
    return parts;
}
} // namespace roundhouse
