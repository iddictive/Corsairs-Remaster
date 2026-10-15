#include "deck_camera.h"
#include "shared/sea_ai/deck_collision.hpp"

#include "core.h"
#include "animation.h"
#include <cstdlib>
#include "geometry.h"

#include "save_load.h"
#include "controls.h"
#include "os_window.hpp"
#include "shared/layers.h"
#include "math_inlines.h"
#include "shared/messages.h"

#define DISCR_F_VAL 0.00001f
#define EQU_FLOAT(x, y) (x) - (y) > DISCR_F_VAL ? false : (y) - (x) > DISCR_F_VAL ? false : true
#define BEF_FLOAT(x, y) (y) - (x) >= DISCR_F_VAL

DECK_CAMERA::DECK_CAMERA()
{
    vb_id = 0;
    RenderService = nullptr;
    camera_pos.y = 1.0f;
    pACharacter = nullptr;
    pathNode = nullptr;
    bLoad = false;
}

DECK_CAMERA::~DECK_CAMERA()
{
    if (zoomWindow && zoomWindowSubscription >= 0)
        zoomWindow->Unsubscribe(zoomWindowSubscription);
    ReleaseWalker();
}

void DECK_CAMERA::ReleaseWalker()
{
    core.EraseEntity(walkerId);
    walkerId = 0;
    walkerVisible = false;
    walkerAction.clear();
}

bool DECK_CAMERA::EnsureWalker()
{
    if (walkerId)
        return core.GetEntityPointer(walkerId) != nullptr;
    if (walkerLoadFailed || !pACharacter)
        return false;
    auto *modelAttr = pACharacter->GetAttributeClass("model");
    const char *name = pACharacter->GetAttribute("model");
    const char *animation = modelAttr ? static_cast<const char *>(modelAttr->GetAttribute("animation")) : nullptr;
    if (!name || !*name || !animation || !*animation)
        return false;
    auto *geometry = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    if (!geometry)
        return false;
    walkerId = core.CreateEntity("modelr");
    geometry->SetTexturePath("characters\\");
    const std::string path = std::string("characters\\") + name;
    const bool loaded = walkerId && core.Send_Message(walkerId, "ls", MSG_MODEL_LOAD_GEO, path.c_str());
    geometry->SetTexturePath("");
    if (!loaded || !core.Send_Message(walkerId, "ls", MSG_MODEL_LOAD_ANI, animation))
    {
        core.Trace("Deck walk: protagonist model/animation failed to load: %s / %s", name, animation);
        ReleaseWalker();
        walkerLoadFailed = true;
        return false;
    }
    auto *model = static_cast<MODEL *>(core.GetEntityPointer(walkerId));
    if (!model || !model->GetAnimation())
    {
        ReleaseWalker();
        walkerLoadFailed = true;
        return false;
    }
    for (int i = 0; auto *node = model->GetNode(i); ++i)
        node->SetTechnique("Animation");
    return true;
}

void DECK_CAMERA::UpdateWalker(bool moving, bool running)
{
    if (!EnsureWalker())
        return;
    auto *model = static_cast<MODEL *>(core.GetEntityPointer(walkerId));
    // The navigation path and visible ship deck are independently authored.
    // Keep path support for movement, but put the rendered feet on the nearest
    // visible surface around it so regional offsets do not make the actor float
    // over the planks or sink into the stairs.
    CMatrix walker = CMatrix(CVECTOR(0.f, walkerYaw, 0.f), camera_pos) * pathNode->glob_mtx;
    CVECTOR support;
    if (ResolveWalkerSupport(camera_pos, support))
        walker.Pos() = support;
    model->mtx = walker;
    const char *action = moving ? (running ? "run" : "walk") : "idle_1";
    auto &player = model->GetAnimation()->Player(0);
    if (walkerAction != action || !player.IsPlaying())
    {
        if (player.SetAction(action))
        {
            player.Play();
            walkerAction = action;
        }
    }
}

bool DECK_CAMERA::ResolveWalkerSupport(const CVECTOR &position, CVECTOR &support) const
{
    const CVECTOR origin = pathNode->glob_mtx * position;
    CVECTOR up = pathNode->glob_mtx * CVECTOR(0.f, 1.f, 0.f) - pathNode->glob_mtx * CVECTOR(0.f);
    up *= 1.f / sqrtf(~up);
    // MODEL::Clip also traverses the hidden navigation mesh. Because origin
    // lies on that mesh, a nearest-hit query would otherwise select the path
    // itself at distance zero and never reach the visible planks.
    const int32_t pathFlags = pathNode->flags;
    pathNode->flags &= ~NODE::CLIP_ENABLE;
    const bool found = deck_collision::findSupport(*pModel, origin, up, 1.5f, support, MEN_STEP_UP);
    pathNode->flags = pathFlags;
    return found;
}

