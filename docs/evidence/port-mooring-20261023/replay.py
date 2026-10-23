import os,pathlib,subprocess,time,json
root=pathlib.Path.cwd(); work=pathlib.Path(os.environ['TASK_SCRATCH_DIR'])
ship='src/gameplay/PROGRAM/sea_ai/AIShip.c'
coords=(root/'src/gameplay/PROGRAM/worldmap/worldmap_coords.c').read_text()
helpers=coords[coords.index('bool Sea_HasAtlasFrame()'):coords.index('float Sea_RegionHeading(')]
before=subprocess.check_output(['git','show','26b8405a1e58b8ef1511621a8c915b9b5356c3ad:'+ship],text=True)
after=(root/ship).read_text()
prelude='''#define MIN_ENEMY_DISTANCE_TO_DISABLE_ENTER_2_LOCATION 100
object AISea; object worldMap; object island; object pchar;
bool bCanEnterToLand; string sIslandID; string sIslandLocator;
aref arIslandReload; bool GlobalOpen=true; bool LocalOpen=true; bool IsFort=false;
int GetSeaToMapScale(){return 10;}
bool Island_isReloadEnableGlobal(string id){return GlobalOpen;}
bool Island_isReloadEnableLocal(string id,string name){return LocalOpen;}
bool Island_isReloadFort(string id,string name){return IsFort;}
'''
driver='''void Evaluate(float x,float z,float enemy){
 ref rIsland=&island; string sPlayerLocation="Test"; int iIslandIndex=0;
 float fMinEnemyDistance=enemy; bCanEnterToLand=false; sIslandLocator="";
 BLOCK
}
void SetPort(){
 worldMap.zeroX=0; worldMap.zeroZ=0;
 island.id="Test"; island.reload.p.name="port"; island.reload.p.go="town";
 island.reload.p.x=100; island.reload.p.z=200; island.reload.p.radius=20;
}
int Run(){
 SetPort();
 // Ordinary unframed sea keeps its local coordinate contract.
 Evaluate(100,200,1000); if(!bCanEnterToLand) return 1;
 // Existing resident v1 translation, independent expected point (10100,-19800).
 AISea.worldFrame.version=1; AISea.worldFrame.scale=10;
 AISea.worldFrame.zeroX=0; AISea.worldFrame.zeroZ=0;
 AISea.worldFrame.regions.count=1; AISea.worldFrame.regions.r0.id="Test";
 AISea.worldFrame.regions.r0.mapX=1000; AISea.worldFrame.regions.r0.mapZ=-2000;
 Evaluate(10100,-19800,1000); if(!bCanEnterToLand) return 2;
 // Atlas rotation+translation: (-z+10000,x-20000) -> (9800,-19900).
 AISea.worldFrame.version=2;
 AISea.worldFrame.sea_to_atlas.a=0;
 AISea.worldFrame.atlas_to_sea.a=0; AISea.worldFrame.atlas_to_sea.b=-1;
 AISea.worldFrame.atlas_to_sea.c=1; AISea.worldFrame.atlas_to_sea.d=0;
 AISea.worldFrame.atlas_to_sea.tx=10000; AISea.worldFrame.atlas_to_sea.tz=-20000;
 AISea.worldFrame.regions.r0.local_to_atlas.a=1;
 AISea.worldFrame.regions.r0.local_to_atlas.b=0;
 AISea.worldFrame.regions.r0.local_to_atlas.c=0;
 AISea.worldFrame.regions.r0.local_to_atlas.d=1;
 AISea.worldFrame.regions.r0.local_to_atlas.tx=0;
 AISea.worldFrame.regions.r0.local_to_atlas.tz=0;
 Evaluate(9800,-19900,1000); if(!bCanEnterToLand) return 3;
 if(sIslandLocator!="port" || pchar.NearestLandLocatorInRadius!="town") return 4;
 Evaluate(9800,-19879,1000); if(bCanEnterToLand) return 5;
 Evaluate(9800,-19880,1000); if(bCanEnterToLand) return 6;
 Evaluate(9800,-19900,50); if(bCanEnterToLand) return 7;
 GlobalOpen=false; Evaluate(9800,-19900,1000); if(bCanEnterToLand) return 8;
 GlobalOpen=true; LocalOpen=false; Evaluate(9800,-19900,1000); if(bCanEnterToLand) return 9;
 LocalOpen=true; IsFort=true; Evaluate(9800,-19900,1000); if(bCanEnterToLand) return 10;
 IsFort=false; DeleteAttribute(&island,"reload.p.x");
 Evaluate(9800,-19900,1000); if(bCanEnterToLand) return 11;
 return 0;
}
int Bench(){
 SetPort();
 AISea.worldFrame.version=1; AISea.worldFrame.scale=10;
 AISea.worldFrame.zeroX=0; AISea.worldFrame.zeroZ=0;
 AISea.worldFrame.regions.count=27;
 for(int r=0;r<27;r++){
  string key="r"+r; AISea.worldFrame.regions.(key).id="Other";
 }
 AISea.worldFrame.regions.r26.id="Test";
 AISea.worldFrame.regions.r26.mapX=1000; AISea.worldFrame.regions.r26.mapZ=-2000;
 for(int i=0;i<10000;i++) Evaluate(50000,50000,1000);
 if(bCanEnterToLand) return 99;
 return 0;
}
'''
probe=root/'experiments/native-metal/.cache/build/bin/script-vm-probe'
results={}
for mode,text in [('baseline',before),('current',after)]:
 block=text[text.index('// check reload to locations possibility'):text.index('// disable any abordage if storm')]
 directory=work/mode;directory.mkdir()
 (directory/'cases.c').write_text(prelude+helpers+driver.replace('BLOCK',block))
 row={}
 for method in ('Run','Bench'):
  start=time.perf_counter()
  result=subprocess.run([str(probe),str(directory),method,'0'],cwd=directory,text=True,capture_output=True)
  row[method]={'seconds':time.perf_counter()-start,'exit':result.returncode,'stdout':result.stdout,'stderr':result.stderr}
  errors=directory/'.vm-userdata/Logs/error.log'
  row[method]['script_errors']=errors.read_text(errors='replace') if errors.exists() else ''
 results[mode]=row
print(json.dumps(results,indent=2))
