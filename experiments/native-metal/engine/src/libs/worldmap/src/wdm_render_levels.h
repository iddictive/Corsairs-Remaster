#pragma once

#include <cstdint>

namespace wdm::render_levels
{

inline constexpr int32_t Sea = -2;
inline constexpr int32_t PortDanger = -1;
inline constexpr int32_t Island = 0;
inline constexpr int32_t Ship = 100;
inline constexpr int32_t Label = 500;

static_assert(Sea < PortDanger);
static_assert(PortDanger < Island);
static_assert(Island < Ship);
static_assert(Ship < Label);

} // namespace wdm::render_levels
