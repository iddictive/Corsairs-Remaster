#include "s_device.h"
#include "sea_map_transition.hpp"
#include "antialiasing.hpp"
#include "metal_renderer_consumers.h"
#ifdef STORM_METAL_ROCKK2
extern "C" void storm_metal_label_texture(storm_render::BaseTexture *, const char *);
#endif

#ifndef _WIN32
#include "texture_payload.hpp"
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
extern "C" void StormMetalSetWorldOrigin(void*, const float*);
extern "C" void StormMetalSetAuthoredWorld(void*, const float*);
extern "C" bool StormMetalGetAuthoredWorld(void*, float*);
extern "C" void StormMetalRebaseWorld(void*, const float*);
#endif
#endif

#include "core.h"

#ifdef __APPLE__
extern "C" bool StormMetalCanApplyGraphicsSettings(void *, uint32_t, uint32_t);
extern "C" bool StormMetalApplyGraphicsSettings(void *, uint32_t, uint32_t);
#endif

#include "entity.h"
#include "math_inlines.h"
#include "s_import_func.h"
#include "texture.h"
#include "texture_mip_layout.hpp"
#include "v_s_stack.h"
#include "fs.h"
#include "debug-trap.h"
#include "string_compare.hpp"

#include <algorithm>
#include <SDL_timer.h>

#include <fmt/chrono.h>

#ifdef _WIN32
#include <DxErr.h>
#include <corecrt_io.h>
#else
#include <unistd.h>
#endif

CREATE_SERVICE(DX9RENDER)

CREATE_SCRIPTLIBRIARY(DX9RENDER_SCRIPT_LIBRIARY)

#define POST_PROCESS_FVF (storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::tex4)

#define S_RELEASE(a, b)                                                                                                \
    if (a)                                                                                                             \
    {                                                                                                                  \
        ULONG refc = a->Release();                                                                                     \
        a = NULL;                                                                                                      \
    }

namespace
{
constexpr auto kKeyTakeScreenshot = "TakeScreenshot";

#ifdef _WIN32 // Screenshot
D3DXIMAGE_FILEFORMAT GetScreenshotFormat(const std::string &fmt)
{
    if (fmt == "bmp")
    {
        return D3DXIFF_BMP;
    }
    if (fmt == "jpg")
    {
        return D3DXIFF_JPG;
    }
    if (fmt == "tga")
    {
        return D3DXIFF_TGA;
    }
    if (fmt == "png")
    {
        return D3DXIFF_PNG;
    }
    if (fmt == "dds")
    {
        return D3DXIFF_DDS;
    }
    if (fmt == "ppm")
    {
        return D3DXIFF_PPM;
    }
    if (fmt == "dib")
    {
        return D3DXIFF_DIB;
    }
    if (fmt == "hdr")
    {
        return D3DXIFF_HDR;
    }
    if (fmt == "pfm")
    {
        return D3DXIFF_PFM;
    }

    return D3DXIFF_FORCE_DWORD;
}
#endif

void InvokeEntitiesLostRender()
{
    core.ForEachEntity([](entptr_t entity_ptr) { entity_ptr->ProcessStage(Entity::Stage::lost_render); });
}
void InvokeEntitiesRestoreRender()
{
    core.ForEachEntity([](entptr_t entity_ptr) { entity_ptr->ProcessStage(Entity::Stage::restore_render); });
}

} // namespace

DX9RENDER *DX9RENDER::pRS = nullptr;

class LostDeviceSentinel : public SERVICE
{
    void RunStart() override
    {
        if (auto renderDevice = static_cast<storm_render::Device *>(DX9RENDER::pRS->GetNativeDevice()))
        {
            switch (renderDevice->GetStatus())
            {
            case storm_render::result::notAvailable:
                if (!DX9RENDER::pRS->ResetDevice())
                {
                    core.stopFrameProcessing();
                }
                break;

            case storm_render::result::failure:
                core.stopFrameProcessing();
                break;
            }
        }

    }
};
CREATE_SERVICE(LostDeviceSentinel)

uint32_t DX9SetTexturePath(VS_STACK *pS)
{
    auto *pString = (VDATA *)pS->Pop();
    auto *pNumber = (VDATA *)pS->Pop();

    const uintptr_t iNumber = pNumber->GetInt();
    auto *const pStr = pString->GetString();

    if (!DX9RENDER::pRS)
    {
        core.GetService("dx9render");
        Assert(DX9RENDER::pRS);
    }

    auto *pVR = (VDATA *)pS->Push();
    if (!pVR || iNumber < 0 || iNumber >= 4)
    {
        pVR->Set(0);
        return IFUNCRESULT_OK;
    }

    DX9RENDER::pRS->TextureCreate((const char *)-1);
    DX9RENDER::pRS->TextureCreate((const char *)iNumber);
    DX9RENDER::pRS->TextureCreate(static_cast<const char *>(pStr));

    pVR->Set(1);

    return IFUNCRESULT_OK;
}

uint32_t RPrint(VS_STACK *pS)
{
    auto *pString = (VDATA *)pS->Pop();
    const int32_t y = ((VDATA *)pS->Pop())->GetInt();
    const int32_t x = ((VDATA *)pS->Pop())->GetInt();

    if (pString->GetString())
        DX9RENDER::pRS->Print(x, y, pString->GetString());
    auto *pVR = (VDATA *)pS->Push();
    pVR->Set(0);
    return IFUNCRESULT_OK;
}

uint32_t SetGlowParams(VS_STACK *pS)
{
    const float fBlurBrushSize = ((VDATA *)pS->Pop())->GetFloat();
    const int32_t Intensivity = ((VDATA *)pS->Pop())->GetInt();
    const int32_t BlurPasses = ((VDATA *)pS->Pop())->GetInt();

    DX9RENDER::pRS->SetGLOWParams(fBlurBrushSize, Intensivity, BlurPasses);

    auto *pVR = (VDATA *)pS->Push();
    pVR->Set(0);
    return IFUNCRESULT_OK;
}

uint32_t slGetTexture(VS_STACK *pS)
{
    auto filename = ((VDATA *)pS->Pop())->GetString();
    int32_t texNum = DX9RENDER::pRS->TextureCreate(filename);

    auto *pVR = (VDATA *)pS->Push();
    pVR->Set(texNum);
    return IFUNCRESULT_OK;
}

uint32_t slReleaseTexture(VS_STACK *pS)
{
    int32_t texNum = ((VDATA *)pS->Pop())->GetInt();

    DX9RENDER::pRS->TextureRelease(texNum);
    return IFUNCRESULT_OK;
}

bool DX9RENDER_SCRIPT_LIBRIARY::Init()
{
    IFUNCINFO sIFuncInfo;

    sIFuncInfo.nArguments = 2;
    sIFuncInfo.pFuncName = "SetTexturePath";
    sIFuncInfo.pReturnValueName = "int";
    sIFuncInfo.pFuncAddress = DX9SetTexturePath;
    core.SetScriptFunction(&sIFuncInfo);

    sIFuncInfo.nArguments = 3;
    sIFuncInfo.pFuncName = "RPrint";
    sIFuncInfo.pReturnValueName = "int";
    sIFuncInfo.pFuncAddress = RPrint;
    core.SetScriptFunction(&sIFuncInfo);

    sIFuncInfo.nArguments = 3;
    sIFuncInfo.pFuncName = "SetGlowParams";
    sIFuncInfo.pReturnValueName = "int";
    sIFuncInfo.pFuncAddress = SetGlowParams;
    core.SetScriptFunction(&sIFuncInfo);

    sIFuncInfo.nArguments = 1;
    sIFuncInfo.pFuncName = "GetTexture";
    sIFuncInfo.pReturnValueName = "int";
    sIFuncInfo.pFuncAddress = slGetTexture;
    core.SetScriptFunction(&sIFuncInfo);

    sIFuncInfo.nArguments = 1;
    sIFuncInfo.pFuncName = "ReleaseTexture";
    sIFuncInfo.pReturnValueName = "void";
    sIFuncInfo.pFuncAddress = slReleaseTexture;
    core.SetScriptFunction(&sIFuncInfo);

    return true;
}

uint64_t renderTimestamp;
uint32_t dwTotalSize = 0;
uint32_t dwSplashTime = 0;
bool bSplash = false;
#pragma warning(push)
#pragma warning(disable : 4305 4309)
char sSplashText[] = {'\xbb', '\x9a', '\x89', '\x9a', '\x93', '\x90', '\x8f', '\x9a', '\x8d', '\x8c', '\xdf',
                      '\xbc', '\x90', '\x8f', '\x86', '\xd1', '\xdf', '\xb1', '\x90', '\x8b', '\xdf', '\x99',
                      '\x90', '\x8d', '\xdf', '\xac', '\x9e', '\x93', '\x9a', '\xd1', '\0'};
#pragma warning(pop)
char splashbuffer[256];

#define VIDEODIR "Resource\\Videos\\%s"

struct DX9SphVertex
{
    CVECTOR v;
    uint32_t c;
};

uint32_t DX9sphereNumTrgs;
DX9SphVertex *DX9sphereVertex = nullptr;

void CreateSphere()
{
#define CalcKColor(ind)                                                                                                \
    {                                                                                                                  \
        kColor = light | !CVECTOR(DX9sphereVertex[t * 3 + ind].v.x, DX9sphereVertex[t * 3 + ind].v.y,                  \
                                  DX9sphereVertex[t * 3 + ind].v.z);                                                   \
        if (kColor < 0.0f)                                                                                             \
            kColor = 0.0f;                                                                                             \
    }
#define CLerp(c, min) (uint32_t(c * (kColor * (1.0f - min) + min)))
#define Color1                                                                                                         \
    ((CLerp(255.0f, 0.5f) << 24) | (CLerp(255.0f, 0.7f) << 16) | (CLerp(255.0f, 0.7f) << 8) |                          \
     (CLerp(255.0f, 0.7f) << 0));

    if (DX9sphereVertex)
        return;

    const float myPI = 3.1415926535897932f;
    const int32_t a1 = 32;
    const int32_t a2 = (a1 / 2);

    DX9sphereNumTrgs = a1 * a2 * 2;
    DX9sphereVertex = new DX9SphVertex[DX9sphereNumTrgs * 6];

    const CVECTOR light = !CVECTOR(0.0f, 0.0f, 1.0f);
    float kColor;
    // fill in the vertices
    for (int32_t i = 0, t = 0; i < a2; i++)
    {
        const float r1 = sinf(myPI * i / static_cast<float>(a2));
        const float y1 = cosf(myPI * i / static_cast<float>(a2));
        const float r2 = sinf(myPI * (i + 1) / static_cast<float>(a2));
        const float y2 = cosf(myPI * (i + 1) / static_cast<float>(a2));
        for (int32_t j = 0; j < a1; j++)
        {
            const float x1 = sinf(2.0f * myPI * j / static_cast<float>(a1));
            const float z1 = cosf(2.0f * myPI * j / static_cast<float>(a1));
            const float x2 = sinf(2.0f * myPI * (j + 1) / static_cast<float>(a1));
            const float z2 = cosf(2.0f * myPI * (j + 1) / static_cast<float>(a1));
            // 0
            DX9sphereVertex[t * 3 + 0].v.x = r1 * x1;
            DX9sphereVertex[t * 3 + 0].v.y = y1;
            DX9sphereVertex[t * 3 + 0].v.z = r1 * z1;
            CalcKColor(0);
            DX9sphereVertex[t * 3 + 0].c = Color1;
            // 1
            DX9sphereVertex[t * 3 + 1].v.x = r2 * x1;
            DX9sphereVertex[t * 3 + 1].v.y = y2;
            DX9sphereVertex[t * 3 + 1].v.z = r2 * z1;
            CalcKColor(1);
            DX9sphereVertex[t * 3 + 1].c = Color1;
            // 2
            DX9sphereVertex[t * 3 + 2].v.x = r1 * x2;
            DX9sphereVertex[t * 3 + 2].v.y = y1;
            DX9sphereVertex[t * 3 + 2].v.z = r1 * z2;
            CalcKColor(2);
            DX9sphereVertex[t * 3 + 2].c = Color1;
            // 3 = 2
            DX9sphereVertex[t * 3 + 3] = DX9sphereVertex[t * 3 + 2];
            // 4 = 1
            DX9sphereVertex[t * 3 + 4] = DX9sphereVertex[t * 3 + 1];
            // 5
            DX9sphereVertex[t * 3 + 5].v.x = r2 * x2;
            DX9sphereVertex[t * 3 + 5].v.y = y2;
            DX9sphereVertex[t * 3 + 5].v.z = r2 * z2;
            CalcKColor(5);
            DX9sphereVertex[t * 3 + 5].c = Color1;
            // added 2 triangles
            t += 2;
        }
    }
}

struct SD_TEXTURE_FORMAT
{
    TX_FORMAT txFormat;
    storm_render::AssetFormat d3dFormat;
    bool isSwizzled;
    const char *format;
};

SD_TEXTURE_FORMAT textureFormats[] = {
    {TXF_DXT1, storm_render::AssetFormat::DXT1, true, "D3DFMT_DXT1"},
    {TXF_DXT3, storm_render::AssetFormat::DXT3, true, "D3DFMT_DXT3"},
    {TXF_DXT5, storm_render::AssetFormat::DXT5, true, "D3DFMT_DXT5"},
    {TXF_A8R8G8B8, storm_render::AssetFormat::A8R8G8B8, false, "D3DFMT_A8R8G8B8"},
    {TXF_X8R8G8B8, storm_render::AssetFormat::X8R8G8B8, false, "D3DFMT_X8R8G8B8"},
    {TXF_R5G6B5, storm_render::AssetFormat::R5G6B5, false, "D3DFMT_R5G6B5"},
    {TXF_A4R4G4B4, storm_render::AssetFormat::A4R4G4B4, false, "D3DFMT_A4R4G4B4"},
    {TXF_A1R5G5B5, storm_render::AssetFormat::A1R5G5B5, false, "D3DFMT_A1R5G5B5"},
};

uint32_t dwSoundBuffersCount = 0;
uint32_t dwSoundBytes = 0;
uint32_t dwSoundBytesCached = 0;

//################################################################################
//~!~
/*#define STRIPPATH(s)\
    (sizeof(s) > 11 && (s)[sizeof(s)-12] == '/' ? (s) + sizeof(s) - 11 : \
    sizeof(s) > 10 && (s)[sizeof(s)-11] == '/' ? (s) + sizeof(s) - 10 : \
    sizeof(s) > 9 && (s)[sizeof(s)-10] == '/' ? (s) + sizeof(s) - 9 : \
    sizeof(s) > 8 && (s)[sizeof(s)-9] == '/' ? (s) + sizeof(s) - 8 : \
    sizeof(s) > 7 && (s)[sizeof(s)-8] == '/' ? (s) + sizeof(s) - 7 : \
    sizeof(s) > 6 && (s)[sizeof(s)-7] == '/' ? (s) + sizeof(s) - 6 : \
    sizeof(s) > 5 && (s)[sizeof(s)-6] == '/' ? (s) + sizeof(s) - 5 : \
    sizeof(s) > 4 && (s)[sizeof(s)-5] == '/' ? (s) + sizeof(s) - 4 : \
    sizeof(s) > 3 && (s)[sizeof(s)-4] == '/' ? (s) + sizeof(s) - 3 : \
    sizeof(s) > 2 && (s)[sizeof(s)-3] == '/' ? (s) + sizeof(s) - 2 : (s))*/

#define CHECKD3DERR(expr) ErrorHandler(expr, __FILE__, __LINE__, __func__, #expr)

inline bool ErrorHandler(storm_render::Result hr, const char *file, unsigned line, const char *func, const char *expr)
{
    if (hr != storm_render::result::ok)
    {
#ifdef _WIN32
        core.Trace("[%s:%s:%d] %s: %s (%s)", file, func, line, DXGetErrorStringA(hr), DXGetErrorDescriptionA(hr), expr);
#else
        core.Trace("[%s:%s:%d] (%s)", file, func, line, expr);
#endif
        return true;
    }

    return false;
}

const uint32_t DX9RENDER::rectsVBuffer_SizeInRects = 512;

//################################################################################
DX9RENDER::DX9RENDER()
{
    rectsVBuffer = nullptr;

    bPostProcessEnabled = true;
    bPostProcessError = false;
    iBlurPasses = 4;
    GlowIntensity = 200;
    fBlurSize = 0.8f;

    pPostProcessTexture = nullptr;
    pSmallPostProcessTexture = nullptr;
    pSmallPostProcessTexture2 = nullptr;

    pPostProcessSurface = nullptr;
    pSmallPostProcessSurface = nullptr;
    pSmallPostProcessSurface2 = nullptr;

    pOriginalScreenSurface = nullptr;
    pOriginalDepthSurface = nullptr;

    pRS = this;
    renderDevice = nullptr;
    aniVBuffer = nullptr;
    numAniVerteces = 0;
    pVTL = nullptr;
    nFontQuantity = 0;
    idFontCurrent = 0;
    fontIniFileName = nullptr;

    bLoadTextureEnabled = true;

    bSeaEffect = false;
    fSeaEffectSize = 0.0f;
    fSeaEffectSpeed = 0.0f;
    dwBackColor = 0x0;

    bTrace = true;
    iSetupPath = 0;

    bDropVideoConveyor = false;
    pDropConveyorVBuffer = nullptr;

    aspectRatio = -1.0f;

    bMakeShoot = false;
    bShowFps = false;
    bShowExInfo = false;
    bInsideScene = false;

    m_fHeightDeformator = 1.f;

    progressImage = nullptr;
    progressImageSize = 0;
    progressTexture = -1;
    progressBackImage = nullptr;
    progressBackImageSize = 0;
    progressTipsImage = nullptr;
    progressTipsImageSize = 0;
    progressTipsTexture = -1;
    loadFrame = 0;
    backTexture = -1;
    back0Texture = -1;
    progressSafeCounter = 0;
    isInPViewProcess = false;
    progressUpdateTime = 0;
    progressFramesPosX = 0.85f;
    progressFramesPosY = 0.8f;
    progressFramesWidth = 64;
    progressFramesHeight = 64;
    progressFramesCountX = 8;
    progressFramesCountY = 8;

    vViewRelationPos = CVECTOR(0.f, 0.f, 0.f);
    vWordRelationPos = -vViewRelationPos;
    bUseLargeBackBuffer = false;
	
	bNewFovCalculation = true;	// evganat - по умолчанию правильный vfov
}

static bool texLog = false;
static float fSin = 0.0f;