bool DECK_CAMERA::CanWalkTo(const CVECTOR &from, const CVECTOR &to)
{
    const CVECTOR delta = to - from;
    if (delta.x * delta.x + delta.z * delta.z < 0.0000001f)
        return true;
    const CVECTOR localOrigin = pathNode->glob_mtx * CVECTOR(0.f);
    CVECTOR up = pathNode->glob_mtx * CVECTOR(0.f, 1.f, 0.f) - localOrigin;
    up *= 1.f / sqrtf(~up);
    CVECTOR fromSupport = pathNode->glob_mtx * from;
    CVECTOR toSupport = pathNode->glob_mtx * to;
    ResolveWalkerSupport(from, fromSupport);
    ResolveWalkerSupport(to, toSupport);
    const CVECTOR bottom = fromSupport + up * .32f;
    const CVECTOR top = fromSupport + up * 1.42f;
    const CVECTOR shift = toSupport - fromSupport;
    const int32_t pathFlags = pathNode->flags;
    pathNode->flags &= ~NODE::CLIP_ENABLE;
    const bool clear = deck_collision::canSweep(*pModel, bottom, top, shift, .28f, MEN_STEP_UP);
    pathNode->flags = pathFlags;
    return clear;
}

bool DECK_CAMERA::Init()
{
    // GUARD(DECK_CAMERA::Init())
    // core.SystemMessages(GetId(),true);
    SetDevice();
    zoomWindow = core.GetWindow();
    if (zoomWindow)
        zoomWindowSubscription = zoomWindow->Subscribe([this](const storm::OSWindow::Event &event) {
            if (event == storm::OSWindow::FocusLost || event == storm::OSWindow::Closed)
            {
                zoomWindowFocused = false;
                ResetAimZoomProjection();
            }
            else if (event == storm::OSWindow::FocusGained)
                zoomWindowFocused = true;
        });
    // UNGUARD
    return true;
}

void DECK_CAMERA::SetDevice()
{
    RenderService = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    Assert(RenderService);
}

bool DECK_CAMERA::CreateState(ENTITY_STATE_GEN *state_gen)
{
    return true;
}

bool DECK_CAMERA::LoadState(ENTITY_STATE *state)
{
    SetDevice();
    return true;
}

void DECK_CAMERA::Execute(uint32_t Delta_Time)
{
    if (!isOn() || !isActive())
    {
        aimZoom.reset();
        ReleaseWalker();
        return;
    }
    if (!FindShip() || !FindPath())
    {
        aimZoom.reset();
        ReleaseWalker();
        return;
    }

    SetPerspective(AttributesPointer->GetAttributeAsFloat("Perspective"));
    UpdateAimZoom();

    Move(Delta_Time);
}

void DECK_CAMERA::ResetAimZoomProjection()
{
    if (isOn() && isActive() && RenderService && aimZoom.magnification() > 1.f)
    {
        CVECTOR position, angles;
        float perspective;
        RenderService->GetCamera(position, angles, perspective);
        // Restore only our current projection. A menu/telescope/other camera
        // may already own the renderer, and must not be overwritten.
        if (std::abs(perspective - zoomRenderedPerspective) < .00001f)
            RenderService->SetCamera(position, angles, GetPerspective());
    }
    aimZoom.reset();
}

void DECK_CAMERA::UpdateAimZoom()
{
    const auto now = std::chrono::steady_clock::now();
    const float elapsed = zoomLastUpdate == std::chrono::steady_clock::time_point{} ? 0.f :
                          std::chrono::duration<float>(now - zoomLastUpdate).count();
    zoomLastUpdate = now;
    const auto pauseRevision = core.GetScenePauseRevision();
    if (zoomPauseRevision != pauseRevision)
    {
        ResetAimZoomProjection();
        zoomPauseRevision = pauseRevision;
    }
    const bool aiming = AttributesPointer->GetAttributeAsDword("WalkMode") != 0 &&
                        AttributesPointer->GetAttributeAsDword("ThirdPerson") == 0 &&
                        AttributesPointer->GetAttributeAsDword("TelescopeActive") == 0;
    if (!aiming || !zoomWindowFocused || core.GetTimeScale() <= 0.f || core.IsLayerFrozen(SEA_EXECUTE) ||
        !core.Controls || core.Controls->IsControlLocked("DeckAimZoom"))
    {
        aimZoom.reset();
        return;
    }
    CONTROL_STATE state{};
    core.Controls->GetControlState("DeckAimZoom", state);
    const bool down = state.state == CST_ACTIVE || state.state == CST_ACTIVATED;
    // Input classification needs real elapsed time: the engine simulation
    // timer clamps hitches to 100ms, which would misclassify a long release.
    // Only visual movement is clamped by Zoom; sea acceleration is ignored.
    aimZoom.update(elapsed, down);
}

void DECK_CAMERA::Realize(uint32_t Delta_Time)
{
    // Sea execution can be frozen while the last scene is still rendered
    // behind a pause/menu. Clear the latch even when Execute is not called.
    if (core.GetTimeScale() <= 0.f || core.IsLayerFrozen(SEA_EXECUTE) || !core.Controls ||
        core.Controls->IsControlLocked("DeckAimZoom"))
        ResetAimZoomProjection();
    if (!isOn() || !walkerVisible)
        return;
    if (auto *model = static_cast<MODEL *>(core.GetEntityPointer(walkerId)))
    {
        uint32_t lighting;
        RenderService->GetRenderState(storm_render::RenderLighting, &lighting);
        RenderService->SetRenderState(storm_render::RenderLighting, true);
        model->ProcessStage(Entity::Stage::realize, Delta_Time);
        RenderService->SetRenderState(storm_render::RenderLighting, lighting);
    }
}

