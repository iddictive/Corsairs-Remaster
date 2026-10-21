#include "dev_telemetry.hpp"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    setenv("STORM_METAL_DEV_TELEMETRY", argv[1], 1);
    StormMetalTelemetryMode("deck", "Sailing1Pers");
    StormMetalTelemetryControl("DeckWalk_Forward", 2, 1.f);
    const float view[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 3,4,5,1};
    const float origin[3] = {-100,0,20}, from[3] = {0,1,0};
    const float desired[3] = {0,1,1}, resolved[3] = {0,1,.4f};
    StormMetalTelemetryCamera(view, origin);
    StormMetalTelemetryDeck(from, desired, resolved, true, true, "crew_adjusted");
    const float positionRange[4] = {1,2,3,4}, color[4] = {1,.5f,.2f,1};
    StormMetalTelemetryBeginLights(1, 7);
    StormMetalTelemetryLight(0, 7, positionRange, color, true, true);
    StormMetalTelemetryShadows(3, 90, 2, 0, 1, 1, 1, false, true);
    StormMetalTelemetryPublish(42);
    std::ifstream input(argv[1]);
    std::string json((std::istreambuf_iterator<char>(input)), {});
    const char* required[] = {"\"frame\":42", "\"DeckWalk_Forward\"", "\"crew_adjusted\"",
                              "\"selectedId\":7", "\"receiverRejects\":1"};
    for (const char* token : required) if (json.find(token) == std::string::npos) return 1;
    std::cout << "PASS dev telemetry atomic snapshot\n";
}
