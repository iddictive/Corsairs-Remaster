#include "dev_telemetry.hpp"

#include <array>
#include <cmath>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace {
struct Control { std::string action; int state = 0; float value = 0; };
struct Light {
    unsigned index = 0; std::uint64_t id = 0;
    std::array<float, 4> positionRange{}, color{};
    bool selected = false, shadowed = false;
};
struct Snapshot {
    std::string mode = "unknown", controlGroup, deckRejection;
    std::vector<Control> controls;
    std::array<float, 16> view{}, projection{};
    std::array<float, 4> viewport{};
    std::array<float, 3> worldOrigin{}, deckFrom{}, deckDesired{}, deckResolved{};
    bool cameraValid = false, cameraSceneBoundary = false, deckValid = false, canWalkTo = false, crewAdjusted = false;
    unsigned catalogCount = 0; std::uint64_t selectedId = 0;
    std::vector<Light> lights;
    unsigned casterDraws = 0, casterVertices = 0, receiverDraws = 0;
    unsigned casterRejects = 0, receiverRejects = 0, requestedLamps = 0, completeLamps = 0;
    bool shadowFailed = false, shadowApplied = false;
} state;

const char* outputPath() { static const char* value = std::getenv("STORM_METAL_DEV_TELEMETRY"); return value && *value ? value : nullptr; }
std::string escaped(const std::string& value) {
    std::string out;
    for (unsigned char c : value) {
        if (c == '"' || c == '\\') { out += '\\'; out += char(c); }
        else if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\t') out += "\\t";
        else if (c >= 0x20) out += char(c);
    }
    return out;
}
template <size_t N> void vectorJson(std::ostringstream& out, const std::array<float, N>& v) {
    out << '['; for (size_t i = 0; i < N; ++i) { if (i) out << ','; out << (std::isfinite(v[i]) ? v[i] : 0.f); } out << ']';
}
std::string controlPath() { return std::string(outputPath() ? outputPath() : "") + ".control.json"; }
bool namedBool(const std::string& json, const char* name, bool& value) {
    const std::string key = "\"" + std::string(name) + "\"";
    auto at = json.find(key); if (at == std::string::npos) return false;
    at = json.find(':', at + key.size()); if (at == std::string::npos) return false;
    at = json.find_first_not_of(" \t\r\n", at + 1); if (at == std::string::npos) return false;
    if (json.compare(at, 4, "true") == 0) { value = true; return true; }
    if (json.compare(at, 5, "false") == 0) { value = false; return true; }
    return false;
}
}