void DECK_CAMERA::Move(uint32_t DeltaTime)
{
    if (!isActive())
        return;

    const bool walkMode = AttributesPointer->GetAttributeAsDword("WalkMode") != 0;
    const bool telescope = AttributesPointer->GetAttributeAsDword("TelescopeActive") != 0;
    const bool thirdPerson = walkMode && !telescope && AttributesPointer->GetAttributeAsDword("ThirdPerson") != 0;
    walkerVisible = false;

    pModel = static_cast<MODEL *>(GetModelPointer());
    Assert(pModel);

    pModel->Update();
    CONTROL_STATE cs;
    // Match both mouse axes to this frame's actual zoom without changing
    // user sensitivity settings or unzoomed/spyglass camera input.
    const float aimInputScale = 1.f / aimZoom.magnification();
    core.Controls->GetControlState("DeckCamera_Turn_H", cs);
    camera_ang.y += fSensivityAzimuthAngle * 3.0f * static_cast<float>(cs.fValue) * aimInputScale;

    core.Controls->GetControlState("DeckCamera_Left", cs);
    if (!walkMode && cs.state == CST_ACTIVE)
        camera_ang.y -= fSensivityAzimuthAngle * 15.f * static_cast<float>(cs.fValue);

    core.Controls->GetControlState("DeckCamera_Right", cs);
    if (!walkMode && cs.state == CST_ACTIVE)
        camera_ang.y += fSensivityAzimuthAngle * 15.f * static_cast<float>(cs.fValue);

    core.Controls->GetControlState("DeckCamera_Turn_V", cs);
    camera_ang.x -= fSensivityHeightAngle * 3.0f * static_cast<float>(cs.fValue) * aimInputScale;

    if (camera_ang.x > CAMERA_MAX_X)
        camera_ang.x = CAMERA_MAX_X;
    else if (camera_ang.x < CAMERA_MIN_X)
        camera_ang.x = CAMERA_MIN_X;
    if (camera_ang.y < 0.f)
        camera_ang.y += PIm2;
    else if (camera_ang.y > PIm2)
        camera_ang.y -= PIm2;

    // determine the real angle of rotation of the camera
    CMatrix glbRotMtx;
    glbRotMtx.BuildMatrix(camera_ang);
    CMatrix rotMtx;
    pathNode->glob_mtx.Get3X3(&rotMtx);
    glbRotMtx *= rotMtx;
    auto cx = cosf(camera_ang.x);
    auto sx = sinf(camera_ang.x);
    auto cy = cosf(camera_ang.y);
    auto sy = sinf(camera_ang.y);
    float xAng, yAng, zAng;
    auto v = glbRotMtx * CVECTOR(0, 0, 1.f);
    yAng = atan2f(v.x, v.z);
    glbRotMtx.RotateY(-yAng);
    v = glbRotMtx * CVECTOR(0, 0, 1.f);
    xAng = atan2f(-v.y, v.z);
    glbRotMtx.RotateX(-xAng);
    v = glbRotMtx * CVECTOR(0, 1.f, 0);
    zAng = atan2f(-v.x, v.y);
    CVECTOR s_ang;
    s_ang.x = xAng + (camera_ang.x - xAng) * (1.0f - fRockingX);
    s_ang.y = yAng;
    s_ang.z = zAng * fRockingZ;

    // find out new camera position
    auto prev_pos = camera_pos;
    auto speed0 = DeltaTime * fSensivityDistance;
    auto speed = 0.f;
    CVECTOR vShift;
    vShift.x = cx * sy;
    vShift.y = 0.f;
    vShift.z = cx * cy;

    /*CVECTOR strafeV = CVECTOR(0.f,0.f,0.f);
    core.Controls->GetControlState("DeckCamera_Left",cs);
    if(cs.state == CST_ACTIVE)
    {
      strafeV = !(CVECTOR(0.f, -1.f, 0.f)^vShift);
      speed=speed0;
    }

    core.Controls->GetControlState("DeckCamera_Right",cs);
    if(cs.state == CST_ACTIVE)
    {
      strafeV = !(CVECTOR(0.f, 1.f, 0.f)^vShift);
      speed=speed0;
    }*/

    core.Controls->GetControlState("DeckCamera_Forward", cs);
    if (cs.state == CST_ACTIVE)
        speed = speed0;

    core.Controls->GetControlState("DeckCamera_Backward", cs);
    if (cs.state == CST_ACTIVE)
    {
        speed = speed0;
        vShift.x *= -1.f;
        vShift.z *= -1.f;
    }

    // vShift += strafeV;

    bool running = false;
    float deckForward = 0.f;
    float deckRight = 0.f;
    if (walkMode)
    {
        auto held = [&](const char *name) {
            CONTROL_STATE state;
            core.Controls->GetControlState(name, state);
            return state.state == CST_ACTIVE || state.state == CST_ACTIVATED;
        };
        deckForward = float(held("ChrForward")) - float(held("ChrBackward"));
        deckRight = float(held("ChrStrafeRight")) - float(held("ChrStrafeLeft"));
        const float length = sqrtf(deckForward * deckForward + deckRight * deckRight);
        speed = 0.f;
        running = held("ChrRun");
        if (length > 0.f && !telescope)
        {
            // Horizontal normalized motion: pitch and diagonal input cannot change speed.
            const float walkSin = sinf(camera_ang.y), walkCos = cosf(camera_ang.y);
            vShift = CVECTOR((walkSin * deckForward + walkCos * deckRight) / length, 0.f,
                             (walkCos * deckForward - walkSin * deckRight) / length);
            speed = std::min(DeltaTime, uint32_t(100)) * (running ? 0.004f : 0.0018f);
            walkerYaw = atan2f(vShift.x, vShift.z);
        }
        else if (!walkerId)
            walkerYaw = camera_ang.y;
    }

    if (speed != 0.f)
    {
        CVECTOR src, dst;
        src.y = 500.f;
        dst.y = -500.f;
        auto bNoFinded = true;
        auto vRes = camera_pos;
        auto step = MEN_STEP_MIN;
        float len;
        auto prev_res = vRes;
        CVECTOR p1, p2;
        int32_t trgNum = -1;
        while (true)
        {
            // define the triangle we are on
            src.x = dst.x = vRes.x;
            src.z = dst.z = vRes.z;
            float htmp;
            htmp = 500.f - 1000.f * MultiTrace(src, dst, camera_pos.y);
            if (htmp >= -500.f && vRes.y - htmp < MEN_STEP_UP && htmp - vRes.y < MEN_STEP_UP)
            {
                if (!GetCrossXZ(vRes, vShift, (p1 = g_gv0), (p2 = g_gv1), vRes))
                    if (!GetCrossXZ(vRes, vShift, (p1 = g_gv1), (p2 = g_gv2), vRes))
                        if (!GetCrossXZ(vRes, vShift, (p1 = g_gv2), (p2 = g_gv0), vRes))
                            // no intersection with the sides of the triangle
                            break;

                len = sqrtf(~(vRes - prev_res));
                if (len > speed)
                {
                    vRes = prev_res + vShift * speed;
                    // determine camera height
                    src.x = dst.x = vRes.x;
                    src.z = dst.z = vRes.z;
                    auto tmp = MultiTrace(src, dst, camera_pos.y); // pathNode->geo->Trace(src,dst);
                    if (tmp <= 1.f)
                    {
                        vRes.y = 500.f - 1000.f * tmp;
                        bNoFinded = false;
                    }
                    else
                        vRes = prev_res;
                    bNoFinded = false;
                    break;
                }
                if (len > 0.f)
                {
                    prev_res = vRes - vShift * .0001f;
                    vRes += vShift * MEN_STEP_MIN;
                    bNoFinded = false;
                    speed -= len;
                }
                else
                {
                    vRes += vShift * MEN_STEP_MIN;
                    bNoFinded = false;
                    break;
                }
            }
            else
            {
                // sliding on a triangle
                if (bNoFinded)
                    break; // standing where there is no floor

                CVECTOR ep;
                CVECTOR dp;
                if (((p2 - p1) | vShift) < 0.f)
                {
                    ep = p1;
                    dp = p1 - p2;
                }
                else
                {
                    ep = p2;
                    dp = p2 - p1;
                }

                len = sqrtf(SQR(ep.x - prev_res.x) + SQR(ep.z - prev_res.z));
                if (len > 0.f)
                    if (speed >= len)
                    {
                        vRes = ep + (!dp) * MEN_STEP_MIN;
                        src.x = dst.x = vRes.x;
                        src.z = dst.z = vRes.z;
                        speed -= len;
                        if (MultiTrace(src, dst, camera_pos.y) <= 1.f)
                        {
                            prev_res = vRes;
                        }
                        else
                        {
                            vRes -= vShift * MEN_STEP_MIN;
                            src.x = dst.x = vRes.x;
                            src.z = dst.z = vRes.z;
                            if (MultiTrace(src, dst, camera_pos.y) > 1.f)
                                break;
                        }
                    }
                    else
                    {
                        vRes = prev_res + (ep - prev_res) * (speed / len);
                        break;
                    }
                else
                    break;
            }
        }

        if (!bNoFinded)
        {
            // check if the camera is in a suitable place
            src.x = dst.x = vRes.x;
            src.z = dst.z = vRes.z;
            auto fTmp = MultiTrace(src, dst, camera_pos.y);
            // float fDist = 1000.f;
            // if(fTmp<=1.f) fDist = src.y + fTmp*(dst.y-src.y) - camera_pos.y;
            if (fTmp <= 1.f)
                camera_pos = vRes;
            else
            {
                src.x = dst.x = prev_res.x;
                src.z = dst.z = prev_res.z;
                if (MultiTrace(src, dst, camera_pos.y) <= 1.f)
                    camera_pos = prev_res;
            }
        }
    }

    const CVECTOR pathResolved = camera_pos;
    if (walkMode && !CanWalkTo(prev_pos, camera_pos))
    {
        const CVECTOR proposed = camera_pos;
        camera_pos = prev_pos;
        // Try the tangential components against both floor support and solid geometry.
        for (int axis = 0; axis < 2; ++axis)
        {
            CVECTOR slide = camera_pos;
            if (axis == 0) slide.x = proposed.x;
            else slide.z = proposed.z;
            const CVECTOR up(slide.x, 500.f, slide.z);
            const CVECTOR down(slide.x, -500.f, slide.z);
            const float floor = MultiTrace(up, down, camera_pos.y);
            if (floor <= 1.f)
            {
                slide.y = 500.f - 1000.f * floor;
                if (fabsf(slide.y - camera_pos.y) < MEN_STEP_UP && CanWalkTo(camera_pos, slide))
                    camera_pos = slide;
            }
        }
    }
    static const bool traceDeckWalk = std::getenv("STORM_TRACE_DECK_WALK") != nullptr;
    static uint32_t traceDeckWalkElapsed = 0;
    traceDeckWalkElapsed += DeltaTime;
    if (traceDeckWalk && walkMode && traceDeckWalkElapsed >= 1000)
    {
        traceDeckWalkElapsed %= 1000;
        CVECTOR walkerSupport;
        const bool hasWalkerSupport = ResolveWalkerSupport(camera_pos, walkerSupport);
        const bool pathMoved = ~(pathResolved - prev_pos) > 0.000001f;
        const bool solidChanged = ~(camera_pos - pathResolved) > 0.000001f;
        const char *reason = telescope ? "telescope" :
            (deckForward == 0.f && deckRight == 0.f) ? "no_input" :
            !pathMoved ? "path_rejected" : solidChanged ? "solid_rejected_or_slid" : "accepted";
        core.Trace("Deck walk trace: input=(%.0f,%.0f) from=(%.3f,%.3f,%.3f) desired=(%.3f,%.3f,%.3f) resolved=(%.3f,%.3f,%.3f) supportY=%.3f reason=%s",
                   deckForward, deckRight, prev_pos.x, prev_pos.y, prev_pos.z,
                   pathResolved.x, pathResolved.y, pathResolved.z,
                   camera_pos.x, camera_pos.y, camera_pos.z,
                   hasWalkerSupport ? walkerSupport.y : camera_pos.y, reason);
    }

    // calculate camera position
    CVECTOR s_pos;
    // Only the first-person firing view clears rails by half a meter.
    // Do not lift the third-person orbit or telescope; h_eye stays saved.
    const float firingEyeLift = walkMode && !telescope &&
                                AttributesPointer->GetAttributeAsDword("ThirdPerson") == 0 ? 0.5f : 0.f;
    // Firing eye never drops below the design height: a low ship locator or a
    // saved crouch must not bury the manual-aim view under the rails.
    const float firingEye = firingEyeLift > 0.f ? std::max(h_eye, DEFAULT_HEIGHT) + firingEyeLift : h_eye;
    s_pos = pathNode->glob_mtx * (camera_pos + CVECTOR(0.f, firingEye, 0.f));

    if (walkMode)
    {
        const bool moved = ~(camera_pos - prev_pos) > 0.000001f;
        UpdateWalker(moved, running);
        if (thirdPerson && walkerId)
        {
            const CVECTOR direction(sinf(s_ang.y) * cosf(s_ang.x), -sinf(s_ang.x),
                                    cosf(s_ang.y) * cosf(s_ang.x));
            const CVECTOR desired = s_pos - direction * 2.7f;
            float fraction = 1.f;
            // Give the camera a small volume; retract immediately at hull/masts.
            const CVECTOR offsets[] = {CVECTOR(0.f, 0.f, 0.f), CVECTOR(.15f, 0.f, 0.f),
                CVECTOR(-.15f, 0.f, 0.f), CVECTOR(0.f, .15f, 0.f), CVECTOR(0.f, -.15f, 0.f)};
            for (const auto &offset : offsets)
            {
                const float hit = pModel->Trace(s_pos + offset, desired + offset);
                if (hit >= 0.f && hit <= 1.f)
                    fraction = std::min(fraction, std::max(0.f, hit - .06f));
            }
            s_pos += (desired - s_pos) * fraction;
            walkerVisible = fraction * 2.7f > .65f;
        }
    }

    // set camera
    RenderService->SetCamera(s_pos, s_ang, GetPerspective(), aimZoom.magnification());
    CVECTOR renderedPosition, renderedAngles;
    RenderService->GetCamera(renderedPosition, renderedAngles, zoomRenderedPerspective);
}