bool DX9RENDER::Init()
{
    if (auto *sentinelService = core.GetService("LostDeviceSentinel"); !sentinelService)
    {
        throw std::runtime_error("Cannot create LostDeviceSentinel! Abort");
    }

    char str[256];
    for (int32_t i = 0; i < MAX_STEXTURES; i++)
        Textures[i].ref = NULL;

    renderDevice = nullptr;

    create_directories(fs::GetScreenshotsPath());

    auto ini = fio->OpenIniFile(core.EngineIniFileName());
    if (ini)
    {
        // bPostProcessEnabled = ini->GetInt(0, "PostProcess", 0) == 1;
        bPostProcessEnabled = false; //~!~

        // screenshot format and extension
        ini->ReadString(nullptr, "screenshot_format", str, sizeof(str), "jpg");
        screenshotExt = str;
        std::ranges::transform(screenshotExt, screenshotExt.begin(),
                               [](const unsigned char c) { return std::tolower(c); });
#ifdef _WIN32 // Screenshot
        screenshotFormat = GetScreenshotFormat(str);
        if (screenshotFormat == D3DXIFF_FORCE_DWORD)
        {
            screenshotExt = "jpg";
            screenshotFormat = D3DXIFF_JPG;
        }
#endif

        bShowFps = ini->GetInt(nullptr, "show_fps", 0) == 1;
        bShowExInfo = ini->GetInt(nullptr, "show_exinfo", 0) == 1;
        bSafeRendering = ini->GetInt(nullptr, "safe_render", 0) == 0;
        bDropVideoConveyor = ini->GetInt(nullptr, "DropVideoConveyor", 0) != 0;
        texLog = ini->GetInt(nullptr, "texture_log", 0) == 1;
        bUseLargeBackBuffer = ini->GetInt(nullptr, "UseLargeBackBuffer", 0) != 0;
		
		bNewFovCalculation = ini->GetInt(nullptr, "NewFovCalculation", 0) == 1;	// evganat - выбор формулы из engine.ini

        bWindow = ini->GetInt(nullptr, "full_screen", 1) == 0;

        nTextureDegradation = ini->GetInt(nullptr, "texture_degradation", 0);

        FovMultiplier = ini->GetFloat(nullptr, "fov_multiplier", 1.0f);

        screen_size.x = ini->GetInt(nullptr, "screen_x", 1024);
        screen_size.y = ini->GetInt(nullptr, "screen_y", 768);
        const auto displayMode = ini->GetInt(nullptr, "display_mode", 0);
        // Desktop/native is resolved after SDL creates the window. Query the
        // display mode rather than the Metal drawable: the renderer owns and
        // resizes that drawable, so it cannot describe the display itself.
        // Keep the saved dimensions as a deterministic fallback when the
        // display query is unavailable or returns an invalid size.
        desktopMode = !bWindow && displayMode == 1;
        if (desktopMode)
        {
            const auto desktop = core.GetWindow()->GetDesktopSize();
            if (desktop.width > 0 && desktop.height > 0)
                screen_size = {desktop.width, desktop.height};
        }
        fNearClipPlane = ini->GetFloat(nullptr, "NearClipPlane", 0.1f);
        fFarClipPlane = ini->GetFloat(nullptr, "FarClipPlane", 4000.0f);
        bBackBufferCanLock = ini->GetInt(nullptr, "lockable_back_buffer", 0) != 0;
        ini->ReadString(nullptr, "screen_bpp", str, sizeof(str), "D3DFMT_R5G6B5");
        screen_bpp = storm_render::AssetFormat::R5G6B5;
        stencil_format = storm_render::AssetFormat::D16;
        if (storm::iEquals(str, "D3DFMT_A8R8G8B8"))
        {
            screen_bpp = storm_render::AssetFormat::A8R8G8B8;
            stencil_format = storm_render::AssetFormat::D24S8;
        }
        if (storm::iEquals(str, "D3DFMT_X8R8G8B8"))
        {
            screen_bpp = storm_render::AssetFormat::X8R8G8B8;
            stencil_format = storm_render::AssetFormat::D24S8;
        }
        if (storm::iEquals(str, "D3DFMT_R5G6B5"))
        {
            screen_bpp = storm_render::AssetFormat::R5G6B5;
            stencil_format = storm_render::AssetFormat::D16;
        }

        // new renderer settings
        vSyncEnabled = ini->GetInt(nullptr, "vsync", 0);

        // stencil_format = D3DFMT_D24S8;
        if (!InitDevice(bWindow, static_cast<HWND>(core.GetWindow()->OSHandle()), screen_size.x, screen_size.y))
            return false;

#ifdef _WIN32 // Effects
        RecompileEffects();
#else
        pTechnique = std::make_unique<CTechnique>(this);
        pTechnique->DecodeFiles();
#endif

        // get start ini file for fonts
        if (!ini->ReadString(nullptr, "startFontIniFile", str, sizeof(str) - 1, ""))
        {
            core.Trace("Not finded 'startFontIniFile' parameter into ENGINE.INI file");
            sprintf_s(str, "resource\\ini\\fonts.ini");
        }
        const auto len = strlen(str) + 1;
        if ((fontIniFileName = new char[len]) == nullptr)
            throw std::runtime_error("allocate memory error");
        strcpy_s(fontIniFileName, len, str);
        // get start font quantity
        if (!ini->ReadString(nullptr, "font", str, sizeof(str) - 1, ""))
        {
            core.Trace("Start font not defined");
            sprintf_s(str, "normal");
        }
        if (LoadFont(str) == -1L)
            core.Trace("can not init start font: %s", str);
        idFontCurrent = 0L;

        // Progress image parameters
        progressFramesPosX = ini->GetFloat("ProgressImage", "RelativePosX", 0.85f);
        progressFramesPosY = ini->GetFloat("ProgressImage", "RelativePosY", 0.8f);
        progressFramesWidth = ini->GetFloat("ProgressImage", "RelativeWidth", 0.0625f);
        if (progressFramesWidth < 0.0f)
            progressFramesWidth = 0.0f;
        if (progressFramesWidth > 10.0f)
            progressFramesWidth = 10.0f;
        progressFramesHeight = ini->GetFloat("ProgressImage", "RelativeHeight", 0.0625f);
        if (progressFramesHeight < 0.0f)
            progressFramesHeight = 0.0f;
        if (progressFramesHeight > 10.0f)
            progressFramesHeight = 10.0f;
        progressFramesCountX = static_cast<int32_t>(ini->GetFloat("ProgressImage", "HorisontalFramesCount", 8));
        if (progressFramesCountX < 1)
            progressFramesCountX = 1;
        if (progressFramesCountX > 64)
            progressFramesCountX = 64;
        progressFramesCountY = static_cast<int32_t>(ini->GetFloat("ProgressImage", "VerticalFramesCount", 8));
        if (progressFramesCountY < 1)
            progressFramesCountY = 1;
        if (progressFramesCountY > 64)
            progressFramesCountY = 64;

        CreateSphere();
        auto *pScriptRender = static_cast<VDATA *>(core.GetScriptVariable("Render"));
        ATTRIBUTES *pARender = pScriptRender->GetAClass();

        pARender->SetAttributeUseDword("full_screen", !bWindow);
        pARender->SetAttributeUseDword("screen_x", screen_size.x);
        pARender->SetAttributeUseDword("screen_y", screen_size.y);
    }

    pDropConveyorVBuffer = nullptr;
    rectsVBuffer = nullptr;
    renderDevice->CreateVertexBuffer(2 * sizeof(CVECTOR), storm_render::usage::writeOnly, storm_render::vertexFormat::xyz, &pDropConveyorVBuffer);
    renderDevice->CreateVertexBuffer(rectsVBuffer_SizeInRects * 6 * sizeof(RECT_VERTEX), storm_render::usage::writeOnly | storm_render::usage::dynamic, RS_RECT_VERTEX_FORMAT, &rectsVBuffer); // D3DPOOL_MANAGED
    if (!pDropConveyorVBuffer || !rectsVBuffer)
    {
        return false;
    }

    uint16_t *pI = &qi[0];
    // setup ibuffer
    for (int32_t y = 0; y < 31; y++)
    {
        for (int32_t x = 0; x < 31; x++)
        {
            *pI++ = static_cast<uint16_t>((y + 0) * 32 + x + 1);
            *pI++ = static_cast<uint16_t>((y + 1) * 32 + x);
            *pI++ = static_cast<uint16_t>((y + 0) * 32 + x);

            *pI++ = static_cast<uint16_t>((y + 0) * 32 + x + 1);
            *pI++ = static_cast<uint16_t>((y + 1) * 32 + x + 1);
            *pI++ = static_cast<uint16_t>((y + 1) * 32 + x);
        }
    }

    const auto ctrl = core.Controls->CreateControl(kKeyTakeScreenshot);
    core.Controls->MapControl(ctrl, VK_F8);

    // UNGUARD
    return true;
}

//################################################################################
DX9RENDER::~DX9RENDER()
{
    // renderDevice->SetRenderTarget(NULL, NULL); //~!~

    S_RELEASE(pSmallPostProcessSurface2, 0);
    S_RELEASE(pPostProcessSurface, 1);
    S_RELEASE(pSmallPostProcessSurface, 2);

    S_RELEASE(pOriginalScreenSurface, 3);
    S_RELEASE(pOriginalDepthSurface, 4);

    S_RELEASE(pSmallPostProcessTexture2, 5);
    S_RELEASE(pPostProcessTexture, 6);
    S_RELEASE(pSmallPostProcessTexture, 7);

    S_RELEASE(rectsVBuffer, 8);

    delete progressImage;
    progressImage = nullptr;
    delete progressBackImage;
    progressBackImage = nullptr;
    delete progressTipsImage;
    progressTipsImage = nullptr;

    for (int i = 0; i < nFontQuantity; i++)
    {
        delete FontList[i].font;

        delete FontList[i].name;
    }
    nFontQuantity = 0;
    delete fontIniFileName;

    STORM_DELETE(DX9sphereVertex);
    ReleaseDevice();
}

bool DX9RENDER::InitDevice(bool windowed, HWND _hwnd, int32_t width, int32_t height)
{
    // GUARD(DX9RENDER::InitDevice)

    aniVBuffer = nullptr;
    numAniVerteces = 0;

    screen_size.x = width;
    screen_size.y = height;
    bWindow = windowed;

    hwnd = _hwnd;
    core.Trace("Initializing native Metal renderer");
    d3dpp = {};
    d3dpp.width = width;
    d3dpp.height = height;
    d3dpp.colorFormat = screen_bpp;
    d3dpp.depthEnabled = true;
    d3dpp.depthFormat = stencil_format;
    d3dpp.vSync = vSyncEnabled;
    if (CHECKD3DERR(storm_render::createMetalDevice(hwnd, &d3dpp, &renderDevice)))
        return false;

    // Create render targets for POST PROCESS effects
    renderDevice->GetRenderTarget(&pOriginalScreenSurface);
    renderDevice->GetDepthStencilSurface(&pOriginalDepthSurface);

    fSmallWidth = 128;
    fSmallHeight = 128;

    if (bPostProcessEnabled)
    {
        renderDevice->CreateTexture(width, height, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, &pPostProcessTexture);
        renderDevice->CreateTexture(static_cast<int>(fSmallWidth), static_cast<int>(fSmallHeight), 1, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, &pSmallPostProcessTexture);
        renderDevice->CreateTexture(static_cast<int>(fSmallWidth * 2.0f), static_cast<int>(fSmallHeight * 2.0f), 1, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, &pSmallPostProcessTexture2);
    }

    if (!pPostProcessTexture || !pSmallPostProcessTexture || !pSmallPostProcessTexture2)
    {
        bPostProcessEnabled = false;
        bPostProcessError = true;
    }

    if (!bPostProcessError)
    {
        pPostProcessTexture->GetSurfaceLevel(0, &pPostProcessSurface);
        pSmallPostProcessTexture2->GetSurfaceLevel(0, &pSmallPostProcessSurface2);
        pSmallPostProcessTexture->GetSurfaceLevel(0, &pSmallPostProcessSurface);

        if (!pPostProcessSurface || !pSmallPostProcessSurface2 || !pSmallPostProcessSurface)
        {
            bPostProcessEnabled = false;
            bPostProcessError = true;
        }
    }

    if (!bPostProcessError)
    {
        ClearPostProcessSurface(pPostProcessSurface);
        ClearPostProcessSurface(pSmallPostProcessSurface);
        ClearPostProcessSurface(pSmallPostProcessSurface2);
    }

    ClearPostProcessSurface(pOriginalScreenSurface);

    OriginalViewPort.X = 0;
    OriginalViewPort.Y = 0;
    OriginalViewPort.Width = width;
    OriginalViewPort.Height = height;
    OriginalViewPort.MinZ = 0.0f;
    OriginalViewPort.MaxZ = 1.0f;

    // if(CHECKD3DERR(SetViewport(&vprt))==true)    return false;

    // set it as a render target
    // FIX
    // SetPostProcessTextureAsRenderTarget();
    // set it as a render target

    for (int32_t b = 0; b < MAX_BUFFERS; b++)
    {
        VertexBuffers[b].buff = nullptr;
        IndexBuffers[b].buff = nullptr;
    }

    int32_t num_stages;
    num_stages = 8;

    for (int32_t s = 0; s < num_stages; s++)
    {
        // texture operation
        SetTextureStageState(s, storm_render::StageColorArg1, storm_render::textureArg::current);
        SetTextureStageState(s, storm_render::StageColorArg2, storm_render::textureArg::texture);
        SetTextureStageState(s, storm_render::StageColorOp, storm_render::TextureOpDisable);

        // texture coord
        SetTextureStageState(s, storm_render::StageTexCoordIndex, s);

        // texture filtering
        SetSamplerState(s, storm_render::SamplerMagFilter, storm_render::TextureFilterLinear);
        SetSamplerState(s, storm_render::SamplerMinFilter, storm_render::TextureFilterLinear);
        SetSamplerState(s, storm_render::SamplerMipFilter, storm_render::TextureFilterLinear);
    }

    // set base texture and diffuse+specular lighting
    SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpAddSigned);

    // texture op for lightmaps
    // SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_ADDSIGNED);

    SetCamera(CVECTOR(0.0f, 0.0f, 0.0f), CVECTOR(0.0f, 0.0f, 0.0f), 1.0f);

    storm_render::Light l{};
    l.Type = storm_render::LightKind::Point;
    l.Range = 100.0f;
    l.Attenuation0 = 1.0f;

    for (int i = 0; i < 8; i++)
    {
        renderDevice->SetLight(i, &l);
        renderDevice->LightEnable(i, false);
    }

    SetCommonStates();

    screen_size.x = width;
    screen_size.y = height;

    m_fHeightDeformator = (float)(height * 4.0f) / (float)(width * 3.0f);

    // UNGUARD
    return true;
}

//################################################################################
bool DX9RENDER::ReleaseDevice()
{
    if (aniVBuffer)
        aniVBuffer->Release();
    aniVBuffer = nullptr;
    numAniVerteces = 0;
    for (int32_t b = 0; b < MAX_BUFFERS; b++)
    {
        if (VertexBuffers[b].buff)
            VertexBuffers[b].buff->Release();
        VertexBuffers[b].buff = nullptr;
        if (IndexBuffers[b].buff)
            IndexBuffers[b].buff->Release();
        IndexBuffers[b].buff = nullptr;
    }

    bool res = true;
    for (int32_t t = 0; t < MAX_STEXTURES; t++)
        if (Textures[t].ref && Textures[t].loaded && Textures[t].d3dtex)
        {
            if (CHECKD3DERR(Textures[t].d3dtex->Release()) == false)
                res = false;
            Textures[t].ref = NULL;
            delete Textures[t].name;
        }

    if (renderDevice != nullptr && CHECKD3DERR(renderDevice->Release()) == false)
        res = false;
    renderDevice = nullptr;
    return res;
}

//################################################################################
bool DX9RENDER::DX9Clear(int32_t type)
{
    if (CHECKD3DERR(renderDevice->Clear(type, dwBackColor, 1.0f)) == true)
        return false;
    // if(CHECKD3DERR(renderDevice->Clear(0L, NULL, type, 0x0, 1.0f, 0L))==true)    return false;
    return true;
}

//################################################################################
bool DX9RENDER::DX9BeginScene()
{
    //~!~
    return false;
}

void DX9RENDER::CreateRenderQuad(float fWidth, float fHeight, float fSrcWidth, float fSrcHeight, float fMulU,
                                 float fMulV)
{
    const float StartX = -0.5f;
    const float StartY = -0.5f;
    fWidth -= 0.5f;
    fHeight -= 0.5f;
    PostProcessQuad[0].vPos = Vector4(StartX, fHeight, 0.0f, 1.0f);
    PostProcessQuad[1].vPos = Vector4(StartX, StartY, 0.0f, 1.0f);
    PostProcessQuad[2].vPos = Vector4(fWidth, fHeight, 0.0f, 1.0f);
    PostProcessQuad[3].vPos = Vector4(fWidth, StartY, 0.0f, 1.0f);

    const float fTexelU = 1.0f / fSrcWidth;
    const float fTexelV = 1.0f / fSrcHeight;

    const float fNearV = fTexelV * 0.5f;
    const float fFarV = 1.0f - (fTexelV * 0.5f);

    const float fNearU = fTexelU * 0.5f;
    const float fFarU = 1.0f - (fTexelU * 0.5f);

    PostProcessQuad[0].v0 = fFarV;
    PostProcessQuad[0].u0 = fNearU;
    PostProcessQuad[1].v0 = fNearV;
    PostProcessQuad[1].u0 = fNearU;
    PostProcessQuad[2].v0 = fFarV;
    PostProcessQuad[2].u0 = fFarU;
    PostProcessQuad[3].v0 = fNearV;
    PostProcessQuad[3].u0 = fFarU;

    for (int i = 0; i < 4; i++)
    {
        float u = PostProcessQuad[i].u0;
        float v = PostProcessQuad[i].v0;

        u = u * fMulU;
        v = v * fMulV;

        PostProcessQuad[i].u0 = u - (fTexelU * fBlurSize);
        PostProcessQuad[i].v0 = v - (fTexelV * fBlurSize);

        PostProcessQuad[i].u1 = u + (fTexelU * fBlurSize);
        PostProcessQuad[i].v1 = v - (fTexelV * fBlurSize);

        PostProcessQuad[i].u2 = u - (fTexelU * fBlurSize);
        PostProcessQuad[i].v2 = v + (fTexelV * fBlurSize);

        PostProcessQuad[i].u3 = u + (fTexelU * fBlurSize);
        PostProcessQuad[i].v3 = v + (fTexelV * fBlurSize);
    }
}

void DX9RENDER::BlurGlowTexture()
{
    // Render everything to a small texture
    CreateRenderQuad(fSmallWidth * 2.0f, fSmallHeight * 2.0f, 1024.0f, 1024.0f);
    SetTexture(0, pPostProcessTexture);
    SetTexture(1, pPostProcessTexture);
    SetTexture(2, pPostProcessTexture);
    SetTexture(3, pPostProcessTexture);
    SetRenderTarget(pSmallPostProcessSurface2, nullptr);
    DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, POST_PROCESS_FVF, 2, PostProcessQuad, sizeof(QuadVertex), "PostProcessBlur");

    // pre-blur iBlurPasses times
    for (int i = 0; i < iBlurPasses; i++)
    {
        CreateRenderQuad(fSmallWidth, fSmallHeight, fSmallWidth * 2.0f, fSmallHeight * 2.0f);

        SetTexture(0, pSmallPostProcessTexture2);
        SetTexture(1, pSmallPostProcessTexture2);
        SetTexture(2, pSmallPostProcessTexture2);
        SetTexture(3, pSmallPostProcessTexture2);
        SetRenderTarget(pSmallPostProcessSurface, nullptr);
        DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, POST_PROCESS_FVF, 2, PostProcessQuad, sizeof(QuadVertex),
                        "PostProcessBlur");

        CreateRenderQuad(fSmallWidth * 2.0f, fSmallHeight * 2.0f, fSmallWidth, fSmallHeight);

        SetTexture(0, pSmallPostProcessTexture);
        SetTexture(1, pSmallPostProcessTexture);
        SetTexture(2, pSmallPostProcessTexture);
        SetTexture(3, pSmallPostProcessTexture);
        SetRenderTarget(pSmallPostProcessSurface2, nullptr);
        DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, POST_PROCESS_FVF, 2, PostProcessQuad, sizeof(QuadVertex),
                        "PostProcessBlur");
    }

    CreateRenderQuad(fSmallWidth, fSmallHeight, fSmallWidth * 2.0f, fSmallHeight * 2.0f);
    SetTexture(0, pSmallPostProcessTexture2);
    SetTexture(1, pSmallPostProcessTexture2);
    SetTexture(2, pSmallPostProcessTexture2);
    SetTexture(3, pSmallPostProcessTexture2);
    SetRenderTarget(pSmallPostProcessSurface, nullptr);
    DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, POST_PROCESS_FVF, 2, PostProcessQuad, sizeof(QuadVertex), "PostProcessBlur");
}

void DX9RENDER::CopyGlowToScreen()
{
    const auto sx = static_cast<float>(screen_size.x);
    const auto sy = static_cast<float>(screen_size.y);
    // Render to screen
    PostProcessQuad[0].vPos = Vector4(0, sy, 0.0f, 1.0f);
    PostProcessQuad[1].vPos = Vector4(0, 0, 0.0f, 1.0f);
    PostProcessQuad[2].vPos = Vector4(sx, sy, 0.0f, 1.0f);
    PostProcessQuad[3].vPos = Vector4(sx, 0, 0.0f, 1.0f);

    PostProcessQuad[0].v0 = 1.0f;
    PostProcessQuad[0].u0 = 0.0f;
    PostProcessQuad[1].v0 = 0.0f;
    PostProcessQuad[1].u0 = 0.0f;
    PostProcessQuad[2].v0 = 1.0f;
    PostProcessQuad[2].u0 = 1.0f;
    PostProcessQuad[3].v0 = 0.0f;
    PostProcessQuad[3].u0 = 1.0f;

    SetRenderTarget(pOriginalScreenSurface, pOriginalDepthSurface);

    if (GlowIntensity < 0)
        GlowIntensity = 0;
    if (GlowIntensity > 255)
        GlowIntensity = 255;
    const uint8_t bGLOW = static_cast<uint8_t>(GlowIntensity);
    const uint32_t dwTFactor = (bGLOW << 24) | (bGLOW << 16) | (bGLOW << 8) | bGLOW;

    // Draw the GLOW screen
    SetRenderState(storm_render::RenderTextureFactor, dwTFactor);

    SetTexture(0, pSmallPostProcessTexture);
    SetTexture(1, pSmallPostProcessTexture);
    SetTexture(2, pSmallPostProcessTexture);
    SetTexture(3, pSmallPostProcessTexture);

    DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, POST_PROCESS_FVF, 2, PostProcessQuad, sizeof(QuadVertex), "PostProcessGlow");
}

void DX9RENDER::CopyPostProcessToScreen()
{
    const auto sx = static_cast<float>(screen_size.x);
    const auto sy = static_cast<float>(screen_size.y);
    PostProcessQuad[0].vPos = Vector4(0, sy, 0.0f, 1.0f);
    PostProcessQuad[1].vPos = Vector4(0, 0, 0.0f, 1.0f);
    PostProcessQuad[2].vPos = Vector4(sx, sy, 0.0f, 1.0f);
    PostProcessQuad[3].vPos = Vector4(sx, 0, 0.0f, 1.0f);

    PostProcessQuad[0].v0 = 1.0f;
    PostProcessQuad[0].u0 = 0.0f;
    PostProcessQuad[1].v0 = 0.0f;
    PostProcessQuad[1].u0 = 0.0f;
    PostProcessQuad[2].v0 = 1.0f;
    PostProcessQuad[2].u0 = 1.0f;
    PostProcessQuad[3].v0 = 0.0f;
    PostProcessQuad[3].u0 = 1.0f;

    SetRenderTarget(pOriginalScreenSurface, pOriginalDepthSurface);

    // Draw the original screen
    SetTexture(0, pPostProcessTexture);
    SetTexture(1, pPostProcessTexture);
    SetTexture(2, pPostProcessTexture);
    SetTexture(3, pPostProcessTexture);

    if (bSeaEffect)
    {
        SetFVF(POST_PROCESS_FVF);
        DrawIndexedPrimitiveUP(storm_render::PrimitiveTriangleList, 0, 32 * 32, 31 * 31 * 2, qi, storm_render::IndexFormat::UInt16, qv, sizeof(QuadVertex),
                               "PostProcess");
    }
    else
    {
        DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, POST_PROCESS_FVF, 2, PostProcessQuad, sizeof(QuadVertex), "PostProcess");
    }
}

void DX9RENDER::ClearPostProcessSurface(storm_render::Surface *pSurf)
{
    storm_render::Result hr = SetRenderTarget(pSurf, nullptr);
    hr = BeginScene();
    hr = renderDevice->Clear(storm_render::clear::color, 0x0, 0.0f);
    hr = EndScene();
}

void DX9RENDER::SetScreenAsRenderTarget()
{
    SetRenderTarget(pOriginalScreenSurface, pOriginalDepthSurface);
    SetViewport(&OriginalViewPort);
}

void DX9RENDER::SetPostProcessTextureAsRenderTarget()
{
    SetRenderTarget(pPostProcessSurface, pOriginalDepthSurface);
    SetViewport(&OriginalViewPort);
}

void DX9RENDER::FinalizeSceneEffects()
{
    storm_render::markSceneBoundary(GetNativeDevice(), storm_render::SceneBoundary::Hud);
}



