#pragma once

#include "render_resource_api.hpp"
#include "render_state_values.hpp"

namespace storm_render {

// SDL owns the window/fullscreen lifecycle. The backend owns these drawable
// resources; presentation never enumerates fake adapters or vertex processors.
struct Presentation {
    std::uint32_t width = 0, height = 0;
    AssetFormat colorFormat = AssetFormat::A8R8G8B8;
    AssetFormat depthFormat = AssetFormat::D24S8;
    bool depthEnabled = false, vSync = true;
};
struct Capabilities {
    std::uint32_t MaxActiveLights = 8;
    bool mipCubeMap = true;
};
Result importProgram(ProgramStage stage, const std::uint32_t *legacyAsset, Program **out);

// Resource IO and imported material state are logical engine operations.
// Platform factories, COM identity, shared handles and unsupported SDK methods
// do not belong to this contract. Metal implementation and frame lifetime stay
// with the one native backend.
class Device : public RefCounted {
public:
    virtual Result GetStatus() = 0;
    virtual Result GetCapabilities(Capabilities *out) = 0;
    virtual Result Reset(const Presentation *presentation) = 0;
    virtual Result BeginScene() = 0;
    virtual Result EndScene() = 0;
    virtual Result Present() = 0;
    virtual Result Clear(std::uint32_t flags, PackedColor color, float depth) = 0;

    virtual Result CreateTexture(std::uint32_t width, std::uint32_t height,
        std::uint32_t levels, Usage usage, AssetFormat format, Texture **out) = 0;
    virtual Result CreateCubeTexture(std::uint32_t edge, std::uint32_t levels,
        Usage usage, AssetFormat format, CubeTexture **out) = 0;
    virtual Result CreateVolumeTexture(std::uint32_t width, std::uint32_t height,
        std::uint32_t depth, std::uint32_t levels, Usage usage,
        AssetFormat format, VolumeTexture **out) = 0;
    virtual Result CreateVertexBuffer(std::uint32_t bytes, Usage usage,
        std::uint32_t vertexFormat, VertexBuffer **out) = 0;
    virtual Result CreateIndexBuffer(std::uint32_t bytes, Usage usage,
        IndexFormat format, IndexBuffer **out) = 0;
    virtual Result CreateRenderTarget(std::uint32_t width, std::uint32_t height,
        AssetFormat format, Surface **out) = 0;
    virtual Result CreateDepthStencilSurface(std::uint32_t width, std::uint32_t height,
        AssetFormat format, Surface **out) = 0;
    virtual Result CreateOffscreenPlainSurface(std::uint32_t width, std::uint32_t height,
        AssetFormat format, Surface **out) = 0;
    virtual Result GetRenderTargetData(Surface *source, Surface *destination) = 0;
    virtual Result StretchRect(Surface *source, const Rect *sourceRegion,
        Surface *destination, const Rect *destinationRegion, TextureFilter filter) = 0;
    virtual Result UpdateSurface(Surface *source, const Rect *sourceRegion,
        Surface *destination, const Point *destinationOrigin) = 0;
    virtual Result GetRenderTarget(Surface **out) = 0;
    virtual Result SetRenderTarget(Surface *surface) = 0;
    virtual Result GetDepthStencilSurface(Surface **out) = 0;
    virtual Result SetDepthStencilSurface(Surface *surface) = 0;

