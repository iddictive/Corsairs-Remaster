#pragma once

#include <cstdint>
#include "render_device_api.hpp"
#include "windows_base.h"
#ifdef _WIN32
#include <d3dx9.h>
#endif
#include <string_view>

#include "entity.h"
#include "matrix.h"
#include "service.h"
#include "storm_assert.h"
#include "types3d.h"
#include "utf8.h"

#define FONT_DEFAULT 0

// define print text alignments
#define PR_ALIGN_LEFT 0
#define PR_ALIGN_RIGHT 1
#define PR_ALIGN_CENTER 2

#define INVALID_BUFFER_ID 0xFFFFFFFF
#define INVALID_LIGHT_ID 0xFFFFFFFF
#define INVALID_TEXTURE_ID 0xFFFFFFFF
#define INVALID_RS_ID 0xFFFFFFFF

using RS_ID = uint32_t;

using renderer_handle = uint32_t;

struct FVF_VERTEX
{
    float x, y, z;
    float Nx, Ny, Nz;
    float tu, tv;
};

struct RS_RECT
{
    CVECTOR vPos;
    float fSize;
    float fAngle;
    uint32_t dwColor;
    uint32_t dwSubTexture;
};

struct RS_LINE
{
    CVECTOR vPos;
    uint32_t dwColor;
};

struct RS_SPRITE
{
    CVECTOR vPos;
    float rhw;
    uint32_t dwColor;
    float tu, tv;
};

inline void FillSpriteVertex(RS_SPRITE &pV, float x, float y, float z = 1.0f, uint32_t dwColor = 0xFFFFFF,
                             float tu = 0.0f, float tv = 0.0f)
{
    pV.vPos.x = x;
    pV.vPos.y = y;
    pV.vPos.z = z;
    pV.dwColor = dwColor;
    pV.rhw = 0.5f;
    pV.tu = tu;
    pV.tv = tv;
}

struct RS_LINE2D
{
    CVECTOR vPos;
    float rhw;
    uint32_t dwColor;
};


struct IPOINT
{
    int32_t x, y;
};

struct FPOINT
{
    float x, y;
};

struct FRECT
{
    union {
        struct
        {
            float x1, y1, x2, y2;
        };

        struct
        {
            float xs, ys, xe, ye;
        };

        struct
        {
            float x_start, y_start, x_end, y_end;
        };

        struct
        {
            float left, top, right, bottom;
        };
    };
};


//TODO: remove
#define COLOR2VECTOR(a) CVECTOR(float((a & 0xFF0000) >> 0x10), float((a & 0xFF00) >> 0x8), float(a & 0xFF));
#define COLOR2VECTOR4(a)                                                                                               \
    CVECTOR4(float((a & 0xFF0000) >> 0x10) / 255.0f, float((a & 0xFF00) >> 0x8) / 255.0f, float(a & 0xFF) / 255.0f,    \
             float((a & 0xFF000000) >> 0x18) / 255.0f);
#define ARGB(a, r, g, b) (uint32_t(b) | (uint32_t(g) << 8L) | (uint32_t(r) << 16L) | (uint32_t(a) << 24L))
constexpr auto makeRGB(uint32_t r, uint32_t g, uint32_t b)
{
    return b | g << 8L | r << 16L;
}
inline uint32_t F2DW(float f)
{
    return *reinterpret_cast<uint32_t *>(&f);
}
#define TEXTURE_RELEASE(rs, idtex)                                                                                     \
    {                                                                                                                  \
        if (rs != NULL && idtex != -1)                                                                                 \
        {                                                                                                              \
            rs->TextureRelease(idtex);                                                                                 \
            idtex = -1;                                                                                                \
        }                                                                                                              \
    }

#define FONT_RELEASE(rs, font)                                                                                         \
    {                                                                                                                  \
        if (rs != NULL && font != -1)                                                                                  \
        {                                                                                                              \
            rs->UnloadFont(font);                                                                                      \
            font = -1;                                                                                                 \
        }                                                                                                              \
    }