//################################################################################
bool DX9RENDER::DX9EndScene()
{
    {
        // Buffer-table growth probe: a slow leak in any subsystem shows up here as a rising count
        // long before the fixed table is exhausted. The scan runs once per 1024 frames.
        static uint32_t s_bufferScanFrame = 0;
        static int32_t s_vertexHighWater = 0;
        static int32_t s_indexHighWater = 0;
        if ((++s_bufferScanFrame & 1023u) == 0u)
        {
            int32_t nVertex = 0;
            int32_t nIndex = 0;
            for (int32_t t = 0; t < MAX_BUFFERS; t++)
            {
                if (VertexBuffers[t].buff)
                    nVertex++;
                if (IndexBuffers[t].buff)
                    nIndex++;
            }
            if (nVertex / 512 > s_vertexHighWater / 512 || nIndex / 512 > s_indexHighWater / 512)
            {
                s_vertexHighWater = nVertex;
                s_indexHighWater = nIndex;
                core.Trace("DX9RENDER: live buffers vertex=%d index=%d of %d", nVertex, nIndex, MAX_BUFFERS);
            }
        }
    }

    if (bShowFps)
    {
        Print(screen_size.x - 100, screen_size.y - 50, "FPS %d", core.EngineFps());
    }

    if (bShowExInfo)
    {
        uint32_t dwTotalTexSize = 0;
        uint32_t dwTotalTexNum = 0, dwTotalVB = 0, dwTotalIB = 0, dwTotalVBSize = 0, dwTotalIBSize = 0;

        int32_t t;
        for (t = 0; t < MAX_STEXTURES; t++)
            if (Textures[t].ref > 0)
            {
                dwTotalTexSize += Textures[t].dwSize;
                dwTotalTexNum++;
            }

        for (t = 0; t < MAX_BUFFERS; t++)
        {
            if (VertexBuffers[t].buff)
            {
                dwTotalVBSize += VertexBuffers[t].size;
                dwTotalVB++;
            }
            if (IndexBuffers[t].buff)
            {
                dwTotalIBSize += IndexBuffers[t].size;
                dwTotalIB++;
            }
        }

        CMatrix mView;
        GetTransform(storm_render::TransformView, mView);
        mView.Transposition();
        Print(80, 50, "Cam: %.3f, %.3f, %.3f", mView.Pos().x, mView.Pos().y, mView.Pos().z);
        Print(80, 70, "t : %d, %.3f Mb", dwTotalTexNum, float(dwTotalTexSize) / (1024.0f * 1024.0f));
        Print(80, 90, "v : %d, %.3f Mb", dwTotalVB, float(dwTotalVBSize) / (1024.0f * 1024.0f));
        Print(80, 110, "i : %d, %.3f Mb", dwTotalIB, float(dwTotalIBSize) / (1024.0f * 1024.0f));
        Print(80, 130, "d : %d, lv: %d, li: %d", dwNumDrawPrimitive, dwNumLV, dwNumLI);
        Print(80, 150, "s : %d, %.3f, %.3f", dwSoundBuffersCount, dwSoundBytes / 1024.f, dwSoundBytesCached / 1024.f);
    }

    // Try to drop video conveyor
    if (bDropVideoConveyor && pDropConveyorVBuffer)
    {
        CVECTOR *pV;
        pDropConveyorVBuffer->Lock(0, 0, (void **)&pV, 0);
        for (int32_t i = 0; i < 2; i++)
            pV[i] = CVECTOR(1e6f, 1e6f, 1e6f);
        pDropConveyorVBuffer->Unlock();
        renderDevice->SetStreamSource(0, pDropConveyorVBuffer, 0, sizeof(CVECTOR));
        SetFVF(storm_render::vertexFormat::xyz);
        DrawPrimitive(storm_render::PrimitiveLineList, 0, 1);
    }

    if (CHECKD3DERR(EndScene()))
        return false;

    if (bMakeShoot)
        MakeScreenShot();

    const storm_render::Result hRes = renderDevice->Present();

    if (hRes == storm_render::result::failure)
    {
        LostRender();
    }

#ifdef _WIN32 // bSafeRendering
    if (bSafeRendering)
    {
        const HDC dc = GetDC(hwnd);
        SetPixel(dc, 0, 0, 0);
        ReleaseDC(hwnd, dc);
    }
#endif

    return true;
}

//################################################################################
static int totSize = 0;

int32_t DX9RENDER::TextureCreate(const char *fname)
{
    // start add texture path
    if ((uintptr_t)fname == -1)
    {
        iSetupPath = 1;
        return -1;
    }

    // continue add texture path process
    if (iSetupPath)
    {
        if (iSetupPath == 1)
        {
            dwSetupNumber = (uintptr_t)(fname);
            iSetupPath = 2;
            return -1;
        }

        TexPaths[dwSetupNumber].str[0] = 0;
        if (fname && fname[0])
            strcpy_s(TexPaths[dwSetupNumber].str, fname);
        iSetupPath = 0;
        return -1;
    }

    if (fname == nullptr)
    {
        core.Trace("Can't create texture with null name");
        return -1L;
    }

    if (!bLoadTextureEnabled)
        return -1;

    for (int32_t i = 3; i >= -1; i--)
    {
        char _fname[256];

        if (i >= 0 && TexPaths[i].str[0] == 0)
            continue;
        if (i >= 0)
        {
            const uint32_t dwLen = strlen(fname);

            int32_t j;
            for (j = dwLen - 1; j >= 0; j--)
                if (fname[j] == '\\')
                    break;

            _fname[0] = 0;
            strncpy_s(_fname, fname, j + 1);
            _fname[j + 1] = 0;
            strcat_s(_fname, TexPaths[i].str);
            strcat_s(_fname, &fname[j + 1]);
            bTrace = false;
        }
        else
        {
            bTrace = true;
            strcpy_s(_fname, fname);
        }

        std::ranges::for_each(_fname, [](char &c) { c = std::toupper(c); });

        const uint32_t hf = MakeHashValue(_fname);

        int32_t t;
        for (t = 0; t < MAX_STEXTURES; t++)
            if (Textures[t].ref != 0)
                if (Textures[t].name)
                    if (Textures[t].hash == hf && storm::iEquals(Textures[t].name, _fname))
                    {
                        Textures[t].ref++;
                        return t;
                    }

        for (t = 0; t < MAX_STEXTURES; t++)
            if (Textures[t].ref == 0)
                break;

        Textures[t].hash = hf;

        const auto len = strlen(_fname) + 1;
        if ((Textures[t].name = new char[len]) == nullptr)
            throw std::runtime_error("allocate memory error");
        strcpy_s(Textures[t].name, len, _fname);
        Textures[t].isCubeMap = false;
        Textures[t].loaded = false;
        Textures[t].ref++;
        if (TextureLoad(t))
            return t;
        Textures[t].ref--;
        STORM_DELETE(Textures[t].name);
    }
    return -1;
}

int32_t DX9RENDER::TextureCreate(UINT width, UINT height, UINT levels, uint32_t usage, storm_render::AssetFormat format, storm_render::MemoryClass pool)
{
    storm_render::Texture *texture = nullptr;

    const auto result = CreateTexture(width, height, levels, usage, format, pool, &texture);
    if (CHECKD3DERR(result))
    {
        return -1;
    }

    int32_t t;
    for (t = 0; t < MAX_STEXTURES; t++)
    {
        if (Textures[t].ref == 0)
        {
            break;
        }
    }

    Textures[t].d3dtex = texture;
    Textures[t].name = nullptr;
    Textures[t].hash = 0;
    Textures[t].ref = 1;
    Textures[t].dwSize = width * height * 4; // Assuming 32-bit pixels
    Textures[t].isCubeMap = false;
    Textures[t].loaded = true;

    return t;
}

bool DX9RENDER::TextureIncReference(int32_t texid)
{
    ++Textures[texid].ref;
    return true;
}

bool DX9RENDER::TextureLoad(int32_t t)
{
    using namespace std::literals;

    ProgressView();
    // Form the path to the texture
    char fn[_MAX_FNAME];
    Textures[t].dwSize = 0;
    if (Textures[t].name == nullptr)
    {
        return false;
    }

    auto lTexture = std::string(Textures[t].name);
    std::transform(lTexture.begin(), lTexture.end(), lTexture.begin(), [](unsigned char c) { return std::tolower(c); });
    auto has_resource_prefix = starts_with(lTexture, "resource\\textures\\");
    auto has_tx_postfix = ends_with(lTexture, ".tx");

    sprintf_s(fn, "%s%s%s", has_resource_prefix ? "" : "resource\\textures\\", Textures[t].name, has_tx_postfix ? "" : ".tx");

    for (int32_t s = 0, d = 0; fn[d]; s++)
    {
        if (d > 0 && (fn[d - 1] == PATH_SEP || fn[d - 1] == WRONG_PATH_SEP) &&
            (fn[s] == PATH_SEP || fn[s] == WRONG_PATH_SEP))
        {
            continue;
        }
        fn[d++] = fn[s];
    }
    // Opening the file
    auto fileS = fio->_CreateFile(fn, std::ios::binary | std::ios::in);
    if (!fileS.is_open())
    {
        // try to load without '.tx' (e.g. raw Targa)
        std::filesystem::path path_to_tex{fn};
        path_to_tex.replace_extension();
        path_to_tex = fio->ConvertPathResource(path_to_tex.string().c_str());
        if (exists(path_to_tex))
        {
            return TextureLoadUsingD3DX(path_to_tex.string().c_str(), t);
        }
        if (bTrace)
        {
            core.Trace("Can't load texture %s", fn);
        }
        delete Textures[t].name;
        Textures[t].name = nullptr;
        return false;
    }
    // Reading the header
    TX_FILE_HEADER head;
    if (!fio->_ReadFile(fileS, &head, sizeof(head)))
    {
        if (bTrace)
        {
            core.Trace("Can't load texture %s", fn);
        }
        delete Textures[t].name;
        Textures[t].name = nullptr;
        fio->_CloseFile(fileS);
        return false;
    }
    // Analyzing the format
    storm_render::AssetFormat d3dFormat = storm_render::AssetFormat::Unknown;
    int32_t textureFI;
    for (textureFI = 0; textureFI < sizeof(textureFormats) / sizeof(SD_TEXTURE_FORMAT); textureFI++)
    {
        if (textureFormats[textureFI].txFormat == head.format)
        {
            break;
        }
    }
    if (textureFI == sizeof(textureFormats) / sizeof(SD_TEXTURE_FORMAT) || head.flags & TX_FLAGS_PALLETTE)
    {
        if (bTrace)
        {
            core.Trace("Invalidate texture format %s, not loading it.", fn);
        }
        delete Textures[t].name;
        Textures[t].name = nullptr;
        fio->_CloseFile(fileS);
        return false;
    }
    d3dFormat = textureFormats[textureFI].d3dFormat;
    bool isSwizzled = textureFormats[textureFI].isSwizzled;
    const char *formatTxt = textureFormats[textureFI].format;
    if (head.width <= 0 || head.height <= 0 || head.nmips <= 0 || head.mip_size <= 0 ||
        !storm::renderer::TextureMipHeaderIsValid(static_cast<uint32_t>(head.format),
                                                  static_cast<uint32_t>(head.width),
                                                  static_cast<uint32_t>(head.height),
                                                  static_cast<uint32_t>(head.nmips),
                                                  static_cast<uint32_t>(head.mip_size)))
    {
        if (bTrace)
        {
            core.Trace("Invalid texture header %s, not loading it.", fn);
        }
        delete Textures[t].name;
        Textures[t].name = nullptr;
        fio->_CloseFile(fileS);
        return false;
    }

    // Skipping mips
    uint32_t seekposition = 0;
    for (int32_t nTD = nTextureDegradation; nTD > 0; nTD--)
    {
        if (head.nmips <= 1 || head.width <= 32 || head.height <= 32)
        {
            break; // degradation limit
        }
        const auto mipSize = storm::renderer::TextureMipSize(static_cast<uint32_t>(d3dFormat),
                                                              static_cast<uint32_t>(head.width),
                                                              static_cast<uint32_t>(head.height));
        if (mipSize == 0 || seekposition > std::numeric_limits<uint32_t>::max() - mipSize)
        {
            if (bTrace)
            {
                core.Trace("Invalid texture mip chain %s, not loading it.", fn);
            }
            delete Textures[t].name;
            Textures[t].name = nullptr;
            fio->_CloseFile(fileS);
            return false;
        }
        seekposition += mipSize;
        head.nmips--;
        head.width = std::max(1, head.width / 2);
        head.height = std::max(1, head.height / 2);
    }
    // Loading the texture
    if (!(head.flags & TX_FLAGS_CUBEMAP))
    {
        // Loading a regular texture
        // Position in file
        if (seekposition)
        {
            fio->_SetFilePointer(fileS, seekposition, std::ios::cur);
        }
        // create the texture
        storm_render::Texture *tex = nullptr;
        if (CHECKD3DERR(renderDevice->CreateTexture(head.width, head.height, head.nmips, 0, d3dFormat, &tex)) == true ||
            !tex)
        {
            if (bTrace)
            {
                core.Trace(
                    "Texture %s is not created (width: %i, height: %i, num mips: %i, format: %s), not loading it.", fn,
                    head.width, head.height, head.nmips, formatTxt);
            }
            delete Textures[t].name;
            Textures[t].name = nullptr;
            fio->_CloseFile(fileS);
            return false;
        }
        // Filling the levels
        for (int32_t m = 0; m < head.nmips; m++)
        {
            const auto mipSize = storm::renderer::TextureMipSize(static_cast<uint32_t>(d3dFormat),
                                                                  static_cast<uint32_t>(head.width),
                                                                  static_cast<uint32_t>(head.height));
            // take into account the size of the mip
            Textures[t].dwSize += mipSize;
            // Getting the mip surface
            bool isError = false;
            storm_render::Surface *surface = nullptr;
            if (CHECKD3DERR(tex->GetSurfaceLevel(m, &surface)) == true || !surface)
            {
                isError = true;
            }
            else
            {
                // read the mip
                isError = mipSize == 0 ||
                    !LoadTextureSurface(fileS, surface, mipSize, head.width, head.height, isSwizzled);
            }
            // Freeing the surface
            if (surface)
            {
                surface->Release();
            }
            // If there was an error, then interrupt the download
            if (isError)
            {
                if (bTrace)
                {
                    core.Trace("Can't loading mip %i, texture %s is not created (width: %i, height: %i, num mips: %i, "
                               "format: %s), not loading it.",
                               m, fn, head.width, head.height, head.nmips, formatTxt);
                }
                delete Textures[t].name;
                Textures[t].name = nullptr;
                fio->_CloseFile(fileS);
                tex->Release();
                return false;
            }
            // recalculate the dimensions for the next mip
            head.width = std::max(1, head.width / 2);
            head.height = std::max(1, head.height / 2);
        }
        Textures[t].d3dtex = tex;
        Textures[t].isCubeMap = false;
#ifdef STORM_METAL_ROCKK2
        storm_metal_label_texture(tex, Textures[t].name);
#endif
    }
    else
    {
        // Download cubemap
        if (head.width != head.height)
        {
            if (bTrace)
            {
                core.Trace("Cube map texture can't has not squared sides %s, not loading it.", fn);
            }
            delete Textures[t].name;
            Textures[t].name = nullptr;
            fio->_CloseFile(fileS);
            return false;
        }
        // Number of mips
        storm_render::Capabilities devcaps;
        if (CHECKD3DERR(renderDevice->GetCapabilities(&devcaps)))
        {
            if (bTrace)
            {
                core.Trace("Cube map texture %s is not created (size: %i, num mips: %i, format: %s), not loading it.",
                           fn, head.width, head.nmips, formatTxt);
            }
            delete Textures[t].name;
            Textures[t].name = nullptr;
            fio->_CloseFile(fileS);
            return false;
        }
        if (!devcaps.mipCubeMap)
        {
            head.nmips = 1;
        }
        // create the texture
        storm_render::CubeTexture *tex = nullptr;
        if (CHECKD3DERR(renderDevice->CreateCubeTexture(head.width, head.nmips, 0, d3dFormat, &tex)) ==
                true ||
            !tex)
        {
            if (bTrace)
            {
                core.Trace("Cube map texture %s is not created (size: %i, num mips: %i, format: %s), not loading it.",
                           fn, head.width, head.nmips, formatTxt);
            }
            delete Textures[t].name;
            Textures[t].name = nullptr;
            fio->_CloseFile(fileS);
            return false;
        }
        // Loading the sides
        bool isError = false;
        if (seekposition)
        {
            fio->_SetFilePointer(fileS, seekposition, std::ios::cur);
        }
        uint32_t sz =
            LoadCubmapSide(fileS, tex, storm_render::CubeFace::PositiveZ, head.nmips, head.mip_size, head.width, isSwizzled);
        if (sz)
        {
            Textures[t].dwSize += sz;
            if (seekposition)
            {
                fio->_SetFilePointer(fileS, seekposition, std::ios::cur);
            }
            sz = LoadCubmapSide(fileS, tex, storm_render::CubeFace::PositiveX, head.nmips, head.mip_size, head.width,
                                isSwizzled);
            if (sz)
            {
                Textures[t].dwSize += sz;
                if (seekposition)
                {
                    fio->_SetFilePointer(fileS, seekposition, std::ios::cur);
                }
                sz = LoadCubmapSide(fileS, tex, storm_render::CubeFace::NegativeZ, head.nmips, head.mip_size, head.width,
                                    isSwizzled);
                if (sz)
                {
                    Textures[t].dwSize += sz;
                    if (seekposition)
                    {
                        fio->_SetFilePointer(fileS, seekposition, std::ios::cur);
                    }
                    sz = LoadCubmapSide(fileS, tex, storm_render::CubeFace::NegativeX, head.nmips, head.mip_size, head.width,
                                        isSwizzled);
                    if (sz)
                    {
                        Textures[t].dwSize += sz;
                        if (seekposition)
                        {
                            fio->_SetFilePointer(fileS, seekposition, std::ios::cur);
                        }
                        sz = LoadCubmapSide(fileS, tex, storm_render::CubeFace::PositiveY, head.nmips, head.mip_size,
                                            head.width, isSwizzled);
                        if (sz)
                        {
                            Textures[t].dwSize += sz;
                            if (seekposition)
                            {
                                fio->_SetFilePointer(fileS, seekposition, std::ios::cur);
                            }
                            sz = LoadCubmapSide(fileS, tex, storm_render::CubeFace::NegativeY, head.nmips, head.mip_size,
                                                head.width, isSwizzled);
                            if (!sz)
                            {
                                isError = true;
                            }
                            Textures[t].dwSize += sz;
                        }
                        else
                        {
                            isError = true;
                        }
                    }
                    else
                    {
                        isError = true;
                    }
                }
                else
                {
                    isError = true;
                }
            }
            else
            {
                isError = true;
            }
        }
        else
        {
            isError = true;
        }

        if (isError)
        {
            if (bTrace)
            {
                core.Trace("Cube map texture %s can't loading (size: %i, num mips: %i, format: %s), not loading it.",
                           fn, head.width, head.nmips, formatTxt);
            }
            delete Textures[t].name;
            Textures[t].name = nullptr;
            fio->_CloseFile(fileS);
            tex->Release();
            return false;
        }
        Textures[t].d3dtex = tex;
        Textures[t].isCubeMap = true;
    }

    //---------------------------------------------------------------
    // print statistics
    //---------------------------------------------------------------
    if (texLog)
    {
        char s[256];
        if (totSize == 0)
        {
            fio->_DeleteFile("texLoad.txt");
        }
        auto fileS2 = fio->_CreateFile("texLoad.txt", std::ios::binary | std::ios::out | std::ios::app);
        totSize += Textures[t].dwSize;
        sprintf_s(s, "%.2f, size: %d, %d * %d, %s\n", totSize / 1024.0f / 1024.0f, Textures[t].dwSize, head.width,
                  head.height, Textures[t].name);
        fio->_WriteFile(fileS2, s, strlen(s));
        fio->_FlushFileBuffers(fileS2);
        fio->_CloseFile(fileS2);
    }
    dwTotalSize += Textures[t].dwSize;
    //---------------------------------------------------------------
    Textures[t].loaded = true;
    // Close the file
    fio->_CloseFile(fileS);
    return true;
}

bool DX9RENDER::TextureLoadUsingD3DX(const char* path, int32_t t)
{
    // TODO: reimplement the whole thing in a tidy way
    storm_render::Texture *pTex;
#ifdef _WIN32
    if(CHECKD3DERR(D3DXCreateTextureFromFileA(renderDevice, path, &pTex)))
#else
    if(CHECKD3DERR(storm_render::ImageLoadTextureFromFile(renderDevice, path, &pTex)))
#endif
    {
        delete Textures[t].name;
        Textures[t].name = nullptr;
        return false;
    }

    storm_render::SurfaceDesc desc;
    pTex->GetLevelDesc(0, &desc);

    Textures[t].hash = 0;
    Textures[t].ref = 1;
    Textures[t].d3dtex = pTex;
    Textures[t].isCubeMap = false;
    Textures[t].dwSize = desc.Height * desc.Width * 4;
    Textures[t].loaded = true;

    return true;
}

storm_render::BaseTexture *DX9RENDER::GetBaseTexture(int32_t iTexture)
{
    return (iTexture >= 0) ? Textures[iTexture].d3dtex : nullptr;
}

uint32_t DX9RENDER::LoadCubmapSide(std::fstream &fileS, storm_render::CubeTexture *tex, storm_render::CubeFace face,
                                   uint32_t numMips, uint32_t mipSize, uint32_t size, bool isSwizzled)
{
    uint32_t texsize = 0;
    // Filling the levels
    for (uint32_t m = 0; m < numMips; m++)
    {
        // Getting the mip surface
        bool isError = false;
        storm_render::Surface *surface = nullptr;
        if (CHECKD3DERR(tex->GetCubeMapSurface(face, m, &surface)) == true || !surface)
        {
            isError = true;
        }
        else
        {
            storm_render::SurfaceDesc desc{};
            const auto mipBytes = CHECKD3DERR(surface->GetDesc(&desc)) ? 0 :
                storm::renderer::TextureMipSize(static_cast<uint32_t>(desc.Format), desc.Width, desc.Height);
            // read the mip
            isError = mipBytes == 0 ||
                !LoadTextureSurface(fileS, surface, mipBytes, size, size, isSwizzled);
            if (!isError)
            {
                texsize += mipBytes;
            }
        }
        // Freeing the surface
        if (surface)
        {
            surface->Release();
        }
        // If there was an error, then interrupt the download
        if (isError)
        {
            if (bTrace)
            {
                core.Trace("Can't loading cubemap mip %i (side: %i), not loading it.", m, face);
            }
            return 0;
        }
        // recalculate the dimensions for the next mip
        size = std::max(1u, size / 2);
    }
    return texsize;
}

