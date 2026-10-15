#ifndef _WIN32 // Effects
#include "technique.h"

#include "core.h"
#include "debug-trap.h"
#include "math_inlines.h"
#include "string_compare.hpp"
#include "vma.hpp"

#include <algorithm>
#include <ranges>

#define USE_FX // Will load techniques from fx files

#ifdef USE_FX
#define SHA_DIR "resource\\techniques"
#define SHA_EXT "*.fx"
#else
#define SHA_DIR "resource\\techniques-sha"
#define SHA_EXT "*.sha"
#endif

#define PASS_OK 0
#define PASS_ERROR 1

#define TECHNIQUE_OK 0
#define TECHNIQUE_ERROR 1

#define BLOCK_OK 0
#define BLOCK_ERROR 1

#define SHADER_OK 0
#define SHADER_ERROR 1

#define COMMENT1 "//"
#define COMMENT2 ";"
#define BLOCK "block"
#define TECHNIQUE "technique"
#define PIXEL_SHADER "pixelshader"
#define VERTEX_SHADER "vertexshader"
#define VERTEX_SHADER_CONST "vertexshaderconstant"
#define PIXEL_SHADER_CONST "pixelshaderconstant"
#define VERTEX_DECL "decl"
#define ASM_SHADER "asm"
#define PASS "pass"
#define INCLUDE "#include"
#define DEFINE "#define"
#define BEGIN_BRACKET "{"
#define END_BRACKET "}"
#define NOSAVE_STATE "!"

#define STSS_CHECK "["
#define TEXTURE_CHECK "texture["
#define RENDER_CHECK "render."
#define INPARAM_CHECK "<"
#define TRANSFORM_CHECK "transform."
#define WORLD_TRANSFORM_CHECK ".world"
#define RESTORE_STATES_CHECK "restore."

#define VDECL_STREAM_CHECK "stream"
#define VDECL_FLOAT_CHECK "float"
#define VDECL_COLOR_CHECK "color"
#define VDECL_UBYTE4_CHECK "ubyte4"

#define DEFSIZE(x) x, (sizeof(x) / sizeof(SRSPARAM)), 0, 0

// codes

#define CODE_SRS 0       // SetRenderState
#define CODE_STSS 1      // SetTextureStageState
#define CODE_TEXTURE 2   // SetTexture
#define CODE_TRANSFORM 3 // SetTransform
#define CODE_RENDER 4    // Render
#define CODE_RESTORE 5   // Restore
#define CODE_SVS 6       // Set vertex shader
#define CODE_SPS 7       // Set pixel shader
#define CODE_SVSCONST 8  // Set vertex shader constant
#define CODE_SPSCONST 9  // Set pixel shader constant
#define CODE_SAMP 10     // SetSamplerState

// SetFVFConstant subcodes
#define SUBCODE_SVSCONST_WORLDVIEWPROJ 0

// render subcodes
#define SUBCODE_RENDER_DRAW 0

// restore subcodes
#define SUBCODE_RESTORE_STATES 100
#define SUBCODE_RESTORE_CLEAR 101

// errors
#define INVALID_INDEX 0xFFFFFFFF
#define INVALID_SRS_INDEX 0xFFFFFFFF
#define INVALID_STSS_INDEX 0xFFFFFFFF
#define INVALID_SHADER_INDEX 0xFFFFFFFF
#define INVALID_SHADER_HANDLE 0xFFFFFFFF

//
#define FLAGS_CODE_RESTORE (0x1 << 0x10)
#define FLAGS_CODE_IN_PARAM (0x1 << 0x11) // use [in] parameter
#define FLAGS_CODE_NUMBER (0x1 << 0x12)   // use [in] parameter

// parameters flags

// common defines
#define SAVED_STATES_ADD 128

namespace
{

inline char *tolwr(char *str)
{
    char *result = str;
    while (*str != '\0')
    {
        *str = tolower(*str);
        str++;
    }
    return result;
}

}

char *SkipToken(char *str, const char *cmp)
{
    int flag;
    char *str1;
    const char *cmp1;

    if (str == nullptr || cmp == nullptr)
        return nullptr;
    cmp1 = cmp;

    if ((str == nullptr) || (cmp == nullptr))
        return nullptr;
    if (strlen(str) == 0)
        return nullptr;

    int lens = strlen(str);
    int lenc = strlen(cmp);
    //	if (strcmp(str,cmp) != 0) return NULL;
    for (int i = 0; i < lens; i++)
    {
        flag = 0;
        str1 = str;
        if (*str == *cmp)
        {
            flag = 1;
            for (int j = 0; j < lenc; j++)
                if (*str++ != *cmp++)
                {
                    flag = 0;
                    break;
                }
            if (!flag)
            {
                cmp = cmp1;
                str = str1;
            }
        }
        if (flag)
            break;
        str++;
    }
    if (flag)
    {
        while (*str == 32)
            str++;
        return str;
    }
    return nullptr;
}

char *FindToken(char *str, const char *cmp)
{
    int flag;
    char *str1;
    const char *cmp1;

    if (str == nullptr || cmp == nullptr)
        return nullptr;
    cmp1 = cmp;

    if ((str == nullptr) || (cmp == nullptr))
        return nullptr;
    if (strlen(str) == 0)
        return nullptr;

    int lens = strlen(str);
    int lenc = strlen(cmp);
    //	if (strcmp(str,cmp) != 0) return NULL;
    for (int i = 0; i < lens; i++)
    {
        flag = 0;
        str1 = str;
        if (*str == *cmp)
        {
            flag = 1;
            for (int j = 0; j < lenc; j++)
                if (*str++ != *cmp++)
                {
                    flag = 0;
                    break;
                }
            if (!flag)
            {
                cmp = cmp1;
                str = str1;
            }
        }
        if (flag)
            break;
        str++;
    }
    if (flag)
        return str;
    return nullptr;
}

// Get token before symbol skip
char *GetTokenWhile(char *src, char *dst, const char *skip)
{
    if (!src || !dst || !skip)
        return nullptr;
    dst[0] = 0;
    char *dst1 = dst;
    int32_t lensrc = strlen(src);
    int32_t dstlen = 0;
    while (lensrc && src[0] == ' ')
    {
        src++;
        lensrc--;
    };
    while ((lensrc--) && (*src != *skip))
    {
        *dst1++ = *src++;
        dstlen++;
    }
    *dst1++ = 0;
    char *pRet = ((lensrc > 0) ? ++src : src);
    if (lensrc)
        lensrc--;
    while (lensrc && pRet[0] == 32)
    {
        pRet++;
        lensrc--;
    };
    while (dstlen && dst[dstlen - 1] == ' ')
    {
        dstlen--;
        dst[dstlen] = 0;
    }
    return pRet;
}

typedef struct
{
    SRSPARAM *pParam;
    uint32_t dwParamNum;
    bool bUse;
    int32_t dwUseSubCode;
    uint32_t SrsType;
    const char *cName;
    storm_render::RenderStateField State;
} SRS;

typedef struct
{
    SRSPARAM *pParam;
    uint32_t dwParamNum;
    bool bUse;
    int32_t dwUseSubCode;
    uint32_t SrsType;
    const char *cName;
    storm_render::StageField State;
} STSS;

typedef struct
{
    SRSPARAM *pParam;
    uint32_t dwParamNum;
    bool bUse;
    int32_t dwUseSubCode;
    uint32_t SrsType;
    const char *cName;
    storm_render::SamplerField State;
} SAMP;

#define SRS_DWORD 0
#define SRS_FLOAT 1

// ---------------------------------------------------------------------------------

SRSPARAM MYDECLUSAGE[] = {{"position", storm_render::VertexSemantic::Position},
                          {"blendweight", storm_render::VertexSemantic::BlendWeight},
                          {"blendindices", storm_render::VertexSemantic::BlendIndices},
                          {"normal", storm_render::VertexSemantic::Normal},
                          {"psize", storm_render::VertexSemantic::PointSize},
                          {"texcoord", storm_render::VertexSemantic::TexCoord},
                          {"tangent", storm_render::VertexSemantic::Tangent},
                          {"binormal", storm_render::VertexSemantic::Binormal},
                          {"tessfactor", storm_render::VertexSemantic::TessFactor},
                          {"positiont", storm_render::VertexSemantic::PositionTransformed},
                          {"color", storm_render::VertexSemantic::Color},
                          {"fog", storm_render::VertexSemantic::Fog},
                          {"depth", storm_render::VertexSemantic::Depth},
                          {"sample", storm_render::VertexSemantic::Sample}};

SRSPARAM MYSETVERTEXSHADERCONSTANT[] = {{"worldviewproj", SUBCODE_SVSCONST_WORLDVIEWPROJ}};

// SetRenderState parameters
SRSPARAM MYD3DZB[] = {{"false", storm_render::DepthDisabled}, {"true", storm_render::DepthEnabled}, {"usew", storm_render::DepthWBuffer}};

SRSPARAM MYD3DFILL[] = {{"point", storm_render::FillPoint}, {"wireframe", storm_render::FillWireframe}, {"solid", storm_render::FillSolid}};

SRSPARAM MYD3DSHADE[] = {{"flat", storm_render::ShadeFlat},
                         {"gouraud", storm_render::ShadeGouraud}
#ifndef _XBOX
                         ,
                         {"phong", storm_render::ShadePhong}
#endif
};

SRSPARAM MYD3DBLEND[] = {{"zero", storm_render::BlendZero},
                         {"one", storm_render::BlendOne},
                         {"srccolor", storm_render::BlendSourceColor},
                         {"invsrccolor", storm_render::BlendInverseSourceColor},
                         {"srcalpha", storm_render::BlendSourceAlpha},
                         {"invsrcalpha", storm_render::BlendInverseSourceAlpha},
                         {"destalpha", storm_render::BlendDestinationAlpha},
                         {"invdestalpha", storm_render::BlendInverseDestinationAlpha},
                         {"destcolor", storm_render::BlendDestinationColor},
                         {"invdestcolor", storm_render::BlendInverseDestinationColor},
                         {"srcalphasat", storm_render::BlendSourceAlphaSaturate},
#ifndef _XBOX
                         {"bothsrcalpha", storm_render::BlendBothSourceAlpha},
                         {"bothinvsrcalpha", storm_render::BlendBothInverseSourceAlpha}
#endif
};

SRSPARAM MYD3DCULL[] = {{"none", storm_render::CullNone}, {"cw", storm_render::CullClockwise}, {"ccw", storm_render::CullCounterClockwise}};

SRSPARAM MYD3DBLENDOP[] = {{"add", storm_render::BlendOpAdd},
                           {"subtract", storm_render::BlendOpSubtract},
                           {"revsubtract", storm_render::BlendOpReverseSubtract},
                           {"min", storm_render::BlendOpMin},
                           {"max", storm_render::BlendOpMax}};

SRSPARAM MYD3DCMPFUNC[] = {{"never", storm_render::CompareNever},
                           {"less", storm_render::CompareLess},
                           {"equal", storm_render::CompareEqual},
                           {"lessequal", storm_render::CompareLessEqual},
                           {"greater", storm_render::CompareGreater},
                           {"notequal", storm_render::CompareNotEqual},
                           {"greaterequal", storm_render::CompareGreaterEqual},
                           {"always", storm_render::CompareAlways}};

SRSPARAM MYD3DFOGMODE[] = {
    {"none", storm_render::FogNone}, {"exp", storm_render::FogExponential}, {"exp2", storm_render::FogExponentialSquared}, {"linear", storm_render::FogLinear}};

SRSPARAM MYD3DSTENCILOP[] = {{"keep", storm_render::StencilOpKeep},       {"zero", storm_render::StencilOpZero},
                             {"replace", storm_render::StencilOpReplace}, {"incrsat", storm_render::StencilOpIncrementSaturate},
                             {"decrsat", storm_render::StencilOpDecrementSaturate}, {"invert", storm_render::StencilOpInvert},
                             {"incr", storm_render::StencilOpIncrement},       {"decr", storm_render::StencilOpDecrement}};

