#include "port_rowboat.h"
#include "animation.h"
#include "controls.h"
#include "core.h"
#include "deck_camera.h"
#include "dx9render.h"
#include "geometry.h"
#include "island_base.h"
#include "math_inlines.h"
#include "sea_base.h"
#include "shared/layers.h"
#include "shared/messages.h"
#include "vma.hpp"

CREATE_CLASS(PORT_ROWBOAT)

namespace
{
constexpr float tau = 6.28318530718f;
MODEL *model(entid_t id) { return static_cast<MODEL *>(core.GetEntityPointer(id)); }
CVECTOR point(ATTRIBUTES *data, const char *prefix)
{
    const std::string key(prefix);
    return CVECTOR(data->GetAttributeAsFloat((key + "X").c_str()),
                   data->GetAttributeAsFloat((key + "Y").c_str()),
                   data->GetAttributeAsFloat((key + "Z").c_str()));
}
void writePoint(ATTRIBUTES *data, const char *prefix, const CVECTOR &value)
{
    const std::string key(prefix);
    data->SetAttributeUseFloat((key + "X").c_str(), value.x);
    data->SetAttributeUseFloat((key + "Y").c_str(), value.y);
    data->SetAttributeUseFloat((key + "Z").c_str(), value.z);
}
float smooth(float p) { p = std::clamp(p, 0.f, 1.f); return p * p * (3.f - 2.f * p); }
float wrapped(float p) { return p - floorf(p); }
float heading(const CVECTOR &direction) { return atan2f(direction.x, direction.z); }
float turnToward(float from, float to, float fraction)
{
    return from + std::remainder(to - from, tau) * smooth(fraction);
}
bool down(const char *name)
{
    if (!core.Controls || core.Controls->IsControlLocked(name)) return false;
    CONTROL_STATE control{};
    core.Controls->GetControlState(name, control);
    return control.state == CST_ACTIVE || control.state == CST_ACTIVATED;
}
}

bool PORT_ROWBOAT::Init()
{
    renderer = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    if (!renderer) return false;
    core.AddToLayer(SEA_EXECUTE, GetId(), 1000);
    core.AddToLayer(SEA_REALIZE, GetId(), 65530);
    return true;
}

PORT_ROWBOAT::~PORT_ROWBOAT() { ReleaseModels(true); }

bool PORT_ROWBOAT::Error(const char *reason)
{
    if (ATTRIBUTES::IsLive(travel)) travel->SetAttribute("error", reason);
    return false;
}

void PORT_ROWBOAT::ReleaseModels(bool all)
{
    core.EraseEntity(actor); actor = 0;
    action.clear();
    if (all)
    {
        core.EraseEntity(hull); hull = 0;
        core.EraseEntity(bench); bench = 0;
        for (auto &id : oars) { core.EraseEntity(id); id = 0; }
        for (auto id : ladder) core.EraseEntity(id);
        ladder.clear();
        deckCamera = 0;
        state = Inactive;
    }
}

bool PORT_ROWBOAT::ConfigureFrom(ATTRIBUTES *character, ATTRIBUTES *descriptor)
{
    if (state != Inactive && state != Handoff) return Error("transfer_active");
    if (!ATTRIBUTES::IsLive(character) || !ATTRIBUTES::IsLive(descriptor)) return false;
    auto *candidate = descriptor->GetAttributeClass("rig");
    if (descriptor->GetAttributeAsDword("version") != 1 || !candidate ||
        candidate->GetAttributeAsDword("version") != 1) return false;
    auto *appearance = character->GetAttributeClass("model");
    auto *animations = candidate->GetAttributeClass("animations");
    const char *animation = appearance ? static_cast<const char *>(appearance->GetAttribute("animation")) : nullptr;
    if (!animation || !animations || !animations->GetAttribute(animation)) return false;
    for (const char *key : {"hull", "oar", "bench", "ladder"})
        if (!candidate->GetAttribute(key)) return false;
    for (const char *key : {"rowPeriod", "climbStride", "ladderRungSpacing", "ladderWidth"})
        if (!(candidate->GetAttributeAsFloat(key) > .05f)) return false;
    ReleaseModels(true);
    hero = character; travel = descriptor; rig = candidate;
    seat = point(rig, "seat");
    return port_contact::finite(seat);
}

