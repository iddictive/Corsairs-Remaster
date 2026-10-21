#pragma once

#include <array>
#include <string_view>

namespace storm::world_damage
{
inline constexpr bool Enabled = true;
inline constexpr std::array<std::string_view, 2> SavedStateAttributes{
    "PhonkWorldCuts", "PhonkWorldMaterial"};

inline bool isSavedStateAttribute(std::string_view name)
{
    const auto lower = [](char c) { return c >= 'A' && c <= 'Z' ? char(c + ('a' - 'A')) : c; };
    for (const auto reserved : SavedStateAttributes)
    {
        if (name.size() != reserved.size()) continue;
        bool equal = true;
        for (size_t n = 0; n < name.size(); ++n)
            if (lower(name[n]) != lower(reserved[n])) { equal = false; break; }
        if (equal) return true;
    }
    return false;
}
}
