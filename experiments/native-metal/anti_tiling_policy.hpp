#pragma once

#include <cctype>
#include <string_view>

// Exact texture-name admission for the fixed-function anti-tiling sampler.
// These are opaque, repeatable diffuse materials only.  A missing name means
// preserve the authored sampling path exactly.
namespace storm_metal::anti_tiling {

enum class Kind : unsigned char { none, naturalDiffuse };

inline bool asciiEqualIgnoreCase(std::string_view left, std::string_view right) {
    if (left.size() != right.size()) return false;
    for (size_t index = 0; index < left.size(); ++index) {
        const auto a = static_cast<unsigned char>(left[index]);
        const auto b = static_cast<unsigned char>(right[index]);
        if (std::tolower(a) != std::tolower(b)) return false;
    }
    return true;
}

inline std::string_view basename(std::string_view label) {
    const auto slash = label.find_last_of("/\\");
    return slash == std::string_view::npos ? label : label.substr(slash + 1);
}

inline bool matchesAuthoredTexture(std::string_view label, std::string_view authoredName) {
    return asciiEqualIgnoreCase(label, authoredName) ||
           (label.size() == authoredName.size() + 3 &&
            asciiEqualIgnoreCase(label.substr(0, authoredName.size()), authoredName) &&
            asciiEqualIgnoreCase(label.substr(authoredName.size()), ".tx"));
}

inline Kind kindForLabel(std::string_view label) {
    const auto name = basename(label);
    // floorU3 is irregular, weathered field stone; Sandtile and the three
    // rockK2 aliases are non-directional terrain materials.  The helper only
    // changes repeated sampling of these existing diffuse images.
    for (const auto eligible : {"floorU3.tga", "Sandtile.tga", "rockA2.tga",
                                "rockB2.tga", "rockC2.tga"}) {
        if (matchesAuthoredTexture(name, eligible)) return Kind::naturalDiffuse;
    }
    // Deliberately absent: cobbleM1/town_cob (fan-pattern paving), tileU2
    // (high-contrast grout), board/wood, alpha masks, foliage and transitions.
    return Kind::none;
}

} // namespace storm_metal::anti_tiling
