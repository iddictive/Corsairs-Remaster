#pragma once

#ifdef _WIN32 // Effects
#include "effects.h"
#else
#include "technique.h"
#endif
#include "font.h"
#include "video_texture.h"
#include "dx9render.h"
#include "vma.hpp"
#include "platform/platform.hpp"

#include "render_device_api.hpp"
#include "script_libriary.h"

#include <stack>
#include <vector>

#define MAX_STEXTURES 10240
#define MAX_BUFFERS 10240
#define MAX_FONTS 256

struct RenderErrors
{
    storm_render::Result err;
    char *descrition;
};

struct texpaths_t
{
    char str[MAX_PATH];
};

struct STEXTURE
{
    storm_render::BaseTexture *d3dtex;
    char *name;
    uint32_t hash;
    int32_t ref;
    uint32_t dwSize;
    bool isCubeMap;
    bool loaded;
};

//-----------buffers-----------
struct VERTEX_BUFFER
{
    uint32_t dwNumLocks;
    uint32_t dwUsage;
    int32_t type;
    int32_t size;
    storm_render::VertexBuffer *buff;
};

struct INDEX_BUFFER
{
    uint32_t dwNumLocks;
    uint32_t dwUsage;
    int32_t size;
    storm_render::IndexBuffer *buff;
};

struct FONTEntity
{
    char *name;
    uint32_t hash;
    FONT *font;
    int32_t ref;
};

struct VideoTextureEntity
{
    char *name;
    uint32_t hash;
    int32_t ref;
    entid_t videoTexture_id;
    CVideoTexture *VideoTexture;
    VideoTextureEntity *next;
};

// ----- Sound statistics -----
extern uint32_t dwSoundBuffersCount;
extern uint32_t dwSoundBytes;
extern uint32_t dwSoundBytesCached;

class DX9RENDER_SCRIPT_LIBRIARY : public SCRIPT_LIBRIARY
{
  public:
    DX9RENDER_SCRIPT_LIBRIARY(){};

    ~DX9RENDER_SCRIPT_LIBRIARY() override{};
    bool Init() override;
};

//-----------SDEVICE-----------
class DX9RENDER : public VDX9RENDER
{
#define RS_RECT_VERTEX_FORMAT (storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1)

  public:
    static DX9RENDER *pRS;

    DX9RENDER();
    ~DX9RENDER() override;

    // DX9Render: Init/Release
    bool InitDevice(bool windowed, HWND hwnd, int32_t width, int32_t height) override;
    bool ReleaseDevice() override;
    int32_t ApplyMetalGraphics(uint32_t flagsMask, uint32_t shadowQuality, int32_t fullscreen,
                               int32_t renderWidth, int32_t renderHeight, int32_t desktopMode) override;
    int32_t ConsumeMetalGraphicsApplyResult() override;

    // DX9Render: Animation
    void RenderAnimation(int32_t ib, void *src, int32_t numVrts, int32_t minv, int32_t numv, int32_t startidx, int32_t numtrg,
                         bool isUpdateVB) override;

    // DX9Render: Return renderDevice device
    virtual void *GetDevice()
    {
        return renderDevice;
    };

    storm_render::Device *GetNativeDevice() override
    {
        return renderDevice;
    };
    void FinalizeSceneEffects() override;

    // DX9Render: Render Target/Begin/End/Clear
    bool DX9Clear(int32_t type) override; // D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER
    bool DX9BeginScene() override;
    bool DX9EndScene() override;

    // DX9Render: Materials/Lights Section
    bool SetLight(uint32_t dwIndex, const storm_render::Light *pLight) override;
    bool LightEnable(uint32_t dwIndex, bool bOn) override;
    bool SetMaterial(storm_render::Material &material) override;
    bool GetLightEnable(uint32_t dwIndex, BOOL *pEnable) override;
    bool GetLight(uint32_t dwIndex, storm_render::Light *pLight) override;

