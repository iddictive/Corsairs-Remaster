#include "combat_damage.h"
#include "animation.h"
#include "core.h"
#include "dx9render.h"
#include "geometry.h"
#include "model.h"
#include "combat_damage_geometry.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <vector>

namespace
{
using namespace storm_ship_hull;
using Mesh = storm_ship_laser::Mesh;
constexpr size_t maxTriangles = 8192, maxWounds = 12;
Vec3 vec(const CVECTOR &v) { return {v.x, v.y, v.z}; }
CVECTOR vec(Vec3 v) { return {v.x, v.y, v.z}; }
CVECTOR direction(const CMatrix &m, const CVECTOR &v)
{
    return {m.matrix[0]*v.x+m.matrix[4]*v.y+m.matrix[8]*v.z,
            m.matrix[1]*v.x+m.matrix[5]*v.y+m.matrix[9]*v.z,
            m.matrix[2]*v.x+m.matrix[6]*v.y+m.matrix[10]*v.z};
}
struct SourceTriangle { std::array<GEOS::AVERTEX0,3> vertex; int material; };
struct Batch { int material; std::vector<GEOS::VERTEX0> vertices; };

// Techniques restore only state they changed at entry. Material publication
// happens after that snapshot and must not leak into the next model/HUD draw.
struct DrawState
{
    VDX9RENDER &renderer;
    storm_render::Device *device;
    CMatrix world;
    storm_render::Material material{};
    storm_render::BaseTexture *texture=nullptr;
    static constexpr std::array<uint32_t,3> renderFields={storm_render::RenderBlend,
        storm_render::RenderSourceBlend,storm_render::RenderDestinationBlend};
    static constexpr std::array<std::array<uint32_t,2>,7> stageFields={{{0,storm_render::StageColorOp},
        {0,storm_render::StageColorArg1},{0,storm_render::StageColorArg2},{0,storm_render::StageAlphaOp},
        {0,storm_render::StageAlphaArg1},{1,storm_render::StageColorOp},{2,storm_render::StageColorOp}}};
    std::array<uint32_t,renderFields.size()> renderValues{};
    std::array<uint32_t,stageFields.size()> stageValues{};
    explicit DrawState(VDX9RENDER &value) : renderer(value),device(value.GetNativeDevice())
    {
        renderer.GetTransform(storm_render::TransformWorld,world);
        for (size_t i=0;i<renderFields.size();++i) renderer.GetRenderState(renderFields[i],&renderValues[i]);
        for (size_t i=0;i<stageFields.size();++i) renderer.GetTextureStageState(stageFields[i][0],stageFields[i][1],&stageValues[i]);
        device->GetMaterial(&material);
        device->GetTexture(0,&texture); // Retained by the renderer API.
    }
    ~DrawState()
    {
        renderer.SetTransform(storm_render::TransformWorld,world);
        for (size_t i=0;i<renderFields.size();++i) renderer.SetRenderState(renderFields[i],renderValues[i]);
        for (size_t i=0;i<stageFields.size();++i) renderer.SetTextureStageState(stageFields[i][0],stageFields[i][1],stageValues[i]);
        device->SetMaterial(&material);
        device->SetTexture(0,texture); // Device takes its own reference.
        if (texture) texture->Release();
    }
};

// uv[2] is the immutable source triangle; uv[3..5] are barycentric weights.
// Clipping interpolates weights, not bone ids. This retains all six possible
// influences at a cut and avoids the stretched mesh caused by hiding joints.
CVECTOR poseVertex(const GEOS::AVERTEX0 &v, Animation &animation, bool normal)
{
    const CVECTOR p = normal ? CVECTOR(v.nrm.x,v.nrm.y,v.nrm.z) : CVECTOR(v.pos.x,v.pos.y,v.pos.z);
    const auto &a = animation.GetAnimationMatrix(v.boneid & 255);
    const auto &b = animation.GetAnimationMatrix((v.boneid >> 8) & 255);
    auto result = (normal ? direction(a,p) : a*p)*v.weight +
                  (normal ? direction(b,p) : b*p)*(1.f-v.weight);
    result.x = -result.x;
    return result;
}
Vertex anchor(const SourceTriangle &source, size_t triangle, unsigned corner)
{
    const auto &v = source.vertex[corner];
    Vertex result;
    result.position = {v.pos.x,v.pos.y,v.pos.z};
    result.normal = {v.nrm.x,v.nrm.y,v.nrm.z};
    result.color = uint32_t(v.color);
    result.uv = {v.tu0,v.tv0,float(triangle),0,0,0,0,0};
    result.uv[3+corner] = 1;
    return result;
}
bool hitTriangle(Vec3 start, Vec3 delta, const Triangle &triangle, float &distance)
{
    const auto a=triangle[1].position-triangle[0].position;
    const auto b=triangle[2].position-triangle[0].position;
    const auto p=cross(delta,b); const float determinant=dot(a,p);
    if (std::abs(determinant)<1.e-8f) return false;
    const auto t=start-triangle[0].position; const float u=dot(t,p)/determinant;
    if (u<0 || u>1) return false;
    const auto q=cross(t,a); const float v=dot(delta,q)/determinant;
    if (v<0 || u+v>1) return false;
    const float d=dot(b,q)/determinant;
    if (d<0 || d>1 || d>=distance) return false;
    distance=d; return true;
}
void material(VDX9RENDER &renderer, GEOS &geometry, int id)
{
    GEOS::MATERIAL source{};
    if (id>=0) geometry.GetMaterial(id,source);
    else { source.diffuse=.9f; source.specular=.08f; source.gloss=12; }
    storm_render::Material value{};
    value.Diffuse={source.diffuse,source.diffuse,source.diffuse,1};
    value.Specular={source.specular,source.specular,source.specular,1};
    value.Power=source.gloss; renderer.SetMaterial(value);
    const int texture=id>=0 && source.texture_type[0]!=GEOS::TEXTURE_NONE ? source.texture[0] : -1;
    renderer.TextureSet(0,texture);
    renderer.SetTextureStageState(0,storm_render::StageColorArg1,storm_render::textureArg::diffuse);
    renderer.SetTextureStageState(0,storm_render::StageColorArg2,storm_render::textureArg::texture);
    renderer.SetTextureStageState(0,storm_render::StageColorOp,texture>=0 ? storm_render::TextureOpModulate2X : storm_render::TextureOpSelectArg1);
    renderer.SetTextureStageState(1,storm_render::StageColorOp,storm_render::TextureOpDisable);
    renderer.SetTextureStageState(2,storm_render::StageColorOp,storm_render::TextureOpDisable);
    renderer.SetRenderState(storm_render::RenderBlend,id==-1 ? TRUE : FALSE);
    if (id==-1)
    {
        renderer.SetRenderState(storm_render::RenderSourceBlend,storm_render::BlendSourceAlpha);
        renderer.SetRenderState(storm_render::RenderDestinationBlend,storm_render::BlendInverseSourceAlpha);
        renderer.SetTextureStageState(0,storm_render::StageAlphaArg1,storm_render::textureArg::diffuse);
        renderer.SetTextureStageState(0,storm_render::StageAlphaOp,storm_render::TextureOpSelectArg1);
    }
}
void draw(VDX9RENDER &renderer, NODE &node, const std::vector<Batch> &batches)
{
    const auto *technique=node.GetTechnique();
    for (const auto &batch:batches)
        if (!batch.vertices.empty() && renderer.TechniqueExecuteStart(technique && *technique ? technique : "Animation"))
            do {
                material(renderer,*node.geo,batch.material);
                renderer.DrawPrimitiveUP(storm_render::PrimitiveTriangleList,
                    storm_render::vertexFormat::xyz|storm_render::vertexFormat::normal|
                    storm_render::vertexFormat::diffuse|storm_render::vertexFormat::tex1,
                    uint32_t(batch.vertices.size()/3),batch.vertices.data(),sizeof(GEOS::VERTEX0));
            } while (renderer.TechniqueExecuteNext());
}
}

