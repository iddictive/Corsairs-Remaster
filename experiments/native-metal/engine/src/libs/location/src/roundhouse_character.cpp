#include "character.h"
#include "roundhouse_character.h"
#include "roundhouse_pose.hpp"
#include "location.h"
#include "player.h"
#include "core.h"
#include "v_file_service.h"
#include "math3d/quaternion.h"
#include "shared/sea_ai/deck_collision.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <cstdlib>
#include <span>
#include <string>

namespace
{
constexpr float ContactSearchRadius = 2.2f;
constexpr float JumpImpactRadius = 1.75f;

bool CanHitRoundhouse(Character &actor, Character &target)
{
    int32_t allowed = 0;
    auto *answer = core.Event("Roundhouse_CanHit","ii",actor.GetId(),target.GetId());
    return answer && answer->Get(allowed) && allowed;
}

bool CanAimRoundhouse(Character &actor, Character *target)
{
    if (!target || target == &actor || target->IsDead() || target->IsPhysicalRagdoll()) return false;
    CVECTOR origin, position;
    actor.GetPosition(origin);
    target->GetPosition(position);
    const float dx = position.x-origin.x, dz = position.z-origin.z;
    return dx*dx+dz*dz <= ContactSearchRadius*ContactSearchRadius && CanHitRoundhouse(actor,*target);
}

CMatrix Inverse(CMatrix matrix) { matrix.Transposition(); return matrix; }

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
    state.strikeStartTime = ini->GetFloat(alias, "strike_start_time", -1.0f);
    state.strikeEndTime = ini->GetFloat(alias, "strike_end_time", -1.0f);
    state.getupDuration = ini->GetFloat(alias, "getup_duration", 2.0f);
    if (state.kickDuration <= 0 || state.getupDuration <= 0 || state.strikeStartTime < 0 ||
        state.strikeEndTime <= state.strikeStartTime || state.strikeEndTime > state.kickDuration) return false;
    state.heelOffset = CVECTOR(ini->GetFloat(alias,"heel_offset_x",0), ini->GetFloat(alias,"heel_offset_y",0), ini->GetFloat(alias,"heel_offset_z",0));
    state.leftHeelOffset = CVECTOR(ini->GetFloat(alias,"left_heel_offset_x",0), ini->GetFloat(alias,"left_heel_offset_y",0), ini->GetFloat(alias,"left_heel_offset_z",0));
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
    if (state.detachedHandle)
        if (auto *location = GetLocation())
            if (auto *world = location->ExistingRagdollWorld()) world->Remove(state.detachedHandle);
    state.detachedHandle = 0;
    state.detachedPose.clear();
    state.handle = 0;
    state.aimTarget = invalid_entity;
    state.hitTargets.clear();
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

bool Character::ApplyCombatWeaponImpact(entid_t attacker, int kind, bool fatal)
{
    auto *source = dynamic_cast<Character *>(core.GetEntityPointer(attacker));
    auto *model = Model();
    auto *location = GetLocation();
    if (!source || !model || !location || source->GetLocation() != location || (kind != 1 && kind != 2))
        return false;
    // Existing combat resolves melee arcs and hitscan targets rather than mesh
    // collision points. Project that accepted attacker-to-victim line onto the
    // actual skinned receiver; a mesh miss creates no invented surface wound.
    CVECTOR start = source->curPos + CVECTOR(0.0f, 1.3f, 0.0f);
    if (kind == 2)
    {
        const auto hand = source->GetHandLightPos();
        if (hand.x != 0.0f || hand.y != 0.0f || hand.z != 0.0f) start = hand;
    }
    const auto delta = curPos + CVECTOR(0.0f, 1.25f, 0.0f) - start;
    const auto end = start + delta * 1.5f;
    int neck = -1;
    if (fatal && PrepareRoundhouse())
    {
        const auto &rig = roundhouseState->rig;
        const int chest = rig.bodies[1].joint;
        int joint = roundhouseState->head;
        for (std::size_t count = 0; count < rig.jointParents.size() && joint >= 0 &&
             joint < static_cast<int>(rig.jointParents.size()); ++count)
        {
            const int parent = rig.jointParents[joint];
            if (parent == chest) { neck = joint; break; }
            joint = parent;
        }
    }
    if (!model->ApplyCombatDamage(start, end, kind, fatal, neck)) return false;
    CMatrix world;
    float radius = 0.0f;
    if (model->GetCombatDetachedPart(world, radius) && PrepareRoundhouse() && !roundhouseState->detachedHandle)
    {
        roundhouse::RigDefinition rig;
        rig.jointParents = {-1};
        rig.bindWorld = {world};
        rig.headBody = 0;
        roundhouse::BodyDefinition body;
        body.joint = 0;
        body.mass = 4.0f;
        body.radius = radius;
        rig.bodies = {body};
        auto &state = *roundhouseState;
        state.detachedPose = {world};
        CVECTOR impulse = delta;
        const float length = std::sqrt(~impulse);
        if (length > 0.001f) impulse *= 8.0f / length;
        impulse.y += 5.0f;
        if (auto *physics = location->GetRagdollWorld())
            state.detachedHandle = physics->Create(rig, state.detachedPose, impulse, world.Pos());
    }
    return true;
}

void Character::UpdateCombatDetached()
{
    if (!roundhouseState || !roundhouseState->detachedHandle) return;
    auto &state = *roundhouseState;
    auto *location = GetLocation();
    auto *world = location ? location->ExistingRagdollWorld() : nullptr;
    auto *model = Model();
    if (world && model && world->GetPose(state.detachedHandle, state.detachedPose) && !state.detachedPose.empty())
    {
        model->SetCombatDetachedTransform(state.detachedPose[0]);
        if (state.detachedPose[0].Pos().y < curPos.y - 10.0f)
        {
            world->Remove(state.detachedHandle);
            state.detachedHandle = 0;
        }
    }
}

bool Character::StartRoundhouse(bool jumping)
{
    const auto *location = GetLocation();
    if (!location || location->IsPaused() || IsDialog() || IsRoundhouseControlled() || IsDead() ||
        !IsFight() || isJump || isSwim || priorityAction.name ||
        !PrepareRoundhouse() || !roundhouseState->kickSupported) return false;
    auto &state = *roundhouseState;
    if (!SelectAction(*state.bank, jumping ? "roundhouse_jump" : "roundhouse_kick")) return false;
    const auto &player = state.bank->Player(0);
    const char *startData = player.GetData("StrikeStart"), *endData = player.GetData("StrikeEnd");
    if (!startData || !endData) return false;
    const float strikeStart = std::strtof(startData,nullptr), strikeEnd = std::strtof(endData,nullptr);
    if (!(strikeStart >= 0 && strikeEnd > strikeStart && strikeEnd <= 1)) return false;
    state.kickDuration = player.GetFrames()/(state.bank->GetFPS()*player.GetDefSpeed());
    if (!(state.kickDuration > 0)) return false;
    int32_t allowed = 0;
    auto *answer = core.Event("Roundhouse_TryStart", "il", GetId(), jumping ? 1 : 0);
    if (!answer || !answer->Get(allowed) || !allowed) return false;
    state.strikeStartTime = (jumping ? std::max(strikeStart, 0.38f) : strikeStart) * state.kickDuration;
    state.strikeEndTime = (jumping ? std::max(strikeEnd, 0.80f) : strikeEnd) * state.kickDuration;
    CapturePalette(*Model()->GetAnimation(), state.blendFrom);
    state.strikeOrigin = JointWorld(*Model()->GetAnimation(),Model()->mtx,state.rig.bodies[1].joint).Pos();
    ClearUnarmedPose();
    StopFightAnimation();
    state.phase = RoundhouseState::Phase::Kick;
    state.jumping = jumping;
    state.time = 0;
    state.contacted = false;
    state.hitTargets.clear();
    state.hasPreviousHeel = false;
    state.origin = curPos;
    state.yaw = ay;
    CMatrix identity;
    identity.SetIdentity();
    state.rootStart = JointWorld(*state.bank,identity,state.rig.bodies[0].joint).Pos();
    state.previousRoot = {};
    // Ordinary melee already owns player target selection. Keep only its ID;
    // deletion, escape or changed eligibility ends steering without retargeting.
    auto *target = IsPlayer() ? static_cast<Player *>(this)->FindAttackCharacter(ContactSearchRadius) : nullptr;
    state.aimTarget = target && target->GetLocation() == location && CanAimRoundhouse(*this,target)
                          ? target->GetId() : invalid_entity;
    return true;
}

bool Character::BeginRoundhouseFall(const CVECTOR &point, const CVECTOR &impulse, entid_t attacker)
{
    if (IsPhysicalRagdoll() || !PrepareRoundhouse()) return false;
    const auto reject = [&](const char *stage) {
        core.Trace("Roundhouse: physical fall rejected stage=%s actor=%llu node=%d",stage,
                   static_cast<unsigned long long>(GetId()),currentNode);
        return false;
    };
    auto *location = GetLocation();
    if (!location) return reject("location");
    // Keep a valid navigation anchor before physics can carry the living actor
    // beyond the walkable patch.
    // Ordinary movement already grounded curPos/currentNode through PtcData.
    // A second GM ray prerequisite can reject a normally standing character.
    const auto recoveryNode = currentNode;
    if (recoveryNode < 0 || recoveryNode >= location->GetPtcData().numTriangles)
        return reject("navigation-anchor");
    auto *world = location->GetRagdollWorld();
    auto *model = Model();
    auto *animation = model ? model->GetAnimation() : nullptr;
    if (!world) return reject("collision-world");
    if (!animation) return reject("animation");
    auto &state = *roundhouseState;
    std::vector<CMatrix> pose(animation->GetNumBones());
    auto rig = state.rig;
    for (int i = 0; i < animation->GetNumBones(); ++i)
    {
        pose[i] = JointWorld(*animation, model->mtx, i);
        rig.bindWorld[i] = roundhouse::MirrorJoint(rig.bindWorld[i]) * model->mtx;
    }
    const auto handle = world->Create(rig, pose, impulse, point);
    if (!handle) return reject("create-ragdoll");
    ClearUnarmedPose();
    CancelRoundhouse();
    state.handle = handle;
    state.phase = RoundhouseState::Phase::Falling;
    state.time = 0;
    state.origin = curPos;
    state.recoveryPosition = curPos;
    state.recoveryNode = recoveryNode;
    state.yaw = ay;
    state.pose = std::move(pose);
    state.pelvisRootOffset = state.pose[state.rig.bodies[0].joint].Pos().y-curPos.y;
    state.fallAttacker = attacker;
    state.fallDamageApplied = false;
    state.recoveryClassified = false;
    state.protectedRecovery = false;
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
        if (!world || (state.handle && !world->GetPose(state.handle,state.pose))) { CancelRoundhouse(); return false; }
        const CVECTOR pelvis = state.pose[state.rig.bodies[0].joint].Pos();
        curPos.x = pelvis.x;
        curPos.y = pelvis.y-state.pelvisRootOffset;
        curPos.z = pelvis.z;
        model->mtx.BuildMatrix(CVECTOR(0,ay,0),curPos);
        updatePalette(state.pose);
        UpdateWeapons();
        // Measure actual body clearance below the launch floor, not the
        // standing pelvis height or the temporary lift from the kick impulse.
        float bottom = pelvis.y;
        for (const auto &body : state.rig.bodies)
        {
            bottom = std::min(bottom,state.pose[body.joint].Pos().y-body.radius);
            if (body.endJoint >= 0) bottom = std::min(bottom,state.pose[body.endJoint].Pos().y-body.radius);
        }
        const auto fallPolicy = [&](bool lethal) {
            const bool credited = core.IsEntityValid(state.fallAttacker);
            int32_t result = 0;
            auto *answer = core.Event("Roundhouse_FallDamage","iill",GetId(),
                                      credited ? state.fallAttacker : GetId(),lethal ? 1 : 0,credited ? 1 : 0);
            if (!answer || !answer->Get(result))
                core.Trace("Roundhouse: fall policy missing actor=%llu",static_cast<unsigned long long>(GetId()));
            state.protectedRecovery = result < 0;
            state.recoveryClassified = true;
        };
        if (!IsDead() && !state.fallDamageApplied && state.origin.y-bottom >= 2.0f)
        {
            state.fallDamageApplied = true;
            fallPolicy(true);
        }
        // A corpse leaving all collision geometry keeps its final physical
        // pose until normal character/location cleanup, without a live body
        // simulating forever below the world.
        if (IsDead() && state.handle && state.origin.y-bottom > 10.0f)
        {
            world->Remove(state.handle);
            state.handle = 0;
        }
        if (!IsDead() && state.time >= 2.5f && (world->IsSleeping(state.handle) || state.time >= 5.0f))
        {
            float ground = 0.0f;
            auto node = location->GetPtcData().FindNode(pelvis,ground);
            CVECTOR recoveryPosition(pelvis.x,ground,pelvis.z);
            const bool supported = node >= 0 && ground <= pelvis.y+0.05f && pelvis.y-ground <= height &&
                                   HasCollisionSupport(*location,pelvis,ground);
            if (!supported && state.time >= 5.0f && !state.recoveryClassified) fallPolicy(false);
            const bool useAnchor = !supported && state.time >= 5.0f && state.protectedRecovery;
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

    const bool kick = state.phase == RoundhouseState::Phase::Kick;
    if (kick && state.aimTarget && state.time < (state.jumping ? state.strikeStartTime : state.strikeEndTime) && !state.contacted)
    {
        auto *target = static_cast<Character *>(core.GetEntityPointerSafe(state.aimTarget));
        if (target && target->GetLocation() == location && CanAimRoundhouse(*this,target))
        {
            const float desired = std::atan2(target->curPos.x-state.origin.x,target->curPos.z-state.origin.z);
            const float delta = std::atan2(std::sin(desired-state.yaw),std::cos(desired-state.yaw));
            const float step = std::max(turnSpeed,0.0f)*seconds;
            state.yaw += std::clamp(delta,-step,step);
            nay = state.yaw;
        }
        else state.aimTarget = invalid_entity;
    }
    ay = state.yaw;
    const float duration = kick ? state.kickDuration : state.getupDuration;
    Sample(*state.bank,state.time/duration);
    CapturePalette(*state.bank,state.pose);
    if (kick && state.jumping)
    {
        CMatrix identity;
        identity.SetIdentity();
        CVECTOR root = JointWorld(*state.bank,identity,state.rig.bodies[0].joint).Pos() - state.rootStart;
        root.y = 0;
        CMatrix rotation;
        rotation.BuildMatrix(CVECTOR(0.0f,ay,0.0f),CVECTOR(0.0f,0.0f,0.0f));
        const CVECTOR step = rotation * (root-state.previousRoot);
        currentNode = location->GetPtcData().Move(currentNode,curPos+step,oldPos);
        curPos = oldPos;
        state.previousRoot = root;
        // The native skinning path mirrors local X. Remove the extracted
        // translation from every palette matrix before applying actor motion.
        for (auto &matrix : state.pose) { matrix.Pos().x += root.x; matrix.Pos().z -= root.z; }
    }
    else curPos = oldPos = state.origin;
    model->mtx.BuildMatrix(CVECTOR(0,ay,0),curPos);
    const float blend = std::clamp(state.time/0.16f,0.0f,1.0f);
    roundhouse::BlendPose(*animation, state.blendFrom, state.pose, blend, state.blendScratch);
    if (kick && state.jumping)
    {
        const float recovery = std::clamp((state.time-(duration-0.22f))/0.22f,0.0f,1.0f);
        if (recovery > 0.f)
        {
            roundhouse::BlendPose(*animation, state.blendFrom, state.pose, 1.f-recovery, state.blendScratch);
        }
    }
    animation->SetPoseOverride(state.pose.data(),static_cast<int>(state.pose.size()));
    UpdateWeapons();

    if (kick)
    {
        auto heelMatrix = JointWorld(*animation,model->mtx,state.heel);
        const CVECTOR heel = roundhouse::JointPointWorld(heelMatrix, state.heelOffset);
        const CVECTOR knee = JointWorld(*animation,model->mtx,state.rig.bodies[13].joint).Pos();
        const CVECTOR leftHeel = state.jumping ? roundhouse::JointPointWorld(
            JointWorld(*animation,model->mtx,state.rig.bodies[11].joint),state.leftHeelOffset) : heel;
        const CVECTOR leftKnee = state.jumping ? JointWorld(*animation,model->mtx,state.rig.bodies[10].joint).Pos() : knee;
        if (!state.hasPreviousHeel) { state.previousHeel = heel; state.previousKnee = knee;
            state.previousLeftHeel = leftHeel; state.previousLeftKnee = leftKnee; state.hasPreviousHeel = true; }
        if ((state.jumping || !state.contacted) && state.time >= state.strikeStartTime && state.time <= state.strikeEndTime)
        {
            struct Contact { entid_t victim; CVECTOR point; float distance; };
            std::vector<Contact> contacts;
            Contact nearest{invalid_entity, {}, 0.0f};
            const CVECTOR torso = JointWorld(*animation,model->mtx,state.rig.bodies[1].joint).Pos();
            auto candidates = location->supervisor.FindCharacters(this,state.jumping ? 2.5f : ContactSearchRadius,state.jumping ? 0.0f : 100.0f,0.0f,0.0f,false,false);
            for (const auto &candidate : candidates)
            {
                auto *other = candidate.c;
                if (!other || other == this || other->IsDead() || other->IsPhysicalRagdoll() ||
                    std::find(state.hitTargets.begin(),state.hitTargets.end(),other->GetId()) != state.hitTargets.end() ||
                    !other->PrepareRoundhouse()) continue;
                auto *otherModel = other->Model();
                auto *otherAnimation = otherModel ? otherModel->GetAnimation() : nullptr;
                if (!otherAnimation) continue;
                auto headMatrix = JointWorld(*otherAnimation,otherModel->mtx,other->roundhouseState->head);
                const auto head = roundhouse::JointPointWorld(headMatrix, other->roundhouseState->headOffset);
                float distance = SegmentDistanceSquared(head,state.previousHeel,heel);
                CVECTOR contactPoint = head;
                bool hit = distance < 0.25f*0.25f;
                if (!state.jumping && !hit)
                {
                    for (const auto &body : other->roundhouseState->rig.bodies)
                    {
                        const CVECTOR a = JointWorld(*otherAnimation,otherModel->mtx,body.joint).Pos();
                        const CVECTOR b = body.endJoint >= 0 ? JointWorld(*otherAnimation,otherModel->mtx,body.endJoint).Pos() : a;
                        const float d = std::min(deck_collision::distance2(a,b,state.previousKnee,state.previousHeel,heel),
                                                 deck_collision::distance2(a,b,state.previousKnee,heel,knee));
                        const float radius = body.radius + 0.20f;
                        if (d < radius*radius && (!hit || d < distance))
                        {
                            const CVECTOR axis = b-a;
                            const float axisLength = ~axis;
                            const float t = axisLength > 1.0e-8f ? std::clamp(((heel-a) | axis)/axisLength,0.0f,1.0f) : 0.0f;
                            hit = true; distance = d; contactPoint = a+axis*t;
                        }
                    }
                }
                if (state.jumping)
                {
                    hit = false;
                    for (const auto &body : other->roundhouseState->rig.bodies)
                    {
                        const CVECTOR a = JointWorld(*otherAnimation,otherModel->mtx,body.joint).Pos();
                        const CVECTOR b = body.endJoint >= 0 ? JointWorld(*otherAnimation,otherModel->mtx,body.endJoint).Pos() : a;
                        const auto sweep = [&](const CVECTOR &pk, const CVECTOR &ph, const CVECTOR &k, const CVECTOR &h) {
                            return std::min(deck_collision::distance2(a,b,pk,ph,h),
                                            deck_collision::distance2(a,b,pk,h,k));
                        };
                        const float d = std::min(sweep(state.previousKnee,state.previousHeel,knee,heel),
                                                 sweep(state.previousLeftKnee,state.previousLeftHeel,leftKnee,leftHeel));
                        const float radius = body.radius + 0.38f;
                        if (d < radius*radius && (!hit || d < distance))
                        { hit = true; distance = d; contactPoint = (a+b)*0.5f; }
                    }
                }
                if (!hit || location->Trace(torso,contactPoint) < 0.995f ||
                    location->Trace(state.strikeOrigin,contactPoint) < 0.995f ||
                    (!state.jumping && location->Trace(state.previousHeel,contactPoint) < 0.995f)) continue;
                if (!CanHitRoundhouse(*this,*other)) continue;
                if (state.jumping) contacts.push_back({other->GetId(),contactPoint,distance});
                else if (!nearest.victim || distance < nearest.distance) nearest = {other->GetId(),contactPoint,distance};
            }
            const auto attacker = GetId();
            const auto resolved = state.jumping ? std::span<const Contact>(contacts) :
                std::span<const Contact>(&nearest, nearest.victim ? 1 : 0);
            for (const auto &contact : resolved)
            {
                auto *victim = static_cast<Character *>(core.GetEntityPointerSafe(contact.victim));
                if (!victim || victim->GetLocation() != location || victim->IsDead() || victim->IsPhysicalRagdoll() ||
                    !CanHitRoundhouse(*this,*victim)) continue;
                CVECTOR direction = contact.point - (curPos+CVECTOR(0,1.2f,0));
                direction.y = 0;
                const float length = std::sqrt(~direction);
                direction = length > 0.001f ? direction/length : CVECTOR(std::sin(ay),0,std::cos(ay));
                // Admit a victim before callbacks. A failed physics admission
                // must not let the same sweep apply damage again next frame.
                if (state.jumping) state.hitTargets.push_back(contact.victim);
                state.contacted = true;
                victim->BeginRoundhouseFall(contact.point,direction*(state.jumping ? 130.0f : 85.0f)+CVECTOR(0,state.jumping ? 32.0f : 22.0f,0),attacker);
                core.Event("Event_ChrSnd_Hit","i",victim->GetId());
                core.Event("Location_CharacterAttack","iislll",attacker,contact.victim,state.jumping ? "roundhouse_jump" : "roundhouse",0,0,-1);
                if (!core.IsEntityValid(attacker)) return true;
                if (state.phase != RoundhouseState::Phase::Kick) return IsRoundhouseControlled();
            }
        }
        state.previousHeel = heel;
        state.previousKnee = knee;
        state.previousLeftHeel = leftHeel;
        state.previousLeftKnee = leftKnee;
    }
    if (state.time >= duration)
    {
        CancelRoundhouse();
        UpdateAnimation();
    }
    return true;
}
