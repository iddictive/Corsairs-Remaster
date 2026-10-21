#include <array>
#include <memory>
#include <vector>
#include <string>
#include <set>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/experiments/native-metal/ship_hull_chips.hpp"
#define STORM_METAL_SHIP_HULL_DAMAGE 1
constexpr int MAX_PATH = 260;
struct VGEOMETRY;
struct GEOS {VGEOMETRY*owner;std::string name;int id;GEOS(VGEOMETRY*o,const char*n,int i):owner(o),name(n),id(i){}~GEOS();};
struct VGEOMETRY {int calls=0,destroyed=0,failAt=-1;std::set<GEOS*>live;GEOS*CreateGeometry(const char*name,const char*,int){++calls;if(calls==failAt)return nullptr;auto*g=new GEOS(this,name,calls);live.insert(g);return g;}};
GEOS::~GEOS(){assert(owner->live.erase(this)==1);++owner->destroyed;}
struct VDX9RENDER{};struct CANNON_TRACE_BASE{};
struct Core {VGEOMETRY*geometry=nullptr;VDX9RENDER renderer;void*GetService(const char*name){return !std::strcmp(name,"geometry")?(void*)geometry:(void*)&renderer;}int GetEntityId(const char*){return 1;}template<class...A>void Trace(A...){}}core;
struct Ini{int GetInt(const char*,const char*,int){return 0;}bool ReadString(const char*,const char*,char*out,size_t,const char*){std::strcpy(out,"normal-blast-mesh");return true;}bool ReadStringNext(const char*,const char*,char*,size_t){return false;}};
struct Fio{std::unique_ptr<Ini>OpenIniFile(const char*){return std::make_unique<Ini>();}}fioInstance;Fio*fio=&fioInstance;
struct Particle{GEOS*geo=nullptr;bool bDouble=false;storm_ship_hull::Chip chip;float chipScale=1;};
class BLAST {public:VDX9RENDER*rs=nullptr;VGEOMETRY*gs=nullptr;std::vector<Particle>Item;CANNON_TRACE_BASE*pSea=nullptr;int sea_eid=0;float AngleDeviation=0;uint32_t ItemsNum=0;int Splash=0;bool hullChipMode=false;storm_ship_hull::ChipLease hullChipLease;std::array<std::shared_ptr<GEOS>,3>hullChipGeometry;explicit BLAST(bool hullChips=false);~BLAST();bool Init();void AddGeometry(char*,int32_t);};

enum class Stage { execute, realize };
struct Entity { virtual ~Entity()=default; virtual bool Init(){return true;} virtual void ProcessStage(Stage,uint32_t)=0; };

bool LoadHullChipAssets(VGEOMETRY *service, std::array<std::shared_ptr<GEOS>,3> &result)
{
    if (!service) return false;
    static VGEOMETRY *assetService = nullptr;
    static std::array<std::weak_ptr<GEOS>,3> assets;
    if (assetService != service) { assets = {}; assetService = service; }
    const char *names[] = {"particles\\palka01", "particles\\palka02", "particles\\palka04"};
    std::array<std::shared_ptr<GEOS>,3> loaded;
    for (unsigned n = 0; n < loaded.size(); ++n)
    {
        loaded[n] = assets[n].lock();
        if (!loaded[n])
        {
            loaded[n].reset(service->CreateGeometry(names[n], nullptr, 0));
            if (!loaded[n]) return false;
            assets[n] = loaded[n];
        }
    }
    result = std::move(loaded);
    return true;
}

class SHIP_HULL_CHIP_ASSETS final : public Entity
{
    std::array<std::shared_ptr<GEOS>,3> assets;
  public:
    bool Init() override
    { return LoadHullChipAssets(static_cast<VGEOMETRY *>(core.GetService("geometry")), assets); }
    void ProcessStage(Stage, uint32_t) override {}
};

#ifdef STORM_METAL_SHIP_HULL_DAMAGE
BLAST::BLAST(bool hullChips)
    : sea_eid(0), Splash(0),hullChipMode(hullChips)