struct CombatDamage::Impl
{
    std::vector<SourceTriangle> source;
    Mesh body;
    std::vector<Mesh> wounds;
    std::vector<Batch> detached;
    CMatrix detachedWorld;
    float detachedRadius=0;
    GEOS *sourceGeometry=nullptr;
    std::vector<size_t> activeTriangles;
    std::vector<Triangle> framePose;

    void RefreshAnchors()
    {
        std::vector<bool> used(source.size());
        const auto include=[&](const Mesh &mesh) {
            for (const auto &object:mesh) for (const auto &triangle:object.second) for (const auto &v:triangle)
                used.at(size_t(std::lround(v.uv[2])))=true;
        };
        include(body); for (const auto &wound:wounds) include(wound);
        activeTriangles.clear();
        for (size_t t=0;t<used.size();++t) if (used[t]) activeTriangles.push_back(t);
        framePose.resize(source.size());
    }

    bool Capture(MODEL &model, VDX9RENDER &renderer)
    {
        auto *node=model.GetNode(0); auto *animation=model.GetAnimation();
        auto *service=static_cast<VGEOMETRY *>(core.GetService("geometry"));
        if (!node || !node->geo || !animation || !service) return false;
        if (sourceGeometry==node->geo) return !source.empty();
        // A substituted model is a new receiver; stale marks must never migrate.
        source.clear(); body.clear(); wounds.clear(); detached.clear(); detachedRadius=0;
        activeTriangles.clear(); framePose.clear();
        sourceGeometry=nullptr;
        GEOS::INFO info{}; node->geo->GetInfo(info);
        if (info.ntriangles<=0 || size_t(info.ntriangles)>maxTriangles || info.nobjects<=0 || info.nobjects>128) return false;
        const int ib=node->geo->GetIndexBuffer();
        const auto *locked=static_cast<const uint16_t *>(renderer.LockIndexBuffer(ib));
        if (!locked) return false;
        std::vector<uint16_t> indices(locked,locked+size_t(info.ntriangles)*3);
        renderer.UnLockIndexBuffer(ib);
        std::vector<SourceTriangle> candidate;
        for (int o=0;o<info.nobjects;++o)
        {
            GEOS::OBJECT object{}; node->geo->GetObj(o,object);
            if (!(object.flags&GEOS::VISIBLE)) continue;
            auto descriptor=service->GetAnimationVBDesc(object.vertex_buff);
            if (!descriptor.buff || object.material<0 || object.material>=info.nmaterials ||
                object.striangle<0 || object.ntriangles<0 || object.striangle>info.ntriangles-object.ntriangles ||
                object.start_vertex<0 || object.num_vertices<=0 || object.start_vertex>descriptor.nvertices-object.num_vertices) return false;
            const auto *vertices=static_cast<const GEOS::AVERTEX0 *>(descriptor.buff);
            for (int t=0;t<object.ntriangles;++t)
            {
                SourceTriangle triangle; triangle.material=object.material;
                for (unsigned c=0;c<3;++c)
                {
                    const auto index=indices[size_t(object.striangle+t)*3+c];
                    if (index>=object.num_vertices) return false;
                    const auto &v=vertices[object.start_vertex+index];
                    if ((v.boneid&255)>=unsigned(animation->GetNumBones()) || ((v.boneid>>8)&255)>=unsigned(animation->GetNumBones()) ||
                        !std::isfinite(v.weight) || v.weight<0 || v.weight>1) return false;
                    triangle.vertex[c]=v;
                }
                candidate.push_back(triangle);
            }
        }
        source=std::move(candidate); sourceGeometry=node->geo;
        return !source.empty();
    }

