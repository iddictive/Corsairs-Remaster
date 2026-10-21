object Heroes[2];
ref mainHero;
aref questRoot;
void OnSave() {}
void OnLoad() { Trace("RAW=" + Heroes[0].id + ":" + Heroes[1].quest.marker); Trace("REF=" + mainHero.id + ":" + questRoot.marker); Trace("REFERENCE_CONTROL_DONE"); ExitProgram(); }
void Main() { Heroes[0].id="CodecHero"; Heroes[1].quest.marker="retained"; makeref(mainHero,Heroes[0]); makearef(questRoot,Heroes[1].quest); SaveEngineState("old.sav"); Trace("REFERENCE_WROTE"); ExitProgram(); }
