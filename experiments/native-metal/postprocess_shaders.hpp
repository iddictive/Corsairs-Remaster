#pragma once
#include "sea_shaders.hpp"
inline bool identifyPostprocessPixelShader(const uint32_t* p) {
    return seaShaderFingerprint(p)==0xd249c37095ecf339ull;
}
// Appended to the fixed-function library: the original PS-only pass consumes
// four interpolated UV sets from the ordinary transformed vertex path.
inline const char* postprocessShaderSource=R"MSL(
fragment float4 postprocess_fs(O v [[stage_in]],
 array<texture2d<float>,8> textures [[texture(0)]],
 array<sampler,8> samplers [[sampler(0)]]) {
 return (textures[0].sample(samplers[0],v.uv01.xy)
       + textures[1].sample(samplers[1],v.uv01.zw)
       + textures[2].sample(samplers[2],v.uv23.xy)
       + textures[3].sample(samplers[3],v.uv23.zw))*.25f;
}
)MSL";