#define VERTEX_BUFFER_RELEASE(rs, vb)                                                                                  \
    {                                                                                                                  \
        if (rs != NULL && vb != -1)                                                                                    \
        {                                                                                                              \
            rs->ReleaseVertexBuffer(vb);                                                                               \
            vb = -1;                                                                                                   \
        }                                                                                                              \
    }

#define INDEX_BUFFER_RELEASE(rs, ib)                                                                                   \
    {                                                                                                                  \
        if (rs != NULL && ib != -1)                                                                                    \
        {                                                                                                              \
            rs->ReleaseIndexBuffer(ib);                                                                                \
            ib = -1;                                                                                                   \
        }                                                                                                              \
    }
//

class CVideoTexture;

class VDX9RENDER : public SERVICE
{
  public:
    // DX9Render: Construct/Destruct
    ~VDX9RENDER() override{};

    // DX9Render: Init/Release
    virtual bool InitDevice(bool windowed, HWND hwnd, int32_t width, int32_t height) = 0;
    virtual bool ReleaseDevice() = 0;
    // UI graphics transaction: flags bits 0..6 and one validated display snapshot.
    virtual int32_t ApplyMetalGraphics(uint32_t flagsMask, uint32_t shadowQuality, int32_t fullscreen,
                                       int32_t renderWidth, int32_t renderHeight, int32_t desktopMode) = 0;
    virtual int32_t ConsumeMetalGraphicsApplyResult() = 0;

    // DX9Render: Animation
    virtual void RenderAnimation(int32_t ib, void *src, int32_t numVrts, int32_t minv, int32_t numv, int32_t startidx, int32_t numtrg,
                                 bool isUpdateVB) = 0;

    // DX9Render: Return d3d9 device
    // virtual void * GetDevice() = 0;            // (very bad function/don't use!!!)
    virtual storm_render::Device *GetNativeDevice() = 0;
    virtual void FinalizeSceneEffects() = 0;

    // DX9Render: Render Target/Begin/End/Clear
    virtual bool DX9Clear(int32_t type) = 0; // D3DCLEAR_STENCIL | D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER
    virtual bool DX9BeginScene() = 0;
    virtual bool DX9EndScene() = 0;

    // DX9Render: Materials/Lights Section
    virtual bool SetLight(uint32_t dwIndex, const storm_render::Light *pLight) = 0;
    virtual bool LightEnable(uint32_t dwIndex, bool bOn) = 0;
    virtual bool SetMaterial(storm_render::Material &material) = 0;
    virtual bool GetLightEnable(uint32_t dwIndex, BOOL *pEnable) = 0;
    virtual bool GetLight(uint32_t dwIndex, storm_render::Light *pLight) = 0;

    // DX9Render: Screenshot Section
    virtual void SaveShoot() = 0;

    // DX9Render: Clip Planes Section
    virtual storm_render::Result SetClipPlane(uint32_t Index, const float *pPlane) = 0;
    virtual PLANE *GetPlanes() = 0;

    // DX9Render: Camera Section
    virtual void SetTransform(int32_t type, storm_render::Matrix *mtx) = 0;
    virtual void GetTransform(int32_t type, storm_render::Matrix *mtx) = 0;

    virtual bool SetCamera(const CVECTOR &pos, const CVECTOR &ang, float perspective) = 0;
    virtual bool SetCamera(const CVECTOR &pos, const CVECTOR &ang, float perspective, float magnification) = 0;
    virtual bool SetCamera(const CVECTOR &pos, const CVECTOR &ang) = 0;
    virtual bool SetCamera(CVECTOR lookFrom, CVECTOR lookTo, CVECTOR up) = 0;
    virtual bool SetPerspective(float perspective, float fAspectRatio = -1.0f, float magnification = 1.f) = 0;
    virtual void GetCamera(CVECTOR &pos, CVECTOR &ang, float &perspective) = 0;
    virtual float GetCameraMagnification() const { return 1.f; }

    virtual bool SetCurrentMatrix(storm_render::Matrix *mtx) = 0;

    // DX9Render: Textures Section
    virtual int32_t TextureCreate(const char *fname) = 0;
    virtual int32_t TextureCreate(UINT width, UINT height, UINT levels, uint32_t usage, storm_render::AssetFormat format, storm_render::MemoryClass pool) = 0;
    virtual bool TextureSet(int32_t stage, int32_t texid) = 0;
    virtual bool TextureRelease(int32_t texid) = 0;
    virtual bool TextureIncReference(int32_t texid) = 0;

