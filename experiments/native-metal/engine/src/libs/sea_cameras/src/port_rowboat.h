#pragma once

#include "port_contact.h"
#include "entity.h"
#include <array>
#include <string>
#include <vector>

class DECK_CAMERA;
class VDX9RENDER;

// Script owns the ordinary character, quest/location lifecycle and SAVE tree.
// This entity owns transient presentation only while PortTravel.enabled is set.
class PORT_ROWBOAT final : public Entity
{
  public:
    enum Operation { Configure, Begin, RequestTransfer, Cancel, ReleaseHero, Restore };
    enum State { Inactive, Entering, Rowing, Leaving, Handoff, Blocked };
    bool Init() override;
    ~PORT_ROWBOAT() override;
    void ProcessStage(Stage stage, uint32_t delta) override;
    uint64_t ProcessMessage(MESSAGE &message) override;

  private:
    ATTRIBUTES *hero = nullptr, *travel = nullptr, *rig = nullptr;
    VDX9RENDER *renderer = nullptr;
    entid_t hull = 0, actor = 0, bench = 0;
    std::array<entid_t, 2> oars{};
    entid_t deckCamera = 0;
    PortDeckContact contact;
    CMatrix boat;
    CVECTOR velocity = CVECTOR(0.f), actorPosition = CVECTOR(0.f);
    CVECTOR seat = CVECTOR(0.f), mooredBoat = CVECTOR(0.f);
    float yaw = 0.f, actorYaw = 0.f, speed = 0.f, phase = 0.f, turn = 0.f;
    float progress = 0.f, transferLength = 0.f, transferYaw = 0.f;
    float blendRemaining = 0.f;
    float mooredYaw = 0.f;
    int strokeDirection = 1;
    State state = Inactive;
    State interrupted = Inactive;
    bool returning = false, ladderRoute = false, routeEntering = false;
    std::array<CVECTOR, 5> route{}; // receiver-local when deckCamera !=0
    std::array<float, 4> lengths{};
    std::vector<entid_t> ladder;
    std::string action;

    bool ConfigureFrom(ATTRIBUTES *character, ATTRIBUTES *descriptor);
    bool CreateModels();
    bool StartTransfer(entid_t camera, bool entering, bool restore = false);
    bool PrepareTransfer(entid_t camera, bool entering, bool restore);
    bool ResolvePier(const CVECTOR &candidate, CVECTOR &support) const;
    bool MakeRoute(bool entering);
    bool CreateLadder();
    bool RefreshRoute();
    CVECTOR RoutePoint(float distance, unsigned *segment = nullptr) const;
    bool WorldClear(const CVECTOR &from, const CVECTOR &to, float radius = .26f,
                    float height = 1.1f, float step = .12f) const;
    bool BoatClear(const CMatrix &from, const CMatrix &to) const;
    bool SetAction(const char *name, float position);
    bool Error(const char *reason);
    void Update(uint32_t delta);
    void Row(float dt);
    void Transfer(float dt);
    void FloatBoat();
    void Present(uint32_t delta);
    void Publish();
    void ReleaseModels(bool all);
};
