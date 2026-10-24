#include "character.h"
#include "roundhouse_character.h"
#include "roundhouse_pose.hpp"
#include "location.h"
#include "core.h"
#include "v_file_service.h"
#include "math3d/quaternion.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>

namespace
{
CMatrix Inverse(CMatrix matrix) { matrix.Transposition(); return matrix; }

CMatrix Interpolate(const CMatrix &a, const CMatrix &b, float blend)
{
    Matrix first, second, result;
    std::copy(std::begin(a.matrix), std::end(a.matrix), first.matrix);
    std::copy(std::begin(b.matrix), std::end(b.matrix), second.matrix);
    Quaternion rotation;
    rotation.SLerp(Quaternion(first), Quaternion(second), blend).GetMatrix(result);
    CMatrix matrix;
    matrix = result;
    matrix.Pos() = a.v.pos + (b.v.pos - a.v.pos) * blend;
    return matrix;
}

void CapturePalette(Animation &animation, std::vector<CMatrix> &pose)
{
    pose.resize(animation.GetNumBones());
    for (int i = 0; i < animation.GetNumBones(); ++i)
        pose[i] = animation.GetAnimationMatrix(i);
}

CMatrix JointWorld(Animation &animation, const CMatrix &model, int joint)
{
    return roundhouse::AnimationJointWorld(animation.GetInverseBindMatrix(joint),
                                           animation.GetAnimationMatrix(joint), model);
}

bool SelectAction(Animation &bank, const char *name)
{
    auto &player = bank.Player(0);
    if (!player.SetAction(name)) return false;
    player.SetAutoStop(false);
    if (!player.Play()) return false;
    player.Pause();
    player.SetPosition(0.0f);
    bank.EvaluatePose();
    return true;
}

void Sample(Animation &bank, float phase)
{
    bank.Player(0).SetPosition(std::clamp(phase, 0.0f, 1.0f));
    bank.EvaluatePose();
}

float SegmentDistanceSquared(const CVECTOR &point, const CVECTOR &a, const CVECTOR &b)
{
    const CVECTOR line = b - a;
    const float lengthSquared = ~line;
    const CVECTOR offset = point - a;
    const float t = lengthSquared > 1.0e-8f ? std::clamp((offset | line) / lengthSquared, 0.0f, 1.0f) : 0.0f;
    return ~(point - (a + line * t));
}

bool HasCollisionSupport(Location &location, const CVECTOR &position, float ground)
{
    constexpr float tolerance = 0.10f;
    const CVECTOR start(position.x,position.y+0.05f,position.z);
    const CVECTOR end(position.x,ground-tolerance,position.z);
    const float hit = location.Trace(start,end);
    if (!(hit >= 0.0f && hit <= 1.0f)) return false;
    const float hitHeight = start.y+(end.y-start.y)*hit;
    // An earlier platform/stair collision cannot be crossed to reach a lower
    // navigation patch, even if that patch is vertically nearby.
    return std::fabs(hitHeight-ground) <= tolerance;
}
}