    // DX9Render: Fonts Section
    virtual int32_t Print(int32_t x, int32_t y, const char *format, ...) = 0;
    virtual int32_t Print(int32_t nFontNum, uint32_t color, int32_t x, int32_t y, const char *format, ...) = 0;
    virtual int32_t ExtPrint(int32_t nFontNum, uint32_t foreColor, uint32_t backColor, int wAlignment, bool bShadow,
                          float fScale, int32_t scrWidth, int32_t scrHeight, int32_t x, int32_t y, const char *format, ...) = 0;
    [[deprecated("Pass string as string_view instead")]]
    virtual int32_t StringWidth(const char *string, int32_t nFontNum = 0, float fScale = 1.f, int32_t scrWidth = 0) = 0;
    virtual int32_t StringWidth(const std::string_view &string, int32_t nFontNum = 0, float fScale = 1.f, int32_t scrWidth = 0) = 0;
    virtual int32_t CharWidth(utf8::u8_char ucVKey, int32_t nFontNum = 0, float fScale = 1.f, int32_t scrWidth = 0) = 0;
    virtual int32_t CharHeight(int32_t fontID) = 0;
    virtual int32_t LoadFont(const char *fontName) = 0;   // returns the number \ font id, or -1 on error
    virtual bool UnloadFont(const char *fontName) = 0; // returns true if the font is still in use
    virtual bool UnloadFont(int32_t fontID) = 0;          // returns true if the font is still in use
    virtual bool IncRefCounter(int32_t fontID) = 0;       // increase reference counter if object is being copied
    virtual bool SetCurFont(const char *fontName) = 0; // returns true if the given font is installed
    virtual bool SetCurFont(int32_t fontID) = 0;          // returns true if the given font is installed
    virtual int32_t GetCurFont() = 0;
    virtual char *GetFontIniFileName() = 0;
    virtual bool SetFontIniFileName(const char *iniName) = 0;

    // DX9Render: Techniques Section
    virtual bool TechniqueExecuteStart(const char *cBlockName) = 0;
    virtual bool TechniqueExecuteNext() = 0;

    // DX9Render: Draw Section
    virtual void DrawRects(RS_RECT *pRSR, uint32_t dwRectsNum, const char *cBlockName = nullptr,
                           uint32_t dwSubTexturesX = 1, uint32_t dwSubTexturesY = 1, float fScaleX = 1.0f,
                           float fScaleY = 1.0f) = 0;
    virtual void DrawSprites(RS_SPRITE *pRSS, uint32_t dwSpritesNum, const char *cBlockName = nullptr) = 0;
    virtual void DrawLines(RS_LINE *pRSL, uint32_t dwLinesNum, const char *cBlockName = nullptr) = 0;
    virtual void DrawVector(const CVECTOR &v1, const CVECTOR &v2, uint32_t dwColor,
                            const char *pTechniqueName = "DXVector") = 0;
    virtual void DrawLines2D(RS_LINE2D *pRSL2D, size_t dwLinesNum, const char *cBlockName = nullptr) = 0;

    virtual void DrawBuffer(int32_t vbuff, int32_t stride, int32_t ibuff, int32_t minv, size_t numv, size_t startidx, size_t numtrg,
                            const char *cBlockName = nullptr) = 0;
    virtual void DrawIndexedPrimitiveNoVShader(storm_render::Primitive dwPrimitiveType, int32_t iVBuff, int32_t iStride, int32_t iIBuff,
                                               int32_t iMinV, int32_t iNumV, int32_t iStartIdx, int32_t iNumTrg,
                                               const char *cBlockName = nullptr) = 0;
    virtual void DrawPrimitive(storm_render::Primitive dwPrimitiveType, int32_t iVBuff, int32_t iStride, int32_t iStartV, int32_t iNumPT,
                               const char *cBlockName = nullptr) = 0;
    virtual void DrawPrimitiveUP(storm_render::Primitive dwPrimitiveType, uint32_t dwVertexBufferFormat, uint32_t dwNumPT,
                                 const void *pVerts, uint32_t dwStride, const char *cBlockName = nullptr) = 0;
    virtual void DrawIndexedPrimitiveUP(storm_render::Primitive dwPrimitiveType, uint32_t dwMinIndex, uint32_t dwNumVertices,
                                        uint32_t dwPrimitiveCount, const void *pIndexData, storm_render::IndexFormat IndexDataFormat,
                                        const void *pVertexData, uint32_t dwVertexStride,
                                        const char *cBlockName = nullptr) = 0;

