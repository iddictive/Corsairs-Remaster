#pragma once
#include "ship_hull_contact_geometry.hpp"

void SHIP::ConfigureHullContactFrame(NODE *node)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if(!node||!node->geo)return;
    const auto &m=node->loc_mtx;
    const std::array<double,12> transform{m.Vx().x,m.Vx().y,m.Vx().z,m.Vy().x,m.Vy().y,m.Vy().z,
        m.Vz().x,m.Vz().y,m.Vz().z,m.Pos().x,m.Pos().y,m.Pos().z};
    node->geo->SetHullContactTransform(uint64_t(blots_id),transform);
#endif
}
bool SHIP::GetHullContactMaterial(HULL_CONTACT_MATERIAL &out) const
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    auto *model=bMounted?const_cast<SHIP*>(this)->GetModel():nullptr;
    auto *root=model?model->GetNode(0):nullptr;
    if(!root||!root->geo)return false;
    storm::ship::material::convex::ContactSnapshot snapshot;
    if(!root->geo->GetHullContactSnapshot(uint64_t(blots_id),snapshot,out.prepared))return false;
    out.revision=snapshot.revision;out.retainedMassFraction=storm::ship::contact::retainedMassFraction(*out.prepared);return true;
#else
    return false;
#endif
}
bool SHIP::CommitHullContactBreak(const HULL_CONTACT_BREAK &request,HULL_CONTACT_BREAK_RESULT &out)
try{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    using namespace storm::ship::material;
    auto *model=bMounted?GetModel():nullptr;auto *root=model?model->GetNode(0):nullptr;
    if(!root||!root->geo||PhonkActive()||!GetACharacter()||phonkLaserWork.active||
       !std::isfinite(request.specificWork)||request.specificWork<=0||
       !finite(request.point)||!finite(request.normal)||request.crossSection.size()<3)return false;
    convex::ContactSnapshot before;std::shared_ptr<const storm::ship::contact::MaterialSnapshot>oldPrepared;
    if(!root->geo->GetHullContactSnapshot(uint64_t(blots_id),before,oldPrepared)||
       before.revision!=request.expectedRevision||!before.strength||!before.strength->layout)return false;
    const double preEditMass=GetHullContactMass();
    if(!(preEditMass>0)||!oldPrepared||!(oldPrepared->retainedVolume>0))return false;
    const float strength=float(GetHullContactStrength(request.point.z,request.normal.z));
    storm::ship::phonk::LaserPlane plane;plane.normal=!request.normal;plane.charred=false;
    const auto start=request.point+plane.normal*.025f;const auto inward=-plane.normal;
    storm_hull_damage::BeamSample beam;beam.start={start.x,start.y,start.z};beam.direction={inward.x,inward.y,inward.z};
    beam.tangent={0,1,0};beam.crossSection=request.crossSection;beam.depth=request.depth+.025f;beam.backDepth=.025f;
    beam.radius=storm_hull_damage::MinBeamRadius;for(const auto &p:beam.crossSection)beam.radius=std::max(beam.radius,std::hypot(p[0],p[1]));
    beam.specificWork=float(request.specificWork);if(double(beam.specificWork)>request.specificWork)beam.specificWork=std::nextafter(beam.specificWork,0.f);
    beam.contactStrength=strength;plane.beams.push_back(std::move(beam));
    plane.distance=(plane.normal|request.point)-request.depth;
    auto field=nodeCutField(model->mtx,root,plane);
    convex::ContactEdit edit;std::shared_ptr<const storm::ship::contact::MaterialSnapshot>prepared;
    if(!root->geo->PrepareHullContactEdit(uint64_t(blots_id),field,request.expectedRevision,edit,prepared))return false;
    const auto &layout=*before.strength->layout;const double whole=root->geo->HullMaterialStructuralArea();
    const double domain=(whole>0?whole:layout.area)*layout.volume/std::max(1.e-12,layout.area);
    if(!(domain>0)||!std::isfinite(domain)||!edit.physicalDelta)return false;
    const double spent=edit.spentWork*strength/domain;
    if(!std::isfinite(spent)||spent<0||spent>request.specificWork)return false;
    const CMatrix eventWorld(CVECTOR(0.f,request.eventWorldYaw,0.f),request.eventWorldPosition);
    if(!PrepareHullContactBodies(edit.physicalDelta,root->loc_mtx,eventWorld,request.eventWorldVelocity,
        request.eventYawVelocity,preEditMass*50.,oldPrepared->retainedVolume))return false;
    if(!root->geo->CommitHullContactEdit(uint64_t(blots_id),edit,prepared)){
        DiscardHullContactBodies(edit.physicalDelta);return false;}
    CommitHullContactBodies(edit.physicalDelta);
    out.material.prepared=std::move(prepared);out.material.revision=edit.next.revision;out.spentSpecificWork=spent;
    out.material.retainedMassFraction=storm::ship::contact::retainedMassFraction(*out.material.prepared);
    out.physicalDelta=edit.physicalDelta;out.retainedMass=GetHullContactMass();
    phonkLaser.materialSavePending=true;return true;
#else
    return false;
#endif
}catch(...){return false;}
