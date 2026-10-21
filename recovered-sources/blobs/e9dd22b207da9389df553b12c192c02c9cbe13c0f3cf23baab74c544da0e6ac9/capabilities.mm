#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

int main() {
 @autoreleasepool {
  id<MTLDevice> device=MTLCreateSystemDefaultDevice();
  if(!device)return 1;
  NSMutableArray *rows=[NSMutableArray new];
  const MTLPixelFormat formats[]={MTLPixelFormatRGBA8Unorm,MTLPixelFormatBC1_RGBA,MTLPixelFormatBC2_RGBA,MTLPixelFormatBC3_RGBA};
  NSArray *names=@[@"RGBA8",@"BC1/DXT1",@"BC2/DXT3",@"BC3/DXT5"];
  bool success=true;
  for(unsigned i=0;i<4;i++) {
   if(i&&!device.supportsBCTextureCompression)continue;
   auto *descriptor=[MTLTextureDescriptor texture2DDescriptorWithPixelFormat:formats[i] width:2048 height:2048 mipmapped:YES];
   descriptor.storageMode=MTLStorageModeShared;
   descriptor.usage=MTLTextureUsageShaderRead;
   auto size=[device heapTextureSizeAndAlignWithDescriptor:descriptor];
   descriptor.width=descriptor.height=4;descriptor.mipmapLevelCount=1;
   id<MTLTexture> texture=[device newTextureWithDescriptor:descriptor];
   success&=texture!=nil;
   if(texture) {
    unsigned char blocks[64]{};
    [texture replaceRegion:MTLRegionMake2D(0,0,4,4) mipmapLevel:0 withBytes:blocks bytesPerRow:i?(i==1?8:16):16];
   }
   [rows addObject:@{@"format":names[i],@"heap_bytes_2048_mips":@(size.size),@"alignment":@(size.align),@"shared_upload_succeeded":@(texture!=nil)}];
  }
  NSDictionary *result=@{@"device":device.name,@"supports_bc":@(device.supportsBCTextureCompression),@"os":NSProcessInfo.processInfo.operatingSystemVersionString,@"submitted_gpu_commands":@0,@"formats":rows};
  NSData *json=[NSJSONSerialization dataWithJSONObject:result options:NSJSONWritingPrettyPrinted error:nil];
  fwrite(json.bytes,1,json.length,stdout);fputc('\n',stdout);
  return success?0:2;
 }
}
