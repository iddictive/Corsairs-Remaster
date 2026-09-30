#include "dev_telemetry.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    setenv("STORM_METAL_DEV_TELEMETRY", argv[1], 1);
    StormMetalTelemetryMode("deck", "Sailing1Pers");
    StormMetalTelemetryControl("DeckWalk_Forward", 2, 1.f);
    const float view[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 3,4,5,1};
    const float origin[3] = {-100,0,20}, from[3] = {0,1,0};
    const float desired[3] = {0,1,1}, resolved[3] = {0,1,.4f};
    const float projection[16] = {2,0,0,0, 0,3,0,0, 0,0,1,1, 0,0,-.1f,0};
    const float viewport[4] = {0,0,2056,1329};
    StormMetalTelemetryCamera(view, projection, origin, viewport, true);
    float hudView[16] = {};
    StormMetalTelemetryCamera(hudView, hudView, origin, viewport, false);
    StormMetalTelemetryDeck(from, desired, resolved, true, true, "crew_adjusted");
    const float positionRange[4] = {1,2,3,4}, color[4] = {1,.5f,.2f,1};
    StormMetalTelemetryBeginLights(1, 7);
    StormMetalTelemetryLight(0, 7, positionRange, color, true, true);
    StormMetalTelemetryShadows(3, 90, 2, 0, 1, 1, 1, false, true);
    StormMetalTelemetryPublish(42);
    std::ifstream input(argv[1]);
    std::string json((std::istreambuf_iterator<char>(input)), {});
    const char* required[] = {"\"frame\":42", "\"DeckWalk_Forward\"", "\"crew_adjusted\"",
                              "\"selectedId\":7", "\"receiverRejects\":1",
                              "\"view\":[1,0,0,0,0,1,0,0,0,0,1,0,3,4,5,1]",
                              "\"projection\":[2,0,0,0,0,3,0,0,0,0,1,1,0,0,-0.1,0]",
                              "\"viewport\":[0,0,2056,1329]", "\"source\":\"scene_boundary\""};
    for (const char* token : required) if (json.find(token) == std::string::npos) return 1;
    auto read = [&] { std::ifstream file(argv[1]); return std::string((std::istreambuf_iterator<char>(file)), {}); };
    StormMetalTelemetryPublish(43);
    if (read().find("\"camera\":{}") == std::string::npos) return 1;
    hudView[0] = std::numeric_limits<float>::quiet_NaN();
    StormMetalTelemetryCamera(hudView, projection, origin, viewport, true);
    StormMetalTelemetryPublish(44);
    if (read().find("\"camera\":{}") == std::string::npos) return 1;
    StormMetalTelemetryCamera(view, projection, origin, viewport, false);
    StormMetalTelemetryPublish(45);
    if (read().find("\"source\":\"present_fallback\"") == std::string::npos) return 1;
    std::cout << "PASS dev telemetry atomic camera/projection/viewport snapshot, HUD overwrite, stale-frame and nonfinite negatives\n";
}
