#pragma once

#include <cstddef>
#include <cmath>
#include <string_view>

namespace storm::metal::audio
{
enum class VoiceClass
{
    ordinary,
    cannon,
    chargeSelection,
};

constexpr std::size_t kMaxCannonVoices = 12;
constexpr std::size_t kMaxRetiringCannonVoices = 2;
constexpr unsigned long long kCannonReplacementFadeMs = 30;
constexpr unsigned long long kChargeReplacementFadeMs = 90;
constexpr std::size_t kMaxImpactVoicesPerAlias = 8;
constexpr std::size_t kMaxLocalImpactVoices = 2;
constexpr unsigned long long kSpatialRepeatIntervalMs = 50;
constexpr float kSpatialRepeatRadiusSquared = 20.0f * 20.0f;

struct SpatialVoicePolicy
{
    bool linearFalloff;
    float rolloff;
    float volumeGain;
};

constexpr SpatialVoicePolicy SpatialPolicyForAlias(std::string_view lowerAlias)
{
    // Each burning fireplace owns a loop; keep their sum quiet and local.
    if (lowerAlias == "ship_onfire")
        return {true, 1.0f, 0.5f};
    // Hull collisions stay local; distant ships must not carry collision noise.
    if (lowerAlias == "coll_ship2rock" || lowerAlias == "coll_ship2ship")
        return {true, 1.0f, 1.0f};
    // Ordinary effects decay smoothly instead of cutting off at the authored range.
    return {false, 0.45f, 1.0f};
}

constexpr bool IsClusteredImpactAlias(std::string_view lowerAlias)
{
    return lowerAlias == "ball_splash" || lowerAlias == "fly_ball" || lowerAlias == "fly_ball_misc" ||
           lowerAlias == "ball2bort" || lowerAlias == "bomb2bort" || lowerAlias == "grapes2bort" ||
           lowerAlias == "ball2sail" || lowerAlias == "knippel2sail" || lowerAlias == "grapes2sail" ||
           lowerAlias == "mast_fall" || lowerAlias == "coll_ship2rock" || lowerAlias == "coll_ship2ship" ||
           lowerAlias == "fort_cann_explode" || lowerAlias == "cannon_explosion" || lowerAlias == "ship_explosion" ||
           lowerAlias == "episode_boom";
}

constexpr float ImpactMixGain(std::size_t activeVoices)
{
    // Bound only nearby copies of the same effect, including coherent samples.
    return activeVoices > 1 ? 1.0f / static_cast<float>(activeVoices) : 1.0f;
}

constexpr VoiceClass ClassifyAlias(std::string_view lowerAlias)
{
    if (lowerAlias == "cannon_fire" || lowerAlias == "fort_cannon_fire" ||
        lowerAlias.starts_with("cannon_fire_"))
        return VoiceClass::cannon;
    if (lowerAlias == "chargeballs" || lowerAlias == "chargegrapes" ||
        lowerAlias == "chargeknippels" || lowerAlias == "chargebombs")
        return VoiceClass::chargeSelection;
    return VoiceClass::ordinary;
}

constexpr bool AdmitCannonVoice(std::size_t activeVoices)
{
    return activeVoices < kMaxCannonVoices;
}

inline float CannonMixGain(std::size_t activeVoices)
{
    if (activeVoices <= 1)
        return 1.0f;
    return std::pow(static_cast<float>(activeVoices), -0.4f);
}
} // namespace storm::metal::audio
