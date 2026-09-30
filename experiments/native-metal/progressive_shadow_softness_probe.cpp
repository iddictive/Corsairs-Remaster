#include "progressive_shadow_softness.hpp"
#include <cstdio>
#include <cstdlib>

static void need(bool condition, const char* message) {
  if (!condition) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() {
  using namespace progressive_shadow;
  const float sunContact = sunRadiusTexels(.5005f, .5f, 160.f, 32.f / 512.f);
  const float sunNear = sunRadiusTexels(.505f, .5f, 160.f, 32.f / 512.f);
  const float sunFar = sunRadiusTexels(.54f, .5f, 160.f, 32.f / 512.f);
  need(sunContact < sunNear && sunNear < sunFar, "sun softness grows with blocker separation");
  need(sunContact < 1.f, "sun contact remains crisp");
  need(sunRadiusTexels(.9f, .1f, 160.f, 32.f / 512.f) == 5.f, "sun kernel is bounded");
  need(sunRadiusTexels(.4f, .5f, 160.f, 32.f / 512.f) == .75f, "lit sun sample has minimum radius");

  const float pointContact = pointRadiusTexels(4.02f, 4.f);
  const float pointNear = pointRadiusTexels(4.4f, 4.f);
  const float pointFar = pointRadiusTexels(7.f, 4.f);
  need(pointContact < pointNear && pointNear < pointFar, "point softness grows with blocker separation");
  need(pointContact < 1.f, "point contact remains crisp");
  need(pointRadiusTexels(20.f, 1.f) == 4.f, "point kernel is bounded");
  need(pointRadiusTexels(3.f, 4.f) == .75f, "non-blocking point depth has minimum radius");
  need(pointRadiusTexels(4.4f, 4.f, .16f) > pointNear,
       "larger point source produces a wider penumbra");
  need(sunRadiusTexels(.505f, .5f, 160.f, 32.f / 512.f, .07f) > sunNear,
       "larger sun angle produces a wider penumbra");
  const float distanceNear=distanceMinimumRadius(4.f,8.f,32.f,1.5f);
  const float distanceMid=distanceMinimumRadius(20.f,8.f,32.f,1.5f);
  const float distanceFar=distanceMinimumRadius(40.f,8.f,32.f,1.5f);
  need(distanceNear==.75f&&distanceNear<distanceMid&&distanceMid<distanceFar,
       "minimum filtering grows with receiver distance");
  need(distanceFar==1.5f,"distance filtering is bounded");
  std::puts("PASS progressive sun/point softness is monotonic, contact-crisp, and bounded");
}
