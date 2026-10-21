#include "cinematic_postfx.hpp"
#include <cmath>
#include <cstdio>
#include <cstdlib>

static void need(bool value, const char *message) {
  if (!value) { std::fprintf(stderr, "FAIL: %s\n", message); std::exit(1); }
}

int main() {
  auto even=CinematicPostFXPlan::halfResolution(1920,1080);
  auto odd=CinematicPostFXPlan::halfResolution(1279,719);
  need(even.valid() && even.bloomWidth==960 && even.bloomHeight==540,
       "even scene uses exact half-resolution bloom targets");
  need(odd.valid() && odd.bloomWidth==640 && odd.bloomHeight==360,
       "odd scene rounds half-resolution targets safely");
  need(even.offscreenPassCount==3 && even.compositePassCount==1,
       "plan fixes three bounded offscreen passes and one composite");
  need(even.floatingPointIntermediates,
       "bloom intermediates preserve subpixel energy without 8-bit banding");
  need(cinematicBloomWeight(.40f) == 0.0f, "ordinary surfaces do not bloom");
  need(cinematicBloomWeight(.68f) > 0.0f, "soft knee avoids a hard bloom pop");
  need(cinematicBloomWeight(.80f) > cinematicBloomWeight(.68f), "bloom rises with brightness");
  need(cinematicBloomWeight(1.0f) > cinematicBloomWeight(.80f), "bright emitters bloom most");

  CinematicExposure exposure;
  float previous = exposure.value;
  for (int i=0;i<90;i++) {
    float current=exposure.update(1.6f,1.0f/60.0f);
    need(current<=previous,"bright view darkens monotonically"); previous=current;
  }
  need(exposure.value>.72f && exposure.value<.88f,"bright view is gently bounded");
  previous=exposure.value;
  for (int i=0;i<90;i++) {
    float current=exposure.update(.10f,1.0f/60.0f);
    need(current>=previous,"dark view recovers monotonically"); previous=current;
  }
  need(exposure.value<1.18f && exposure.value>1.02f,"dark recovery is gradual and bounded");

  CinematicExposure hitchA, hitchB;
  hitchA.update(2.0f,5.0f); hitchB.update(2.0f,.1f);
  need(std::abs(hitchA.value-hitchB.value)<1e-6f,"frame hitch cannot jump adaptation");
  need(CinematicPostFXUniforms{}.bloomStrength>=.20f && CinematicPostFXUniforms{}.bloomStrength<=.28f,
       "default bloom is visible but remains bounded");
  need(cinematicStreakWeight(.98f,.52f,.18f)>.10f,
       "isolated lamp highlight produces a restrained streak source");
  need(cinematicStreakWeight(.94f,.91f,.02f)<.01f,
       "diffuse bright wall or sky is suppressed");
  need(cinematicStreakWeight(.94f,.91f,.32f)<.01f,
       "diffuse colored sky is suppressed despite chroma");
  need(cinematicStreakWeight(.70f,.20f,.40f)==0.f,
       "saturated ordinary surfaces remain below the highlight threshold");
  need(CinematicPostFXUniforms{}.streakStrength>=.045f && CinematicPostFXUniforms{}.streakStrength<=.06f,
       "anamorphic streak is visible but remains lower intensity than bloom");
  need(cinematicStreakSampleCount==7,
       "anamorphic composite cost stays bounded to seven half-resolution samples");
  std::puts("PASS: bloom/exposure, isolated-highlight anamorphic mask, diffuse rejection, bounded streak cost");
}