bool PORT_ROWBOAT::CreateModels()
{
    if (!ATTRIBUTES::IsLive(hero) || !ATTRIBUTES::IsLive(travel) || !ATTRIBUTES::IsLive(rig) ||
        !travel->GetAttributeAsDword("enabled")) return Error("inactive");
    auto *geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    if (!geometry) return Error("geometry_unavailable");
    const std::array<std::pair<entid_t *, entid_t>, 5> previous = {{{&hull, hull}, {&bench, bench},
        {&oars[0], oars[0]}, {&oars[1], oars[1]}, {&actor, actor}}};
    auto rollback = [&] {
        for (const auto &[slot, value] : previous)
            if (*slot != value) { core.EraseEntity(*slot); *slot = value; }
        action.clear();
    };
    auto load = [&](entid_t &id, const char *name, const char *texture) {
        if (model(id)) return true;
        if (!name || !*name) return false;
        id = core.CreateEntity("modelr");
        geometry->SetTexturePath(texture ? texture : "");
        const bool ok = id && core.Send_Message(id, "ls", MSG_MODEL_LOAD_GEO, name);
        geometry->SetTexturePath("");
        return ok;
    };
    auto *appearance = hero->GetAttributeClass("model");
    const char *baseAnimation = appearance ? static_cast<const char *>(appearance->GetAttribute("animation")) : nullptr;
    auto *animations = rig->GetAttributeClass("animations");
    const char *rowAnimation = animations && baseAnimation ? static_cast<const char *>(animations->GetAttribute(baseAnimation)) : nullptr;
    const char *heroName = hero->GetAttribute("model");
    if (!heroName || !rowAnimation) return Error("unsupported_protagonist_animation");
    const std::string actorPath = std::string("characters\\") + heroName;
    if (!load(hull, rig->GetAttribute("hull"), rig->GetAttribute("hullTexturePath")) ||
        !load(bench, rig->GetAttribute("bench"), "") ||
        !load(oars[0], rig->GetAttribute("oar"), "") || !load(oars[1], rig->GetAttribute("oar"), "") ||
        !load(actor, actorPath.c_str(), "characters\\") ||
        !core.Send_Message(actor, "ls", MSG_MODEL_LOAD_ANI, rowAnimation))
    {
        rollback();
        return Error("presentation_asset_load_failed");
    }
    auto *animated = model(actor);
    if (!animated || !animated->GetAnimation()) { rollback(); return Error("animation_unavailable"); }
    for (const char *name : {"row", "row_left", "row_right", "board", "leave", "climb", "climb_over"})
        if (!animated->GetAnimation()->Player(0).SetAction(name))
        { rollback(); return Error("required_animation_action_missing"); }
    for (int i = 0; auto *node = animated->GetNode(i); ++i) node->SetTechnique("Animation");
    action.clear();
    return true;
}

bool PORT_ROWBOAT::SetAction(const char *name, float position)
{
    auto *animated = model(actor);
    if (!animated || !animated->GetAnimation()) return false;
    auto &player = animated->GetAnimation()->Player(0);
    if (action != name)
    {
        const bool hadPose = !action.empty();
        if (hadPose) animated->GetAnimation()->CopyPlayerState(0, 1);
        if (!player.SetAction(name)) return false;
        player.Play();
        action = name;
        blendRemaining = hadPose ? .18f : 0.f;
    }
    player.Pause();
    player.SetPosition(std::clamp(position, 0.f, 1.f));
    return true;
}

bool PORT_ROWBOAT::WorldClear(const CVECTOR &from, const CVECTOR &to, float radius, float height, float step) const
{
    if (!port_contact::finite(from) || !port_contact::finite(to)) return false;
    const CVECTOR up(0.f, 1.f, 0.f), delta = to - from;
    auto clear = [&](MODEL *receiver) {
        return !receiver || (port_contact::canOccupy(*receiver, to, up, radius, height, step) &&
            deck_collision::canSweep(*receiver, from + up * (radius + .06f),
                from + up * (radius + .06f + height), delta, radius, step));
    };
    auto *island = static_cast<ISLAND_BASE *>(core.GetEntityPointer(core.GetEntityId("island")));
    if (island)
        for (const auto &surface : island->GetWorldSurfaces())
            if (!clear(model(surface.model))) return false;
    for (auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<VAI_OBJBASE *>(core.GetEntityPointer(id));
        auto *receiver = ship ? ship->GetModel() : nullptr;
        auto *path = receiver ? receiver->FindNode("path") : nullptr;
        const int flags = path ? path->flags : 0;
        if (path) path->flags &= ~NODE::CLIP_ENABLE;
        const bool result = clear(receiver);
        if (path) path->flags = flags;
        if (!result) return false;
    }
    return true;
}

bool PORT_ROWBOAT::ResolvePier(const CVECTOR &candidate, CVECTOR &support) const
{
    auto *island = static_cast<ISLAND_BASE *>(core.GetEntityPointer(core.GetEntityId("island")));
    if (!island || !port_contact::finite(candidate)) return false;
    float best = 1e10f;
    for (const auto &surface : island->GetWorldSurfaces())
    {
        auto *receiver = model(surface.model);
        CVECTOR found;
        if (!receiver || !deck_collision::findSupport(*receiver, candidate, CVECTOR(0.f, 1.f, 0.f), .7f, found, .3f) ||
            !port_contact::canOccupy(*receiver, found, CVECTOR(0.f, 1.f, 0.f))) continue;
        if (~(found - candidate) < best) { best = ~(found - candidate); support = found; }
    }
    return best < 1e10f;
}

void PORT_ROWBOAT::FloatBoat()
{
    auto *sea = static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
    if (!sea) return;
    CVECTOR position = boat.Pos();
    const CVECTOR ahead(sinf(yaw), 0.f, cosf(yaw)), right(cosf(yaw), 0.f, -sinf(yaw));
    auto height = [&](const CVECTOR &offset) { return sea->WaveXZ(position.x + offset.x, position.z + offset.z); };
    position.y = height(CVECTOR(0.f)) + travel->GetAttributeAsFloat("waterline");
    const float pitch = -atan2f(height(ahead * 1.8f) - height(ahead * -1.8f), 3.6f);
    const float roll = atan2f(height(right * .6f) - height(right * -.6f), 1.2f);
    boat = CMatrix(CVECTOR(pitch, yaw, roll), position);
}

