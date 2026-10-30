//============================================================================================
//    Spirenkov Maxim, 2001
//--------------------------------------------------------------------------------------------
//    Sea Dogs II
//--------------------------------------------------------------------------------------------
//    WdmShip
//--------------------------------------------------------------------------------------------
//
//============================================================================================

#include "wdm_ship.h"
#include "metal_world_map_bridge.h"
#include "wdm_islands.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <queue>

//============================================================================================

#define WDM_SHIP_MAXDTIME 0.1f   // The largest allowed time period for calculations
#define WDM_SHIP_CLAMPDTIME 0.5f // Longest valid time period in Update

//============================================================================================

#define WDM_SHIP_WMSZ (sizeof(lines) / sizeof(Line))

// ============================================================================================
// Construction, destruction
// ============================================================================================

WdmShip::WdmShip()
{
    modelL05 = 6.0f;
    modelW05 = 1.9f;
    modelRadius2 = modelL05 * modelL05 + modelW05 * modelW05;
    modelRadius = sqrtf(modelRadius2);
    dltAx = 60.0f * (0.01f + rand() * (0.005f / RAND_MAX));
    dltAz = 60.0f * (0.02f + rand() * (0.005f / RAND_MAX));
    ax = ay = az = 0.0f;
    speed = 0.0f;
    rspeed = 0.0f;
    turnspd = 0.0f;
    slope = 0.0f;
    for (int32_t i = 1; i < sizeof(lines) / sizeof(Line); i++)
    {
        lines[i].x = 0.0f;
        lines[i].z = 0.0f;
        lines[i].ay = 0.0f;
        lines[i].size = 0.0f;
    }
    wmtexture = -1;
    wdmObjects->AddShip(this);
    kMaxSpeed = 1.0f;
    wmtexture = wdmObjects->rs->TextureCreate("WorldMap\\watermark.tga");
    isLive = true;
    isSelect = false;
    collisionCounter = 0;
}

WdmShip::~WdmShip()
{
    wdmObjects->DelShip(this);
    wdmObjects->rs->TextureRelease(wmtexture);
    if ((WdmShip *)wdmObjects->enemyShip == this)
        wdmObjects->enemyShip = nullptr;
}

bool WdmShip::Load(const char *modelName)
{
    const auto res = WdmRenderModel::Load(modelName);
    if (geo)
    {
        GEOS::INFO ginfo;
        geo->GetInfo(ginfo);
        modelL05 = ginfo.boxsize.z * 0.25f;
        modelW05 = ginfo.boxsize.x * 0.25f;
    }
    return res;
}

void WdmShip::Teleport(float x, float z, float ay)
{
    mtx.BuildMatrix(0.0f, ay, 0.0f, x, 0.0f, z);
    this->ay = ay;
    for (int32_t i = 0; i < WDM_SHIP_WMSZ; i++)
    {
        lines[i].x = x;
        lines[i].z = z;
        lines[i].ay = ay;
        lines[i].size = 0.0f;
    }
}

void WdmShip::GetPosition(float &x, float &z, float &ay) const
{
    x = mtx.Pos().x;
    z = mtx.Pos().z;
    ay = this->ay;
}

void WdmShip::SetMaxSpeed(float k)
{
    if (k > 5.0f)
        k = 5.0f;
    if (k < 0.0f)
        k = 0.0f;
    kMaxSpeed = k;
}

void WdmShip::Update(float dltTime)
{
    if (dltTime > WDM_SHIP_CLAMPDTIME)
        dltTime = WDM_SHIP_CLAMPDTIME;
    for (; dltTime >= WDM_SHIP_MAXDTIME; dltTime -= WDM_SHIP_MAXDTIME)
        ShipUpdate(WDM_SHIP_MAXDTIME);
    if (dltTime > 0.0f)
        ShipUpdate(dltTime);
}