void DECK_CAMERA::SetCharacter(ATTRIBUTES *_pACharacter)
{
    // Also called when reselecting this camera for the same character.
    aimZoom.reset();
    if (pACharacter != _pACharacter)
    {
        ReleaseWalker();
        walkerLoadFailed = false;
    }
    pACharacter = _pACharacter;
}

bool DECK_CAMERA::FindPath()
{
    pModel = GetModelPointer();
    Assert(pModel); // if the object is not null

    auto *const pNewPathNode = pModel->FindNode("path");
    Assert(pNewPathNode);
    if (pathNode != pNewPathNode)
    {
        pathNode = pNewPathNode;
        if (!bLoad)
            SetStartPos();
    }
    bLoad = false;

    return true;
}

uint64_t DECK_CAMERA::ProcessMessage(MESSAGE &message)
{
    switch (message.Long())
    {
    case MSG_DECK_CAMERA_SET_VIEWPOINT: {
        CVECTOR cv;
        cv.x = message.Float();
        cv.y = message.Float();
        cv.z = message.Float();
        SetViewPoint(cv);
    }
    break;
    }

    return 0;
}

void DECK_CAMERA::SetStartPos()
{
    if (pModel)
    {
        GEOS::VERTEX src, dst;

        src.y = 500.f;
        dst.y = -500.f;

        camera_ang.x = 0.f;
        camera_ang.y = 0.f;
        camera_ang.z = 0.f;

        camera_pos.x = 0.f;
        camera_pos.z = 0.f;
        camera_pos.y = -500.f;

        for (auto i = 0; i < 1000; i++)
        {
            auto *root = pModel->GetNode(i);
            if (root == nullptr)
                break;
            GEOS::INFO gi;
            root->geo->GetInfo(gi);
            int j;
            for (j = 0; j < gi.nlabels; j++)
            {
                GEOS::LABEL gl;
                root->geo->GetLabel(j, gl);
                if (!strncmp(gl.name, "camera", 6))
                {
                    camera_pos.x = gl.m[3][0];
                    camera_pos.y = gl.m[3][1];
                    camera_pos.z = gl.m[3][2];
                    break;
                }
            }
            if (j < gi.nlabels)
            {
                pathNode->glob_mtx.MulToInv(root->glob_mtx * camera_pos, camera_pos);
                break;
            }
        }
        src.x = dst.x = camera_pos.x;
        src.z = dst.z = camera_pos.z;

        auto tmp = pathNode->geo->Trace(src, dst);
        if (tmp <= 1.f)
        {
            tmp = 500.f - tmp * 1000.f;
            if (camera_pos.y == -500.f)
                h_eye = DEFAULT_HEIGHT;
            else
                h_eye = camera_pos.y - tmp;
            camera_pos.y = tmp;
            if (h_eye > HEIGHT_MAX)
                h_eye = HEIGHT_MAX;
            else if (h_eye < HEIGHT_MIN)
                h_eye = HEIGHT_MIN;
        }
        else
        {
            core.Trace("WARNING! Bad camera locator on ship %s", pModel->GetNode(0)->GetName());
        }

        // SetCursorPos(lock_x,lock_y);
    }
}

