#pragma once
#include "dx9render.h"
#include "shared/layers.h"
#include <vector>
#include <string>
struct FakeCore {VDX9RENDER*rs=nullptr;uint64_t revision=0;float timeScale=1,dt=16;bool erased=false;std::vector<std::string> actions;void*GetService(const char*){return rs;} uint64_t GetScenePauseRevision(){return revision;} float GetTimeScale(){return timeScale;}float GetRDeltaTime(){return dt;}void SetLayerType(int,layer_type_t){}void AddToLayer(int,int,int){}void PostEvent(const char*n,int,void*){actions.push_back(n);}void EraseEntity(int){actions.push_back("erase");erased=true;}void Trace(const char*){} };
inline FakeCore core;
