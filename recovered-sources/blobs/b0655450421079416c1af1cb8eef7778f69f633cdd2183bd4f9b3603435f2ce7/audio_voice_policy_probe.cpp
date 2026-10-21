#include "audio_voice_policy.h"

#include <cmath>
#include <cstdio>
#include <cstdlib>

using storm::metal::audio::AdmitCannonVoice;
using storm::metal::audio::CannonMixGain;
using storm::metal::audio::ClassifyAlias;
using storm::metal::audio::VoiceClass;

static void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
        std::exit(1);
    }
}

int main()
{
    require(ClassifyAlias("cannon_fire_cannon_48") == VoiceClass::cannon,
            "calibre aliases share the cannon bus");
    require(ClassifyAlias("fort_cannon_fire") == VoiceClass::cannon,
            "fort fire shares the cannon bus");
    require(ClassifyAlias("chargebombs") == VoiceClass::chargeSelection,
            "charge announcements use replacement semantics");
    require(ClassifyAlias("ball_splash") == VoiceClass::ordinary,
            "impacts remain independent ordinary effects");
    require(AdmitCannonVoice(0) && AdmitCannonVoice(11) && !AdmitCannonVoice(12),
            "cannon concurrency has a hard bound");
    require(std::fabs(CannonMixGain(0.5f) - 1.0f) < 0.0001f,
            "one quiet cannon retains its authored level");
    require(std::fabs(CannonMixGain(3.6f) * 3.6f - 0.9f) < 0.0001f,
            "a broadside stays within the cannon bus ceiling");
    std::puts("PASS: cannon classification, bounded polyphony, peak-safe gain and charge replacement class");
}
