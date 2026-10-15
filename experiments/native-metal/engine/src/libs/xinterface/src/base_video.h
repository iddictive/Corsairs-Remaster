#pragma once

#include "dx9render.h"
#include "vma.hpp"

class xiBaseVideo : public Entity
{
  public:
    virtual void SetShowVideo(bool bShowVideo)
    {
    }

    virtual storm_render::Texture *GetCurrentVideoTexture()
    {
        return nullptr;
    }
};
