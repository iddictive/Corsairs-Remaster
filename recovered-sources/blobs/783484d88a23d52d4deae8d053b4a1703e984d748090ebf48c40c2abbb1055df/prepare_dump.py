#!/usr/bin/env python3
"""Derive a bounded CPU-only lighting observation variant; never edit canonical source."""
from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[3]
SOURCE = ROOT / "experiments/native-metal/backend.mm"
EXPECTED_SHA256 = "4397960abc0e8e6fb6091bd8172da0c667d66959fc2a943f46274506d893c3c9"
ANCHOR = " const NSUInteger logicalRawIndexOffset=rawIndexOffset;\n"
INSERT = r'''
 // Private diagnostic: CPU source bytes only, no readbacks or render mutations.
 // FrameProfile::frames resets at120; trace.sequence is cumulative in benchmarks.
 if(target==back&&!offscreenPointCasterDepth&&!rawSkinned&&
    (rawDynamicSail||rawSeaLit)&&profile.enabled&&
    (profile.trace.sequence>=128||profile.frames>=128)){
  static std::array<uint64_t,32> observed{};static unsigned observedCount=0,hullCount=0;
  uint64_t cohort=1469598103934665603ull;
  auto mixKey=[&](const void*p,size_t count){const auto*b=static_cast<const uint8_t*>(p);for(size_t n=0;n<count;++n){cohort^=b[n];cohort*=1099511628211ull;}};
  const uint64_t identity[]={uint64_t(rawDynamicSail),landTexture?landTexture->cacheIdentity:0,
   uint64_t(fvf),uint64_t(raw.stride),uint64_t(raw.diffuseFromColor),uint64_t(raw.ambientFromColor),
   uint64_t(raw.lightMask),uint64_t(raw.directionalMask)};
  mixKey(identity,sizeof(identity));mixKey(&raw.materialDiffuse,sizeof(raw.materialDiffuse));
  mixKey(&raw.materialAmbient,sizeof(raw.materialAmbient));mixKey(&raw.materialEmissive,sizeof(raw.materialEmissive));
  bool seen=false;for(unsigned n=0;n<observedCount;++n)seen|=observed[n]==cohort;
  if(!seen&&observedCount<32&&(rawDynamicSail||hullCount<31)&&vb&&ib&&drawCount&&raw.stride){
   std::string json="{\"event\":\"daylight_ship_light\"";
   auto escaped=[](const char*p){std::string s="\"";if(p)for(const unsigned char*b=reinterpret_cast<const unsigned char*>(p);*b;++b){if(*b=='"'||*b=='\\'){s+='\\';s+=char(*b);}else if(*b<32){char q[7];snprintf(q,sizeof(q),"\\u%04x",unsigned(*b));s+=q;}else s+=char(*b);}return s+"\"";};
   auto number=[](double v){if(!std::isfinite(v))return std::string("null");char b[64];snprintf(b,sizeof(b),"%.9g",v);return std::string(b);};
   auto value=[&](const char*k,double v){json+=","+escaped(k)+":"+number(v);};
   auto text=[&](const char*k,const char*v){json+=","+escaped(k)+":"+escaped(v);};
   auto vec=[&](const char*k,simd_float4 v,unsigned count=4){json+=","+escaped(k)+":[";for(unsigned n=0;n<count;++n){if(n)json+=",";json+=number(v[n]);}json+="]";};
   text("category",rawDynamicSail?"sail":"hull");value("packet",observedCount);
   value("frame",profile.trace.sequence);value("profileFrames",profile.frames);
   value("fvf",fvf);value("stride",raw.stride);value("vertexOffset",logicalRawVertexOffset);
   value("indexOffset",logicalRawIndexOffset);value("baseVertex",rawBaseVertex);value("originalBase",base);
   value("originalOffset",offset);value("indexWidth",rawIndexWidth);value("drawCount",drawCount);
   value("vbBytes",vb->bytes.size());value("ibBytes",ib->bytes.size());
   value("textureId",landTexture?landTexture->cacheIdentity:0);text("sourceTextureLabel",landTexture?landTexture->diagnosticLabel.c_str():"");
   id<MTLTexture> diagnosticTexture=landTexture?landTexture->gpu():nil;
   text("textureLabel",diagnosticTexture.label.UTF8String);value("textureWidth",diagnosticTexture.width);value("textureHeight",diagnosticTexture.height);
   value("hasColor",raw.hasColor);value("hasSpecular",raw.hasSpecular);value("normalOffset",raw.normalOffset);
   value("colorOffset",raw.colorOffset);value("uvOffset",raw.uvOffset);value("uvCount",raw.uvCount);
   value("diffuseFromColor",raw.diffuseFromColor);value("ambientFromColor",raw.ambientFromColor);
   value("outdoorResponse",raw.outdoorResponse);value("unlit",raw.unlit);value("authoredAlpha",raw.authoredAlpha);
   value("worldMapLighting",raw.worldMapLighting);value("skyLighting",skyLighting);
   value("lightMask",raw.lightMask);value("directionalMask",raw.directionalMask);value("spotMask",raw.spotMask);
   value("enabledLight0",enabled[0]);value("light0Type",uint32_t(lights[0].Type));
   value("priorSeaSun",priorSeaSun);value("prepassLit",prepassLit);value("prepassSunValid",landShadow.prepass.sunValid);
   value("prepassFrame",landShadow.prepass.frame);value("locationActive",landShadow.locationActive);
   value("indoor",landShadow.indoor);value("lightingFlagsZ",lighting.flags.z);value("catalogLampCount",lighting.flags.w);
   value("ambientMaterialSource",rs[storm_render::RenderAmbientMaterialSource]);
   value("diffuseMaterialSource",rs[storm_render::RenderDiffuseMaterialSource]);value("colorVertex",rs[storm_render::RenderVertexColor]);
   vec("materialDiffuse",raw.materialDiffuse);vec("materialAmbient",raw.materialAmbient);vec("materialEmissive",raw.materialEmissive);
   vec("sceneAmbient",raw.sceneAmbient);vec("solarGain",solarGain);
   for(unsigned n=0;n<4;++n){const auto k="skySH"+std::to_string(n);vec(k.c_str(),raw.skyLight[n]);}
   vec("rawLight0Diffuse",raw.lightDiffuse[0]);vec("rawLight0Ambient",raw.lightAmbient[0]);vec("rawLight0Direction",raw.lightDirection[0]);
   vec("enabledLight0Diffuse",simd_make_float4(lights[0].Diffuse.r,lights[0].Diffuse.g,lights[0].Diffuse.b,lights[0].Diffuse.a));
   vec("enabledLight0Ambient",simd_make_float4(lights[0].Ambient.r,lights[0].Ambient.g,lights[0].Ambient.b,lights[0].Ambient.a));
   vec("enabledLight0Direction",simd_make_float4(lights[0].Direction.x,lights[0].Direction.y,lights[0].Direction.z,0.f));
   json+=",\"samples\":[";
   for(unsigned sample=0;sample<8;++sample){
    if(sample)json+=",";json+="{";
    const uint64_t ordinal=uint64_t(sample)*(drawCount-1)/7;
    const uint64_t indexByte=uint64_t(logicalRawIndexOffset)+ordinal*rawIndexWidth;
    uint32_t sourceIndex=0;bool valid=indexByte<=ib->bytes.size()&&rawIndexWidth<=ib->bytes.size()-indexByte;
    if(valid){if(rawIndexWidth==2){uint16_t i;memcpy(&i,ib->bytes.data()+indexByte,2);sourceIndex=i;}else memcpy(&sourceIndex,ib->bytes.data()+indexByte,4);}
    const int64_t logicalIndex=int64_t(sourceIndex)+int64_t(rawBaseVertex);
    uint64_t vertexByte=0;if(logicalIndex<0)valid=false;
    else {const uint64_t vertex=uint64_t(logicalIndex);if(vertex>(UINT64_MAX-uint64_t(logicalRawVertexOffset))/raw.stride)valid=false;else vertexByte=uint64_t(logicalRawVertexOffset)+vertex*raw.stride;}
    const uint64_t required=std::max({uint64_t(12),uint64_t(raw.normalOffset)+12,raw.hasColor?uint64_t(raw.colorOffset)+4:0,raw.uvCount?uint64_t(raw.uvOffset)+8*std::min(raw.uvCount,4u):0});
    valid&=required<=raw.stride&&vertexByte<=vb->bytes.size()&&required<=vb->bytes.size()-vertexByte;
    json+="\"ordinal\":"+number(ordinal);value("index",sourceIndex);value("logicalIndex",logicalIndex);value("vertexByte",vertexByte);value("valid",valid);
    if(valid){
     const uint8_t*p=vb->bytes.data()+vertexByte;float a[3],b[3];memcpy(a,p,12);memcpy(b,p+raw.normalOffset,12);
     const simd_float3 local={a[0],a[1],a[2]},sourceNormal={b[0],b[1],b[2]};
     const simd_float3 normal=simd_normalize(simd_mul(raw.world,simd_make_float4(sourceNormal,0)).xyz);
     const simd_float3 worldPosition=simd_mul(raw.world,simd_make_float4(local,1)).xyz;
     simd_float4 authored={1,1,1,1};if(raw.hasColor){uint32_t packed;memcpy(&packed,p+raw.colorOffset,4);authored=color(packed);}
     const simd_float4 diffuse=raw.diffuseFromColor?authored:raw.materialDiffuse,ambient=raw.ambientFromColor?authored:raw.materialAmbient;
     const simd_float3 albedo=simd_max(diffuse.xyz,simd_make_float3(.001f));
     const simd_float3 effAmbient=raw.outdoorResponse?simd_max(ambient.xyz,albedo*.38f):ambient.xyz;
     const float sky=std::clamp(normal.y*.5f+.5f,0.f,1.f);
     const float q=std::clamp((normal.y+.15f)/.97f,0.f,1.f);
     const float ambientFactor=raw.worldMapLighting?.32f+(.75f-.32f)*q*q*(3.f-2.f*q):(raw.outdoorResponse?.18f+.44f*sky:1.f);
     const auto irradiance=storm_metal::skyIrradiance(normal,raw.skyLight);
     const simd_float3 skyContribution=diffuse.xyz*irradiance,ambientContribution=effAmbient*raw.sceneAmbient.xyz*ambientFactor;
     simd_float3 preclamp=raw.unlit?(raw.outdoorResponse?diffuse.xyz*(raw.sceneAmbient.xyz+storm_metal::skyIrradiance({0,1,0},raw.skyLight)+raw.lightDiffuse[0].xyz):diffuse.xyz):raw.materialEmissive.xyz+ambientContribution+skyContribution;
     simd_float3 directTotal={0,0,0},lightAmbientTotal={0,0,0},sunDirect={0,0,0};
     for(unsigned n=0;n<8;++n)if(!raw.unlit&&(raw.lightMask&(1u<<n))){
      float attenuation=1.f;simd_float3 direction;
      if(raw.directionalMask&(1u<<n))direction=simd_normalize(-raw.lightDirection[n].xyz);
      else {direction=raw.lightPositionRange[n].xyz-worldPosition;const float distance=simd_length(direction);if(distance>raw.lightPositionRange[n].w)continue;
       const auto at=raw.lightAttenuation[n];attenuation=1.f/std::max(.0001f,at.x+at.y*distance+at.z*distance*distance);direction=simd_normalize(direction);
       if(raw.spotMask&(1u<<n)){const float rho=simd_dot(simd_normalize(-raw.lightDirection[n].xyz),direction),inner=std::cos(raw.lightSpot[n].x*.5f),outer=std::cos(raw.lightSpot[n].y*.5f);
        attenuation*=rho>=inner?1.f:rho<=outer?0.f:std::pow(std::clamp((rho-outer)/std::max(.000001f,inner-outer),0.f,1.f),raw.lightSpot[n].z);}}
      float response=std::max(0.f,simd_dot(normal,direction));if(raw.outdoorResponse&&(raw.directionalMask&(1u<<n)))response=.82f*std::max(0.f,(response-.04f)/.96f);
      const auto direct=attenuation*diffuse.xyz*raw.lightDiffuse[n].xyz*response;
      const auto ambientLight=attenuation*ambient.xyz*raw.lightAmbient[n].xyz;
      directTotal+=direct;lightAmbientTotal+=ambientLight;preclamp+=ambientLight+direct;if(n==0)sunDirect=direct;
     }
     vec("local",simd_make_float4(local,0),3);vec("sourceNormal",simd_make_float4(sourceNormal,0),3);vec("normal",simd_make_float4(normal,0),3);
     vec("authoredColor",authored);vec("resolvedDiffuse",diffuse);vec("resolvedAmbient",ambient);
     vec("skyIrradiance",simd_make_float4(irradiance,0),3);vec("skyRGB",simd_make_float4(skyContribution,0),3);
     vec("ambientRGB",simd_make_float4(ambientContribution,0),3);vec("directRGB",simd_make_float4(directTotal,0),3);
     vec("lightAmbientRGB",simd_make_float4(lightAmbientTotal,0),3);vec("sunDirectRGB",simd_make_float4(sunDirect,0),3);
     vec("preclampRGB",simd_make_float4(preclamp,0),3);vec("clampedRGB",simd_make_float4(simd_clamp(preclamp,0.f,1.f),0),3);
     json+=",\"uv\":[";for(unsigned n=0;n<std::min(raw.uvCount,4u);++n){float uv[2];memcpy(uv,p+raw.uvOffset+n*8,8);if(n)json+=",";json+="["+number(uv[0])+","+number(uv[1])+"]";}json+="]";
    }
    json+="}";
   }
   json+="]}";fprintf(stderr,"[StormMetal daylight-light] %s\n",json.c_str());
   observed[observedCount++]=cohort;if(!rawDynamicSail)++hullCount;
  }
 }
'''

payload = SOURCE.read_bytes()
actual = hashlib.sha256(payload).hexdigest()
assert actual == EXPECTED_SHA256, ("canonical backend changed", actual)
source = payload.decode()
assert source.count(ANCHOR) == 1, "logical-offset anchor must be unique"
assert "[StormMetal daylight-light]" not in source, "dump already present"
result = source.replace(ANCHOR, ANCHOR + INSERT, 1)
assert result.count(INSERT) == 1 and result.replace(INSERT, "", 1) == source
destination = Path(__file__).with_name("backend-light-dump.mm")
destination.write_text(result)
receipt = {"source": str(SOURCE), "source_sha256": actual,
           "variant": str(destination), "variant_sha256": hashlib.sha256(result.encode()).hexdigest(),
           "insertion_count": 1, "packet_limit": 32,
           "gate": "main backbuffer, non-skinned raw sea/sail; trace.sequence>=128; profile enabled",
           "scope": "CPU source byte observations only; no command, readback, or pixel changes"}
Path(__file__).with_name("prepare-receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
print(json.dumps(receipt, indent=2))
