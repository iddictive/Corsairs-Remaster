#pragma once
#include "ship_hull_material.hpp"
#include "ship_contact_material.hpp"

namespace storm::ship::material
{
using ContactTransform = std::array<double, 12>;
inline ContactTransform identityContactTransform() { return {1,0,0, 0,1,0, 0,0,1, 0,0,0}; }
inline contact::MaterialVertex contactVertex(storm_ship_hull::Vec3 p, const ContactTransform &m)
{
    return {p.x*m[0]+p.y*m[3]+p.z*m[6]+m[9],
            p.x*m[1]+p.y*m[4]+p.z*m[7]+m[10],
            p.x*m[2]+p.y*m[5]+p.z*m[8]+m[11]};
}
inline bool appendContactPiece(contact::MaterialSnapshot &out, const convex::Piece &piece,
                               uint32_t source, const ContactTransform &transform)
{
    std::vector<contact::MaterialVertex> vertices;
    for (const auto &face : piece.faces) for (const auto &vertex : face.vertices)
        vertices.push_back(contactVertex(vertex.position, transform));
    if(!contact::appendMaterialPiece(out, vertices, source, piece.supportCell))return false;
    out.pieces.back().volume=piece.volume;return true;
}
inline std::shared_ptr<const contact::MaterialSnapshot> prepareContactMaterial(
    const convex::ContactSnapshot &snapshot, const ContactTransform &transform)
{
    if (!snapshot.cells || !convex::compatible(*snapshot.cells)) return {};
    auto next = std::make_shared<contact::MaterialSnapshot>();
    next->retainedSupports = snapshot.retainedSupports;
    auto supportVolumes=std::make_shared<std::vector<double>>(snapshot.strength&&snapshot.strength->layout?snapshot.strength->layout->cells.size():0);
    for(const auto&source:snapshot.cells->convex->source->pieces)if(source)for(const auto&piece:*source){
        storm_geometry_work::checkpoint();next->authoredVolume+=piece.volume;}
    for (uint32_t id = 0; id < snapshot.cells->convex->cells.size(); ++id)
    {
        storm_geometry_work::checkpoint();
        const auto &cell = snapshot.cells->convex->cells[id];
        if (!cell || !cell->pieces) continue;
        for (const auto &piece : *cell->pieces){
            if (!appendContactPiece(*next, piece, id, transform)) return {};
            if(piece.supportCell<supportVolumes->size())(*supportVolumes)[piece.supportCell]+=piece.volume;
            if(!snapshot.retainedSupports||piece.supportCell==storm_ship_structure::NoCell||
               (piece.supportCell<snapshot.retainedSupports->size()&&snapshot.retainedSupports->at(piece.supportCell)))next->retainedVolume+=piece.volume;
        }
    }
    next->supportVolumes=std::move(supportVolumes);
    contact::buildMaterialIndex(*next, [] { storm_geometry_work::checkpoint(); });
    return next;
}
inline std::shared_ptr<const contact::MaterialSnapshot> patchContactMaterial(
    const std::shared_ptr<const contact::MaterialSnapshot> &before,
    const convex::ContactEdit &edit, const ContactTransform &transform)
{
    if (!before || !edit.complete || !edit.physicalDelta || !edit.next.cells ||
        !convex::compatible(*edit.next.cells)) return {};
    std::map<uint32_t, std::vector<contact::MaterialPiece>> replacements;
    const bool ownershipOnly = edit.next.cells == edit.physicalDelta->before.cells;
    double retainedVolume=before->retainedVolume;
    const auto retained=[](const auto&mask,uint32_t support){return !mask||support==storm_ship_structure::NoCell||(support<mask->size()&&mask->at(support));};
    auto volumes=before->supportVolumes?std::make_shared<std::vector<double>>(*before->supportVolumes):nullptr;
    std::vector<double>changedBefore(volumes?volumes->size():0,0.);
    if (!ownershipOnly) for (uint32_t id : edit.changedCells)
    {
        if (id >= edit.next.cells->convex->cells.size()) return {};
        const auto &cell = edit.next.cells->convex->cells[id];
        const auto&old=edit.physicalDelta->before.cells->convex->cells[id];
        if(cell==old)continue; // Ownership masks already cover unchanged source pieces.
        contact::MaterialSnapshot source;
        if(old&&old->pieces)for(const auto&piece:*old->pieces){
            if(retained(edit.physicalDelta->before.retainedSupports,piece.supportCell))retainedVolume-=piece.volume;
            if(volumes&&piece.supportCell<volumes->size()){volumes->at(piece.supportCell)-=piece.volume;changedBefore[piece.supportCell]+=piece.volume;}}
        if (cell && cell->pieces) for (const auto &piece : *cell->pieces)
            {if (!appendContactPiece(source, piece, id, transform)) return {};
             if(retained(edit.next.retainedSupports,piece.supportCell))retainedVolume+=piece.volume;
             if(volumes&&piece.supportCell<volumes->size())volumes->at(piece.supportCell)+=piece.volume;}
        replacements.emplace(id, std::move(source.pieces));
    }
    if(before->supportVolumes)for(uint32_t id=0;id<before->supportVolumes->size();++id){
        const bool was=retained(edit.physicalDelta->before.retainedSupports,id),now=retained(edit.next.retainedSupports,id);
        if(was!=now)retainedVolume+=(now?1.:-1.)*(before->supportVolumes->at(id)-changedBefore[id]);
    }
    if (replacements.empty() && edit.next.retainedSupports == edit.physicalDelta->before.retainedSupports)
        return before;
    return contact::applyMaterialReplacements(before, std::move(replacements), edit.next.retainedSupports,std::max(0.,retainedVolume),std::move(volumes));
}
inline void prepareContactProjection(const storm_ship_laser::Input &input, storm_ship_laser::Projection &projection,
                                     const ContactTransform &transform)
{
    if (!projection.kept || !projection.kept->cells ||
        (!projection.result.changed && projection.kept == input.surface)) return;
    auto retained = *projection.kept;
    if (!preparePhysicalCache(retained)) return;
    const auto strength = projection.structure ? projection.structure : retained.structure;
    convex::ContactSnapshot snapshot{retained.cells, strength, input.materialRevision};
    // The worker already knows exactly which positive cells changed. Reuse
    // the immutable full-model BVH rather than projecting every timber piece
    // after each narrow laser sample. Rebase at the existing overlay bound.
    if(input.contactPrepared&&input.surface->cells&&
       convex::compatible(*input.surface->cells)&&
       input.surface->cells->convex->source==retained.cells->convex->source){
        convex::ContactEdit edit;edit.complete=true;edit.next=snapshot;
        auto delta=std::make_shared<convex::PhysicalDelta>();
        delta->before={input.surface->cells,input.structure,input.materialRevision};
        edit.physicalDelta=std::move(delta);
        for(uint32_t id=0;id<retained.cells->convex->cells.size();++id)
            if(retained.cells->convex->cells[id]!=input.surface->cells->convex->cells[id])edit.changedCells.push_back(id);
        projection.contactPrepared=patchContactMaterial(input.contactPrepared,edit,transform);
    }
    if(!projection.contactPrepared)projection.contactPrepared = prepareContactMaterial(snapshot, transform);
    projection.kept = std::make_shared<const storm_ship_laser::Surface>(std::move(retained));
}
}