// Calculations
void WdmShip::SteerToDirection(const CVECTOR &direction, float seconds, float slowing)
{
    const auto dl = direction.x * direction.x + direction.z * direction.z;
    const auto vx = mtx.Vz().x;
    const auto vz = mtx.Vz().z;
    const auto vl = vx * vx + vz * vz;
    auto sn = vz * direction.x - vx * direction.z;
    if (dl * vl > 0.0f) sn /= sqrtf(dl * vl);
    const auto cs = vx * direction.x + vz * direction.z;
    if (cs < 0.0f)
    {
        if (fabsf(sn) < 0.05f) sn = turnspd >= 0.0f ? 1.0f : -1.0f;
        else sn = sn < 0.0f ? -1.0f : 1.0f;
    }
    turnspd += 2.0f * WDM_SHIP_TSPEED * sn * seconds;
    if (cs >= 0.0f)
        speed += kMaxSpeed * WDM_SHIP_INER_ST * WDM_SHIP_SPEED * seconds;
    else
    {
        const float minimum = 0.35f * WDM_SHIP_SPEED * kMaxSpeed;
        if (speed > minimum)
        {
            speed -= kMaxSpeed * WDM_SHIP_INER_ST * WDM_SHIP_SPEED * seconds;
            if (speed < minimum) speed = minimum;
        }
        else if (speed < minimum)
        {
            speed += kMaxSpeed * WDM_SHIP_INER_ST * WDM_SHIP_SPEED * seconds;
            if (speed > minimum) speed = minimum;
        }
    }
    speed *= slowing;
    turnspd *= slowing;
}

bool WdmShip::BeginRouteRecovery(const CVECTOR &goal,float arrival)
{
    if (!wdmObjects->islands) return false;
    const CVECTOR origin=mtx.Pos();
    const float heading=atan2f(mtx.Vz().x,mtx.Vz().z);
    constexpr float step=0.5f,angleStep=3.14159265f/16.0f;
    const float extent=4.0f*sqrtf(modelL05*modelL05+modelW05*modelW05);
    const int limit=std::max(2,static_cast<int>(ceilf(extent/step))),width=2*limit+1;
    const int count=width*width*32;
    std::vector<float> cost(count,std::numeric_limits<float>::infinity());
    std::vector<int> parent(count,-1);
    using Entry=std::pair<float,int>;
    std::priority_queue<Entry,std::vector<Entry>,std::greater<Entry>> pending;
    auto index=[&](int x,int z,int a){return ((z+limit)*width+x+limit)*32+a;};
    auto pose=[&](int id){int a=id%32;id/=32;int x=id%width-limit,z=id/width-limit;return RecoveryPose{origin+CVECTOR(float(x)*step,0.0f,float(z)*step),heading+float(a)*angleStep};};
    auto distance=[&](const CVECTOR&p){return sqrtf((p.x-goal.x)*(p.x-goal.x)+(p.z-goal.z)*(p.z-goal.z));};
    const int first=index(0,0,0);cost[first]=0;pending.emplace(std::max(0.0f,distance(origin)-arrival)/5.0f,first);
    int end=-1;
    for(int expanded=0;!pending.empty()&&expanded<1024;++expanded)
    {
        const int current=pending.top().second;pending.pop();const auto from=pose(current);
        if(distance(from.pos)<=arrival){end=current;break;}
        int a=current%32,cell=current/32,x=cell%width-limit,z=cell/width-limit;
        for(int move=0;move<10;++move)
        {
            static constexpr int ox[8]={1,0,-1,0,1,1,-1,-1},oz[8]={0,1,0,-1,1,-1,1,-1};
            int nx=x,nz=z,na=a;
            if(move<8){nx+=ox[move];nz+=oz[move];}else na=(a+(move==8?1:31))%32;
            if(abs(nx)>limit||abs(nz)>limit)continue;
            const int next=index(nx,nz,na);const auto to=pose(next);
            const float angle=std::remainder(to.yaw-from.yaw,6.2831853f);
            const float duration=move<8?sqrtf(~(to.pos-from.pos))/5.0f:fabsf(angle)/WDM_SHIP_MAX_TURNSPD;
            const float incoming=cost[current]+duration;
            if(incoming>=cost[next])continue;
            bool clear=true;
            const int samples=move<8?8:16;
            for(int sample=1;sample<=samples;++sample)
            {
                const float k=float(sample)/float(samples);const CVECTOR p=from.pos+(to.pos-from.pos)*k;
                if(fabsf(p.x)>0.5f*wdmObjects->worldSizeX||fabsf(p.z)>0.5f*wdmObjects->worldSizeZ){clear=false;break;}
                CMatrix check(0,from.yaw+angle*k,0,p.x,0,p.z);
                if(wdmObjects->islands->CollisionTest(check,modelL05,modelW05)){clear=false;break;}
            }
            if(!clear)continue;
            cost[next]=incoming;parent[next]=current;pending.emplace(incoming+std::max(0.0f,distance(to.pos)-arrival)/5.0f,next);
        }
    }
    if(end<0)return false;
    recoveryPath.clear();for(int id=end;id!=first;id=parent[id])recoveryPath.push_back(pose(id));
    std::reverse(recoveryPath.begin(),recoveryPath.end());recoveryStep=0;recovering=!recoveryPath.empty();return recovering;
}

