
with open('docs/evidence/autopilot-20261030/save-policy-replay.py', 'r', encoding='utf-8') as f:
    text = f.read()

old_block = '''if os.environ.get('AUTOPILOT_PROBE') == 'hunt':
    cases = cases[:cases.index('int Run(){')] + r'''
int Run(){
 pchar=GetMainCharacter();
 DiagStatus=pchar.Ship.Autopilot.status;DiagReason=pchar.Ship.Autopilot.blocked;
 DiagShip=SeaAutopilot_Mode();'''

new_block = '''if os.environ.get('AUTOPILOT_PROBE') == 'hunt':
    cases = cases[:cases.index('int Run(){')] + r'''
int Run(){
 pchar=GetMainCharacter();
 if(!CheckAttribute(pchar,"Ship.Autopilot.status"))pchar.Ship.Autopilot.status="off";
 if(!CheckAttribute(pchar,"Ship.Autopilot.blocked"))pchar.Ship.Autopilot.blocked="";
 DiagStatus=pchar.Ship.Autopilot.status;DiagReason=pchar.Ship.Autopilot.blocked;
 DiagShip=SeaAutopilot_Mode();'''

assert old_block in text, 'old_block not found in save-policy-replay.py'
text = text.replace(old_block, new_block, 1)

with open('docs/evidence/autopilot-20261030/save-policy-replay.py', 'w', encoding='utf-8') as f:
    f.write(text)
print('Fixed save-policy-replay.py hunt test case!')

