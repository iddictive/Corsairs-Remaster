from pathlib import Path
import subprocess, re, tempfile
repo=Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
temp=Path(tempfile.mkdtemp(prefix='vm-',dir=Path(__file__).parent))
source=(repo/'src/gameplay/PROGRAM/sea_ai/AIShip.c').read_text()
def extract(name):
    start=re.search(r'^(?:float|void) '+re.escape(name)+r'\(',source,re.M).start()
    at=start
    body=source.index('{',at)
    end=body+1;level=1
    while level:
        level+=(source[end]=='{')-(source[end]=='}');end+=1
    return source[start:end]
prelude='''object Characters[2];
bool FixturePhonk=false;
float FixtureScale=1.0;
int FixtureSounds=0;
float FixtureResult=0.0;
#define KILL_BY_ISLAND 5
void OnLoad(){}
bool SeaPhonk_IsActive(){return FixturePhonk;}
int GetMainCharacterIndex(){return 0;}
ref GetCharacter(int index){return &Characters[index];}
float Clampf(float x){if(x<0.0)return 0.0;if(x>1.0)return 1.0;return x;}
float Ship_HullDamageMultiplier(ref target,int owner){return 1.3;}
void Ship_ApplyCollisionHitpoints(ref target,float damage,int cause,int owner){target.Ship.HP=stf(target.Ship.HP)-damage*1.3*FixtureScale;}
void Play3DSound(string name,float x,float y,float z){FixtureSounds++;}
'''
cases=prelude+'\n'.join(extract(name) for name in ['Ship_GetHP','Ship_ContactMaterialFraction','Ship_Ship2IslandCollision'])+'''
void OnShore(){FixtureResult=Ship_Ship2IslandCollision();}
int Run(){
 Characters[0].index=0;Characters[0].Ship.HP=1000;
 Characters[1].index=1;Characters[1].Ship.HP=1000;
 SetEventHandler("shore","OnShore",0);
 Event("shore","llfflfff",0,-1,10.0,0.0,1,5.0,0.0,2.0);
 float value=FixtureResult;
 if(abs(value-1.0)>0.00001||abs(stf(Characters[0].Ship.HP)-961.0)>0.001)return 1;
 FixturePhonk=true;FixtureScale=0.4;
 Event("shore","llfflfff",0,-1,10.0,0.0,1,5.0,0.0,2.0);value=FixtureResult;
 if(abs(value-0.4)>0.00001)return 2;
 FixtureScale=0.0;
 Event("shore","llfflfff",0,-1,10.0,0.0,1,5.0,0.0,2.0);value=FixtureResult;
 if(value!=0.0)return 3;
 float before=stf(Characters[0].Ship.HP);
 Characters[0].AlwaysSandbankManeuver=true;FixtureScale=1.0;
 Event("shore","llfflfff",0,-1,1000.0,0.0,1,5.0,0.0,2.0);value=FixtureResult;
 if(value!=0.0||stf(Characters[0].Ship.HP)!=before)return 4;
 Event("shore","llfflfff",1,-1,10.0,0.0,1,5.0,0.0,2.0);value=FixtureResult;
 if(value!=1.0||abs(stf(Characters[1].Ship.HP)-961.0)>0.001)return 5;
 return 0;
}
int Loaded(){if(!CheckAttribute(&Characters[0],"AlwaysSandbankManeuver"))return 1;return Run();}
'''
# Script Event is void; native return capture belongs to the C++ integration
# driver. Keep this VM oracle on the actual shared admission helper plus a
# compile of the complete shore handler and the normal saved-attribute codec.
cases=prelude+'\n'.join(extract(name) for name in ['Ship_GetHP','Ship_ContactMaterialFraction','Ship_Ship2IslandCollision'])+'''
int Run(){
 Characters[0].index=0;Characters[0].Ship.HP=961.0;
 Characters[1].index=1;Characters[1].Ship.HP=961.0;
 ref hero=GetCharacter(0);
 ref npc=GetCharacter(1);
 FixturePhonk=false;
 if(Ship_ContactMaterialFraction(hero,1000.0,30.0,-1)!=1.0)return 1;
 FixturePhonk=true;hero.Ship.HP=984.4;
 if(abs(Ship_ContactMaterialFraction(hero,1000.0,30.0,-1)-0.4)>0.00001)return 2;
 hero.Ship.HP=1000.0;
 if(Ship_ContactMaterialFraction(hero,1000.0,30.0,-1)!=0.0)return 3;
 if(Ship_ContactMaterialFraction(hero,1000.0,0.0,-1)!=0.0)return 4;
 if(Ship_ContactMaterialFraction(npc,1000.0,30.0,-1)!=1.0)return 5;
 hero.Ship.PhonkFlood.volume=100;
 return 0;
}
int Loaded(){if(Characters[0].Ship.PhonkFlood.volume!="100")return 10;return Run();}
'''
(temp/'cases.c').write_text(cases)
binary=repo/'experiments/native-metal/.cache/build/bin/script-vm-probe'
subprocess.run([str(binary),str(temp),'Run','1','Loaded'],cwd=repo,check=True)