SRSPARAM MYD3DMATERIALCOLORSOURCE[] = {
    {"material", storm_render::MaterialSourceMaterial}, {"color1", storm_render::MaterialSourceDiffuseColor}, {"color2", storm_render::MaterialSourceSpecularColor}};

SRSPARAM MYD3DVERTEXBLENDFLAGS[] = {
    {"disable", storm_render::VertexBlendDisable},   {"1weights", storm_render::VertexBlendOneWeight}, {"2weights", storm_render::VertexBlendTwoWeights},
    {"3weights", storm_render::VertexBlendThreeWeights},
#ifndef _XBOX
    {"tweening", storm_render::VertexBlendTweening}, {"0WEIGHTS ", storm_render::VertexBlendZeroWeights}
#endif
};

SRSPARAM MYD3DPATCHEDGESTYLE[] = {{"discrete", storm_render::PatchEdgeDiscrete}, {"continuous", storm_render::PatchEdgeContinuous}};

SRSPARAM MYD3DCOLORWRITEMASK[] = {{"alpha", storm_render::colorWrite::alpha},
                                  {"red", storm_render::colorWrite::red},
                                  {"green", storm_render::colorWrite::green},
                                  {"blue", storm_render::colorWrite::blue}};

SRSPARAM MYD3DWRAP[] = {{"wrapcoord0", storm_render::wrap::coord0}, {"wrapcoord1", storm_render::wrap::coord1}, {"wrapcoord2", storm_render::wrap::coord2},
                        {"wrapcoord3", storm_render::wrap::coord3}, {"wrapu", storm_render::wrap::u},           {"wrapv", storm_render::wrap::v}};

SRSPARAM MYTRUEFALSE[] = {{"true", true}, {"false", false}};

SRS RenderStates[] = {
    {DEFSIZE(MYD3DZB), SRS_DWORD, "ZEnable", storm_render::RenderDepthTest},
    {DEFSIZE(MYD3DFILL), SRS_DWORD, "FillMode", storm_render::RenderFillMode},
    {DEFSIZE(MYD3DSHADE), SRS_DWORD, "ShadeMode", storm_render::RenderShadeMode},
#ifndef _XBOX
//{ DEFSIZE(0) , SRS_DWORD, "LinePattern",						D3DRS_LINEPATTERN },
#endif
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "ZWriteEnable", storm_render::RenderDepthWrite},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "AlphaTestEnable", storm_render::RenderAlphaTest},
#ifndef _XBOX
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "LastPixel", storm_render::RenderLastPixel},
#endif
    {DEFSIZE(MYD3DBLEND), SRS_DWORD, "SrcBlend", storm_render::RenderSourceBlend},
    {DEFSIZE(MYD3DBLEND), SRS_DWORD, "DestBlend", storm_render::RenderDestinationBlend},
    {DEFSIZE(MYD3DCULL), SRS_DWORD, "CullMode", storm_render::RenderCullMode},
    {DEFSIZE(MYD3DCMPFUNC), SRS_DWORD, "ZFunc", storm_render::RenderDepthCompare},
    {DEFSIZE(0), SRS_DWORD, "AlphaRef", storm_render::RenderAlphaReference},
    {DEFSIZE(MYD3DCMPFUNC), SRS_DWORD, "AlphaFunc", storm_render::RenderAlphaCompare},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "DitherEnable", storm_render::RenderDither},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "AlphaBlendEnable", storm_render::RenderBlend},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "FogEnable", storm_render::RenderFog},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "SpecularEnable", storm_render::RenderSpecular},
#ifndef _XBOX
//{ DEFSIZE(MYTRUEFALSE) , SRS_DWORD, "ZVisible",					D3DRS_ZVISIBLE },
#endif
    {DEFSIZE(0), SRS_DWORD, "FogColor", storm_render::RenderFogColor},
    {DEFSIZE(MYD3DFOGMODE), SRS_DWORD, "FogTableMode", storm_render::RenderFogTableMode},
    {DEFSIZE(0), SRS_FLOAT, "FogStart", storm_render::RenderFogStart},
    {DEFSIZE(0), SRS_FLOAT, "FogEnd", storm_render::RenderFogEnd},
    {DEFSIZE(0), SRS_FLOAT, "FogDensity", storm_render::RenderFogDensity},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "EdgeAntialias", storm_render::RenderAntialiasedLine},
#ifdef USE_FX
    {DEFSIZE(0), SRS_DWORD, "DepthBias", storm_render::RenderDepthBias},
    {DEFSIZE(0), SRS_DWORD, "SlopeScaleDepthBias", storm_render::RenderSlopeScaleDepthBias},
#else
    {DEFSIZE(0), SRS_DWORD, "Zbias", storm_render::RenderDepthBias},
    {DEFSIZE(0), SRS_DWORD, "SSZbias", storm_render::RenderSlopeScaleDepthBias},
#endif
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "RangeFogEnable", storm_render::RenderRangeFog},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "StencilEnable", storm_render::RenderStencil},
    {DEFSIZE(MYD3DSTENCILOP), SRS_DWORD, "StencilFail", storm_render::RenderStencilFail},
    {DEFSIZE(MYD3DSTENCILOP), SRS_DWORD, "StencilZFail", storm_render::RenderStencilDepthFail},
    {DEFSIZE(MYD3DSTENCILOP), SRS_DWORD, "StencilPass", storm_render::RenderStencilPass},
    {DEFSIZE(MYD3DCMPFUNC), SRS_DWORD, "StencilFunc", storm_render::RenderStencilCompare},
    {DEFSIZE(0), SRS_DWORD, "StencilRef", storm_render::RenderStencilReference},
    {DEFSIZE(0), SRS_DWORD, "StencilMask", storm_render::RenderStencilReadMask},
    {DEFSIZE(0), SRS_DWORD, "StencilWriteMask", storm_render::RenderStencilWriteMask},
    {DEFSIZE(0), SRS_DWORD, "TextureFactor", storm_render::RenderTextureFactor},
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap0", storm_render::RenderWrap0},
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap1", storm_render::RenderWrap1},
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap2", storm_render::RenderWrap2},
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap3", storm_render::RenderWrap3},
#ifndef _XBOX
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap4", storm_render::RenderWrap4},
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap5", storm_render::RenderWrap5},
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap6", storm_render::RenderWrap6},
    {DEFSIZE(MYD3DWRAP), SRS_DWORD, "Wrap7", storm_render::RenderWrap7},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "Clipping", storm_render::RenderClipping},
#endif
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "Lighting", storm_render::RenderLighting},
    {DEFSIZE(0), SRS_DWORD, "Ambient", storm_render::RenderAmbient},
#ifndef _XBOX
    {DEFSIZE(MYD3DFOGMODE), SRS_DWORD, "FogVertexMode", storm_render::RenderFogVertexMode},
#endif
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "ColorVertex", storm_render::RenderVertexColor},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "LocalViewer", storm_render::RenderLocalViewer},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "NormalizeNormals", storm_render::RenderNormalizeNormals},
    {DEFSIZE(MYD3DMATERIALCOLORSOURCE), SRS_DWORD, "DiffuseMaterialSource", storm_render::RenderDiffuseMaterialSource},
    {DEFSIZE(MYD3DMATERIALCOLORSOURCE), SRS_DWORD, "SpecularMaterialSource", storm_render::RenderSpecularMaterialSource},
    {DEFSIZE(MYD3DMATERIALCOLORSOURCE), SRS_DWORD, "AmbientMaterialSource", storm_render::RenderAmbientMaterialSource},
    {DEFSIZE(MYD3DMATERIALCOLORSOURCE), SRS_DWORD, "EmissiveMaterialSource", storm_render::RenderEmissiveMaterialSource},
    {DEFSIZE(MYD3DVERTEXBLENDFLAGS), SRS_DWORD, "VertexBlend", storm_render::RenderVertexBlend},
#ifndef _XBOX
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "ClipPlaneEnable", storm_render::RenderClipPlaneMask},
//{ DEFSIZE(MYTRUEFALSE) , SRS_DWORD, "SoftwareVertexProcessing",	D3DRS_SOFTWAREVERTEXPROCESSING },
#endif
    {DEFSIZE(0), SRS_FLOAT, "PointSize", storm_render::RenderPointSize},
    {DEFSIZE(0), SRS_FLOAT, "PointSize_Min", storm_render::RenderPointSizeMin},
    {DEFSIZE(0), SRS_FLOAT, "PointSize_Max", storm_render::RenderPointSizeMax},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "PointSpriteEnable", storm_render::RenderPointSprite},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "PointScaleEnable", storm_render::RenderPointScale},
    {DEFSIZE(0), SRS_FLOAT, "PointScale_A", storm_render::RenderPointScaleA},
    {DEFSIZE(0), SRS_FLOAT, "PointScale_B", storm_render::RenderPointScaleB},
    {DEFSIZE(0), SRS_FLOAT, "PointScale_C", storm_render::RenderPointScaleC},
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "MultiSampleAntialias", storm_render::RenderMultisampleAntialias},
    {DEFSIZE(0), SRS_DWORD, "MultiSampleMask", storm_render::RenderMultisampleMask},
    //{ DEFSIZE(0), SRS_FLOAT, "PatchSegments",						D3DRS_PATCHSEGMENTS },
    {DEFSIZE(MYD3DPATCHEDGESTYLE), SRS_DWORD, "PatchEdgeStyle", storm_render::RenderPatchEdgeStyle},
#ifndef _XBOX
    {DEFSIZE(MYTRUEFALSE), SRS_DWORD, "IndexedVertexBlendEnable", storm_render::RenderIndexedVertexBlend},
#endif
    {DEFSIZE(MYD3DCOLORWRITEMASK), SRS_DWORD, "ColorWriteEnable", storm_render::RenderColorWriteMask},
#ifndef _XBOX
    {DEFSIZE(0), SRS_FLOAT, "TweenFactor", storm_render::RenderTweenFactor},
#endif
    {DEFSIZE(MYD3DBLENDOP), SRS_DWORD, "BlendOp", storm_render::RenderBlendOp}};

// ---------------------------------------------------------------------------------
// SetTextureStageState parameters
SRSPARAM MYD3DTEXTUREOP[] = {{"disable", storm_render::TextureOpDisable},
                             {"selectarg1", storm_render::TextureOpSelectArg1},
                             {"selectarg2", storm_render::TextureOpSelectArg2},
                             {"modulate", storm_render::TextureOpModulate},
                             {"modulate2x", storm_render::TextureOpModulate2X},
                             {"modulate4x", storm_render::TextureOpModulate4X},
                             {"add", storm_render::TextureOpAdd},
                             {"addsigned", storm_render::TextureOpAddSigned},
                             {"addsigned2x", storm_render::TextureOpAddSigned2X},
                             {"subtract", storm_render::TextureOpSubtract},
                             {"substract", storm_render::TextureOpSubtract},
                             {"addsmooth", storm_render::TextureOpAddSmooth},
                             {"blenddiffusealpha", storm_render::TextureOpBlendDiffuseAlpha},
                             {"blendtexturealpha", storm_render::TextureOpBlendTextureAlpha},
                             {"blendfactoralpha", storm_render::TextureOpBlendFactorAlpha},
                             {"blendcurrentalpha", storm_render::TextureOpBlendCurrentAlpha},
                             {"blendtexturealphapm", storm_render::TextureOpBlendTextureAlphaPremultiplied},
                             {"premodulate", storm_render::TextureOpPremodulate},
                             {"modulatealpha_addcolor", storm_render::TextureOpModulateAlphaAddColor},
                             {"modulatecolor_addalpha", storm_render::TextureOpModulateColorAddAlpha},
                             {"modulateinvalpha_addcolor", storm_render::TextureOpModulateInverseAlphaAddColor},
                             {"modulateinvcolor_addalpha", storm_render::TextureOpModulateInverseColorAddAlpha},
                             {"bumpenvmap", storm_render::TextureOpBumpEnvMap},
                             {"bumpenvmapluminance", storm_render::TextureOpBumpEnvMapLuminance},
                             {"dotproduct3", storm_render::TextureOpDotProduct3},
                             {"multiplyadd", storm_render::TextureOpMultiplyAdd},
                             {"lerp", storm_render::TextureOpLerp}};

