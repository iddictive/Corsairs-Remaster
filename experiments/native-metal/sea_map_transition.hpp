#pragma once

#include "core.h"
#include "dx9render.h"
#include "entity.h"
#include "shared/layers.h"
#include "world_map_player_model.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>

// One owner for the outgoing camera and GPU-only handoff. The map loader owns
// readiness; elapsed time never exposes an incomplete destination.
class SeaMapTransition final : public Entity
{
    using Clock = std::chrono::steady_clock;
    VDX9RENDER *rs = nullptr;
    IDirect3DTexture9 *frame = nullptr;
    Clock::time_point started;
    float flightTime = 0, landingTime = 0;
    uint64_t pauseRevision = 0;
    CVECTOR origin{}, angles{}, focus{}, look{}, offset{};
    float perspective = 1.285f;
    float targetArea = 0, targetDistance = 0, shipYaw = 0, baseShipYaw = 0, mapYaw = 0, mapPitch = 1.0f;
    float mapWorldYaw = 0, mapDistance = 0;
    static constexpr float flightDuration = 1.05f, tiltDuration = .30f, revealDuration = .18f;
    CVECTOR miniatureSize{}, miniatureCenter{};
    bool mapLocked = false;
    bool captured = false, committed = false, ready = false;
    bool reverse = false;
    inline static SeaMapTransition *active = nullptr;

