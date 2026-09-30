#include "raw_texture.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_TGA
#include <stb_image.h>
#include <algorithm>
#include <cstring>

HRESULT LoadNativeRawTexture(IDirect3DDevice9 *device, const char *path, IDirect3DTexture9 **out)
{
    *out = nullptr;
    int width = 0, height = 0, channels = 0;
    auto *pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels)
        return D3DERR_INVALIDCALL;
    IDirect3DTexture9 *texture = nullptr;
    auto result = device->CreateTexture(width, height, 1, 0, D3DFMT_A8R8G8B8,
                                       D3DPOOL_MANAGED, &texture, nullptr);
    if (SUCCEEDED(result))
    {
        D3DLOCKED_RECT locked{};
        result = texture->LockRect(0, &locked, nullptr, 0);
        if (SUCCEEDED(result))
        {
            for (int y = 0; y < height; ++y)
            {
                auto *src = pixels + size_t(y) * width * 4;
                for (int x = 0; x < width; ++x)
                    std::swap(src[x * 4], src[x * 4 + 2]);
                std::memcpy(static_cast<unsigned char *>(locked.pBits) + size_t(y) * locked.Pitch,
                            src, size_t(width) * 4);
            }
            result = texture->UnlockRect(0);
        }
    }
    stbi_image_free(pixels);
    if (FAILED(result))
    {
        if (texture) texture->Release();
        return result;
    }
    *out = texture;
    return D3D_OK;
}
