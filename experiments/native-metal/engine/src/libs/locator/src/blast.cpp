#include "blast.h"
#include "entity.h"
#include "core.h"
#include "shared/messages.h"
#ifdef STORM_METAL_SHIP_HULL_DAMAGE
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
        char one[]="particles\\palka01",two[]="particles\\palka02",four[]="particles\\palka04";AddGeometry(one,2);AddGeometry(two,4);AddGeometry(four,2);
        for(auto&item:Item){item.chip.alive=false;if(!item.geo)return false;}
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
    normal=storm_ship_hull::normalized(normal);for(auto&item:Item){item.chip=storm_ship_hull::launchChip(position,normal,seed,radius);item.chipScale=.12f+storm_ship_hull::randomUnit(seed)*std::min(.25f,radius);}
}
void BLAST::StepHullChips(uint32_t delta)
{
    if(!delta||core.GetTimeScale()<=0)return;
    if(!core.GetEntityPointer(sea_eid)){sea_eid=core.GetEntityId("sea");pSea=static_cast<CANNON_TRACE_BASE*>(core.GetEntityPointer(sea_eid));}
    bool alive=false;for(auto&item:Item){auto before=item.chip.position;storm_ship_hull::step(item.chip,float(delta)*.001f);auto after=item.chip.position;
        if(item.chip.alive&&pSea&&pSea->Cannon_Trace(-1,CVECTOR(before.x,before.y,before.z),CVECTOR(after.x,after.y,after.z))<=1.f)item.chip.alive=false;alive|=item.chip.alive;}
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