    std::vector<Triangle> Pose(Animation &animation) const
    {
        std::vector<Triangle> result(source.size());
        for (size_t t=0;t<source.size();++t) for (unsigned c=0;c<3;++c)
        {
            result[t][c]=anchor(source[t],t,c);
            result[t][c].position=vec(poseVertex(source[t].vertex[c],animation,false));
            result[t][c].normal=normalized(vec(poseVertex(source[t].vertex[c],animation,true)));
        }
        return result;
    }
    std::vector<Batch> Expand(const Mesh &mesh, const std::vector<Triangle> &posed, float offset) const
    {
        std::vector<Batch> result;
        for (const auto &object:mesh)
        {
            Batch batch; batch.material=object.first; batch.vertices.reserve(object.second.size()*3);
            for (const auto &triangle:object.second)
            {
                std::array<GEOS::VERTEX0,3> vertices;
                for (unsigned c=0;c<3;++c)
                {
                    const auto &v=triangle[c]; const auto &original=posed.at(size_t(std::lround(v.uv[2])));
                    Vec3 p{},n{};
                    for (unsigned j=0;j<3;++j) { p=p+original[j].position*v.uv[3+j]; n=n+original[j].normal*v.uv[3+j]; }
                    n=normalized(n); p=p+n*offset;
                    vertices[c]={{p.x,p.y,p.z},{n.x,n.y,n.z},int32_t(v.color),v.uv[0],v.uv[1]};
                }
                if (object.first==-2) // Flesh caps use their deformed face normal.
                {
                    const auto a=vertices[0].pos,b=vertices[1].pos,c=vertices[2].pos;
                    const auto n=normalized(cross(Vec3{b.x-a.x,b.y-a.y,b.z-a.z},Vec3{c.x-a.x,c.y-a.y,c.z-a.z}));
                    for (auto &v:vertices) v.nrm={n.x,n.y,n.z};
                }
                batch.vertices.insert(batch.vertices.end(),vertices.begin(),vertices.end());
            }
            result.push_back(std::move(batch));
        }
        return result;
    }
};