SRSPARAM MYD3DTAFLAGS[] = {{"current", storm_render::textureArg::current},
                           {"diffuse", storm_render::textureArg::diffuse},
                           {"selectmask", storm_render::textureArg::selectMask},
                           {"specular", storm_render::textureArg::specular},
                           {"temp", storm_render::textureArg::temporary},
                           {"texture", storm_render::textureArg::texture},
                           {"tfactor", storm_render::textureArg::factor},
                           {"alphareplicate", storm_render::textureArg::alphaReplicate},
                           {"complement", storm_render::textureArg::complement},
                           {"texture | complement", storm_render::textureArg::texture | storm_render::textureArg::complement},
                           {"current_alphareplicate", storm_render::textureArg::current | storm_render::textureArg::alphaReplicate},
                           {"diffuse_alphareplicate", storm_render::textureArg::diffuse | storm_render::textureArg::alphaReplicate},
                           {"specular_alphareplicate", storm_render::textureArg::specular | storm_render::textureArg::alphaReplicate},
                           {"temp_alphareplicate", storm_render::textureArg::temporary | storm_render::textureArg::alphaReplicate},
                           {"texture_alphareplicate", storm_render::textureArg::texture | storm_render::textureArg::alphaReplicate},
                           {"tfactor_alphareplicate", storm_render::textureArg::factor | storm_render::textureArg::alphaReplicate},
                           {"current_complement", storm_render::textureArg::current | storm_render::textureArg::complement},
                           {"diffuse_complement", storm_render::textureArg::diffuse | storm_render::textureArg::complement},
                           {"specular_complement", storm_render::textureArg::specular | storm_render::textureArg::complement},
                           {"temp_complement", storm_render::textureArg::temporary | storm_render::textureArg::complement},
                           {"texture_complement", storm_render::textureArg::texture | storm_render::textureArg::complement},
                           {"tfactor_complement", storm_render::textureArg::factor | storm_render::textureArg::complement}};

SRSPARAM MYD3DTEXCOORDINDEX[] = {{"passthru", storm_render::textureCoord::passthrough},
                                 {"cameraspacenormal", storm_render::textureCoord::cameraNormal},
                                 {"cameraspaceposition", storm_render::textureCoord::cameraPosition},
                                 {"cameraspacereflectionvector", storm_render::textureCoord::cameraReflection}};

SRSPARAM MYD3DTEXTUREADDRESS[] = {{"wrap", storm_render::TextureAddressWrap},
                                  {"mirror", storm_render::TextureAddressMirror},
                                  {"clamp", storm_render::TextureAddressClamp},
                                  {"border", storm_render::TextureAddressBorder},
#ifndef _XBOX
                                  {"mirroronce", storm_render::TextureAddressMirrorOnce}
#endif
};

SRSPARAM MYD3DTEXTUREFILTERTYPE[] = {
    {"none", storm_render::TextureFilterNone}, {"point", storm_render::TextureFilterPoint}, {"linear", storm_render::TextureFilterLinear}, {"anisotropic", storm_render::TextureFilterAnisotropic},
#ifndef _XBOX
//{ "flatcubic", D3DTEXF_FLATCUBIC },
#endif
    //{ "gaussiancubic", D3DTEXF_GAUSSIANCUBIC } };
};

SRSPARAM MYD3DTEXTURETRANSFORMFLAGS[] = {{"disable", storm_render::textureTransform::disabled}, {"count1", storm_render::textureTransform::count1},
                                         {"count2", storm_render::textureTransform::count2},   {"count3", storm_render::textureTransform::count3},
                                         {"count4", storm_render::textureTransform::count4},   {"projected", storm_render::textureTransform::projected}};

STSS TexturesStageStates[] = {
    {DEFSIZE(MYD3DTEXTUREOP), SRS_DWORD, "colorop", storm_render::StageColorOp},
    {DEFSIZE(MYD3DTAFLAGS), SRS_DWORD, "colorarg1", storm_render::StageColorArg1},
    {DEFSIZE(MYD3DTAFLAGS), SRS_DWORD, "colorarg2", storm_render::StageColorArg2},
    {DEFSIZE(MYD3DTEXTUREOP), SRS_DWORD, "alphaop", storm_render::StageAlphaOp},
    {DEFSIZE(MYD3DTAFLAGS), SRS_DWORD, "alphaarg1", storm_render::StageAlphaArg1},
    {DEFSIZE(MYD3DTAFLAGS), SRS_DWORD, "alphaarg2", storm_render::StageAlphaArg2},
    {DEFSIZE(0), SRS_FLOAT, "bumpenvmat00", storm_render::StageBumpEnvMatrix00},
    {DEFSIZE(0), SRS_FLOAT, "bumpenvmat01", storm_render::StageBumpEnvMatrix01},
    {DEFSIZE(0), SRS_FLOAT, "bumpenvmat10", storm_render::StageBumpEnvMatrix10},
    {DEFSIZE(0), SRS_FLOAT, "bumpenvmat11", storm_render::StageBumpEnvMatrix11},
    {DEFSIZE(MYD3DTEXCOORDINDEX), SRS_DWORD, "texcoordindex", storm_render::StageTexCoordIndex},
    /* { DEFSIZE(MYD3DTEXTUREADDRESS) , SRS_DWORD, "addressu",		D3DTSS_ADDRESSU },
     { DEFSIZE(MYD3DTEXTUREADDRESS) , SRS_DWORD, "addressv",		D3DTSS_ADDRESSV },
     { DEFSIZE(0), SRS_DWORD, "bordercolor",						D3DTSS_BORDERCOLOR },
     { DEFSIZE(MYD3DTEXTUREFILTERTYPE) , SRS_DWORD, "magfilter", D3DTSS_MAGFILTER },
     { DEFSIZE(MYD3DTEXTUREFILTERTYPE) , SRS_DWORD, "minfilter", D3DTSS_MINFILTER },
     { DEFSIZE(MYD3DTEXTUREFILTERTYPE) , SRS_DWORD, "mipfilter", D3DTSS_MIPFILTER },
     { DEFSIZE(0), SRS_FLOAT, "mipmaplodbias",					D3DTSS_MIPMAPLODBIAS },
     { DEFSIZE(0), SRS_DWORD, "maxmiplevel",						D3DTSS_MAXMIPLEVEL },
     { DEFSIZE(0), SRS_DWORD, "maxanisotropy",					D3DTSS_MAXANISOTROPY },*/
    {DEFSIZE(0), SRS_FLOAT, "bumpenvlscale", storm_render::StageBumpEnvLuminanceScale},
    {DEFSIZE(0), SRS_FLOAT, "bumpenvloffset", storm_render::StageBumpEnvLuminanceOffset},
    {DEFSIZE(MYD3DTEXTURETRANSFORMFLAGS), SRS_DWORD, "texturetransformflags", storm_render::StageTextureTransform},
    // { DEFSIZE(MYD3DTEXTUREADDRESS) , SRS_DWORD, "addressw", D3DTSS_ADDRESSW },
    {DEFSIZE(MYD3DTAFLAGS), SRS_DWORD, "colorarg0", storm_render::StageColorArg0},
    {DEFSIZE(MYD3DTAFLAGS), SRS_DWORD, "alphaarg0", storm_render::StageAlphaArg0},
    {DEFSIZE(MYD3DTAFLAGS), SRS_DWORD, "resultarg", storm_render::StageResultArg}};

SAMP SampleStates[] = {{DEFSIZE(MYD3DTEXTUREADDRESS), SRS_DWORD, "addressu", storm_render::SamplerAddressU},
                       {DEFSIZE(MYD3DTEXTUREADDRESS), SRS_DWORD, "addressv", storm_render::SamplerAddressV},
                       {DEFSIZE(0), SRS_DWORD, "bordercolor", storm_render::SamplerBorderColor},
                       {DEFSIZE(MYD3DTEXTUREFILTERTYPE), SRS_DWORD, "magfilter", storm_render::SamplerMagFilter},
                       {DEFSIZE(MYD3DTEXTUREFILTERTYPE), SRS_DWORD, "minfilter", storm_render::SamplerMinFilter},
                       {DEFSIZE(MYD3DTEXTUREFILTERTYPE), SRS_DWORD, "mipfilter", storm_render::SamplerMipFilter},
                       {DEFSIZE(0), SRS_FLOAT, "mipmaplodbias", storm_render::SamplerMipLodBias},
                       {DEFSIZE(0), SRS_DWORD, "maxmiplevel", storm_render::SamplerMaxMipLevel},
                       {DEFSIZE(0), SRS_DWORD, "maxanisotropy", storm_render::SamplerMaxAnisotropy},
                       {DEFSIZE(MYD3DTEXTUREADDRESS), SRS_DWORD, "addressw", storm_render::SamplerAddressW}};

// ---------------------------------------------------------------------------------
// transform block
SRSPARAM TRANSFORM_TYPES[] = {{"view", storm_render::TransformView},     {"projection", storm_render::TransformProjection}, {"world", storm_render::TransformWorld},
                              {"tex0", storm_render::TransformTexture0}, {"tex1", storm_render::TransformTexture1},         {"tex2", storm_render::TransformTexture2},
                              {"tex3", storm_render::TransformTexture3},
#ifndef _XBOX
                              {"tex4", storm_render::TransformTexture4}, {"tex5", storm_render::TransformTexture5},         {"tex6", storm_render::TransformTexture6},
                              {"tex7", storm_render::TransformTexture7}
#endif
};

// ---------------------------------------------------------------------------------
// params section for block
#define PARAM_MATRIX 0
#define PARAM_TEXTURE 1
#define PARAM_DWORD 2
#define PARAM_FLOAT 3

SRSPARAM PARAM_TYPES[] = {
    {"matrix", PARAM_MATRIX}, {"texture", PARAM_TEXTURE}, {"dword", PARAM_DWORD}, {"float", PARAM_FLOAT}};

// restores types

SRSPARAM RESTORE_TYPES[] = {{"states", SUBCODE_RESTORE_STATES}, {"clear", SUBCODE_RESTORE_CLEAR}};

CTechnique::CTechnique(VDX9RENDER *_pRS)
{
    dwNumBlocks = 0;
    dwNumShaders = 0;
    dwCurBlock = 0;
    dwNumParams = 0;
    dwCurMaxSavedSize = 0;
    dwNumSavedStates = 0;
    dwCurSavedStatesPos = 0;
    dwCurParamsMax = 0;

    pShaders = nullptr;
    pBlocks = nullptr;
    pParams = nullptr;
    pSavedStates = nullptr;
    pCurParams = nullptr;

    pRS = _pRS;

    std::ranges::fill(sDelimTable, 0);
    char sDelimeters[] = " ,.[]-+\0\n\r\t";
    size_t len = strlen(sDelimeters);
    for (uint32_t i = 0; i < len; i++)
        sDelimTable[sDelimeters[i]] = 1;
}

CTechnique::~CTechnique()
{
    uint32_t i, j, k;
    for (i = 0; i < dwNumBlocks; i++)
    {
        block_t *pB = &pBlocks[i];
        for (j = 0; j < pB->dwNumTechniques; j++)
        {
            technique_t *pTech = &pB->pTechniques[j];
            for (k = 0; k < pTech->dwNumPasses; k++)
                STORM_DELETE(pTech->pPasses[k].pPass);
            free(pTech->pPasses);
        }
        STORM_DELETE(pB->pParams);
        STORM_DELETE(pB->pBlockName);
        free(pB->pTechniques);
    }
    for (i = 0; i < dwNumShaders; i++)
    {
        STORM_DELETE(pShaders[i].pName);
        STORM_DELETE(pShaders[i].pDecl);
        if (pShaders[i].pPixelShader != nullptr)
        {
            pShaders[i].pPixelShader->Release();
            pShaders[i].pPixelShader = nullptr;
        }
        if (pShaders[i].pVertexShader != nullptr)
        {
            pShaders[i].pVertexShader->Release();
            pShaders[i].pVertexShader = nullptr;
        }
    }
    free(pShaders);
    free(pBlocks);
    free(pSavedStates);
    free(pCurParams);
}