bool DX9RENDER::LoadTextureSurface(std::fstream &fileS, storm_render::Surface *suface, uint32_t mipSize, uint32_t width,
                                   uint32_t height, bool isSwizzled)
{
    //------------------------------------------------------------------------------------------
    // PC version
    // ------------------------------------------------------------------------------------------
    // Surface pointer
    storm_render::LockedRect lock;
    storm_render::SurfaceDesc desc{};
    if (CHECKD3DERR(suface->GetDesc(&desc)) == true)
    {
        return false;
    }
    const auto surfaceBytes = storm::renderer::TextureMipSize(static_cast<uint32_t>(desc.Format), desc.Width,
                                                               desc.Height);
    if (!storm::renderer::TextureMipReadIsValid(surfaceBytes, mipSize))
    {
        return false;
    }
    const auto position = fileS.tellg();
    if (position < 0)
    {
        return false;
    }
    fileS.seekg(0, std::ios::end);
    const auto end = fileS.tellg();
    fileS.seekg(position);
    if (end < position || !storm::renderer::TextureMipPayloadAvailable(
                              static_cast<uint64_t>(end - position), mipSize))
    {
        return false;
    }

    if (CHECKD3DERR(suface->LockRect(&lock, NULL, 0L)) == true)
    {
        return false;
    }
    // Reading out
    if (!fio->_ReadFile(fileS, lock.pBits, mipSize))
    {
        if (CHECKD3DERR(suface->UnlockRect()) == true)
        {
            return false;
        }
        return false;
    }
    // Surface release
    if (CHECKD3DERR(suface->UnlockRect()) == true)
    {
        return false;
    }
    return true;
    //------------------------------------------------------------------------------------------
}

//################################################################################
bool DX9RENDER::TextureSet(int32_t stage, int32_t texid)
{
    if (texid == -1)
    {
        if (CHECKD3DERR(renderDevice->SetTexture(stage, NULL)) == true)
        {
            return false;
        }
        return true;
    }

    /*
    if(Textures[texid].loaded==false)
    {
    int tex2load[MAX_STEXTURES];
    int t2l=0;
    for(int32_t t=0; t<MAX_STEXTURES; t++)
    if(Textures[t].ref>0 && Textures[t].loaded==false)    tex2load[t2l++] = t;
    */
    /*/sort textures
    for(t=0; t<t2l; t++)
    for(int32_t tt=t; tt<t2l; tt++)
    if(_strcmpi(Textures[tex2load[tt]].name, Textures[tex2load[t]].name)<0)
    {
    int ttemp = tex2load[t];
    tex2load[t] = tex2load[tt];
    tex2load[tt] = ttemp;
    }*/
    /*
    for(t=0; t<t2l; t++)
    {
    Textures[tex2load[t]].loaded = true;
    TextureLoad(tex2load[t]);
    }
    //Textures[texid].loaded = true;    TextureLoad(texid);
    }
    */

    if (CHECKD3DERR(renderDevice->SetTexture(stage, Textures[texid].d3dtex)) == true)
    {
        return false;
    }
    return true;
}

//################################################################################
bool DX9RENDER::TextureRelease(int32_t texid)
{
    if (texid == -1)
    {
        return true;
    }
    Textures[texid].ref--;
    if (Textures[texid].ref != 0)
    {
        return false;
    }
    if (Textures[texid].name != nullptr)
    {
        if (texLog)
        {
            auto fileS = fio->_CreateFile("texLoad.txt", std::ios::binary | std::ios::in | std::ios::out);
            const int bytes = fio->_GetFileSize("texLoad.txt");
            auto buf = new char[bytes + 1];
            fio->_ReadFile(fileS, buf, bytes);
            buf[bytes] = 0;

            char *str = strstr(buf, Textures[texid].name);
            if (str != nullptr)
            {
                fio->_SetFilePointer(fileS, str - buf, std::ios::beg);
                auto s = "*";
                fio->_WriteFile(fileS, s, 1);
            }
            delete[] buf;
            fio->_FlushFileBuffers(fileS);
            fio->_CloseFile(fileS);

            /*FILE *flstat = fopen("texLoad.txt", "r+b");
            totSize -= Textures[texid].dwSize;
            fseek(flstat, 0, SEEK_END);
            int bytes = ftell(flstat);
            char *buf = new char[bytes+1];
            fseek(flstat, 0, SEEK_SET);
            fread(buf, bytes, sizeof *buf, flstat);
            buf[bytes] = 0;

            char *str = strstr(buf, Textures[texid].name);
            if(str!=0)
            {
            fseek(flstat, str-buf, SEEK_SET);
            const char *s = "*";
            fwrite(s, 1, 1, flstat);
            }
            delete buf;
            fclose(flstat);*/
        }

        delete Textures[texid].name;
        Textures[texid].name = nullptr;
    }
    if (Textures[texid].loaded == false)
    {
        return false;
    }

    if (Textures[texid].d3dtex)
    {
        if (CHECKD3DERR(Textures[texid].d3dtex->Release()) == true)
        {
            return false;
        }
    }
    Textures[texid].d3dtex = nullptr;
    dwTotalSize -= Textures[texid].dwSize;

    return true;
}

//################################################################################
bool DX9RENDER::SetCamera(const CVECTOR &pos, const CVECTOR &ang, float fov)
{
    return SetCamera(pos, ang, fov, 1.f);
}

bool DX9RENDER::SetCamera(const CVECTOR &pos, const CVECTOR &ang, float fov, float magnification)
{
    if (!SetCamera(pos, ang))
        return false;
    if (!SetPerspective(fov, aspectRatio, magnification))
        return false;

    ProcessScriptPosAng(pos, ang);
    return true;
}

bool DX9RENDER::SetCamera(const CVECTOR &pos, const CVECTOR &ang)
{
    CMatrix mtx;
    const CVECTOR vOldWordRelationPos = vWordRelationPos;
    /*if (!pos || !ang)
    {
      if (!pos && !ang) return true;
      //if(CHECKD3DERR(GetTransform(D3DTS_VIEW, mtx))==true) return false;
      renderDevice->GetTransform(D3DTS_VIEW, mtx);
      if (pos)
      {
        //mtx.SetInversePosition(pos->x, pos->y, pos->z);
        Pos = *pos;
        vWordRelationPos = -Pos;
      }
      else
      {
        //CVECTOR p;
        //mtx.MulToInvNorm(mtx.Pos(), p);
        mtx.BuildMatrix(*ang);
        mtx.Transposition3X3();
        //mtx.SetInversePosition(p.x, p.y, p.z);
        Ang = *ang;
      }
    }
    else*/
    {
        mtx.BuildMatrix(ang);
        mtx.Transposition3X3();
        // mtx.SetInversePosition(pos->x, pos->y, pos->z);
        Pos = pos;
        Ang = ang;
        vWordRelationPos = -Pos;
    }
    // if(CHECKD3DERR(SetTransform(D3DTS_VIEW, mtx))==true)    return false;

    renderDevice->SetTransform(storm_render::TransformView, mtx);
    vViewRelationPos = -(mtx * vWordRelationPos);
    renderDevice->GetTransform(storm_render::TransformWorld, mtx);
    mtx.Pos() += vWordRelationPos - vOldWordRelationPos;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    StormMetalRebaseWorld(renderDevice, mtx);
    StormMetalSetWorldOrigin(renderDevice, &vWordRelationPos.x);
#else
    renderDevice->SetTransform(storm_render::TransformWorld, mtx);
#endif

    FindPlanes(renderDevice);
    ProcessScriptPosAng(pos, ang);
    return true;
}

bool DX9RENDER::SetCamera(CVECTOR lookFrom, CVECTOR lookTo, CVECTOR up)
{
    CMatrix mtx;
    if (!mtx.BuildViewMatrix(lookFrom, lookTo, up))
        return false;
    // if(CHECKD3DERR(SetTransform(D3DTS_VIEW, mtx))==true)    return false;
    SetTransform(storm_render::TransformView, mtx);
    Pos = lookFrom;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    StormMetalSetWorldOrigin(renderDevice, &vWordRelationPos.x);
#endif

    // calculate the angle of the camera
    // Ang = 0.0f;
    const CVECTOR vNorm = !(lookTo - lookFrom);
    Ang.y = atan2f(vNorm.x, vNorm.z);
    Ang.x = atan2f(-vNorm.y, sqrtf(vNorm.x * vNorm.x + vNorm.z * vNorm.z));
    Ang.z = 0.f;

    FindPlanes(renderDevice);
    ProcessScriptPosAng(lookFrom, Ang);
    return true;
}

void DX9RENDER::ProcessScriptPosAng(const CVECTOR &vPos, const CVECTOR &vAng)
{
    core.Event("CameraPosAng", "ffffff", vPos.x, vPos.y, vPos.z, vAng.x, vAng.y, vAng.z);
}

void DX9RENDER::GetNearFarPlane(float &fNear, float &fFar)
{
    fNear = fNearClipPlane;
    fFar = fFarClipPlane;
}

void DX9RENDER::SetNearFarPlane(float fNear, float fFar)
{
    fNearClipPlane = fNear;
    fFarClipPlane = fFar;

    // Keep the original pre-multiplier FOV and both-axis zoom when island
    // passes temporarily change their depth range.
    SetPerspective(perspectiveInput, aspectRatio, projectionMagnification);
}

bool DX9RENDER::SetPerspective(float perspective, float fAspectRatio, float magnification)
{
    if (!std::isfinite(magnification) || magnification <= 0.f)
        return false;
    const float inputPerspective = perspective;
    perspective *= FovMultiplier;

    const float near_plane = fNearClipPlane; // Distance to near clipping
    const float far_plane = fFarClipPlane;   // Distance to far clipping
    const float fov_horiz = perspective;     // Horizontal field of view  angle, in radians
    if (fAspectRatio < 0)
    {
        fAspectRatio = static_cast<float>(screen_size.y) / screen_size.x;
    }
    aspectRatio = fAspectRatio;
	
    // evganat - по заявка выводим формулу vfov в engine.ini
	float fov_vert;
	if(bNewFovCalculation)
	{
		fov_vert = 2.f * atanf(tanf(perspective / 2.f) * fAspectRatio);
	}
	else
	{
		fov_vert = perspective * fAspectRatio;
	}

    const float w = 1.0f / tanf(fov_horiz * 0.5f);
    const float h = 1.0f / tanf(fov_vert * 0.5f);
    const float Q = far_plane / (far_plane - near_plane);

    storm_render::Matrix mtx{};

    // Scale the existing projection equally on both axes. This preserves
    // legacy vertical-FOV policy and user fov_multiplier without distortion.
    mtx._11 = w * magnification;
    mtx._22 = h * magnification;
    mtx._33 = Q;
    mtx._43 = -Q * near_plane;
    mtx._34 = 1.0f;

    if (CHECKD3DERR(renderDevice->SetTransform(storm_render::TransformProjection, &mtx)) == true)
        return false;
    perspectiveInput = inputPerspective;
    projectionMagnification = magnification;
    Fov = magnification == 1.f ? perspective : 2.f * atanf(tanf(perspective * .5f) / magnification);
    FindPlanes(renderDevice);
    return true;
}

//################################################################################
bool DX9RENDER::SetCurrentMatrix(storm_render::Matrix *mtx)
{
    // return CHECKD3DERR(SetTransform(D3DTS_WORLD, mtx)) == D3D_OK;
    SetTransform(storm_render::TransformWorld, mtx);
    return true;
}

bool DX9RENDER::SetMaterial(storm_render::Material &m)
{
    if (CHECKD3DERR(renderDevice->SetMaterial(&m)))
        return false;
    return true;
}

bool DX9RENDER::SetLight(uint32_t dwIndex, const storm_render::Light *pLight)
{
    // Set the property information for the first light.
    storm_render::Light tmpLight = *pLight;
    tmpLight.Position.x += vWordRelationPos.x;
    tmpLight.Position.y += vWordRelationPos.y;
    tmpLight.Position.z += vWordRelationPos.z;
    if (CHECKD3DERR(renderDevice->SetLight(dwIndex, &tmpLight)))
        return false;
    return true;
}

bool DX9RENDER::LightEnable(uint32_t dwIndex, bool bOn)
{
    if (CHECKD3DERR(renderDevice->LightEnable(dwIndex, bOn)))
        return false;
    return true;
}

bool DX9RENDER::GetLightEnable(uint32_t dwIndex, BOOL *pEnable)
{
    if (!pEnable)
        return false;
    bool enabled = false;
    if (CHECKD3DERR(renderDevice->GetLightEnable(dwIndex, &enabled)))
        return false;
    *pEnable = enabled;
    return true;
}

bool DX9RENDER::GetLight(uint32_t dwIndex, storm_render::Light *pLight)
{
    if (CHECKD3DERR(renderDevice->GetLight(dwIndex, pLight)))
        return false;

    pLight->Position.x -= vWordRelationPos.x;
    pLight->Position.y -= vWordRelationPos.y;
    pLight->Position.z -= vWordRelationPos.z;
    return true;
}

//################################################################################

int32_t DX9RENDER::CreateVertexBuffer(int32_t type, size_t size, uint32_t dwUsage, storm_render::MemoryClass dwPool)
{
    if (size <= 0)
        return -1; // fix

    int32_t b;
    for (b = 0; b < MAX_BUFFERS; b++)
        if (VertexBuffers[b].buff == nullptr)
            break;

    if (b == MAX_BUFFERS)
    {
        core.Trace("DX9RENDER: vertex buffer table exhausted (%d slots); new geometry cannot be created", MAX_BUFFERS);
        return -1;
    }

    if (CHECKD3DERR(renderDevice->CreateVertexBuffer(size, dwUsage, type, &VertexBuffers[b].buff)))
        return -1;

    VertexBuffers[b].type = type;
    VertexBuffers[b].size = size;
    VertexBuffers[b].dwUsage = dwUsage;

    return b;
}

storm_render::VertexBuffer *DX9RENDER::GetVertexBuffer(int32_t id)
{
    if (id < 0 || id >= MAX_BUFFERS)
        return nullptr;
    return VertexBuffers[id].buff;
}

int32_t DX9RENDER::GetVertexBufferFVF(int32_t id)
{
    if (id < 0 || id >= MAX_BUFFERS)
        return 0;
    return VertexBuffers[id].type;
}

//################################################################################
int32_t DX9RENDER::CreateIndexBuffer(size_t size, uint32_t dwUsage)
{
    int32_t b;
    for (b = 0; b < MAX_BUFFERS; b++)
        if (IndexBuffers[b].buff == nullptr)
            break;

    if (b == MAX_BUFFERS)
    {
        core.Trace("DX9RENDER: index buffer table exhausted (%d slots); new geometry cannot be created", MAX_BUFFERS);
        return -1;
    }

    // renderDevice->CreateIndexBuffer(size, D3DUSAGE_WRITEONLY, D3DFMT_INDEX16,
    if (CHECKD3DERR(
            renderDevice->CreateIndexBuffer(size, dwUsage, storm_render::IndexFormat::UInt16, &IndexBuffers[b].buff)))
        return -1;

    IndexBuffers[b].size = size;
    IndexBuffers[b].dwUsage = dwUsage;

    return b;
}

namespace
{
// A rejected buffer id must never reach the fixed buffer table: reading before
// or past it dereferences whatever memory lies there. Report the first
// rejection once instead of crashing, and let the caller skip that draw.
void ReportRejectedBuffer(const char *where, int32_t vbuff, int32_t ibuff)
{
    static bool s_reported = false;
    if (s_reported)
        return;
    s_reported = true;
    core.Trace("DX9RENDER: %s rejected an unallocated buffer (vertex %d, index %d); drawing continues without it",
               where, vbuff, ibuff);
}
} // namespace

//################################################################################
void DX9RENDER::DrawBuffer(int32_t vbuff, int32_t stride, int32_t ibuff, int32_t minv, size_t numv, size_t startidx, size_t numtrg,
                           const char *cBlockName)
{
    bool bDraw = true;

    if (vbuff >= MAX_BUFFERS || (vbuff >= 0 && VertexBuffers[vbuff].buff == nullptr) || ibuff < 0 ||
        ibuff >= MAX_BUFFERS || IndexBuffers[ibuff].buff == nullptr)
    {
        ReportRejectedBuffer("DrawBuffer", vbuff, ibuff);
        return;
    }

    if (vbuff >= 0)
        if (CHECKD3DERR(SetFVF(VertexBuffers[vbuff].type)))
            return;
    // else VertexBuffer already set

    if (CHECKD3DERR(renderDevice->SetIndices(IndexBuffers[ibuff].buff)))
        return;

    if (vbuff >= 0)
        if (CHECKD3DERR(renderDevice->SetStreamSource(0, VertexBuffers[vbuff].buff, 0, stride)))
            return;
    // else VertexBuffer already set

    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);

    if (bDraw)
        do
        {
            dwNumDrawPrimitive++;
            CHECKD3DERR(renderDevice->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList, minv, 0, numv, startidx, numtrg));
        } while (cBlockName && cBlockName[0] && TechniqueExecuteNext());
}

void DX9RENDER::DrawIndexedPrimitiveNoVShader(storm_render::Primitive dwPrimitiveType, int32_t iVBuff, int32_t iStride, int32_t iIBuff,
                                              int32_t iMinV, int32_t iNumV, int32_t iStartIdx, int32_t iNumTrg,
                                              const char *cBlockName)
{
    bool bDraw = true;

    if (iIBuff < 0 || iIBuff >= MAX_BUFFERS || IndexBuffers[iIBuff].buff == nullptr)
    {
        ReportRejectedBuffer("DrawIndexedPrimitiveNoVShader", iVBuff, iIBuff);
        return;
    }

    if (CHECKD3DERR(renderDevice->SetIndices(IndexBuffers[iIBuff].buff)) == true)
        return;

    if (iVBuff >= 0)
    {
        if (iVBuff >= MAX_BUFFERS || VertexBuffers[iVBuff].buff == nullptr)
        {
            ReportRejectedBuffer("DrawIndexedPrimitiveNoVShader", iVBuff, iIBuff);
            return;
        }
        if (CHECKD3DERR(renderDevice->SetStreamSource(0, VertexBuffers[iVBuff].buff, 0, iStride)) == true)
            return;
    }

    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);
    if (bDraw)
        do
        {
            dwNumDrawPrimitive++;
            CHECKD3DERR(renderDevice->DrawIndexedPrimitive(dwPrimitiveType, iMinV, 0, iNumV, iStartIdx, iNumTrg));
        } while (cBlockName && TechniqueExecuteNext());
}

void DX9RENDER::DrawIndexedPrimitiveUP(storm_render::Primitive dwPrimitiveType, uint32_t dwMinIndex, uint32_t dwNumVertices,
                                       uint32_t dwPrimitiveCount, const void *pIndexData, storm_render::IndexFormat IndexDataFormat,
                                       const void *pVertexData, uint32_t dwVertexStride, const char *cBlockName)
{
    bool bDraw = true;
    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);
    if (bDraw)
        do
        {
            dwNumDrawPrimitive++;
#ifdef STORM_METAL_RENDERER_CONSUMERS
            if (!StormMetalDrawCompactIndexedPrimitive(renderDevice, dwPrimitiveType, dwMinIndex, dwNumVertices,
                                                        dwPrimitiveCount, pIndexData, uint32_t(IndexDataFormat),
                                                        pVertexData, dwVertexStride))
                CHECKD3DERR(renderDevice->DrawIndexedPrimitiveUP(dwPrimitiveType, dwMinIndex, dwNumVertices, dwPrimitiveCount,
                                                         pIndexData, IndexDataFormat, pVertexData, dwVertexStride));
#else
            CHECKD3DERR(renderDevice->DrawIndexedPrimitiveUP(dwPrimitiveType, dwMinIndex, dwNumVertices, dwPrimitiveCount,
                                                     pIndexData, IndexDataFormat, pVertexData, dwVertexStride));
#endif
        } while (cBlockName && TechniqueExecuteNext());
}

void DX9RENDER::DrawPrimitiveUP(storm_render::Primitive dwPrimitiveType, uint32_t dwVertexBufferFormat, uint32_t dwNumPT,
                                const void *pVerts, uint32_t dwStride, const char *cBlockName)
{
    bool bDraw = true;

    if (CHECKD3DERR(SetFVF(dwVertexBufferFormat)) == true)
        return;

    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);
    if (bDraw)
        do
        {
            dwNumDrawPrimitive++;
#ifdef STORM_METAL_RENDERER_CONSUMERS
            if (!StormMetalDrawCompactPrimitive(renderDevice, dwPrimitiveType, dwVertexBufferFormat, dwNumPT,
                                                 pVerts, dwStride))
                CHECKD3DERR(renderDevice->DrawPrimitiveUP(dwPrimitiveType, dwNumPT, pVerts, dwStride));
#else
            CHECKD3DERR(renderDevice->DrawPrimitiveUP(dwPrimitiveType, dwNumPT, pVerts, dwStride));
#endif
        } while (cBlockName && TechniqueExecuteNext());
}

void DX9RENDER::DrawPrimitive(storm_render::Primitive dwPrimitiveType, int32_t iVBuff, int32_t iStride, int32_t iStartV, int32_t iNumPT,
                              const char *cBlockName)
{
    bool bDraw = true;

    if (iVBuff < 0 || iVBuff >= MAX_BUFFERS || VertexBuffers[iVBuff].buff == nullptr)
    {
        ReportRejectedBuffer("DrawPrimitive", iVBuff, -1);
        return;
    }

    if (CHECKD3DERR(SetFVF(VertexBuffers[iVBuff].type)) == true)
        return;

    if (CHECKD3DERR(renderDevice->SetStreamSource(0, VertexBuffers[iVBuff].buff, 0, iStride)) == true)
        return;

    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);
    if (bDraw)
        do
        {
            dwNumDrawPrimitive++;
            CHECKD3DERR(renderDevice->DrawPrimitive(dwPrimitiveType, iStartV, iNumPT));
        } while (cBlockName && TechniqueExecuteNext());
}