extern "C" bool StormMetalTelemetryEnabled() { return outputPath() != nullptr; }
extern "C" void StormMetalTelemetryMode(const char* mode, const char* group) { if (!outputPath()) return; state.mode = mode ? mode : "unknown"; state.controlGroup = group ? group : ""; state.controls.clear(); }
extern "C" void StormMetalTelemetryControl(const char* action, int controlState, float value) { if (outputPath() && action) state.controls.push_back({action, controlState, value}); }
extern "C" void StormMetalTelemetryCamera(const float* view, const float* projection,
                                         const float* origin, const float* viewport,
                                         bool sceneBoundary) {
    if (!outputPath() || state.cameraValid || !view || !projection || !origin || !viewport) return;
    for (unsigned i=0; i<16; ++i) if (!std::isfinite(view[i]) || !std::isfinite(projection[i])) return;
    for (unsigned i=0; i<3; ++i) if (!std::isfinite(origin[i])) return;
    for (unsigned i=0; i<4; ++i) if (!std::isfinite(viewport[i])) return;
    if (viewport[2] <= 0 || viewport[3] <= 0) return;
    std::memcpy(state.view.data(), view, sizeof(float) * 16);
    std::memcpy(state.projection.data(), projection, sizeof(float) * 16);
    std::memcpy(state.worldOrigin.data(), origin, sizeof(float) * 3);
    std::memcpy(state.viewport.data(), viewport, sizeof(float) * 4);
    state.cameraSceneBoundary = sceneBoundary;
    state.cameraValid = true;
}
extern "C" void StormMetalTelemetryDeck(const float* from, const float* desired, const float* resolved, bool walkable, bool adjusted, const char* rejection) { if (!outputPath() || !from || !desired || !resolved) return; std::memcpy(state.deckFrom.data(), from, 12); std::memcpy(state.deckDesired.data(), desired, 12); std::memcpy(state.deckResolved.data(), resolved, 12); state.canWalkTo = walkable; state.crewAdjusted = adjusted; state.deckRejection = rejection ? rejection : ""; state.deckValid = true; }
extern "C" void StormMetalTelemetryBeginLights(unsigned count, std::uint64_t selected) { if (!outputPath()) return; state.catalogCount = count; state.selectedId = selected; state.lights.clear(); }
extern "C" void StormMetalTelemetryLight(unsigned index, std::uint64_t id, const float* pr, const float* color, bool selected, bool shadowed) { if (!outputPath() || !pr || !color) return; Light light; light.index=index; light.id=id; std::memcpy(light.positionRange.data(),pr,16); std::memcpy(light.color.data(),color,16); light.selected=selected; light.shadowed=shadowed; state.lights.push_back(light); }
extern "C" void StormMetalTelemetryShadows(unsigned draws, unsigned vertices, unsigned receivers, unsigned casterRejects, unsigned receiverRejects, unsigned requested, unsigned complete, bool failed, bool applied) { if (!outputPath()) return; state.casterDraws=draws; state.casterVertices=vertices; state.receiverDraws=receivers; state.casterRejects=casterRejects; state.receiverRejects=receiverRejects; state.requestedLamps=requested; state.completeLamps=complete; state.shadowFailed=failed; state.shadowApplied=applied; }
extern "C" int StormMetalTelemetryToggle(const char* name, int fallback) { if (!outputPath() || !name) return fallback; std::ifstream input(controlPath()); if (!input) return fallback; std::string json((std::istreambuf_iterator<char>(input)), {}); bool value=false; return namedBool(json,name,value) ? int(value) : fallback; }
extern "C" void StormMetalTelemetryPublish(std::uint64_t frame) {
    const char* path = outputPath(); if (!path) return;
    const bool cameraValid = state.cameraValid;
    state.cameraValid = false; // Never carry an earlier frame's camera through an empty frame or failed write.
    std::ostringstream out; out << "{\"schema\":1,\"pid\":" << getpid() << ",\"frame\":" << frame << ",\"mode\":\"" << escaped(state.mode) << "\",\"controlGroup\":\"" << escaped(state.controlGroup) << "\",\"controls\":[";
    for (size_t i=0;i<state.controls.size();++i) { if(i)out<<','; const auto& c=state.controls[i]; out << "{\"action\":\""<<escaped(c.action)<<"\",\"state\":"<<c.state<<",\"value\":"<<(std::isfinite(c.value)?c.value:0.f)<<'}'; }
    out << "],\"camera\":{"; if(cameraValid){out<<"\"view\":";vectorJson(out,state.view);out<<",\"projection\":";vectorJson(out,state.projection);out<<",\"worldOrigin\":";vectorJson(out,state.worldOrigin);out<<",\"viewport\":";vectorJson(out,state.viewport);out<<",\"source\":\""<<(state.cameraSceneBoundary?"scene_boundary":"present_fallback")<<'"';} out << "},\"deck\":{";
    if(state.deckValid){out<<"\"from\":";vectorJson(out,state.deckFrom);out<<",\"desired\":";vectorJson(out,state.deckDesired);out<<",\"resolved\":";vectorJson(out,state.deckResolved);out<<",\"canWalkTo\":"<<(state.canWalkTo?"true":"false")<<",\"crewAdjusted\":"<<(state.crewAdjusted?"true":"false")<<",\"rejection\":\""<<escaped(state.deckRejection)<<'"';} out << "},\"lights\":{\"catalogCount\":"<<state.catalogCount<<",\"selectedId\":"<<state.selectedId<<",\"items\":[";
    for(size_t i=0;i<state.lights.size();++i){if(i)out<<',';const auto& l=state.lights[i];out<<"{\"index\":"<<l.index<<",\"id\":"<<l.id<<",\"positionRange\":";vectorJson(out,l.positionRange);out<<",\"color\":";vectorJson(out,l.color);out<<",\"selected\":"<<(l.selected?"true":"false")<<",\"shadowed\":"<<(l.shadowed?"true":"false")<<'}';}
    out << "]},\"shadows\":{\"casterDraws\":"<<state.casterDraws<<",\"casterVertices\":"<<state.casterVertices<<",\"receiverDraws\":"<<state.receiverDraws<<",\"casterRejects\":"<<state.casterRejects<<",\"receiverRejects\":"<<state.receiverRejects<<",\"requestedLamps\":"<<state.requestedLamps<<",\"completeLamps\":"<<state.completeLamps<<",\"failed\":"<<(state.shadowFailed?"true":"false")<<",\"applied\":"<<(state.shadowApplied?"true":"false")<<"}}\n";
    const std::string temp = std::string(path) + ".tmp." + std::to_string(getpid());
    { std::ofstream file(temp, std::ios::binary | std::ios::trunc); if (!file) return; file << out.str(); if (!file) return; }
    if (std::rename(temp.c_str(), path) != 0) std::remove(temp.c_str());
}