bool Character::PrepareRoundhouse()
{
    if (roundhouseState) return roundhouseState->supported;
    roundhouseState = std::make_unique<RoundhouseState>();
    auto &state = *roundhouseState;
    auto *model = Model();
    auto *animation = model ? model->GetAnimation() : nullptr;
    auto *modelAttributes = AttributesPointer ? AttributesPointer->FindAClass(AttributesPointer,"model") : nullptr;
    const char *alias = modelAttributes ? static_cast<const char *>(modelAttributes->GetAttribute("animation")) : nullptr;
    if (!animation || !alias || !fio) return false;
    auto ini = fio->OpenIniFile("RESOURCE\\INI\\roundhouse-rigs.ini");
    if (!ini || ini->GetInt(alias, "supported", 0) != 1 ||
        ini->GetInt(alias, "bones", 0) != animation->GetNumBones()) return false;
    char bankName[128];
    ini->ReadString(alias, "bank", bankName, sizeof(bankName), "");
    auto *service = static_cast<AnimationService *>(core.GetService("AnimationServiceImp"));
    if (!service || !bankName[0]) return false;
    state.bank.reset(service->CreateAnimation(bankName));
    if (!state.bank || state.bank->GetNumBones() != animation->GetNumBones()) return false;
    state.kickSupported = ini->GetInt(alias, "kick_supported", 0) == 1;
    state.head = ini->GetInt(alias, "head_joint", -1);
    state.heel = ini->GetInt(alias, "heel_joint", -1);
    const int count = animation->GetNumBones();
    if (state.head < 0 || state.head >= count || state.heel < 0 || state.heel >= count) return false;
    state.kickDuration = ini->GetFloat(alias, "kick_duration", 1.6f);
    state.contactTime = ini->GetFloat(alias, "contact_time", 0.9f);
    state.getupDuration = ini->GetFloat(alias, "getup_duration", 2.0f);
    if (state.kickDuration <= 0 || state.getupDuration <= 0 || state.contactTime <= 0 ||
        state.contactTime >= state.kickDuration) return false;
    state.heelOffset = CVECTOR(ini->GetFloat(alias,"heel_offset_x",0), ini->GetFloat(alias,"heel_offset_y",0), ini->GetFloat(alias,"heel_offset_z",0));
    state.headOffset = CVECTOR(ini->GetFloat(alias,"head_offset_x",0), ini->GetFloat(alias,"head_offset_y",0), ini->GetFloat(alias,"head_offset_z",0));
    state.rig.headBody = 2;
    state.rig.jointParents.resize(count);
    state.rig.bindWorld.resize(count);
    for (int i = 0; i < count; ++i)
    {
        state.rig.jointParents[i] = animation->GetBoneParent(i);
        state.rig.bindWorld[i] = Inverse(animation->GetInverseBindMatrix(i));
    }
    static constexpr float masses[] = {12,20,5,3,2,1,3,2,1,8,4,1,8,4,1};
    for (int i = 0; i < 15; ++i)
    {
        const std::string prefix = "body" + std::to_string(i);
        roundhouse::BodyDefinition body;
        body.joint = ini->GetInt(alias,(prefix + "_joint").c_str(),-1);
        body.endJoint = ini->GetInt(alias,(prefix + "_end").c_str(),-1);
        body.parentBody = ini->GetInt(alias,(prefix + "_parent").c_str(),-1);
        if (body.joint < 0 || body.joint >= count || body.endJoint >= count || body.parentBody >= i) return false;
        if (body.endJoint == body.joint) body.endJoint = -1;
        body.mass = masses[i];
        body.radius = i == 0 ? 0.12f : (i == 1 ? 0.13f : (i == 2 ? 0.105f : 0.055f));
        body.swingRadians = i == 2 ? 0.6f : 1.2f;
        body.twistMinRadians = -0.5f;
        body.twistMaxRadians = 0.5f;
        state.rig.bodies.push_back(body);
    }
    state.supported = true;
    return true;
}

bool Character::IsRoundhouseControlled() const
{
    return roundhouseState && roundhouseState->phase != RoundhouseState::Phase::Idle;
}

bool Character::IsPhysicalRagdoll() const
{
    return roundhouseState && roundhouseState->phase == RoundhouseState::Phase::Falling;
}

void Character::CancelRoundhouse()
{
    if (!roundhouseState) return;
    auto &state = *roundhouseState;
    if (state.handle)
        if (auto *location = GetLocation())
            if (auto *world = location->ExistingRagdollWorld()) world->Remove(state.handle);
    state.handle = 0;
    const bool controlled = IsRoundhouseControlled();
    state.phase = RoundhouseState::Phase::Idle;
    if (auto *model = Model())
        if (auto *animation = model->GetAnimation()) animation->ClearPoseOverride();
    if (controlled)
    {
        fgtCurType = fgtSetType = fgt_none;
        priorityAction.SetName(nullptr);
        isSetPriorityAction = false;
    }
}

bool Character::StartRoundhouse()
{
    const auto *location = GetLocation();
    if (!location || location->IsPaused() || IsDialog() || IsRoundhouseControlled() || IsDead() ||
        !IsFight() || isJump || isSwim || priorityAction.name ||
        !PrepareRoundhouse() || !roundhouseState->kickSupported) return false;
    auto &state = *roundhouseState;
    if (!SelectAction(*state.bank, "roundhouse_kick")) return false;
    int32_t allowed = 0;
    auto *answer = core.Event("Roundhouse_TryStart", "i", GetId());
    if (!answer || !answer->Get(allowed) || !allowed) return false;
    CapturePalette(*Model()->GetAnimation(), state.blendFrom);
    StopFightAnimation();
    state.phase = RoundhouseState::Phase::Kick;
    state.time = 0;
    state.contacted = false;
    state.hasPreviousHeel = false;
    state.origin = curPos;
    state.yaw = ay;
    return true;
}

