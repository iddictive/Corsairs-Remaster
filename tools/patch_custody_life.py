"""Exact-input Metal-only custody layer; never mutates the archived baseline."""
from pathlib import Path
import hashlib

SOURCE = Path(__file__).with_name('gameplay') / 'custody_life.c'
BASE = {
    'PROGRAM/scripts/custody.c': '86471b60239f60035271a4d033a341033f6a25e7c45d45714db3014b4a5b9ffe',
    'PROGRAM/dialogs/russian/Common_Prison.c': 'fbe34cd9460157870f2754c3037a4b6c5949bedfb5833ea844bcd96209ba4b64',
    'PROGRAM/scripts/GoldFleet.c': '1ea9fc5a3d95983fc6ac4b664de90541ae1324b60bca187666441ed134631c3e',
    'PROGRAM/quests/quests_reaction.c': '4ef98d00aa9ae4b0eef913c817e62ec6f0db0b53292f20633a897128a8bd0de6',
}
# Installed inputs are distinct from the layer inputs, after earlier Metal layers.
INSTALLED = dict(BASE)
BASELINE = {
    **BASE,
    'PROGRAM/scripts/custody.c': 'b434f09a612bdd154239a7a8afae08e8d213e0982effed45926c8fd903aa4b1f',
    'PROGRAM/quests/quests_reaction.c': 'b435b2fbc99346d06a7376b3f02de5fec306fa65da096099a0f092b971414514',
}
UPDATED = {
    'PROGRAM/scripts/custody.c': 'b764435349c4c2e8f2c408b5ae018601e32fd1a1cfc6507adc702d4ebe2c377b',
    'PROGRAM/dialogs/russian/Common_Prison.c': '2088d0a7b75d720541a97155d2864a8854eb01fa77d088f5bd3d5e310a075520',
    'PROGRAM/scripts/GoldFleet.c': 'c1a124264892cdd72bad8b3a8ab31f0ffc4a094bfb7df07e0d03707ea2a6b8db',
    'PROGRAM/quests/quests_reaction.c': '8d8169d2f22e550c64aa5c1771fa035bcf518945cf6508c1f596b50e23242ffd',
}


def digest(data):
    return hashlib.sha256(data).hexdigest()


def replace(data, old, new):
    old = old.replace('\n', '\r\n').encode()
    new = new.replace('\n', '\r\n').encode()
    if data.count(old) != 1:
        raise ValueError(f'custody layer: expected one anchor {old[:90]!r}, got {data.count(old)}')
    return data.replace(old, new, 1)


DIALOG = r'''
        case "CustodyLife_Daily":
            dialog.text = "Один выбор займёт день. Перед боем лучше поесть или отлежаться.";
            link.l1 = "Отдохнуть и залечить ушибы."; link.l1.go = "Custody_Quiet";
            link.l2 = "Поработать за 60 пиастров. Охрана оценит, заключённые — нет."; link.l2.go = "CustodyLife_Work";
            link.l3 = "Потренироваться с сокамерником."; link.l3.go = "Custody_Resist";
            if (sti(pchar.money) >= 75) { link.l4 = "Купить еду и отлежаться: 75 пиастров."; link.l4.go = "CustodyLife_Food"; }
            if (sti(pchar.money) >= 50) { link.l5 = "Сыграть в кости. Рискнуть 50 пиастрами."; link.l5.go = "CustodyLife_Dice"; }
            link.l6 = "Назад."; link.l6.go = "Custody_Cellmate";
        break;
        case "CustodyLife_Authority":
            dialog.text = "Работа улучшает отношения с охраной и начальником. При расположении охраны от 50 прошение дойдёт до начальника. При расположении начальника 100 тебя выпустят.";
            link.l1 = "Потратить день на прошение об освобождении."; link.l1.go = "CustodyLife_Petition";
            link.l2 = "Выдать тайник. Начальство оценит, авторитет упадёт на 20."; link.l2.go = "CustodyLife_Inform";
            link.l3 = "Назад."; link.l3.go = "Custody_Cellmate";
        break;
        case "CustodyLife_Tournament":
            CustodyLife_Ensure();
            dialog.text = "Караул водит нас на нижнюю площадку форта. Обоим выдают одинаковые пехотные сабли. Бой до сдачи, оружие потом забирают. После каждого боя — обратно в камеру. Победа даёт 100 пиастров и авторитет; три победы — вольную. Проиграешь — потеряешь день и здоровье, но останешься жив. Твои победы: " + pchar.Custody.Life.Wins + "/3.";
            if (LAi_GetCharacterHP(pchar) >= LAi_GetCharacterMaxHP(pchar) * 0.5 && !CheckAttribute(pchar, "chr_ai.hpchecker"))
            {
                link.l1 = "Записаться. Пусть караул ведёт во двор."; link.l1.go = "CustodyLife_Start";
            }
            else dialog.text += " Сейчас тебя не допустят: нужно восстановить хотя бы половину здоровья и закончить другие испытания.";
            link.l2 = "Сначала подготовлюсь."; link.l2.go = "Custody_Cellmate";
        break;
        case "CustodyLife_Start":
            DialogExit();
            AddDialogExitQuestFunction("CustodyLife_StartAfterDialog");
        break;
'''
for node, action in [('Work', 'work'), ('Food', 'food'), ('Dice', 'dice'), ('Petition', 'petition'), ('Inform', 'inform')]:
    DIALOG += f'''
        case "CustodyLife_{node}":
            CustodyLife_Action("{action}");
            dialog.text = pchar.Custody.LastResult;
            link.l1 = "Дальше."; link.l1.go = "Custody_Cellmate";
        break;
'''