    virtual Result SetTransform(TransformSlot slot, const Matrix *matrix) = 0;
    virtual Result GetTransform(TransformSlot slot, Matrix *matrix) = 0;
    virtual Result SetViewport(const Viewport *viewport) = 0;
    virtual Result GetViewport(Viewport *viewport) = 0;
    virtual Result SetRenderState(RenderStateField field, std::uint32_t value) = 0;
    virtual Result GetRenderState(RenderStateField field, std::uint32_t *value) = 0;
    virtual Result SetTextureStageState(std::uint32_t stage, StageField field,
        std::uint32_t value) = 0;
    virtual Result GetTextureStageState(std::uint32_t stage, StageField field,
        std::uint32_t *value) = 0;
    virtual Result SetSamplerState(std::uint32_t stage, SamplerField field,
        std::uint32_t value) = 0;
    virtual Result GetSamplerState(std::uint32_t stage, SamplerField field,
        std::uint32_t *value) = 0;
    virtual Result SetTexture(std::uint32_t stage, BaseTexture *texture) = 0;
    virtual Result GetTexture(std::uint32_t stage, BaseTexture **out) = 0;
    virtual Result SetMaterial(const Material *material) = 0;
    virtual Result GetMaterial(Material *material) = 0;
    virtual Result SetLight(std::uint32_t slot, const Light *light) = 0;
    virtual Result GetLight(std::uint32_t slot, Light *light) = 0;
    virtual Result LightEnable(std::uint32_t slot, bool enabled) = 0;
    virtual Result GetLightEnable(std::uint32_t slot, bool *enabled) = 0;
    virtual Result SetFVF(std::uint32_t vertexFormat) = 0;
    virtual Result GetFVF(std::uint32_t *vertexFormat) = 0;
    virtual Result SetStreamSource(std::uint32_t stream, VertexBuffer *buffer,
        std::uint32_t offset, std::uint32_t stride) = 0;
    virtual Result GetStreamSource(std::uint32_t stream, VertexBuffer **out,
        std::uint32_t *offset, std::uint32_t *stride) = 0;
    virtual Result SetIndices(IndexBuffer *buffer) = 0;
    virtual Result CreateVertexDeclaration(const VertexElement *elements,
        VertexLayout **out) = 0;
    virtual Result SetVertexDeclaration(VertexLayout *layout) = 0;
    virtual Result SetProgram(ProgramStage stage, Program *program) = 0;
    virtual Result GetProgram(ProgramStage stage, Program **out) = 0;
    Result ImportVertexProgram(const std::uint32_t *asset, Program **out) {
        return importProgram(ProgramStage::Vertex, asset, out);
    }
    Result ImportFragmentProgram(const std::uint32_t *asset, Program **out) {
        return importProgram(ProgramStage::Fragment, asset, out);
    }
    Result BindVertexProgram(Program *program) { return SetProgram(ProgramStage::Vertex, program); }
    Result BindFragmentProgram(Program *program) { return SetProgram(ProgramStage::Fragment, program); }
    Result GetVertexProgram(Program **out) { return GetProgram(ProgramStage::Vertex, out); }
    Result GetFragmentProgram(Program **out) { return GetProgram(ProgramStage::Fragment, out); }
    virtual Result SetVertexShaderConstantF(std::uint32_t first, const float *values,
        std::uint32_t count) = 0;
    virtual Result SetPixelShaderConstantF(std::uint32_t first, const float *values,
        std::uint32_t count) = 0;
    virtual Result GetVertexShaderConstantF(std::uint32_t first, float *values,
        std::uint32_t count) = 0;
    virtual Result GetPixelShaderConstantF(std::uint32_t first, float *values,
        std::uint32_t count) = 0;
    virtual Result DrawPrimitive(Primitive primitive, std::uint32_t first,
        std::uint32_t count) = 0;
    virtual Result DrawIndexedPrimitive(Primitive primitive, std::int32_t base,
        std::uint32_t minimum, std::uint32_t vertices, std::uint32_t firstIndex,
        std::uint32_t count) = 0;
    virtual Result DrawPrimitiveUP(Primitive primitive, std::uint32_t count,
        const void *vertices, std::uint32_t stride) = 0;
    virtual Result DrawIndexedPrimitiveUP(Primitive primitive, std::uint32_t minimum,
        std::uint32_t vertices, std::uint32_t count, const void *indices,
        IndexFormat indexFormat, const void *vertexBytes, std::uint32_t stride) = 0;
};

Result createMetalDevice(void *sdlWindow, const Presentation *presentation, Device **out);
enum class SceneBoundary { Effects, Hud };
void markSceneBoundary(Device *device, SceneBoundary boundary);
void markSceneBoundary(SceneBoundary boundary);

} // namespace storm_render
