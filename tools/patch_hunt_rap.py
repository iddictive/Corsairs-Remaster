with open('docs/evidence/autopilot-20261030/save-policy-replay.py', 'r', encoding='utf-8') as f:
    text = f.read()

old_hunt = '''if os.environ.get('AUTOPILOT_PROBE') == 'hunt':\n    cases = cases[:cases.index('int Run(){')] + r'''\nint Run(){\n pchar=GetMainCharacter();'''
new_hunt = '''if os.environ.get('AUTOPILOT_PROBE') == 'hunt':\n    cases = cases[:cases.index('int Run(){')] + r'''\nint Run(){\n pchar=GetMainCharacter();\n makearef(rAP, pchar.Ship.Autopilot);\n pchar.Ship.Autopilot.status="off";\n pchar.Ship.Autopilot.blocked="";\n pchar.Ship.Autopilot.mode=0;'''
assert old_hunt in text, 'old_hunt not found'
text = text.replace(old_hunt, new_hunt, 1)
with open('docs/evidence/autopilot-20261030/save-policy-replay.py', 'w', encoding='utf-8') as f:
    f.write(text)
print('Fixed rAP creation in save-policy-replay.py!')