#else
BLAST::BLAST()
    : sea_eid(0), Splash(0)
#endif
{
    rs = nullptr;
    gs = nullptr;
    AngleDeviation = 1.57f;
    ItemsNum = 0;
    pSea = nullptr;
}

BLAST::~BLAST()
{
    for (uint32_t i = 0; i < ItemsNum; i++)
        if (!Item[i].bDouble)
            delete Item[i].geo;
}

bool BLAST::Init()
{
    gs = static_cast<VGEOMETRY *>(core.GetService("geometry"));
    if (!gs)
        return false;
    rs = static_cast<VDX9RENDER *>(core.GetService("dx9render"));
    if (!rs)
        return false;
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if(hullChipMode){
        if(!hullChipLease.acquire())return false;
        if (!LoadHullChipAssets(gs, hullChipGeometry)) return false;
        Item.resize(storm_ship_hull::ChipLease::particlesPerBurst);ItemsNum=uint32_t(Item.size());
        for(unsigned n=0;n<ItemsNum;++n){auto&item=Item[n];item.geo=hullChipGeometry[n<2?0:n<6?1:2].get();item.bDouble=true;item.chip.alive=false;}
        return true;
    }
#endif

    //    int32_t n;
    auto ini = fio->OpenIniFile("resource\\ini\\particles\\particles.ini");
    if (!ini)
    {
        core.Trace("not found: resource\\ini\\particles\\particles.ini");
        return false;
    }

    const auto RandomNum = ini->GetInt("geo", "randomnum", 0);

    char name[MAX_PATH];
    if (ini->ReadString("geo", "file", name, sizeof(name), ""))
    {
        AddGeometry(name, RandomNum * rand() / RAND_MAX + 1);
    }
    while (ini->ReadStringNext("geo", "file", name, sizeof(name)))
    {
        AddGeometry(name, RandomNum * rand() / RAND_MAX + 1);
    }

    Splash = core.GetEntityId("BallSplash");

    return true;
}

void BLAST::AddGeometry(char *name, int32_t num)
{
    // n = ItemsNum;
    // ItemsNum++;
    // Item = (GEOPARTICLE*)RESIZE(Item,sizeof(GEOPARTICLE)*ItemsNum);
    // Item[n].geo = 0;
    // Item[n].geo = gs->CreateGeometry(name,0,0);

    Item.resize(ItemsNum + num); //~!~
    auto *const gp = gs->CreateGeometry(name, nullptr, 0);
    for (int32_t n = 0; n < num; n++)
    {
        if (n == 0)
            Item[n + ItemsNum].bDouble = false;
        else
            Item[n + ItemsNum].bDouble = true;
        Item[n + ItemsNum].geo = gp;
    }
    ItemsNum += num;
}