#define END_TEST (uint32_t(pCurrent - pBegin) >= dwSize - 2)

char *GetString(char *pBegin, uint32_t dwSize, char *pCurrent)
{
    if (END_TEST)
        return nullptr;
    // go for a null, or 0xA, or 0xD
    if (pCurrent != pBegin) // for first string
    {
        while ((*pCurrent && *pCurrent != 0xa && *pCurrent != 0xd) && !END_TEST)
            *pCurrent++; //~!~
        if (END_TEST)
            return nullptr;
    }
    // skip all nulls, or 0xA, or 0xD
    while ((!(*pCurrent) || (*pCurrent == 0xa && *pCurrent == 0xd)) && !END_TEST)
        *pCurrent++; //~!~
    if (END_TEST)
        return nullptr;
    return pCurrent;
}

void ClearComment(char *pStr)
{
    // search comment and delete all symbols after
    if (!pStr || !pStr[0])
        return;
    int32_t iLen = strlen(pStr);
    char *pSkip1 = FindToken(pStr, COMMENT1);
    int32_t iStart1 = iLen, iStart2 = iLen;
    if (pSkip1)
        iStart1 = (pSkip1 - pStr) - strlen(COMMENT1);
    char *pSkip2 = FindToken(pStr, COMMENT2);
    if (pSkip2)
        iStart2 = (pSkip2 - pStr) - strlen(COMMENT2);
    int32_t iStart = (iStart2 > iStart1) ? iStart1 : iStart2;
    for (int32_t i = iStart; i < iLen; i++)
        pStr[i] = 0;
}

bool isVertexDeclaration(char *pStr)
{
    return (SkipToken(pStr, VERTEX_DECL)) ? true : false;
}
bool isAsm(char *pStr)
{
    return (SkipToken(pStr, ASM_SHADER)) ? true : false;
}
bool isVertexShaderConst(char *pStr)
{
    return (SkipToken(pStr, VERTEX_SHADER_CONST)) ? true : false;
}
bool isPixelShaderConst(char *pStr)
{
    return (SkipToken(pStr, PIXEL_SHADER_CONST)) ? true : false;
}
bool isVertexShader(char *pStr)
{
    return (SkipToken(pStr, VERTEX_SHADER)) ? true : false;
}
bool isPixelShader(char *pStr)
{
    return (SkipToken(pStr, PIXEL_SHADER)) ? true : false;
}
bool isBlock(char *pStr)
{
    return (SkipToken(pStr, BLOCK)) ? true : false;
}
bool isTechnique(char *pStr)
{
    return (SkipToken(pStr, TECHNIQUE)) ? true : false;
}
bool isPass(char *pStr)
{
    return (SkipToken(pStr, PASS)) ? true : false;
}
bool isInclude(char *pStr)
{
    return (SkipToken(pStr, INCLUDE)) ? true : false;
}
bool isDefine(char *pStr)
{
    return (SkipToken(pStr, DEFINE)) ? true : false;
}
bool isBeginBracket(char *pStr)
{
    return (SkipToken(pStr, BEGIN_BRACKET)) ? true : false;
}
bool isEndBracket(char *pStr)
{
    return (SkipToken(pStr, END_BRACKET)) ? true : false;
}
bool isDigit(char *pStr)
{
    return ((pStr) ? (pStr[0] >= '0' && pStr[0] <= '9' || pStr[0] == '-') : false);
}
bool hasPoint(const char *pStr)
{
    while (*pStr != '\0')
    {
        if (*pStr == '.')
        {
            return true;
        }
        pStr++;
    }
    return false;
}

uint32_t CTechnique::GetSRSIndex(char *pStr)
{
    uint32_t dwNumParam = sizeof(RenderStates) / sizeof(STSS);
    for (uint32_t i = 0; i < dwNumParam; i++)
        if (storm::iEquals(pStr, RenderStates[i].cName))
            return i;
    core.Trace("ERROR: SetRenderState: unknown parameter type <%s> in <%s> file, technique <%s>", pStr,
               sCurrentFileName, sCurrentBlockName);
    // THROW;
    return INVALID_SRS_INDEX;
}

uint32_t CTechnique::GetSTSSIndex(char *pStr)
{
    uint32_t dwNumParam = sizeof(TexturesStageStates) / sizeof(STSS);
    for (uint32_t i = 0; i < dwNumParam; i++)
        if (storm::iEquals(pStr, TexturesStageStates[i].cName))
            return i;
    // api->Trace("ERROR: SetTextureStageState: unknown parameter type <%s> in <%s> file, technique
    // <%s>",pStr,sCurrentFileName,sCurrentBlockName); THROW;
    return INVALID_INDEX;
}

uint32_t CTechnique::GetSAMPIndex(char *pStr)
{
    uint32_t dwNumParam = sizeof(SampleStates) / sizeof(SAMP);
    for (uint32_t i = 0; i < dwNumParam; i++)
        if (storm::iEquals(pStr, SampleStates[i].cName))
            return i;
    // api->Trace("ERROR: SetSamplerState: unknown parameter type <%s> in <%s> file, technique <%s>", pStr,
    // sCurrentFileName, sCurrentBlockName); THROW;
    return INVALID_INDEX;
}

uint32_t CTechnique::GetIndex(char *pStr, SRSPARAM *pParam, uint32_t dwNumParam, bool bCanBeNumber)
{
    for (uint32_t i = 0; i < dwNumParam; i++)
        if (storm::iEquals(pStr, pParam[i].cName))
            return i;
    if (!bCanBeNumber)
        core.Trace("ERROR: Unknown parameter type <%s> in <%s> file, technique <%s>", pStr, sCurrentFileName,
                   sCurrentBlockName);
    // THROW;
    return INVALID_INDEX;
}

uint32_t CTechnique::GetCode(char *pStr, SRSPARAM *pParam, uint32_t dwNumParam, uint32_t *pPassCode, bool bCanBeNumber)
{
    Assert(pStr);

    uint32_t dwCode = 0;
    if (pParam && dwNumParam)
    {
        uint32_t dwIndex = GetIndex(pStr, pParam, dwNumParam, bCanBeNumber);
        if (dwIndex != INVALID_INDEX)
            return pParam[dwIndex].dwCode;
    }
    // check for numbers

    if (isDigit(pStr))
    {
        if (hasPoint(pStr))
        {
            float fvalue;
            sscanf(pStr, "%f", &fvalue);
            dwCode = F2DW(fvalue);
        }
        else
        {
            sscanf(pStr, (SkipToken(pStr, "0x")) ? "%x" : "%d", &dwCode);
        }
        if (pPassCode)
            *pPassCode |= FLAGS_CODE_NUMBER;
        return dwCode;
    }
    else
    {
        core.Trace("ERROR: unknown parameter type <%s> in <%s> file, technique <%s>", pStr, sCurrentFileName,
                   sCurrentBlockName);
        return 0;
    }
}

uint32_t CTechnique::AddShader(char *pShaderName)
{
    // find exist shader or create empty new
    for (uint32_t i = 0; i < dwNumShaders; i++)
        if (storm::iEquals(pShaders[i].pName, pShaderName))
            return i;
    pShaders = (shader_t *)realloc(pShaders, sizeof(shader_t) * (dwNumShaders + 1));
    shader_t *pS = &pShaders[dwNumShaders];
    pShaders[dwNumShaders] = {};
    const auto len = strlen(pShaderName) + 1;
    pS->pName = new char[len];
    memcpy(pS->pName, pShaderName, len);
    pS->dwHashName = MakeHashValue(pShaderName);
    pS->pVertexDecl = nullptr;
    pS->pVertexShader = nullptr;
    pS->pPixelShader = nullptr;
    dwNumShaders++;
    return (dwNumShaders - 1);
}

