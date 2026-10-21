#define main depthProofMain
#include "probe-production.mm"
#undef main
#include <algorithm>
#include <memory>

int main(int argc,const char** argv){@autoreleasepool{
    try{
        need(argc==2,"capture path required");
        id<MTLDevice> device=MTLCreateSystemDefaultDevice();
        Loader loader(device,@(argv[1]),nil,true);
        std::vector<std::unique_ptr<Runner>> runners;
        std::array<Faces,8> faces,selected;
        for(unsigned cube=0;cube<8;++cube){
            auto rows=loader.load(cube,faces[cube]);
            size_t seed=rows.size(),best=0;auto light=faces[cube][0].lightRange;
            for(size_t i=0;i<rows.size();++i)
                if(!rows[i].skinPalette&&LifecycleModel::intersects(rows[i].bounds,light.xyz,light.w)&&rows[i].indexCount>best){seed=i;best=rows[i].indexCount;}
            need(seed<rows.size(),"no rigid partition");
            const auto root=rows[seed].world;
            for(auto& row:rows)if(!row.skinPalette&&std::memcmp(&row.world,&root,sizeof(root))==0){row.rigidModel=77;row.modelLocal=row.world;row.modelBounds=row.bounds;}
            auto runner=std::make_unique<Runner>(device);
            Runner::fill(runner->fresh,rows);Runner::fill(runner->rigid,rows);Runner::fill(runner->dynamic,rows);
            selected[cube]=faces[cube];
            for(auto& face:selected[cube]){face.rigidModel=77;face.modelViewProjection=face.viewProjection;face.modelLightRange=face.lightRange;face.filter=1;}
            auto cb=[runner->queue commandBuffer];
            Runner::render(runner->rigid,cb,runner->cache,selected[cube],false,MTLLoadActionClear);
            [cb commit];[cb waitUntilCompleted];need(cb.status==MTLCommandBufferStatusCompleted,"cache bake");
            for(auto& face:selected[cube])face.filter=2;
            runners.push_back(std::move(runner));
        }
        auto queue=[device newCommandQueue];std::vector<double> full,cached;
        for(unsigned iteration=0;iteration<112;++iteration){
            for(unsigned order=0;order<2;++order){
                const bool split=(iteration%2)^order;auto cb=[queue commandBuffer];
                for(unsigned cube=0;cube<8;++cube){auto& r=*runners[cube];
                    if(split)Runner::render(r.dynamic,cb,r.work,selected[cube],false,MTLLoadActionClear,r.cache);
                    else Runner::render(r.fresh,cb,r.reference,faces[cube],false,MTLLoadActionClear);
                }
                [cb commit];[cb waitUntilCompleted];need(cb.status==MTLCommandBufferStatusCompleted,"timing command");
                if(iteration>=16)(split?cached:full).push_back((cb.GPUEndTime-cb.GPUStartTime)*1000);
            }
        }
        auto stats=[](std::vector<double> a){double sum=0;for(auto v:a)sum+=v;std::sort(a.begin(),a.end());return @{@"count":@(a.size()),@"mean_ms":@(sum/a.size()),@"median_ms":@(a[a.size()/2]),@"min_ms":@(a.front()),@"max_ms":@(a.back())};};
        auto result=@{@"status":@"COMPONENT_TIMING_ONLY",@"alpha":@"synthetic_checker",@"eight_cube_full":stats(full),@"eight_cube_cached_warm":stats(cached),@"whole_scene_acceptance":@NO};
        auto out=[NSJSONSerialization dataWithJSONObject:result options:NSJSONWritingPrettyPrinted error:nil];fwrite(out.bytes,1,out.length,stdout);puts("");return 0;
    }catch(const std::exception& e){fprintf(stderr,"%s\n",e.what());return 1;}
}}
