#pragma once

#include <algorithm>
#include <cmath>

struct SeaTsunamiParameters
{
    static constexpr float minAmplitude = 28.f, maxAmplitude = 84.f;
    static constexpr float minWidth = 220.f, maxWidth = 285.f;
    static constexpr float minSpeed = 22.f, maxSpeed = 28.f;
    static constexpr float minDamage = .10f, maxDamage = .50f;
    static constexpr float minContinuationDistance = 2500.f, maxContinuationDistance = 5000.f;
    static constexpr float maxPitch = .20f, maxRoll = 2.05f, responseFrequency = 2.6f;
    // Broadside means 90 degrees +/- 40 degrees to propagation, on either side.
    static constexpr float capsizeBeamThreshold = .76604444f, overturnAngle = 1.57079633f;
    static constexpr float minDriftDistance = 60.f, maxDriftDistance = 300.f;
    static constexpr float profileArea = 4.f / 3.f;
};

// One transient wave state for rendered sea geometry and scalar physics queries.
// A compact C1 crest supplies height, material and matching slopes to all consumers.
struct SeaTsunamiFrame
{
    bool active = false;
    unsigned serial = 0;
    float originX = 0.f, originZ = 0.f;
    float directionX = 0.f, directionZ = 1.f;
    float age = 0.f, launchDistance = 0.f, frontDistance = 0.f;
    float duration = 0.f, envelope = 0.f;
    float amplitude = 28.f, speed = 22.f, width = 220.f;
    float phase = 0.f, phase2 = 0.f;
    float severity = 0.f, damageFraction = SeaTsunamiParameters::minDamage;
};

struct SeaTsunamiSample
{
    float height = 0.f, slopeX = 0.f, slopeZ = 0.f;
    float signedDistance = 0.f, crestStrength = 0.f, foam = 0.f;
    unsigned serial = 0;
    float amplitude = SeaTsunamiParameters::minAmplitude, severity = 0.f, damageFraction = 0.f;
    float speed = SeaTsunamiParameters::minSpeed, width = SeaTsunamiParameters::minWidth;
    float directionX = 0.f, directionZ = 1.f;
};

