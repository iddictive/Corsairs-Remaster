#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#include <simd/simd.h>
#include <vector>
#include <stdexcept>
struct Uniform { simd_float4 centerScale; simd_float4 view; };
struct Batch { NSUInteger start,count; id<MTLTexture> texture; };
static NSString *shader = @"#include <metal_stdlib>\nusing namespace metal;\nstruct V{packed_float3 p;packed_float3 n;packed_float2 uv;};struct U{float4 cs;float4 view;};struct O{float4 p[[position]];float2 uv;float3 n;};vertex O vs(uint i[[vertex_id]],const device V*v[[buffer(0)]],constant U&u[[buffer(1)]]){float3 p=(float3(v[i].p)-u.cs.xyz)*u.cs.w;float a=u.view.x,c=cos(a),s=sin(a);float3 q=float3(c*p.x+s*p.z,p.y,-s*p.x+c*p.z);float y=q.y*.94-q.z*.342,z=q.y*.342+q.z*.94;float d=3.4-z;O o;o.p=float4(q.x*2.1/u.view.y,y*2.1,(d-.1)*10./9.9,d);o.uv=v[i].uv;float3 n=v[i].n;o.n=float3(c*n.x+s*n.z,n.y,-s*n.x+c*n.z);return o;}fragment float4 fs(O o[[stage_in]],texture2d<float> t[[texture(0)]]){constexpr sampler s(address::repeat,filter::linear);float4 c=t.sample(s,o.uv);if(c.a<.2)discard_fragment();float l=.4+.6*abs(dot(normalize(o.n),normalize(float3(.4,.8,.5))));return float4(c.rgb*l,1);}";
@interface Renderer : NSObject<MTKViewDelegate,NSWindowDelegate> {
 id<MTLDevice> device; id<MTLCommandQueue> queue; id<MTLRenderPipelineState> pipeline; id<MTLDepthStencilState> depth;
 id<MTLBuffer> vertices; std::vector<Batch> batches; Uniform uniform; double angle;
}
-(instancetype)initWithView:(MTKView*)view scene:(NSString*)folder;
@end
@implementation Renderer
-(instancetype)initWithView:(MTKView*)view scene:(NSString*)folder {
 if(!(self=[super init]))return nil;
 device=view.device; queue=[device newCommandQueue]; NSError *error=nil;
 auto lib=[device newLibraryWithSource:shader options:nil error:&error];
 if(!lib) { NSLog(@"%@",error); exit(2); }
 auto desc=[MTLRenderPipelineDescriptor new];desc.vertexFunction=[lib newFunctionWithName:@"vs"];desc.fragmentFunction=[lib newFunctionWithName:@"fs"];
 desc.colorAttachments[0].pixelFormat=view.colorPixelFormat;desc.depthAttachmentPixelFormat=view.depthStencilPixelFormat;
 pipeline=[device newRenderPipelineStateWithDescriptor:desc error:&error];if(!pipeline){NSLog(@"%@",error);exit(2);}
 auto dd=[MTLDepthStencilDescriptor new];dd.depthCompareFunction=MTLCompareFunctionLess;dd.depthWriteEnabled=YES;depth=[device newDepthStencilStateWithDescriptor:dd];
 NSData *json=[NSData dataWithContentsOfFile:[folder stringByAppendingPathComponent:@"scene.json"]];
 NSDictionary *scene=json?[NSJSONSerialization JSONObjectWithData:json options:0 error:&error]:nil;
 NSData *mesh=[NSData dataWithContentsOfFile:[folder stringByAppendingPathComponent:@"vertices.bin"]];
 if(!scene || !mesh.length){NSLog(@"Missing scene/vertices");exit(2);}
 vertices=[device newBufferWithBytes:mesh.bytes length:mesh.length options:MTLResourceStorageModeShared];
 NSArray *center=scene[@"center"];uniform.centerScale={(float)[center[0] doubleValue],(float)[center[1] doubleValue],(float)[center[2] doubleValue],[scene[@"scale"] floatValue]};
 NSMutableDictionary *cache=[NSMutableDictionary new];
 for(NSDictionary *draw in scene[@"draws"]){
 NSString *path=draw[@"texture"];id<MTLTexture> tex=cache[path];
 if(!tex){NSData *tx=[NSData dataWithContentsOfFile:path];if(tx.length<24){NSLog(@"Invalid TX %@",path);exit(2);}int32_t h[6];memcpy(h,tx.bytes,24);
 MTLPixelFormat fmt=MTLPixelFormatInvalid;NSUInteger row=0;
 if(h[4]==0x31545844){fmt=MTLPixelFormatBC1_RGBA;row=((h[1]+3)/4)*8;}
 if(h[4]==0x33545844){fmt=MTLPixelFormatBC2_RGBA;row=((h[1]+3)/4)*16;}
 if(h[4]==0x35545844){fmt=MTLPixelFormatBC3_RGBA;row=((h[1]+3)/4)*16;}
 if(h[4]==21 || h[4]==22){fmt=MTLPixelFormatBGRA8Unorm;row=h[1]*4;}
 NSUInteger rows=(h[4]==21||h[4]==22)?h[2]:(h[2]+3)/4;
 if(h[0]!=0||h[1]<=0||h[2]<=0||fmt==MTLPixelFormatInvalid||tx.length<24+row*rows){NSLog(@"Unsupported TX %@ format %x",path,h[4]);exit(2);}
 auto td=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:fmt width:h[1] height:h[2] mipmapped:NO];td.storageMode=MTLStorageModeShared;
 tex=[device newTextureWithDescriptor:td];if(!tex){NSLog(@"Texture creation failed %@",path);exit(2);}
 [tex replaceRegion:MTLRegionMake2D(0,0,h[1],h[2]) mipmapLevel:0 withBytes:(const char*)tx.bytes+24 bytesPerRow:row];cache[path]=tex;
 }
 NSUInteger start=[draw[@"start"] unsignedIntegerValue], count=[draw[@"count"] unsignedIntegerValue];if((start+count)*32>mesh.length)exit(2);
 batches.push_back({start,count,tex});
 }
 NSLog(@"Metal device: %@; draws: %lu",device.name,batches.size());return self;
}
-(void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {}
-(void)drawInMTKView:(MTKView*)view {
 auto drawable=view.currentDrawable;auto pass=view.currentRenderPassDescriptor;if(!drawable||!pass)return;
 uniform.view={(float)angle,(float)(view.drawableSize.width/view.drawableSize.height),0,0};
 auto command=[queue commandBuffer];auto enc=[command renderCommandEncoderWithDescriptor:pass];
 [enc setRenderPipelineState:pipeline];[enc setDepthStencilState:depth];[enc setCullMode:MTLCullModeNone];[enc setVertexBuffer:vertices offset:0 atIndex:0];[enc setVertexBytes:&uniform length:sizeof(uniform) atIndex:1];
 for(auto &b:batches){[enc setFragmentTexture:b.texture atIndex:0];[enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:b.start vertexCount:b.count];}
 [enc endEncoding];[command presentDrawable:drawable];[command commit];angle+=.003;
}
-(BOOL)windowShouldClose:(NSWindow*)sender {[NSApp terminate:nil];return YES;}
@end
int main(int argc,const char**argv){@autoreleasepool{
 if(argc!=2){fprintf(stderr,"usage: metal-probe SCENE_DIRECTORY\n");return 2;}
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
 id<MTLDevice> device=MTLCreateSystemDefaultDevice();if(!device)return 2;
 auto window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,1100,760) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
 window.title=@"Storm · native Metal probe · static model";
 auto view=[[MTKView alloc]initWithFrame:window.contentView.bounds device:device];view.colorPixelFormat=MTLPixelFormatBGRA8Unorm;view.depthStencilPixelFormat=MTLPixelFormatDepth32Float;view.clearColor=MTLClearColorMake(.055,.065,.08,1);view.preferredFramesPerSecond=60;
 auto renderer=[[Renderer alloc]initWithView:view scene:[NSString stringWithUTF8String:argv[1]]];view.delegate=renderer;window.delegate=renderer;window.contentView=view;
 [window center];[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];[NSApp run];
}}
