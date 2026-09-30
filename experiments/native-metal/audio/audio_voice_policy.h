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

inline float CannonMixGain(std::size_t activeVoices)
{
    if (activeVoices <= 1)
        return 1.0f;
    return std::pow(static_cast<float>(activeVoices), -0.4f);
}
} // namespace storm::metal::audio
