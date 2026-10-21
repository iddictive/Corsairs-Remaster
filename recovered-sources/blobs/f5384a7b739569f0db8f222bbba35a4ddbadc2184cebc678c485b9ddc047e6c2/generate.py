from pathlib import Path
import re, json, hashlib

out = Path(__file__).resolve().parent
installed = Path('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/PROGRAM/weather')
def func(src, name):
    match = re.search(r'(?m)^[a-z]+ ' + re.escape(name) + r'\(', src)
    assert match, name
    return src[match.start():src.index('\n}', match.start()) + 2]

# Installed legacy files have mixed comment encodings. Tested declarations,
# expressions and panorama paths are ASCII; latin1 preserves every input byte.
weather = (installed / 'WhrWeather.c').read_text(encoding='latin1')
utils = (installed / 'WhrUtils.c').read_text(encoding='latin1')
sky_path = Path('src/gameplay/PROGRAM/weather/WhrSky.c')
sky = sky_path.read_text(encoding='utf-8-sig')
slots, files = {}, {}
for name in ('Night.c', 'Morning.c', 'Day.c', 'Evening.c'):
    path = installed / 'Init' / name
    raw = path.read_bytes()
    files[str(path)] = hashlib.sha256(raw).hexdigest()
    for block in re.split(r'Weathers\[n\]\.Hour\.Min\s*=\s*', raw.decode('latin1'))[1:]:
        hour = int(re.match(r'\d+', block).group())
        directory = re.search(r'Weathers\[n\]\.Sky\.Dir\s*=\s*("[^\n]+?");', block).group(1)
        color = tuple(map(int, re.search(r'Weathers\[n\]\.Sky\.Color\s*=\s*argb\(\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(\d+)\s*\)', block).groups()))
        if hour < 24:
            assert hour not in slots
            slots[hour] = directory, color
assert sorted(slots) == list(range(24)), sorted(slots)
tint = sky[sky.index('int nextWeather ='):sky.index('Sky.Color = skyColor;') + len('Sky.Color = skyColor;')]
prelude = '''#define true 1
#define false 0
#define MAX_WEATHERS 34
#define EXECUTE 0
#define REALIZE 1
#define SEA_EXECUTE 2
#define SEA_REALIZE 3
#define SEA_REFLECTION 4
object Weather;
object Weathers[MAX_WEATHERS];
int iCurWeatherNum;
int iTotalNumWeathers = 24;
int bSeaActive;
float TimeScaleCounter;
float FixtureTime;
float GetTime(){return FixtureTime;}
float Whr_GetWindSpeed(){return 5.0;}
float Whr_GetFloat(aref base,string key){return stf(base.(key));}
'''
helpers = '\n'.join(func(weather, name) for name in ('GetCurrentWeather', 'FindWeatherByHour', 'FindBlendWeather', 'Whr_BlendColor')) + '\n' + func(utils, 'Whr_GetColor')
setup = '\n'.join(f'Weathers[{h}].Hour.Min={h}; Weathers[{h}].Sky.Dir={d}; Weathers[{h}].Sky.Color=argb({",".join(map(str,c))});' for h, (d, c) in sorted(slots.items()))
checks = []
for n, (time, hour) in enumerate([(2., 2), (14., 14), (20., 20), (23.5, 23), (0., 0), (5.5, 5)]):
    rgb = [a + int((time - int(time)) * (b - a)) for a, b in zip(slots[hour][1][1:], slots[(hour + 1) % 24][1][1:])]
    checks.append(f'FixtureTime={time}; iCurWeatherNum={hour}; FixtureTint(); if(sti(Sky.Color)!=argb(0,{",".join(map(str,rgb))}))return {10+n};')
source = prelude + helpers + '\n' + sky + '\nvoid FixtureTint(){\n' + tint + '\n}\nint VerifySky(){\n' + setup + '''
FixtureTime=14.25; FillSkyDir(&Sky);
aref dirs; makearef(dirs,Sky.Dir);
if(GetAttributesNum(dirs)!=24)return 1;
for(int h=0;h<24;h++){string key="d"+h; if(!CheckAttribute(dirs,key))return 2;}
''' + '\n'.join(checks) + '\nreturn 0;\n}\n'
(out / 'cases.c').write_text(source)
(out / 'inputs.json').write_text(json.dumps({'fixture': 'actual full WhrSky compiler input; exact tint block executed without renderer entity; 24 installed hourly presets and shared color/hour helpers', 'source_sha256': hashlib.sha256(sky_path.read_bytes()).hexdigest(), 'slots': slots, 'inputs': files}, indent=2) + '\n')
print('24 original hourly slots; six tint transitions; no renderer/game scene')