inline float SeaTsunamiSmooth(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

struct SeaTsunamiPolynomial
{
    float constant = 0.f, linear = 0.f, quadratic = 0.f;
};
inline SeaTsunamiPolynomial SeaTsunamiProfilePolynomial(float u)
{
    if (std::abs(u) >= 1.5f) return {};
    if (std::abs(u) <= 0.5f) return {1.f, 0.f, -4.f / 3.f};
    return {1.5f, u < 0.f ? 2.f : -2.f, 2.f / 3.f};
}

inline SeaTsunamiSample EvaluateSeaTsunami(const SeaTsunamiFrame &wave, float x, float z)
{
    if (!wave.active || wave.envelope <= 0.f) return {};
    const float dx = x - wave.originX, dz = z - wave.originZ;
    const float along = dx * wave.directionX + dz * wave.directionZ - wave.frontDistance;
    const float across = -dx * wave.directionZ + dz * wave.directionX;
    // Long irregular lobes, fixed for this event. The launch centre keeps its
    // original crossing time and peak; the broad support bound includes meander.
    const float p = across / 480.f + wave.phase, q = across / 173.f + wave.phase2;
    const float offset = 42.f * (std::sin(p) - std::sin(wave.phase)) +
                         22.f * (std::sin(q) - std::sin(wave.phase2));
    const float dOffset = (42.f / 480.f) * std::cos(p) + (22.f / 173.f) * std::cos(q);
    const float centre = std::exp(-across * across / (2.f * 560.f * 560.f));
    const float lobe = .78f + .14f * std::sin(across / 650.f + wave.phase2) +
                            .08f * std::sin(across / 241.f + wave.phase);
    const float dLobe = (.14f / 650.f) * std::cos(across / 650.f + wave.phase2) +
                        (.08f / 241.f) * std::cos(across / 241.f + wave.phase);
    const float amplitude = centre + (1.f - centre) * lobe;
    const float dAmplitude = -across * centre / (560.f * 560.f) * (1.f - lobe) +
                             (1.f - centre) * dLobe;
    const float signedDistance = along - offset;
    const float u = signedDistance / wave.width;
    const float a = std::abs(u);
    const float strength = wave.amplitude * wave.envelope;
    SeaTsunamiSample sample;
    sample.serial = wave.serial;
    sample.amplitude = wave.amplitude;
    sample.severity = wave.severity;
    sample.damageFraction = wave.damageFraction;
    sample.speed = wave.speed;
    sample.width = wave.width;
    sample.directionX = wave.directionX;
    sample.directionZ = wave.directionZ;
    sample.signedDistance = signedDistance;
    sample.crestStrength = std::clamp(wave.envelope * amplitude, 0.f, 1.f);
    if (a >= 1.5f) return sample;
    // An asymmetric crest with unchanged support and C1 endpoints.
    const float taper = 1.f - a / 1.5f;
    const float warped = u + .20f * u * u * taper * taper;
    const float dWarp = 1.f + .40f * u * taper * taper -
                        (.40f / 1.5f) * u * u * taper * (u < 0.f ? -1.f : 1.f);
    const auto polynomial = SeaTsunamiProfilePolynomial(warped);
    // Factor the edge pieces to preserve nonnegative height near the support end.
    const float edge = 1.5f - std::abs(warped);
    const float profile = std::abs(warped) <= 0.5f ? 1.f - (4.f / 3.f) * warped * warped :
                          (2.f / 3.f) * edge * edge;
    const float dProfile = (2.f * polynomial.quadratic * warped + polynomial.linear) * dWarp;
    const float dAlong = strength * amplitude * dProfile / wave.width;
    const float dAcross = strength * dAmplitude * profile - dAlong * dOffset;
    sample.height = strength * amplitude * profile;
    sample.slopeX = dAlong * wave.directionX - dAcross * wave.directionZ;
    sample.slopeZ = dAlong * wave.directionZ + dAcross * wave.directionX;
    sample.foam = sample.crestStrength * SeaTsunamiSmooth((profile - .62f) / .34f) *
                  SeaTsunamiSmooth((.45f - u) / .45f);
    return sample;
}

// Native per-ship contact state: no attributes, saved flags or render clock.
struct SeaTsunamiContact
{
    float x = 0.f, z = 0.f, age = 0.f, peak = 0.f, paid = 0.f;
    unsigned serial = 0;
    bool initialized = false;

    float Query(const SeaTsunamiFrame &wave, float nextX, float nextZ)
    {
        if (!wave.active) { *this = {}; return 0.f; }
        if (serial != wave.serial) { *this = {}; serial = wave.serial; }
        const auto sample = EvaluateSeaTsunami(wave, nextX, nextZ);
        peak = std::max(peak, std::clamp(sample.height / wave.amplitude, 0.f, 1.f));
        float exposure = 0.f;
        if (initialized && wave.age <= age) return 0.f;
        if (initialized)
        {
            // Integrate actual profile contact between the ordinary one-second
            // ship updates. Neither crest crossing nor exit pays a lump sum.
            const float seconds = wave.age - age;
            const int steps = std::clamp(static_cast<int>(std::ceil(seconds / .25f)), 1, 64);
            for (int i = 0; i < steps; ++i)
            {
                const float t = (static_cast<float>(i) + .5f) / static_cast<float>(steps);
                auto crossing = wave;
                crossing.age = age + t * seconds;
                crossing.frontDistance = crossing.launchDistance - crossing.speed * crossing.age;
                crossing.envelope = SeaTsunamiSmooth(crossing.age / 5.f) *
                                    SeaTsunamiSmooth((crossing.duration - crossing.age) / 8.f);
                const float load = std::clamp(EvaluateSeaTsunami(crossing,
                    x + t * (nextX - x), z + t * (nextZ - z)).height / wave.amplitude, 0.f, 1.f);
                peak = std::max(peak, load);
                exposure += load * seconds * wave.speed /
                            (static_cast<float>(steps) * SeaTsunamiParameters::profileArea * wave.width);
            }
        }
        // First discovery starts the clock; no retroactive entry payment.
        initialized = true;
        x = nextX; z = nextZ; age = wave.age;
        exposure = std::clamp(exposure, 0.f, std::max(0.f, peak - paid));
        paid += exposure;
        return exposure;
    }
};

struct SeaTsunamiMotion
{
    float pitch = 0.f, roll = 0.f, pitchSpeed = 0.f, rollSpeed = 0.f;
    unsigned serial = 0;
    void Reset() { *this = {}; }

    void Update(const SeaTsunamiSample &sample, float head, float beam, float protection, float seconds)
    {
        if (sample.serial == 0 || protection <= 0.f) { Reset(); return; }
        if (serial != sample.serial) { Reset(); serial = sample.serial; }
        if (!(seconds > 0.f) || !std::isfinite(seconds)) return;
        // Ordinary buoyancy follows the surface; event torque follows its signed
        // flank, never its height on a flat crest. Bow impact stays below 12 degrees.
        const float alongSlope = sample.slopeX * sample.directionX +
                                 sample.slopeZ * sample.directionZ;
        const float flank = std::clamp(alongSlope * sample.width /
            (sample.amplitude * SeaTsunamiParameters::profileArea), -1.f, 1.f);
        const float load = flank * sample.severity * sample.severity * std::clamp(protection, 0.f, 1.f);
        const float targetPitch = SeaTsunamiParameters::maxPitch * load * head;
        const float targetRoll = SeaTsunamiParameters::maxRoll * load * beam;
        const float delta = std::min(seconds, .1f);
        const int steps = std::max(1, static_cast<int>(std::ceil(delta / .025f)));
        const float step = delta / static_cast<float>(steps);
        const float omega = SeaTsunamiParameters::responseFrequency;
        for (int i = 0; i < steps; ++i)
        {
            pitchSpeed += (omega * omega * (targetPitch - pitch) - 2.f * omega * pitchSpeed) * step;
            rollSpeed += (omega * omega * (targetRoll - roll) - 2.f * omega * rollSpeed) * step;
            pitch += pitchSpeed * step;
            roll += rollSpeed * step;
        }
    }
};

inline bool SeaTsunamiCanCapsize(const SeaTsunamiSample &sample, float signedBeam, float modelUpY)
{
    return sample.serial != 0 && sample.height > 0.f &&
           std::abs(signedBeam) >= SeaTsunamiParameters::capsizeBeamThreshold &&
           SeaTsunamiParameters::maxRoll * sample.severity * sample.severity >=
               SeaTsunamiParameters::overturnAngle && modelUpY <= 0.f;
}

// Current velocity is temporary in Move; only its finite travel budget lives here.
// Prediction reads the same budget without mutating it, including event changes.
struct SeaTsunamiDrift
{
    unsigned serial = 0;
    float spent = 0.f;
    void Reset() { *this = {}; }

    float Speed(const SeaTsunamiSample &sample, float protection, float seconds, bool consume)
    {
        if (sample.serial == 0 || protection <= 0.f)
        {
            if (consume) Reset();
            return 0.f;
        }
        if (consume && serial != sample.serial) { Reset(); serial = sample.serial; }
        if (!(seconds > 0.f) || !std::isfinite(seconds)) return 0.f;
        const float distance = (SeaTsunamiParameters::minDriftDistance + sample.severity *
            (SeaTsunamiParameters::maxDriftDistance - SeaTsunamiParameters::minDriftDistance)) *
            std::clamp(protection, 0.f, 1.f);
        const float remaining = std::max(0.f, distance - (serial == sample.serial ? spent : 0.f));
        const float load = std::clamp(sample.height / sample.amplitude, 0.f, 1.f);
        const float ratio = distance * load / (SeaTsunamiParameters::profileArea * sample.width);
        // Account for a ship moving with the crest: v/(c-v) follows the profile.
        const float speed = std::min(sample.speed * ratio / (1.f + ratio), remaining / seconds);
        if (consume) spent += speed * seconds;
        return speed;
    }
};

class SeaTsunami
{
    SeaTsunamiFrame frame_;
    unsigned serial_ = 0;

  public:
    const SeaTsunamiFrame &Frame() const { return frame_; }
    float HeightBound() const { return frame_.active ? frame_.amplitude : 0.f; }
    float SupportBound() const { return frame_.active ? 1.5f * frame_.width + 128.f : 0.f; }
    void Cancel() { frame_ = {}; }

    void Start(float x, float z, float directionX, float directionZ, float maxSeaDistance, float severity = 0.f)
    {
        Cancel();
        const float length = std::hypot(directionX, directionZ);
        if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(length) ||
            length < 0.001f || !std::isfinite(maxSeaDistance) || maxSeaDistance < 900.f ||
            !std::isfinite(severity)) return;
        frame_.active = true;
        frame_.serial = ++serial_;
        frame_.phase = std::fmod(static_cast<float>(serial_) * 2.39996323f, 6.28318531f);
        frame_.phase2 = std::fmod(static_cast<float>(serial_) * 1.61803399f, 6.28318531f);
        frame_.severity = std::clamp(severity, 0.f, 1.f);
        const auto interpolate = [severity = frame_.severity](float low, float high) {
            return low + (high - low) * severity;
        };
        frame_.amplitude = interpolate(SeaTsunamiParameters::minAmplitude, SeaTsunamiParameters::maxAmplitude);
        frame_.width = interpolate(SeaTsunamiParameters::minWidth, SeaTsunamiParameters::maxWidth);
        frame_.speed = interpolate(SeaTsunamiParameters::minSpeed, SeaTsunamiParameters::maxSpeed);
        frame_.damageFraction = interpolate(SeaTsunamiParameters::minDamage, SeaTsunamiParameters::maxDamage);
        frame_.originX = x;
        frame_.originZ = z;
        frame_.directionX = directionX / length;
        frame_.directionZ = directionZ / length;
        // Leave a conservative launch margin for the 330-unit tail inside ordinary sea.
        frame_.launchDistance = std::min(2000.f, maxSeaDistance - 600.f);
        frame_.frontDistance = frame_.launchDistance;
        // Keep affecting ships and shore after the launch hull has crossed.
        const float continuation = std::max(700.f, std::min(maxSeaDistance - 600.f,
            interpolate(SeaTsunamiParameters::minContinuationDistance, SeaTsunamiParameters::maxContinuationDistance)));
        frame_.duration = (frame_.launchDistance + continuation) / frame_.speed;
    }

    void Advance(float seconds)
    {
        if (!frame_.active || !(seconds > 0.f) || !std::isfinite(seconds)) return;
        frame_.age += seconds;
        if (frame_.age >= frame_.duration) { Cancel(); return; }
        frame_.frontDistance = frame_.launchDistance - frame_.speed * frame_.age;
        frame_.envelope = SeaTsunamiSmooth(frame_.age / 5.f) *
                          SeaTsunamiSmooth((frame_.duration - frame_.age) / 8.f);
    }
};
