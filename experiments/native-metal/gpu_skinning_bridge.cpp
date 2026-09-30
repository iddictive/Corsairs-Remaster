#include "gpu_skinning_bridge.hpp"

#include <atomic>

namespace storm::metal::skinning {
namespace {
std::atomic<const Provider *> installed{nullptr};
thread_local const Provider *active = nullptr;
}

void registerProvider(const Provider *provider)
{
    installed.store(provider, std::memory_order_release);
}

bool acceptVertices(const AnimatedVertex *vertices, std::uint32_t totalVertices, std::uint32_t boneCount)
{
    const Provider *candidate = installed.load(std::memory_order_acquire);
    return candidate && candidate->acceptVertices && vertices && totalVertices && boneCount && boneCount <= 256 &&
           candidate->acceptVertices(candidate->context, vertices, totalVertices, boneCount);
}

bool beginPose(void *device, const Matrix4x4 *palette, std::uint32_t boneCount)
{
    const Provider *candidate = installed.load(std::memory_order_acquire);
    if (active || !candidate || !candidate->acceptVertices || !candidate->beginPose || !candidate->bindVertices ||
        !candidate->endPose || !palette ||
        boneCount == 0 || boneCount > 256 || !candidate->beginPose(candidate->context, device, palette, boneCount))
        return false;
    active = candidate;
    return true;
}

void *bindVertices(const AnimatedVertex *vertices, std::uint32_t firstVertex,
                   std::uint32_t vertexCount, std::uint32_t totalVertices)
{
    return active ? active->bindVertices(active->context, vertices, firstVertex, vertexCount, totalVertices) : nullptr;
}

void endPose()
{
    if (!active)
        return;
    const Provider *provider = active;
    active = nullptr;
    provider->endPose(provider->context);
}

} // namespace storm::metal::skinning