EXTERNS = '''
extern void CustodyLife_Ensure();
extern void CustodyLife_Action(string action);
extern bool CustodyLife_Menu(ref inmate, aref Link);
extern void CustodyLife_Arrived();
extern void CustodyLife_Finish(bool won);
extern void CustodyLife_Returned();
'''
QUESTS = '''
        case "CustodyLife_Arrived": CustodyLife_Arrived(); break;
        case "CustodyLife_Won": CustodyLife_Finish(true); break;
        case "CustodyLife_Lost": CustodyLife_Finish(false); break;
        case "CustodyLife_Returned": CustodyLife_Returned(); break;
'''


def transform(outputs):
    result = dict(outputs)
    for path, expected in BASE.items():
        if digest(result[path]) != expected:
            raise ValueError(f'custody layer: unreviewed input {path}: {digest(result[path])}')
    path = 'PROGRAM/scripts/custody.c'
    data = result[path]
    start = data.index(b'void Custody_SpendDay(string action)')
    end = data.index(b'void Custody_RehydrateJail(aref loc)', start)
    data = data[:start] + b'void Custody_SpendDay(string action)\r\n{\r\n\tCustodyLife_Action(action);\r\n}\r\n\r\n' + data[end:]
    data = replace(data, 'void Custody_RehydrateJail(aref loc)\n{',
                   'void Custody_RehydrateJail(aref loc)\n{\n\tif (CustodyLife_Recover(loc)) return;')
    start = data.index(b'bool Custody_RoutePrisonDialog(ref inmate, aref Link)')
    end = data.index(b'void Custody_Release()', start)
    data = data[:start] + b'bool Custody_RoutePrisonDialog(ref inmate, aref Link)\r\n{\r\n\treturn CustodyLife_Menu(inmate, Link);\r\n}\r\n\r\n' + data[end:]
    data = replace(data, '\tif (sti(pchar.Custody.DaysRemaining) > 0) return;',
                   '\tif (sti(pchar.Custody.DaysRemaining) > 0) return;\n\tif (CheckAttribute(pchar, "Custody.Bout")) return;\n\tif (pchar.location != pchar.Custody.Jail) return;')
    data = replace(data, '\tCustody_MoveFleetTo(destination);\n\tCustody_RestoreEscrow();',
                   '\tCustodyLife_RemoveActors();\n\tCustody_MoveFleetTo(destination);\n\tCustody_RestoreEscrow();')
    data = replace(data, '\tstring city = XI_ConvertString("Colony" + pchar.Custody.City);',
                   '\tCustodyLife_Ensure();\n\tstring city = XI_ConvertString("Colony" + pchar.Custody.City);')
    data = replace(data, '\tif (CheckAttribute(pchar, "Custody.LastResult")) body +=',
                   '\tbody += " Начальник: " + pchar.Custody.Life.Admin + "/100. Охрана: " + pchar.Custody.Life.Guards + "/100. Победы: " + pchar.Custody.Life.Wins + "/3.";\n\tif (CheckAttribute(pchar, "Custody.LastResult")) body +=')
    result[path] = data + b'\r\n' + SOURCE.read_text().replace('\n', '\r\n').encode()
    path = 'PROGRAM/dialogs/russian/Common_Prison.c'
    data = result[path]
    start = data.index(b'\t\tcase "Custody_Cellmate":')
    end = data.index(b'\t\tcase "Custody_Quiet":', start)
    data = data[:start] + '''
        case "Custody_Cellmate":
            NextDiag.TempNode = "Custody_Cellmate";
            if (!CustodyLife_Menu(NPChar, Link))
            {
                dialog.text = "Поговорим в другой раз.";
                link.l1 = "Ладно."; link.l1.go = "Exit";
            }
        break;
'''.replace('\n', '\r\n').encode() + data[end:]
    result[path] = replace(data, '\t\tcase "Custody_Quiet":', DIALOG + '\n\t\tcase "Custody_Quiet":')
    path = 'PROGRAM/scripts/GoldFleet.c'
    result[path] = replace(result[path], 'extern void Custody_Release();', 'extern void Custody_Release();\n' + EXTERNS)
    path = 'PROGRAM/quests/quests_reaction.c'
    result[path] = replace(result[path], '\tswitch(sQuestName)\n\t{', '\tswitch(sQuestName)\n\t{' + QUESTS)
    if UPDATED:
        for path, expected in UPDATED.items():
            if digest(result[path]) != expected:
                raise ValueError(f'custody layer: output hash changed {path}')
    return result