void WdmShip::ShipUpdate(float dltTime)
{
    if(recovering)
    {
        const auto target=recoveryPath[recoveryStep];
        const float difference=std::remainder(target.yaw-ay,6.2831853f);
        const float newYaw=ay+std::clamp(difference,-WDM_SHIP_MAX_TURNSPD*dltTime,WDM_SHIP_MAX_TURNSPD*dltTime);
        CVECTOR delta=target.pos-mtx.Pos();delta.y=0.0f;const float distance=sqrtf(~delta);
        const CVECTOR next=distance>1e-5f?mtx.Pos()+delta*(std::min(distance,5.0f*dltTime)/distance):target.pos;
        CMatrix candidate(0,newYaw,0,next.x,0,next.z);
        if(!wdmObjects->islands||!wdmObjects->islands->CollisionTest(candidate,modelL05,modelW05))
        {
            ay=newYaw;mtx=candidate;speed=turnspd=0.0f;rspeed=0.0f;collisionCounter=0;isWMRender=false;
            if(distance<=5.0f*dltTime&&fabsf(difference)<=WDM_SHIP_MAX_TURNSPD*dltTime)
            {if(++recoveryStep==recoveryPath.size())CancelRouteRecovery();}
            UpdateWaterMark(dltTime);return;
        }
        CancelRouteRecovery();recoveryRetry=1.0f;collisionCounter=10;
    }
    const auto pi2 = 3.14159265f * 2.0f;
    // Uncheck the render flag
    isWMRender = false;
    // Keep the previous position
    const auto oay = ay;
    auto opos = mtx.Pos();
    // Limiting the speed
    if (speed > WDM_SHIP_MAX_SPEED * kMaxSpeed)
        speed = WDM_SHIP_MAX_SPEED * kMaxSpeed;
    if (speed < -0.4f * WDM_SHIP_MAX_SPEED * kMaxSpeed)
        speed = -0.4f * WDM_SHIP_MAX_SPEED * kMaxSpeed;
    if (turnspd > WDM_SHIP_MAX_TURNSPD)
        turnspd = WDM_SHIP_MAX_TURNSPD;
    if (turnspd < -WDM_SHIP_MAX_TURNSPD)
        turnspd = -WDM_SHIP_MAX_TURNSPD;
    // swing
    ax += dltAx * dltTime;
    az += dltAz * dltTime;
    // Turning
    ay += turnspd * dltTime;
    // Limiting the angles
    if (ax > pi2)
        ax -= pi2;
    if (ax < -pi2)
        ax += pi2;
    if (ay > pi2)
        ay -= pi2;
    if (ay < -pi2)
        ay += pi2;
    if (az > pi2)
        az -= pi2;
    if (az < -pi2)
        az += pi2;
    // Coefficient of dependence of speed on wind
    CVECTOR windDir = 0.0f;
    auto kWind = wdmObjects->GetWind(mtx.Pos().x, mtx.Pos().z, windDir);
    kWind *= (sinf(ay) * windDir.x + cosf(ay) * windDir.z) * 0.5f + 0.5f;
    kWind =
        wdmObjects->shipSpeedOppositeWind + (wdmObjects->shipSpeedOverWind - wdmObjects->shipSpeedOppositeWind) * kWind;
    // Find a new position
    CVECTOR pos = mtx.Pos();
    const float dltMove = speed * kWind * dltTime;
    pos += mtx.Vz() * dltMove + rspeed * dltTime;
    baseV -= dltMove * 0.04f;
    if (baseV < -1.0f)
        baseV += 1.0f;
    pos.y = 0.0f;
    float k = WDM_SHIP_INER_BR * dltTime;
    if (k > 1.0f)
        k = 1.0f;
    turnspd -= turnspd * k;
    speed -= k * speed;
    rspeed -= k * rspeed;
    slope += (turnspd * speed * (0.5f / WDM_SHIP_MAX_SPEED) - slope) * k;
    // recalculate the matrix
    mtx.BuildMatrix(0.1f * sinf(ax), ay, 0.1f * sinf(az) + slope, pos.x, 0.0f, pos.z);
    // Collision with the ground
    if (wdmObjects->islands)
    {
        if (wdmObjects->islands->CollisionTest(mtx, modelL05, modelW05))
        {
            collisionCounter++;
            if (collisionCounter > 10)
            {
                if (collisionCounter == 11)
                {
                    collisionEscapeDirection = pos - wdmObjects->islands->centPos;
                    collisionEscapeDirection.y = 0.0f;
                    if (~collisionEscapeDirection > 1e-20f) collisionEscapeDirection = !collisionEscapeDirection;
                    else collisionEscapeDirection = -mtx.Vz();
                }
                opos += collisionEscapeDirection * (5.0f * dltTime);
            }
            ay = oay;
            // calculate the reaction to a collision
            pos = wdmObjects->islands->centPos - pos;
            k = ~pos;
            if (k)
            {
                pos *= 1.0f / sqrtf(k);
                k = speed;
                if (k >= 0.0f)
                {
                    if (k < 0.8f)
                        k = 0.8f;
                }
                else
                {
                    k = -0.1f;
                }
                rspeed += pos * (-0.8f) * (k);
                k = ~rspeed;

                mtx.MulToInvNorm(CVECTOR(pos), pos);
                turnspd = -1.5f * 2.0f * pos.x * pos.z * (pos.z > 0.0f ? 2.0f : 1.0f);

                // Limiting the kick-back speed
                if (k > 25.0f)
                    rspeed *= 5.0f / k;
            }
            // return the ship to the previous position
            mtx.BuildMatrix(0.1f * sinf(ax), oay, 0.1f * sinf(az) + slope, opos.x, 0.0f, opos.z);
            speed = 0.0f;
            Collide();
        }
        else
        {
            collisionCounter = 0;
        }
    }
    // Limiting coordinates
    if (mtx.Pos().x < -0.5f * wdmObjects->worldSizeX)
        mtx.Pos().x = -0.5f * wdmObjects->worldSizeX;
    if (mtx.Pos().x > 0.5f * wdmObjects->worldSizeX)
        mtx.Pos().x = 0.5f * wdmObjects->worldSizeX;
    if (mtx.Pos().z < -0.5f * wdmObjects->worldSizeZ)
        mtx.Pos().z = -0.5f * wdmObjects->worldSizeZ;
    if (mtx.Pos().z > 0.5f * wdmObjects->worldSizeZ)
        mtx.Pos().z = 0.5f * wdmObjects->worldSizeZ;
    // Update trail
    UpdateWaterMark(dltTime);
}