    static float ease(float t)
    {
        t = std::clamp(t, 0.0f, 1.0f);
        return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
    }
    float seconds() const
    {
        return std::chrono::duration<float>(Clock::now() - started).count();
    }
    bool matchShip()
    {
        CVECTOR size, position;
        if (!storm_map_player::seaBounds(size, shipYaw, position)) return false;
        if (mapLocked) mapYaw = mapWorldYaw - shipYaw;
        targetArea = storm_map_player::area(miniatureSize, miniatureCenter, mapYaw, mapPitch,
                                            storm_map_player::perspective, mapDistance);
        const CVECTOR center(0, size.y * .5f, 0);
        targetDistance = storm_map_player::distance(size, center, mapYaw, mapPitch, storm_map_player::perspective, targetArea);
        return std::isfinite(targetDistance) && targetDistance > 0;
    }
    void camera(float time)
    {
        // Follow live translation throughout ascent/descent, including Phonk
        // and moving waves. The old fade used to tear down the sea immediately.
        CVECTOR size, shipPosition;
        float yawNow;
        if (storm_map_player::seaBounds(size, yawNow, shipPosition))
        {
            look += shipPosition - focus;
            focus = shipPosition;
            shipYaw = yawNow;
            if (mapLocked) matchShip();
        }
        // Establish the overhead view first; then change only vertical range.
        // Quintic easing and logarithmic distance keep both ends continuous.
        const float step = ease((time - tiltDuration) / (flightDuration - tiltDuration));
        const float distance = std::max(1.0f, std::sqrt(~offset));
        const float range = distance * std::pow(targetDistance / distance, step);
        const float tilt = ease(time / tiltDuration);
        const float fromYaw = std::atan2(-offset.x, -offset.z) + std::remainder(shipYaw - baseShipYaw, 6.283185307f);
        const float deltaYaw = std::remainder(shipYaw + mapYaw - fromYaw, 6.283185307f);
        const float yaw = fromYaw + deltaYaw * tilt;
        const float pitch = std::asin(std::clamp(offset.y / distance, -1.0f, 1.0f)) * (1 - tilt) + mapPitch * tilt;
        const CVECTOR position = focus + CVECTOR(-std::sin(yaw) * std::cos(pitch), std::sin(pitch),
                                                 -std::cos(yaw) * std::cos(pitch)) * range;
        const CVECTOR target = look + (focus - look) * tilt;
        rs->SetCamera(position, target, CVECTOR(0.0f, 1.0f, 0.0f));
        rs->SetPerspective(perspective + (storm_map_player::perspective - perspective) * step);
    }
    bool capture()
    {
        IDirect3DSurface9 *source = nullptr, *destination = nullptr;
        D3DSURFACE_DESC desc{};
        bool ok = SUCCEEDED(rs->GetRenderTarget(&source)) && source &&
                  SUCCEEDED(source->GetDesc(&desc)) &&
                  SUCCEEDED(rs->CreateTexture(desc.Width, desc.Height, 1,
                      D3DUSAGE_RENDERTARGET, desc.Format, D3DPOOL_DEFAULT, &frame)) &&
                  SUCCEEDED(frame->GetSurfaceLevel(0, &destination)) &&
                  SUCCEEDED(rs->StretchRect(source, nullptr, destination, nullptr, D3DTEXF_NONE));
        if (destination) destination->Release();
        if (source) source->Release();
        if (!ok && frame) { frame->Release(); frame = nullptr; }
        return ok;
    }
    void quad(uint32_t color, float u0 = 0, float v0 = 0, float u1 = 1, float v1 = 1,
              const char *technique = "Fader")
    {
        D3DVIEWPORT9 viewport{};
        rs->GetViewport(&viewport);
        struct Vertex { float x, y, z, rhw; uint32_t color; float u, v; };
        const float w = static_cast<float>(viewport.Width), h = static_cast<float>(viewport.Height);
        const Vertex vertices[] = {
            {-.5f, -.5f, .5f, 1, color, u0, v0}, {w, -.5f, .5f, 1, color, u1, v0},
            {-.5f, h, .5f, 1, color, u0, v1}, {-.5f, h, .5f, 1, color, u0, v1},
            {w, -.5f, .5f, 1, color, u1, v0}, {w, h, .5f, 1, color, u1, v1}};
        rs->DrawPrimitiveUP(D3DPT_TRIANGLELIST, D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1,
                            2, vertices, sizeof(Vertex), technique);
    }
    void realize()
    {
        const float time = flightTime;
        if (!captured && time >= (reverse ? tiltDuration : flightDuration))
        {
            // Capture the rendered apex, before drawing the cloud veil or
            // deleting sea entities. No readback, synchronization or screenshot.
            if (!capture()) core.Trace("SeaMapTransition: GPU frame capture failed");
            captured = true;
        }
        if (reverse && committed && !ready && AttributesPointer->GetAttributeAsDword("scenePrepared"))
        {
            // Called after SEA_REALIZE: this is the first complete destination
            // frame, not merely SeaLogin's completion or a timer.
            rs->GetCamera(origin, angles, perspective);
            focus = CVECTOR(AttributesPointer->GetAttributeAsFloat("x"),
                            AttributesPointer->GetAttributeAsFloat("y"),
                            AttributesPointer->GetAttributeAsFloat("z"));
            offset = origin - focus;
            if (!matchShip()) core.Trace("SeaMapTransition: destination ship bounds unavailable");
            baseShipYaw = shipYaw;
            const float distance = std::max(20.0f, std::sqrt(~offset));
            look = origin + CVECTOR(std::sin(angles.y) * std::cos(angles.x), -std::sin(angles.x),
                                    std::cos(angles.y) * std::cos(angles.x)) * distance;
            ready = true;
            landingTime = 0;
        }
        drawOverlay();
    }
    void drawOverlay()
    {
        const float time = flightTime;
        const float landing = ready ? landingTime : 0;
        const float reveal = ready ? ease(landing / revealDuration) : 0;
        if (committed && frame)
        {
            rs->SetTexture(0, frame);
            // Continue the outgoing scale impulse while preparation advances.
            quad((static_cast<uint32_t>((1 - reveal) * 255) << 24) | 0x00ffffff,
                 0, 0, 1, 1);
        }
        if (ready && (reverse ? landing >= flightDuration : reveal >= 1))
        {
            core.PostEvent("Sea_MapTransitionDone", 0, nullptr);
            core.EraseEntity(GetId());
        }
    }
public:
    static bool Loading() { return active && active->committed && !active->ready; }
    static void DrawProgress() { if (active) active->drawOverlay(); }
    static bool OverrideMapCamera()
    {
        if (!active || !active->reverse || active->committed) return false;
        // Tilt the map to the same overhead axis before starting descent.
        CVECTOR view = active->angles;
        view.x += (storm_map_player::overheadPitch - view.x) * ease(active->flightTime / tiltDuration);
        const float range = active->origin.y / std::sin(view.x);
        const CVECTOR cameraPosition = active->focus +
            CVECTOR(-std::sin(view.y) * std::cos(view.x), std::sin(view.x),
                    -std::cos(view.y) * std::cos(view.x)) * range;
        active->rs->SetCamera(cameraPosition, view, active->perspective);
        return true;
    }
    static bool Active() { return active != nullptr; }
    ~SeaMapTransition() override
    {
        if (active == this) active = nullptr;
        if (frame) frame->Release();
    }
    bool Init() override
    {
        rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
        if (!rs || !AttributesPointer || active) return false;
        reverse = AttributesPointer->GetAttributeAsDword("reverse") != 0;
        mapPitch = storm_map_player::overheadPitch;
        rs->GetCamera(origin, angles, perspective);
        mapYaw = AttributesPointer->GetAttributeAsFloat("mapYaw");
        if (!storm_map_player::bounds(miniatureSize, miniatureCenter)) return false;
        const float mapHeight = std::clamp(AttributesPointer->GetAttributeAsFloat("mapHeight", storm_map_player::minHeight),
                                          storm_map_player::minHeight, storm_map_player::maxHeight);
        mapLocked = reverse || AttributesPointer->GetAttributeAsDword("mapRotLock");
        if (reverse)
        {
            shipYaw = AttributesPointer->GetAttributeAsFloat("shipHeading");
            mapWorldYaw = angles.y;
            mapYaw = mapWorldYaw - shipYaw;
        }
        mapDistance = (reverse ? origin.y : mapHeight) / std::sin(mapPitch);
        targetArea = storm_map_player::area(miniatureSize, miniatureCenter, mapYaw, mapPitch,
                                            storm_map_player::perspective, mapDistance);
        if (!(targetArea > 0) || !std::isfinite(targetArea)) return false;
        if (!reverse && !matchShip()) return false;
        baseShipYaw = shipYaw;
        focus = CVECTOR(AttributesPointer->GetAttributeAsFloat("x"),
                        AttributesPointer->GetAttributeAsFloat("y"),
                        AttributesPointer->GetAttributeAsFloat("z"));
        offset = origin - focus;
        const float distance = std::max(20.0f, std::sqrt(~offset));
        look = origin + CVECTOR(std::sin(angles.y) * std::cos(angles.x), -std::sin(angles.x),
                                std::cos(angles.y) * std::cos(angles.x)) * distance;
        started = Clock::now();
        pauseRevision = core.GetScenePauseRevision();
        active = this;
        core.SetLayerType(FADER_EXECUTE, layer_type_t::execute);
        core.SetLayerType(FADER_REALIZE, layer_type_t::realize);
        core.AddToLayer(FADER_EXECUTE, GetId(), 65535);
        core.AddToLayer(FADER_REALIZE, GetId(), 65535);
        return true;
    }
    void ProcessStage(Stage stage, uint32_t) override
    {
        if (stage == Stage::execute)
        {
            const auto revision = core.GetScenePauseRevision();
            if (revision == pauseRevision && core.GetTimeScale() > 0)
            {
                const float delta = core.GetRDeltaTime() * .001f;
                flightTime += delta;
                if (ready) landingTime += delta;
            }
            pauseRevision = revision;
            if (!committed && !reverse) camera(flightTime);
            else if (reverse && ready)
                camera(std::max(0.0f, flightDuration - landingTime));
            if (captured && !committed)
            {
                committed = true;
                core.PostEvent(reverse ? "Wdm_SeaTransitionCommit" : "Sea_MapTransitionCommit", 0, nullptr);
            }
            if (committed && !ready && AttributesPointer->GetAttributeAsDword("mapReady"))
            {
                ready = true;
                landingTime = 0;
            }
        }
        else if (stage == Stage::realize) realize();
    }
};