uint32_t CTechnique::ProcessPass(char *pFile, uint32_t dwSize, char **pStr)
{
    block_t *pB = &pBlocks[dwNumBlocks];
    technique_t *pTechniques = &pB->pTechniques[pB->dwNumTechniques];

    pTechniques->pPasses = (pass_t *)realloc(pTechniques->pPasses, sizeof(pass_t) * (pTechniques->dwNumPasses + 1));
    pTechniques->pPasses[pTechniques->dwNumPasses] = {};

    uint32_t *pPass = pPassStorage;
    uint32_t *pPassBegin = pPass;
    uint32_t dwTextureIndex, dwAdditionalFlags = 0;
    char *pTemp, temp[256];
    while (nullptr != (*pStr = GetString(pFile, dwSize, *pStr)))
    {
#define SKIP3                                                                                                          \
    {                                                                                                                  \
        (*pStr)++;                                                                                                     \
        continue;                                                                                                      \
    }
        tolwr(*pStr);
        ClearComment(*pStr);
        if (isEndBracket(*pStr))
        {
#ifdef USE_FX // fx files has no "Render." and "Restore." strings
            *pPass++ = CODE_RENDER;
            *pPass++ = SUBCODE_RENDER_DRAW;

            *pPass++ = CODE_RESTORE;
            *pPass++ = SUBCODE_RESTORE_STATES;
            // clear STSS and SRS bUse
            ClearSRS_STSS_bUse();
#endif
            break; // end of pass
        }

        // check right side of expression for [in] parameters
        bool bIn = false;
        uint32_t dwInParamIndex = 0;
#ifdef false // current sha files has no params. Last usage was in
             // ENGINE/modules/techniques/weather/shaders/SeaFoam.sha. Removed here:
             // https://github.com/storm-devs/thunderstorm-engine/commit/f290ca08ff33d3f86fcdf6aef26b0e20e36ab54c
        if (pTemp = SkipToken(*pStr, INPARAM_CHECK)) // STSS or SetTexture
        {
            GetTokenWhile(pTemp, &temp[0], ">");
            bIn = true;
            dwInParamIndex = GetIndex(temp, pParams, dwNumParams, false);
            dwAdditionalFlags |= FLAGS_CODE_IN_PARAM;
        }
#endif

        // check for additional flags such as "no restore states" and other
        dwAdditionalFlags |= FLAGS_CODE_RESTORE;
        if (SkipToken(*pStr, NOSAVE_STATE))
        {
            *pStr = SkipToken(*pStr, NOSAVE_STATE);
            dwAdditionalFlags &= (~FLAGS_CODE_RESTORE);
        }

#ifndef USE_FX // fx files has no "Restore." strings
        // restore states check
        if (pTemp = SkipToken(*pStr, RESTORE_STATES_CHECK))
        {
            *pPass++ = CODE_RESTORE;
            *pPass++ = GetCode(pTemp, RESTORE_TYPES, sizeof(RESTORE_TYPES) / sizeof(SRSPARAM), nullptr);
            // clear STSS and SRS bUse
            ClearSRS_STSS_bUse();
            SKIP3;
        }
#endif
        if (isPixelShaderConst(*pStr))
        {
            if (!bIn)
                psnip_trap();
            uint32_t dwIndex = 0;
            *pPass++ = CODE_SPSCONST | dwAdditionalFlags;
            GetTokenWhile(SkipToken(*pStr, "["), &temp[0], "]");
            sscanf(temp, "%u", &dwIndex);
            *pPass++ = dwIndex;
            *pPass++ = dwInParamIndex;
            // GetTokenWhile(SkipToken(*pStr,"="),temp,";");
            //*pPass++ = GetCode(temp,0,0,null,false);
            SKIP3;
        }

        if (isVertexShaderConst(*pStr))
        {
            uint32_t dwIndex = 0;
            *pPass++ = CODE_SVSCONST | dwAdditionalFlags;
            GetTokenWhile(SkipToken(*pStr, "["), &temp[0], "]");
            sscanf(temp, "%u", &dwIndex);
            *pPass++ = dwIndex;
            GetTokenWhile(SkipToken(*pStr, "="), temp, ";");
            *pPass++ = GetCode(temp, &MYSETVERTEXSHADERCONSTANT[0],
                               sizeof(MYSETVERTEXSHADERCONSTANT) / sizeof(SRSPARAM), nullptr, false);
            SKIP3;
        }

        // check for '[' if exist mean what STSS used
        if (pTemp = SkipToken(*pStr, STSS_CHECK)) // STSS or SetTexture
        {
            // get index = [index]
            GetTokenWhile(pTemp, &temp[0], "]");
            sscanf(temp, "%d", &dwTextureIndex);

            // check for "texture["
            if (SkipToken(*pStr, TEXTURE_CHECK))
            {
                // texture set
                *pPass++ = CODE_TEXTURE | dwAdditionalFlags;
                *pPass++ = dwTextureIndex;
                if (bIn)
                    *pPass++ = dwInParamIndex;
                else
                {
                }
                SKIP3;
            }
            // SetTextureStageState
            uint32_t *pPassCode = pPass;

            // get left side of expression
            GetTokenWhile(*pStr, temp, "[");
            uint32_t dwIndex = GetSTSSIndex(temp);
            if (dwIndex != INVALID_INDEX)
            {
                *pPass++ = CODE_STSS | dwAdditionalFlags;
                *pPass++ = dwTextureIndex;
                if (TexturesStageStates[dwIndex].bUse &&
                    TexturesStageStates[dwIndex].dwUseSubCode & (1 << dwTextureIndex) &&
                    dwAdditionalFlags & FLAGS_CODE_RESTORE)
                    core.Trace("WARN: STSS: dup restore param type <%s[%d]> in <%s> file, technique <%s>", temp,
                               dwTextureIndex, sCurrentFileName, sCurrentBlockName);
                if (dwAdditionalFlags & FLAGS_CODE_RESTORE)
                {
                    TexturesStageStates[dwIndex].bUse = true;
                    TexturesStageStates[dwIndex].dwUseSubCode |= (1 << dwTextureIndex);
                }
                *pPass++ = (uint32_t)TexturesStageStates[dwIndex].State;

                if (bIn)
                    *pPass++ = dwInParamIndex;
                else
                {
                    // get right side of expression
                    GetTokenWhile(SkipToken(*pStr, "="), temp, ";");
                    *pPass++ = GetCode(temp, TexturesStageStates[dwIndex].pParam,
                                       TexturesStageStates[dwIndex].dwParamNum, pPassCode, true);
                }
            }
            else
            {
                *pPass++ = CODE_SAMP | dwAdditionalFlags;
                *pPass++ = dwTextureIndex;
                dwIndex = GetSAMPIndex(temp);
                if (dwIndex != INVALID_INDEX)
                {
                    if (SampleStates[dwIndex].bUse && SampleStates[dwIndex].dwUseSubCode & (1 << dwTextureIndex) &&
                        dwAdditionalFlags & FLAGS_CODE_RESTORE)
                        core.Trace("WARN: SAMP: dup restore param type <%s[%d]> in <%s> file, technique <%s>", temp,
                                   dwTextureIndex, sCurrentFileName, sCurrentBlockName);
                    if (dwAdditionalFlags & FLAGS_CODE_RESTORE)
                    {
                        SampleStates[dwIndex].bUse = true;
                        SampleStates[dwIndex].dwUseSubCode |= (1 << dwTextureIndex);
                    }
                    *pPass++ = (uint32_t)SampleStates[dwIndex].State;

                    if (bIn)
                        *pPass++ = dwInParamIndex;
                    else
                    {
                        // get right side of expression
                        GetTokenWhile(SkipToken(*pStr, "="), temp, ";");
                        *pPass++ = GetCode(temp, SampleStates[dwIndex].pParam, SampleStates[dwIndex].dwParamNum,
                                           pPassCode, true);
                    }
                }
                else
                    core.Trace("ERROR: unknown parameter type <%s> in <%s> file, technique <%s>", pStr,
                               sCurrentFileName, sCurrentBlockName);
            }
            SKIP3;
        }

        // PIXEL_SHADER, VERTEX_SHADER
        if (SkipToken(*pStr, PIXEL_SHADER))
        {
            *pPass++ = CODE_SPS;
#ifdef USE_FX
            pTemp = SkipToken(*pStr, "=");
            GetShaderBinPath(pTemp, CODE_SPS, temp);

            auto i = AddShader(temp);
            shader_t *pS = &pShaders[i];
            if (pS->dwShaderType == 0)
            {
                pS->dwShaderType = CODE_SPS;
                ProcessShaderBin(pS, temp, CODE_SPS);
            }
#else
            GetTokenWhile(SkipToken(*pStr, "\""), temp, "\"");
            auto i = AddShader(temp);
#endif
            *pPass++ = i;
            SKIP3;
        }

        if (SkipToken(*pStr, VERTEX_SHADER))
        {
            *pPass++ = CODE_SVS;
#ifdef USE_FX
            pTemp = SkipToken(*pStr, "=");
            GetShaderBinPath(pTemp, CODE_SVS, temp);

            auto i = AddShader(temp);
            shader_t *pS = &pShaders[i];
            if (pS->dwShaderType == 0)
            {
                pS->dwShaderType = CODE_SVS;
                ProcessShaderBin(pS, temp, CODE_SVS);
            }
#else
            GetTokenWhile(SkipToken(*pStr, "\""), temp, "\"");
            auto i = AddShader(temp);
#endif
            *pPass++ = i;
            SKIP3;
        }

#ifndef USE_FX // fx files has no "Render." strings
        // RENDER
        if (SkipToken(*pStr, RENDER_CHECK))
        {
            *pPass++ = CODE_RENDER;
            *pPass++ = SUBCODE_RENDER_DRAW;
            SKIP3;
        }
#endif

#ifdef false // current sha files has no "transform." strings
        // transform
        if (SkipToken(*pStr, TRANSFORM_CHECK))
        {
            //
            GetTokenWhile(SkipToken(*pStr, "."), temp, "=");
            uint32_t dwCode = GetCode(temp, TRANSFORM_TYPES, sizeof(TRANSFORM_TYPES) / sizeof(SRSPARAM), nullptr);
            if (dwCode == 0xFFFFFFFF) // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
            {
                // maybe world0-world256
                if (nullptr == (pTemp = SkipToken(*pStr, WORLD_TRANSFORM_CHECK)))
                    throw std::runtime_error("transform. error!");
                sscanf(pTemp, "%u", &dwCode);
                dwCode = (uint32_t)storm_render::worldTransform(dwCode);
            }
            *pPass++ = CODE_TRANSFORM;
            *pPass++ = dwCode;
            *pPass++ = dwInParamIndex;
            SKIP3;
        }
#endif

        // SetRenderState
        if (SkipToken(*pStr, "="))
        {
            // get left side of expression
            GetTokenWhile(*pStr, temp, "=");
            uint32_t dwSRSIndex = GetSRSIndex(temp);
            Assert(dwSRSIndex != INVALID_SRS_INDEX);
            if (RenderStates[dwSRSIndex].bUse && dwAdditionalFlags & FLAGS_CODE_RESTORE)
                core.Trace("WARN: SRS: dup restore param type <%s> in <%s> file, technique <%s>", temp,
                           sCurrentFileName, sCurrentBlockName);
            if (dwAdditionalFlags & FLAGS_CODE_RESTORE)
                RenderStates[dwSRSIndex].bUse = true;
            uint32_t *pPassCode = pPass;
            *pPass++ = CODE_SRS | dwAdditionalFlags;
            *pPass++ = (uint32_t)RenderStates[dwSRSIndex].State;

            if (bIn)
                *pPass++ = dwInParamIndex;
            else
            {
                // get right side of expression
                GetTokenWhile(SkipToken(*pStr, "="), temp, ";");
                *pPass++ = GetCode(temp, RenderStates[dwSRSIndex].pParam, RenderStates[dwSRSIndex].dwParamNum,
                                   pPassCode, true);
            }
            SKIP3;
        }

        // other skip
        SKIP3;
    }

    uint32_t dwSizePass = pPass - pPassBegin;
    pPassBegin = new uint32_t[dwSizePass]; //(uint32_t*)RESIZE(pPassBegin,dwSizePass*sizeof(uint32_t));
    memcpy(pPassBegin, pPassStorage, dwSizePass * sizeof(uint32_t));
    pTechniques->pPasses[pTechniques->dwNumPasses].dwSize = dwSizePass;
    pTechniques->pPasses[pTechniques->dwNumPasses].isValidate = true;
    pTechniques->pPasses[pTechniques->dwNumPasses].pPass = pPassBegin;

    pTechniques->dwNumPasses++;
    return PASS_OK;
}

uint32_t CTechnique::ProcessTechnique(char *pFile, uint32_t dwSize, char **pStr)
{
    block_t *pB = &pBlocks[dwNumBlocks];
    pB->pTechniques = (technique_t *)realloc(pB->pTechniques, sizeof(technique_t) * (pB->dwNumTechniques + 1));
    pB->pTechniques[pB->dwNumTechniques] = {};
    // clear STSS and SRS bUse
    ClearSRS_STSS_bUse();
    // search for pass and '}'
    while (nullptr != (*pStr = GetString(pFile, dwSize, *pStr)))
    {
#define SKIP2                                                                                                          \
    {                                                                                                                  \
        (*pStr)++;                                                                                                     \
        continue;                                                                                                      \
    }
        tolwr(*pStr);
        ClearComment(*pStr);
        // if (isComment(*pStr)) SKIP2;
        if (isEndBracket(*pStr))
            break; // end of technique
        if (isPass(*pStr))
            ProcessPass(pFile, dwSize, pStr);
        SKIP2;
    }

    pB->dwNumTechniques++;
    return TECHNIQUE_OK;
}

#define TOTAL_SKIP                                                                                                     \
    {                                                                                                                  \
        (*pStr)++;                                                                                                     \
        continue;                                                                                                      \
    }

void AddDeclElement(shader_t *pS)
{
    pS->dwDeclSize++;
    pS->pDecl = (storm_render::VertexElement *)realloc(pS->pDecl, sizeof(storm_render::VertexElement) * pS->dwDeclSize);
    pS->pDecl[pS->dwDeclSize - 1].Method = storm_render::VertexMethod::Default;
}

storm_render::VertexElement *CurDeclElement(shader_t *pS)
{
    return &pS->pDecl[pS->dwDeclSize - 1];
}

uint32_t CTechnique::ProcessVertexDeclaration(shader_t *pS, char *pFile, uint32_t dwSize, char **pStr)
{
    uint32_t dwTemp;
    ;
    uint32_t dwStream = 0, dwOffset = 0;
    uint32_t dwIndexes[std::size(MYDECLUSAGE)] = {0};

    while (nullptr != (*pStr = GetString(pFile, dwSize, *pStr)))
    {
        tolwr(*pStr);
        ClearComment(*pStr);
        if (isEndBracket(*pStr))
            break; // end of declaration
        if (SkipToken(*pStr, VDECL_STREAM_CHECK))
        {
            sscanf(SkipToken(*pStr, "["), "%d", &dwTemp);
            dwStream = dwTemp;
        }
        if (SkipToken(*pStr, VDECL_FLOAT_CHECK))
        {
            sscanf(SkipToken(*pStr, "["), "%u", &dwTemp);
            switch (dwTemp)
            {
            case 1:
                dwTemp = static_cast<uint32_t>(storm_render::VertexComponent::Float1);
                break;
            case 2:
                dwTemp = static_cast<uint32_t>(storm_render::VertexComponent::Float2);
                break;
            case 3:
                dwTemp = static_cast<uint32_t>(storm_render::VertexComponent::Float3);
                break;
            case 4:
                dwTemp = static_cast<uint32_t>(storm_render::VertexComponent::Float4);
                break;
            }
            AddDeclElement(pS);
            CurDeclElement(pS)->Offset = dwOffset;
            CurDeclElement(pS)->Stream = dwStream;
            CurDeclElement(pS)->Type = static_cast<storm_render::VertexComponent>(dwTemp);
            dwOffset += 4 * (dwTemp + 1);

            // Usage
            dwTemp = ~0;
            for (int i = 0; i < std::size(MYDECLUSAGE); i++)
            {
                if (SkipToken(*pStr, MYDECLUSAGE[i].cName))
                {
                    dwTemp = MYDECLUSAGE[i].dwCode;
                    break;
                }
            }
            CurDeclElement(pS)->Usage = static_cast<storm_render::VertexSemantic>(dwTemp);
            if (dwTemp != ~0)
                CurDeclElement(pS)->UsageIndex = dwIndexes[dwTemp]++;
        }
        if (SkipToken(*pStr, VDECL_COLOR_CHECK))
        {
            AddDeclElement(pS);
            CurDeclElement(pS)->Offset = dwOffset;
            CurDeclElement(pS)->Stream = dwStream;
            CurDeclElement(pS)->Type = storm_render::VertexComponent::Color;
            CurDeclElement(pS)->Usage = storm_render::VertexSemantic::Color;
            CurDeclElement(pS)->UsageIndex = dwIndexes[static_cast<uint32_t>(storm_render::VertexSemantic::Color)]++;
            dwOffset += 4;
        }

        TOTAL_SKIP;
    }

    AddDeclElement(pS);
    *CurDeclElement(pS) = storm_render::vertexLayoutTerminator();

    storm_render::Result hr = pRS->CreateVertexDeclaration(pS->pDecl, &pS->pVertexDecl);
    if (hr != storm_render::result::ok)
        core.Trace("ERROR: invalid shader declaration <%s>", pS->pName);

    return 0;
}

