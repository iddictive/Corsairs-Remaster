object Tornado;
void WhrDeleteTornadoEnvironment()
{
    if (isEntity(&Tornado)) DeleteClass(&Tornado);
    DeleteAttribute(&Tornado,"");
}
void WhrCreateTornadoEnvironment()
{
    // Creation is requested by native meteorology, not by an hourly preset.
}
#event_handler("NativeWeatherTornado","Whr_NativeTornado");
void Whr_NativeTornado()
{
    float seconds=GetEventData();
    if (!bSeaActive || bAbordageStarted) return;
    if (isEntity(&Tornado)) return;
    Tornado.x=10000.0; Tornado.z=10000.0;
    CreateEntity(&Tornado,"Tornado");
    SendMessage(&Tornado,"f",seconds);
    LayerAddObject(SEA_EXECUTE,&Tornado,65535); LayerAddObject(SEA_REALIZE,&Tornado,65535);
}
