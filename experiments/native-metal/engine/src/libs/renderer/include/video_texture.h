#pragma once
#include "dx9render.h"

//-----------------------------------------------------------------------------
// Name: class VideoToTexture
// Desc: play video into texture
//-----------------------------------------------------------------------------
class CVideoTexture : public Entity
{
  public:
    virtual storm_render::Texture *Initialize(VDX9RENDER *pRS, const char *sFileName, bool bCicled) = 0;
    virtual bool FrameUpdate() = 0;
    virtual void Release() = 0;
    storm_render::Texture *m_pTexture;

    operator storm_render::Texture *() const
    {
        return m_pTexture;
    }

    operator storm_render::BaseTexture *() const
    {
        return m_pTexture;
    }
};
