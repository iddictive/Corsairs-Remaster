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
void OnLoad(){}
void Main(){Heroes[0].id="CodecHero"; Heroes[0].hp=29; Heroes[0].quest.marker="quest-kept"; Heroes[0].items.blade4=3;
 Heroes[1].id="CodecOfficer"; Heroes[1].quest.marker="officer-kept"; Heroes[1].role="companion"; Heroes[1].hp=63;
 Heroes[2].id="CodecDeadGuard"; Heroes[2].hp=0;
 RemovedTarget.id="removed"; RemovedTarget.quest.marker="removed-path";
 makeref(mainHero,Heroes[0]); makearef(heroQuest,Heroes[0].quest); makearef(rootAttrs,Heroes[1]); makeref(heroAlias,mainHero);
 retiredRefs[0] = &Heroes[1]; retiredRefs[1] = &Heroes[0]; retiredPaths[0] = rootAttrs; retiredPaths[1] = heroQuest;
 makeref(missingRoot,RemovedTarget); makearef(missingPath,RemovedTarget.quest); revision=17;
SaveEngineState("old.sav"); Trace("MATRIX_WRITTEN"); ExitProgram();}
