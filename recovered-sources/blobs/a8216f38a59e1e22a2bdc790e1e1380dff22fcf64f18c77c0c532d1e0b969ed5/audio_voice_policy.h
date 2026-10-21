#pragma once

#include <algorithm>
#include <cstddef>
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
constexpr float kCannonBusCeiling = 0.9f;
constexpr unsigned long long kChargeReplacementFadeMs = 90;

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

inline float CannonMixGain(float summedEffectiveVolume)
{
    if (summedEffectiveVolume <= kCannonBusCeiling || summedEffectiveVolume <= 0.0f)
        return 1.0f;
    return std::clamp(kCannonBusCeiling / summedEffectiveVolume, 0.0f, 1.0f);
}
} // namespace storm::metal::audio