bool DECK_CAMERA::GetCrossXZ(CVECTOR &spos, CVECTOR &dv, CVECTOR &p1, CVECTOR &p2, CVECTOR &res)
{
    auto bNoCross = false;

    auto bXset = false;
    auto bZset = false;

    auto xRes = 0.f;
    auto zRes = 0.f;

    // check for dx = 0 for the direction vector
    if (!bNoCross)
        if (EQU_FLOAT(dv.x, 0.f))
            if (bXset)
            {
                if (spos.x != xRes)
                    bNoCross = true;
            }
            else
            {
                bXset = true;
                xRes = spos.x;
            }
    // check for dz = 0 for the direction vector
    if (!bNoCross)
        if (EQU_FLOAT(dv.z, 0.f))
            if (bZset)
            {
                if (spos.z != zRes)
                    bNoCross = true;
            } //~!~
            else
            {
                bZset = true;
                zRes = spos.z;
            }

    // check for dx = 0 for a segment
    if (!bNoCross) // ~!~
        if (EQU_FLOAT(p1.x, p2.x))
            if (bXset)
            {
                if (p1.x != xRes)
                    bNoCross = true;
            }
            else
            {
                bXset = true;
                xRes = p1.x;
            }
    // check for dz = 0 for a segment
    if (!bNoCross)
        if (EQU_FLOAT(p1.z, p2.z))
            if (bZset)
            {
                if (p1.z != zRes)
                    bNoCross = true;
            }
            else
            {
                bZset = true;
                zRes = p1.z;
            }

    if (bNoCross)
        return false; // no intersection
    if (bXset && bZset)
        if ((spos.x - p1.x) * (p2.z - p1.z) != (p2.x - p1.x) * (spos.z - p1.z))
            return false; // no intersection

    if (bXset)
        if (!bZset)
            if (dv.x != 0.f)
            {
                bZset = true;
                zRes = (xRes - spos.x) * dv.z / dv.x + spos.z;
            }
            else if (p1.x != p2.x)
            {
                bZset = true;
                zRes = (xRes - p1.x) * (p2.z - p1.z) / (p2.x - p1.x) + p1.z;
            }
            else
            {
                bZset = true;
                zRes = dv.z < 0.f ? p1.z < p2.z ? p1.z : p2.z : p1.z < p2.z ? p2.z : p1.z;
            }

    if (bZset)
        if (!bXset)
            if (dv.z != 0.f)
            {
                bXset = true;
                xRes = (zRes - spos.z) * dv.x / dv.z + spos.x;
            }
            else if (p1.z != p2.z)
            {
                bXset = true;
                xRes = (zRes - p1.z) * (p2.x - p1.x) / (p2.z - p1.z) + p1.x;
            }
            else
            {
                bXset = true;
                xRes = dv.x < 0.f ? p1.x < p2.x ? p1.x : p2.x : p1.x < p2.x ? p2.x : p1.x;
            }

    if (!bZset && !bXset)
    {
        xRes = (p1.z - spos.z + spos.x * dv.z / dv.x - p1.x * (p2.z - p1.z) / (p2.x - p1.x)) /
               (dv.z / dv.x - (p2.z - p1.z) / (p2.x - p1.x));
        zRes = spos.z + (xRes - spos.x) * dv.z / dv.x;
    }

    // check for finding the intersection point on a given segment
    // on X
    if (p1.x < p2.x)
    {
        if (xRes < p1.x || xRes > p2.x)
            return false;
    } // no intersection on the segment
    else if (xRes < p2.x || xRes > p1.x)
        return false;
    // on Z
    if (p1.z < p2.z)
    {
        if (zRes < p1.z || zRes > p2.z)
            return false;
    }
    else if (zRes < p2.z || zRes > p1.z)
        return false;

    // calculate the Y coordinate from the segment [P1, P2]
    float yRes;
    if (p1.x == p2.x)
        if (p1.z == p2.z)
            yRes = p1.y > p2.y ? p1.y : p2.y;
        else
            yRes = p1.y + (zRes - p1.z) * (p2.y - p1.y) / (p2.z - p1.z);
    else
        yRes = p1.y + (xRes - p1.x) * (p2.y - p1.y) / (p2.x - p1.x);

    if ((xRes > spos.x && dv.x < 0.f) || (xRes < spos.x && dv.x > 0.f) || (zRes > spos.z && dv.z < 0.f) ||
        (zRes < spos.z && dv.z > 0.f))
        return false;
    res.x = xRes;
    res.y = yRes;
    res.z = zRes;
    return true;
}