    // DX9Render: Screenshot Section
    void SaveShoot() override;

    // DX9Render: Clip Planes Section
    storm_render::Result SetClipPlane(uint32_t Index, const float *pPlane) override;
    PLANE *GetPlanes() override;

    // DX9Render: Camera Section
    void SetTransform(int32_t type, storm_render::Matrix *mtx) override;
    void GetTransform(int32_t type, storm_render::Matrix *mtx) override;

    bool SetCamera(const CVECTOR &pos, const CVECTOR &ang, float perspective) override;
    bool SetCamera(const CVECTOR &pos, const CVECTOR &ang, float perspective, float magnification) override;
    bool SetCamera(const CVECTOR &pos, const CVECTOR &ang) override;
    bool SetCamera(CVECTOR lookFrom, CVECTOR lookTo, CVECTOR up) override;
    bool SetPerspective(float perspective, float fAspectRatio = -1.0f, float magnification = 1.f) override;
    void GetCamera(CVECTOR &pos, CVECTOR &ang, float &perspective) override;
    float GetCameraMagnification() const override { return projectionMagnification; }

    bool SetCurrentMatrix(storm_render::Matrix *mtx) override;

    // DX9Render: Textures Section
    int32_t TextureCreate(const char *fname) override;
    int32_t TextureCreate(UINT width, UINT height, UINT levels, uint32_t usage, storm_render::AssetFormat format, storm_render::MemoryClass pool) override;
    bool TextureSet(int32_t stage, int32_t texid) override;
    bool TextureRelease(int32_t texid) override;
    bool TextureIncReference(int32_t texid) override;

    // DX9Render: Fonts Section
    int32_t Print(int32_t x, int32_t y, const char *format, ...) override;
    int32_t Print(int32_t nFontNum, uint32_t color, int32_t x, int32_t y, const char *format, ...) override;
    int32_t ExtPrint(int32_t nFontNum, uint32_t foreColor, uint32_t backColor, int wAlignment, bool bShadow, float fScale,
                  int32_t scrWidth, int32_t scrHeight, int32_t x, int32_t y, const char *format, ...) override;
    int32_t StringWidth(const char *string, int32_t nFontNum = 0, float fScale = 1.f, int32_t scrWidth = 0) override;
    int32_t StringWidth(const std::string_view &string, int32_t nFontNum = 0, float fScale = 1.f, int32_t scrWidth = 0) override;
    int32_t CharWidth(utf8::u8_char, int32_t nFontNum = 0, float fScale = 1.f, int32_t scrWidth = 0) override;
    int32_t CharHeight(int32_t fontID) override;
    int32_t LoadFont(const char *fontName) override;   // returns the number \ font id, or -1 on error
    bool UnloadFont(const char *fontName) override; // returns true if the font is still in use
    bool UnloadFont(int32_t fontID) override;          // returns true if the font is still in use
    bool IncRefCounter(int32_t fontID) override;       // increase reference counter if object is being copied
    bool SetCurFont(const char *fontName) override; // returns true if the given font is installed
    bool SetCurFont(int32_t fontID) override;          // returns true if the given font is installed
    int32_t GetCurFont() override;
    char *GetFontIniFileName() override;
    bool SetFontIniFileName(const char *iniName) override;

    // DX9Render: Techniques Section
    bool TechniqueExecuteStart(const char *cBlockName) override;
    bool TechniqueExecuteNext() override;

    // DX9Render: Draw Section
    void DrawRects(RS_RECT *pRSR, uint32_t dwRectsNum, const char *cBlockName = nullptr, uint32_t dwSubTexturesX = 1,
                   uint32_t dwSubTexturesY = 1, float fScaleX = 1.0f, float fScaleY = 1.0f) override;
    void DrawSprites(RS_SPRITE *pRSS, uint32_t dwSpritesNum, const char *cBlockName = nullptr) override;
    void DrawLines(RS_LINE *pRSL, uint32_t dwLinesNum, const char *cBlockName = nullptr) override;
    void DrawLines2D(RS_LINE2D *pRSL2D, size_t dwLinesNum, const char *cBlockName = nullptr) override;

