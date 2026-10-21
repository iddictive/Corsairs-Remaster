#pragma once
#include "entity.h"
namespace storm_map_player {inline constexpr float perspective=1.285f,overheadPitch=1.0f,minHeight=50,maxHeight=1000;inline CVECTOR seaPosition{10,3,20};inline float yaw=.4f;inline bool metricsAvailable=true;inline int boundsCalls=0,visualCalls=0,seaCalls=0,prepareCalls=0,discardCalls=0;
inline bool bounds(CVECTOR&s,CVECTOR&c){boundsCalls++;s={20,30,40};c={0,0,0};return metricsAvailable;}inline bool seaVisualBounds(CVECTOR&s,CVECTOR&c){visualCalls++;s={20,30,40};c={0,0,0};return metricsAvailable;}inline float miniatureScale(CVECTOR){return .1f;}inline bool seaBounds(CVECTOR&s,float&y,CVECTOR&p){seaCalls++;s={20,30,40};y=yaw;p=seaPosition;return true;}inline float area(CVECTOR,CVECTOR,float,float,float,float){return 20;}inline float distance(CVECTOR,CVECTOR,float,float,float,float){return 150;}inline bool prepareSeaVisual(const char*){prepareCalls++;return true;}inline void discardSeaVisual(){discardCalls++;}
}