uint32_t DECK_CAMERA::AttributeChanged(ATTRIBUTES *pAttr)
{
    if (*pAttr == "SensivityDistance")
        fSensivityDistance = pAttr->GetAttributeAsFloat();
    if (*pAttr == "SensivityHeightAngle")
        fSensivityHeightAngle = pAttr->GetAttributeAsFloat();
    if (*pAttr == "SensivityAzimuthAngle")
        fSensivityAzimuthAngle = pAttr->GetAttributeAsFloat();
    if (*pAttr == "StepUp")
        MEN_STEP_UP = pAttr->GetAttributeAsFloat();
    if (*pAttr == "StepMin")
        MEN_STEP_MIN = pAttr->GetAttributeAsFloat();
    if (*pAttr == "h_max")
        HEIGHT_MAX = pAttr->GetAttributeAsFloat();
    if (*pAttr == "h_min")
        HEIGHT_MIN = pAttr->GetAttributeAsFloat();
    if (*pAttr == "h_step")
        HEIGHT_STEP = pAttr->GetAttributeAsFloat();
    if (*pAttr == "h_def")
        DEFAULT_HEIGHT = pAttr->GetAttributeAsFloat();
    if (*pAttr == "MaxAngleX")
        CAMERA_MAX_X = pAttr->GetAttributeAsFloat();
    if (*pAttr == "MinAngleX")
        CAMERA_MIN_X = pAttr->GetAttributeAsFloat();
    if (*pAttr == "RockingX")
        fRockingX = pAttr->GetAttributeAsFloat();
    if (*pAttr == "RockingZ")
        fRockingZ = pAttr->GetAttributeAsFloat();

    return 0;
}

