object Lightning;
#define WHR_LIGHTNING "whr_lightn"
#define WHR_LIGHTNING_SOUND "whr_lightn_snd"
#define WHR_LIGHTNING_DOIT "whr_lightn_doit"

void WhrDeleteLightningEnvironment()
{
    if (isEntity(&Lightning)) DeleteClass(&Lightning);
    DeleteAttribute(&Lightning,"");
    DelEventHandler(WHR_LIGHTNING_SOUND,"Lightning_Sound");
    DelEventHandler(WHR_LIGHTNING_DOIT,"Lightning_DoIt");
}
void WhrCreateLightningEnvironment()
{
    DelEventHandler(WHR_LIGHTNING_DOIT,"Lightning_DoIt");
    if (!isEntity(&Lightning)) CreateEntity(&Lightning,"Lightning");
    Lightning.Clear="";
    Lightning.Texture="weather\lightning\lightning_storm.tga";
    Lightning.Flash.Texture="weather\lightning\flash.tga";
    Lightning.SubTexX=4; Lightning.SubTexY=1;
    Lightning.isDone="";
    LayerAddObject(SEA_REFLECTION,&Lightning,10);
    SetEventHandler(WHR_LIGHTNING_SOUND,"Lightning_Sound",0);
}
void MoveLightningToLayers(int executeLayer, int realizeLayer)
{
    LayerDelObject(EXECUTE,&Lightning); LayerDelObject(REALIZE,&Lightning);
    LayerDelObject(SEA_EXECUTE,&Lightning); LayerDelObject(SEA_REALIZE,&Lightning);
    LayerAddObject(executeLayer,&Lightning,-2); LayerAddObject(realizeLayer,&Lightning,-2);
}
#event_handler("NativeWeatherLightning","Lightning_DoIt");
void Lightning_DoIt()
{
    float x=GetEventData(); float y=GetEventData(); float z=GetEventData(); int tile=GetEventData();
    if (!isEntity(&Lightning)) return;
    // Native WEATHER supplies the time, position and atlas tile. This is only an effect adapter.
    SendMessage(&Lightning,"llsflffffffsff",MSG_WHR_LIGHTNING_ADD,tile,"lightning",0.16,32,600.0,0.7,1.0,x,y,z,"flash_lightning",1500.0,0.08);
    float dx=x-stf(Camera.Pos.x); float dz=z-stf(Camera.Pos.z);
    float distance=sqrt(dx*dx+dz*dz);
    PostEvent(WHR_LIGHTNING_SOUND,MakeInt(distance/333.0*1000.0),"fff",x,y,z);
    Event(WHR_LIGHTNING,"fff",x,y,z);
}
void Lightning_Sound()
{
    float x=GetEventData(); float y=GetEventData(); float z=GetEventData();
    Play3DSound("thunder",x,y,z);
}
