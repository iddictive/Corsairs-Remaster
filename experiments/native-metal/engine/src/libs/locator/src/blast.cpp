#include "blast.h"
#include "entity.h"
#include "core.h"
#include "shared/messages.h"
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
namespace
{
std::vector<entid_t> hullChipBursts;
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
}

// The scene holds these three meshes between bursts without consuming a burst
// lease, creating particles or participating in any render/execute layer.
class SHIP_HULL_CHIP_ASSETS final : public Entity
{
    std::array<std::shared_ptr<GEOS>,3> assets;
  public:
    bool Init() override
    { return LoadHullChipAssets(static_cast<VGEOMETRY *>(core.GetService("geometry")), assets); }
    void ProcessStage(Stage, uint32_t) override {}
};
CREATE_CLASS(SHIP_HULL_CHIP_ASSETS)

class SHIP_HULL_CHIPS final:public BLAST {public:SHIP_HULL_CHIPS():BLAST(true){}};
CREATE_CLASS(SHIP_HULL_CHIPS)
#endif

#define ANGLESPEED_MUL 0.2f

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
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if(hullChipMode)std::erase(hullChipBursts,GetId());
#endif
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
        if (!LoadHullChipAssets(gs, hullChipGeometry)) return false;
        // Retire settled flotsam for a fresh impact, never pop airborne boards
        // in and out as the remaining guns of the same salvo hit.
        if(!hullChipLease.acquire()){
            for(size_t n=0;n<hullChipBursts.size();){
                const auto oldest=hullChipBursts[n];
                auto*burst=static_cast<BLAST*>(core.GetEntityPointer(oldest));
                if(!burst){hullChipBursts.erase(hullChipBursts.begin()+n);continue;}
                if(!std::all_of(burst->Item.begin(),burst->Item.end(),[](const auto&item){return !item.chip.alive||item.chipFloatAge>=2.f;})){++n;continue;}
                hullChipBursts.erase(hullChipBursts.begin()+n);
                for(auto&item:burst->Item)item.chip.alive=false;
                burst->hullChipLease.release();core.EraseEntity(oldest);break;
            }
            if(!hullChipLease.acquire())return false;
        }
        hullChipBursts.push_back(GetId());
        Item.resize(storm_ship_hull::ChipLease::particlesPerBurst);ItemsNum=uint32_t(Item.size());
        for(unsigned n=0;n<ItemsNum;++n){auto&item=Item[n];item.geo=hullChipGeometry[n%hullChipGeometry.size()].get();item.bDouble=true;item.chip.alive=false;}
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

void BLAST::SetBlastCenter(CVECTOR pos, CVECTOR ang)
{
    uint32_t n;
    CMatrix m;
    for (n = 0; n < ItemsNum; n++)
    {
        Item[n].bEffect = false;
        Item[n].pos = pos;
        Item[n].ang = ang;
        m.BuildMatrix(ang);
        Item[n].dir = m.Vz();

        Item[n].ang.x += AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f;
        Item[n].ang.y += AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f;
        Item[n].ang.z += AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f;

        Item[n].ang_speed.x = ANGLESPEED_MUL * (AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f);
        Item[n].ang_speed.y = ANGLESPEED_MUL * (AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f);
        Item[n].ang_speed.z = ANGLESPEED_MUL * (AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f);

        Item[n].dir.x += AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f;
        Item[n].dir.y += AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f;
        Item[n].dir.z += AngleDeviation * rand() / RAND_MAX - AngleDeviation / 2.0f;
        Item[n].dir = !Item[n].dir;
        Item[n].speed = rand() * 0.025f / RAND_MAX + 0.005f;
    }
}