bool Character::BeginRoundhouseFall(const CVECTOR &point, const CVECTOR &impulse)
{
    if (IsPhysicalRagdoll() || !PrepareRoundhouse()) return false;
    auto *location = GetLocation();
    if (!location) return false;
    // Keep a valid navigation anchor before physics can carry the living actor
    // beyond the walkable patch.
    float recoveryGround;
    const auto recoveryNode = location->GetPtcData().FindNode(curPos,recoveryGround);
    if (recoveryNode < 0 || recoveryGround > curPos.y+0.05f || curPos.y-recoveryGround > height ||
        !HasCollisionSupport(*location,curPos,recoveryGround)) return false;
    auto *world = location->GetRagdollWorld();
    auto *model = Model();
    auto *animation = model ? model->GetAnimation() : nullptr;
    if (!world || !animation) return false;
    auto &state = *roundhouseState;
    std::vector<CMatrix> pose(animation->GetNumBones());
    auto rig = state.rig;
    for (int i = 0; i < animation->GetNumBones(); ++i)
    {
        pose[i] = JointWorld(*animation, model->mtx, i);
        rig.bindWorld[i] = roundhouse::MirrorJoint(rig.bindWorld[i]) * model->mtx;
    }
    const auto handle = world->Create(rig, pose, impulse, point);
    if (!handle) return false;
    CancelRoundhouse();
    state.handle = handle;
    state.phase = RoundhouseState::Phase::Falling;
    state.time = 0;
    state.origin = curPos;
    state.recoveryPosition = CVECTOR(curPos.x,recoveryGround,curPos.z);
    state.recoveryNode = recoveryNode;
    state.yaw = ay;
    state.pose = std::move(pose);
    StopFightAnimation();
    return true;
}