    void DrawBuffer(int32_t vbuff, int32_t stride, int32_t ibuff, int32_t minv, size_t numv, size_t startidx, size_t numtrg,
                    const char *cBlockName = nullptr) override;
    void DrawIndexedPrimitiveNoVShader(storm_render::Primitive dwPrimitiveType, int32_t iVBuff, int32_t iStride, int32_t iIBuff,
                                       int32_t iMinV, int32_t iNumV, int32_t iStartIdx, int32_t iNumTrg,
                                       const char *cBlockName = nullptr) override;
    void DrawPrimitive(storm_render::Primitive dwPrimitiveType, int32_t iVBuff, int32_t iStride, int32_t iStartV, int32_t iNumPT,
                       const char *cBlockName = nullptr) override;
    void DrawPrimitiveUP(storm_render::Primitive dwPrimitiveType, uint32_t dwVertexBufferFormat, uint32_t dwNumPT,
                         const void *pVerts, uint32_t dwStride, const char *cBlockName = nullptr) override;
    void DrawIndexedPrimitiveUP(storm_render::Primitive dwPrimitiveType, uint32_t dwMinIndex, uint32_t dwNumVertices,
                                uint32_t dwPrimitiveCount, const void *pIndexData, storm_render::IndexFormat IndexDataFormat,
                                const void *pVertexData, uint32_t dwVertexStride,
                                const char *cBlockName = nullptr) override;

    // DX9Render: Video Section
    void PlayToTexture() override;
    CVideoTexture *GetVideoTexture(const char *sVideoName) override;
    void ReleaseVideoTexture(CVideoTexture *pVTexture) override;

    // DX9Render: Vertex/Index Buffers Section
    int32_t CreateVertexBuffer(int32_t type, size_t nverts, uint32_t usage, storm_render::MemoryClass dwPool) override;
    int32_t CreateIndexBuffer(size_t ntrgs, uint32_t dwUsage) override;

    storm_render::VertexBuffer *GetVertexBuffer(int32_t id) override;
    int32_t GetVertexBufferFVF(int32_t id) override;
    void *LockVertexBuffer(int32_t id, uint32_t dwFlags = 0) override;
    void UnLockVertexBuffer(int32_t id) override;
    int32_t GetVertexBufferSize(int32_t id) override;
    void *LockIndexBuffer(int32_t id, uint32_t dwFlags = 0) override;
    void UnLockIndexBuffer(int32_t id) override;
    void ReleaseVertexBuffer(int32_t id) override;
    void ReleaseIndexBuffer(int32_t id) override;

