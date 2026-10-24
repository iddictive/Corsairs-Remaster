#pragma once

#include <cstddef>
#include <cmath>
#include <limits>
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
constexpr unsigned long long kChargeReplacementFadeMs = 90;
constexpr std::size_t kMaxImpactVoices = 8;
constexpr std::size_t kMaxLocalImpactVoices = 2;
constexpr unsigned long long kImpactClusterIntervalMs = 120;
constexpr float kImpactClusterRadiusSquared = 20.0f * 20.0f;

struct SpatialVoicePolicy
{
    bool linearFalloff;
    float rolloff;
    float volumeGain;
};

constexpr SpatialVoicePolicy SpatialPolicyForAlias(std::string_view lowerAlias,
                                                  float minDistance = -1.0f, float maxDistance = -1.0f)
{
    // Each burning fireplace owns a loop; keep their sum quiet and local.
    if (lowerAlias == "ship_onfire")
        return {true, 1.0f, 0.5f};
    // Exponential attenuation clamps at maxDistance and stays audible forever.
    // Authored finite ranges instead end in silence; unbounded voices retain their curve.
    if (minDistance >= 0.0f && maxDistance > minDistance &&
        maxDistance < std::numeric_limits<float>::max())
        return {true, 1.0f, 1.0f};
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
    // Bound summed amplitude, including coherent copies of the same sample.
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
