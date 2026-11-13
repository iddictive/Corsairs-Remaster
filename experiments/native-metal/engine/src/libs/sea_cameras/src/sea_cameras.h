#pragma once

#include "common_camera.h"
#include "dart_view_borrower.h"
#include "camera_borrower.h"

#include <vector>
#include <chrono>

class VDX9RENDER;

class SEA_CAMERAS : public Entity
{
    friend bool storm_dart_view::ReferencesCharacter(int index);
    friend bool storm_sea_cameras::ReferencesCharacter(int index);
    std::vector<COMMON_CAMERA *> CamerasArray;
    bool bActive;
    VDX9RENDER *render = nullptr;
    bool dartViewPending = false, dartViewActive = false, dartViewApplied = false;
    uint32_t dartSequence = 0, dartHits = 0, dartCancel = 0;
    uint64_t dartPauseRevision = 0;
    entid_t dartSource = 0, dartPlayer = 0;
    CVECTOR returnPosition, returnAngles, dartDirection, dartPosition;
    CVECTOR dartInputAngles;
    float dartInputDistance = 1.f;
    float returnPerspective = 1.f, returnMagnification = 1.f;
    float dartPerspective = 1.f;
    uint32_t missileViewSequence = 0;
    uint32_t missileExpiredSequence = 0;
    std::chrono::steady_clock::time_point missileLastFlight{};
    bool missileManual = false;
    CVECTOR missileInputAngles;
    float missileInputDistance = 1.f;
    COMMON_CAMERA *missileInputCamera = nullptr;
    CVECTOR missileOrbitOffset = CVECTOR(0.f);
    float missileZoom = 1.f;
    std::chrono::steady_clock::time_point dartStarted{}, dartLastHit{};

    void RestoreDartView();
    void CancelDartView();
    void ClearMissileControl();
    void RealizeDartView();
    bool RealizeMissileView();

  public:
    SEA_CAMERAS();
    ~SEA_CAMERAS() override;

    void ProcessMessage(uint32_t iMsg, uint32_t wParam, uint32_t lParam);
    uint64_t ProcessMessage(MESSAGE &message) override;

    bool Init() override;

    void ProcessStage(Stage stage, uint32_t delta) override;
};
