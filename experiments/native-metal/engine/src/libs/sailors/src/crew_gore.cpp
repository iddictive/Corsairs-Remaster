#include "crew_gore.h"

#include "animation.h"
#include "core.h"
#include "dx9render.h"
#include "geometry.h"
#include "model.h"
#include "sea_base.h"
#include "ship_laser_geometry.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace
{
using namespace storm_ship_hull;
using Mesh = storm_ship_laser::Mesh;
constexpr size_t maxFragments = 96;
constexpr size_t maxSourceTriangles = 8192;
constexpr size_t maxPieceTriangles = 16384;

CVECTOR vector(Vec3 v) { return {v.x, v.y, v.z}; }
Vec3 vector(const CVECTOR &v) { return {v.x, v.y, v.z}; }
CVECTOR direction(const CMatrix &m, const CVECTOR &v)
{
    return {m.matrix[0] * v.x + m.matrix[4] * v.y + m.matrix[8] * v.z,
            m.matrix[1] * v.x + m.matrix[5] * v.y + m.matrix[9] * v.z,
            m.matrix[2] * v.x + m.matrix[6] * v.y + m.matrix[10] * v.z};
}
CVECTOR point(const CMatrix &m, const CVECTOR &v)
{
    return direction(m, v) + CVECTOR(m.matrix[12], m.matrix[13], m.matrix[14]);
}
size_t triangleCount(const Mesh &mesh)
{
    size_t n = 0;
    for (const auto &object : mesh) n += object.second.size();
    return n;
}

struct Material
{
    VDX9RENDER *renderer = nullptr;
    GEOS::MATERIAL source{};
    std::string technique;
    std::array<int32_t, 4> retained{{-1, -1, -1, -1}};
    ~Material()
    {
        for (int32_t texture : retained)
            if (texture >= 0) renderer->TextureRelease(texture);
    }
};
struct Batch
{
    std::shared_ptr<Material> material;
    std::vector<GEOS::VERTEX0> vertices;
};
struct Fragment
{
    std::vector<Batch> batches;
    CVECTOR position{0.f}, velocity{0.f}, angle{0.f}, spin{0.f};
    float age = 0.f, life = 0.f;
    bool wet = false;
    uint64_t capture = 0;
    float supportRadius = 0.0f;
};

// Split in bind space, carrying current posed positions through the unused UV
// channels. A cut therefore follows the same anatomical boundary in a climbing
// or reloading pose instead of slicing arbitrary world-space horizontal slabs.
std::pair<Mesh, Mesh> split(const Mesh &mesh, Vec3 normal, float distance)
{
    storm_ship_laser::Plane plane;
    plane.normal = normal;
    plane.distance = distance;
    plane.charred = false;
    auto result = storm_ship_laser::planarSplit(mesh, plane, false, true);
    return {std::move(result.kept), std::move(result.detached)};
}

bool poseAndCap(Mesh &mesh, bool laser)
{
    // Each clipped body's open boundary is reconstructed from its actual posed
    // edges, including edges split across UV/material seams. Cap uses the same
    // maintained ring triangulator as ship cuts, with a distinct flesh material.
    using Key = storm_ship_laser::PositionKey;
    struct Edge { Vertex a, b; unsigned count = 0; };
    std::map<std::pair<Key, Key>, Edge> edges;
    for (auto &object : mesh)
        for (auto &triangle : object.second)
        {
            for (auto &v : triangle) v.position = {v.uv[2], v.uv[3], v.uv[4]};
            for (unsigned n = 0; n < 3; ++n)
            {
                auto a = storm_ship_laser::positionKey(triangle[n].position);
                auto b = storm_ship_laser::positionKey(triangle[(n + 1) % 3].position);
                if (a == b) continue;
                if (b < a) std::swap(a, b);
                auto &edge = edges[{a, b}];
                if (!edge.count) { edge.a = triangle[n]; edge.b = triangle[(n + 1) % 3]; }
                ++edge.count;
            }
        }
    // Separate loops have different joint normals; reconstruct one connected
    // boundary at a time rather than projecting all wounds onto a single plane.
    std::vector<storm_ship_laser::Segment> open;
    for (const auto &entry : edges)
        if (entry.second.count == 1) open.push_back({entry.second.a, entry.second.b, -1});
    // Malformed/open assets cannot turn an impact into an unbounded ring walk.
    if (open.size() > 2048) return false;
    while (!open.empty())
    {
        std::vector<storm_ship_laser::Segment> loop{open.back()};
        open.pop_back();
        for (size_t i = 0; i < loop.size(); ++i)
            for (size_t n = 0; n < open.size();)
            {
                if (storm_ship_laser::near(loop[i].a.position, open[n].a.position) ||
                    storm_ship_laser::near(loop[i].a.position, open[n].b.position) ||
                    storm_ship_laser::near(loop[i].b.position, open[n].a.position) ||
                    storm_ship_laser::near(loop[i].b.position, open[n].b.position))
                { loop.push_back(open[n]); open.erase(open.begin() + n); }
                else ++n;
            }
        if (loop.size() < 3) continue;
        Vec3 normal{};
        const auto center = loop.front().a.position;
        for (const auto &edge : loop)
            normal = normal + cross(edge.a.position - center, edge.b.position - center);
        if (length(normal) < 1.e-7f) continue;
        storm_ship_laser::Plane plane;
        // The open edge follows the retained face winding; cap faces oppose it.
        plane.normal = normalized(normal) * -1.f;
        plane.distance = dot(plane.normal, center);
        plane.charred = false;
        Mesh caps, other;
        storm_ship_laser::cap(loop, plane, caps, other);
        for (auto &object : caps)
            for (auto &triangle : object.second)
            {
                for (auto &v : triangle) v.color = laser ? 0xff6c1720u : 0xffa32a36u;
                mesh[-1].push_back(triangle);
            }
    }
    return true;
}

std::shared_ptr<Material> retainMaterial(GEOS &geometry, int index, NODE &node,
                                       VDX9RENDER &renderer)
{
    auto result = std::make_shared<Material>();
    result->renderer = &renderer;
    geometry.GetMaterial(index, result->source);
    result->source.name = result->source.group_name = nullptr;
    const auto *technique = node.GetTechnique();
    result->technique = technique && technique[0] ? technique : "Animation";
    for (unsigned n = 0; n < 4; ++n)
    {
        const auto texture = result->source.texture[n];
        if (result->source.texture_type[n] != GEOS::TEXTURE_NONE && texture >= 0)
        {
            // GetMaterial exposes renderer ids; GetTexture owns their lookup.
            // Retain only a texture belonging to this geometry's texture table.
            GEOS::INFO info{};
            geometry.GetInfo(info);
            bool found = false;
            for (int t = 0; t < info.ntextures; ++t) found |= geometry.GetTexture(t) == texture;
            if (!found || !renderer.TextureIncReference(texture)) return {};
            result->retained[n] = texture;
        }
    }
    return result;
}

void applyMaterial(VDX9RENDER &renderer, const Material &material)
{
    const auto &source = material.source;
    storm_render::Material value{};
    value.Diffuse = {source.diffuse, source.diffuse, source.diffuse, 1.f};
    value.Specular = {source.specular, source.specular, source.specular, 1.f};
    value.Power = source.gloss;
    renderer.SetMaterial(value);
    renderer.TextureSet(0, material.retained[0]);
    renderer.SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    renderer.SetTextureStageState(0, storm_render::StageColorArg2, storm_render::textureArg::texture);
    renderer.SetTextureStageState(0, storm_render::StageColorOp,
        material.retained[0] >= 0 ? storm_render::TextureOpModulate2X : storm_render::TextureOpSelectArg1);
    renderer.SetTextureStageState(1, storm_render::StageColorOp, storm_render::TextureOpDisable);
    renderer.SetTextureStageState(2, storm_render::StageColorOp, storm_render::TextureOpDisable);
    renderer.SetRenderState(storm_render::RenderBlend, FALSE);
}
}

