
with open('experiments/native-metal/.cache/CorsairsMetal.app/Contents/Resources/PROGRAM/sea_ai/sea.c', 'w', encoding='utf-8') as f1:
    with open('src/gameplay/PROGRAM/sea_ai/sea.c', 'r', encoding='utf-8') as f2:
        f1.write(f2.read())

with open('/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/PROGRAM/sea_ai/sea.c', 'w', encoding='utf-8') as f1:
    with open('src/gameplay/PROGRAM/sea_ai/sea.c', 'r', encoding='utf-8') as f2:
        f1.write(f2.read())

print('Synchronized sea.c to app and cache!')

