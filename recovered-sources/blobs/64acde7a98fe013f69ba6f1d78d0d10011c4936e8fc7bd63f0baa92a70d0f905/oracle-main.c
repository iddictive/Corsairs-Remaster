
 InterfaceStates.Launched=false;
 makeref(pchar,Characters[0]);nMainCharacterIndex=0;
 pchar.index=0;pchar.id="OracleHero";pchar.chr_ai.hp=37;pchar.items.blade4=3;
 nLocationsNum=2;
 Locations[0].id="PriorContext";Locations[0].index=0;Locations[0].fixtureLive=1;
 Locations[1].id="SanJuan_town";Locations[1].index=1;Locations[1].fixtureLive=1;
 Locations[1].locators.reload.pier=1;
 SetArraySize(&LAi_loginedcharacters,64);LAi_numloginedcharacters=64;
 for(int i=0;i<64;i++)
 {
  Characters[100+i].index=100+i;Characters[100+i].id="Prior"+i;
  LAi_loginedcharacters[i]=100+i;
 }
 for(i=0;i<80;i++){Characters[200+i].index=200+i;Characters[200+i].id="New"+i;LAi_AddLoginedCharacter(&Characters[200+i]);}
 if(GetArraySize(&LAi_loginedcharacters)!=TOTAL_CHARACTERS || LAi_numloginedcharacters!=144)Trace("PROBE_FAILED registry capacity/count");
 for(i=0;i<64;i++)if(LAi_loginedcharacters[i]!=100+i)Trace("PROBE_FAILED prior registry entry");
 for(i=0;i<80;i++)if(LAi_loginedcharacters[64+i]!=200+i)Trace("PROBE_FAILED appended registry entry");
 LAi_AddLoginedCharacter(&Characters[120]);LAi_AddLoginedCharacter(&Characters[220]);
 if(LAi_numloginedcharacters!=144)Trace("PROBE_FAILED duplicate registry actor");
 Trace("ORACLE registry length64→"+GetArraySize(&LAi_loginedcharacters)+" count144 prior64 retained duplicates no-op");
 LAi_IsInitedAI=true;probeGroupLive=false;probeGroupBuilds=0;LAi_restoreStates=false;actLoadFlag=1;
 LocAi_Init(&Locations[1]);
 if(!probeGroupLive || probeGroupBuilds!=1 || LAi_numloginedcharacters!=0 || !LAi_IsInitedAI || sti(pchar.chr_ai.hp)!=37)Trace("PROBE_FAILED missing group rebuild");
 LAi_numloginedcharacters=3;LAi_loginedcharacters[0]=101;LAi_loginedcharacters[1]=102;LAi_loginedcharacters[2]=103;
 LAi_restoreStates=true;LAi_grp_relations.sentinel=777;
 LocAi_Init(&Locations[1]);
 if(probeGroupBuilds!=1 || LAi_numloginedcharacters!=3 || LAi_loginedcharacters[0]!=101 || LAi_loginedcharacters[2]!=103 || !LAi_restoreStates || sti(LAi_grp_relations.sentinel)!=777)Trace("PROBE_FAILED live group context preserve");
 Trace("ORACLE missing group rebuilt; live group/registry/restore flag retained");
 object expectedAddress;aref address,expectedTree; makearef(expectedTree,expectedAddress.location);
 for(int stage=1;stage<=4;stage++)
 {
  Probe_SeedPrior();probeFailStage=stage;
  makearef(address,pchar.location);CopyAttributes(expectedTree,address);
  bool result=Port_CreateLandHero(1,"reload","pier",true);
  makearef(address,pchar.location);
  if(result || !Probe_SameTree(address,expectedTree) || sti(pchar.fixtureEid)!=10 || probeActiveContext!=0 || loadedLocation.id!="PriorContext" || pchar.PortTravel.mode!="boat" || probeCameraWrites!=7 || bCanSwitchCameras || !bSeaActive || probeActivateCalls!=0 || probePostLoginCalls!=0)Trace("PROBE_FAILED rollback state stage"+stage+" result="+result+" sameTree="+Probe_SameTree(address,expectedTree)+" eid="+pchar.fixtureEid+" context="+probeActiveContext+" loaded="+loadedLocation.id+" mode="+pchar.PortTravel.mode+" camera="+probeCameraWrites+" canSwitch="+bCanSwitchCameras+" sea="+bSeaActive+" activated="+probeActivateCalls+" post="+probePostLoginCalls);
  if((stage==1 && probeDeletedActors!=0) || (stage>1 && (probeDeletedActors!=1 || probeDeletedEid!=20)))Trace("PROBE_FAILED rollback deletion stage"+stage);
  if(sti(pchar.chr_ai.hp)!=37 || sti(pchar.items.blade4)!=3)Trace("PROBE_FAILED canonical actor mutation stage"+stage);
  Trace("ORACLE rollback stage"+stage+" EID10 context0 address exact mode/camera retained disposable="+probeDeletedEid);
 }
 Probe_SeedPrior();probeFailStage=0;
 if(!Port_CreateLandHero(1,"reload","pier",true) || pchar.location!="SanJuan_town" || sti(pchar.fixtureEid)!=20 || probeActiveContext!=1 || loadedLocation.id!="SanJuan_town" || pchar.PortTravel.mode!="land" || bSeaActive || probeCameraWrites!=8 || bCanSwitchCameras || probeDeletedActors!=0 || probePostLoginCalls!=1 || probeActivateCalls!=1)Trace("PROBE_FAILED normal success");
 Trace("ORACLE normal success EID20 target context mode land committed once");
 Trace("PORT_RESTORATION_ORACLES_FINISHED");ExitProgram();
