#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>
#include <simd/simd.h>
#include <vector>
#include <stdexcept>
struct Uniform { simd_float4 centerScale; simd_float4 view; simd_float4 options; };
struct Batch { NSUInteger start,count; id<MTLTexture> texture; float kind; };
@interface SceneView : MTKView
@property float yaw;
@property float pitch;
@property float distance;
@property BOOL orbit;
@end
@implementation SceneView
-(BOOL)acceptsFirstResponder { return YES; }
-(void)mouseDragged:(NSEvent*)event { self.orbit=NO; self.yaw+=event.deltaX*.008; self.pitch=fmaxf(-1.3,fminf(1.3,self.pitch+event.deltaY*.008)); }
-(void)scrollWheel:(NSEvent*)event { self.distance=fmaxf(1.2,fminf(12,self.distance*expf(event.scrollingDeltaY*.015))); }
-(void)keyDown:(NSEvent*)event { if([event.charactersIgnoringModifiers isEqualToString:@" "])self.orbit=!self.orbit; else [super keyDown:event]; }
@end
@interface Renderer : NSObject<MTKViewDelegate,NSWindowDelegate> {
 id<MTLDevice> device; id<MTLCommandQueue> queue; id<MTLRenderPipelineState> pipeline; id<MTLDepthStencilState> depth;
 id<MTLBuffer> vertices; std::vector<Batch> batches; Uniform uniform; double startTime;
}
-(instancetype)initWithView:(MTKView*)view scene:(NSString*)folder shader:(NSString*)shaderPath;
@end
@implementation Renderer
-(instancetype)initWithView:(MTKView*)view scene:(NSString*)folder shader:(NSString*)shaderPath {
 if(!(self=[super init]))return nil;
 device=view.device; queue=[device newCommandQueue]; NSError *error=nil;
 NSString *shader=[NSString stringWithContentsOfFile:shaderPath encoding:NSUTF8StringEncoding error:&error];
 if(!shader){NSLog(@"%@",error);exit(2);}
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
 SceneView *camera=(SceneView*)view;
 camera.yaw=[scene[@"yaw"] floatValue];camera.pitch=scene[@"pitch"]?[scene[@"pitch"] floatValue]:.35;
 camera.distance=scene[@"distance"]?[scene[@"distance"] floatValue]:3.4;camera.orbit=![scene[@"harbor"] boolValue];
 startTime=[NSDate timeIntervalSinceReferenceDate];
 NSMutableDictionary *cache=[NSMutableDictionary new];
 auto whiteDesc=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm width:1 height:1 mipmapped:NO];whiteDesc.storageMode=MTLStorageModeShared;
 id<MTLTexture> white=[device newTextureWithDescriptor:whiteDesc];uint32_t pixel=0xffffffff;[white replaceRegion:MTLRegionMake2D(0,0,1,1) mipmapLevel:0 withBytes:&pixel bytesPerRow:4];
 for(NSDictionary *draw in scene[@"draws"]){
 NSString *path=[draw[@"texture"] isKindOfClass:[NSString class]]?draw[@"texture"]:nil;id<MTLTexture> tex=path?cache[path]:white;
 if(!tex){NSData *tx=[NSData dataWithContentsOfFile:path];if(tx.length<24){NSLog(@"Invalid TX %@",path);exit(2);}int32_t h[6];memcpy(h,tx.bytes,24);
 // Expand Storm's packed 16-bit formats once at load time.
 NSMutableData *expanded=nil;
 if(h[4]==23 || h[4]==25 || h[4]==26){
   if(h[1]<=0||h[2]<=0||h[1]>16384||h[2]>16384||tx.length<24+(NSUInteger)h[1]*h[2]*2){NSLog(@"Invalid packed TX %@",path);exit(2);}
   expanded=[NSMutableData dataWithLength:24+(NSUInteger)h[1]*h[2]*4];memcpy(expanded.mutableBytes,h,24);
   const uint8_t *src=(const uint8_t*)tx.bytes+24;uint8_t *dst=(uint8_t*)expanded.mutableBytes+24;
   for(NSUInteger i=0;i<(NSUInteger)h[1]*h[2];i++){
     uint16_t c=src[2*i]|(src[2*i+1]<<8);uint8_t r,g,b,a=255;
     if(h[4]==26){b=(c&15)*17;g=((c>>4)&15)*17;r=((c>>8)&15)*17;a=((c>>12)&15)*17;}
     else if(h[4]==25){b=(c&31)*255/31;g=((c>>5)&31)*255/31;r=((c>>10)&31)*255/31;a=(c&0x8000)?255:0;}
     else{b=(c&31)*255/31;g=((c>>5)&63)*255/63;r=((c>>11)&31)*255/31;}
     dst[i*4]=b;dst[i*4+1]=g;dst[i*4+2]=r;dst[i*4+3]=a;
   }
   tx=expanded;h[4]=21;
 }
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
 batches.push_back({start,count,tex,[draw[@"kind"] floatValue]});
 }
 NSLog(@"Metal device: %@; draws: %lu",device.name,batches.size());return self;
}
-(void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {}
-(void)drawInMTKView:(MTKView*)view {
 auto drawable=view.currentDrawable;auto pass=view.currentRenderPassDescriptor;if(!drawable||!pass)return;
 SceneView *camera=(SceneView*)view;
 uniform.view={camera.yaw,(float)(view.drawableSize.width/view.drawableSize.height),camera.pitch,camera.distance};
 uniform.options={(float)([NSDate timeIntervalSinceReferenceDate]-startTime),0,0,0};
 auto command=[queue commandBuffer];auto enc=[command renderCommandEncoderWithDescriptor:pass];
 [enc setRenderPipelineState:pipeline];[enc setDepthStencilState:depth];[enc setCullMode:MTLCullModeNone];[enc setVertexBuffer:vertices offset:0 atIndex:0];[enc setVertexBytes:&uniform length:sizeof(uniform) atIndex:1];
 for(auto &b:batches){uniform.options.y=b.kind;[enc setVertexBytes:&uniform length:sizeof(uniform) atIndex:1];[enc setFragmentBytes:&uniform length:sizeof(uniform) atIndex:1];[enc setFragmentTexture:b.texture atIndex:0];[enc drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:b.start vertexCount:b.count];}
 [enc endEncoding];[command presentDrawable:drawable];[command commit];if(camera.orbit)camera.yaw+=.003;
}
-(BOOL)windowShouldClose:(NSWindow*)sender {[NSApp terminate:nil];return YES;}
@end
int main(int argc,const char**argv){@autoreleasepool{
 if(argc!=3){fprintf(stderr,"usage: metal-probe SCENE_DIRECTORY SHADER_FILE\n");return 2;}
 [NSApplication sharedApplication];[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
 id<MTLDevice> device=MTLCreateSystemDefaultDevice();if(!device)return 2;
 auto window=[[NSWindow alloc]initWithContentRect:NSMakeRect(0,0,1100,760) styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable|NSWindowStyleMaskResizable backing:NSBackingStoreBuffered defer:NO];
 window.title=@"Storm · Metal — drag to orbit · scroll to zoom · Space to rotate";
 auto view=[[SceneView alloc]initWithFrame:window.contentView.bounds device:device];view.autoresizingMask=NSViewWidthSizable|NSViewHeightSizable;view.autoResizeDrawable=YES;view.colorPixelFormat=MTLPixelFormatBGRA8Unorm;view.depthStencilPixelFormat=MTLPixelFormatDepth32Float;view.clearColor=MTLClearColorMake(.36,.57,.72,1);view.preferredFramesPerSecond=60;
 auto renderer=[[Renderer alloc]initWithView:view scene:[NSString stringWithUTF8String:argv[1]] shader:[NSString stringWithUTF8String:argv[2]]];view.delegate=renderer;window.delegate=renderer;window.contentView=view;[window makeFirstResponder:view];
 [window center];[window makeKeyAndOrderFront:nil];[NSApp activateIgnoringOtherApps:YES];[NSApp run];
}}