void BLAST::ProcessTime(uint32_t DT)
{
    uint32_t n;
    float res;

    if (!core.GetEntityPointer(sea_eid))
    {
        sea_eid = core.GetEntityId("sea");
        pSea = static_cast<CANNON_TRACE_BASE *>(core.GetEntityPointer(sea_eid));
    }

    const auto Delta_Time = static_cast<float>(DT); //*0.1;
    auto bStop = true;
    for (n = 0; n < ItemsNum; n++)
    {
        // if(Item[n].speed > 0) Item[n].speed -= 0.001f*Delta_Time;
        // else Item[n].speed = 0.0;
        // Item[n].pos += Item[n].speed*Item[n].dir*Delta_Time;
        CVECTOR a;
        a = Item[n].pos;
        Item[n].pos.x += (Item[n].dir.x * Item[n].speed * Delta_Time);
        Item[n].pos.y += (Item[n].dir.y * Item[n].speed * Delta_Time);
        Item[n].pos.z += (Item[n].dir.z * Item[n].speed * Delta_Time);

        // Item[n].pos.y -= Delta_Time*0.0005;
        Item[n].dir.y -= Delta_Time * 0.0008f;
        Item[n].pos.y += Item[n].dir.y * 0.01f * Delta_Time;

        Item[n].ang.x += Item[n].ang_speed.x * Delta_Time * 0.05f;
        Item[n].ang.y += Item[n].ang_speed.y * Delta_Time * 0.05f;
        Item[n].ang.z += Item[n].ang_speed.z * Delta_Time * 0.05f;

        if (Item[n].pos.y > 0)
            bStop = false;

        if (!Item[n].bEffect)
        {
            if (pSea)
            {
                res = pSea->Cannon_Trace(-1, a, Item[n].pos);
                if (res <= 1)
                    Item[n].bEffect = true;
            }
            else
            {
                if (Item[n].pos.y < 0)
                {
                    Item[n].bEffect = true;
                    // core.Send_Message(Splash,"lfff",MSG_BALLSPLASH_ADD,Item[n].pos.x,Item[n].pos.y,Item[n].pos.z);
                }
            }
        }

        /*
        if(Item[n].pos.y < 0 && !Item[n].bEffect)
        {
          Item[n].bEffect = true;
          //core.Send_Message(Splash,"lfff",MSG_BALLSPLASH_ADD,Item[n].pos.x,Item[n].pos.y,Item[n].pos.z);

        }*/
    }

    if (bStop)
        core.EraseEntity(GetId());
}

#ifdef STORM_METAL_SHIP_HULL_DAMAGE
void BLAST::LaunchHullChips(MESSAGE&message)
{
    storm_ship_hull::Vec3 position,normal;position.x=message.Float();position.y=message.Float();position.z=message.Float();normal.x=message.Float();normal.y=message.Float();normal.z=message.Float();float radius=message.Float();uint32_t seed=uint32_t(message.Long());if(!seed)seed=1;
    if(!storm_ship_hull::finite(position)||!storm_ship_hull::finite(normal)||storm_ship_hull::length(normal)<.9f||!std::isfinite(radius)||radius<storm_hull_damage::MinHitRadius||radius>storm_hull_damage::MaxHitRadius){core.EraseEntity(GetId());return;}
    normal=storm_ship_hull::normalized(normal);
    for(auto&item:Item){
        item.chip=storm_ship_hull::launchChip(position,normal,seed,radius);
        item.chip.life=20.f;item.chipFloatAge=-1.f;
        GEOS::INFO info{};item.geo->GetInfo(info);
        const float nativeLength=std::max({info.boxsize.x,info.boxsize.y,info.boxsize.z});
        if(!std::isfinite(nativeLength)||nativeLength<.001f){item.chip.alive=false;continue;}
        // Keep the authored broken-wood silhouette/material; normalize actual
        // mesh length instead of shrinking all three models into tiny slivers.
        const float length=.45f+storm_ship_hull::randomUnit(seed)*std::min(.45f,radius*.7f);
        item.chipScale=length/nativeLength;
    }
}
void BLAST::StepHullChips(uint32_t delta)
{
    if(!delta||core.GetTimeScale()<=0)return;
    if(!core.GetEntityPointer(sea_eid)){sea_eid=core.GetEntityId("sea");pSea=static_cast<CANNON_TRACE_BASE*>(core.GetEntityPointer(sea_eid));chipSea=static_cast<SEA_BASE*>(core.GetEntityPointer(sea_eid));}
    const float seconds=float(delta)*.001f;
    bool alive=false;for(auto&item:Item){auto&chip=item.chip;if(!chip.alive)continue;float floating=seconds;
        if(item.chipFloatAge<0.f){const auto before=chip.position;storm_ship_hull::step(chip,seconds);const auto after=chip.position;
            float contact=2.f;if(chipSea){const float water=chipSea->WaveXZ(after.x,after.z);
                if(before.y<=chipSea->WaveXZ(before.x,before.z))contact=0.f;
                else if(after.y<=water)contact=pSea->Cannon_Trace(-1,CVECTOR(before.x,before.y,before.z),CVECTOR(after.x,after.y,after.z));}
            if(contact<=1.f){chip.position=before+(after-before)*std::clamp(contact,0.f,1.f);item.chipFloatAge=0.f;chip.velocity=chip.velocity*.15f;chip.velocity.y=0.f;chip.spin=chip.spin*.12f;chip.angles.x=chip.angles.z=0.f;floating*=1.f-contact;}
            else floating=0.f;
        }else chip.age+=seconds;
        if(item.chipFloatAge>=0.f&&chipSea){
            // Reuse the native sink-flotsam convention: these palka meshes lie
            // flat at zero pitch/roll, yaw freely, then sink below WaveXZ.
            item.chipFloatAge+=floating;const float damping=std::exp(-1.2f*floating);
            chip.position=chip.position+chip.velocity*((1.f-damping)/1.2f);chip.velocity=chip.velocity*damping;
            chip.angles.y+=chip.spin.y*floating;chip.spin=chip.spin*std::exp(-.3f*floating);
            const float sink=std::max(0.f,item.chipFloatAge-8.f)*.6f;
            chip.position.y=chipSea->WaveXZ(chip.position.x,chip.position.z)+.04f-sink;
            if(sink>2.f||chip.age>=chip.life)chip.alive=false;
        }
        alive|=chip.alive;}
    if(!alive)core.EraseEntity(GetId());
}
#endif