CombatDamage::CombatDamage() : impl_(std::make_unique<Impl>()) {}
CombatDamage::~CombatDamage()=default;
bool CombatDamage::ReplacesSkin() const { return !impl_->body.empty(); }

bool CombatDamage::Apply(MODEL &model, VDX9RENDER &renderer, const CVECTOR &rayStart,
                         const CVECTOR &rayEnd, int kind, bool fatal, int severJoint)
{
    if ((kind!=1 && kind!=2) || !finite(vec(rayStart)) || !finite(vec(rayEnd)) || !impl_->Capture(model,renderer)) return false;
    auto &state=*impl_; auto &animation=*model.GetAnimation(); auto &node=*model.GetNode(0);
    model.Update(); CVECTOR start,end; node.glob_mtx.MulToInv(rayStart,start); node.glob_mtx.MulToInv(rayEnd,end);
    const auto posed=state.Pose(animation);
    float distance=2; size_t receiver=posed.size();
    for (size_t t=0;t<posed.size();++t) if (hitTriangle(vec(start),vec(end-start),posed[t],distance)) receiver=t;
    if (receiver==posed.size()) return false;
    const auto hit=vec(start)+vec(end-start)*distance;
    const auto normal=normalized(cross(posed[receiver][1].position-posed[receiver][0].position,
                                       posed[receiver][2].position-posed[receiver][0].position));
    // Three nested, surface-clipped footprints: soaked rim, torn tissue, dark
    // puncture. Their support is finite in depth and excludes back-facing skin.
    auto wound=storm::combat::wound(posed,hit,normal,kind==1);
    if (!wound.empty())
    {
        if (state.wounds.size()==maxWounds) state.wounds.erase(state.wounds.begin());
        state.wounds.push_back(std::move(wound));
        state.RefreshAnchors();
    }
    if (!fatal || kind!=1 || state.detachedRadius>0 || severJoint<0 || severJoint>=animation.GetNumBones()) return true;
    const int parent=animation.GetBoneParent(severJoint);
    if (parent<0 || parent>=animation.GetNumBones()) return true;
    auto head=animation.GetInverseBindMatrix(severJoint), neck=animation.GetInverseBindMatrix(parent);
    head.Transposition(); neck.Transposition();
    const auto axis=normalized(vec(head.Pos()-neck.Pos()));
    if (length(axis)<.9f) return true;
    // Caller supplies the first head-chain joint above the canonical chest.
    // Head-tip indices vary between otherwise compatible animation banks.
    const auto seam=vec(head.Pos());
    const auto headInfluence=[&](unsigned bone) {
        for (int steps=0;steps<animation.GetNumBones();++steps)
        {
            if (int(bone)==severJoint) return true;
            const int parent=animation.GetBoneParent(int(bone));
            if (parent<0) break;
            bone=unsigned(parent);
        }
        return false;
    };
    Mesh source;
    for (size_t t=0;t<state.source.size();++t)
    {
        Triangle triangle;
        for (unsigned c=0;c<3;++c)
        {
            triangle[c]=anchor(state.source[t],t,c);const auto &v=state.source[t].vertex[c];
            triangle[c].uv[7]=(headInfluence(v.boneid&255) ? v.weight : 0.f)+
                               (headInfluence((v.boneid>>8)&255) ? 1.f-v.weight : 0.f);
        }
        source[state.source[t].material].push_back(triangle);
    }
    storm_ship_laser::Plane plane; plane.normal=axis; plane.distance=dot(axis,seam); plane.charred=false;
    auto cut=storm::combat::sever(source,plane);
    if (cut.kept.empty() || cut.detached.empty()) return true;
    auto detached=state.Expand(cut.detached,posed,0);
    CVECTOR center(0.f); size_t count=0;
    for (const auto &batch:detached) for (const auto &v:batch.vertices) { center+=CVECTOR(v.pos.x,v.pos.y,v.pos.z); ++count; }
    if (!count) return true;
    center*=1.f/float(count); float radius=0;
    for (auto &batch:detached) for (auto &v:batch.vertices)
    {
        auto position=direction(node.glob_mtx,CVECTOR(v.pos.x,v.pos.y,v.pos.z)-center);
        radius=std::max(radius,std::sqrt(~position)); v.pos={position.x,position.y,position.z};
        const auto n=direction(node.glob_mtx,CVECTOR(v.nrm.x,v.nrm.y,v.nrm.z)); v.nrm={n.x,n.y,n.z};
    }
    if (!std::isfinite(radius) || radius<.03f || radius>.65f) return true;
    state.body=std::move(cut.kept); state.detached=std::move(detached);
    state.detachedWorld.SetIdentity(); state.detachedWorld.Pos()=node.glob_mtx*center; state.detachedRadius=radius;
    state.wounds.clear(); // No stale marks across the newly open anatomical cut.
    state.RefreshAnchors();
    return true;
}

