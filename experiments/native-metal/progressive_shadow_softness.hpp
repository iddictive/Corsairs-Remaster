#pragma once
#include <algorithm>
#include <cmath>

namespace progressive_shadow {

inline float sunRadiusTexels(float receiverDepth, float blockerDepth,
                             float depthWorldSpan, float worldTexel,
                             float angularRadius = .035f,
                             float minimumRadius = .75f) {
  if (!(blockerDepth < receiverDepth) || worldTexel <= 0.f)
    return minimumRadius;
  const float separation = (receiverDepth - blockerDepth) * depthWorldSpan;
  return std::clamp(minimumRadius + separation * angularRadius / worldTexel,
                    minimumRadius, 5.f);
}

inline float pointRadiusTexels(float receiverMajor, float blockerMajor,
                               float sourceRadius = .08f,
                               float minimumRadius = .75f) {
  if (!(blockerMajor < receiverMajor) || blockerMajor <= .1f)
    return minimumRadius;
  const float penumbraRatio = (receiverMajor - blockerMajor) / blockerMajor;
  // A cube face covers 90 degrees. Half its 256 texels project one radian,
  // making a world-space source radius map to 128 texels at unit distance.
  return std::clamp(minimumRadius + penumbraRatio * sourceRadius * 128.f,
                    minimumRadius, 4.f);
}

inline float distanceMinimumRadius(float distance, float nearDistance,
                                   float farDistance, float maximumRadius) {
  if (farDistance <= nearDistance) return .75f;
  const float t = std::clamp((distance - nearDistance) /
                             (farDistance - nearDistance), 0.f, 1.f);
  return .75f + t * (maximumRadius - .75f);
}

} // namespace progressive_shadow
