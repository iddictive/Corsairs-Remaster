ref mainHero;
aref heroQuest;
aref rootAttrs;
ref heroAlias;
ref nullHero;
aref nullAttrs;
ref retiredRefs[2];
aref retiredPaths[2];
object RemovedTarget;
ref missingRoot;
aref missingPath;
object Heroes[3];
int revision;
void OnSave(){}
void OnLoad(){SaveEngineState("roundtrip.sav");Trace("RAW="+Heroes[0].id+":"+Heroes[0].hp+":"+Heroes[0].items.blade4+":"+Heroes[2].hp);
 if (mainHero.id!="CodecHero" || sti(mainHero.hp)!=29 || sti(mainHero.items.blade4)!=3 || heroQuest.marker!="quest-kept" || rootAttrs.id!="CodecOfficer" || rootAttrs.quest.marker!="officer-kept" || heroAlias.id!="CodecHero" || sti(Heroes[2].hp)!=0 || revision!=17) Trace("PROBE_FAILED reference matrix");
 heroAlias.hp=31; heroQuest.marker="quest-advanced"; rootAttrs.hp=65;
 if(sti(Heroes[0].hp)!=31 || Heroes[0].quest.marker!="quest-advanced" || sti(Heroes[1].hp)!=65) Trace("PROBE_FAILED canonical reference writes");
 makeref(nullHero,Heroes[2]); makearef(nullAttrs,Heroes[2].quest); nullAttrs.marker="null-rebound";
 if(nullHero.id!="CodecDeadGuard" || Heroes[2].quest.marker!="null-rebound") Trace("PROBE_FAILED null slot rebind");
 Trace("REFERENCE_MATRIX_DONE"); ExitProgram();}
void Main(){LoadEngineState("old.sav");}