bool PORT_ROWBOAT::BoatClear(const CMatrix &from, const CMatrix &to) const
{
    CMatrix previous(from), next(to);
    auto *m = model(hull);
    auto *root = m ? m->GetNode(0) : nullptr;
    if (!root || !root->geo) return false;
    GEOS::INFO geometry{};
    root->geo->GetInfo(geometry);
    const float radius = geometry.boxsize.x * .5f;
    const float halfLength = std::max(0.f, geometry.boxsize.z * .5f - radius);
    const float keel = geometry.boxcenter.y - geometry.boxsize.y * .5f;
    for (int i = -2; i <= 2; ++i)
    {
        const CVECTOR sample(geometry.boxcenter.x, keel, geometry.boxcenter.z + float(i) * halfLength * .5f);
        if (!WorldClear(previous * sample, next * sample, radius, .1f, -radius + .01f)) return false;
    }
    return true;
}

bool PORT_ROWBOAT::StartTransfer(entid_t cameraId, bool entering, bool restore)
{
    if (!ATTRIBUTES::IsLive(hero) || !ATTRIBUTES::IsLive(travel) || !ATTRIBUTES::IsLive(rig) ||
        (entering ? state != Inactive && state != Handoff : state != Rowing))
        return Error("invalid_state");
    if (entering && actor) return Error("previous_handoff_not_released");
    const bool retainedBerth = entering && state == Handoff && hull;
    const auto previousDeck = deckCamera;
    const auto previousContact = contact;
    const auto previousRoute = route;
    const auto previousLengths = lengths;
    const auto previousBoat = boat;
    const auto previousMooring = mooredBoat;
    const auto previousYaw = yaw, previousMooredYaw = mooredYaw;
    const auto previousLength = transferLength;
    const auto previousRouteEntering = routeEntering, previousLadderRoute = ladderRoute;
    if (PrepareTransfer(cameraId, entering, restore)) return true;
    for (auto id : ladder) core.EraseEntity(id);
    ladder.clear();
    deckCamera = 0;
    if (retainedBerth)
    {
        ReleaseModels(false);
        deckCamera = previousDeck; contact = previousContact; route = previousRoute; lengths = previousLengths;
        boat = previousBoat; mooredBoat = previousMooring; yaw = previousYaw; mooredYaw = previousMooredYaw;
        transferLength = previousLength; routeEntering = previousRouteEntering; ladderRoute = previousLadderRoute;
        state = Handoff;
        CreateLadder();
        Publish();
    }
    else if (entering) ReleaseModels(true);
    else state = Rowing;
    return false;
}

bool PORT_ROWBOAT::PrepareTransfer(entid_t cameraId, bool entering, bool restore)
{
    if (!travel || (entering ? state != Inactive && state != Handoff : state != Rowing)) return Error("invalid_state");
    const bool reuseBoat = entering && state == Handoff && hull;
    if (entering && !reuseBoat)
    {
        const CVECTOR position(travel->GetAttributeAsFloat("x"), travel->GetAttributeAsFloat("y"), travel->GetAttributeAsFloat("z"));
        yaw = travel->GetAttributeAsFloat("yaw");
        if (!port_contact::finite(position) || !std::isfinite(yaw)) return Error("invalid_pose");
        boat = CMatrix(CVECTOR(0.f, yaw, 0.f), position);
        FloatBoat();
        if (!CreateModels()) return false;
    }
    deckCamera = cameraId;
    auto *camera = cameraId ? dynamic_cast<DECK_CAMERA *>(core.GetEntityPointer(cameraId)) : nullptr;
    if (cameraId && !camera) return Error("deck_camera_unavailable");
    CVECTOR requested = point(travel, entering ? "source" : "target");
    transferYaw = travel->GetAttributeAsFloat(entering ? "sourceYaw" : "targetYaw");
    if (camera)
    {
        if (entering && !restore)
        {
            if (!camera->ObservePortWalker(contact, transferYaw)) return Error("walk_to_ship_edge_first");
            requested = contact.world;
        }
        if (entering && !restore && !reuseBoat)
        {
            auto *ship = camera->GetAIObj();
            auto *receiver = ship->GetModel();
            CVECTOR local;
            receiver->mtx.MulToInv(contact.world, local);
            GEOS::INFO hullBounds{};
            model(hull)->GetNode(0)->geo->GetInfo(hullBounds);
            const float side = local.x < 0.f ? -1.f : 1.f;
            const float outer = ship->GetRealBoxsize().x * .5f + 3.f;
            CVECTOR start = receiver->mtx * CVECTOR(side * outer, 0.f, local.z);
            CVECTOR end = receiver->mtx * CVECTOR(0.f, 0.f, local.z);
            auto *sea = static_cast<SEA_BASE *>(core.GetEntityPointer(core.GetEntityId("sea")));
            const float water = sea->WaveXZ(start.x, start.z);
            float shell = 2.f;
            for (float band : {-.1f, .2f, .8f})
            {
                start.y = end.y = water + band;
                const float hit = receiver->Trace(start, end);
                if (hit >= 0.f && hit <= 1.f) shell = std::min(shell, hit);
            }
            if (shell > 1.f) return Error("no_live_hull_side");
            CVECTOR outward = start - end; outward.y = 0.f;
            outward *= 1.f / sqrtf(~outward);
            const CVECTOR shellPoint = start + (end - start) * shell;
            const CVECTOR forward = receiver->mtx.Vz();
            yaw = heading(forward);
            bool foundWater = false;
            for (float clearance = .4f; clearance <= 2.f; clearance += .2f)
            {
                boat.Pos() = shellPoint + outward * (hullBounds.boxsize.x * .5f + clearance);
                FloatBoat();
                if (BoatClear(boat, boat)) { foundWater = true; break; }
            }
            if (!foundWater) return Error("no_free_water_beside_ship");
        }
        else if ((!entering || restore) && !camera->QueryPortContact(boat * seat, contact)) return Error("no_valid_deck_support");
        if (contact.up.y < .85f) return Error("unsafe_ship_inclination");
        if (~(contact.velocity - velocity) > 1.25f * 1.25f) return Error("unsafe_relative_ship_speed");
        auto *receiver = model(contact.model);
        receiver->mtx.MulToInv(boat.Pos(), mooredBoat);
        mooredYaw = yaw - heading(receiver->mtx.Vz());
    }
    else
    {
        contact = {};
        if (!ResolvePier(requested, contact.world)) return Error("no_valid_pier_support");
    }
    if (!MakeRoute(entering)) return false;
    if (entering && !actor && !CreateModels()) return false;
    if (entering && !BoatClear(boat, boat)) return Error("boat_launch_obstructed");
    progress = 0.f; returning = false;
    state = entering ? Entering : Leaving;
    speed = 0.f; velocity = CVECTOR(0.f);
    actorPosition = RoutePoint(0.f);
    actorYaw = entering ? transferYaw : yaw + rig->GetAttributeAsFloat("seatYaw");
    SetAction(ladderRoute && entering ? "climb_over" : entering ? "board" : "leave",
              ladderRoute && entering ? 1.f : 0.f);
    travel->SetAttribute("error", "");
    travel->SetAttributeUseFloat(entering ? "sourceYaw" : "targetYaw", transferYaw);
    Publish();
    return true;
}