bool Character::UpdateRoundhouse(float seconds)
{
    if (!IsRoundhouseControlled()) return false;
    if (IsDialog() || !GetLocation()) { CancelRoundhouse(); return false; }
    auto &state = *roundhouseState;
    auto *location = GetLocation();
    auto *model = Model();
    auto *animation = model ? model->GetAnimation() : nullptr;
    if (!animation) { CancelRoundhouse(); return false; }
    if (!(seconds > 0.0f) || location->IsPaused()) return true;
    seconds = std::clamp(seconds, 0.0f, 0.1f);
    state.time += seconds;
    const auto updatePalette = [&](const std::vector<CMatrix> &worldPose) {
        const auto inverseModel = Inverse(model->mtx);
        std::vector<CMatrix> palette(worldPose.size());
        for (std::size_t i = 0; i < palette.size(); ++i)
            palette[i] = roundhouse::WorldToSkinPalette(animation->GetInverseBindMatrix(static_cast<int>(i)),
                                                       worldPose[i], inverseModel);
        animation->SetPoseOverride(palette.data(),static_cast<int>(palette.size()));
    };
    if (state.phase == RoundhouseState::Phase::Falling)
    {
        auto *world = location->ExistingRagdollWorld();
        if (!world || !world->GetPose(state.handle,state.pose)) { CancelRoundhouse(); return false; }
        const CVECTOR pelvis = state.pose[state.rig.bodies[0].joint].Pos();
        curPos.x = pelvis.x;
        curPos.z = pelvis.z;
        model->mtx.BuildMatrix(CVECTOR(0,ay,0),curPos);
        updatePalette(state.pose);
        UpdateWeapons();
        if (!IsDead() && state.time >= 2.5f && (world->IsSleeping(state.handle) || state.time >= 5.0f))
        {
            float ground = 0.0f;
            auto node = location->GetPtcData().FindNode(pelvis,ground);
            CVECTOR recoveryPosition(pelvis.x,ground,pelvis.z);
            const bool supported = node >= 0 && ground <= pelvis.y+0.05f && pelvis.y-ground <= height &&
                                   HasCollisionSupport(*location,pelvis,ground);
            const bool useAnchor = !supported && state.time >= 5.0f;
            if (!supported)
            {
                node = useAnchor ? state.recoveryNode : -1;
                recoveryPosition = state.recoveryPosition;
            }
            if (node >= 0)
            {
                currentNode = node;
                curPos = oldPos = recoveryPosition;
                vy = 0.0f;
                model->mtx.BuildMatrix(CVECTOR(0,state.yaw,0),curPos);
                updatePalette(state.pose);
                CapturePalette(*animation,state.blendFrom);
                const bool faceDown = state.pose[state.rig.bodies[1].joint].Vz().y < 0;
                if (SelectAction(*state.bank,faceDown ? "getup_front" : "getup_back"))
                {
                    // Off-patch recovery relocates to the saved anchor; do not
                    // blend the mesh through intervening walls or empty space.
                    if (useAnchor)
                    {
                        CapturePalette(*state.bank,state.blendFrom);
                        animation->SetPoseOverride(state.blendFrom.data(),static_cast<int>(state.blendFrom.size()));
                    }
                    world->Remove(state.handle);
                    state.handle = 0;
                    state.phase = RoundhouseState::Phase::GetUp;
                    state.time = 0;
                    state.origin = curPos;
                    ay = state.yaw;
                }
                else
                {
                    CancelRoundhouse();
                    UpdateAnimation();
                }
            }
        }
        return true;
    }

    curPos = oldPos = state.origin;
    ay = state.yaw;
    model->mtx.BuildMatrix(CVECTOR(0,ay,0),curPos);
    const bool kick = state.phase == RoundhouseState::Phase::Kick;
    const float duration = kick ? state.kickDuration : state.getupDuration;
    Sample(*state.bank,state.time/duration);
    CapturePalette(*state.bank,state.pose);
    const float blend = std::clamp(state.time/0.16f,0.0f,1.0f);
    for (std::size_t i = 0; i < state.pose.size() && i < state.blendFrom.size(); ++i)
        state.pose[i] = Interpolate(state.blendFrom[i],state.pose[i],blend);
    animation->SetPoseOverride(state.pose.data(),static_cast<int>(state.pose.size()));
    UpdateWeapons();

    if (kick)
    {
        auto heelMatrix = JointWorld(*animation,model->mtx,state.heel);
        const CVECTOR heel = roundhouse::JointPointWorld(heelMatrix, state.heelOffset);
        if (!state.hasPreviousHeel) { state.previousHeel = heel; state.hasPreviousHeel = true; }
        if (!state.contacted && state.time >= state.contactTime-0.08f && state.time <= state.contactTime+0.08f)
        {
            Character *victim = nullptr;
            CVECTOR contact;
            float nearest = 0.22f*0.22f;
            const CVECTOR torso = JointWorld(*animation,model->mtx,state.rig.bodies[1].joint).Pos();
            auto candidates = location->supervisor.FindCharacters(this,2.2f,100.0f,0.0f,0.0f,false,false);
            for (const auto &candidate : candidates)
            {
                auto *other = candidate.c;
                if (!other || other == this || other->IsDead() || other->IsPhysicalRagdoll() || !other->PrepareRoundhouse()) continue;
                auto *otherModel = other->Model();
                auto *otherAnimation = otherModel ? otherModel->GetAnimation() : nullptr;
                if (!otherAnimation) continue;
                auto headMatrix = JointWorld(*otherAnimation,otherModel->mtx,other->roundhouseState->head);
                const auto head = roundhouse::JointPointWorld(headMatrix, other->roundhouseState->headOffset);
                const float distance = SegmentDistanceSquared(head,state.previousHeel,heel);
                if (distance >= nearest || location->Trace(state.previousHeel,head) < 0.995f ||
                    location->Trace(torso,head) < 0.995f) continue;
                int32_t allowed = 0;
                auto *answer = core.Event("Roundhouse_CanHit","ii",GetId(),other->GetId());
                if (!answer || !answer->Get(allowed) || !allowed) continue;
                nearest = distance;
                victim = other;
                contact = head;
            }
            if (victim)
            {
                CVECTOR direction = contact - (curPos+CVECTOR(0,1.2f,0));
                direction.y = 0;
                const float length = std::sqrt(~direction);
                direction = length > 0.001f ? direction/length : CVECTOR(std::sin(ay),0,std::cos(ay));
                if (victim->BeginRoundhouseFall(contact,direction*85.0f+CVECTOR(0,22.0f,0)))
                {
                    state.contacted = true;
                    core.Event("Event_ChrSnd_Hit","i",victim->GetId());
                    core.Event("Location_CharacterAttack","iislll",GetId(),victim->GetId(),"roundhouse",0,0,-1);
                }
            }
        }
        state.previousHeel = heel;
    }
    if (state.time >= duration)
    {
        CancelRoundhouse();
        UpdateAnimation();
    }
    return true;
}