void CombatDamage::Draw(MODEL &model, VDX9RENDER &renderer)
{
    auto &state=*impl_; auto *node=model.GetNode(0); auto *animation=model.GetAnimation();
    if (!node || node->geo!=state.sourceGeometry || !animation || (state.body.empty() && state.wounds.empty())) return;
    // Skin only the triangles referenced by marks. Intact receivers retain GPU
    // skinning; a small puncture never adds a second whole-character CPU pass.
    auto &posed=state.framePose;
    for (size_t t:state.activeTriangles) for (unsigned c=0;c<3;++c)
    {
        posed[t][c].position=vec(poseVertex(state.source[t].vertex[c],*animation,false));
        posed[t][c].normal=normalized(vec(poseVertex(state.source[t].vertex[c],*animation,true)));
    }
    DrawState restore(renderer);
    renderer.SetTransform(storm_render::TransformWorld,node->glob_mtx);
    if (!state.body.empty()) draw(renderer,*node,state.Expand(state.body,posed,0));
    // Offset follows the skinned surface normal, never camera/screen space.
    for (const auto &wound:state.wounds)
    {
        for (unsigned layer=0;layer<3;++layer)
        {
            Mesh selected;
            for (const auto &triangle:wound.at(-1)) if (unsigned(std::lround(triangle[0].uv[6]))==layer) selected[-1].push_back(triangle);
            draw(renderer,*node,state.Expand(selected,posed,.001f+float(layer)*.0004f));
        }
    }
    if (!state.detached.empty()) { renderer.SetTransform(storm_render::TransformWorld,state.detachedWorld); draw(renderer,*node,state.detached); }
}

bool CombatDamage::DetachedPart(CMatrix &world,float &radius) const
{
    if (impl_->detachedRadius<=0) return false;
    world=impl_->detachedWorld; radius=impl_->detachedRadius; return true;
}
void CombatDamage::SetDetachedTransform(const CMatrix &world) { impl_->detachedWorld=world; }
