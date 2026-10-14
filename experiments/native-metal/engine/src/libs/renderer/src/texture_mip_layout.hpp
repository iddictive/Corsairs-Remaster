#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace storm::renderer
{
constexpr uint32_t kDxt1 = 0x31545844u;
constexpr uint32_t kDxt3 = 0x33545844u;
constexpr uint32_t kDxt5 = 0x35545844u;

inline uint32_t TextureMipSize(uint32_t format, uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        return 0;
    }

    uint64_t bytes = 0;
    switch (format)
    {
    case kDxt1:
        bytes = uint64_t((width + 3) / 4) * ((height + 3) / 4) * 8;
        break;
    case kDxt3:
    case kDxt5:
        bytes = uint64_t((width + 3) / 4) * ((height + 3) / 4) * 16;
        break;
    case 21:
    case 22:
        bytes = uint64_t(width) * height * 4;
        break;
    case 23:
    case 25:
    case 26:
        bytes = uint64_t(width) * height * 2;
        break;
    default:
        return 0;
    }

    return bytes <= std::numeric_limits<uint32_t>::max() ? static_cast<uint32_t>(bytes) : 0;
}

inline uint32_t TextureMipCount(uint32_t width, uint32_t height)
{
    uint32_t count = 1;
    for (uint32_t dimension = std::max(width, height); dimension > 1; dimension /= 2)
    {
        ++count;
    }
    return count;
}

inline bool TextureMipHeaderIsValid(uint32_t format, uint32_t width, uint32_t height, uint32_t mips,
                                    uint32_t levelZeroBytes)
{
    return mips > 0 && mips <= TextureMipCount(width, height) && levelZeroBytes == TextureMipSize(format, width, height);
}

inline bool TextureMipReadIsValid(uint32_t surfaceBytes, uint32_t inputBytes)
{
    return surfaceBytes != 0 && inputBytes == surfaceBytes;
}

inline bool TextureMipPayloadAvailable(uint64_t availableBytes, uint32_t requestedBytes)
{
    return requestedBytes <= availableBytes;
}
} // namespace storm::renderer