bool PORT_ROWBOAT::MakeRoute(bool entering)
{
    routeEntering = entering;
    const CVECTOR boatSeat = boat * seat;
    const CVECTOR up = deckCamera ? contact.up : CVECTOR(0.f, 1.f, 0.f);
    CVECTOR outside = boatSeat, deck = contact.world;
    const CVECTOR gap = deck - boatSeat;
    if (gap.x * gap.x + gap.z * gap.z > 4.5f * 4.5f) return Error("contact_too_far");
    ladderRoute = deckCamera != 0;
    std::array<CVECTOR, 5> points;
    if (ladderRoute)
    {
        CVECTOR inward = gap - up * (gap | up);
        if (~inward < .01f) return Error("no_ladder_standoff");
        inward *= 1.f / sqrtf(~inward);
        auto *receiver = model(contact.model);
        const CVECTOR rayFrom = boatSeat + up * ((gap | up) + .65f);
        const CVECTOR rayTo = deck + up * .65f;
        const float hit = receiver->Trace(rayFrom, rayTo);
        outside = hit >= 0.f && hit <= 1.f ? rayFrom + (rayTo - rayFrom) * hit - inward * .58f : deck - inward * .8f;
        outside -= up * ((outside - boatSeat) | up);
        float railClearance = .35f;
        for (; railClearance <= 2.3f; railClearance += .15f)
        {
            const CVECTOR top = outside + up * (((deck - outside) | up) + railClearance);
            if (WorldClear(outside, top) && WorldClear(top, deck + up * railClearance) &&
                WorldClear(deck + up * railClearance, deck)) break;
        }
        if (railClearance > 2.3f) return Error("ladder_route_obstructed");
        points = {boatSeat, outside, outside + up * (((deck - outside) | up) + railClearance),
                  deck + up * railClearance, deck};
        if (deck.y <= boatSeat.y) return Error("deck_below_boat");
    }
    else
    {
        if (fabsf(gap.y) > 1.6f) return Error("pier_requires_gangway");
        points = {boatSeat, boatSeat + up * .35f, (boatSeat + deck) * .5f + up * .35f,
                  deck + up * .35f, deck};
    }
    if (entering) std::reverse(points.begin(), points.end());
    for (size_t i = 1; i < points.size(); ++i)
        if (!WorldClear(points[i - 1], points[i])) return Error("transfer_route_obstructed");
    transferLength = 0.f;
    for (size_t i = 0; i < 4; ++i)
    {
        lengths[i] = sqrtf(~(points[i + 1] - points[i]));
        transferLength += lengths[i];
    }
    if (!(transferLength > .1f) || transferLength > 40.f) return Error("invalid_transfer_length");
    route = points;
    if (deckCamera)
    {
        auto *receiver = model(contact.model);
        for (auto &p : route) { CVECTOR local; receiver->mtx.MulToInv(p, local); p = local; }
    }
    return CreateLadder();
}

