#pragma once

#include "../base_video.h"
#ifdef __APPLE__
#include "native_video.h"
#endif
#ifdef _WIN32 // FIX_LINUX ddraw.h and amstream.h
#include <amstream.h>
#include <ddraw.h>
#endif

#define XI_AVIVIDEO_FVF (storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::tex1 | storm_render::vertexFormat::texCoord2)

struct XI_AVIVIDEO_VERTEX
{
    CVECTOR pos;
    float w;
    float tu, tv;
};

class CAviPlayer : public xiBaseVideo
{
    VDX9RENDER *rs;
    bool m_bShowVideo;

  public:
    CAviPlayer();
    ~CAviPlayer() override;
    bool Init() override;
    void Execute(uint32_t delta_time);
    void Realize(uint32_t delta_time);
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

    void SetShowVideo(bool bShowVideo) override
    {
        m_bShowVideo = bShowVideo;
    }

    storm_render::Texture *GetCurrentVideoTexture() override
    {
        return pTex;
    }

  protected:
    bool m_bContinue;

#ifdef _WIN32 // FIX_LINUX ddraw.h and amstream.h
    IDirectDraw *pDD;
    IDirectDrawSurface *pPrimarySurface;
    IDirectDrawSurface *pVideoSurface;

    IAMMultiMediaStream *pAMStream;
    IMediaStream *pPrimaryVidStream;
    IDirectDrawMediaStream *pDDStream;
    IDirectDrawStreamSample *pSample;
#endif

#ifdef __APPLE__
    NativeVideo *nativeVideo{};
    bool nativeEndEventSent{};
    float nativeAspect{1.f};
#endif

    storm_render::Point dstPnt;
    storm_render::Rect lockRect;

    XI_AVIVIDEO_VERTEX v[4];

    storm_render::Surface *pTmpRenderTarget;
    storm_render::Texture *pTex;

    void ReleaseAll();
    bool PlayMedia(const char *fileName);
    bool GetInterfaces();
    void CleanupInterfaces();

    bool m_bFirstDraw;
    bool m_bMakeUninitializeDD;

    std::string filename;
    bool bLoop{false};
};