    // DX9Render: Render/Texture States Section
    uint32_t SetRenderState(uint32_t State, uint32_t Value) override;
    uint32_t GetRenderState(uint32_t State, uint32_t *pValue) override;
    uint32_t GetSamplerState(uint32_t Sampler, storm_render::SamplerField Type, uint32_t *pValue) override;
    uint32_t SetSamplerState(uint32_t Sampler, storm_render::SamplerField Type, uint32_t Value) override;
    uint32_t SetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t Value) override;
    uint32_t GetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t *pValue) override;

    // aspect ratio section
    float GetHeightDeformator() override
    {
        return m_fHeightDeformator;
    }

    storm_render::Point GetScreenSize() override
    {
        return screen_size;
    }

    // ===============================================================================================
    // --------------------===================== D3D SECTION =====================--------------------
    // ===============================================================================================

    // D3D Device/Viewport Section
    storm_render::Result GetViewport(storm_render::Viewport *pViewport) override;
    storm_render::Result SetViewport(const storm_render::Viewport *pViewport) override;
    storm_render::Result GetCapabilities(storm_render::Capabilities *pCaps) override;

    // D3D
    storm_render::Result SetStreamSource(UINT StreamNumber, void *pStreamData, UINT Stride) override;
    storm_render::Result SetIndices(void *pIndexData) override;
    storm_render::Result DrawPrimitive(storm_render::Primitive PrimitiveType, UINT StartVertex, UINT PrimitiveCount) override;
    storm_render::Result Release(storm_render::RefCounted *pSurface) override;

    // Vertex/Index Buffers Section
    storm_render::Result CreateVertexBuffer(UINT Length, uint32_t Usage, uint32_t fvf, storm_render::MemoryClass Pool,
                               storm_render::VertexBuffer **ppVertexBuffer) override;
    storm_render::Result VBLock(storm_render::VertexBuffer *pVB, UINT OffsetToLock, UINT SizeToLock, uint8_t **ppbData,
                   uint32_t Flags) override;
    void VBUnlock(storm_render::VertexBuffer *pVB) override;

    // D3D Textures/Surfaces Section
    storm_render::Result GetDepthStencilSurface(storm_render::Surface **ppZStencilSurface) override;
    storm_render::Result GetCubeMapSurface(storm_render::CubeTexture *ppCubeTexture, storm_render::CubeFace FaceType, UINT Level,
                              storm_render::Surface **ppCubeMapSurface) override;
    storm_render::Result CreateTexture(UINT Width, UINT Height, UINT Levels, uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool,
                          storm_render::Texture **ppTexture) override;
    storm_render::Result CreateCubeTexture(UINT EdgeLength, UINT Levels, uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool,
                              storm_render::CubeTexture **ppCubeTexture) override;
    storm_render::Result CreateOffscreenPlainSurface(UINT Width, UINT Height, storm_render::AssetFormat Format,
                                        storm_render::Surface **ppSurface) override;
    storm_render::Result CreateDepthStencilSurface(UINT Width, UINT Height, storm_render::AssetFormat Format, storm_render::SampleCount MultiSample,
                                      storm_render::Surface **ppSurface) override;
    storm_render::Result SetTexture(uint32_t Stage, storm_render::BaseTexture *pTexture) override;
    storm_render::Result GetLevelDesc(storm_render::Texture *ppTexture, UINT Level, storm_render::SurfaceDesc *pDesc) override;
    storm_render::Result GetLevelDesc(storm_render::CubeTexture *ppCubeTexture, UINT Level, storm_render::SurfaceDesc *pDesc) override;
    storm_render::Result LockRect(storm_render::CubeTexture *ppCubeTexture, storm_render::CubeFace FaceType, UINT Level,
                     storm_render::LockedRect *pLockedRect, const storm_render::Rect *pRect, uint32_t Flags) override;
    storm_render::Result LockRect(storm_render::Texture *ppTexture, UINT Level, storm_render::LockedRect *pLockedRect, const storm_render::Rect *pRect,
                     uint32_t Flags) override;
    storm_render::Result UnlockRect(storm_render::CubeTexture *pCubeTexture, storm_render::CubeFace FaceType, UINT Level) override;
    storm_render::Result UnlockRect(storm_render::Texture *pTexture, UINT Level) override;
    storm_render::Result GetSurfaceLevel(storm_render::Texture *ppTexture, UINT Level, storm_render::Surface **ppSurfaceLevel) override;
    storm_render::Result UpdateSurface(storm_render::Surface *pSourceSurface, const storm_render::Rect *pSourceRectsArray, UINT cRects,
                          storm_render::Surface *pDestinationSurface, const storm_render::Point *pDestPointsArray) override;
    storm_render::Result StretchRect(storm_render::Surface *pSourceSurface, const storm_render::Rect *pSourceRect, storm_render::Surface *pDestSurface,
                        const storm_render::Rect *pDestRect, storm_render::TextureFilter Filter) override;
    storm_render::Result GetRenderTargetData(storm_render::Surface *pRenderTarget, storm_render::Surface *pDestSurface) override;

    // D3D Pixel/Vertex Shaders Section
    storm_render::Result CreateVertexDeclaration(const storm_render::VertexElement *pVertexElements,
                                    storm_render::VertexLayout **ppDecl) override;
    storm_render::Result SetVertexDeclaration(storm_render::VertexLayout *pDecl) override;
    storm_render::Result ImportFragmentProgram(const uint32_t *pFunction, storm_render::Program **ppShader) override;
    storm_render::Result ImportVertexProgram(const uint32_t *pFunction, storm_render::Program **ppShader) override;
    storm_render::Result DeletePixelShader(storm_render::Program *pShader) override;
    storm_render::Result DeleteVertexShader(storm_render::Program *pShader) override;
    storm_render::Result BindVertexProgram(storm_render::Program *pShader) override;
    storm_render::Result BindFragmentProgram(storm_render::Program *pShader) override;
    /*virtual HRESULT SetFVFConstant(DWORD Register, const void* pConstantData, DWORD  ConstantCount );*/
    storm_render::Result SetVertexShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4iCount) override;
    storm_render::Result SetPixelShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4iCount) override;
    storm_render::Result SetFVF(uint32_t handle) override;
    storm_render::Result GetVertexProgram(storm_render::Program **ppShader) override;
    storm_render::Result GetFragmentProgram(storm_render::Program **ppShader) override;