void WdmShip::LRender(VDX9RENDER *rs)
{
    if (wmtexture >= 0 && isWMRender)
    {
        isWMRender = false;
        // draw trail
        float al = alpha * 255.0f;
        if (al < 0.0f)
            al = 0.0f;
        if (al > 255.0f)
            al = 255.0f;
        // Vertices
        static Vertex vrt[WDM_SHIP_WMSZ * 2 + 1];
        const float y = 0.01f;
        uint32_t color = (static_cast<uint32_t>(al) << 16) | (static_cast<uint32_t>(al) << 8) |
                         (static_cast<uint32_t>(al) << 0) | 0xff000000;
#ifdef STORM_METAL_RENDERER_CONSUMERS
        rs->TextureSet(0, wmtexture);
        static CMatrix wmIdentity;
        rs->SetTransform(storm_render::TransformWorld, wmIdentity);
        const WdmMetalShipWake compact{mtx.Pos(), lines, WDM_SHIP_WMSZ, y, baseV, al,
                                       WDM_SHIP_SPEED / 5.0f};
        if (WdmMetalDrawShipWake(rs, compact, "WdmShipWaterTrack"))
            return;
#endif
        vrt[0].x = mtx.Pos().x + (lines[0].x - mtx.Pos().x) * 1.2f;
        vrt[0].y = y;
        vrt[0].z = mtx.Pos().z + (lines[0].z - mtx.Pos().z) * 1.2f;
        vrt[0].color = color;
        vrt[0].tu = 0.5f;
        vrt[0].tv = 0.0f;
        for (int32_t i = 0; i < WDM_SHIP_WMSZ; i++)
        {
            float k = (WDM_SHIP_SPEED / 5.0f) * powf((i + 1) * 1.0f / WDM_SHIP_WMSZ, 0.4f) * lines[i].size;
            const float dx = -cosf(lines[i].ay) * k;
            const float dz = sinf(lines[i].ay) * k;
            k = (1.0f - powf(i * 1.0f / (WDM_SHIP_WMSZ - 1.0f), 0.3f)); //*lines[i].size;
            k = al * k;
            if (k < 0.0f)
                k = 0.0f;
            if (k > 255.0f)
                k = 255.0f;
            color = static_cast<uint32_t>(k);
            color |= 0xff000000 | (color << 16) | (color << 8) | color;
            vrt[i * 2 + 1].x = lines[i].x - dx;
            vrt[i * 2 + 1].y = y;
            vrt[i * 2 + 1].z = lines[i].z - dz;
            vrt[i * 2 + 1].color = color;
            vrt[i * 2 + 1].tu = 0.0f;
            vrt[i * 2 + 1].tv = baseV + (i + 1) * (1.0f / WDM_SHIP_WMSZ);
            vrt[i * 2 + 2].x = lines[i].x + dx;
            vrt[i * 2 + 2].y = y;
            vrt[i * 2 + 2].z = lines[i].z + dz;
            vrt[i * 2 + 2].color = color;
            vrt[i * 2 + 2].tu = 1.0f;
            vrt[i * 2 + 2].tv = baseV + (i + 1) * (1.0f / WDM_SHIP_WMSZ);
        }
        // draw a strip
        // Textures
        rs->TextureSet(0, wmtexture);
        // Transform
        static CMatrix identity;
        rs->SetTransform(storm_render::TransformWorld, identity);
        // Render
        rs->DrawPrimitiveUP(storm_render::PrimitiveTriangleStrip, storm_render::vertexFormat::xyz | storm_render::vertexFormat::diffuse | storm_render::vertexFormat::tex1, WDM_SHIP_WMSZ * 2 - 1, vrt,
                            sizeof(Vertex), "WdmShipWaterTrack");
    }
    else
    {
        isWMRender = true;
        // Draw a model
        WdmRenderModel::LRender(rs);
    }
    // wdmObjects->DrawVector(rs, mtx.Pos(), mtx.Pos() + rspeed, 0xff00ff00);
}

