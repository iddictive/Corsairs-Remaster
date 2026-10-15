#pragma once

#include "render_value_types.hpp"

namespace storm_render {

class Device;

// The render thread owns reference transitions, matching resource bindings and
// the device's child-drain lifetime. Resource payload revisions remain backend-owned.
class RefCounted {
public:
    std::uint32_t refs = 1;
    RefCounted() = default;
    RefCounted(const RefCounted &) = delete;
    RefCounted &operator=(const RefCounted &) = delete;
    virtual ~RefCounted() = default;
    virtual std::uint32_t AddRef() { return ++refs; }
    virtual std::uint32_t Release() {
        const auto remaining = --refs;
        if (!remaining) delete this;
        return remaining;
    }
};

class Resource : public RefCounted {
public:
    // Successful object getters return one retained reference. Null output or
    // a missing device is invalidCall; callers release each successful result.
    virtual Result GetDevice(Device **out) = 0;
    virtual ResourceKind GetType() = 0;
};

class Surface : public Resource {
public:
    virtual Result GetDesc(SurfaceDesc *out) = 0;
    virtual Result LockRect(LockedRect *out, const Rect *region, LockFlags flags) = 0;
    virtual Result UnlockRect() = 0;
};

class BaseTexture : public Resource {
public:
    virtual std::uint32_t GetLevelCount() = 0;
    virtual std::uint32_t SetLOD(std::uint32_t level) = 0;
    virtual std::uint32_t GetLOD() = 0;
};

class Texture : public BaseTexture {
public:
    virtual Result GetLevelDesc(std::uint32_t level, SurfaceDesc *out) = 0;
    virtual Result GetSurfaceLevel(std::uint32_t level, Surface **out) = 0;
    virtual Result LockRect(std::uint32_t level, LockedRect *out,
                            const Rect *region, LockFlags flags) = 0;
    virtual Result UnlockRect(std::uint32_t level) = 0;
};

class CubeTexture : public BaseTexture {
public:
    virtual Result GetLevelDesc(std::uint32_t level, SurfaceDesc *out) = 0;
    virtual Result GetCubeMapSurface(CubeFace face, std::uint32_t level, Surface **out) = 0;
    virtual Result LockRect(CubeFace face, std::uint32_t level, LockedRect *out,
                            const Rect *region, LockFlags flags) = 0;
    virtual Result UnlockRect(CubeFace face, std::uint32_t level) = 0;
};

class Volume : public Resource {
public:
    virtual Result GetDesc(VolumeDesc *out) = 0;
    virtual Result LockBox(LockedBox *out, const Box *region, LockFlags flags) = 0;
    virtual Result UnlockBox() = 0;
};

class VolumeTexture : public BaseTexture {
public:
    virtual Result GetLevelDesc(std::uint32_t level, VolumeDesc *out) = 0;
    virtual Result GetVolumeLevel(std::uint32_t level, Volume **out) = 0;
    virtual Result LockBox(std::uint32_t level, LockedBox *out,
                           const Box *region, LockFlags flags) = 0;
    virtual Result UnlockBox(std::uint32_t level) = 0;
};

class VertexBuffer : public Resource {
public:
    // A zero length maps the remainder. Unlock publishes the backend revision;
    // retained getters/bindings do not create a new payload identity.
    virtual Result Lock(std::uint32_t offset, std::uint32_t length,
                        void **out, LockFlags flags) = 0;
    virtual Result Unlock() = 0;
    virtual Result GetDesc(VertexBufferDesc *out) = 0;
};

class IndexBuffer : public Resource {
public:
    virtual Result Lock(std::uint32_t offset, std::uint32_t length,
                        void **out, LockFlags flags) = 0;
    virtual Result Unlock() = 0;
    virtual Result GetDesc(IndexBufferDesc *out) = 0;
};

class VertexLayout : public RefCounted {
public:
    // With out==nullptr, return the required count (including the terminator).
    // Otherwise count is input capacity; insufficient capacity is invalidCall.
    virtual Result GetDeclaration(VertexElement *out, std::uint32_t *count) = 0;
};

// Loaded techniques identify a supported native program once at the asset
// boundary. A bound program contains no foreign shader bytecode or interface.
class Program : public RefCounted {
public:
    const ProgramStage stage;
    const ProgramKind kind;
    Program(ProgramStage stage, ProgramKind kind) : stage(stage), kind(kind) {}
};

} // namespace storm_render