std::unique_ptr<BLAST>make(VGEOMETRY&service,bool chips=true){core.geometry=&service;auto b=std::make_unique<BLAST>(chips);if(!b->Init())return {};return b;}
void appearance(const BLAST&b,VGEOMETRY&service){assert(b.ItemsNum==8&&b.Item.size()==8);const char*names[]={"particles\\palka01","particles\\palka02","particles\\palka04"};for(unsigned n=0;n<8;n++){const auto&item=b.Item[n];assert(item.geo&&item.geo->owner==&service&&service.live.count(item.geo)&&item.bDouble&&!item.chip.alive);assert(item.geo->name==names[n<2?0:n<6?1:2]);}}
int main(){using storm_ship_hull::ChipLease;
 { VGEOMETRY scene; core.geometry=&scene; auto anchor=std::make_unique<SHIP_HULL_CHIP_ASSETS>();
   assert(anchor->Init()&&scene.calls==3&&scene.live.size()==3&&ChipLease::liveBursts()==0);
   {auto first=make(scene);assert(first);appearance(*first,scene);assert(scene.calls==3);}
   assert(scene.live.size()==3&&ChipLease::liveBursts()==0);
   std::vector<std::unique_ptr<BLAST>> active;
   for(unsigned n=0;n<ChipLease::maxBursts;++n){active.push_back(make(scene));assert(active.back());appearance(*active.back(),scene);}
   assert(!make(scene)&&scene.calls==3&&ChipLease::liveBursts()==8);
   anchor.reset();assert(scene.live.size()==3);active.clear();assert(scene.live.empty()&&scene.destroyed==3&&ChipLease::liveBursts()==0);
 }
 {VGEOMETRY partial;partial.failAt=2;core.geometry=&partial;SHIP_HULL_CHIP_ASSETS anchor;assert(!anchor.Init());assert(partial.live.empty()&&ChipLease::liveBursts()==0);partial.failAt=-1;assert(anchor.Init()&&partial.live.size()==3&&partial.calls==5);}
assert(ChipLease::liveBursts()==0);VGEOMETRY a,b;
 {std::vector<std::unique_ptr<BLAST>>bursts;for(int n=0;n<4;n++){bursts.push_back(make(a));assert(bursts.back());appearance(*bursts.back(),a);}assert(a.calls==3&&a.live.size()==3&&ChipLease::liveBursts()==4);for(int n=1;n<4;n++)for(int i=0;i<8;i++)assert(bursts[n]->Item[i].geo==bursts[0]->Item[i].geo);auto survivor=bursts.back()->Item[0].geo;bursts.front().reset();assert(a.live.size()==3&&a.live.count(survivor)&&ChipLease::liveBursts()==3);
  auto ordinary=make(a,false);assert(ordinary&&a.calls==4&&a.live.size()==4&&ordinary->Item.size()==1&&!ordinary->Item[0].bDouble);ordinary.reset();assert(a.live.size()==3&&a.destroyed==1);appearance(*bursts.back(),a);bursts.clear();assert(a.live.empty()&&a.destroyed==4&&ChipLease::liveBursts()==0);}
 {auto recreated=make(a);assert(recreated&&a.calls==7&&a.live.size()==3);appearance(*recreated,a);auto foreign=make(b);assert(foreign&&b.calls==3&&b.live.size()==3);appearance(*foreign,b);for(int i=0;i<8;i++)assert(recreated->Item[i].geo!=foreign->Item[i].geo);foreign.reset();assert(b.live.empty()&&a.live.size()==3);appearance(*recreated,a);recreated.reset();assert(a.live.empty()&&ChipLease::liveBursts()==0);}
 {int before=a.calls;std::vector<std::unique_ptr<BLAST>>bursts;for(unsigned n=0;n<ChipLease::maxBursts;n++){bursts.push_back(make(a));assert(bursts.back());appearance(*bursts.back(),a);}assert(a.calls-before==3&&ChipLease::liveBursts()==8);assert(!make(a)&&a.calls-before==3&&ChipLease::liveBursts()==8);bursts.clear();assert(a.live.empty()&&ChipLease::liveBursts()==0);}
 {VGEOMETRY failing;failing.failAt=2;assert(!make(failing)&&failing.live.empty()&&failing.calls==2&&failing.destroyed==1&&ChipLease::liveBursts()==0);failing.failAt=-1;auto recovered=make(failing);assert(recovered&&failing.live.size()==3&&failing.calls==5);appearance(*recovered,failing);recovered.reset();assert(failing.live.empty()&&ChipLease::liveBursts()==0);}
 {auto ordinary=make(a,false);assert(ordinary&&ordinary->Item.size()==1);auto before=a.calls;char name[]="ordinary-shared-inside-one-blast";ordinary->AddGeometry(name,3);assert(a.calls==before+1&&ordinary->Item.size()==4&&!ordinary->Item[1].bDouble&&ordinary->Item[2].bDouble&&ordinary->Item[3].bDouble&&ordinary->Item[1].geo==ordinary->Item[3].geo);ordinary.reset();assert(a.live.empty());}
 std::cout<<"PASS exact BLAST Init/AddGeometry/destructor: four bursts preserve32 particles/2-4-2 mesh order with3 asset creates instead of12; last-release/recreate/service boundary/failure recovery/max8leases64particles/ordinary blast ownership; zero live assets at closure\n";
}