    // DX9Render: Video Section
    virtual void PlayToTexture() = 0;
    virtual CVideoTexture *GetVideoTexture(const char *sVideoName) = 0;
    virtual void ReleaseVideoTexture(CVideoTexture *pVTexture) = 0;

    // DX9Render: Vertex/Index Buffers Section
    virtual int32_t CreateVertexBuffer(int32_t type, size_t nverts, uint32_t usage,
                                       storm_render::MemoryClass dwPool = storm_render::MemoryClass::Default) = 0;
    virtual int32_t CreateIndexBuffer(size_t ntrgs, uint32_t dwUsage = storm_render::usage::writeOnly) = 0;

    virtual storm_render::VertexBuffer *GetVertexBuffer(int32_t id) = 0;
    virtual int32_t GetVertexBufferFVF(int32_t id) = 0;
    virtual void *LockVertexBuffer(int32_t id, uint32_t dwFlags = 0) = 0;
    virtual void UnLockVertexBuffer(int32_t id) = 0;
    virtual int32_t GetVertexBufferSize(int32_t id) = 0;
    virtual void *LockIndexBuffer(int32_t id, uint32_t dwFlags = 0) = 0;
    virtual void UnLockIndexBuffer(int32_t id) = 0;
    virtual void ReleaseVertexBuffer(int32_t id) = 0;
    virtual void ReleaseIndexBuffer(int32_t id) = 0;

