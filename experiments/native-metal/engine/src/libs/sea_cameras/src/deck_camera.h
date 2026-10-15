// ==================================================================== ||
// Ship tie camera                                                      ||
// ==================================================================== ||
#pragma once

#include "common_camera.h"
#include "deck_aim_zoom.h"
#include <chrono>
#include "dx9render.h"
#include "model.h"
#include "vma.hpp"

#define DECK_VERTEX_FORMAT (storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1)
#define AIM_SIZE 0.05f

namespace storm { class OSWindow; }

class DECK_CAMERA : public COMMON_CAMERA
{
    struct VERTEX
    {
        storm_render::Vector3 v;
        storm_render::PackedColor color;
        float tu;
        float tv;
    };

    VERTEX tri[4];

    // parameters loaded from INI file //
    // --------------------------------------
    float fSensivityDistance, fSensivityHeightAngle, fSensivityAzimuthAngle;
    float fRockingX, fRockingZ;

    float MEN_STEP_UP;
    float MEN_STEP_MIN;
    float HEIGHT_MAX;
    float HEIGHT_MIN;
    float HEIGHT_STEP;
    float CAMERA_MAX_X, CAMERA_MIN_X;
    float DEFAULT_HEIGHT;
    //--------------------------------------

    CVECTOR g_gv0, g_gv1, g_gv2;

    MODEL *pModel;
    VDX9RENDER *RenderService;
    CVECTOR camera_pos, camera_ang;
    float h_eye;
    storm_render::Rect Screen_Rect;
    int32_t lock_x;
    int32_t lock_y;
    int32_t vb_id;
    bool bLoad;
    NODE *pathNode;
    // Transient presentation only: keep the existing binary save layout intact.
    entid_t walkerId = 0;
    bool walkerVisible = false;
    bool walkerLoadFailed = false;
    float walkerYaw = 0.f;
    std::string walkerAction;
    deck_aim::Zoom aimZoom;
    std::chrono::steady_clock::time_point zoomLastUpdate{};
    uint64_t zoomPauseRevision = 0;
    float zoomRenderedPerspective = 0.f;
    storm::OSWindow *zoomWindow = nullptr;
    int zoomWindowSubscription = -1;
    bool zoomWindowFocused = true;
    void UpdateAimZoom();
    void ResetAimZoomProjection();
    void ReleaseWalker();
    bool EnsureWalker();
    void UpdateWalker(bool moving, bool running);
    bool ResolveWalkerSupport(const CVECTOR &position, CVECTOR &support) const;
    bool CanWalkTo(const CVECTOR &from, const CVECTOR &to);
    void SetStartPos();
    bool GetCrossXZ(CVECTOR &spos, CVECTOR &dv, CVECTOR &p1, CVECTOR &p2, CVECTOR &res);
    bool FindPath();
    void SetViewPoint(CVECTOR &cViewPoint);
    float MultiTrace(const CVECTOR &cvBase, const CVECTOR &cvDown, float fHBase);

  public:
    DECK_CAMERA();
    ~DECK_CAMERA() override;

    void SetCharacter(ATTRIBUTES *_pACharacter) override;

    void SetDevice();
    bool Init() override;
    void Move(uint32_t DeltaTime);
    void Realize(uint32_t Delta_Time);
    void Execute(uint32_t Delta_Time);
    bool CreateState(ENTITY_STATE_GEN *state_gen);
    bool LoadState(ENTITY_STATE *state);
    uint32_t AttributeChanged(ATTRIBUTES *pAttr) override;
    uint64_t ProcessMessage(MESSAGE &message) override;

    void ProcessStage(Stage stage, uint32_t delta) override
    {
        switch (stage)
        {
        case Stage::execute:
            Execute(delta);
            break;
        case Stage::realize:
            Realize(delta);
            break;
            /*case Stage::lost_render:
              LostRender(delta); break;
            case Stage::restore_render:
              RestoreRender(delta); break;*/
        }
    }

    void Save(CSaveLoad *pSL) override;
    void Load(CSaveLoad *pSL) override;
};