//################################################################################
void DX9RENDER::RenderAnimation(int32_t ib, void *src, int32_t numVrts, int32_t minv, int32_t numv, int32_t startidx, int32_t numtrg,
                                bool isUpdateVB)
{
    if (numVrts <= 0 || !src || ib < 0)
        return;
    const uint32_t type = storm_render::vertexFormat::xyz | storm_render::vertexFormat::normal | storm_render::vertexFormat::tex1;
    const int32_t size = numVrts * sizeof(FVF_VERTEX);
    if (isUpdateVB || numVrts > numAniVerteces || !aniVBuffer)
    {
        // Create vertex buffer
        if (numVrts > numAniVerteces)
        {
            if (aniVBuffer)
                aniVBuffer->Release();
            aniVBuffer = nullptr;
            numAniVerteces = 0;
            if (CHECKD3DERR(renderDevice->CreateVertexBuffer(size, storm_render::usage::writeOnly, type, &aniVBuffer)) == true)
                return;
            numAniVerteces = numVrts;
        }
        // Copy verteces
        uint8_t *ptr;
        RDTSC_B(renderTimestamp);
        if (CHECKD3DERR(aniVBuffer->Lock(0, size, (void **)&ptr, 0)) == true)
            return;
        dwNumLV++;
        RDTSC_E(renderTimestamp);
        memcpy(ptr, src, size);
        CHECKD3DERR(aniVBuffer->Unlock());
    }
    // Render
    if (CHECKD3DERR(SetFVF(type)) == true)
        return;

    if (ib >= MAX_BUFFERS || IndexBuffers[ib].buff == nullptr)
    {
        ReportRejectedBuffer("RenderAnimation", -1, ib);
        return;
    }

    if (CHECKD3DERR(renderDevice->SetIndices(IndexBuffers[ib].buff)) == true)
        return;

    if (CHECKD3DERR(renderDevice->SetStreamSource(0, aniVBuffer, 0, sizeof(FVF_VERTEX))) == true)
        return;

    CHECKD3DERR(renderDevice->DrawIndexedPrimitive(storm_render::PrimitiveTriangleList, minv, 0, numv, startidx, numtrg));

    dwNumDrawPrimitive++;
}

//################################################################################
void *DX9RENDER::LockVertexBuffer(int32_t id, uint32_t dwFlags)
{
    uint8_t *ptr;
    if (id < 0 || id >= MAX_BUFFERS || VertexBuffers[id].buff == nullptr)
    {
        ReportRejectedBuffer("LockVertexBuffer", id, -1);
        return nullptr;
    }
    VertexBuffers[id].dwNumLocks++;
    if (CHECKD3DERR(VertexBuffers[id].buff->Lock(0, VertexBuffers[id].size, (void **)&ptr, dwFlags)))
        return nullptr;

    dwNumLV++;
    return ptr;
}

//################################################################################
void DX9RENDER::UnLockVertexBuffer(int32_t id)
{
        if (id < 0 || id >= MAX_BUFFERS || VertexBuffers[id].buff == nullptr)
            return;
        VertexBuffers[id].dwNumLocks--;
        CHECKD3DERR(VertexBuffers[id].buff->Unlock());
}

int32_t DX9RENDER::GetVertexBufferSize(int32_t id)
{
    if (id < 0 || id >= MAX_BUFFERS)
        return 0;
    return VertexBuffers[id].size;
}

void *DX9RENDER::LockIndexBuffer(int32_t id, uint32_t dwFlags)
{
    uint8_t *ptr = nullptr;
    if (id < 0 || id >= MAX_BUFFERS || IndexBuffers[id].buff == nullptr)
    {
        ReportRejectedBuffer("LockIndexBuffer", -1, id);
        return nullptr;
    }
    IndexBuffers[id].dwNumLocks++;
    if (CHECKD3DERR(IndexBuffers[id].buff->Lock(0, IndexBuffers[id].size, (void **)&ptr, dwFlags)))
        return nullptr;

    dwNumLI++;
    return ptr;
}

void DX9RENDER::UnLockIndexBuffer(int32_t id)
{
        if (id < 0 || id >= MAX_BUFFERS || IndexBuffers[id].buff == nullptr)
            return;
        IndexBuffers[id].dwNumLocks--;
        CHECKD3DERR(IndexBuffers[id].buff->Unlock());
}

//################################################################################
void DX9RENDER::ReleaseVertexBuffer(int32_t id)
{
    if (id < 0 || id >= MAX_BUFFERS || VertexBuffers[id].buff == nullptr)
        return;
    CHECKD3DERR(VertexBuffers[id].buff->Release());
    VertexBuffers[id].buff = nullptr;
    VertexBuffers[id].dwNumLocks = 0;
}

//################################################################################
void DX9RENDER::ReleaseIndexBuffer(int32_t id)
{
    if (id < 0 || id >= MAX_BUFFERS || IndexBuffers[id].buff == nullptr)
        return;
    CHECKD3DERR(IndexBuffers[id].buff->Release());
    IndexBuffers[id].buff = nullptr;
    IndexBuffers[id].dwNumLocks = 0;
}

void DX9RENDER::SetTransform(int32_t type, storm_render::Matrix *mtx)
{
    CMatrix m = *(CMatrix *)mtx;
    if (type == storm_render::TransformView)
    {
        vViewRelationPos.x = -mtx->_41;
        vViewRelationPos.y = -mtx->_42;
        vViewRelationPos.z = -mtx->_43;

        CVECTOR vDeltaWorld = vWordRelationPos;
        m.MulToInvNorm(-vViewRelationPos, vWordRelationPos);
        vDeltaWorld -= vWordRelationPos;
        CMatrix mw;
        renderDevice->GetTransform(storm_render::TransformWorld, (storm_render::Matrix *)&mw);
        mw.Pos() -= vDeltaWorld;
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
        StormMetalRebaseWorld(renderDevice, mw);
#else
        renderDevice->SetTransform(storm_render::TransformWorld, mw);
#endif

        m.Pos() += vViewRelationPos;
    }
    else if (type == storm_render::TransformWorld)
    {
        m.Pos() += vWordRelationPos;
    }

    CHECKD3DERR(renderDevice->SetTransform(static_cast<storm_render::TransformSlot>(type), (storm_render::Matrix *)&m));
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    if (type == storm_render::TransformWorld)
        StormMetalSetAuthoredWorld(renderDevice, &mtx->_11);
#endif
}

void DX9RENDER::GetTransform(int32_t type, storm_render::Matrix *mtx)
{
#ifdef STORM_METAL_DYNAMIC_LIGHTING_BRIDGE
    if (type == storm_render::TransformWorld && mtx && StormMetalGetAuthoredWorld(renderDevice, &mtx->_11)) return;
#endif
    CHECKD3DERR(renderDevice->GetTransform(static_cast<storm_render::TransformSlot>(type), mtx));

    if (type == storm_render::TransformView)
    {
        mtx->_41 -= vViewRelationPos.x;
        mtx->_42 -= vViewRelationPos.y;
        mtx->_43 -= vViewRelationPos.z;
    }
    else if (type == storm_render::TransformWorld)
    {
        mtx->_41 -= vWordRelationPos.x;
        mtx->_42 -= vWordRelationPos.y;
        mtx->_43 -= vWordRelationPos.z;
    }
}

bool DX9RENDER::CreateState(ENTITY_STATE_GEN *state_gen)
{
    // state_gen->SetState("vm",sizeof(screen_size),screen_size,sizeof(bool),&window);
    return true;
}

bool DX9RENDER::LoadState(ENTITY_STATE *state)
{
    // GUARD(DX9RENDER::Init)
    // for(int32_t t=0; t<MAX_STEXTURES; t++)    Textures[t].d3dtex = NULL;
    // renderDevice = NULL;
    // state->Struct(sizeof(screen_size),(char *)&screen_size);
    // state->MemoryBlock(sizeof(bool),(char *)&window);
    // InitDevice(window,core.GetAppHWND(),screen_size.x,screen_size.y);
    // UNGUARD
    return true;
}

void DX9RENDER::LostRender()
{
    if (resourcesReleased)
    {
        return;
    }

    // Metal keeps references to bound geometry. Drop them before the legacy
    // table releases its own references, or the refcount assertion will trap.
    renderDevice->SetIndices(nullptr);
    for (uint32_t stream = 0; stream < 16; ++stream)
        renderDevice->SetStreamSource(stream, nullptr, 0, 0);

    InvokeEntitiesLostRender();

    Release(pOriginalScreenSurface);
    Release(pOriginalDepthSurface);
    Release(rectsVBuffer);
    for (int32_t b = 0; b < MAX_BUFFERS; b++)
    {
        if (VertexBuffers[b].buff)
        {
            if (VertexBuffers[b].buff->Release() > 0)
                psnip_trap();
        }
        if (IndexBuffers[b].buff)
        {
            if (IndexBuffers[b].buff->Release() > 0)
                psnip_trap();
        }
    }

    resourcesReleased = true;
}

void DX9RENDER::RestoreRender()
{
    renderDevice->GetRenderTarget(&pOriginalScreenSurface);
    renderDevice->GetDepthStencilSurface(&pOriginalDepthSurface);
    fSmallWidth = 128;
    fSmallHeight = 128;
    if (bPostProcessEnabled)
    {
        renderDevice->CreateTexture(screen_size.x, screen_size.y, 1, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, &pPostProcessTexture);
        renderDevice->CreateTexture(static_cast<int>(fSmallWidth), static_cast<int>(fSmallHeight), 1, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, &pSmallPostProcessTexture);
        renderDevice->CreateTexture(static_cast<int>(fSmallWidth * 2.0f), static_cast<int>(fSmallHeight * 2.0f), 1, storm_render::usage::renderTarget, storm_render::AssetFormat::A8R8G8B8, &pSmallPostProcessTexture2);
    }
    if (!pPostProcessTexture || !pSmallPostProcessTexture || !pSmallPostProcessTexture2)
    {
        bPostProcessEnabled = false;
        bPostProcessError = true;
    }
    if (!bPostProcessError)
    {
        pPostProcessTexture->GetSurfaceLevel(0, &pPostProcessSurface);
        pSmallPostProcessTexture2->GetSurfaceLevel(0, &pSmallPostProcessSurface2);
        pSmallPostProcessTexture->GetSurfaceLevel(0, &pSmallPostProcessSurface);

        if (!pPostProcessSurface || !pSmallPostProcessSurface2 || !pSmallPostProcessSurface)
        {
            bPostProcessEnabled = false;
            bPostProcessError = true;
        }
    }
    if (!bPostProcessError)
    {
        ClearPostProcessSurface(pPostProcessSurface);
        ClearPostProcessSurface(pSmallPostProcessSurface);
        ClearPostProcessSurface(pSmallPostProcessSurface2);
    }
    ClearPostProcessSurface(pOriginalScreenSurface);
    renderDevice->CreateVertexBuffer(rectsVBuffer_SizeInRects * 6 * sizeof(RECT_VERTEX), storm_render::usage::writeOnly | storm_render::usage::dynamic, RS_RECT_VERTEX_FORMAT, &rectsVBuffer);
    for (int32_t b = 0; b < MAX_BUFFERS; b++)
    {
        if (VertexBuffers[b].buff)
        {
            CHECKD3DERR(renderDevice->CreateVertexBuffer(VertexBuffers[b].size, VertexBuffers[b].dwUsage, VertexBuffers[b].type, &VertexBuffers[b].buff));
        }
        if (IndexBuffers[b].buff)
        {
            CHECKD3DERR(renderDevice->CreateIndexBuffer(IndexBuffers[b].size, IndexBuffers[b].dwUsage, storm_render::IndexFormat::UInt16, &IndexBuffers[b].buff));
        }
    }
    int32_t num_stages;
    num_stages = 8;
    for (int32_t s = 0; s < num_stages; s++)
    {
        // texture operation
        SetTextureStageState(s, storm_render::StageColorArg1, storm_render::textureArg::current);
        SetTextureStageState(s, storm_render::StageColorArg2, storm_render::textureArg::texture);
        SetTextureStageState(s, storm_render::StageColorOp, storm_render::TextureOpDisable);

        // texture coord
        SetTextureStageState(s, storm_render::StageTexCoordIndex, s);

        // texture filtering
        SetSamplerState(s, storm_render::SamplerMagFilter, storm_render::TextureFilterLinear);
        SetSamplerState(s, storm_render::SamplerMinFilter, storm_render::TextureFilterLinear);
        SetSamplerState(s, storm_render::SamplerMipFilter, storm_render::TextureFilterLinear);
    }
    // set base texture and diffuse+specular lighting
    SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    SetTextureStageState(0, storm_render::StageColorOp, storm_render::TextureOpAddSigned);
    storm_render::Light l{};
    l.Type = storm_render::LightKind::Point;
    l.Range = 100.0f;
    l.Attenuation0 = 1.0f;
    for (int i = 0; i < 8; i++)
    {
        CHECKD3DERR(renderDevice->SetLight(i, &l));
        CHECKD3DERR(renderDevice->LightEnable(i, false));
    }
    SetCommonStates();

#ifdef _WIN32 // Effects
    RecompileEffects();
#else
    pTechnique = std::make_unique<CTechnique>(this);
    pTechnique->DecodeFiles();
#endif

    InvokeEntitiesRestoreRender();

    resourcesReleased = false;
}

void DX9RENDER::RecompileEffects()
{
#ifdef _WIN32 // Effects
    effects_.release();

    std::filesystem::path cur_path = std::filesystem::current_path();
    std::filesystem::current_path(std::filesystem::u8path(fio->_GetExecutableDirectory()));
    for (const auto &p : std::filesystem::recursive_directory_iterator("resource/techniques"))
        if (is_regular_file(p) && p.path().extension() == ".fx")
        {
            auto s = p.path().string(); // hug microsoft
            effects_.compile(s.c_str());
        }
    std::filesystem::current_path(cur_path);
#endif
}

int32_t DX9RENDER::ApplyMetalGraphics(uint32_t flagsMask, uint32_t shadowQuality, int32_t fullscreen,
                                      int32_t renderWidth, int32_t renderHeight, int32_t requestedDesktopMode)
{
    constexpr uint32_t knownFlags = storm_metal::knownGraphicsFlags;
    std::fprintf(stderr, "[StormMetal graphics] apply request flags=%u quality=%u fullscreen=%d size=%dx%d desktop=%d\n",
                 flagsMask, shadowQuality, fullscreen, renderWidth, renderHeight, requestedDesktopMode);
    if ((flagsMask & ~knownFlags) != 0 || shadowQuality > 2 || (fullscreen != 0 && fullscreen != 1) ||
        (requestedDesktopMode != 0 && requestedDesktopMode != 1) || renderWidth < 320 || renderHeight < 200 ||
        (requestedDesktopMode != 0 && fullscreen == 0) || renderWidth > 16384 || renderHeight > 16384 || !renderDevice)
    {
        std::fprintf(stderr, "[StormMetal graphics] reject=invalid-payload\n");
        return 0;
    }

#ifndef __APPLE__
    return 0;
#else
    auto *window = core.GetWindow();
    if (!window || pendingMetalGraphics.queued || metalGraphicsApplyResult != 0 ||
        !StormMetalCanApplyGraphicsSettings(renderDevice, flagsMask, shadowQuality))
    {
        std::fprintf(stderr, "[StormMetal graphics] reject=unsafe-phase window=%d queued=%d result=%d\n",
                     window ? 1 : 0, pendingMetalGraphics.queued ? 1 : 0, metalGraphicsApplyResult);
        return 0;
    }

    pendingMetalGraphics = {flagsMask, shadowQuality, fullscreen, renderWidth, renderHeight, requestedDesktopMode, true};
    std::fprintf(stderr, "[StormMetal graphics] queued until renderer RunStart\n");
    return 3;
#endif
}

int32_t DX9RENDER::ConsumeMetalGraphicsApplyResult()
{
    const auto result = metalGraphicsApplyResult;
    metalGraphicsApplyResult = 0;
    return result;
}

void DX9RENDER::ProcessPendingMetalGraphics()
{
    if (!pendingMetalGraphics.queued)
        return;

    const auto request = pendingMetalGraphics;
    pendingMetalGraphics.queued = false;
    metalGraphicsApplyResult = 2;

#ifndef __APPLE__
    return;
#else
    auto *window = core.GetWindow();
    if (!window || !renderDevice || bInsideScene || resourcesReleased || !stRenderTarget.empty() ||
        !StormMetalCanApplyGraphicsSettings(renderDevice, request.flagsMask, request.shadowQuality))
    {
        std::fprintf(stderr, "[StormMetal graphics] deferred reject=unsafe-frame window=%d scene=%d released=%d targets=%zu\n",
                     window ? 1 : 0, bInsideScene ? 1 : 0, resourcesReleased ? 1 : 0, stRenderTarget.size());
        return;
    }

    if (!StormMetalApplyGraphicsSettings(renderDevice, request.flagsMask, request.shadowQuality))
    {
        std::fprintf(stderr, "[StormMetal graphics] deferred reject=backend-apply\n");
        return;
    }

    metalGraphicsApplyResult = 1;
    std::fprintf(stderr, "[StormMetal graphics] deferred success size=%dx%d fullscreen=%d desktop=%d\n",
                 screen_size.x, screen_size.y, bWindow ? 0 : 1, desktopMode ? 1 : 0);
#endif
}

bool DX9RENDER::ResetDevice()
{
    LostRender();

    if (CHECKD3DERR(renderDevice->Reset(&d3dpp)))
        return false;

    RestoreRender();

    return true;
}

void DX9RENDER::SetGLOWParams(float _fBlurBrushSize, int32_t _GlowIntensity, int32_t _GlowPasses)
{
    fBlurSize = _fBlurBrushSize;
    GlowIntensity = _GlowIntensity;
    iBlurPasses = _GlowPasses;
}

void DX9RENDER::RunStart()
{
    ProcessPendingMetalGraphics();

    auto *pScriptRender = static_cast<VDATA *>(core.GetScriptVariable("Render"));
    ATTRIBUTES *pARender = pScriptRender->GetAClass();

    bSeaEffect = pARender->GetAttributeAsDword("SeaEffect", 0) != 0;
    fSeaEffectSize = pARender->GetAttributeAsFloat("SeaEffectSize", 0.0f);
    fSeaEffectSpeed = pARender->GetAttributeAsFloat("SeaEffectSpeed", 0.0f);
    dwBackColor = pARender->GetAttributeAsDword("BackColor", 0);

    if (bSeaEffect)
    {
        fSin += static_cast<float>(core.GetRDeltaTime()) * 0.001f * fSeaEffectSpeed;

        const auto sx = static_cast<float>(screen_size.x);
        const auto sy = static_cast<float>(screen_size.y);

        const auto fDX = static_cast<float>(static_cast<int32_t>(sx * fSeaEffectSize));
        const float fDY = static_cast<float>(static_cast<int32_t>(sy * fSeaEffectSize));

        const float sx2 = sx + fDX * 2.0f;
        const float sy2 = sy + fDY * 2.0f;

        for (int32_t y = 0; y < 32; y++)
        {
            for (int32_t x = 0; x < 32; x++)
            {
                qv[x + y * 32].vPos =
                    Vector4(sx * static_cast<float>(x) / 31.0f, sy * static_cast<float>(y) / 31.0f, 0.0f, 1.0f);

                qv[x + y * 32].u0 = (sx * static_cast<float>(x) / 31.0f +
                                     sinf(fSin + static_cast<float>(x) / 31.0f * PI * 16.0f) * fDX + fDX) /
                                    sx2;
                qv[x + y * 32].v0 = (sy * static_cast<float>(y) / 31.0f +
                                     sinf(fSin + static_cast<float>(y) / 31.0f * PI * 16.0f) * fDY + fDY) /
                                    sy2;

                // POST_PROCESS_FVF exposes four texture-coordinate sets and
                // the authored postprocess shader samples all four stages.
                // The sea distortion grid historically initialized only UV0,
                // leaving UV1..UV3 as indeterminate memory. Direct Metal reads
                // those channels exactly, which produced camera-dependent
                // magenta/green wedges and apparent frame displacement.
                qv[x + y * 32].u1 = qv[x + y * 32].u0;
                qv[x + y * 32].v1 = qv[x + y * 32].v0;
                qv[x + y * 32].u2 = qv[x + y * 32].u0;
                qv[x + y * 32].v2 = qv[x + y * 32].v0;
                qv[x + y * 32].u3 = qv[x + y * 32].u0;
                qv[x + y * 32].v3 = qv[x + y * 32].v0;
            }
        }
    }

    bNeedCopyToScreen = true;
    //------------------------------------------
    if ((bPostProcessEnabled || bSeaEffect) && (!bPostProcessError))
    {
        SetPostProcessTextureAsRenderTarget();
    }
    else
    {
        SetScreenAsRenderTarget();
    }

    DX9Clear(storm_render::clear::color | storm_render::clear::depth | ((stencil_format == storm_render::AssetFormat::D24S8) ? storm_render::clear::stencil : 0));

    dwNumDrawPrimitive = 0;
    dwNumLV = 0;
    dwNumLI = 0;
    BeginScene();

    //------------------------------------------
    bInsideScene = true;

    // execute default technique for set default render/texture states
    // if (TechniqueExecuteStart("default")) do{} while (TechniqueExecuteNext());

    // boal del_cheat
    if (core.Controls->GetDebugAsyncKeyState(VK_SHIFT) < 0 && core.Controls->GetDebugAsyncKeyState(VK_F11) < 0)
    {
        InvokeEntitiesLostRender();
#ifdef _WIN32 // Effects
        RecompileEffects();
#else
        pTechnique = std::make_unique<CTechnique>(this);
        pTechnique->DecodeFiles();
#endif
        InvokeEntitiesRestoreRender();
    }

    SetRenderState(storm_render::RenderFillMode, (core.Controls->GetDebugAsyncKeyState('F') < 0) ? storm_render::FillWireframe : storm_render::FillSolid);
    // SetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID); eddy

    PlayToTexture();

    // make screenshot
    if (CONTROL_STATE cs; core.Controls->GetControlState(kKeyTakeScreenshot, cs) 
                          && cs.state == CST_ACTIVATED)
    {
        SaveShoot();
    }
}

void DX9RENDER::RunEnd()
{

    bInsideScene = false;
    DX9EndScene();
    if (progressTexture >= 0)
    {
        const int32_t oldCnt = progressSafeCounter;
        ProgressView();
        progressSafeCounter = oldCnt;
        if (progressSafeCounter >= 50)
        {
            EndProgressView();
        }
        else
            progressSafeCounter++;
    }
}

char Buff_4k[4096];

int32_t DX9RENDER::Print(int32_t x, int32_t y, const char *format, ...)
{
    // GUARD(DX9RENDER::Print)
    if (idFontCurrent < 0 || idFontCurrent >= nFontQuantity)
        return 0;
    if (FontList[idFontCurrent].font == nullptr || FontList[idFontCurrent].ref == 0)
        return 0;

    va_list args;
    va_start(args, format);
    vsnprintf(Buff_4k, sizeof(Buff_4k), format, args);
    va_end(args);

    return FontList[idFontCurrent].font->Print(x, y, Buff_4k);
    // UNGUARD
}