#ifdef _WIN32 // Effects
    ID3DXEffect *GetEffectPointer(const char *techniqueName) override;
#endif

    // D3D Render Target/Begin/End/Clear
    storm_render::Result GetRenderTarget(storm_render::Surface **ppRenderTarget) override;
    storm_render::Result SetRenderTarget(storm_render::Surface *pRenderTarget, storm_render::Surface *pNewZStencil) override;
    storm_render::Result Clear(uint32_t Flags, storm_render::PackedColor Color, float Z) override;
    storm_render::Result BeginScene() override;
    storm_render::Result EndScene() override;

    storm_render::Result ImageBlt(const char *pName, storm_render::Rect *pDstRect, storm_render::Rect *pSrcRect) override;
    storm_render::Result ImageBlt(int32_t nTextureId, storm_render::Rect *pDstRect, storm_render::Rect *pSrcRect) override;

    void MakeScreenShot();
    bool LoadTextureSurface(std::fstream &fileS, storm_render::Surface *suface, uint32_t mipSize, uint32_t width,
                            uint32_t height, bool isSwizzled);
    uint32_t LoadCubmapSide(std::fstream &fileS, storm_render::CubeTexture *tex, storm_render::CubeFace face, uint32_t numMips,
                            uint32_t mipSize, uint32_t size, bool isSwizzled);

    // core interface
    bool Init() override;
    void RunStart() override;
    void RunEnd() override;

    uint32_t RunSection() override
    {
        return SECTION_REALIZE;
    };
    bool LoadState(ENTITY_STATE *state) override;
    bool CreateState(ENTITY_STATE_GEN *state_gen) override;

    void ProcessScriptPosAng(const CVECTOR &vPos, const CVECTOR &vAng);
    void FindPlanes(storm_render::Device *d3dDevice);

    void SetCommonStates();
    void SetProgressImage(const char *image) override;
    void SetProgressBackImage(const char *image) override;
    void SetTipsImage(const char *image) override;
    void StartProgressView() override;
    void ProgressView() override;
    void EndProgressView() override;

    static const uint32_t rectsVBuffer_SizeInRects;
    storm_render::VertexBuffer *rectsVBuffer;

    char *progressImage;
    int32_t progressImageSize;
    int32_t backTexture;
    char *progressBackImage;
    int32_t progressBackImageSize;
    int32_t back0Texture;
    int32_t progressTexture;
    char *progressTipsImage;
    int32_t progressTipsImageSize;
    int32_t progressTipsTexture;

    int32_t loadFrame;
    int32_t progressSafeCounter;
    bool isInPViewProcess;
    uint32_t progressUpdateTime;
    float progressFramesPosX;
    float progressFramesPosY;
    float progressFramesWidth;
    float progressFramesHeight;
    int32_t progressFramesCountX;
    int32_t progressFramesCountY;

    // new renderer settings
    bool vSyncEnabled;

    storm_render::Presentation d3dpp;

    CMatrix mView, mWorld, mProjection;

    CVECTOR vWordRelationPos;
    CVECTOR vViewRelationPos;

    bool bUseLargeBackBuffer;
	
	bool bNewFovCalculation;	// evganat - выбор формулы vfov

    bool resourcesReleased = false;
    bool desktopMode = false;
    struct MetalGraphicsRequest
    {
        uint32_t flagsMask{};
        uint32_t shadowQuality{};
        int32_t fullscreen{};
        int32_t renderWidth{};
        int32_t renderHeight{};
        int32_t desktopMode{};
        bool queued{};
    } pendingMetalGraphics;
    int32_t metalGraphicsApplyResult = 0;

    bool IsInsideScene() override
    {
        return bInsideScene;
    }

    char *GetTipsImage() override;

    void SetColorParameters(float fGamma, float fBrightness, float fContrast) override;
    void DrawSphere(const CVECTOR &vPos, float fRadius, uint32_t dwColor) override;
    void DrawEllipsoid(const CVECTOR &vPos, float a, float b, float c, float ay, uint32_t dwColor) override;

    void GetNearFarPlane(float &fNear, float &fFar) override;
    void SetNearFarPlane(float fNear, float fFar) override;

    void SetLoadTextureEnable(bool bEnable = true) override;
    bool ResetDevice();

    void MakeDrawVector(RS_LINE *pLines, uint32_t dwNumSubLines, const CMatrix &mMatrix, CVECTOR vUp, CVECTOR v1,
                        CVECTOR v2, float fScale, uint32_t dwColor);
    void DrawVector(const CVECTOR &v1, const CVECTOR &v2, uint32_t dwColor,
                    const char *pTechniqueName = "DXVector") override;
    storm_render::BaseTexture *GetBaseTexture(int32_t iTexture) override;

    bool PushRenderTarget() override;
    bool PopRenderTarget() override;
    bool SetRenderTarget(storm_render::CubeTexture *pCubeTex, uint32_t dwFaceType, uint32_t dwLevel,
                         storm_render::Surface *pNewZStencil) override;
    void SetView(const CMatrix &mView) override;
    void SetWorld(const CMatrix &mView) override;
    void SetProjection(const CMatrix &mView) override;
    const CMatrix &GetView() override;
    const CMatrix &GetWorld() override;
    const CMatrix &GetProjection() override;

    storm_render::VolumeTexture *CreateVolumeTexture(uint32_t Width, uint32_t Height, uint32_t Depth, uint32_t Levels,
                                                 uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool) override;

    void SetGLOWParams(float _fBlurBrushSize, int32_t _GlowIntensity, int32_t _GlowPasses) override;

    storm_render::BaseTexture *GetTextureFromID(int32_t nTextureID) override;

    bool GetRenderTargetAsTexture(storm_render::Texture **tex) override;

    void LostRender();
    void RestoreRender();
    void ProcessPendingMetalGraphics();

    void RecompileEffects();

