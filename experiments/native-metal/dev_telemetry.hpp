#pragma once

#include <cstdint>

// Local development diagnostics. Every entry point is a no-op unless
// STORM_METAL_DEV_TELEMETRY names an output JSON file.
extern "C" {
bool StormMetalTelemetryEnabled();
void StormMetalTelemetryMode(const char* mode, const char* controlGroup);
void StormMetalTelemetryControl(const char* action, int state, float value);
// Preserve the first scene camera until publication; Present is only a fallback.
void StormMetalTelemetryCamera(const float* viewMatrix16, const float* projectionMatrix16,
                               const float* worldOrigin3, const float* viewport4,
                               bool sceneBoundary);
void StormMetalTelemetryDeck(const float* from3, const float* desired3, const float* resolved3,
                             bool canWalkTo, bool crewAdjusted, const char* rejection);
void StormMetalTelemetryBeginLights(unsigned catalogCount, std::uint64_t selectedId);
void StormMetalTelemetryLight(unsigned index, std::uint64_t id, const float* positionRange4,
                              const float* color4, bool selected, bool shadowed);
void StormMetalTelemetryShadows(unsigned casterDraws, unsigned casterVertices,
                                unsigned receiverDraws, unsigned casterRejects,
                                unsigned receiverRejects, unsigned requestedLamps,
                                unsigned completeLamps, bool failed, bool applied);
int StormMetalTelemetryToggle(const char* name, int fallback);
void StormMetalTelemetryPublish(std::uint64_t frame);
}
