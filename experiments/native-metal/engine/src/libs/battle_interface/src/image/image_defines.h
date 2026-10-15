#pragma once

#include "dx9render.h"

#define BI_IMAGE_VERTEX_FORMAT (storm_render::vertexFormat::xyzwTransformed | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1 | storm_render::vertexFormat::texCoord2)

struct BI_IMAGE_VERTEX
{
    CVECTOR pos;
    float w;
    uint32_t col;
    float tu, tv;
};

enum BIImageType
{
    BIType_square,
    BIType_clocksquare,

    BIType_ForceDword = 0xFFFFFFFF
};

class IBIImage
{
  protected:
    IBIImage()
    {
    }

  public:
    virtual ~IBIImage()
    {
    }

    virtual void SetColor(uint32_t color) = 0;
    virtual void SetPosition(int32_t nLeft, int32_t nTop, int32_t nRight, int32_t nBottom) = 0;
    virtual void Set3DPosition(const CVECTOR &vPos, float fWidth, float fHeight) = 0;
    virtual void SetUV(const FRECT &uv) = 0;

    virtual void CutSide(float fleft, float fright, float ftop, float fbottom) = 0;
    virtual void CutClock(float fBegin, float fEnd, float fFactor) = 0;
};

class IBIString
{
  protected:
    IBIString()
    {
    }

  public:
    virtual ~IBIString()
    {
    }

    virtual void SetColor(uint32_t color) = 0;
    virtual void SetScale(float fScale) = 0;
    virtual void SetFont(const char *pcFontName) = 0;
    virtual void SetAlign(int32_t nHorzAlign, int32_t nVertAlign) = 0;
    virtual void SetPosition(int32_t nLeft, int32_t nTop, int32_t nRight, int32_t nBottom) = 0;
    virtual void SetString(const char *pcStr) = 0;
};

#define ImagePrioritet_MaxValue 100000
#define ImagePrioritet_DefaultValue 10000