    // DX9Render: Render/Texture States Section
    virtual uint32_t SetRenderState(uint32_t State, uint32_t Value) = 0;
    virtual uint32_t GetRenderState(uint32_t State, uint32_t *pValue) = 0;
    virtual uint32_t GetSamplerState(uint32_t Sampler, storm_render::SamplerField Type, uint32_t *pValue) = 0;
    virtual uint32_t SetSamplerState(uint32_t Sampler, storm_render::SamplerField Type, uint32_t Value) = 0;
    virtual uint32_t SetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t Value) = 0;
    virtual uint32_t GetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t *pValue) = 0;

    // aspect ratio section
    virtual float GetHeightDeformator() = 0;
    virtual storm_render::Point GetScreenSize() = 0;
    // ===============================================================================================
    // --------------------===================== D3D SECTION =====================--------------------
    // ===============================================================================================

    // D3D Device/Viewport Section
    virtual storm_render::Result GetViewport(storm_render::Viewport *pViewport) = 0;
    virtual storm_render::Result SetViewport(const storm_render::Viewport *pViewport) = 0;
    virtual storm_render::Result GetCapabilities(storm_render::Capabilities *pCaps) = 0;

    // D3D
    virtual storm_render::Result SetStreamSource(UINT StreamNumber, void *pStreamData, UINT Stride) = 0;
    virtual storm_render::Result SetIndices(void *pIndexData) = 0;
    virtual storm_render::Result DrawPrimitive(storm_render::Primitive PrimitiveType, UINT StartVertex, UINT PrimitiveCount) = 0;

    virtual storm_render::Result Release(storm_render::RefCounted *pSurface) = 0;

    // Vertex/Index Buffers Section
    virtual storm_render::Result CreateVertexBuffer(UINT Length, uint32_t Usage, uint32_t FVF, storm_render::MemoryClass Pool,
                                       storm_render::VertexBuffer **ppVertexBuffer) = 0;
    virtual storm_render::Result VBLock(storm_render::VertexBuffer *pVB, UINT OffsetToLock, UINT SizeToLock, uint8_t **ppbData,
                           uint32_t Flags) = 0;
    virtual void VBUnlock(storm_render::VertexBuffer *pVB) = 0;

    // D3D Textures/Surfaces Section
    virtual storm_render::Result GetDepthStencilSurface(storm_render::Surface **ppZStencilSurface) = 0;
    virtual storm_render::Result GetCubeMapSurface(storm_render::CubeTexture *ppCubeTexture, storm_render::CubeFace FaceType, UINT Level,
                                      storm_render::Surface **ppCubeMapSurface) = 0;
    virtual storm_render::Result CreateTexture(UINT Width, UINT Height, UINT Levels, uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool,
                                  storm_render::Texture **ppTexture) = 0;
    virtual storm_render::Result CreateCubeTexture(UINT EdgeLength, UINT Levels, uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool,
                                      storm_render::CubeTexture **ppCubeTexture) = 0;
    virtual storm_render::Result CreateOffscreenPlainSurface(UINT Width, UINT Height, storm_render::AssetFormat Format,
                                                storm_render::Surface **ppSurface) = 0;
    virtual storm_render::Result CreateDepthStencilSurface(UINT Width, UINT Height, storm_render::AssetFormat Format,
                                              storm_render::SampleCount MultiSample, storm_render::Surface **ppSurface) = 0;
    virtual storm_render::Result SetTexture(uint32_t Stage, storm_render::BaseTexture *pTexture) = 0;
    virtual storm_render::Result GetLevelDesc(storm_render::Texture *ppTexture, UINT Level, storm_render::SurfaceDesc *pDesc) = 0;
    virtual storm_render::Result GetLevelDesc(storm_render::CubeTexture *ppCubeTexture, UINT Level, storm_render::SurfaceDesc *pDesc) = 0;
    virtual storm_render::Result LockRect(storm_render::CubeTexture *ppCubeTexture, storm_render::CubeFace FaceType, UINT Level,
                             storm_render::LockedRect *pLockedRect, const storm_render::Rect *pRect, uint32_t Flags) = 0;
    virtual storm_render::Result LockRect(storm_render::Texture *ppTexture, UINT Level, storm_render::LockedRect *pLockedRect, const storm_render::Rect *pRect,
                             uint32_t Flags) = 0;
    virtual storm_render::Result UnlockRect(storm_render::CubeTexture *pCubeTexture, storm_render::CubeFace FaceType, UINT Level) = 0;
    virtual storm_render::Result UnlockRect(storm_render::Texture *pTexture, UINT Level) = 0;
    virtual storm_render::Result GetSurfaceLevel(storm_render::Texture *ppTexture, UINT Level, storm_render::Surface **ppSurfaceLevel) = 0;
    virtual storm_render::Result UpdateSurface(storm_render::Surface *pSourceSurface, const storm_render::Rect *pSourceRectsArray, UINT cRects,
                                  storm_render::Surface *pDestinationSurface, const storm_render::Point *pDestPointsArray) = 0;
    virtual storm_render::Result StretchRect(storm_render::Surface *pSourceSurface, const storm_render::Rect *pSourceRect,
                                storm_render::Surface *pDestSurface, const storm_render::Rect *pDestRect,
                                storm_render::TextureFilter Filter) = 0;
    virtual storm_render::Result GetRenderTargetData(storm_render::Surface *pRenderTarget, storm_render::Surface *pDestSurface) = 0;

    // D3D Pixel/Vertex Shaders Section
    virtual storm_render::Result CreateVertexDeclaration(const storm_render::VertexElement *pVertexElements,
                                            storm_render::VertexLayout **ppDecl) = 0;
    virtual storm_render::Result SetVertexDeclaration(storm_render::VertexLayout *pDecl) = 0;
    virtual storm_render::Result ImportFragmentProgram(const uint32_t *pFunction, storm_render::Program **ppShader) = 0;
    virtual storm_render::Result ImportVertexProgram(const uint32_t *pFunction, storm_render::Program **ppShader) = 0;
    virtual storm_render::Result DeletePixelShader(storm_render::Program *pShader) = 0;
    virtual storm_render::Result DeleteVertexShader(storm_render::Program *pShader) = 0;
    virtual storm_render::Result BindVertexProgram(storm_render::Program *pShader) = 0;
    virtual storm_render::Result BindFragmentProgram(storm_render::Program *pShader) = 0;
    /*virtual HRESULT SetFVFConstant(DWORD Register, const void * pConstantData, DWORD  ConstantCount ) = 0;*/
    virtual storm_render::Result SetVertexShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4iCount) = 0;
    virtual storm_render::Result SetPixelShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4iCount) = 0;
    virtual storm_render::Result SetFVF(uint32_t handle) = 0;
    virtual storm_render::Result GetVertexProgram(storm_render::Program **ppShader) = 0;
    virtual storm_render::Result GetFragmentProgram(storm_render::Program **ppShader) = 0;
