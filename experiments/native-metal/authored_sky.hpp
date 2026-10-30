#pragma once
namespace storm_metal {
struct AuthoredSkyFrame { float centerY, cameraOffsetY, cameraCentering; };

inline AuthoredSkyFrame authoredSkyFrame(float cameraY, float skyHalfExtent, float fogRadius) {
    const AuthoredSkyFrame legacy{cameraY / 6.f, cameraY * (5.f / 6.f), 0.f};
    const float extent = skyHalfExtent < fogRadius ? skyHalfExtent : fogRadius;
    if (!(extent > 0.f)) return legacy;
    // Preserve authored near-sea parallax. Before the eye can leave either
    // finite enclosure, continuously change to a camera-centred sky frame.
    // This changes only the render origin; world/camera altitude is untouched.
    const float altitude = cameraY < 0.f ? -cameraY : cameraY;
    const float start = extent * .1f, finish = extent * .5f;
    if (altitude <= start) return legacy;
    if (altitude >= finish) return {cameraY, 0.f, 1.f};
    const float t = (altitude - start) / (finish - start);
    const float blend = t * t * (3.f - 2.f * t);
    const float offset = legacy.cameraOffsetY * (1.f - blend);
    return {cameraY - offset, offset, blend};
}

// SKY's five authored faces share this mapping for astronomy opacity and the
// atmospheric field. The same tokens compile as C++ and shader source, so the
// GPU cannot grow a second face/UV convention. Inputs are local to SKY.
#define STORM_AUTHORED_SKY_COORDINATES \
struct AuthoredSkyCoordinate {int face;float u,v;}; \
inline AuthoredSkyCoordinate authoredSkyCoordinate(float x,float y,float z) { \
    if(y<0.f)y=0.f; \
    const float ax=x<0.f?-x:x,az=z<0.f?-z:z; \
    if(y>0.f&&y>=ax&&y>=az)return {4,.5f-x/(2.f*y),.5f+z/(2.f*y)}; \
    if(az>0.f&&az>=ax)return z<0.f? \
        AuthoredSkyCoordinate{0,.5f+x/(2.f*az),1.f-y/az}: \
        AuthoredSkyCoordinate{2,.5f-x/(2.f*az),1.f-y/az}; \
    if(ax>0.f)return x>0.f? \
        AuthoredSkyCoordinate{1,.5f+z/(2.f*ax),1.f-y/ax}: \
        AuthoredSkyCoordinate{3,.5f-z/(2.f*ax),1.f-y/ax}; \
    return {-1,0,0}; \
}
STORM_AUTHORED_SKY_COORDINATES
#define STORM_SKY_STRING_LITERAL_(...) #__VA_ARGS__
#define STORM_SKY_STRING_LITERAL(...) STORM_SKY_STRING_LITERAL_(__VA_ARGS__)
inline constexpr const char*authoredSkyCoordinatesMSL=STORM_SKY_STRING_LITERAL(STORM_AUTHORED_SKY_COORDINATES);
#undef STORM_SKY_STRING_LITERAL
#undef STORM_SKY_STRING_LITERAL_
#undef STORM_AUTHORED_SKY_COORDINATES
}
