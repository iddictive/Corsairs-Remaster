#pragma once
struct Stage {simd_uint4 color,alpha;uint32_t coord,textured,result,transformFlags,stochastic;simd_float4 constantColor;simd_float4x4 transform;};
struct Uniform {simd_float4x4 m;simd_float4 viewport;uint32_t transformed,textured,alphaTest;float alphaRef;uint32_t specularEnable;simd_float4 factor,fogColor,fogParams;Stage stages[8];simd_float4 surfaceLight;};
struct RawLandUniform {simd_float4x4 m,world;simd_float4 materialDiffuse,materialAmbient,materialEmissive,materialSpecular,sceneAmbient,cameraPosition;simd_float4 skyLight[4];simd_float4 lightDiffuse[8],lightAmbient[8],lightSpecular[8],lightDirection[8],lightPositionRange[8],lightAttenuation[8],lightSpot[8];uint32_t stride,normalOffset,colorOffset,specularOffset,uvOffset,uvCount,hasColor,hasSpecular,lightMask,directionalMask,spotMask,diffuseFromColor,ambientFromColor,specularFromColor,outdoorResponse,authoredAlpha,specularEnable,localViewer,unlit,worldMapLighting;float materialPower;};
 struct PointLight {simd_float4 positionRange,color,attenuation;};
struct Sampling {simd_float4x4 sunWorld[2],world;simd_float4 point,sunFocusSplits,shadowTexel;simd_uint4 flags;PointLight lamps[64]{};simd_float4 pointSlots[8]{};simd_float4 pointWeights[2]{};};
static_assert(sizeof(Uniform)==1328);
static_assert(sizeof(Sampling)==3488);
