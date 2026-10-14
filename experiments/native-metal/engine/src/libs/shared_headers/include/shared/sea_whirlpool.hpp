#pragma once
#include <algorithm>
#include <cmath>

inline float SeaWhirlpoolSmooth(float t)
{
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

inline float SeaWhirlpoolLengthScale(float radius) { return std::min(radius, 180.f); }
inline float SeaWhirlpoolTheaterScale(float radius)
{ return SeaWhirlpoolSmooth((radius - 240.f) / 760.f); }
inline float SeaWhirlpoolDepthScale(float radius, float theater)
{ return (1.f - theater) * .28f * SeaWhirlpoolLengthScale(radius) + theater * .16f * radius; }
inline float SeaWhirlpoolOnset(float radius)
{ return 5.f + 15.f * SeaWhirlpoolSmooth((radius - 240.f) / 760.f); }
inline float SeaWhirlpoolFade(float radius)
{ return 3.f + 7.f * SeaWhirlpoolSmooth((radius - 240.f) / 760.f); }
inline float SeaWhirlpoolControlMultiplier(float control)
{ return std::isfinite(control) ? 1.f - std::clamp(control, 0.f, 1.f) : 1.f; }

// One scalar current profile; the fine-texture adapter uses these same tokens.
// Height, slopes and authored foam remain owned by the physical surface below.
#define STORM_SEA_WHIRLPOOL_FLOW_BODY \
    const float lengthScale = min(radius, 180.f); \
    const float q = r / radius; \
    const float inner = clamp(q / .10f, 0.f, 1.f); \
    const float outer = clamp((1.f - q) / .10f, 0.f, 1.f); \
    const float support = inner * inner * (3.f - 2.f * inner) * outer * outer * (3.f - 2.f * outer); \
    const float rimDistance = (q - .78f) / .18f; \
    const float band = exp(-rimDistance * rimDistance); \
    const float radialSpeed = .035f * lengthScale * strength * support * tanh((.78f - q) / .12f); \
    const float swirl = strength * min(.055f * lengthScale, .015f * r) * band * support;

struct SeaWhirlpoolFlow { float radialSpeed, swirl, support; };
inline SeaWhirlpoolFlow EvaluateSeaWhirlpoolFlow(float r, float radius, float strength)
{
    using std::min; using std::clamp; using std::exp; using std::tanh;
    STORM_SEA_WHIRLPOOL_FLOW_BODY
    return {radialSpeed, swirl, support};
}
#define STORM_SEA_WHIRLPOOL_STRINGIFY_INNER(value) #value
#define STORM_SEA_WHIRLPOOL_STRINGIFY(value) STORM_SEA_WHIRLPOOL_STRINGIFY_INNER(value)
inline constexpr char SeaWhirlpoolMetalFlow[] =
    "float3 seaWhirlpoolFlow(float r,float radius,float strength) { "
    STORM_SEA_WHIRLPOOL_STRINGIFY(STORM_SEA_WHIRLPOOL_FLOW_BODY)
    " return float3(radialSpeed,swirl,support); }\n";
#undef STORM_SEA_WHIRLPOOL_STRINGIFY
#undef STORM_SEA_WHIRLPOOL_STRINGIFY_INNER
#undef STORM_SEA_WHIRLPOOL_FLOW_BODY

struct SeaWhirlpoolFrame
{
    bool active = false;
    unsigned serial = 0;
    float centerX = 0.f, centerZ = 0.f, radius = 90.f, intensity = 1.f;
    float age = 0.f, duration = 90.f, envelope = 0.f;
    float stopAge = -1.f, stopEnvelope = 0.f;
};

inline float SeaWhirlpoolEnvelope(const SeaWhirlpoolFrame &frame, float age)
{
    const float life = SeaWhirlpoolSmooth(age / SeaWhirlpoolOnset(frame.radius)) *
                       SeaWhirlpoolSmooth((frame.duration - age) / SeaWhirlpoolFade(frame.radius));
    return frame.stopAge < 0.f ? life :
        std::min(life, frame.stopEnvelope * (1.f - SeaWhirlpoolSmooth((age - frame.stopAge) / SeaWhirlpoolFade(frame.radius))));
}

struct SeaWhirlpoolSample
{
    unsigned serial = 0;
    float height = 0.f, slopeX = 0.f, slopeZ = 0.f, foam = 0.f, load = 0.f;
    float velocityX = 0.f, velocityZ = 0.f, yaw = 0.f;
    float inwardX = 0.f, inwardZ = 0.f;
    float orbitX = 0.f, orbitZ = 0.f;
};

// Character control changes response to a common current, never water geometry.
// Full control still rides55% of tangential flow without inward trapping/yaw.
inline SeaWhirlpoolSample SeaWhirlpoolSteeringSample(SeaWhirlpoolSample sample, float control)
{
    const float struggle = SeaWhirlpoolControlMultiplier(control);
    const float mastered = 1.f - struggle, ride = 1.f - .45f * mastered;
    sample.velocityX = (sample.velocityX - sample.inwardX * mastered) * ride;
    sample.velocityZ = (sample.velocityZ - sample.inwardZ * mastered) * ride;
    sample.yaw *= struggle * ride;
    sample.orbitX *= struggle; sample.orbitZ *= struggle;
    sample.load *= struggle;
    return sample;
}

// One compact physical field, consumed by scalar/SSE water, hull and debris.
// Analytic gradients avoid finite-difference work in the per-vertex sea jobs.
inline SeaWhirlpoolSample EvaluateSeaWhirlpool(const SeaWhirlpoolFrame &frame, float x, float z)
{
    if (!frame.active || frame.envelope <= 0.f || !std::isfinite(x) || !std::isfinite(z)) return {};
    const float dx = x - frame.centerX, dz = z - frame.centerZ;
    const float r = std::hypot(dx, dz), q = r / frame.radius;
    if (q >= 1.f) return {};
    SeaWhirlpoolSample sample;
    sample.serial = frame.serial;
    const float a = frame.envelope * frame.intensity;
    const float lengthScale = SeaWhirlpoolLengthScale(frame.radius);
    const float clock = frame.age * lengthScale / frame.radius;
    const float t = 1.f - q * q, t2 = t * t, t3 = t2 * t;
    const float theater = SeaWhirlpoolTheaterScale(frame.radius);
    const float depth = .28f * lengthScale * a;
    const float theta = r > .0001f ? std::atan2(dz, dx) : 0.f;
    const float q2 = q * q, q3 = q2 * q;
    // A torn lip instead of a rotationally symmetric raised ring. Both its
    // radius and height vary coherently; q^2 keeps height/gradient zero at eye.
    const float lipA = 5.f * theta - .48f * clock;
    const float lipB = 9.f * theta + .31f * clock;
    const float lip = .78f + .04f * std::sin(lipA) + .015f * std::sin(lipB);
    const float dLipTheta = .20f * std::cos(lipA) + .135f * std::cos(lipB);
    const float rimQ = (q - lip) / .085f;
    const float mouth = std::exp(-rimQ * rimQ);
    const float breakA = 7.f * theta - 1.5f * q - .65f * clock;
    const float breakB = 11.f * theta + 3.f * q + .32f * clock;
    const float broken = .75f + .30f * std::sin(breakA) + .18f * std::sin(breakB);
    const float dBrokenQ = -.45f * std::cos(breakA) + .54f * std::cos(breakB);
    const float dBrokenTheta = 2.10f * std::cos(breakA) + 1.98f * std::cos(breakB);
    const float rimScale = .30f * lengthScale * a * (1.f + .30f * theater);
    const float rimEnvelope = q2 * t2;
    const float rim = rimScale * rimEnvelope * mouth * broken;
    sample.height = -depth * t3 + rim;
    float dQ = 6.f * depth * q * t2 + rimScale * mouth *
        ((2.f * q * t2 - 4.f * q3 * t) * broken +
         rimEnvelope * (-2.f * rimQ / .085f * broken + dBrokenQ));
    if (theater > 0.f)
    {
        const float theaterDepth = SeaWhirlpoolDepthScale(frame.radius, theater) * a;
        sample.height += depth * t3 - theaterDepth * ((1.f - theater) * t3 + theater * t2);
        dQ += theaterDepth * ((1.f - theater) * 6.f * q * t2 + theater * 4.f * q * t) -
              6.f * depth * q * t2;
    }
    float dTheta = rimScale * rimEnvelope * mouth *
        (2.f * rimQ / .085f * dLipTheta * broken + dBrokenTheta);
    const auto addFold = [&](float envelope, float derivativeQ, float phase,
                             float phaseQ, float phaseTheta, float sharpness) {
        const float shape = std::sin(phase) + sharpness * std::sin(2.f * phase);
        const float derivative = std::cos(phase) + 2.f * sharpness * std::cos(2.f * phase);
        sample.height += envelope * shape;
        dQ += derivativeQ * shape + envelope * derivative * phaseQ;
        dTheta += envelope * derivative * phaseTheta;
    };
    // Strong sheared primary folds, plus two secondary wave scales. These
    // actually displace the water/hull; shader noise cannot supply their shape.
    const float shear = 2.f * theta - 3.f * q - .30f * clock;
    const float phase = 3.f * theta - 13.f * q - .90f * clock + .7f * std::sin(shear);
    const float foldScale = .55f * lengthScale * a * (1.f + .65f * theater);
    addFold(foldScale * q3 * t2, foldScale * (3.f * q2 * t2 - 4.f * q2 * q2 * t),
        phase, -13.f - 2.1f * std::cos(shear), 3.f + 1.4f * std::cos(shear), .28f);
    const float secondary = 5.f * theta + 19.f * q - .75f * clock;
    const float crossScale = .15f * lengthScale * a * (1.f + .50f * theater);
    addFold(crossScale * q2 * t3, crossScale * (2.f * q * t3 - 6.f * q3 * t2),
        secondary, 19.f, 5.f, .18f);
    const float shortScale = .09f * lengthScale * a * (1.f + .35f * theater);
    addFold(shortScale * q3 * t2, shortScale * (3.f * q2 * t2 - 4.f * q2 * q2 * t),
        11.f * theta - 37.f * q + 1.31f * clock, -37.f, 11.f, .0f);
    // dh/dr and (dh/dtheta)/r. All angular terms have q^2 or stronger support,
    // so slopes converge to zero at the fixed eye and at the outer boundary.
    const float radial = dQ / frame.radius;
    const float angular = r > .0001f ? dTheta / r : 0.f;
    const float nx = r > .0001f ? dx / r : 0.f, nz = r > .0001f ? dz / r : 0.f;
    sample.slopeX = radial * nx - angular * nz;
    sample.slopeZ = radial * nz + angular * nx;
    sample.load = a * t2;
    const auto flow = EvaluateSeaWhirlpoolFlow(r, frame.radius, a);
    const float support = flow.support, radialSpeed = flow.radialSpeed, swirl = flow.swirl;
    sample.inwardX = nx * radialSpeed; sample.inwardZ = nz * radialSpeed;
    sample.velocityX = sample.inwardX - nz * swirl;
    sample.velocityZ = sample.inwardZ + nx * swirl;
    // Co-rotating frame: actual propulsion curvature is added by the hull owner.
    if (r > .0001f) {
        sample.orbitX = a * support * nz / r;
        sample.orbitZ = -a * support * nx / r;
        sample.yaw = -swirl / r;
    }
    const float bands = SeaWhirlpoolSmooth((std::sin(phase) - .1f) / .75f);
    const float whitewater = SeaWhirlpoolSmooth((std::sin(secondary) - .25f) / .70f);
    const float turbulence = SeaWhirlpoolSmooth((std::hypot(sample.slopeX, sample.slopeZ) - .12f) / .32f);
    sample.foam = std::clamp(a * t * (.10f + (.50f + .30f * theater) * bands +
        (.40f + .35f * theater) * mouth * broken + (.16f + .16f * theater) * whitewater +
        .25f * theater * turbulence), 0.f, .9f);
    return sample;
}

class SeaWhirlpool
{
    SeaWhirlpoolFrame frame_;
    unsigned serial_ = 0;
  public:
    const SeaWhirlpoolFrame &Frame() const { return frame_; }
    float CrestBound() const { return frame_.active ? .25f * SeaWhirlpoolLengthScale(frame_.radius) * frame_.intensity : 0.f; }
    float HeightBound() const
    {
        const float lengthScale = SeaWhirlpoolLengthScale(frame_.radius);
        const float theater = SeaWhirlpoolTheaterScale(frame_.radius);
        const float depth = SeaWhirlpoolDepthScale(frame_.radius, theater);
        return frame_.active ? std::max(.5f * lengthScale, depth + .25f * lengthScale) * frame_.intensity : 0.f;
    }
    void Clear() { frame_ = {}; }
    void Stop()
    {
        if (frame_.active && frame_.stopAge < 0.f)
        { frame_.stopAge = frame_.age; frame_.stopEnvelope = frame_.envelope; }
    }
    bool Start(float x, float z, float radius = 90.f, float intensity = 1.f,
               float duration = 90.f, float resumeAge = 0.f)
    {
        if (!std::isfinite(x) || !std::isfinite(z) || !std::isfinite(radius) ||
            !std::isfinite(intensity) || !std::isfinite(duration) || !std::isfinite(resumeAge)) return false;
        radius = std::clamp(radius, 30.f, 1600.f);
        intensity = std::clamp(intensity, 0.f, 1.f);
        duration = std::clamp(duration, 6.f, 900.f);
        if (frame_.active && frame_.centerX == x && frame_.centerZ == z && frame_.radius == radius &&
            frame_.intensity == intensity && frame_.duration == duration) return false;
        Clear();
        frame_.centerX = x; frame_.centerZ = z; frame_.radius = radius;
        frame_.intensity = intensity; frame_.duration = duration;
        frame_.age = std::clamp(resumeAge, 0.f, duration);
        frame_.serial = ++serial_;
        if (frame_.serial == 0) frame_.serial = ++serial_;
        frame_.active = intensity > 0.f && frame_.age < duration;
        frame_.envelope = SeaWhirlpoolEnvelope(frame_, frame_.age);
        return true;
    }
    void Advance(float seconds)
    {
        if (!frame_.active || !(seconds > 0.f) || !std::isfinite(seconds)) return;
        frame_.age = std::min(frame_.age + seconds, frame_.duration);
        frame_.envelope = SeaWhirlpoolEnvelope(frame_, frame_.age);
        if (frame_.age >= frame_.duration ||
            (frame_.stopAge >= 0.f && frame_.age >= frame_.stopAge + SeaWhirlpoolFade(frame_.radius))) frame_.active = false;
    }
};

struct SeaWhirlpoolContact
{
    unsigned serial = 0;
    bool initialized = false;
    float x = 0.f, z = 0.f, age = 0.f, paid = 0.f;
    float Query(const SeaWhirlpoolFrame &frame, float nextX, float nextZ, float response = 1.f)
    {
        if (frame.serial == 0) return paid;
        if (serial != frame.serial) { *this = {}; serial = frame.serial; }
        if (!std::isfinite(nextX) || !std::isfinite(nextZ) || (initialized && frame.age <= age)) return paid;
        if (initialized)
        {
            const float seconds = frame.age - age;
            const int steps = std::clamp(static_cast<int>(std::ceil(seconds / .125f)), 1, 256);
            for (int i = 0; i < steps; ++i)
            {
                const float u = (static_cast<float>(i) + .5f) / steps;
                auto crossing = frame;
                crossing.active = true;
                crossing.age = age + u * seconds;
                crossing.envelope = SeaWhirlpoolEnvelope(frame, crossing.age);
                paid = std::min(.5f, paid + EvaluateSeaWhirlpool(crossing,
                    x + (nextX - x) * u, z + (nextZ - z) * u).load * seconds *
                    (std::isfinite(response) ? std::clamp(response, 0.f, 1.f) : 1.f) / (40.f * steps));
            }
        }
        initialized = true; x = nextX; z = nextZ; age = frame.age;
        return paid;
    }
};

struct SeaWhirlpoolMotion
{
    float pitch = 0.f, roll = 0.f;
    void Reset() { *this = {}; }
    void Update(const SeaWhirlpoolSample &sample, float heading, float protection, float seconds)
    {
        if (sample.serial == 0 || protection <= 0.f) { Reset(); return; }
        if (!(seconds > 0.f) || !std::isfinite(seconds)) return;
        const float bowSlope = sample.slopeX * std::sin(heading) + sample.slopeZ * std::cos(heading);
        const float beamSlope = sample.slopeX * std::cos(heading) - sample.slopeZ * std::sin(heading);
        const float blend = 1.f - std::exp(-3.f * std::min(seconds, .25f));
        pitch += (std::clamp(bowSlope * .10f, -.12f, .12f) * protection - pitch) * blend;
        roll += (std::clamp(beamSlope * .15f - sample.load * .10f, -.24f, .24f) * protection - roll) * blend;
    }
};


struct SeaShipEmergenceSample
{
    float height = 0.f, velocity = 0.f, pitch = 0.f, roll = 0.f, splash = 0.f;
};

inline float SeaShipHermite(float u, float p, float v, float duration)
{
    return (2.f*u*u*u - 3.f*u*u + 1.f)*p + (u*u*u - 2.f*u*u + u)*duration*v;
}
inline float SeaShipHermiteVelocity(float u, float p, float v, float duration)
{
    return (6.f*u*u - 6.f*u)*p/duration + (3.f*u*u - 4.f*u + 1.f)*v;
}

// One finite vertical impulse relative to ordinary, displaced-water buoyancy.
// Hermite entry/return match the ballistic segment's position and velocity.
inline SeaShipEmergenceSample EvaluateSeaShipEmergence(float age, float duration, float depth)
{
    if (!std::isfinite(age) || !std::isfinite(duration) || !std::isfinite(depth) ||
        duration <= 0.f || age < 0.f || age >= duration || depth <= 0.f) return {};
    constexpr float launch = 14.f, gravity = 9.81f, entry = 2.f, settle = 1.5f;
    constexpr float flight = 2.f*launch/gravity, total = entry + flight + settle;
    const float scale = total/duration, time = age*scale;
    SeaShipEmergenceSample sample;
    if (time < entry) {
        const float u = time/entry;
        sample.height = SeaShipHermite(u, -depth, 0.f, entry) +
            (u*u*u-u*u)*entry*launch;
        sample.velocity = SeaShipHermiteVelocity(u, -depth, 0.f, entry) +
            (3.f*u*u-2.f*u)*launch;
        sample.pitch = .31415927f*u*u*(3.f-2.f*u);
    } else if (time < entry + flight) {
        const float t = time-entry;
        sample.height = launch*t - .5f*gravity*t*t;
        sample.velocity = launch-gravity*t;
        sample.pitch = .31415927f*std::cos(3.14159265f*t/flight);
    } else {
        const float u = (time-entry-flight)/settle;
        sample.height = SeaShipHermite(u, 0.f, -launch, settle);
        sample.velocity = SeaShipHermiteVelocity(u, 0.f, -launch, settle);
        sample.pitch = -.31415927f*(1.f-u*u*(3.f-2.f*u));
    }
    sample.velocity *= scale;
    const float u = time/total;
    sample.roll = .17453293f*std::sin(6.2831853f*u)*std::sin(3.14159265f*u);
    const float wet = std::clamp((10.f-std::abs(sample.height))/6.f,0.f,1.f);
    const float impact = std::clamp((std::abs(sample.velocity)-3.f)/6.f,0.f,1.f);
    sample.splash = wet*wet*(3.f-2.f*wet)*impact*impact*(3.f-2.f*impact);
    return sample;
}

inline SeaShipEmergenceSample SettleSeaShipEmergence(float age, SeaShipEmergenceSample start)
{
    if (!std::isfinite(age) || !std::isfinite(start.height) || !std::isfinite(start.velocity) ||
        !std::isfinite(start.pitch) || !std::isfinite(start.roll) || age < 0.f || age >= 1.5f) return {};
    const float u = age/1.5f, decay = 1.f-u*u*(3.f-2.f*u);
    return {SeaShipHermite(u,start.height,start.velocity,1.5f),
        SeaShipHermiteVelocity(u,start.height,start.velocity,1.5f),
        start.pitch*decay,start.roll*decay,0.f};
}