uint64_t BLAST::ProcessMessage(MESSAGE &message)
{
    int32_t code;
    CVECTOR ang;
    CVECTOR pos;
    code = message.Long();
    switch (code)
    {
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    case storm_hull_damage::ChipBurstMessage: if(hullChipMode)LaunchHullChips(message);break;
#endif
    case LM_SETPOINT:
        ang.x = ang.y = ang.z = 0;
        ang.x = -1.57f;
        pos.x = message.Float();
        pos.y = message.Float();
        pos.z = message.Float();
        SetBlastCenter(pos, ang);
        break;
    case LM_SETPOINTANDANGLES:
        pos.x = message.Float();
        pos.y = message.Float();
        pos.z = message.Float();
        ang.x = message.Float();
        ang.y = message.Float();
        ang.z = message.Float();
        SetBlastCenter(pos, ang);
        break;
    }
    return 0;
}

uint32_t BLAST::AttributeChanged(ATTRIBUTES *pA)
{
    return 0;
}

void BLAST::Realize(uint32_t Delta_Time)
{
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
    if(hullChipMode){gs->SetTechnique("");for(auto&item:Item)if(item.chip.alive&&item.geo){auto&c=item.chip;
        Center.BuildMatrix(CVECTOR(c.angles.x,c.angles.y,c.angles.z),CVECTOR(c.position.x,c.position.y,c.position.z));for(unsigned row=0;row<3;row++)for(unsigned col=0;col<3;col++)Center.m[row][col]*=item.chipScale;
        rs->SetTransform(storm_render::TransformWorld,Center);item.geo->Draw((GEOS::PLANE*)rs->GetPlanes(),0,nullptr);}return;}
#endif
    uint32_t n;

    if (Item.empty())
        return;

    ProcessTime(Delta_Time);

    gs->SetTechnique("");
    for (n = 0; n < ItemsNum; n++)
    {
        if (Item[n].geo)
        {
            Center.BuildMatrix(Item[n].ang, Item[n].pos);
            rs->SetTransform(storm_render::TransformWorld, Center);
            Item[n].geo->Draw((GEOS::PLANE *)rs->GetPlanes(), 0, nullptr);
        }
    }
}
