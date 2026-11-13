#pragma once
#include <cstdint>
namespace storm::vfx {
inline constexpr std::uint32_t explosionLightMessage = 0x56465831u;
inline constexpr std::uint64_t shellLightDomain = std::uint64_t{1} << 58;
inline constexpr std::uint64_t nuclearLightDomain = std::uint64_t{1} << 59;
inline constexpr bool isExplosionLight(std::uint64_t id)
{
    return (id >> 58) == 1 || (id >> 59) == 1;
}
}
