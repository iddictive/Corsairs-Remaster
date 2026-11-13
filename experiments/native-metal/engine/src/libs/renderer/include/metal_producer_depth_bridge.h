#pragma once

#ifdef STORM_METAL_RENDERER_CONSUMERS
// The query invalidates any pending publication and snapshots the draw target.
// Matrices are borrowed for the duration of the synchronous calls.
extern "C" bool StormMetalGetProducerPhysicalProjection(void *device, float projection[16]);

enum StormMetalDepthProducer : unsigned
{
    StormMetalDepthGrass = 1,   // View * projection, Grass vc[32..35] layout.
    StormMetalDepthCaustic = 2, // Transposed world * view * projection, vc[0..3] layout.
    StormMetalDepthWorldMapCloud = 3, // Projection, WdmClouds vc[0..3] layout.
};

// The backend consumes this once, only for the matching producer and target;
// failed queries/publications and mismatched draws must never reuse old data.
extern "C" bool StormMetalSetProducerDrawMatrices(void *device, unsigned producer,
                                                   const float physical[16], const float logical[16]);
#endif