#ifdef _WIN32 // Effects
    virtual ID3DXEffect *GetEffectPointer(const char *techniqueName) = 0;
#endif

    // D3D Render Target/Begin/End/Clear
    virtual storm_render::Result GetRenderTarget(storm_render::Surface **ppRenderTarget) = 0;
    virtual storm_render::Result SetRenderTarget(storm_render::Surface *pRenderTarget, storm_render::Surface *pNewZStencil) = 0;
    virtual storm_render::Result Clear(uint32_t Flags, storm_render::PackedColor Color, float Z) = 0;
    virtual storm_render::Result BeginScene() = 0;
    virtual storm_render::Result EndScene() = 0;

    virtual storm_render::Result ImageBlt(const char *pName, storm_render::Rect *pDstRect = nullptr, storm_render::Rect *pSrcRect = nullptr) = 0;
    virtual storm_render::Result ImageBlt(int32_t nTextureId, storm_render::Rect *pDstRect = nullptr, storm_render::Rect *pSrcRect = nullptr) = 0;

    virtual void SetProgressImage(const char *image) = 0;
    virtual void SetProgressBackImage(const char *image) = 0;
    virtual void SetTipsImage(const char *image) = 0;
    virtual void StartProgressView() = 0;
    virtual void ProgressView() = 0;
    virtual void EndProgressView() = 0;

    virtual bool IsInsideScene() = 0;
    virtual char *GetTipsImage() = 0;
    virtual void SetColorParameters(float fGamma, float fBrightness, float fContrast) = 0;

    virtual void DrawSphere(const CVECTOR &vPos, float fRadius, uint32_t dwColor) = 0;
    virtual void DrawEllipsoid(const CVECTOR &vPos, float a, float b, float c, float ay, uint32_t dwColor) = 0;

    virtual void GetNearFarPlane(float &fNear, float &fFar) = 0;
    virtual void SetNearFarPlane(float fNear, float fFar) = 0;

    virtual void SetLoadTextureEnable(bool bEnable = true) = 0;
    virtual storm_render::BaseTexture *GetBaseTexture(int32_t iTexture) = 0;

    virtual bool PushRenderTarget() = 0;
    virtual bool PopRenderTarget() = 0;
    virtual bool SetRenderTarget(storm_render::CubeTexture *pCubeTex, uint32_t dwFaceType, uint32_t dwLevel,
                                 storm_render::Surface *pNewZStencil) = 0;
    virtual void SetView(const CMatrix &mView) = 0;
    virtual void SetWorld(const CMatrix &mView) = 0;
    virtual void SetProjection(const CMatrix &mView) = 0;
    virtual const CMatrix &GetView() = 0;
    virtual const CMatrix &GetWorld() = 0;
    virtual const CMatrix &GetProjection() = 0;

    virtual storm_render::VolumeTexture *CreateVolumeTexture(uint32_t Width, uint32_t Height, uint32_t Depth,
                                                         uint32_t Levels, uint32_t Usage, storm_render::AssetFormat Format,
                                                         storm_render::MemoryClass Pool) = 0;

    virtual void SetGLOWParams(float _fBlurBrushSize, int32_t _GlowIntensity, int32_t _GlowPasses) = 0;

    virtual storm_render::BaseTexture *GetTextureFromID(int32_t nTextureID) = 0;

    virtual bool GetRenderTargetAsTexture(storm_render::Texture **tex) = 0;
};
