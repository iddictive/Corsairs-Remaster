// Isolated diagnostic bootstrap: no main game, menus, SAVE or user controls.
native float Bring2RangeNoCheck(float fMin1, float fMax1, float fMin2, float fMax2, float fValue);
native float Bring2Range(float fMin1, float fMax1, float fMin2, float fMax2, float fValue);
native float Clampf(float fValue);
native float Degree2Radian(float fDegree);
native int RDTSC_B();
native int RDTSC_E(int iRDTSC);

#libriary "script_libriary_test"
object AIBalls,Fleet[16];
#include "native-range-source.c"
void RangeVolley(int kind)
{
    int shooter=kind+2;
    int cannonIndex=kind+14;
    float offset=kind*200.0;
    float distance=stf(Fleet[kind].ship.pos.x);
    Fleet[shooter].Ship.Cannons.Type=cannonIndex;
    Fleet[shooter].Ship.Cannons.Charge.Type=52;
    Fleet[shooter].TmpSkill.Accuracy=1.0;
    Fleet[shooter].TmpSkill.Cannons=1.0;
    Fleet[shooter].perks.list.GunProfessional=1;
    Fleet[shooter].perks.list.LongRangeShoot=1;
    Fleet[shooter].TmpPerks.LongRangeShoot=1;
    float speed=stf(Cannon[cannonIndex].SpeedV0)*stf(Goods[52].SpeedV0)*1.15;
    for (int volley=0; volley<20; volley++) {
        float aimY=8.6+(frnd()-0.5)*4.3*0.3;
        for (int gun=0; gun<51; gun++) {
            float x=stf(RangeGun[gun].x);
            float y=stf(RangeGun[gun].y);
            float z=stf(RangeGun[gun].z)+offset;
            float aimZ=-35.790977478+gun*(69.490001679/50.0)+offset;
            float dx=distance-x;
            float dz=aimZ-z;
            float dy=aimY-y;
            float horizontal=sqrt(dx*dx+dz*dz);
            float b=9.81*dy-speed*speed;
            float disc=b*b-9.81*9.81*(horizontal*horizontal+dy*dy);
            float flight=sqrt((-b-sqrt(disc))/(9.81*9.81/2.0));
            float elevation=atan2(dy+0.5*9.81*flight*flight,horizontal);
            float yaw=atan2(dx,dz);
            float raw=atan2(dy,horizontal);
            Ball_AddBall(&Fleet[shooter],x,y,z,speed,yaw,elevation,0.0,distance,raw);
        }
    }
}

int Probe(){InitRangeSource();Fleet[0].ship.pos.x=2098.377758;Fleet[2].index=1000;RangeVolley(0);if(!CheckAttribute(&AIBalls,"x"))return 1;if(AIBalls.Event!="@dart:52")return 2;return 0;}
