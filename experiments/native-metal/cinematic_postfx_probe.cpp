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
  need(CinematicPostFXUniforms{}.bloomStrength>=.18f && CinematicPostFXUniforms{}.bloomStrength<=.22f,
       "default bloom is visible but remains bounded");
  need(CinematicPostFXUniforms{}.bloomThreshold>=.82f,
       "ordinary lamp-lit rigging stays below the bloom extraction floor");
  need(cinematicBloomWeight(.72f,CinematicPostFXUniforms{}.bloomThreshold,
                            CinematicPostFXUniforms{}.bloomKnee)==0.f,
       "bright diffuse receiver does not become self-illuminated");
  need(cinematicBloomWeight(.98f,CinematicPostFXUniforms{}.bloomThreshold,
                            CinematicPostFXUniforms{}.bloomKnee)>0.f,
       "near-white lamp emitter still blooms");
  need(cinematicEdgeBlend(.50f,.47f,.53f,.49f,.51f)==0.f,
       "low-contrast texture detail is not filtered");
  need(cinematicEdgeBlend(.02f,.02f,.98f,.02f,.02f)>.25f,
       "high-contrast scene geometry receives bounded edge reconstruction");
  need(cinematicEdgeBlend(.02f,.02f,.98f,.02f,.02f)<=.34f,
       "edge reconstruction never becomes a full-pixel blur");
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
  const auto outdoor=cinematicSceneTuning(false);
  const auto indoor=cinematicSceneTuning(true);
  need(!cinematicSceneEnabled(true,false,false),
       "sea skips cinematic extraction, blur, composite and luminance passes");
  need(!cinematicSceneEnabled(true,true,false),
       "outdoor locations preserve the authored scene without cinematic passes");
  need(cinematicSceneEnabled(true,true,true),
       "enabled interiors retain the accepted cinematic treatment");
  need(!cinematicSceneEnabled(false,true,true),
       "global disable still bypasses the interior treatment");
  need(outdoor.exposure==1.f && outdoor.contrast==1.f &&
       outdoor.bloomStrength==CinematicPostFXUniforms{}.bloomStrength,
       "outdoor postfx remains exactly at the authored default");
  need(indoor.exposure>1.f && indoor.contrast<1.f,
       "interior grade lifts dark midtones and lowers contrast");
  need(indoor.bloomStrength<outdoor.bloomStrength &&
       indoor.bloomThreshold>outdoor.bloomThreshold &&
       indoor.streakStrength<outdoor.streakStrength,
       "interior lamps receive less bloom and streak energy");
  need(((.10f*indoor.exposure-indoor.contrastPivot)*indoor.contrast+
        indoor.contrastPivot)>.10f,
       "interior grade visibly lifts a dark surface");
  need(((.95f*indoor.exposure-indoor.contrastPivot)*indoor.contrast+
        indoor.contrastPivot)<.95f*1.03f,
       "interior grade compresses highlights instead of clipping them harder");
  need(cinematicStreakSampleCount==7,
       "anamorphic composite cost stays bounded to seven half-resolution samples");
  std::puts("PASS: indoor-only bloom/exposure, authored outdoor/sea passthrough, scene edge AA, bounded streak cost");
}