bool PORT_ROWBOAT::CreateLadder()
{
    for (auto id : ladder) core.EraseEntity(id);
    ladder.clear();
    if (deckCamera)
    {
        const float rung = rig->GetAttributeAsFloat("ladderRungSpacing");
        const unsigned count = unsigned(ceilf(lengths[routeEntering ? 2 : 1] / rung)) + 5;
        if (count > 128) return Error("ladder_span_unsupported");
        for (unsigned i = 0; i < count; ++i)
        {
            auto id = core.CreateEntity("modelr");
            if (!id || !core.Send_Message(id, "ls", MSG_MODEL_LOAD_GEO, static_cast<const char *>(rig->GetAttribute("ladder"))))
            { core.EraseEntity(id); return Error("ladder_asset_load_failed"); }
            ladder.push_back(id);
        }
    }
    return true;
}

bool PORT_ROWBOAT::RefreshRoute()
{
    if (!deckCamera) return true;
    auto *camera = dynamic_cast<DECK_CAMERA *>(core.GetEntityPointer(deckCamera));
    if (!camera || !camera->RefreshPortContact(contact)) return Error("moving_contact_lost");
    if (state != Handoff && ~contact.velocity > 1.5f * 1.5f) return Error("ship_accelerated_during_transfer");
    auto *receiver = model(contact.model);
    boat.Pos() = receiver->mtx * mooredBoat;
    yaw = heading(receiver->mtx.Vz()) + mooredYaw;
    // Keep sea contact while following the actual hull's horizontal movement.
    FloatBoat();
    const bool entering = routeEntering;
    CVECTOR localSeat;
    receiver->mtx.MulToInv(boat * seat, localSeat);
    route[entering ? 4 : 0] = localSeat;
    route[entering ? 0 : 4] = [&] { CVECTOR p; receiver->mtx.MulToInv(contact.world, p); return p; }();
    return true;
}

CVECTOR PORT_ROWBOAT::RoutePoint(float distance, unsigned *segment) const
{
    unsigned i = 0;
    while (i < 3 && distance > lengths[i]) { distance -= lengths[i]; ++i; }
    if (segment) *segment = i;
    const float t = lengths[i] > .0001f ? std::clamp(distance / lengths[i], 0.f, 1.f) : 1.f;
    const CVECTOR local = route[i] + (route[i + 1] - route[i]) * t;
    auto *receiver = deckCamera ? model(contact.model) : nullptr;
    return receiver ? receiver->mtx * local : local;
}

void PORT_ROWBOAT::Row(float dt)
{
    const int ahead = int(down("ChrForward")) - int(down("ChrBackward"));
    turn = float(int(down("ChrStrafeRight")) - int(down("ChrStrafeLeft")));
    if (ahead) strokeDirection = ahead;
    const bool stroking = ahead || turn != 0.f;
    if (stroking) phase = wrapped(phase + dt * strokeDirection / rig->GetAttributeAsFloat("rowPeriod"));
    const float drive = stroking ? std::max(0.f, -sinf(tau * phase)) : 0.f;
    const float activeOars = turn == 0.f ? 2.f : 1.f;
    speed += (drive * float(strokeDirection) * activeOars * .85f - speed * fabsf(speed) * .2f - speed * .12f) * dt;
    speed = std::clamp(speed, -1.8f, 3.2f);
    yaw += turn * drive * float(strokeDirection) * .55f * dt;
    const CMatrix previous = boat;
    velocity = CVECTOR(sinf(yaw) * speed, 0.f, cosf(yaw) * speed);
    boat.Pos() += velocity * dt;
    FloatBoat();
    if (!BoatClear(previous, boat)) { boat = previous; speed = 0.f; velocity = CVECTOR(0.f); }
    actorPosition = boat * seat;
    actorYaw = yaw + rig->GetAttributeAsFloat("seatYaw");
    SetAction(turn < 0.f ? "row_left" : turn > 0.f ? "row_right" : "row", phase);
}