void WdmShip::UpdateWaterMark(float dltTime)
{
    const float inrtPos = 5.5f;
    const float inrtAng = 3.5f;
    const float inrtSize = 12.0f;
    const float pi = 3.14159265f;
    float kSpeed = speed * (1.5f / WDM_SHIP_MAX_SPEED);
    if (kSpeed < 0.0f)
        kSpeed = 0.0f;
    if (kSpeed > 0.8f)
        kSpeed = 0.8f;
    lines[0].x = mtx.Pos().x + sinf(ay) * modelL05 * 0.4f * kSpeed;
    lines[0].z = mtx.Pos().z + cosf(ay) * modelL05 * 0.4f * kSpeed;
    lines[0].ay = ay;
    lines[0].size = kSpeed * modelW05 * 0.36f;
    float kp = inrtPos * dltTime;
    if (kp > 1.0f)
        kp = 1.0f;
    float ka = inrtAng * dltTime;
    if (ka > 1.0f)
        ka = 1.0f;
    float ks = inrtSize * dltTime;
    ks *= modelW05 * 1.33f;
    if (ks > 1.0f)
        ks = 1.0f;
    for (int32_t i = 1; i < WDM_SHIP_WMSZ; i++)
    {
        lines[i].x += (lines[i - 1].x - lines[i].x) * kp;
        lines[i].z += (lines[i - 1].z - lines[i].z) * kp;
        while (lines[i - 1].ay - lines[i].ay > pi)
            lines[i].ay += 2 * pi;
        while (lines[i - 1].ay - lines[i].ay < -pi)
            lines[i].ay -= 2 * pi;
        lines[i].ay += (lines[i - 1].ay - lines[i].ay) * ka;
        float kSize = 1.0f - i / static_cast<float>(WDM_SHIP_WMSZ);
        kSize = kSize * kSize * kSpeed * 0.6f;
        lines[i].size += (lines[i - 1].size - lines[i].size + kSize) * ks;
    }
}

// true if free
bool WdmShip::CheckPosition(float x, float z, float objRadius, bool checkShips)
{
    // World
    if (x - objRadius < -0.5f * wdmObjects->worldSizeX)
        return false;
    if (x + objRadius > 0.5f * wdmObjects->worldSizeX)
        return false;
    if (z - objRadius < -0.5f * wdmObjects->worldSizeZ)
        return false;
    if (z + objRadius > 0.5f * wdmObjects->worldSizeZ)
        return false;
    // The ships
    int32_t i;
    for (i = 0; checkShips && i < wdmObjects->ships.size(); i++)
    {
        if (!wdmObjects->ships[i]->isLive)
            continue;
        if (wdmObjects->ships[i]->killMe)
            continue;
        const float dx = wdmObjects->ships[i]->mtx.Pos().x - x;
        const float dz = wdmObjects->ships[i]->mtx.Pos().z - z;
        const float clearance = objRadius + wdmObjects->ships[i]->modelRadius;
        if (dx * dx + dz * dz < clearance * clearance)
            return false;
    }
    // Islands
    if (!checkShips || i >= wdmObjects->ships.size())
    {
        if (wdmObjects->islands)
        {
            if (!wdmObjects->islands->ObstacleTest(x, z, objRadius))
                return true;
        }
        else
            return true;
    }
    return false;
}