char *CTechnique::GetToken(char *pToken, char *pResult, bool &bToken)
{
    char *pStart = pToken;
    bToken = false;
    while (*pToken && !sDelimTable[*pToken])
        pToken++;
    pResult[pToken - pStart] = 0;
    if (pToken - pStart == 0 && *pToken == 0)
        return nullptr;
    if (pToken - pStart == 0 && sDelimTable[*pToken])
    {
        bToken = true;
        pResult[0] = *pToken;
        pResult[1] = 0;
        return pToken + 1;
    }
    strncpy_s(pResult, pToken - pStart + 1, pStart, pToken - pStart);
    return pToken;
}

char *CTechnique::Preprocessor(char *pBuffer, uint32_t &dwSize)
{
    bool bToken;
    char sValue[256], sName[256], sToken[1024];
    char *pStr = pBuffer;
    uint32_t dwBufSize = strlen(pBuffer);
    uint32_t i, dwNumDefines = 0;
    char *pProgram = nullptr;
    uint32_t dwProgramSize = 0;
    define_t *pDefines = nullptr;
    uint32_t dwMaxDefineElements = 0;
    uint32_t dwMaxProgramElements = 0;
    uint32_t dwNewSize;

    // change all 0xd 0xa to 0x0
    for (i = 0; i < dwSize; i++)
    {
        if (pBuffer[i] == 0xa || pBuffer[i] == 0xd)
            pBuffer[i] = 0;
        if (pBuffer[i] == 0x9)
            pBuffer[i] = ' ';
    }

    while (nullptr != (pStr = GetString(pBuffer, dwBufSize, pStr)))
    {
        // tolwr(pStr);
        ClearComment(pStr);
        if (isDefine(pStr))
        {
            GetTokenWhile(SkipToken(pStr, DEFINE), sName, " ");
            GetTokenWhile(SkipToken(pStr, sName), sValue, " ");
            dwNewSize = 16 * ((dwNumDefines / 16) + 1);
            if (dwMaxDefineElements != dwNewSize)
            {
                dwMaxDefineElements = dwNewSize;
                pDefines = (define_t *)realloc(pDefines, sizeof(define_t) * dwMaxDefineElements);
            }
            auto len = strlen(sName) + 1;
            pDefines[dwNumDefines].pName = new char[len];
            memcpy(pDefines[dwNumDefines].pName, sName, len);

            pDefines[dwNumDefines].dwNameLen = len - 1;

            len = strlen(sValue) + 1;
            pDefines[dwNumDefines].pValue = new char[len];
            memcpy(pDefines[dwNumDefines].pValue, sValue, len);

            dwNumDefines++;
            pStr++;
            continue;
        }
        // else real code string
        bool bNewString = false;
        char *pToken = pStr;
        while (nullptr != (pToken = GetToken(pToken, sToken, bToken)))
        {
            uint32_t dwTempLen = strlen(sToken);
            if (!bToken)
            {
                // find define
                for (i = 0; i < dwNumDefines; i++)
                    if (dwTempLen == pDefines[i].dwNameLen && storm::iEquals(pDefines[i].pName, sToken))
                    {
                        if (pDefines[i].pValue)
                            strcpy_s(sToken, pDefines[i].pValue);
                        dwTempLen = strlen(sToken);
                        break;
                    }
            }
            // add sToken to programm
            dwNewSize = 8192 * (((dwTempLen + dwProgramSize + 4) / 8192) + 1);
            if (dwNewSize > dwMaxProgramElements)
            {
                dwMaxProgramElements = dwNewSize;
                pProgram = (char *)realloc(pProgram, dwMaxProgramElements);
            }
            strcpy_s(&pProgram[dwProgramSize], dwNewSize - dwProgramSize, sToken);
            dwProgramSize += dwTempLen;
            bNewString = true;
        }
        if (pProgram && bNewString)
        {
            pProgram[dwProgramSize] = 0xd;
            pProgram[dwProgramSize + 1] = 0xa;
            pProgram[dwProgramSize + 2] = 0x0;
            dwProgramSize += 2;
        }
        pStr++;
        continue;
    }
    STORM_DELETE(pBuffer);
    if (pProgram)
        pProgram[dwProgramSize] = 0x0;
    dwSize = dwProgramSize;
    // api->Trace(pProgram);
    for (i = 0; i < dwNumDefines; i++)
    {
        STORM_DELETE(pDefines[i].pName);
        STORM_DELETE(pDefines[i].pValue);
    }
    STORM_DELETE(pDefines);

    return pProgram;
}

uint32_t CTechnique::ProcessShaderAsm(shader_t *pS, char *pFile, uint32_t dwSize, char **pStr, uint32_t dwShaderType,
                                      bool HLSL)
{
    uint32_t dwFileSize;
    uint32_t dwTotalLen = 0;
    char *pBuffer = nullptr;
    while (nullptr != (*pStr = GetString(pFile, dwSize, *pStr)))
    {
        // tolwr(*pStr);
        ClearComment(*pStr);
        if (isBeginBracket(*pStr))
            TOTAL_SKIP;
        if (isEndBracket(*pStr))
            break; // end of declaration

        char *pTemp = *pStr;
        char *pTempBuffer = nullptr;

        if (isInclude(*pStr))
        {
            char sIncFileName[256], sName[256];
            GetTokenWhile(SkipToken(*pStr, "\""), &sName[0], "\"");
            sprintf_s(sIncFileName, "%s\\%s", sCurrentDir, sName);
            if (!fio->LoadFile(sIncFileName, &pTempBuffer, &dwFileSize))
            {
                core.Trace("ERROR: in file %s, file not found : %s", sCurrentFileName, sIncFileName);
                TOTAL_SKIP;
            }
            pTempBuffer[dwFileSize - 1] = 0x0;
            pTemp = pTempBuffer;
        }

        int32_t iLen = strlen(pTemp) - 1;
        pBuffer = (char *)realloc(pBuffer, dwTotalLen + iLen + 4);
        strcpy_s(&pBuffer[dwTotalLen], iLen + 4, pTemp);
        strcpy_s(&pBuffer[dwTotalLen + iLen + 1], 3, "\r\n");
        dwTotalLen += iLen + 2;
        STORM_DELETE(pTempBuffer);
        TOTAL_SKIP;
    }
#ifdef _WIN32 // replace D3DXAssembleShader with D3DCompile or similar
#ifndef _XBOX
    ID3DXBuffer *CompiledShader = nullptr, *ErrorShader = nullptr;
#else
    LPXGBUFFER CompiledShader = null, ErrorShader = null;
#endif
    storm_render::Result hr;
    pBuffer = Preprocessor(pBuffer, dwTotalLen);

    if (HLSL)
    {
        if (dwShaderType == CODE_SVS)
            hr = D3DXCompileShader(pBuffer, dwTotalLen, nullptr, nullptr, "main", "vs_3_0",
                                   D3DXSHADER_OPTIMIZATION_LEVEL3, &CompiledShader, &ErrorShader, nullptr);
        else
            hr = D3DXCompileShader(pBuffer, dwTotalLen, nullptr, nullptr, "main", "ps_3_0",
                                   D3DXSHADER_OPTIMIZATION_LEVEL3, &CompiledShader, &ErrorShader, nullptr);
    }
    else
        hr = D3DXAssembleShader(pBuffer, dwTotalLen, nullptr, nullptr, 0, &CompiledShader, &ErrorShader);

    //#ifndef _XBOX
    /*#else
        hr = XGAssembleShader(0, pBuffer, dwTotalLen, 0, 0, &CompiledShader, &ErrorShader, 0, 0, 0,
    0);//D3DXAssembleShader(pBuffer,dwTotalLen,0,0,&CompiledShader,&ErrorShader); #endif*/
    if (storm_render::result::ok != hr && ErrorShader)
    {
        const auto len = ErrorShader->GetBufferSize() + 1;
        char *pErrStr = new char[len];
        strncpy_s(pErrStr, len, (char *)ErrorShader->GetBufferPointer(), ErrorShader->GetBufferSize());
        pErrStr[ErrorShader->GetBufferSize()] = 0;
        core.Trace("ERROR: in compile shader %s\nerror:\n%s", pS->pName, pErrStr);
        RELEASE(CompiledShader);
        RELEASE(ErrorShader);
        STORM_DELETE(pBuffer);
        STORM_DELETE(pErrStr);
        return 0;
    }
    if (dwShaderType == CODE_SVS)
        hr = pRS->ImportVertexProgram((uint32_t *)CompiledShader->GetBufferPointer(), &pS->pVertexShader);
    else
        hr = pRS->ImportFragmentProgram((uint32_t *)CompiledShader->GetBufferPointer(), &pS->pPixelShader);

    /*#ifndef _XBOX
    #else
            hr = pRS->CreatePixelShader((uint32_t*)(D3DPIXELSHADERDEF
    *)CompiledShader->GetBufferPointer(),&pS->dwShaderHandle); #endif*/

    RELEASE(CompiledShader);
    RELEASE(ErrorShader);
#endif
    STORM_DELETE(pBuffer);
    return 0;
}

void CTechnique::GetShaderBinPath(char *pShaderStr, uint32_t dwShaderType, char *dest)
{
    // dest will contain path to shader file "currentFilename_shaderName.vso" (or .pso) in folder of sCurrentFileName
    char sShaderName[256];
    if (pShaderStr[strlen(pShaderStr)-2] == '(' && pShaderStr[strlen(pShaderStr)-1] == ')')
    {
        // for pShaderStr like "compile vs_1_0 main()"
        GetTokenWhile(SkipToken(SkipToken(pShaderStr, " "), " "), sShaderName, "(");
    }
    else
    {
        strcpy_s(sShaderName, pShaderStr);
    }

    GetTokenWhile(sCurrentFileName, dest, ".");
    sprintf(dest + strlen(dest), "_%s", sShaderName);
    if (dwShaderType == CODE_SVS)
        sprintf(dest + strlen(dest), ".vso");
    else
        sprintf(dest + strlen(dest), ".pso");
}

uint32_t CTechnique::ProcessShaderBin(shader_t *pS, char *pFile, uint32_t dwShaderType)
{
    char *pBuffer = nullptr;
    if (!fio->LoadFile(pFile, &pBuffer, nullptr))
    {
        core.Trace("ERROR: in file %s, file not found : %s", sCurrentFileName, pFile);
        return 0;
    }

    storm_render::Result hr;
    if (dwShaderType == CODE_SVS)
        hr = pRS->ImportVertexProgram((uint32_t *)pBuffer, &pS->pVertexShader);
    else
        hr = pRS->ImportFragmentProgram((uint32_t *)pBuffer, &pS->pPixelShader);

    if (hr != storm_render::result::ok)
        core.Trace("ERROR: can't create shader from %s\nfrom file: %s", pS->pName, pFile);

    STORM_DELETE(pBuffer);
    return 0;
}