void PORT_ROWBOAT::Transfer(float dt)
{
    if (!RefreshRoute()) { interrupted = state; state = Blocked; return; }
    const float next = std::clamp(progress + (returning ? -1.f : 1.f) * dt * .64f, 0.f, transferLength);
    const CVECTOR desired = RoutePoint(next);
    if (!WorldClear(actorPosition, desired)) { interrupted = state; state = Blocked; Error("transfer_obstructed"); return; }
    progress = next;
    actorPosition = desired;
    unsigned segment;
    RoutePoint(progress, &segment);
    const bool entering = state == Entering;
    const unsigned climbSegment = entering ? 2 : 1;
    float before = 0.f;
    for (unsigned i = 0; i < segment; ++i) before += lengths[i];
    const float t = lengths[segment] > .0001f ? (progress - before) / lengths[segment] : 1.f;
    if (ladderRoute && segment == climbSegment)
    {
        const float climbed = entering ? lengths[segment] * (1.f - t) : lengths[segment] * t;
        SetAction("climb", wrapped(climbed / rig->GetAttributeAsFloat("climbStride")));
    }
    else if (ladderRoute && (entering ? segment < 2 : segment > 1))
    {
        const float overLength = entering ? lengths[0] + lengths[1] : lengths[2] + lengths[3];
        const float over = entering ? 1.f - progress / overLength :
            (progress - lengths[0] - lengths[1]) / overLength;
        SetAction("climb_over", over);
    }
    else
    {
        const float boardPhase = ladderRoute ? t : progress / transferLength;
        SetAction(entering ? "board" : "leave", boardPhase);
    }
    if (ladderRoute)
    {
        const float climbYaw = heading(contact.world - boat.Pos());
        const float startYaw = entering ? transferYaw : yaw + rig->GetAttributeAsFloat("seatYaw");
        actorYaw = turnToward(startYaw, climbYaw, progress / std::max(.2f, lengths[0]));
    }
    else actorYaw = turnToward(entering ? transferYaw : yaw + rig->GetAttributeAsFloat("seatYaw"),
        entering ? yaw + rig->GetAttributeAsFloat("seatYaw") : transferYaw, progress / transferLength);

    if ((!returning && progress >= transferLength) || (returning && progress <= 0.f))
    {
        const bool onBoat = entering != returning;
        if (onBoat)
        {
            state = Rowing; deckCamera = 0;
            actorPosition = boat * seat; actorYaw = yaw + rig->GetAttributeAsFloat("seatYaw");
            SetAction("row", phase);
            for (auto id : ladder) core.EraseEntity(id);
            ladder.clear();
        }
        else
        {
            if (deckCamera)
            {
                auto *camera = dynamic_cast<DECK_CAMERA *>(core.GetEntityPointer(deckCamera));
                if (!camera || !camera->PlacePortContact(contact, transferYaw))
                { interrupted = state; state = Blocked; Error("deck_placement_invalidated"); return; }
            }
            state = Handoff;
            actorYaw = transferYaw;
        }
        returning = false;
    }
}

void PORT_ROWBOAT::Publish()
{
    if (!ATTRIBUTES::IsLive(travel)) return;
    travel->SetAttributeUseDword("state", state);
    travel->SetAttributeUseDword("heroOwned", actor != 0);
    travel->SetAttributeUseDword("returning", returning);
    travel->SetAttributeUseDword("interrupted", interrupted);
    travel->SetAttributeUseDword("transferDeck", deckCamera != 0);
    travel->SetAttributeUseDword("transferEntering", routeEntering);
    travel->SetAttributeUseDword("handoffDeck", state == Handoff && deckCamera != 0);
    travel->SetAttributeUseFloat("x", boat.Pos().x); travel->SetAttributeUseFloat("y", boat.Pos().y); travel->SetAttributeUseFloat("z", boat.Pos().z);
    travel->SetAttributeUseFloat("yaw", yaw); travel->SetAttributeUseFloat("speed", speed);
    travel->SetAttributeUseFloat("phase", phase); travel->SetAttributeUseFloat("progress", progress);
    travel->SetAttributeUseFloat("mooredYaw", mooredYaw);
    travel->SetAttributeUseFloat("heroYaw", actorYaw);
    writePoint(travel, "hero", actorPosition);
    if (state != Rowing && state != Inactive)
    {
        writePoint(travel, "target", contact.world);
        writePoint(travel, "contactLocal", contact.local);
        writePoint(travel, "mooredBoat", mooredBoat);
        for (size_t i = 0; i < route.size(); ++i)
            writePoint(travel, ("route" + std::to_string(i)).c_str(), route[i]);
    }
}

void PORT_ROWBOAT::Update(uint32_t delta)
{
    if (!ATTRIBUTES::IsLive(hero) || !ATTRIBUTES::IsLive(travel) || !ATTRIBUTES::IsLive(rig) ||
        !travel->GetAttributeAsDword("enabled") || !core.GetEntityId("sea"))
    { ReleaseModels(true); Publish(); return; }
    if (state == Inactive || (!actor && state != Handoff) || !delta ||
        core.GetTimeScale() <= 0.f || core.IsLayerFrozen(SEA_EXECUTE)) return;
    const float dt = std::min(float(delta) * .001f, .1f);
    blendRemaining = std::max(0.f, blendRemaining - dt);
    if (state == Rowing) Row(dt);
    else if (state == Entering || state == Leaving) Transfer(dt);
    else if (state == Blocked && RefreshRoute())
    {
        // A moving obstruction may clear without discarding a visible actor.
        const CVECTOR next = RoutePoint(std::clamp(progress + (returning ? -1.f : 1.f) * .02f, 0.f, transferLength));
        if (WorldClear(actorPosition, next)) { state = interrupted; travel->SetAttribute("error", ""); }
    }
    else if (state == Handoff && deckCamera && RefreshRoute()) actorPosition = contact.world;
    else if (state == Handoff && !deckCamera) FloatBoat();
    if (auto *m = model(actor))
    {
        m->GetAnimation()->Player(0).SetBlend(1.f - blendRemaining / .18f);
        m->GetAnimation()->Player(1).SetBlend(blendRemaining / .18f);
    }
    Publish();
}