void DECK_CAMERA::SetViewPoint(CVECTOR &cViewPoint)
{
    if (pathNode == nullptr)
        return;

    /*calculate camera position
    CVECTOR s_pos;
    s_pos = pathNode->glob_mtx*(camera_pos+CVECTOR(0.f,h_eye,0.f));

    CMatrix glbRotMtx;
    glbRotMtx.BuildViewMatrix(s_pos,cViewPoint,CVECTOR(0.f,1.f,0.f));
    glbRotMtx.SetPosition(0.f,0.f,0.f);

    CMatrix invRotMtx;
    pathNode->glob_mtx.Get3X3(invRotMtx);
    invRotMtx.Transposition3X3();

    glbRotMtx *= invRotMtx;

    CVECTOR v = glbRotMtx*CVECTOR(0,0,1.f);
    float yAng = atan2f(v.x,v.z);
    glbRotMtx.RotateY(-yAng);

    v = glbRotMtx*CVECTOR(0,0,1.f);
    float xAng = atan2f(-v.y,v.z);*/

    // Viewpoint in local coordinates
    CVECTOR e_pos;
    pathNode->glob_mtx.MulToInv(cViewPoint, e_pos);

    e_pos -= camera_pos;

    camera_ang.x = -atan2f(e_pos.y, sqrtf(e_pos.x * e_pos.x + e_pos.z * e_pos.z));
    camera_ang.y = atan2f(e_pos.x, e_pos.z);
}