private:
    struct RECT_VERTEX
    {
        CVECTOR pos;
        uint32_t color;
        float u, v;
    };

    struct RenderTarget
    {
        storm_render::Surface *pRenderTarget;
        storm_render::Surface *pDepthSurface;
        storm_render::Viewport ViewPort;
    };

    storm_render::Device *renderDevice;
    HWND hwnd;

    CVECTOR Pos, Ang;
    float Fov;
    float perspectiveInput = 1.f;
    float projectionMagnification = 1.f;

    float FovMultiplier{ 1.0f };

#ifdef _WIN32 // Effects
    Effects effects_;
#else
    std::unique_ptr<CTechnique> pTechnique;
#endif

    char *fontIniFileName;
    int32_t nFontQuantity;
    FONTEntity FontList[MAX_FONTS]{};
    int32_t idFontCurrent;

    VideoTextureEntity *pVTL;

    int32_t nTextureDegradation;
    float aspectRatio;
    float m_fHeightDeformator;

    bool bSafeRendering;
    bool bShowFps, bShowExInfo;
    bool bInsideScene;

    PLANE viewplane[4];

    STEXTURE Textures[MAX_STEXTURES]{};
    INDEX_BUFFER IndexBuffers[MAX_BUFFERS]{};
    VERTEX_BUFFER VertexBuffers[MAX_BUFFERS]{};

    bool MakeAvi;
    storm_render::Surface *ImageBuffer;

    //-------- post process

    struct QuadVertex
    {
        Vector4 vPos;

        float u0;
        float v0;

        float u1;
        float v1;

        float u2;
        float v2;

        float u3;
        float v3;
    };

    QuadVertex PostProcessQuad[4];
    QuadVertex qv[32 * 32];
    uint16_t qi[31 * 31 * 2 * 3];

    float fSmallWidth;
    float fSmallHeight;
    storm_render::Texture *pPostProcessTexture;
    storm_render::Surface *pPostProcessSurface;

    storm_render::Texture *pSmallPostProcessTexture;
    storm_render::Surface *pSmallPostProcessSurface;

    storm_render::Texture *pSmallPostProcessTexture2;
    storm_render::Surface *pSmallPostProcessSurface2;

    storm_render::Surface *pOriginalScreenSurface;
    storm_render::Surface *pOriginalDepthSurface;

    storm_render::Viewport OriginalViewPort;

    void CreateRenderQuad(float fWidth, float fHeight, float fSrcWidth, float fSrcHeight, float fMulU = 1.0f,
                          float fMulV = 1.0f);

    void ClearPostProcessSurface(storm_render::Surface *pSurf);
    void BlurGlowTexture();
    void CopyGlowToScreen();
    void CopyPostProcessToScreen();

    void SetPostProcessTextureAsRenderTarget();
    void SetScreenAsRenderTarget();

    float fBlurSize;
    int GlowIntensity;
    int iBlurPasses;

    bool bNeedCopyToScreen;

    bool bPostProcessEnabled;
    bool bPostProcessError;

    bool bSeaEffect;
    float fSeaEffectSize;
    float fSeaEffectSpeed;
    uint32_t dwBackColor;

    //-------- post process

    // state save/load ex
    storm_render::Point screen_size;
    storm_render::AssetFormat screen_bpp;
    storm_render::AssetFormat stencil_format;

    bool bMakeShoot;
    bool bWindow;
    bool bBackBufferCanLock;

    storm_render::VertexBuffer *aniVBuffer;
    int32_t numAniVerteces;

    storm_render::VertexBuffer *pDropConveyorVBuffer;

    uint32_t dwNumDrawPrimitive, dwNumLV, dwNumLI;
    float fG, fB, fC;

    float fNearClipPlane, fFarClipPlane;

    bool bLoadTextureEnabled;

    bool bTrace;
    int32_t iSetupPath;
    uint64_t dwSetupNumber;
    texpaths_t TexPaths[4]{};

    bool bDropVideoConveyor;

    std::stack<RenderTarget> stRenderTarget;

#ifdef _WIN32 // Screenshot
    D3DXIMAGE_FILEFORMAT screenshotFormat;
#endif
    std::string screenshotExt;

    bool TextureLoad(int32_t texid);
    bool TextureLoadUsingD3DX(const char *path, int32_t texid);
};