void PORT_ROWBOAT::Present(uint32_t delta)
{
    if (state == Inactive || !hull || !ATTRIBUTES::IsLive(travel) || !travel->GetAttributeAsDword("enabled")) return;
    if (auto *m = model(hull)) m->mtx = boat;
    if (auto *m = model(bench)) m->mtx = boat;
    for (unsigned i = 0; i < oars.size(); ++i)
        if (auto *m = model(oars[i]))
        {
            const bool right = i == 0;
            const bool feather = (turn > 0.f && right) || (turn < 0.f && !right);
            const float sweep = feather ? 0.f : rig->GetAttributeAsFloat("oarSweep") * cosf(tau * phase);
            const float lift = feather ? rig->GetAttributeAsFloat("oarFeather", .3f) : rig->GetAttributeAsFloat("oarLift") * sinf(tau * phase);
            m->mtx = CMatrix(CVECTOR(0.f, right ? sweep : PI - sweep, right ? lift : -lift),
                             point(rig, right ? "oarRight" : "oarLeft")) * boat;
        }
    if (auto *m = model(actor))
    {
        CMatrix pose(CVECTOR(0.f, actorYaw, 0.f), actorPosition);
        if (state == Rowing) pose = CMatrix(CVECTOR(0.f, rig->GetAttributeAsFloat("seatYaw"), 0.f), seat) * boat;
        else if (deckCamera && ladderRoute)
        {
            auto *receiver = model(contact.model);
            const float receiverYaw = heading(receiver->mtx.Vz());
            pose = CMatrix(CVECTOR(0.f, actorYaw - receiverYaw, 0.f), CVECTOR(0.f)) * receiver->mtx;
            pose.Pos() = actorPosition;
            if (action == "climb_over")
            {
                const float t = smooth(m->GetAnimation()->Player(0).GetPosition());
                const CVECTOR compensation(0.f, rig->GetAttributeAsFloat("climbOverY") * t,
                                                rig->GetAttributeAsFloat("climbOverZ") * t);
                pose.Pos() -= pose * compensation - pose * CVECTOR(0.f);
            }
        }
        m->mtx = pose;
    }
    if (!ladder.empty() && model(contact.model))
    {
        auto *receiver = model(contact.model);
        const bool entering = routeEntering;
        const CVECTOR base = route[entering ? 3 : 1];
        const CVECTOR inward = route[entering ? 0 : 4] - base;
        const float localYaw = heading(inward);
        const float spacing = rig->GetAttributeAsFloat("ladderRungSpacing");
        for (size_t i = 0; i < ladder.size(); ++i)
            if (auto *m = model(ladder[i]))
            {
                const float plane = rig->GetAttributeAsFloat("ladderPlaneZ");
                const CVECTOR offset(sinf(localYaw) * plane,
                    rig->GetAttributeAsFloat("ladderRungPhaseY") + float(i) * spacing, cosf(localYaw) * plane);
                m->mtx = CMatrix(CVECTOR(0.f, localYaw, 0.f), base + offset) * receiver->mtx;
            }
    }
    uint32_t lighting;
    renderer->GetRenderState(storm_render::RenderLighting, &lighting);
    renderer->SetRenderState(storm_render::RenderLighting, true);
    for (auto id : {hull, bench, oars[0], oars[1], actor})
        if (auto *m = model(id)) m->ProcessStage(Stage::realize, delta);
    for (auto id : ladder) if (auto *m = model(id)) m->ProcessStage(Stage::realize, delta);
    renderer->SetRenderState(storm_render::RenderLighting, lighting);
    if (actor && travel->GetAttributeAsDword("cameraOwned", 1))
    {
        const CVECTOR target = actorPosition + CVECTOR(0.f, 1.2f, 0.f);
        CVECTOR eye = target - CVECTOR(sinf(yaw), -.35f, cosf(yaw)) * 4.f;
        const CVECTOR desired = eye;
        eye = target + CVECTOR(0.f, .1f, 0.f);
        // Camera retraction uses actual solid receivers, not the hero's sweep.
        for (float t = 1.f; t > .1f; t -= .1f)
            if (WorldClear(target, target + (desired - target) * t, .12f, .01f))
            { eye = target + (desired - target) * t; break; }
        renderer->SetCamera(eye, target, CVECTOR(0.f, 1.f, 0.f));
    }
}

void PORT_ROWBOAT::ProcessStage(Stage stage, uint32_t delta)
{
    if (stage == Stage::execute) Update(delta);
    else if (stage == Stage::realize) Present(delta);
}