uint32_t CTechnique::ProcessVertexShader(char *pFile, uint32_t dwSize, char **pStr)
{
    char temp[1024];

    // add shader
    char *pName = SkipToken(*pStr, VERTEX_SHADER);
    GetTokenWhile(pName, &temp[0], " ");

    auto i = AddShader(temp);
    shader_t *pS = &pShaders[i];
    pS->dwShaderType = CODE_SVS;

    while (nullptr != (*pStr = GetString(pFile, dwSize, *pStr)))
    {
        tolwr(*pStr);
        ClearComment(*pStr);
        if (isEndBracket(*pStr))
            break; // end of vertex shader
        if (isVertexDeclaration(*pStr))
            ProcessVertexDeclaration(pS, pFile, dwSize, pStr);
        if (isAsm(*pStr))
            ProcessShaderAsm(pS, pFile, dwSize, pStr, CODE_SVS);
        TOTAL_SKIP;
    }

    return SHADER_OK;
}

uint32_t CTechnique::ProcessPixelShader(char *pFile, uint32_t dwSize, char **pStr)
{
    char temp[1024];

    // add shader
    char *pName = SkipToken(*pStr, PIXEL_SHADER);
    GetTokenWhile(pName, &temp[0], " ");

    auto i = AddShader(temp);
    shader_t *pS = &pShaders[i];
    pS->dwShaderType = CODE_SPS;

    while (nullptr != (*pStr = GetString(pFile, dwSize, *pStr)))
    {
        tolwr(*pStr);
        ClearComment(*pStr);
        if (isEndBracket(*pStr))
            break; // end of vertex shader
        if (isAsm(*pStr))
            ProcessShaderAsm(pS, pFile, dwSize, pStr, CODE_SPS);
        TOTAL_SKIP;
    }

    return SHADER_OK;
}

uint32_t CTechnique::ProcessBlock(char *pFile, uint32_t dwSize, char **pStr)
{
    uint32_t i;
    char temp[1024];
#ifndef USE_FX // fx files has no parameters
    char pTempParamStr[1024];
#endif

    // resize
    pBlocks = (block_t *)realloc(pBlocks, sizeof(block_t) * (dwNumBlocks + 1));

    block_t *pB = &pBlocks[dwNumBlocks];
    *pB = {};

    dwNumParams = 0;

    // get name this block
#ifdef USE_FX
    char *pName = SkipToken(*pStr, TECHNIQUE);
#else
    char *pName = SkipToken(*pStr, BLOCK);
#endif
    strcpy_s(sCurrentBlockName, pName);
    GetTokenWhile(pName, &temp[0], "(");
    pB->dwHashBlockName = MakeHashValue(temp);
    const auto len = strlen(temp) + 1;
    pB->pBlockName = new char[len];
    memcpy(pB->pBlockName, temp, len);
    for (i = 0; i < dwNumBlocks; i++)
        if (pBlocks[i].dwHashBlockName == pB->dwHashBlockName &&
            (storm::iEquals(pBlocks[i].pBlockName, pB->pBlockName)))
        {
            core.Trace("ERROR: Techniques: Find duplicate technique name: %s", pB->pBlockName);
            break;
        }

    htBlocks[pB->pBlockName] = dwNumBlocks;
    // get parameters
#ifndef USE_FX // fx files has no parameters
    pTempParamStr[0] = 0;
    GetTokenWhile(SkipToken(*pStr, "("), &pTempParamStr[0], ")");
    char *pParamStr = &pTempParamStr[0];
    while (true)
    {
        // get type
        pParamStr = GetTokenWhile(pParamStr, &temp[0], " ");
        if (!pParamStr[0])
            break;
        pParams = (SRSPARAM *)realloc(pParams, sizeof(SRSPARAM) * (dwNumParams + 1));
        pB->pParams = (uint32_t *)realloc(pB->pParams, sizeof(uint32_t) * (dwNumParams + 1));
        pParams[dwNumParams].dwCode = GetCode(&temp[0], PARAM_TYPES, sizeof(PARAM_TYPES) / sizeof(SRSPARAM));
        pB->pParams[dwNumParams] = pParams[dwNumParams].dwCode;
        // get parameter name
        pParamStr = GetTokenWhile(pParamStr, &temp[0], ",");
        const auto len = strlen(temp) + 1;
        pParams[dwNumParams].cName = new char[len];
        strcpy_s(pParams[dwNumParams].cName, len, temp);
        dwNumParams++;
        pB->dwNumParams++;
    }
#endif

#ifdef USE_FX
    // HACK: remove after drop sha files support
    // For all lines except the first one, we need to get the previous line
    // in order to get correct line when entering the loop
    if (pFile != *pStr)
        (*pStr)--;
#endif

    // search for technique and '}'
    while (nullptr != (*pStr = GetString(pFile, dwSize, *pStr)))
    {
#define SKIP1                                                                                                          \
    {                                                                                                                  \
        (*pStr)++;                                                                                                     \
        continue;                                                                                                      \
    }
        tolwr(*pStr);
        ClearComment(*pStr);
        // if (isComment(*pStr)) SKIP1;
        if (isEndBracket(*pStr))
            break; // end of block
        if (isTechnique(*pStr))
        {
            ProcessTechnique(pFile, dwSize, pStr);
#ifdef USE_FX
            // HACK: remove after drop sha files support
            // fx files don't have "block" => it has 1 less level. Entry into ProcessBlock occurred
            // by "technique" and the exit from it should use SKIP1 for keepeng "{"
            (*pStr)--;
            continue;
#endif
        }
        SKIP1;
    }

    // delete parameters
    for (i = 0; i < dwNumParams; i++)
        STORM_DELETE(pParams[i].cName);
    dwNumParams = 0;
    STORM_DELETE(pParams);

    sCurrentBlockName[0] = 0;

    return BLOCK_OK;
}

void CTechnique::ClearSRS_STSS_bUse()
{
    uint32_t i, dwNumParam;

    dwNumParam = sizeof(RenderStates) / sizeof(SRS);
    for (i = 0; i < dwNumParam; i++)
        RenderStates[i].bUse = false;

    dwNumParam = sizeof(TexturesStageStates) / sizeof(STSS);
    for (i = 0; i < dwNumParam; i++)
    {
        TexturesStageStates[i].bUse = false;
        TexturesStageStates[i].dwUseSubCode = 0;
    }

    dwNumParam = sizeof(SampleStates) / sizeof(SAMP);
    for (i = 0; i < dwNumParam; i++)
    {
        SampleStates[i].bUse = false;
        SampleStates[i].dwUseSubCode = 0;
    }
}

void CTechnique::DecodeFiles(char *sub_dir)
{
    uint64_t dwRDTSC;
    RDTSC_B(dwRDTSC);
    pPassStorage = new uint32_t[16384];

    InnerDecodeFiles(sub_dir);

    STORM_DELETE(pPassStorage);
    RDTSC_E(dwRDTSC);
    core.Trace("Techniques: %d shaders compiled.", dwNumShaders);
    core.Trace("Techniques: %d techniques compiled.", dwNumBlocks);
    core.Trace("Techniques: compiled by %d ticks.", dwRDTSC);

    // some optimize
    for (uint32_t i = 0; i < dwNumBlocks; i++)
        STORM_DELETE(pBlocks[i].pBlockName);
}

void CTechnique::InnerDecodeFiles(char *sub_dir)
{
    sprintf(sCurrentDir, "%s%s", SHA_DIR, (sub_dir) ? sub_dir : "");

    const auto vFilenames = fio->_GetPathsOrFilenamesByMask(sCurrentDir, SHA_EXT, true, false, true, true);
    for (std::string path : vFilenames)
    {
        DecodeFile(path);
    }
}

bool CTechnique::DecodeFile(std::string sname)
{
    char fname[256];

    sprintf(fname, "%s", sname.c_str());
    strcpy(sCurrentFileName, fname);
    auto fileS = fio->_CreateFile(fname, std::ios::binary | std::ios::in);
    auto dwSize = fio->_GetFileSize(fname);
    Assert(dwSize != 0);
    char *pFile = new char[dwSize];
    fio->_ReadFile(fileS, pFile, dwSize);
    fio->_CloseFile(fileS);

    // change 0xd and 0xa to 0x0
    for (uint32_t i = 0; i < dwSize; i++)
    {
        if (pFile[i] == 0xD || pFile[i] == 0xA)
            pFile[i] = 0x0;
        if (pFile[i] == 0x9)
            pFile[i] = ' ';
    }

    char *pStr = pFile;

    // dwCurBlock = 0;

    while (nullptr != (pStr = GetString(pFile, dwSize, pStr)))
    {
#define SKIP                                                                                                           \
    {                                                                                                                  \
        pStr++;                                                                                                        \
        continue;                                                                                                      \
    }
        tolwr(pStr);
        ClearComment(pStr);
        // if (isComment(pStr)) SKIP;
        if (isPixelShader(pStr))
        {
            uint32_t dwRes = ProcessPixelShader(pFile, dwSize, &pStr);
            if (dwRes == SHADER_ERROR)
                return false;
        }
        if (isVertexShader(pStr))
        {
            uint32_t dwRes = ProcessVertexShader(pFile, dwSize, &pStr);
            if (dwRes == SHADER_ERROR)
                return false;
        }
#ifdef USE_FX
        if (isTechnique(pStr)) // fx files has no blocks
#else
        if (isBlock(pStr))
#endif
        {
            uint32_t dwRes = ProcessBlock(pFile, dwSize, &pStr);
            if (dwRes == BLOCK_ERROR)
                return false;
            dwNumBlocks++;
        }
        SKIP;
    }

    STORM_DELETE(pFile);
    return true;
}

// return true for drawbuffer, and false for exit
bool CTechnique::ExecutePassStart()
{
    // search block
    // ~!~
    auto it = htBlocks.find(sCurrentBlockName);
    if (it == htBlocks.end())
        return false;
    dwCurBlock = it->second;
    // for (uint32_t i=0;i<dwNumBlocks;i++) if (pBlocks[i].dwHashBlockName == dwHashCode &&
    // (storm::iEquals(pBlocks[i].pBlockName,sCurrentBlockName))) break; if (i==dwNumBlocks) return false;	// don't
    // find concur block Assert(i == dwFind); dwCurBlock = i;

    block_t *pB = &pBlocks[dwCurBlock];
    if (pB->dwNumTechniques == 0)
        return false; // not found any techniques

    // search first validated technique and execute
    // while execute technique 0
    dwCurTechnique = 0;
    dwCurNumPass = pB->pTechniques[dwCurTechnique].dwNumPasses;
    if (dwCurNumPass == 0)
        return false; // not found any passes in techniques

    dwCurPass = 0;

    // clear old saved states
    ClearSavedStates();

    return ExecutePass(true);
}

// return true for drawbuffer, and false for exit
bool CTechnique::ExecutePassNext()
{
    bool bExecute = ExecutePass(false);
    if (bExecute)
        return true; // if once again render in these pass
    dwCurPass++;
    return ExecutePass(true);
}

bool CTechnique::ExecutePassClose()
{
    ClearSavedStates();
    pRS->BindFragmentProgram(nullptr);
    return true;
}

uint32_t CTechnique::GetPassParameter(uint32_t dwParam, uint32_t dwFlags)
{
    if (dwFlags & FLAGS_CODE_IN_PARAM)
    {
        if (dwParam >= dwCurNumParams)
            throw std::runtime_error("Technique: Exceeded parameter numbers!");
        return pCurParams[dwParam];
    }

    return dwParam;
}

