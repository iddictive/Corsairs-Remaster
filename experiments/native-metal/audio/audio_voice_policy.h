#pragma once

#include <cstddef>
#include <cstdint>
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
    // Water fragments may use either alias; missile impacts also layer bomb2bort.
    // The authored maximum is a silence boundary for these positional effects.
    if (lowerAlias == "debris_splash" || lowerAlias == "ball_splash" ||
        lowerAlias == "bomb2bort" || lowerAlias.starts_with("phonk_missile_"))
        return {true, 1.0f, 1.0f};
    // Ordinary effects decay smoothly instead of cutting off at the authored range.
    return {false, 0.45f, 1.0f};
}

// Stage buses share a budget across missile variants and ships. Keep arrivals
// independent from launches, so a broadside cannot consume the impact budget.
constexpr std::string_view ClusteredVoiceKey(std::string_view alias)
{
    if (alias == "debris_splash") return "ball_splash";
    if (alias == "phonk_missile_heavy" || alias.starts_with("phonk_missile_launch_") ||
        alias == "phonk_missile_interceptor" || alias == "phonk_missile_interceptor_patriot")
        return "phonk_missile_launch";
    return alias;
}

constexpr std::size_t MaxClusteredVoices(std::string_view key)
{
    if (key == "phonk_missile_launch") return 6;
    if (key == "phonk_missile_cruise" || key == "phonk_missile_sonic") return 4;
    if (key == "phonk_missile_flyby") return 3;
    return kMaxImpactVoicesPerAlias;
}

// Audio-only sequence: never consume the gameplay RNG. Attenuation only,
// so variation cannot exceed the authored level or defeat the mix limiter.
constexpr float MissileEventGain(std::uint32_t sequence)
{
    sequence ^= sequence >> 16;
    sequence *= 0x7feb352du;
    sequence ^= sequence >> 15;
    sequence *= 0x846ca68bu;
    sequence ^= sequence >> 16;
    return 0.90f + static_cast<float>(sequence & 0xffffu) * (0.10f / 65535.0f);
}

constexpr bool IsClusteredImpactAlias(std::string_view lowerAlias)
{
    return lowerAlias.starts_with("phonk_missile_") || lowerAlias == "ball_splash" || lowerAlias == "debris_splash" || lowerAlias == "fly_ball" || lowerAlias == "fly_ball_misc" ||
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