uint64_t PORT_ROWBOAT::ProcessMessage(MESSAGE &message)
{
    switch (message.Long())
    {
    case Configure: {
        auto *character = message.AttributePointer();
        return ConfigureFrom(character, message.AttributePointer());
    }
    case Begin: return StartTransfer(message.ParamValid() ? message.EntityID() : invalid_entity, true);
    case RequestTransfer: return StartTransfer(message.ParamValid() ? message.EntityID() : invalid_entity, false);
    case Cancel:
        if (state == Entering || state == Leaving || state == Blocked)
        { returning = true; if (state == Blocked) state = interrupted; Publish(); return 1; }
        return 0;
    case ReleaseHero:
        if (state != Handoff) return 0;
        ReleaseModels(false); Publish(); return 1;
    case Restore: {
        const auto camera = message.ParamValid() ? message.EntityID() : invalid_entity;
        if (!travel || state != Inactive) return 0;
        const auto rawState = travel->GetAttributeAsDword("state");
        const auto savedState = rawState == Blocked ? travel->GetAttributeAsDword("interrupted") : rawState;
        const float savedProgress = travel->GetAttributeAsFloat("progress");
        const float savedPhase = travel->GetAttributeAsFloat("phase");
        const float savedSpeed = travel->GetAttributeAsFloat("speed");
        const bool savedReturning = travel->GetAttributeAsDword("returning") != 0;
        std::array<CVECTOR, 5> savedRoute;
        for (size_t i = 0; i < savedRoute.size(); ++i)
            savedRoute[i] = point(travel, ("route" + std::to_string(i)).c_str());
        const CVECTOR savedContact = point(travel, "contactLocal");
        const CVECTOR savedMooring = point(travel, "mooredBoat");
        const float savedMooredYaw = travel->GetAttributeAsFloat("mooredYaw");
        const bool savedRouteEntering = travel->GetAttributeAsDword("transferEntering") != 0;
        const CVECTOR savedActor = point(travel, "hero");
        const float savedActorYaw = travel->GetAttributeAsFloat("heroYaw");
        if (savedState == Rowing)
        {
            yaw = travel->GetAttributeAsFloat("yaw");
            boat = CMatrix(CVECTOR(0.f, yaw, 0.f), CVECTOR(travel->GetAttributeAsFloat("x"), travel->GetAttributeAsFloat("y"), travel->GetAttributeAsFloat("z")));
            if (!CreateModels()) return 0;
            FloatBoat(); state = Rowing;
            phase = wrapped(savedPhase); speed = std::clamp(savedSpeed, -1.8f, 3.2f);
            actorPosition = boat * seat; actorYaw = yaw + rig->GetAttributeAsFloat("seatYaw");
            SetAction("row", phase); Publish(); return 1;
        }
        if (savedState == Handoff)
        {
            yaw = travel->GetAttributeAsFloat("yaw");
            boat = CMatrix(CVECTOR(0.f, yaw, 0.f), CVECTOR(travel->GetAttributeAsFloat("x"), travel->GetAttributeAsFloat("y"), travel->GetAttributeAsFloat("z")));
            if (!CreateModels()) return 0;
            FloatBoat(); state = Handoff; deckCamera = camera;
            route = savedRoute; routeEntering = savedRouteEntering; ladderRoute = camera != 0;
            mooredBoat = savedMooring; mooredYaw = savedMooredYaw;
            actorPosition = savedActor; actorYaw = savedActorYaw; phase = wrapped(savedPhase);
            if (camera)
            {
                auto *receiver = dynamic_cast<DECK_CAMERA *>(core.GetEntityPointer(camera));
                if (!receiver || !receiver->QueryPortContact(boat * seat, contact))
                { ReleaseModels(true); return Error("saved_berth_receiver_missing"); }
                contact.local = savedContact;
                if (!receiver->RefreshPortContact(contact))
                { ReleaseModels(true); return Error("saved_berth_contact_invalidated"); }
                for (size_t i = 0; i < lengths.size(); ++i) lengths[i] = sqrtf(~(route[i + 1] - route[i]));
                if (!CreateLadder()) { ReleaseModels(true); return 0; }
            }
            ReleaseModels(false); Publish(); return 1;
        }
        if (savedState != Entering && savedState != Leaving) return Error("saved_transfer_requires_recovery");
        if (savedState == Entering)
        { if (!StartTransfer(camera, true, true)) return 0; }
        else
        {
            yaw = travel->GetAttributeAsFloat("yaw");
            boat = CMatrix(CVECTOR(0.f, yaw, 0.f), CVECTOR(travel->GetAttributeAsFloat("x"), travel->GetAttributeAsFloat("y"), travel->GetAttributeAsFloat("z")));
            if (!CreateModels()) return 0;
            FloatBoat(); state = Rowing;
            if (!StartTransfer(camera, false, true)) { ReleaseModels(true); return 0; }
        }
        if (camera)
        {
            contact.local = savedContact;
            auto *receiver = dynamic_cast<DECK_CAMERA *>(core.GetEntityPointer(camera));
            if (!receiver || !receiver->RefreshPortContact(contact))
            { ReleaseModels(true); return Error("saved_deck_contact_invalidated"); }
        }
        route = savedRoute;
        routeEntering = savedRouteEntering;
        mooredBoat = savedMooring;
        mooredYaw = savedMooredYaw;
        transferLength = 0.f;
        for (size_t i = 0; i < lengths.size(); ++i)
        {
            if (!port_contact::finite(route[i]) || !port_contact::finite(route[i + 1]))
            { ReleaseModels(true); return Error("saved_transfer_route_invalid"); }
            lengths[i] = sqrtf(~(route[i + 1] - route[i]));
            transferLength += lengths[i];
        }
        if (!(transferLength > .1f) || transferLength > 40.f)
        { ReleaseModels(true); return Error("saved_transfer_length_invalid"); }
        for (unsigned i = 0; i < 4; ++i)
        {
            float begin = 0.f;
            for (unsigned j = 0; j < i; ++j) begin += lengths[j];
            if (!WorldClear(RoutePoint(begin), RoutePoint(begin + lengths[i])))
            { ReleaseModels(true); return Error("saved_transfer_route_obstructed"); }
        }
        if (!CreateLadder()) { ReleaseModels(true); return 0; }
        progress = std::clamp(savedProgress, 0.f, transferLength);
        returning = savedReturning; actorPosition = RoutePoint(progress); phase = wrapped(savedPhase);
        Publish(); return 1;
    }
    }
    return 0;
}