int32_t DX9RENDER::Print(int32_t nFontNum, uint32_t color, int32_t x, int32_t y, const char *format, ...)
{
    // GUARD(DX9RENDER::Print)
    if (nFontNum < 0 || nFontNum >= nFontQuantity)
        return 0;
    if (FontList[nFontNum].font == nullptr || FontList[nFontNum].ref == 0)
        return 0;

    va_list args;
    va_start(args, format);
    vsnprintf(Buff_4k, sizeof(Buff_4k), format, args);
    va_end(args);

    const int32_t retVal = FontList[nFontNum].font->Print(x, y, Buff_4k, {.color = color});
    return retVal;
    // UNGUARD
}

int32_t DX9RENDER::StringWidth(const char *string, int32_t nFontNum, float fScale, int32_t scrWidth)
{
    if (string == nullptr)
    {
        return 0;
    }
    return StringWidth(std::string_view(string), nFontNum, fScale, scrWidth);
}

int32_t DX9RENDER::StringWidth(const std::string_view &string, int32_t nFontNum, float fScale, int32_t scrWidth)
{
    if (nFontNum < 0 || nFontNum >= nFontQuantity)
        return 0;
    FONT *pFont = FontList[nFontNum].font;
    if (FontList[nFontNum].ref == 0 || pFont == nullptr)
        return 0;


    const int32_t xs = screen_size.x;
    if (scrWidth == 0)
        scrWidth = xs;
    if (xs != scrWidth)
        fScale *= static_cast<float>(xs) / scrWidth;
    const int32_t retVal = pFont->GetStringWidth(string, fScale);
    return retVal;
}

int32_t DX9RENDER::CharWidth(utf8::u8_char ch, int32_t nFontNum, float fScale, int32_t scrWidth)
{
    std::string str(ch.b, ch.l);
    return StringWidth(str.c_str(), nFontNum, fScale, scrWidth);
}

int32_t DX9RENDER::CharHeight(int32_t fontID)
{
    if (fontID < 0 || fontID >= nFontQuantity)
        return 0;
    if (FontList[fontID].ref == 0 || FontList[fontID].font == nullptr)
        return 0;

    return FontList[fontID].font->GetHeight();
}

int32_t DX9RENDER::ExtPrint(int32_t nFontNum, uint32_t foreColor, uint32_t backColor, int wAlign, bool bShadow, float fScale,
                         int32_t scrWidth, int32_t scrHeight, int32_t x, int32_t y, const char *format, ...)
{
    // GUARD(DX9RENDER::ExtPrint)

    if (nFontNum < 0 || nFontNum >= nFontQuantity)
        return 0;
    FONT *pFont = FontList[nFontNum].font;
    if (FontList[nFontNum].ref == 0 || pFont == nullptr)
        return 0;

    va_list args;
    va_start(args, format);
    vsnprintf(Buff_4k, sizeof(Buff_4k), format, args);
    va_end(args);

    storm_render::Surface *pRenderTarget;
    GetRenderTarget(&pRenderTarget);
    storm_render::SurfaceDesc dscrSurface;
    pRenderTarget->GetDesc(&dscrSurface);
    const int32_t xs = dscrSurface.Width;
    const int32_t ys = dscrSurface.Height;
    pRenderTarget->Release();
    if (scrWidth == 0)
        scrWidth = xs;
    if (scrHeight == 0)
        scrHeight = ys;
    if (xs != scrWidth)
    {
        const float fHorzScale = static_cast<float>(xs) / scrWidth;
        x = static_cast<int32_t>(x * fHorzScale);
        fScale *= fHorzScale;
    }
    if (ys != scrHeight)
        y = static_cast<int32_t>(y * (static_cast<float>(ys) / scrHeight));

    switch (wAlign)
    {
    case PR_ALIGN_CENTER:
        x -= static_cast<int32_t>((pFont->GetStringWidth(Buff_4k) + 1L) / 2L * fScale);
        break;
    case PR_ALIGN_RIGHT:
        x -= static_cast<int32_t>(pFont->GetStringWidth(Buff_4k) * fScale);
        break;
    }

    const int32_t retVal = pFont->Print(x, y, Buff_4k,
                                        {
                                            .scale = fScale,
                                            .color = foreColor,
                                            .shadow = bShadow,
                                        });
    return retVal;
    // UNGUARD
}

int32_t DX9RENDER::LoadFont(const char *fontName)
{
    if (fontName == nullptr)
        return -1L;
    char sDup[256];
    if (strlen(fontName) < sizeof(sDup) - 1)
        strcpy_s(sDup, fontName);
    else
    {
        strncpy_s(sDup, fontName, sizeof(sDup) - 1);
        sDup[sizeof(sDup) - 1] = 0;
    }

    std::ranges::for_each(sDup, [](char &c) { c = std::toupper(c); });
    fontName = sDup;

    const uint32_t hashVal = MakeHashValue(fontName);

    int32_t i;
    for (i = 0; i < nFontQuantity; i++)
        if (FontList[i].hash == hashVal && storm::iEquals(FontList[i].name, fontName))
        {
            if (FontList[i].ref > 0)
                FontList[i].ref++;
            else
            {
                FontList[i].ref = 1;
                FontList[i].font->RepeatInit();
            }
            return i;
        }
    if (nFontQuantity < MAX_FONTS)
    {
        if ((FontList[i].font = new FONT(*this, *renderDevice)) == nullptr)
            throw std::runtime_error("allocate memory error");
        if (!FontList[i].font->Init(fontName, fontIniFileName))
        {
            delete FontList[i].font;
            core.Trace("Can't init font %s", fontName);
            return -1L;
        }
        FontList[i].hash = hashVal;
        FontList[i].ref = 1;
        const auto len = strlen(fontName) + 1;
        if ((FontList[i].name = new char[len]) == nullptr)
            throw std::runtime_error("allocate memory error");
        strcpy_s(FontList[i].name, len, fontName);
        nFontQuantity++;
    }
    else
        throw std::runtime_error("maximal font quantity exceeded");
    return i;
}

bool DX9RENDER::UnloadFont(const char *fontName)
{
    if (fontName == nullptr)
        return false;
    char sDup[256];
    if (strlen(fontName) < sizeof(sDup) - 1)
        strcpy_s(sDup, fontName);
    else
    {
        strncpy_s(sDup, fontName, sizeof(sDup) - 1);
        sDup[sizeof(sDup) - 1] = 0;
    }
    std::ranges::for_each(sDup, [](char &c) { c = std::toupper(c); });
    fontName = sDup;
    const uint32_t hashVal = MakeHashValue(fontName);

    for (int i = 0; i < nFontQuantity; i++)
        if (FontList[i].hash == hashVal && storm::iEquals(FontList[i].name, fontName))
            return UnloadFont(i);
    core.Trace("Font name \"%s\" is not containing", fontName);
    return false;
}

bool DX9RENDER::UnloadFont(int32_t fontID)
{
    if (fontID < 0 || fontID >= nFontQuantity)
        return false;

    if (FontList[fontID].ref > 0)
    {
        FontList[fontID].ref--;
        if (FontList[fontID].ref == 0)
        {
            FontList[fontID].font->TempUnload();
            // if (idFontCurrent == fontID)
            idFontCurrent = FONT_DEFAULT;
        }
        else
            return false;
    }

    return true;
}

bool DX9RENDER::IncRefCounter(int32_t fontID)
{
    if (fontID < 0 || fontID >= nFontQuantity)
        return false;

    FontList[fontID].ref++;

    return true;
}

bool DX9RENDER::SetCurFont(const char *fontName)
{
    if (fontName == nullptr)
        return false;
    char sDup[256];
    if (strlen(fontName) < sizeof(sDup) - 1)
        strcpy_s(sDup, fontName);
    else
    {
        strncpy_s(sDup, fontName, sizeof(sDup) - 1);
        sDup[sizeof(sDup) - 1] = 0;
    }
    std::ranges::for_each(sDup, [](char &c) { c = std::toupper(c); });
    fontName = sDup;
    const uint32_t hashVal = MakeHashValue(fontName);

    for (int i = 0; i < nFontQuantity; i++)
        if (FontList[i].hash == hashVal)
        {
            idFontCurrent = i;
            return true;
        }
    core.Trace("Font name \"%s\" is not containing", fontName);
    return false;
}

bool DX9RENDER::SetCurFont(int32_t fontID)
{
    if (fontID < 0 || fontID >= nFontQuantity)
        return false;
    idFontCurrent = fontID;
    return true;
}

int32_t DX9RENDER::GetCurFont()
{
    if (idFontCurrent >= 0 && idFontCurrent < nFontQuantity)
        return idFontCurrent;
    return -1L;
}

char *DX9RENDER::GetFontIniFileName()
{
    return fontIniFileName;
}

bool DX9RENDER::SetFontIniFileName(const char *iniName)
{
    if (fontIniFileName != nullptr && iniName != nullptr && storm::iEquals(fontIniFileName, iniName))
        return true;

    delete fontIniFileName;
    if (iniName == nullptr)
    {
        fontIniFileName = nullptr;
        return false;
    }
    const auto len = strlen(iniName) + 1;
    if ((fontIniFileName = new char[len]) == nullptr)
        throw std::runtime_error("allocate memory error");
    strcpy_s(fontIniFileName, len, iniName);

    for (int n = 0; n < nFontQuantity; n++)
    {
        delete FontList[n].font;

        if ((FontList[n].font = new FONT(*this, *renderDevice)) == nullptr)
            throw std::runtime_error("allocate memory error");
        FontList[n].font->Init(FontList[n].name, fontIniFileName);
        if (FontList[n].ref == 0)
            FontList[n].font->TempUnload();
    }

    return true;
}

void DX9RENDER::SetCommonStates()
{
    SetRenderState(storm_render::RenderDither, TRUE);
    SetRenderState(storm_render::RenderAlphaTest, true);
    //----------Z---------
    SetRenderState(storm_render::RenderDepthTest, storm_render::DepthEnabled);
    SetRenderState(storm_render::RenderDepthWrite, TRUE);
    SetRenderState(storm_render::RenderDepthCompare, storm_render::CompareLessEqual);

    SetRenderState(storm_render::RenderCullMode, storm_render::CullCounterClockwise);
    SetRenderState(storm_render::RenderBlend, true);
    SetSamplerState(0, storm_render::SamplerMagFilter, storm_render::TextureFilterLinear);
    SetSamplerState(0, storm_render::SamplerMinFilter, storm_render::TextureFilterLinear);
    SetSamplerState(0, storm_render::SamplerMipFilter, storm_render::TextureFilterLinear);

    SetRenderState(storm_render::RenderFogTableMode, storm_render::FogExponential);

    SetRenderState(storm_render::RenderLighting, TRUE);
    SetRenderState(storm_render::RenderLocalViewer, FALSE);
    SetRenderState(storm_render::RenderAmbient, 0x505050);
    SetRenderState(storm_render::RenderAmbientMaterialSource, storm_render::MaterialSourceDiffuseColor);

    SetRenderState(storm_render::RenderSpecular, FALSE);
    SetRenderState(storm_render::RenderVertexColor, TRUE);
    SetRenderState(storm_render::RenderBlend, TRUE);
    SetRenderState(storm_render::RenderSourceBlend, storm_render::BlendSourceAlpha);
    SetRenderState(storm_render::RenderDestinationBlend, storm_render::BlendInverseSourceAlpha);
    SetRenderState(storm_render::RenderAlphaTest, TRUE);
    SetRenderState(storm_render::RenderAlphaReference, 0xa0);
    SetRenderState(storm_render::RenderAlphaCompare, storm_render::CompareGreater);

    // *****************************************************************88
    // texture filtering
    SetSamplerState(0, storm_render::SamplerMagFilter, storm_render::TextureFilterLinear);
    SetSamplerState(0, storm_render::SamplerMinFilter, storm_render::TextureFilterAnisotropic);
    SetSamplerState(0, storm_render::SamplerMipFilter, storm_render::TextureFilterLinear);
    SetSamplerState(0, storm_render::SamplerMaxAnisotropy, 3);

    // unchanged texture stage states - both for base and detal texture
    SetTextureStageState(0, storm_render::StageColorArg1, storm_render::textureArg::diffuse);
    SetTextureStageState(0, storm_render::StageColorArg2, storm_render::textureArg::texture);
    SetTextureStageState(1, storm_render::StageColorArg1, storm_render::textureArg::current);
    SetTextureStageState(1, storm_render::StageColorArg2, storm_render::textureArg::texture);
    SetTextureStageState(2, storm_render::StageColorArg1, storm_render::textureArg::current);
    SetTextureStageState(2, storm_render::StageColorArg2, storm_render::textureArg::diffuse);

    SetTextureStageState(0, storm_render::StageAlphaArg1, storm_render::textureArg::diffuse);
    SetTextureStageState(0, storm_render::StageAlphaArg2, storm_render::textureArg::texture);
    SetTextureStageState(0, storm_render::StageAlphaOp, storm_render::TextureOpModulate);

    // general
    SetRenderState(storm_render::RenderCullMode, storm_render::CullCounterClockwise);
    SetRenderState(storm_render::RenderBlend, TRUE);
    SetRenderState(storm_render::RenderAlphaTest, TRUE);
    SetRenderState(storm_render::RenderSourceBlend, storm_render::BlendSourceAlpha);
    SetRenderState(storm_render::RenderDestinationBlend, storm_render::BlendInverseSourceAlpha);
    SetRenderState(storm_render::RenderAlphaReference, 0xa0);
    SetRenderState(storm_render::RenderAlphaCompare, storm_render::CompareGreater);

    // lighting effects
    SetRenderState(storm_render::RenderAmbient, 0x404040);
    SetRenderState(storm_render::RenderLighting, FALSE); // TRUE);
    SetRenderState(storm_render::RenderVertexColor, TRUE);
    SetRenderState(storm_render::RenderSpecular, FALSE); // TRUE);

    // AA
    SetRenderState(storm_render::RenderMultisampleAntialias, TRUE);
}

storm_render::Result DX9RENDER::GetViewport(storm_render::Viewport *pViewport)
{
    return CHECKD3DERR(renderDevice->GetViewport(pViewport));
}

storm_render::Result DX9RENDER::SetViewport(const storm_render::Viewport *pViewport)
{
    return CHECKD3DERR(renderDevice->SetViewport(pViewport));
}

uint32_t DX9RENDER::SetRenderState(uint32_t State, uint32_t Value)
{
    return CHECKD3DERR(renderDevice->SetRenderState(static_cast<storm_render::RenderStateField>(State), Value));
}

uint32_t DX9RENDER::GetRenderState(uint32_t State, uint32_t *pValue)
{
    return CHECKD3DERR(renderDevice->GetRenderState(static_cast<storm_render::RenderStateField>(State), (DWORD *)pValue));
}

uint32_t DX9RENDER::GetSamplerState(uint32_t Sampler, storm_render::SamplerField Type, uint32_t *pValue)
{
    return CHECKD3DERR(renderDevice->GetSamplerState(Sampler, Type, (DWORD *)pValue));
}

uint32_t DX9RENDER::SetSamplerState(uint32_t Sampler, storm_render::SamplerField Type, uint32_t Value)
{
    return CHECKD3DERR(renderDevice->SetSamplerState(Sampler, Type, Value));
}

uint32_t DX9RENDER::SetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t Value)
{
    return CHECKD3DERR(renderDevice->SetTextureStageState(Stage, static_cast<storm_render::StageField>(Type), Value));
}

uint32_t DX9RENDER::GetTextureStageState(uint32_t Stage, uint32_t Type, uint32_t *pValue)
{
    return CHECKD3DERR(renderDevice->GetTextureStageState(Stage, static_cast<storm_render::StageField>(Type), (DWORD *)pValue));
}

void DX9RENDER::GetCamera(CVECTOR &pos, CVECTOR &ang, float &perspective)
{
    pos = Pos;
    ang = Ang;
    perspective = Fov;
}

using TGA_H = struct tagTGA_H
{
    uint8_t byte1; // = 0
    uint8_t byte2; // = 0
    uint8_t type;
    uint8_t byte4_9[9]; // = 0
    uint16_t width;
    uint16_t height;
    uint8_t bpp; // bit per pixel
    uint8_t attr : 4;
    uint8_t rez : 1;
    uint8_t origin : 1;
    uint8_t storage : 2;
};

// WORD Temp[1600*4];

void DX9RENDER::SaveShoot()
{
    bMakeShoot = true;
}

void DX9RENDER::MakeScreenShot()
{
    bMakeShoot = false;

    storm_render::Surface *renderTarget;
    if (FAILED(GetRenderTarget(&renderTarget)))
    {
        core.Trace("Failed to make screenshot");
        return;
    }

    storm_render::Surface *surface;
    if (FAILED(CreateOffscreenPlainSurface(screen_size.x, screen_size.y, storm_render::AssetFormat::X8R8G8B8, &surface)))
    {
        renderTarget->Release();
        core.Trace("Failed to make screenshot");
        return;
    }

    if (CHECKD3DERR(storm_render::ImageLoadSurfaceFromSurface(surface, NULL, NULL, renderTarget, NULL, NULL, storm_render::ImageDefault, 0)))
    {
        surface->Release();
        renderTarget->Release();
        core.Trace("Failed to make screenshot");
        return;
    }

    const auto screenshot_base_filename = fmt::format("{:%Y-%m-%d_%H-%M-%S}", fmt::localtime(std::time(nullptr)));
    auto screenshot_path = fs::GetScreenshotsPath() / screenshot_base_filename;
    screenshot_path.replace_extension(screenshotExt);
    for(size_t i = 0; exists(screenshot_path); ++i)
    {
        screenshot_path.replace_filename(screenshot_base_filename + "_" + std::to_string(i));
        screenshot_path.replace_extension(screenshotExt);
    }
#ifdef _WIN32 // Screenshot
    D3DXSaveSurfaceToFile(screenshot_path.c_str(), screenshotFormat, surface, nullptr, nullptr);
#endif

    surface->Release();
    renderTarget->Release();
}

PLANE *DX9RENDER::GetPlanes()
{
    FindPlanes(renderDevice);
    return viewplane;
}

void DX9RENDER::FindPlanes(storm_render::Device *d3dDevice)
{
    storm_render::Matrix m;
    CVECTOR v[4];
    GetTransform(storm_render::TransformProjection, static_cast<storm_render::Matrix *>(&m));
    // left
    v[0].x = m.m[0][0];
    v[0].y = 0.0f;
    v[0].z = 1.0f;
    // right
    v[1].x = -m.m[0][0];
    v[1].y = 0.0f;
    v[1].z = 1.0f;
    // top
    v[2].x = 0.0f;
    v[2].y = -m.m[1][1];
    v[2].z = 1.0f;
    // bottom
    v[3].x = 0.0f;
    v[3].y = m.m[1][1];
    v[3].z = 1.0f;
    v[0] = !v[0];
    v[1] = !v[1];
    v[2] = !v[2];
    v[3] = !v[3];

    GetTransform(storm_render::TransformView, static_cast<storm_render::Matrix *>(&m));
    CVECTOR pos;

    pos.x = -m._41 * m._11 - m._42 * m._12 - m._43 * m._13;
    pos.y = -m._41 * m._21 - m._42 * m._22 - m._43 * m._23;
    pos.z = -m._41 * m._31 - m._42 * m._32 - m._43 * m._33;

    viewplane[0].Nx = v[0].x * m._11 + v[0].y * m._12 + v[0].z * m._13;
    viewplane[0].Ny = v[0].x * m._21 + v[0].y * m._22 + v[0].z * m._23;
    viewplane[0].Nz = v[0].x * m._31 + v[0].y * m._32 + v[0].z * m._33;

    viewplane[1].Nx = v[1].x * m._11 + v[1].y * m._12 + v[1].z * m._13;
    viewplane[1].Ny = v[1].x * m._21 + v[1].y * m._22 + v[1].z * m._23;
    viewplane[1].Nz = v[1].x * m._31 + v[1].y * m._32 + v[1].z * m._33;

    viewplane[2].Nx = v[2].x * m._11 + v[2].y * m._12 + v[2].z * m._13;
    viewplane[2].Ny = v[2].x * m._21 + v[2].y * m._22 + v[2].z * m._23;
    viewplane[2].Nz = v[2].x * m._31 + v[2].y * m._32 + v[2].z * m._33;

    viewplane[3].Nx = v[3].x * m._11 + v[3].y * m._12 + v[3].z * m._13;
    viewplane[3].Ny = v[3].x * m._21 + v[3].y * m._22 + v[3].z * m._23;
    viewplane[3].Nz = v[3].x * m._31 + v[3].y * m._32 + v[3].z * m._33;

    viewplane[0].D = (pos.x * viewplane[0].Nx + pos.y * viewplane[0].Ny + pos.z * viewplane[0].Nz);
    viewplane[1].D = (pos.x * viewplane[1].Nx + pos.y * viewplane[1].Ny + pos.z * viewplane[1].Nz);
    viewplane[2].D = (pos.x * viewplane[2].Nx + pos.y * viewplane[2].Ny + pos.z * viewplane[2].Nz);
    viewplane[3].D = (pos.x * viewplane[3].Nx + pos.y * viewplane[3].Ny + pos.z * viewplane[3].Nz);
}

#ifdef _WIN32 // Effects
bool DX9RENDER::TechniqueExecuteStart(const char *cBlockName)
{
    if (!cBlockName)
        return false;
    return effects_.begin(cBlockName);
}
#else
bool DX9RENDER::TechniqueExecuteStart(const char *cBlockName)
{
    if (!cBlockName)
        return false;
    pTechnique->SetCurrentBlock(cBlockName, 0, nullptr);
    return pTechnique->ExecutePassStart();
}
#endif

bool DX9RENDER::TechniqueExecuteNext()
{
#ifdef _WIN32 // Effects
    return effects_.next();
#else
    return pTechnique->ExecutePassNext();
#endif
}

