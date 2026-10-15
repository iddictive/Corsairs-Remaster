#include "texture_payload.hpp"
#include "render_device_api.hpp"
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_TGA
#include <stb_image.h>
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <vector>

namespace storm_render {
namespace {

bool loadDdsLevels(std::ifstream &file, Texture *texture, const storm::raw_texture::DdsLayout &layout)
{
    std::vector<uint8_t> payload;
    for (size_t level = 0; level < layout.levels.size(); ++level)
    {
        const auto &source = layout.levels[level];
        payload.resize(static_cast<size_t>(source.size));
        file.seekg(static_cast<std::streamoff>(source.offset));
        file.read(reinterpret_cast<char *>(payload.data()), static_cast<std::streamsize>(payload.size()));
        if (!file || static_cast<size_t>(file.gcount()) != payload.size())
        {
            return false;
        }

        LockedRect locked{};
        if (failed(texture->LockRect(static_cast<std::uint32_t>(level), &locked, nullptr, 0)))
        {
            return false;
        }
        const auto destinationPitch = static_cast<uint32_t>(locked.Pitch);
        const auto rowBytes = std::min(source.pitch, destinationPitch);
        for (uint32_t row = 0; row < source.rows; ++row)
        {
            std::memcpy(static_cast<uint8_t *>(locked.pBits) + size_t(row) * destinationPitch,
                        payload.data() + size_t(row) * source.pitch, rowBytes);
        }
        if (failed(texture->UnlockRect(static_cast<std::uint32_t>(level))))
        {
            return false;
        }
    }
    return !layout.levels.empty();
}

// A DDS container is copied level by level with the authored BC format, exactly
// like the compiled .tx path in DX9RENDER::TextureLoad, so the backend keeps its
// existing compressed-format handling.
Result loadDdsTexture(Device *device, std::ifstream &file, uint64_t fileSize, Texture **out)
{
    uint8_t header[128];
    file.read(reinterpret_cast<char *>(header), sizeof(header));
    if (!file || static_cast<size_t>(file.gcount()) != sizeof(header))
    {
        return result::invalidCall;
    }

    storm::raw_texture::DdsLayout layout;
    if (!storm::raw_texture::ParseDds(header, sizeof(header), fileSize, layout))
    {
        return result::invalidCall;
    }

    Texture *texture = nullptr;
    auto result = device->CreateTexture(layout.width, layout.height, static_cast<std::uint32_t>(layout.levels.size()), 0,
                                        layout.format, &texture);
    if (succeeded(result))
    {
        if (!texture || !loadDdsLevels(file, texture, layout))
        {
            result = result::invalidCall;
        }
    }
    if (failed(result))
    {
        if (texture)
        {
            texture->Release();
        }
        return result;
    }
    *out = texture;
    return result::ok;
}

} // namespace

Result ImageLoadTextureFromFile(Device *device, const char *path, Texture **out)
{
    *out = nullptr;

    if (device && path)
    {
        std::error_code error;
        const auto fileSize = std::filesystem::file_size(path, error);
        if (!error && fileSize >= 128)
        {
            std::ifstream file(path, std::ios::binary);
            if (file)
            {
                uint8_t magic[4]{};
                file.read(reinterpret_cast<char *>(magic), sizeof(magic));
                file.clear();
                file.seekg(0);
                if (storm::raw_texture::LooksLikeDds(magic, sizeof(magic)))
                {
                    return loadDdsTexture(device, file, fileSize, out);
                }
            }
        }
    }

    int width = 0, height = 0, channels = 0;
    auto *pixels = stbi_load(path, &width, &height, &channels, 4);
    if (!pixels)
        return result::invalidCall;
    Texture *texture = nullptr;
    auto result = device->CreateTexture(width, height, 1, 0, AssetFormat::A8R8G8B8,
                                       &texture);
    if (succeeded(result))
    {
        LockedRect locked{};
        result = texture->LockRect(0, &locked, nullptr, 0);
        if (succeeded(result))
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
    if (failed(result))
    {
        if (texture) texture->Release();
        return result;
    }
    *out = texture;
    return result::ok;
}

} // namespace storm_render