struct CrewGore::Impl
{
    std::vector<Fragment> fragments;
    uint32_t seed = 0xc0de51u;
    uint64_t nextCapture = 0;
};

CrewGore::CrewGore() : impl_(std::make_unique<Impl>()) {}
CrewGore::~CrewGore() = default;

bool CrewGore::capture(MODEL &model, VDX9RENDER &renderer, CVECTOR impulse, bool laser, uint64_t *token)
{
    if (token) *token = 0;
    if (!finite(vector(impulse))) return false;
    auto *geometryService = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    auto *animation = model.GetAnimation();
    if (!geometryService || !animation || animation->GetNumBones() <= 0) return false;
    model.Update();
    Mesh source;
    std::vector<std::shared_ptr<Material>> materials;
    CMatrix modelWorld = model.mtx;
    CMatrix inverse = modelWorld;
    inverse.Transposition();
    size_t sourceTriangles = 0;
    for (int n = 0; n < 16; ++n)
    {
        auto *node = model.GetNode(n);
        if (!node) break;
        if (!node->geo || !(node->flags & NODE::VISIBLE)) continue;
        GEOS::INFO info{};
        node->geo->GetInfo(info);
        if (info.nobjects < 0 || info.nobjects > 128 || info.ntriangles < 0 ||
            size_t(info.ntriangles) + sourceTriangles > maxSourceTriangles) return false;
        const int ib = node->geo->GetIndexBuffer();
        auto *indices = static_cast<const uint16_t *>(renderer.LockIndexBuffer(ib));
        if (!indices) return false;
        // Copy before reading any other buffer; no lock survives model deletion.
        std::vector<uint16_t> indexCopy(indices, indices + size_t(info.ntriangles) * 3);
        renderer.UnLockIndexBuffer(ib);
        for (int o = 0; o < info.nobjects; ++o)
        {
            GEOS::OBJECT object{};
            node->geo->GetObj(o, object);
            if (!(object.flags & GEOS::VISIBLE) || !object.ntriangles) continue;
            if (object.material < 0 || object.material >= info.nmaterials || object.striangle < 0 ||
                object.ntriangles < 0 || object.striangle > info.ntriangles - object.ntriangles ||
                object.start_vertex < 0 || object.num_vertices <= 0) return false;
            auto descriptor = geometryService->GetAnimationVBDesc(object.vertex_buff);
            // Sailor skinning is explicitly identified by the geometry owner,
            // never guessed from the renderer's legacy 36-byte FVF/stride.
            if (!descriptor.buff || descriptor.nvertices <= 0 ||
                object.start_vertex > descriptor.nvertices - object.num_vertices) return false;
            auto material = retainMaterial(*node->geo, object.material, *node, renderer);
            if (!material) return false;
            const int materialId = int(materials.size());
            materials.push_back(std::move(material));
            auto &triangles = source[materialId];
            const auto *vertices = static_cast<const GEOS::AVERTEX0 *>(descriptor.buff);
            for (int t = 0; t < object.ntriangles; ++t)
            {
                Triangle triangle;
                for (unsigned c = 0; c < 3; ++c)
                {
                    const auto index = indexCopy[size_t(object.striangle + t) * 3 + c];
                    if (index >= object.num_vertices) return false;
                    const auto &v = vertices[object.start_vertex + index];
                    const auto bone0 = v.boneid & 255u, bone1 = (v.boneid >> 8) & 255u;
                    if (bone0 >= unsigned(animation->GetNumBones()) || bone1 >= unsigned(animation->GetNumBones()) ||
                        !std::isfinite(v.weight)) return false;
                    const auto &a = animation->GetAnimationMatrix(bone0);
                    const auto &b = animation->GetAnimationMatrix(bone1);
                    CVECTOR bind(v.pos.x, v.pos.y, v.pos.z), normal(v.nrm.x, v.nrm.y, v.nrm.z);
                    auto posed = point(a, bind) * v.weight + point(b, bind) * (1.f - v.weight);
                    auto posedNormal = direction(a, normal) * v.weight + direction(b, normal) * (1.f - v.weight);
                    posed.x = -posed.x;
                    posedNormal.x = -posedNormal.x;
                    // The root and its attachments already carry the full model
                    // transform. Store model-local pose for independent spin.
                    const auto local = inverse * (node->glob_mtx * posed);
                    const auto localNormal = direction(inverse, direction(node->glob_mtx, posedNormal));
                    if (!finite(vector(bind)) || !finite(vector(local)) || !finite(vector(localNormal))) return false;
                    triangle[c].position = vector(bind);
                    triangle[c].normal = normalized(vector(localNormal));
                    triangle[c].color = uint32_t(v.color);
                    triangle[c].uv = {v.tu0, v.tv0, local.x, local.y, local.z, 0.f, 0.f, 0.f};
                }
                triangles.push_back(triangle);
            }
        }
        sourceTriangles += info.ntriangles;
    }
    if (model.GetNode(16)) return false;
    if (source.empty()) return false;
    const auto bounds = storm_ship_laser::bounds(source);
    const auto size = bounds.hi - bounds.lo;
    if (size.y < .1f || size.x < .05f) return false;
    auto head = split(source, {0, 1, 0}, bounds.lo.y + size.y * .82f);
    auto legs = split(head.first, {0, 1, 0}, bounds.lo.y + size.y * .48f);
    auto feet = split(legs.first, {1, 0, 0}, (bounds.lo.x + bounds.hi.x) * .5f);
    const float centerX = (bounds.lo.x + bounds.hi.x) * .5f;
    auto rightArm = split(legs.second, {1, 0, 0}, centerX + size.x * .2f);
    auto leftArm = split(rightArm.first, {-1, 0, 0}, -centerX + size.x * .2f);
    std::vector<Mesh> pieces;
    pieces.push_back(std::move(head.second));
    pieces.push_back(std::move(feet.first));
    pieces.push_back(std::move(feet.second));
    pieces.push_back(std::move(rightArm.second));
    pieces.push_back(std::move(leftArm.second));
    if (laser) pieces.push_back(std::move(leftArm.first));
    else
    {
        auto torso = split(leftArm.first, {0, 1, 0}, bounds.lo.y + size.y * .65f);
        pieces.push_back(std::move(torso.first));
        pieces.push_back(std::move(torso.second));
    }
    auto flesh = std::make_shared<Material>();
    flesh->renderer = &renderer;
    flesh->source.diffuse = .85f;
    flesh->source.specular = .12f;
    flesh->source.gloss = 14.f;
    flesh->technique = "Animation";
    std::vector<Fragment> captured;
    for (auto &piece : pieces)
    {
        if (!triangleCount(piece)) continue;
        if (!poseAndCap(piece, laser)) return false;
        if (triangleCount(piece) > maxPieceTriangles) return false;
        const auto properties = storm_ship_laser::fragmentProperties(piece);
        if (properties.surfaceArea < 1.e-6f) continue;
        Fragment fragment;
        const auto dimensions = properties.bounds.hi - properties.bounds.lo;
        fragment.supportRadius = std::max(.035f, std::min({dimensions.x, dimensions.y, dimensions.z}) * .5f);
        fragment.position = modelWorld * vector(properties.centroid);
        const auto radial = normalized(vector(fragment.position - modelWorld.Pos()) + Vec3{0, .2f, 0});
        const auto random = [&]() { return randomUnit(impl_->seed); };
        fragment.velocity = impulse + vector(radial) * (1.f + 3.f * random());
        fragment.velocity.y += 2.5f + 3.f * random();
        fragment.spin = CVECTOR(random() - .5f, random() - .5f, random() - .5f) * 12.f;
        fragment.life = 9.f + 4.f * random();
        for (const auto &object : piece)
        {
            if (object.second.empty()) continue;
            Batch batch;
            batch.material = object.first < 0 ? flesh : materials.at(object.first);
            batch.vertices.reserve(object.second.size() * 3);
            for (const auto &triangle : object.second)
                for (const auto &v : triangle)
                {
                    // Preserve the ship's orientation at severance, then spin
                    // rigidly around this piece's own surface centroid.
                    const auto position = direction(modelWorld, vector(v.position - properties.centroid));
                    const auto normal = direction(modelWorld, vector(v.normal));
                    batch.vertices.push_back({{position.x, position.y, position.z},
                        {normal.x, normal.y, normal.z}, int32_t(v.color), v.uv[0], v.uv[1]});
                }
            fragment.batches.push_back(std::move(batch));
        }
        captured.push_back(std::move(fragment));
    }
    if (captured.size() < 2) return false;
    // Whole captures replace the oldest fragments; no partial man on overflow.
    const size_t overflow = impl_->fragments.size() + captured.size() > maxFragments ?
        impl_->fragments.size() + captured.size() - maxFragments : 0;
    impl_->fragments.erase(impl_->fragments.begin(), impl_->fragments.begin() + overflow);
    const uint64_t capture = ++impl_->nextCapture;
    for (auto &fragment : captured)
    {
        fragment.capture = capture;
        impl_->fragments.push_back(std::move(fragment));
    }
    if (token) *token = capture;
    return true;
}