float DECK_CAMERA::MultiTrace(const CVECTOR &cvUp, const CVECTOR &cvDown, float fHBase)
{
    if (pathNode == nullptr)
        return 2.f;

    auto fRet = 2.f;

    float fTmp;
    GEOS::VERTEX curUp, curDown;
    curUp.x = cvUp.x;
    curUp.y = cvUp.y;
    curUp.z = cvUp.z;
    curDown.x = cvDown.x;
    curDown.y = cvDown.y;
    curDown.z = cvDown.z;

    auto fDist = MEN_STEP_UP;

    while ((fTmp = pathNode->geo->Trace(curUp, curDown)) <= 1.f)
    {
        fTmp += 0.00001f;
        curUp.y += fTmp * (cvDown.y - curUp.y);
        if ((curUp.y - fHBase <= fDist) && (fHBase - curUp.y <= fDist))
        {
            if (curUp.y > fHBase)
                fDist = curUp.y - fHBase;
            else
                fDist = fHBase - curUp.y;
            fRet = (curUp.y - cvUp.y) / (cvDown.y - cvUp.y);

            GEOS::TRACE_INFO ti;
            pathNode->geo->GetCollisionDetails(ti);
            memcpy(&g_gv0, &ti.vrt[0], sizeof(g_gv0));
            memcpy(&g_gv1, &ti.vrt[1], sizeof(g_gv1));
            memcpy(&g_gv2, &ti.vrt[2], sizeof(g_gv2));
        }
    }

    return fRet;
}

void DECK_CAMERA::Save(CSaveLoad *pSL)
{
    pSL->SaveBuffer((const char *)&tri, sizeof(tri));
    pSL->SaveFloat(fSensivityDistance);
    pSL->SaveFloat(fSensivityHeightAngle);
    pSL->SaveFloat(fSensivityAzimuthAngle);
    pSL->SaveFloat(fRockingX);
    pSL->SaveFloat(fRockingZ);

    pSL->SaveFloat(MEN_STEP_UP);
    pSL->SaveFloat(MEN_STEP_MIN);
    pSL->SaveFloat(HEIGHT_MAX);
    pSL->SaveFloat(HEIGHT_MIN);
    pSL->SaveFloat(HEIGHT_STEP);
    pSL->SaveFloat(CAMERA_MAX_X);
    pSL->SaveFloat(CAMERA_MIN_X);
    pSL->SaveFloat(DEFAULT_HEIGHT);
    pSL->SaveVector(g_gv0);
    pSL->SaveVector(g_gv1);
    pSL->SaveVector(g_gv2);

    pSL->SaveVector(camera_pos);
    pSL->SaveVector(camera_ang);
    pSL->SaveFloat(h_eye);
    pSL->SaveBuffer((const char *)&Screen_Rect, sizeof(Screen_Rect));
    pSL->SaveLong(lock_x);
    pSL->SaveLong(lock_y);

    pSL->SaveDword(isOn());
    pSL->SaveDword(isActive());
    pSL->SaveFloat(GetPerspective());
    pSL->SaveAPointer("character", pACharacter);
}

void DECK_CAMERA::Load(CSaveLoad *pSL)
{
    pSL->Load2Buffer(&tri);
    fSensivityDistance = pSL->LoadFloat();
    fSensivityHeightAngle = pSL->LoadFloat();
    fSensivityAzimuthAngle = pSL->LoadFloat();
    fRockingX = pSL->LoadFloat();
    fRockingZ = pSL->LoadFloat();

    MEN_STEP_UP = pSL->LoadFloat();
    MEN_STEP_MIN = pSL->LoadFloat();
    HEIGHT_MAX = pSL->LoadFloat();
    HEIGHT_MIN = pSL->LoadFloat();
    HEIGHT_STEP = pSL->LoadFloat();
    CAMERA_MAX_X = pSL->LoadFloat();
    CAMERA_MIN_X = pSL->LoadFloat();
    DEFAULT_HEIGHT = pSL->LoadFloat();
    g_gv0 = pSL->LoadVector();
    g_gv1 = pSL->LoadVector();
    g_gv2 = pSL->LoadVector();

    camera_pos = pSL->LoadVector();
    camera_ang = pSL->LoadVector();
    h_eye = pSL->LoadFloat();
    pSL->Load2Buffer(&Screen_Rect);
    lock_x = pSL->LoadLong();
    lock_y = pSL->LoadLong();

    SetOn(pSL->LoadDword() != 0);
    SetActive(pSL->LoadDword() != 0);
    SetPerspective(pSL->LoadFloat());
    SetCharacter(pSL->LoadAPointer("character"));

    if (isOn())
        bLoad = true;
}