bool CTechnique::ExecutePass(bool bStart)
{
    uint32_t dwStage, dwValue, dwSaveValue;
    uint32_t dwCode, dwSubCode, dwRenderSubCode, dwRestoreSubCode;
    bool bSave;

    if (dwCurPass >= dwCurNumPass)
    {
        ExecutePassClose();
        return false;
    }

    pass_t *pCurPass = &pBlocks[dwCurBlock].pTechniques[dwCurTechnique].pPasses[dwCurPass];

    if (!bStart && dwCurPassSize == dwCurPassPos)
        return false;
    if (bStart)
    {
        dwCurPassPos = 0;
        dwCurPassSize = pCurPass->dwSize;
    }
    // begin or continue execute current pass
    uint32_t *pPassStart = &pCurPass->pPass[0];
    uint32_t *pPass = &pCurPass->pPass[dwCurPassPos];

    while (uint32_t(pPass - pPassStart) < dwCurPassSize)
    {
        dwCode = (*pPass++);
        dwSubCode = dwCode & 0xFFFF0000;
        dwCode &= 0xFFFF;
        bSave = (dwSubCode & FLAGS_CODE_RESTORE) ? true : false;

        switch (dwCode)
        {
            // SRS	type:	CODE_SRS,State index,value
            // STSS type:	CODE_STSS,Stage,State index,value
            // RENDER:		CODE_RENDER,SUBCODE_RENDER
            // TRANSFOR:	CODE_TRANSFORM,D3DTRANSFORMSTATETYPE,Value
            // TEXTURE:		CODE_TEXTURE,Stage,Value
            // RESTORE:		CODE_RESTORE
        case CODE_SRS: // SetRenderState(
        {
            storm_render::RenderStateField State = (storm_render::RenderStateField)(*pPass++);
            dwValue = GetPassParameter(*pPass++, dwSubCode);
            if (bSave)
            {
                pRS->GetRenderState(State, &dwSaveValue);
                if (dwSaveValue == dwValue)
                    break;
                AddState2Restore3(dwCode, State, dwSaveValue);
            }
            pRS->SetRenderState(State, dwValue);
        }
        break;
        case CODE_STSS: // SetTextureStageState(
        {
            dwStage = *pPass++;
            storm_render::StageField StageState = (storm_render::StageField)(*pPass++);
            dwValue = GetPassParameter(*pPass++, dwSubCode);
            if (bSave)
            {
                pRS->GetTextureStageState(dwStage, StageState, &dwSaveValue);
                if (dwSaveValue == dwValue)
                    break;
                AddState2Restore3(dwCode, dwStage, StageState);
                AddState2Restore(dwSaveValue);
            }
            pRS->SetTextureStageState(dwStage, StageState, dwValue);
        }
        break;
        case CODE_SAMP: // SetSamplerState(
        {
            dwStage = *pPass++;
            storm_render::SamplerField StageState = (storm_render::SamplerField)(*pPass++);
            dwValue = GetPassParameter(*pPass++, dwSubCode);
            if (bSave)
            {
                pRS->GetSamplerState(dwStage, StageState, &dwSaveValue);
                if (dwSaveValue == dwValue)
                    break;
                AddState2Restore3(dwCode, dwStage, StageState);
                AddState2Restore(dwSaveValue);
            }
            pRS->SetSamplerState(dwStage, StageState, dwValue);
        }
        break;
        case CODE_TEXTURE: // SetTexture(
        {
            uint32_t dwTextureIndex = *pPass++;
            int32_t iTextureIndex = (int32_t)*pPass++;
            if (iTextureIndex >= 0)
                iTextureIndex = (int32_t)GetPassParameter(*pPass++, dwSubCode);
            pRS->TextureSet(dwTextureIndex, iTextureIndex);
        }
        break;
        case CODE_TRANSFORM: // SetTransform(
        {
            storm_render::TransformSlot TransformType = (storm_render::TransformSlot)*pPass++;
            throw std::runtime_error("conversion from uint32_t to D3DMATRIX*");
            // pRS->SetTransform(TransformType,(D3DMATRIX*)GetPassParameter(*pPass++,dwSubCode));
        }
        break;
        case CODE_RENDER: // Render functions
        {
            dwRenderSubCode = *pPass++;
            switch (dwRenderSubCode)
            {
            case SUBCODE_RENDER_DRAW:
                dwCurPassPos = pPass - pPassStart;
                return true;
                break;
            }
        }
        break;
        case CODE_SVS: {
            uint32_t dwShaderIndex = *pPass++;
            if (dwShaderIndex != INVALID_SHADER_INDEX && pShaders[dwShaderIndex].pVertexShader != nullptr)
            {
                if (bSave)
                {
                    pRS->GetVertexProgram((storm_render::Program **)&dwSaveValue);
                    AddState2Restore2(dwCode, dwSaveValue);
                }

                if (pShaders[dwShaderIndex].pVertexDecl != nullptr)
                    pRS->SetVertexDeclaration(pShaders[dwShaderIndex].pVertexDecl);

                pRS->BindVertexProgram(pShaders[dwShaderIndex].pVertexShader);
            }
        }
        break;
        case CODE_SPS: {
            uint32_t dwShaderIndex = *pPass++;
            if (dwShaderIndex != INVALID_SHADER_INDEX && pShaders[dwShaderIndex].pPixelShader != nullptr)
            {
                if (bSave)
                {
                    pRS->GetFragmentProgram((storm_render::Program **)&dwSaveValue);
                    AddState2Restore2(dwCode, dwSaveValue);
                }

                pRS->BindFragmentProgram(pShaders[dwShaderIndex].pPixelShader);
            }
        }
        break;
        case CODE_SPSCONST: {
            uint32_t dwShaderConstIndex = *pPass++;
            throw std::runtime_error("conversion from uint32_t to const float *");
            // pRS->SetPixelShaderConstantF(dwShaderConstIndex, (const float*)GetPassParameter(*pPass++, dwSubCode), 1);
            //~!~
        }
        break;
        case CODE_SVSCONST: {
            uint32_t dwShaderConstIndex = *pPass++;
            uint32_t dwCode = *pPass++;
            switch (dwCode)
            {
            case SUBCODE_SVSCONST_WORLDVIEWPROJ: {
#ifdef _WIN32 // replace D3DXMATRIX
                D3DXMATRIX matTemp, matWorldViewProj;

                D3DXMATRIX matWorld;
                pRS->GetTransform(storm_render::TransformWorld, &matWorld);
                D3DXMATRIX matProj;
                pRS->GetTransform(storm_render::TransformProjection, &matProj);
                D3DXMATRIX matView;
                pRS->GetTransform(storm_render::TransformView, &matView);

                D3DXMatrixMultiply(&matTemp, &matWorld, &matView);
                D3DXMatrixMultiply(&matWorldViewProj, &matTemp, &matProj);

                // Projection to clip space
                D3DXMatrixTranspose(&matWorldViewProj, &matWorldViewProj);
                pRS->SetVertexShaderConstantF(dwShaderConstIndex, &matWorldViewProj(0, 0), 4);
#else
                throw std::runtime_error("replace D3DXMATRIX in technique.cpp");
#endif
            }
            break;
            }
        }
        break;
        case CODE_RESTORE: // restore states
            dwRestoreSubCode = *pPass++;
            switch (dwRestoreSubCode)
            {
            case SUBCODE_RESTORE_CLEAR:
                ClearSavedStates();
                break;
            case SUBCODE_RESTORE_STATES:
                RestoreSavedStates();
                break;
            }
            break;
        }
    }

    dwCurPassPos = pPass - pPassStart;
    return false;
}

void inline CTechnique::AddState2Restore2(uint32_t dwState, uint32_t dw1)
{
    // add new memory if need
    if ((dwCurSavedStatesPos + 2) >= dwCurMaxSavedSize / sizeof(pSavedStates[0]))
    {
        pSavedStates =
            (uint32_t *)realloc(pSavedStates, dwCurMaxSavedSize + SAVED_STATES_ADD * sizeof(pSavedStates[0]));
        dwCurMaxSavedSize += SAVED_STATES_ADD;
    }

    // save state (or part of state)
    pSavedStates[dwCurSavedStatesPos++] = dwState;
    pSavedStates[dwCurSavedStatesPos++] = dw1;
}

void inline CTechnique::AddState2Restore3(uint32_t dwState, uint32_t dw1, uint32_t dw2)
{
    // add new memory if need
    if ((dwCurSavedStatesPos + 3) >= dwCurMaxSavedSize / sizeof(pSavedStates[0]))
    {
        pSavedStates =
            (uint32_t *)realloc(pSavedStates, dwCurMaxSavedSize + SAVED_STATES_ADD * sizeof(pSavedStates[0]));
        dwCurMaxSavedSize += SAVED_STATES_ADD;
    }

    // save state (or part of state)
    pSavedStates[dwCurSavedStatesPos++] = dwState;
    pSavedStates[dwCurSavedStatesPos++] = dw1;
    pSavedStates[dwCurSavedStatesPos++] = dw2;
}

void inline CTechnique::AddState2Restore(uint32_t dwState)
{
    // add new memory if need
    if (dwCurSavedStatesPos >= dwCurMaxSavedSize / sizeof(pSavedStates[0]))
    {
        pSavedStates =
            (uint32_t *)realloc(pSavedStates, dwCurMaxSavedSize + SAVED_STATES_ADD * sizeof(pSavedStates[0]));
        dwCurMaxSavedSize += SAVED_STATES_ADD;
    }

    // save state (or part of state)
    pSavedStates[dwCurSavedStatesPos++] = dwState;
}

void CTechnique::ClearSavedStates()
{
    dwNumSavedStates = 0;
    dwCurSavedStatesPos = 0;
}

void CTechnique::RestoreSavedStates()
{
    uint32_t dwCode, dwValue;
    // restore saved render states and texture stage states

    // for walking
    uint32_t dwTempSavedStatesPos = 0;

    while (dwTempSavedStatesPos < dwCurSavedStatesPos)
    {
        dwCode = pSavedStates[dwTempSavedStatesPos++];
        switch (dwCode)
        {
        case CODE_SRS: {
            storm_render::RenderStateField State = (storm_render::RenderStateField)pSavedStates[dwTempSavedStatesPos++];
            dwValue = pSavedStates[dwTempSavedStatesPos++];
            pRS->SetRenderState(State, dwValue);
        }
        break;
        case CODE_STSS: {
            uint32_t dwStage = pSavedStates[dwTempSavedStatesPos++];
            storm_render::StageField StageState = (storm_render::StageField)pSavedStates[dwTempSavedStatesPos++];
            dwValue = pSavedStates[dwTempSavedStatesPos++];
            pRS->SetTextureStageState(dwStage, StageState, dwValue);
        }
        break;
        case CODE_SAMP: {
            uint32_t dwStage = pSavedStates[dwTempSavedStatesPos++];
            storm_render::SamplerField StageState = (storm_render::SamplerField)pSavedStates[dwTempSavedStatesPos++];
            dwValue = pSavedStates[dwTempSavedStatesPos++];
            pRS->SetSamplerState(dwStage, StageState, dwValue);
        }
        break;
        case CODE_SPS:
            dwValue = pSavedStates[dwTempSavedStatesPos++];
            throw std::runtime_error("conversion from uint32_t to IDirect3DPixelShader9 *");
            // pRS->SetPixelShader((IDirect3DPixelShader9*)dwValue);
            break;
        case CODE_SVS:
            dwValue = pSavedStates[dwTempSavedStatesPos++];
            throw std::runtime_error("conversion from uint32_t to IDirect3DPixelShader9 *");
            // pRS->SetVertexShader((IDirect3DVertexShader9*)dwValue);
            break;
        }
    }

    ClearSavedStates();
}

void CTechnique::SetCurrentBlock(const char *name, uint32_t _dwNumParams, void *pParams)
{
    if (name && name[0])
    {

        strcpy_s(sCurrentBlockName, name);
        tolwr(sCurrentBlockName);
        // dwHashCode = MakeHashValue(sCurrentBlockName);
        dwCurNumParams = _dwNumParams;

        if (dwCurNumParams > dwCurParamsMax)
        {
            while (dwCurNumParams > dwCurParamsMax)
                dwCurParamsMax += 10;
            pCurParams = (uint32_t *)realloc(pCurParams, sizeof(uint32_t) * dwCurParamsMax);
        }

        for (uint32_t i = 0; i < _dwNumParams; i++)
            pCurParams[i] = ((uint32_t *)pParams)[i];
    }
    else
    {
        core.Trace("ERROR: SetCurrentBlock: unknown technique <%s> first character is <%s> ", name, name[0]);
    }
}
#endif // _WIN32