bool CrewGore::hasCapture(uint64_t token) const
{
    return token && std::any_of(impl_->fragments.begin(), impl_->fragments.end(),
                              [token](const auto &fragment) { return fragment.capture == token; });
}

void CrewGore::discardCapture(uint64_t token)
{
    std::erase_if(impl_->fragments, [token](const auto &fragment) { return fragment.capture == token; });
}

void CrewGore::updateDraw(VDX9RENDER &renderer, SEA_BASE *sea, float dt, bool draw, const GroundTrace &ground)
{
    if (!std::isfinite(dt) || dt < 0.f) return;
    const float elapsed = dt;
    // Substeps bound ballistic integration after a stalled frame. Fragment
    // lifetime still consumes all elapsed time, including frames without draws.
    dt = std::min(dt, .25f);
    const int steps = std::max(1, int(std::ceil(dt / .025f)));
    const float step = dt / float(steps);
    for (auto &fragment : impl_->fragments)
    {
        fragment.age += elapsed;
        for (int s = 0; s < steps; ++s)
        {
            const CVECTOR previous = fragment.position;
            fragment.velocity.y -= (fragment.wet ? 1.5f : 9.81f) * step;
            fragment.position += fragment.velocity * step;
            fragment.angle += fragment.spin * step;
            if (ground && step > 0.0f)
            {
                CVECTOR contact, normal;
                const CVECTOR support(0.0f, fragment.supportRadius, 0.0f);
                if (ground(previous - support, fragment.position - support, contact, normal))
                {
                    fragment.position = contact + support;
                    const float inward = fragment.velocity | normal;
                    if (inward < 0.0f) fragment.velocity -= normal * (1.15f * inward);
                    fragment.velocity *= std::exp(-8.0f * step);
                    fragment.spin *= std::exp(-10.0f * step);
                }
            }
            const float water = sea ? sea->WaveXZ(fragment.position.x, fragment.position.z) : -10000.f;
            if (fragment.position.y < water)
            {
                fragment.wet = true;
                fragment.velocity *= std::exp(-4.f * step);
                fragment.velocity.y = std::min(fragment.velocity.y, -.7f);
                fragment.spin *= std::exp(-3.f * step);
                fragment.life = std::min(fragment.life, fragment.age + 2.5f);
            }
            if (fragment.position.y < water - 3.f) fragment.life = 0.f;
        }
    }
    std::erase_if(impl_->fragments, [](const Fragment &fragment) { return fragment.age >= fragment.life; });
    if (!draw || impl_->fragments.empty()) return;
    CMatrix previous;
    renderer.GetTransform(storm_render::TransformWorld, previous);
    for (const auto &fragment : impl_->fragments)
    {
        CMatrix world(fragment.angle, fragment.position);
        renderer.SetTransform(storm_render::TransformWorld, world);
        for (const auto &batch : fragment.batches)
        {
            if (renderer.TechniqueExecuteStart(batch.material->technique.c_str()))
                do
                {
                    // Technique publication precedes material setup: otherwise
                    // Animation's textured ColorOp would black out flesh caps.
                    applyMaterial(renderer, *batch.material);
                    renderer.DrawPrimitiveUP(storm_render::PrimitiveTriangleList,
                        storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal |
                        storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1,
                        uint32_t(batch.vertices.size() / 3), batch.vertices.data(), sizeof(GEOS::VERTEX0));
                } while (renderer.TechniqueExecuteNext());
        }
    }
    renderer.SetTransform(storm_render::TransformWorld, previous);
}
