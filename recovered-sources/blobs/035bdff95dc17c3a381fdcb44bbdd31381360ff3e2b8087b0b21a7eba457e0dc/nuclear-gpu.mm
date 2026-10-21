#include "phonk_ordnance_render.hpp"
#include <cassert>
#include <fstream>
#include <vector>

int main(int argc, char **argv) { @autoreleasepool {
    assert(argc == 2);
    auto device = MTLCreateSystemDefaultDevice(); assert(device);
    auto queue = [device newCommandQueue];
    auto colorDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm width:256 height:256 mipmapped:NO];
    colorDesc.storageMode = MTLStorageModeShared; colorDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    auto color = [device newTextureWithDescriptor:colorDesc];
    auto depthDesc = [MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:256 height:256 mipmapped:NO];
    depthDesc.storageMode = MTLStorageModePrivate; depthDesc.usage = MTLTextureUsageRenderTarget | MTLTextureUsageShaderRead;
    auto depth = [device newTextureWithDescriptor:depthDesc]; assert(color && depth);
    simd_float4x4 projection{};
    projection.columns[0] = {1,0,0,0}; projection.columns[1] = {0,1,0,0};
    projection.columns[2] = {0,0,10000.f/9999.9f,1}; projection.columns[3] = {0,0,-1000.f/9999.9f,0};
    storm_metal::PhonkNuclearRenderer renderer;
    const float center[3] = {0,-60,600};
    std::vector<uint32_t> baseline, visible;
    const auto run = [&](float age, double clearDepth, bool draw) {
        auto command = [queue commandBuffer];
        auto pass = [MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture = color; pass.colorAttachments[0].loadAction = MTLLoadActionClear;
        pass.colorAttachments[0].storeAction = MTLStoreActionStore; pass.colorAttachments[0].clearColor = MTLClearColorMake(.05,.10,.18,1);
        pass.depthAttachment.texture = depth; pass.depthAttachment.loadAction = MTLLoadActionClear;
        pass.depthAttachment.storeAction = MTLStoreActionStore; pass.depthAttachment.clearDepth = clearDepth;
        auto encoder = [command renderCommandEncoderWithDescriptor:pass]; [encoder endEncoding];
        if (draw) assert(renderer.encode(device, command, color, depth, matrix_identity_float4x4,
            projection, {0,0,0}, {0,0,256,256}, center, 80, age));
        [command commit]; [command waitUntilCompleted]; assert(command.status == MTLCommandBufferStatusCompleted);
        std::vector<uint32_t> pixels(256*256); [color getBytes:pixels.data() bytesPerRow:256*4 fromRegion:MTLRegionMake2D(0,0,256,256) mipmapLevel:0];
        return pixels;
    };
    baseline = run(0,1,false); visible = run(1.5,1,true);
    size_t changed = 0; for (size_t n=0;n<visible.size();++n) changed += visible[n] != baseline[n];
    assert(changed > 1000 && changed < visible.size());
    std::ofstream image(argv[1],std::ios::binary); image << "P6\n256 256\n255\n";
    for (auto pixel:visible) { const char rgb[3] = {char(pixel>>16),char(pixel>>8),char(pixel)}; image.write(rgb,3); }
    const auto clip = simd_mul(projection,simd_make_float4(0,0,300,1));
    const auto hidden = run(1.5,clip.z/clip.w,true); assert(hidden == baseline);
    const auto early = run(.1,1,true), late = run(7.8,1,true); assert(early != baseline && late != baseline && early != late);
    std::printf("PASS actual Metal library/pipeline; visible=%zu; foreground occlusion exact; early/late distinct\n",changed);
} }