void DX9RENDER::DrawRects(RS_RECT *pRSR, uint32_t dwRectsNum, const char *cBlockName, uint32_t dwSubTexturesX,
                          uint32_t dwSubTexturesY, float fScaleX, float fScaleY)
{
    if (!pRSR || dwRectsNum == 0 || !rectsVBuffer)
        return;

    bool bDraw = true;

    static CMatrix camMtx, oldWorldMatrix, IMatrix;
    renderDevice->GetTransform(storm_render::TransformView, camMtx);
    renderDevice->GetTransform(storm_render::TransformWorld, oldWorldMatrix);

    fScaleY *= GetHeightDeformator();

    float du, dv;
    bool bUseSubTextures = false;
    if (dwSubTexturesX > 1)
    {
        bUseSubTextures = true;
        du = 1.0f / dwSubTexturesX;
    }
    else
    {
        dwSubTexturesX = 1;
        du = 1.0f;
    }
    if (dwSubTexturesY > 1)
    {
        bUseSubTextures = true;
        dv = 1.0f / dwSubTexturesY;
    }
    else
    {
        dwSubTexturesY = 1;
        dv = 1.0f;
    }

    renderDevice->SetTransform(storm_render::TransformView, IMatrix);
    renderDevice->SetTransform(storm_render::TransformWorld, IMatrix);

    for (uint32_t cnt = 0; cnt < dwRectsNum;)
    {
        // Number of rectangles to draw at a time
        uint32_t drawCount = dwRectsNum - cnt;
        if (drawCount > rectsVBuffer_SizeInRects)
            drawCount = rectsVBuffer_SizeInRects;
        // Buffer
        RECT_VERTEX *data = nullptr;
        if (rectsVBuffer->Lock(0, drawCount * 6 * sizeof(RECT_VERTEX), (void **)&data, storm_render::lock::discard) != storm_render::result::ok)
            return;
        if (!data)
            return;
        // Filling the buffer
        for (uint32_t i = 0; i < drawCount && cnt < dwRectsNum; i++)
        {
            // Local array of a particle
            RECT_VERTEX *buffer = &data[i * 6];
            RS_RECT &rect = pRSR[cnt++];
            CVECTOR pos = camMtx * (rect.vPos + vWordRelationPos);
            const float sizex = rect.fSize * fScaleX;
            const float sizey = rect.fSize * fScaleY;
            const float sn = sinf(rect.fAngle);
            const float cs = cosf(rect.fAngle);
            const int32_t color = rect.dwColor;
            float u1, v1, u2, v2;
            if (!bUseSubTextures)
            {
                v1 = u1 = 0.0f;
                v2 = u2 = 1.0f;
            }
            else
            {
                u1 = (rect.dwSubTexture % dwSubTexturesX) * du;
                v1 = ((rect.dwSubTexture / dwSubTexturesX) % dwSubTexturesY) * dv;
                u2 = u1 + du;
                v2 = v1 + dv;
            }
            // Filling the particle buffer
            buffer[0].pos = pos + CVECTOR(sizex * (-cs - sn), sizey * (sn - cs), 0.0f);
            buffer[0].color = color;
            buffer[0].u = u1;
            buffer[0].v = v2;
            buffer[1].pos = pos + CVECTOR(sizex * (-cs + sn), sizey * (sn + cs), 0.0f);
            buffer[1].color = color;
            buffer[1].u = u1;
            buffer[1].v = v1;
            buffer[2].pos = pos + CVECTOR(sizex * (cs + sn), sizey * (-sn + cs), 0.0f);
            buffer[2].color = color;
            buffer[2].u = u2;
            buffer[2].v = v1;
            buffer[3].pos = buffer[0].pos;
            buffer[3].color = color;
            buffer[3].u = u1;
            buffer[3].v = v2;
            buffer[4].pos = buffer[2].pos;
            buffer[4].color = color;
            buffer[4].u = u2;
            buffer[4].v = v1;
            buffer[5].pos = pos + CVECTOR(sizex * (cs - sn), sizey * (-sn - cs), 0.0f);
            buffer[5].color = color;
            buffer[5].u = u2;
            buffer[5].v = v2;
        }
        // Draw a buffer
        rectsVBuffer->Unlock();
        CHECKD3DERR(SetFVF(RS_RECT_VERTEX_FORMAT));
        if (cBlockName && cBlockName[0])
            bDraw = TechniqueExecuteStart(cBlockName);
        if (bDraw)
            do
            {
                CHECKD3DERR(SetStreamSource(0, rectsVBuffer, sizeof(RECT_VERTEX)));
                CHECKD3DERR(DrawPrimitive(storm_render::PrimitiveTriangleList, 0, drawCount * 2));
            } while (cBlockName && TechniqueExecuteNext());
    }

    renderDevice->SetTransform(storm_render::TransformView, camMtx);
    renderDevice->SetTransform(storm_render::TransformWorld, oldWorldMatrix);
}

void DX9RENDER::DrawSprites(RS_SPRITE *pRSS, uint32_t dwSpritesNum, const char *cBlockName)
{
    uint32_t i;
#define RS_SPRITE_VERTEX_FORMAT (storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1)
    if (dwSpritesNum == 0)
        return;

    auto *pIndices = new uint16_t[dwSpritesNum * 6];

    for (i = 0; i < dwSpritesNum; i++)
    {
        pIndices[i * 6 + 0] = static_cast<uint16_t>(i * 4 + 0);
        pIndices[i * 6 + 1] = static_cast<uint16_t>(i * 4 + 3);
        pIndices[i * 6 + 2] = static_cast<uint16_t>(i * 4 + 2);
        pIndices[i * 6 + 3] = static_cast<uint16_t>(i * 4 + 0);
        pIndices[i * 6 + 4] = static_cast<uint16_t>(i * 4 + 2);
        pIndices[i * 6 + 5] = static_cast<uint16_t>(i * 4 + 1);
    }

    SetFVF(RS_SPRITE_VERTEX_FORMAT);

    bool bDraw = true;
    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);
    if (bDraw)
        do
        {
            DrawIndexedPrimitiveUP(storm_render::PrimitiveTriangleList, 0, dwSpritesNum * 4, dwSpritesNum * 2, pIndices, storm_render::IndexFormat::UInt16,
                                   pRSS, sizeof(RS_SPRITE));
        } while (cBlockName && TechniqueExecuteNext());
    delete[] pIndices;
}

void DX9RENDER::DrawLines(RS_LINE *pRSL, uint32_t dwLinesNum, const char *cBlockName)
{
    if (!pRSL || dwLinesNum == 0)
        return;

#define RS_LINE_VERTEX_FORMAT (storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse)

    bool bDraw = true;

    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);
    if (bDraw)
        do
        {
            DrawPrimitiveUP(storm_render::PrimitiveLineList, RS_LINE_VERTEX_FORMAT, dwLinesNum, pRSL, sizeof(RS_LINE));
        } while (cBlockName && TechniqueExecuteNext());
}

void DX9RENDER::DrawLines2D(RS_LINE2D *pRSL2D, size_t dwLinesNum, const char *cBlockName)
{
    if (!pRSL2D || dwLinesNum == 0)
        return;

#define RS_LINE2D_VERTEX_FORMAT (storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse)

    bool bDraw = true;

    if (cBlockName && cBlockName[0])
        bDraw = TechniqueExecuteStart(cBlockName);
    if (bDraw)
        do
        {
            DrawPrimitiveUP(storm_render::PrimitiveLineList, RS_LINE2D_VERTEX_FORMAT, dwLinesNum, pRSL2D, sizeof(RS_LINE2D));
        } while (cBlockName && TechniqueExecuteNext());
}

//-----------------------
storm_render::Result DX9RENDER::CreateVertexBuffer(UINT Length, uint32_t Usage, uint32_t FVF, storm_render::MemoryClass Pool,
                                      storm_render::VertexBuffer **ppVertexBuffer)
{
    return CHECKD3DERR(renderDevice->CreateVertexBuffer(Length, Usage, FVF, ppVertexBuffer));
}

storm_render::Result DX9RENDER::VBLock(storm_render::VertexBuffer *pVB, UINT OffsetToLock, UINT SizeToLock, uint8_t **ppbData,
                          uint32_t Flags)
{
    dwNumLV++;
    return CHECKD3DERR(pVB->Lock(OffsetToLock, SizeToLock, (void **)ppbData, Flags));
}

void DX9RENDER::VBUnlock(storm_render::VertexBuffer *pVB)
{
    CHECKD3DERR(pVB->Unlock());
}

storm_render::Result DX9RENDER::SetFVF(uint32_t handle)
{
    return CHECKD3DERR(renderDevice->BindVertexProgram(NULL)) || CHECKD3DERR(renderDevice->SetFVF(handle));
}

storm_render::Result DX9RENDER::SetStreamSource(UINT StreamNumber, void *pStreamData, UINT Stride)
{
    return CHECKD3DERR(
        renderDevice->SetStreamSource(StreamNumber, static_cast<storm_render::VertexBuffer *>(pStreamData), 0, Stride));
}

storm_render::Result DX9RENDER::SetIndices(void *pIndexData)
{
    return CHECKD3DERR(renderDevice->SetIndices(static_cast<storm_render::IndexBuffer *>(pIndexData)));
}

storm_render::Result DX9RENDER::DrawPrimitive(storm_render::Primitive dwPrimitiveType, UINT StartVertex, UINT PrimitiveCount)
{
    dwNumDrawPrimitive++;
    return CHECKD3DERR(renderDevice->DrawPrimitive(dwPrimitiveType, StartVertex, PrimitiveCount));
}

storm_render::Result DX9RENDER::Release(storm_render::RefCounted *pObject)
{
    if (pObject)
    {
        const auto result = pObject->Release();
        pObject = nullptr;
        return result;
    }

    return storm_render::result::ok;
}

storm_render::Result DX9RENDER::GetRenderTarget(storm_render::Surface **ppRenderTarget)
{
    return CHECKD3DERR(renderDevice->GetRenderTarget(ppRenderTarget));
}

storm_render::Result DX9RENDER::GetDepthStencilSurface(storm_render::Surface **ppZStencilSurface)
{
    return CHECKD3DERR(renderDevice->GetDepthStencilSurface(ppZStencilSurface));
}

storm_render::Result DX9RENDER::GetCubeMapSurface(storm_render::CubeTexture *ppCubeTexture, storm_render::CubeFace FaceType, UINT Level,
                                     storm_render::Surface **ppCubeMapSurface)
{
    return ppCubeTexture->GetCubeMapSurface(FaceType, Level, ppCubeMapSurface);
}

storm_render::Result DX9RENDER::SetRenderTarget(storm_render::Surface *pRenderTarget, storm_render::Surface *pNewZStencil)
{
    auto result = !CHECKD3DERR(renderDevice->SetDepthStencilSurface(pNewZStencil)) &&
                  !CHECKD3DERR(renderDevice->SetRenderTarget(pRenderTarget));
    return result ? storm_render::result::ok : S_FALSE;
}

storm_render::Result DX9RENDER::Clear(uint32_t Flags, storm_render::PackedColor Color, float Z)
{
    return CHECKD3DERR(renderDevice->Clear(Flags, Color, Z));
}

static bool isInScene = false;

storm_render::Result DX9RENDER::BeginScene()
{
    if (!isInScene)
    {
        isInScene = true;
        return CHECKD3DERR(renderDevice->BeginScene());
    }

    return storm_render::result::ok;
}

storm_render::Result DX9RENDER::EndScene()
{
    if (isInScene)
    {
        isInScene = false;
        return CHECKD3DERR(renderDevice->EndScene());
    }

    return storm_render::result::ok;
}

storm_render::Result DX9RENDER::SetClipPlane(uint32_t Index, const float *pPlane)
{
    // return renderDevice->SetClipPlane( Index, pPlane );
    return storm_render::result::ok;
}

storm_render::Result DX9RENDER::CreateTexture(UINT Width, UINT Height, UINT Levels, uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool,
                                 storm_render::Texture **ppTexture)
{
    return CHECKD3DERR(renderDevice->CreateTexture(Width, Height, Levels, Usage, Format, ppTexture));
}

storm_render::Result DX9RENDER::CreateCubeTexture(UINT EdgeLength, UINT Levels, uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool,
                                     storm_render::CubeTexture **ppCubeTexture)
{
    return CHECKD3DERR(renderDevice->CreateCubeTexture(EdgeLength, Levels, Usage, Format, ppCubeTexture));
}

storm_render::Result DX9RENDER::CreateOffscreenPlainSurface(UINT Width, UINT Height, storm_render::AssetFormat Format, storm_render::Surface **ppSurface)
{
    return CHECKD3DERR(renderDevice->CreateOffscreenPlainSurface(Width, Height, Format, ppSurface));
    //~!~D3DERR_OUTOFVIDEOMEMORY
    // GetAvailableTextureMem
}

storm_render::Result DX9RENDER::CreateDepthStencilSurface(UINT Width, UINT Height, storm_render::AssetFormat Format, storm_render::SampleCount MultiSample,
                                             storm_render::Surface **ppSurface)
{
    return CHECKD3DERR(renderDevice->CreateDepthStencilSurface(Width, Height, Format, ppSurface));
}

storm_render::Result DX9RENDER::CreateVertexDeclaration(const storm_render::VertexElement *pVertexElements,
                                           storm_render::VertexLayout **ppDecl)
{
    return CHECKD3DERR(renderDevice->CreateVertexDeclaration(pVertexElements, ppDecl));
}

storm_render::Result DX9RENDER::SetVertexDeclaration(storm_render::VertexLayout *pDecl)
{
    return CHECKD3DERR(renderDevice->SetVertexDeclaration(pDecl));
}

storm_render::Result DX9RENDER::ImportVertexProgram(const uint32_t *pFunction, storm_render::Program **ppShader)
{
    return CHECKD3DERR(renderDevice->ImportVertexProgram((const DWORD *)pFunction, ppShader));
}

storm_render::Result DX9RENDER::ImportFragmentProgram(const uint32_t *pFunction, storm_render::Program **ppShader)
{
    return CHECKD3DERR(renderDevice->ImportFragmentProgram((const DWORD *)pFunction, ppShader));
}

storm_render::Result DX9RENDER::DeleteVertexShader(storm_render::Program *pShader)
{
    if (pShader)
    {
        return pShader->Release();
    }
    return storm_render::result::ok;
}

storm_render::Result DX9RENDER::DeletePixelShader(storm_render::Program *pShader)
{
    if (pShader)
    {
        return pShader->Release();
    }
    return storm_render::result::ok;
}

storm_render::Result DX9RENDER::GetVertexProgram(storm_render::Program **ppShader)
{
    return CHECKD3DERR(renderDevice->GetVertexProgram(ppShader));
}

storm_render::Result DX9RENDER::GetFragmentProgram(storm_render::Program **ppShader)
{
    return CHECKD3DERR(renderDevice->GetFragmentProgram(ppShader));
}

#ifdef _WIN32 // Effects
ID3DXEffect *DX9RENDER::GetEffectPointer(const char *techniqueName)
{
    return effects_.getEffectPointer(techniqueName);
}
#endif

storm_render::Result DX9RENDER::SetTexture(uint32_t Stage, storm_render::BaseTexture *pTexture)
{
    return CHECKD3DERR(renderDevice->SetTexture(Stage, pTexture));
}

storm_render::Result DX9RENDER::GetLevelDesc(storm_render::Texture *ppTexture, UINT Level, storm_render::SurfaceDesc *pDesc)
{
    return CHECKD3DERR(ppTexture->GetLevelDesc(Level, pDesc));
}

storm_render::Result DX9RENDER::GetLevelDesc(storm_render::CubeTexture *ppCubeTexture, UINT Level, storm_render::SurfaceDesc *pDesc)
{
    return CHECKD3DERR(ppCubeTexture->GetLevelDesc(Level, pDesc));
}

storm_render::Result DX9RENDER::LockRect(storm_render::CubeTexture *ppCubeTexture, storm_render::CubeFace FaceType, UINT Level,
                            storm_render::LockedRect *pLockedRect, const storm_render::Rect *pRect, uint32_t Flags)
{
    return CHECKD3DERR(ppCubeTexture->LockRect(FaceType, Level, pLockedRect, pRect, Flags));
}

storm_render::Result DX9RENDER::LockRect(storm_render::Texture *ppTexture, UINT Level, storm_render::LockedRect *pLockedRect, const storm_render::Rect *pRect,
                            uint32_t Flags)
{
    return CHECKD3DERR(ppTexture->LockRect(Level, pLockedRect, pRect, Flags));
}

storm_render::Result DX9RENDER::UnlockRect(storm_render::CubeTexture *pCubeTexture, storm_render::CubeFace FaceType, UINT Level)
{
    return CHECKD3DERR(pCubeTexture->UnlockRect(FaceType, Level));
}

storm_render::Result DX9RENDER::UnlockRect(storm_render::Texture *pTexture, UINT Level)
{
    return CHECKD3DERR(pTexture->UnlockRect(Level));
}

storm_render::Result DX9RENDER::GetSurfaceLevel(storm_render::Texture *ppTexture, UINT Level, storm_render::Surface **ppSurfaceLevel)
{
    return CHECKD3DERR(ppTexture->GetSurfaceLevel(Level, ppSurfaceLevel));
}

storm_render::Result DX9RENDER::UpdateSurface(storm_render::Surface *pSourceSurface, const storm_render::Rect *pSourceRectsArray, UINT cRects,
                                 storm_render::Surface *pDestinationSurface, const storm_render::Point *pDestPointsArray)
{
    return CHECKD3DERR(storm_render::ImageLoadSurfaceFromSurface(pDestinationSurface, nullptr, nullptr, pSourceSurface, nullptr,
                                                  nullptr, storm_render::ImageDefault, 0));
    //return CHECKD3DERR(renderDevice->UpdateSurface(pSourceSurface, pSourceRectsArray, pDestinationSurface, pDestPointsArray));
}

storm_render::Result DX9RENDER::StretchRect(storm_render::Surface *pSourceSurface, const storm_render::Rect *pSourceRect,
                               storm_render::Surface *pDestSurface, const storm_render::Rect *pDestRect, storm_render::TextureFilter Filter)
{
    return CHECKD3DERR(renderDevice->StretchRect(pSourceSurface, pSourceRect, pDestSurface, pDestRect, Filter));
}

storm_render::Result DX9RENDER::GetRenderTargetData(storm_render::Surface *pRenderTarget, storm_render::Surface *pDestSurface)
{
    storm_render::SurfaceDesc desc;
    if (CHECKD3DERR(pRenderTarget->GetDesc(&desc)))
        return storm_render::result::notAvailable;

    if (desc.MultiSampleType != storm_render::SampleCount::None)
    {
        storm_render::Surface *pNonsampledSurface = nullptr;

        if(CHECKD3DERR(renderDevice->CreateRenderTarget(desc.Width, desc.Height, desc.Format, &pNonsampledSurface)))
        {
            return storm_render::result::notAvailable;
        }

        if (CHECKD3DERR(pDestSurface->GetDesc(&desc)))
        {
            return storm_render::result::notAvailable;
        }

        if (CHECKD3DERR(renderDevice->StretchRect(pRenderTarget, nullptr, pNonsampledSurface, nullptr, storm_render::TextureFilterNone)))
        {
            return storm_render::result::notAvailable;
        }

        bool error;
        if (desc.Pool == storm_render::MemoryClass::SystemMemory || desc.Pool == storm_render::MemoryClass::Scratch)
        {
            error = CHECKD3DERR(renderDevice->GetRenderTargetData(pNonsampledSurface, pDestSurface));
        }
        else
        {
            error = UpdateSurface(pNonsampledSurface, nullptr, 0, pDestSurface, nullptr);
        }

        pNonsampledSurface->Release();
        return error ? storm_render::result::notAvailable : storm_render::result::ok;
    }

    return CHECKD3DERR(renderDevice->GetRenderTargetData(pRenderTarget, pDestSurface));
}

storm_render::Result DX9RENDER::BindVertexProgram(storm_render::Program *pShader)
{
    return CHECKD3DERR(renderDevice->BindVertexProgram(pShader));
}

storm_render::Result DX9RENDER::BindFragmentProgram(storm_render::Program *pShader)
{
    return CHECKD3DERR(renderDevice->BindFragmentProgram(pShader));
}

storm_render::Result DX9RENDER::SetVertexShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4iCount)
{
    return CHECKD3DERR(renderDevice->SetVertexShaderConstantF(StartRegister, pConstantData, Vector4iCount));
}

storm_render::Result DX9RENDER::SetPixelShaderConstantF(UINT StartRegister, const float *pConstantData, UINT Vector4iCount)
{
    return CHECKD3DERR(renderDevice->SetPixelShaderConstantF(StartRegister, pConstantData, Vector4iCount));
}

storm_render::Result DX9RENDER::GetCapabilities(storm_render::Capabilities *pCaps)
{
    return CHECKD3DERR(renderDevice->GetCapabilities(pCaps));
}

CVideoTexture *DX9RENDER::GetVideoTexture(const char *sVideoName)
{
    if (sVideoName == nullptr)
        return nullptr;
    CVideoTexture *retVal = nullptr;
    VideoTextureEntity *pVTLcur = pVTL;

    // check already loaded
    const uint32_t newHash = MakeHashValue(sVideoName);
    while (pVTLcur != nullptr)
    {
        if (pVTLcur->hash == newHash && storm::iEquals(pVTLcur->name, sVideoName))
        {
            if (core.GetEntityPointer(pVTLcur->videoTexture_id))
            {
                pVTLcur->ref++;
                return pVTLcur->VideoTexture;
            }
            VideoTextureEntity *pVTmp = pVTLcur;
            pVTLcur = pVTLcur->next;
            delete pVTmp->name;
            delete pVTmp;
            continue;
        }
        pVTLcur = pVTLcur->next;
    }

    // create new video texture
    pVTLcur = new VideoTextureEntity;
    if (pVTLcur == nullptr)
        throw std::runtime_error("memory allocate error");
    pVTLcur->next = pVTL;
    pVTLcur->VideoTexture = nullptr;
    pVTLcur->hash = newHash;
    pVTLcur->ref = 1;
    const auto len = strlen(sVideoName) + 1;
    if ((pVTLcur->name = new char[len]) == nullptr)
        throw std::runtime_error("memory allocate error");
    strcpy_s(pVTLcur->name, len, sVideoName);
    const entid_t ei = core.CreateEntity("TextureSequence");
    pVTLcur->VideoTexture = static_cast<CVideoTexture *>(core.GetEntityPointer(ei));
    if (pVTLcur->VideoTexture != nullptr)
    {
        pVTLcur->videoTexture_id = ei;
        if (pVTLcur->VideoTexture->Initialize(this, sVideoName, true) == nullptr)
        {
            delete pVTLcur;
            core.EraseEntity(ei);
        }
        else
        {
            pVTL = pVTLcur;
            retVal = pVTLcur->VideoTexture;
        }
    }
    else
    {
        delete pVTLcur;
        core.EraseEntity(ei);
    }

    return retVal;
}

