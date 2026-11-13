#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>

// Isolated 1-pixel Metal depth replay. No engine, window, or game process.
struct Params { float z, n, f, reversed, red; };
static void require(bool ok, const char *what) { if (!ok) { std::fprintf(stderr, "%s\n", what); std::exit(1); } }
int main() { @autoreleasepool {
    id<MTLDevice> dev = MTLCreateSystemDefaultDevice(); require(dev != nil, "Metal unavailable");
    id<MTLCommandQueue> queue = [dev newCommandQueue];
    NSString *source = @"#include <metal_stdlib>\nusing namespace metal;\n"
    "struct P {float z,n,f,reversed,red;}; struct O {float4 p [[position]]; float4 color;};\n"
    "vertex O vs(uint i [[vertex_id]], constant P& a [[buffer(0)]]) {"
    "float2 xy[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};"
    "float q=a.reversed ? a.n/(a.n-a.f) : a.f/(a.f-a.n);"
    "float b=a.reversed ? a.n*a.f/(a.f-a.n) : -q*a.n;"
    "O o; o.p=float4(xy[i]*a.z, q*a.z+b, a.z);"
    "o.color=a.red>0.5?float4(1,0,0,1):float4(0,0,1,1); return o;}\n"
    "fragment float4 fs(O a [[stage_in]]) {return a.color;}";
    NSError *error = nil; id<MTLLibrary> lib = [dev newLibraryWithSource:source options:nil error:&error];
    if (!lib) { std::fprintf(stderr, "%s\n", error.localizedDescription.UTF8String); return 1; }
    MTLRenderPipelineDescriptor *pd = [MTLRenderPipelineDescriptor new];
    pd.vertexFunction=[lib newFunctionWithName:@"vs"]; pd.fragmentFunction=[lib newFunctionWithName:@"fs"];
    pd.colorAttachments[0].pixelFormat=MTLPixelFormatRGBA8Unorm; pd.depthAttachmentPixelFormat=MTLPixelFormatDepth32Float;
    id<MTLRenderPipelineState> pipe=[dev newRenderPipelineStateWithDescriptor:pd error:&error]; require(pipe != nil, "pipeline failed");
    MTLTextureDescriptor *td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:1 height:1 mipmapped:NO];
    td.usage=MTLTextureUsageRenderTarget; td.storageMode=MTLStorageModeShared; id<MTLTexture> color=[dev newTextureWithDescriptor:td];
    td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatDepth32Float width:1 height:1 mipmapped:NO];
    td.usage=MTLTextureUsageRenderTarget; td.storageMode=MTLStorageModePrivate; id<MTLTexture> depth=[dev newTextureWithDescriptor:td];
    require(color && depth, "texture failed");
    auto run = [&](bool reverse, float front, float separation, float travel, bool backLast, bool islandNearOverride = false) {
        MTLDepthStencilDescriptor *dd=[MTLDepthStencilDescriptor new]; dd.depthWriteEnabled=YES;
        dd.depthCompareFunction=reverse ? MTLCompareFunctionGreaterEqual : MTLCompareFunctionLessEqual;
        id<MTLDepthStencilState> state=[dev newDepthStencilStateWithDescriptor:dd];
        MTLRenderPassDescriptor *pass=[MTLRenderPassDescriptor renderPassDescriptor];
        pass.colorAttachments[0].texture=color; pass.colorAttachments[0].loadAction=MTLLoadActionClear; pass.colorAttachments[0].storeAction=MTLStoreActionStore;
        pass.depthAttachment.texture=depth; pass.depthAttachment.loadAction=MTLLoadActionClear; pass.depthAttachment.storeAction=MTLStoreActionDontCare; pass.depthAttachment.clearDepth=reverse?0:1;
        id<MTLCommandBuffer> cb=[queue commandBuffer]; id<MTLRenderCommandEncoder> e=[cb renderCommandEncoderWithDescriptor:pass];
        [e setRenderPipelineState:pipe]; [e setDepthStencilState:state];
        for (int i=0;i<2;i++) { bool isFront = backLast ? i==0 : i==1;
            Params p{front-travel+(isFront?0.f:separation),(!isFront && islandNearOverride)?10.f:.115f,32000.f,reverse?1.f:0.f,isFront?1.f:0.f};
            [e setVertexBytes:&p length:sizeof(p) atIndex:0]; [e drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:3]; }
        [e endEncoding]; [cb commit]; [cb waitUntilCompleted]; require(cb.status == MTLCommandBufferStatusCompleted, "GPU command failed");
        unsigned char pixel[4]{}; [color getBytes:pixel bytesPerRow:4 fromRegion:MTLRegionMake2D(0,0,1,1) mipmapLevel:0];
        require(pixel[0]>240 || pixel[2]>240, "no visible surface"); return pixel[0]>240;
    };
    std::printf("device=%s near=.115 far=32000 format=Depth32Float frames=101 travel_step=.01m\n", dev.name.UTF8String);
    for (auto scenario : {0,1,2}) for (bool reverse : {false,true}) {
        float front=scenario==0?5.f:scenario==1?2600.f:5000.f;
        float gap=scenario==0?.01f:scenario==1?.2f:1.f;
        int wrong=0,changed=0,order=0,previous=-1;
        for (int i=0;i<=100;i++) { float travel=float(i)*.01f; bool a=run(reverse,front,gap,travel,true); bool b=run(reverse,front,gap,travel,false);
            wrong+=!a; order+=a!=b; if(previous>=0 && previous!=int(a)) changed++; previous=a; }
        std::printf("mode=%s front=%.2f separation=%.2f incorrect=%d order_disagreements=%d temporal_flips=%d\n",reverse?"reverse":"standard",front,gap,wrong,order,changed);
    }
    bool mixed=run(false,1000.f,1600.f,0.f,true,true);
    std::printf("mixed_near front=1000 island=2600 front_visible=%s\n",mixed?"yes":"no");
} }
