from pathlib import Path
import subprocess,json,os
ROOT=Path(__file__).resolve().parents[3]; OUT=Path(os.environ['CODEX_SCRATCH_DIR'])
def function(path, signature):
    text=(ROOT/path).read_text(); start=text.index(signature); at=text.index('{',start)+1; depth=1
    while depth:
        depth+=(text[at]=='{')-(text[at]=='}'); at+=1
    return text[start:at]
source='''
#define MSG_CHARACTER_EX_MSG 0
object victim, attacker;
float LAi_GetCharacterHP(aref ch){return stf(ch.chr_ai.hp);}
float LAi_GetCharacterMaxHP(aref ch){return stf(ch.chr_ai.hp_max);}
bool LAi_IsDead(aref ch){return stf(ch.chr_ai.hp)<1.0;}
bool LAi_IsImmortal(aref ch){return CheckAttribute(ch,"chr_ai.immortal") && sti(ch.chr_ai.immortal);}
void LAi_ProcessCheckMinHP(aref ch){}
'''
source+=function('src/gameplay/PROGRAM/Loc_ai/LAi_character.c','void LAi_UpdateBleeding(')+'\n'
source+=function('src/gameplay/PROGRAM/Loc_ai/LAi_fightparams.c','void LAi_RecordWeaponWound(')+'\n'
source+='''
int RunBleeding(){
 victim.chr_ai.hp_max=100.0; victim.chr_ai.hp=100.0;
 LAi_UpdateBleeding(&victim,1.0); if(stf(victim.chr_ai.hp)!=100.0)return 1;
 victim.chr_ai.hp=93.0; LAi_RecordWeaponWound(&attacker,&victim,100.0,1);
 if(CheckAttribute(&victim,"chr_ai.bleeding"))return 2;
 victim.chr_ai.hp=92.0; LAi_RecordWeaponWound(&attacker,&victim,100.0,2);
 if(stf(victim.chr_ai.bleeding.remaining)!=90.0)return 3;
 LAi_UpdateBleeding(&victim,1.0); if(abs(stf(victim.chr_ai.hp)-90.16)>0.001)return 4;
 victim.chr_ai.hp=100.0; LAi_UpdateBleeding(&victim,1.0);
 if(abs(stf(victim.chr_ai.hp)-98.0)>0.001 || stf(victim.chr_ai.bleeding.remaining)!=88.0)return 5;
 LAi_RecordWeaponWound(&attacker,&victim,110.0,1);
 if(stf(victim.chr_ai.bleeding.remaining)!=90.0 || stf(victim.chr_ai.bleeding.rate)!=0.02)return 6;
 victim.chr_ai.hp=100.0; LAi_UpdateBleeding(&victim,90.0);
 if(abs(stf(victim.chr_ai.hp)-16.231)>0.01 || CheckAttribute(&victim,"chr_ai.bleeding"))return 7;
 victim.chr_ai.hp=1.01; victim.chr_ai.bleeding.remaining=90.0;
 LAi_UpdateBleeding(&victim,90.0); if(stf(victim.chr_ai.hp)!=1.0)return 8;
 victim.chr_ai.hp=0.0; victim.chr_ai.bleeding.remaining=90.0;
 LAi_UpdateBleeding(&victim,1.0); if(CheckAttribute(&victim,"chr_ai.bleeding"))return 9;
 victim.chr_ai.hp=100.0; victim.chr_ai.bleeding.remaining=90.0; victim.chr_ai.immortal=1;
 LAi_UpdateBleeding(&victim,1.0); if(stf(victim.chr_ai.hp)!=100.0 || CheckAttribute(&victim,"chr_ai.bleeding"))return 10;
 victim.chr_ai.immortal=0; victim.chr_ai.bleeding.remaining=90.0;
 LAi_UpdateBleeding(&victim,0.0); if(stf(victim.chr_ai.bleeding.remaining)!=90.0)return 11;
 victim.chr_ai.hp_max=500.0; victim.chr_ai.hp=470.0; DeleteAttribute(&victim,"chr_ai.bleeding");
 LAi_RecordWeaponWound(&attacker,&victim,500.0,1); if(CheckAttribute(&victim,"chr_ai.bleeding"))return 12;
 return 0;
}
'''
(OUT/'cases.c').write_text(source)
r=subprocess.run([str(ROOT/'experiments/native-metal/.cache/build/bin/script-vm-probe'),str(OUT),'RunBleeding','0'],capture_output=True,text=True)
result=dict(exit=r.returncode,stdout=r.stdout,stderr=r.stderr)
(OUT/'result.json').write_text(json.dumps(result,indent=2));print(json.dumps(result));raise SystemExit(r.returncode)