void DX9RENDER::ReleaseVideoTexture(CVideoTexture *pVTexture)
{
    VideoTextureEntity *cur = pVTL;
    VideoTextureEntity *prev = nullptr;

    if (cur != nullptr)
        do
        {
            if (cur->VideoTexture == pVTexture)
            {
                cur->ref--;
                if (cur->ref > 0)
                    return;
                if (prev == nullptr)
                    pVTL = cur->next;
                else
                    prev->next = cur->next;
                core.EraseEntity(cur->videoTexture_id);
                delete cur->name;
                delete cur;
                break;
            }
            prev = cur;
        } while ((cur = cur->next) != nullptr);
}

void DX9RENDER::PlayToTexture()
{
    VideoTextureEntity *cur = pVTL;
    while (cur != nullptr)
    {
        if (core.GetEntityPointer(pVTL->videoTexture_id))
        {
            cur->VideoTexture->FrameUpdate();
            cur = cur->next;
        }
        else
        {
            core.Trace("ERROR: void DX9RENDER::PlayToTexture()");
            delete cur->name;
            VideoTextureEntity *pcur = cur;
            cur = cur->next;
            if (pVTL == pcur)
                pVTL = cur;
            delete pcur;
        }
    }
}

storm_render::Result DX9RENDER::ImageBlt(int32_t TextureID, storm_render::Rect *pDstRect, storm_render::Rect *pSrcRect)
{
    struct F3DVERTEX
    {
        float x, y, z, rhw;
        float tu, tv;
    };
    storm_render::Rect dr;
    F3DVERTEX v[6];
    storm_render::Result hRes;

    if (pDstRect)
    {
        dr = *pDstRect;
    }
    else
    {
        dr.left = 0;
        dr.top = 0;
        dr.right = screen_size.x - 1;
        dr.bottom = screen_size.y - 1;
    }

    for (uint32_t n = 0; n < 6; n++)
    {
        v[n].rhw = 1.0f;
        v[n].z = 0.5f;
    }

    v[0].x = static_cast<float>(dr.left);
    v[0].y = static_cast<float>(dr.top);
    v[0].tu = 0.0f;
    v[0].tv = 0.0f;

    v[1].x = static_cast<float>(dr.left);
    v[1].y = static_cast<float>(dr.bottom);
    v[1].tu = 0.0f;
    v[1].tv = 1.0f;

    v[2].x = static_cast<float>(dr.right);
    v[2].y = static_cast<float>(dr.top);
    v[2].tu = 1.0f;
    v[2].tv = 0.0f;

    v[3].x = static_cast<float>(dr.right);
    v[3].y = static_cast<float>(dr.top);
    v[3].tu = 1.0f;
    v[3].tv = 0.0f;

    v[4].x = static_cast<float>(dr.left);
    v[4].y = static_cast<float>(dr.bottom);
    v[4].tu = 0.0f;
    v[4].tv = 1.0f;

    v[5].x = static_cast<float>(dr.right);
    v[5].y = static_cast<float>(dr.bottom);
    v[5].tu = 1.0f;
    v[5].tv = 1.0f;

    TextureSet(0, TextureID);

    const bool bDraw = TechniqueExecuteStart("texturedialogfon");
    if (bDraw)
        do
        {
            SetFVF(storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::tex1 | storm_render::vertexFormat::texCoord2);
            hRes = renderDevice->DrawPrimitiveUP(storm_render::PrimitiveTriangleList, 2, &v, sizeof(F3DVERTEX));
            dwNumDrawPrimitive++;
        } while (TechniqueExecuteNext());

    return hRes;
}

storm_render::Result DX9RENDER::ImageBlt(const char *pName, storm_render::Rect *pDstRect, storm_render::Rect *pSrcRect)
{
    int32_t TextureID;
    TextureID = TextureCreate(pName);
    const storm_render::Result hRes = ImageBlt(TextureID, pDstRect, pSrcRect);
    TextureRelease(TextureID);
    SetProgressImage(pName);
    return hRes;
}

void DX9RENDER::SetProgressImage(const char *image)
{
    if (!image || !image[0])
    {
        if (progressImageSize > 0 && progressImage)
            progressImage[0] = 0;
        return;
    }
    const int32_t s = strlen(image) + 1;
    if (s > progressImageSize)
    {
        progressImageSize = s;
        delete progressImage;
        progressImage = new char[progressImageSize];
    }
    strcpy_s(progressImage, s, image);
}

void DX9RENDER::SetProgressBackImage(const char *image)
{
    if (!image || !image[0])
    {
        if (progressBackImageSize > 0 && progressBackImage)
            progressBackImage[0] = 0;
        return;
    }
    const int32_t s = strlen(image) + 1;
    if (s > progressBackImageSize)
    {
        progressBackImageSize = s;
        delete progressBackImage;
        progressBackImage = new char[progressBackImageSize];
    }
    strcpy_s(progressBackImage, s, image);
}

void DX9RENDER::SetTipsImage(const char *image)
{
    if (!image || !image[0])
    {
        if (progressTipsImageSize > 0 && progressTipsImage)
            progressTipsImage[0] = 0;
        return;
    }
    const int32_t s = strlen(image) + 1;
    if (s > progressTipsImageSize)
    {
        progressTipsImageSize = s;
        delete progressTipsImage;
        progressTipsImage = new char[progressTipsImageSize];
    }
    memcpy(progressTipsImage, image, s);
}

char *DX9RENDER::GetTipsImage()
{
    return progressTipsImage;
}

void DX9RENDER::StartProgressView()
{
    if (SeaMapTransition::Loading())
    {
        progressSafeCounter = 0;
        progressUpdateTime = SDL_GetTicks() - 1000;
        return;
    }
    progressSafeCounter = 0;
    if (progressTexture < 0)
    {
        // Loading the texture
        loadFrame = 0;
        isInPViewProcess = true;
        const int32_t t = TextureCreate("Loading\\progress.tga");
        isInPViewProcess = false;
        if (t < 0)
        {
            core.Trace("Progress error!");
            return;
        }
        progressTexture = t;
    }
    else
        return;
    // Loading an unscaled background image
    if (progressBackImage && progressBackImage[0])
    {
        isInPViewProcess = true;
        if (back0Texture >= 0)
            TextureRelease(back0Texture);
        back0Texture = -1;
        back0Texture = TextureCreate(progressBackImage);
        isInPViewProcess = false;
    }
    else
    {
        back0Texture = -1;
    }
    // Loading a scalable background image
    if (progressImage && progressImage[0])
    {
        isInPViewProcess = true;
        if (backTexture >= 0)
            TextureRelease(backTexture);
        backTexture = -1;
        backTexture = TextureCreate(progressImage);
        isInPViewProcess = false;
    }
    else
    {
        backTexture = -1;
    }
    // Loading tips
    if (progressTipsImage && progressTipsImage[0])
    {
        isInPViewProcess = true;
        if (progressTipsTexture >= 0)
            TextureRelease(progressTipsTexture);
        progressTipsTexture = -1;
        progressTipsTexture = TextureCreate(progressTipsImage);
        isInPViewProcess = false;
    }
    progressUpdateTime = SDL_GetTicks() - 1000;
}

void DX9RENDER::ProgressView()
{
    if (SeaMapTransition::Loading())
    {
        const uint32_t time = SDL_GetTicks();
        if (isInPViewProcess || time - progressUpdateTime < 16) return;
        progressUpdateTime = time;
        isInPViewProcess = true;
        progressSafeCounter = 0;
        BeginScene();
        SeaMapTransition::DrawProgress();
        EndScene();
        renderDevice->Present();
        BeginScene();
        isInPViewProcess = false;
        return;
    }
    // get the texture
    if (progressTexture < 0)
        return;
    if (isInPViewProcess)
        return;
    // Analyzing time
    const uint32_t time = SDL_GetTicks();
    if (abs(static_cast<int32_t>(progressUpdateTime - time)) < 50)
        return;
    progressUpdateTime = time;
    isInPViewProcess = true;
    progressSafeCounter = 0;
	const float msaaOffset = -0.5f;
    // Drawing mode
    BeginScene();
    // Filling the vertices of the texture
    struct LoadVertex
    {
        float x, y, z, rhw;
        uint32_t color;
        float u, v;
    } v[4];
    uint32_t i;
    for (i = 0; i < 4; i++)
    {
        v[i].z = 0.5;
        v[i].rhw = 2.0;
        v[i].color = 0xffffffff;
    }
    // Calculate the rectangle in which to draw
    storm_render::Viewport vp;
    GetViewport(&vp);

    v[0].x = msaaOffset;
    v[0].y = msaaOffset;
    v[1].x = float(vp.Width);
    v[1].y = msaaOffset;
    v[2].x = msaaOffset;
    v[2].y = float(vp.Height);
    v[3].x = float(vp.Width);
    v[3].y = float(vp.Height);
    v[0].u = 0.0f;
    v[0].v = 0.0f;
    v[1].u = 1.0f;
    v[1].v = 0.0f;
    v[2].u = 0.0f;
    v[2].v = 1.0f;
    v[3].u = 1.0f;
    v[3].v = 1.0f;
    TextureSet(0, back0Texture);
    if (back0Texture < 0)
        for (i = 0; i < 4; i++)
            v[i].color = 0;
    if (back0Texture >= 0)
    {
        DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1, 2, v, sizeof(v[0]),
                        "ProgressBackTech");
    }

    float dy = 0.0f;
    float dx = ((float(vp.Width) - (4.0f * float(vp.Height) / 3.0f)) / 2.0f);
    if (dx < 10.0f)
        dx = 0.0f;
    else
    {
        dy = 25.0f;
        dx = ((float(vp.Width) - (4.0f * (float(vp.Height) - 2.0f * dy) / 3.0f)) / 2.0f);
    }

    v[0].x = 0.0f + dx;
    v[0].y = 0.0f + dy;
    v[1].x = float(vp.Width) - dx;
    v[1].y = 0.0f + dy;
    v[2].x = 0.0f + dx;
    v[2].y = float(vp.Height) - dy;
    v[3].x = float(vp.Width) - dx;
    v[3].y = float(vp.Height) - dy;
    v[0].u = 0.0f;
    v[0].v = 0.0f;
    v[1].u = 1.0f;
    v[1].v = 0.0f;
    v[2].u = 0.0f;
    v[2].v = 1.0f;
    v[3].u = 1.0f;
    v[3].v = 1.0f;

    TextureSet(0, backTexture);
    if (backTexture < 0)
    {
        for (i = 0; i < 4; i++)
            v[i].color = 0;
    }
    else
    {
        if (progressTipsTexture >= 0)
        {
            TextureSet(1, progressTipsTexture);
            DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1, 2, v, sizeof(v[0]),
                            "ProgressBackTechWithTips");
        }
        else
        {
            DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1, 2, v, sizeof(v[0]),
                            "ProgressBackTech");
        }
    }
    if (backTexture < 0)
        for (i = 0; i < 4; i++)
            v[i].color = 0xffffffff;
    // Animated object
    m_fHeightDeformator = ((float)vp.Height * 4.0f) / ((float)vp.Width * 3.0f);
    // core.Trace(" size_x %f", (vp.Width - dx * 2.0f)*progressFramesWidth);
    CVECTOR pos((vp.Width - dx * 2.0f) * progressFramesPosX + dx, (vp.Height - dy * 2.0f) * progressFramesPosY + dy,
                0.0f);
    CVECTOR size((vp.Width - dx * 2.0f) * progressFramesWidth,
                 (vp.Height - dy * 2.0f) * progressFramesHeight * 4.0f / 3.0f, 0.0f);
    v[0].x = pos.x;
    v[0].y = pos.y;
    v[1].x = pos.x + size.x + 0.5f;
    v[1].y = pos.y;
    v[2].x = pos.x;
    v[2].y = pos.y + size.y + 0.5f;
    v[3].x = pos.x + size.x + 0.5f;
    v[3].y = pos.y + size.y + 0.5f;
    v[3].y = pos.y + size.y + 0.5f;
    // Frame grid size
    int32_t sizeX = progressFramesCountX;
    int32_t sizeY = progressFramesCountY;
    // Position of the current frame
    int32_t fx = loadFrame % sizeX;
    int32_t fy = loadFrame / sizeY;
    v[0].u = fx / float(sizeX);
    v[0].v = fy / float(sizeY);
    v[1].u = (fx + 1.0f) / float(sizeX);
    v[1].v = fy / float(sizeY);
    v[2].u = fx / float(sizeX);
    v[2].v = (fy + 1.0f) / float(sizeY);
    v[3].u = (fx + 1.0f) / float(sizeX);
    v[3].v = (fy + 1.0f) / float(sizeY);
    // Draw
    TextureSet(0, progressTexture);
    DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1, 2, v, sizeof(v[0]),
                    "ProgressTech");
    EndScene();
    renderDevice->Present();
    BeginScene();
    // Next frame
    loadFrame++;
    if (loadFrame >= sizeX * sizeY)
        loadFrame = 0;
    // leave
    isInPViewProcess = false;
}

void DX9RENDER::EndProgressView()
{
    if (progressTexture >= 0)
        TextureRelease(progressTexture);
    progressTexture = -1;
    if (backTexture >= 0)
        TextureRelease(backTexture);
    backTexture = -1;
    if (back0Texture >= 0)
        TextureRelease(back0Texture);
    back0Texture = -1;
    if (progressTipsTexture >= 0)
        TextureRelease(progressTipsTexture);
    progressTipsTexture = -1;
    if (progressImage && progressImageSize > 0)
        progressImage[0] = 0;
    if (progressBackImage && progressBackImageSize > 0)
        progressBackImage[0] = 0;
    if (progressTipsImage && progressTipsImageSize > 0)
        progressTipsImage[0] = 0;
}

void DX9RENDER::SetColorParameters(float fGamma, float fBrightness, float fContrast)
{
    uint16_t rgb[256];
    for (uint32_t i = 0; i < 256; i++)
    {
        float fRamp = std::clamp(fContrast * 255.0f * 256.0f * powf(static_cast<float>(i / 255.0f), 1.0f / fGamma) +
                                     fBrightness * 256.0f,
                                 0.0f, 65535.0f);
        rgb[i] = static_cast<uint16_t>(fRamp);
    }
    core.GetWindow()->SetGamma(rgb, rgb, rgb);
}

void DX9RENDER::MakeDrawVector(RS_LINE *pLines, uint32_t dwNumSubLines, const CMatrix &mMatrix, CVECTOR vUp, CVECTOR v1,
                               CVECTOR v2, float fScale, uint32_t dwColor)
{
    uint32_t i;
    uint32_t k;

    // for (i=0; i<dwNumSubLines * 2 + 2; i++) pLines[i].dwColor = dwColor;
    k = dwNumSubLines * 2 + 2; // boal optimization if the for loop runs the calculations every iteration
    for (i = 0; i < k; i++)
    {
        pLines[i].dwColor = dwColor;
    }
    pLines[0].vPos = v1;
    pLines[1].vPos = v1 + (fScale * v2);

    const float fRadius = 0.03f * fScale;
    const float fDist = 0.85f * fScale * sqrtf(~v2);

    for (i = 0; i < dwNumSubLines; i++)
    {
        const float fAng = PIm2 * static_cast<float>(i) / static_cast<float>(dwNumSubLines);

        const float x = fRadius * sinf(fAng);
        const float z = fRadius * cosf(fAng);

        CVECTOR vRes;

        if (fabsf(vUp.x) < 1e-5f)
            vRes = CVECTOR(fDist, x, z);
        if (fabsf(vUp.y) < 1e-5f)
            vRes = CVECTOR(x, fDist, z);
        if (fabsf(vUp.z) < 1e-5f)
            vRes = CVECTOR(x, z, fDist);
        vRes = (CMatrix &)mMatrix * vRes;
        pLines[2 + i * 2 + 0].vPos = vRes;
        pLines[2 + i * 2 + 1].vPos = pLines[1].vPos;
    }
}

void DX9RENDER::DrawVector(const CVECTOR &v1, const CVECTOR &v2, uint32_t dwColor, const char *pTechniqueName)
{
    RS_LINE lines[51 * 2];
    CMatrix mView;

    const float fScale = sqrtf(~(v2 - v1));
    if (!mView.BuildViewMatrix(v1, v2, CVECTOR(0.0f, 1.0f, 0.0f)))
        if (!mView.BuildViewMatrix(v1, v2, CVECTOR(1.0f, 0.0f, 0.0f)))
            return;

    mView.Transposition();

    MakeDrawVector(&lines[0], 50, mView, CVECTOR(1.0f, 1.0f, 0.0f), mView.Pos(), mView.Vz(), fScale, dwColor);

    const CMatrix mWorldSave;
    GetTransform(storm_render::TransformWorld, mWorldSave);
    SetTransform(storm_render::TransformWorld, CMatrix());
    DrawLines(lines, 51, pTechniqueName);
    SetTransform(storm_render::TransformWorld, mWorldSave);
}

void DX9RENDER::DrawSphere(const CVECTOR &vPos, float fRadius, uint32_t dwColor)
{
    CMatrix m;
    m.BuildPosition(vPos.x, vPos.y, vPos.z);
    m.m[0][0] = fRadius;
    m.m[1][1] = fRadius;
    m.m[2][2] = fRadius;

    SetRenderState(storm_render::RenderTextureFactor, dwColor);
    SetTransform(storm_render::TransformWorld, m);
    DrawPrimitiveUP(storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse, DX9sphereNumTrgs, DX9sphereVertex,
                    sizeof(DX9SphVertex), "DXSphere");
}


void DX9RENDER::DrawEllipsoid(const CVECTOR &vPos, float a, float b, float c, float ay, uint32_t dwColor)
{
    CMatrix trans, scale, rot;
    trans.BuildPosition(vPos.x, vPos.y, vPos.z);
    scale.BuildScale(a, b, c);
    rot.BuildRotateY(ay);

    SetTransform(storm_render::TransformWorld, scale * rot * trans);
    SetRenderState(storm_render::RenderTextureFactor, dwColor);
    DrawPrimitiveUP(storm_render::PrimitiveTriangleList, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse, DX9sphereNumTrgs, DX9sphereVertex,
                    sizeof(DX9SphVertex), "DXEllipsoid");
}

void DX9RENDER::SetLoadTextureEnable(bool bEnable)
{
    bLoadTextureEnabled = bEnable;
}

storm_render::VolumeTexture *DX9RENDER::CreateVolumeTexture(uint32_t Width, uint32_t Height, uint32_t Depth,
                                                        uint32_t Levels, uint32_t Usage, storm_render::AssetFormat Format, storm_render::MemoryClass Pool)
{
    storm_render::VolumeTexture *pVolumeTexture = nullptr;
    CHECKD3DERR(renderDevice->CreateVolumeTexture(Width, Height, Depth, Levels, Usage, Format, &pVolumeTexture));
    return pVolumeTexture;
}

bool DX9RENDER::PushRenderTarget()
{
    RenderTarget renderTarget{};
    GetRenderTarget(&renderTarget.pRenderTarget);
    CHECKD3DERR(renderDevice->GetDepthStencilSurface(&renderTarget.pDepthSurface));
    GetViewport(&renderTarget.ViewPort);
    stRenderTarget.push(renderTarget);

    return true;
}

bool DX9RENDER::PopRenderTarget()
{
    if (stRenderTarget.empty())
    {
        core.Trace("DX9Error: Try to pop RenderTarget, but RenderTarget stack is empty");
        return false;
    }

    auto top = stRenderTarget.top();
    SetRenderTarget(top.pRenderTarget, top.pDepthSurface);
    SetViewport(&top.ViewPort);
    RELEASE(top.pRenderTarget);
    RELEASE(top.pDepthSurface);
    stRenderTarget.pop();

    return true;
}

bool DX9RENDER::SetRenderTarget(storm_render::CubeTexture *pRenderTarget, uint32_t FaceType, uint32_t dwLevel,
                                storm_render::Surface *pZStencil)
{
    storm_render::Surface *pSurface;
    return !CHECKD3DERR(pRenderTarget->GetCubeMapSurface(static_cast<storm_render::CubeFace>(FaceType), dwLevel, &pSurface)) &&
           !CHECKD3DERR(SetRenderTarget(pSurface, pZStencil)) && Release(pSurface) == storm_render::result::ok;
}

void DX9RENDER::SetView(const CMatrix &mView)
{
    SetTransform(storm_render::TransformView, mView);
}

void DX9RENDER::SetWorld(const CMatrix &mWorld)
{
    SetTransform(storm_render::TransformWorld, mWorld);
}

void DX9RENDER::SetProjection(const CMatrix &mProjection)
{
    SetTransform(storm_render::TransformProjection, mProjection);
}

const CMatrix &DX9RENDER::GetView()
{
    GetTransform(storm_render::TransformView, mView);
    return mView;
}

const CMatrix &DX9RENDER::GetWorld()
{
    GetTransform(storm_render::TransformWorld, mWorld);
    return mWorld;
}

const CMatrix &DX9RENDER::GetProjection()
{
    GetTransform(storm_render::TransformProjection, mProjection);
    return mProjection;
}

storm_render::BaseTexture *DX9RENDER::GetTextureFromID(int32_t nTextureID)
{
    if (nTextureID < 0)
        return nullptr;
    return Textures[nTextureID].d3dtex;
}

bool DX9RENDER::GetRenderTargetAsTexture(storm_render::Texture **tex)
{
    Release(*tex);
    storm_render::Surface *renderTarget;
    storm_render::SurfaceDesc desc;

    auto success = false;

    if (GetRenderTarget(&renderTarget) == storm_render::result::ok && renderTarget->GetDesc(&desc) == storm_render::result::ok)
    {
        if (CreateTexture(desc.Width, desc.Height, 0, 0, desc.Format, storm_render::MemoryClass::Default, tex) == storm_render::result::ok)
        {
            storm_render::Surface *pTexSurface;
            if ((*tex)->GetSurfaceLevel(0, &pTexSurface) == storm_render::result::ok)
            {
                if (GetRenderTargetData(renderTarget, pTexSurface) == storm_render::result::ok)
                {
                    success = true;
                }
                pTexSurface->Release();
            }
        }
    }

    return success;
}
