"""Compose Bermuda's recipe quest over reviewed final gameplay owners."""
from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "gameplay"
NORMALIZED_HASHES = {
    "PROGRAM/seadogs.c": "36cfa57fd6d14fd3ab76894a91a3646d0467d7cf68a188c50ff84958b30de26d",
    "PROGRAM/sea_ai/sea.c": "354723a2b9226290fd55eb5a49b486f30f0d5e937e6f70ef27564147dd4a502c",
    "PROGRAM/sea_ai/AIShip.c": "3c893df554cf3e318d8801e308d9d40ef042eac9b7d4d164f70e46ad53c0589c",
    "PROGRAM/store/storeutilite.c": "4d7633799afaa87be06c9c51c31e6dc96fc4c5ddc23e4de37403d74656ff81fa",
    "PROGRAM/locations/locations_loader.c": "2c2e69a4af9431a9bdda8d0978f09dbd5a5e9713f4e48560f2e193077ea19ea9",
    "PROGRAM/dialogs/russian/Tavern/Pirates_Tavern.c": "a96d00ebacfecab2702a55e143ba88a96416a0fe189734161bbf55a041daab28",
    "PROGRAM/QuestBook/QuestBook_New.txt": "ef54267b32f27962b710acdc9a116eac2cc17302fe2bdc67dfafbac687f255a0",
    "PROGRAM/scripts/ShipsUtilites.c": "2cd9ca95f3e8eb79b4ebe2371a2ce6812e9a1f8048bbba2c273cc267d77459e7",
}
BASE_HASHES = {
    "PROGRAM/seadogs.c": ("36cfa57fd6d14fd3ab76894a91a3646d0467d7cf68a188c50ff84958b30de26d",),
    "PROGRAM/sea_ai/sea.c": ("354723a2b9226290fd55eb5a49b486f30f0d5e937e6f70ef27564147dd4a502c",),
    "PROGRAM/sea_ai/AIShip.c": ("3c893df554cf3e318d8801e308d9d40ef042eac9b7d4d164f70e46ad53c0589c",),
    "PROGRAM/store/storeutilite.c": ("4d7633799afaa87be06c9c51c31e6dc96fc4c5ddc23e4de37403d74656ff81fa",),
    "PROGRAM/locations/locations_loader.c": ("2e496b1da697478553b24f22145ced5fe5011a63e76ecb02faf8f21ac0693fd2",),
    "PROGRAM/dialogs/russian/Tavern/Pirates_Tavern.c": ("62b40971c7ede802cb47353990e99af909a14d33d869bcca86b8700edae85b06",),
    "PROGRAM/QuestBook/QuestBook_New.txt": ("0b44d01978f7bc69d1fecb95138a1e66bcf194295b6ae18915cd3272a860f3cf",),
    "PROGRAM/scripts/ShipsUtilites.c": ("d4cb33dc34420e88cad1ebb5e784b4d06dd65783ea98506cfd71a01ea2c37183",),
}
HOOKS = frozenset(BASE_HASHES)
CREATED = {
    "PROGRAM/scripts/great_recipe.c": "great-recipe.c",
    "PROGRAM/dialogs/russian/GreatRecipe.c": "great-recipe-dialog.c",
}

TAVERN_CASES = '''
        case "GreatRecipe_Offer":
            dialog.text = "Борода прозвал свои ядра жемчугом черепахи. Начинка рвёт паруса и косит людей за бортом. Достать бы его рецепт... Моряки видели остров у Бермуд при ясной луне: с 22 до 4 часов, с 12 по 18 число. Только обратно вернулись немногие. Возьмёшься?";
            if (GreatRecipe_Phase() == "" && CheckAttribute(pchar, "Ship.Type") && sti(pchar.Ship.Type) != SHIP_NOTUSED)
            { link.l1 = "Возьмусь. Найду жемчуг и узнаю рецепт."; link.l1.go = "GreatRecipe_Accept"; }
            else if (GreatRecipe_Phase() == "search") dialog.text = "Ищи при ясной луне в море у Бермуд. С 22 до 4 часов, с 12 по 18 число — тогда остров видели моряки.";
            else if (GreatRecipe_Phase() == "won") dialog.text = "О тебе уже слагают песни! Корабль и запас ядер твои. За новой шрапнелью — к Паскалю в бермудский магазин.";
            else if (GreatRecipe_Phase() == "lost") dialog.text = "Сон-то сон, а Борода живой. В шторм и грозу видят его корвет у Бермуд. Бьёт он крепко, зато борта трухлявые. Возьмёшь его — заберёшь жемчуг из трюма. Рецептом он с тобой больше не поделится.";
            else dialog.text = "Без корабля за жемчугом не отправишься.";
            link.l2 = "Пока обойдусь."; link.l2.go = "exit";
        break;
        case "GreatRecipe_Accept":
            if (GreatRecipe_Accept()) dialog.text = "Тогда держи курс у Бермуд. Да вернёт тебя море.";
            else dialog.text = "Сейчас за это дело не взяться. Сначала нужен собственный корабль и свобода.";
            link.l1 = "До встречи."; link.l1.go = "exit";
        break;
'''

EDITS = {
    "PROGRAM/seadogs.c": [
        ('#include "interactive.c"', '#include "interactive.c"\n#include "scripts\\great_recipe.c"'),
        ('void GameOverE()\n{', 'void GameOverE()\n{\n    if (GreatRecipe_ClaimDeath(nMainCharacterIndex)) return;'),
        ('void GameOver(string sName)\n{', 'void GameOver(string sName)\n{\n    if (sName != "MainMenu" && GreatRecipe_ClaimDeath(nMainCharacterIndex)) return;'),
        ('\tCrewDebt_Ensure();', '\tCrewDebt_Ensure();\n    GreatRecipe_OnLoad();'),
    ],
    "PROGRAM/sea_ai/sea.c": [
        ('void DeleteSeaEnvironment()\n{', 'void DeleteSeaEnvironment()\n{\n    GreatRecipe_ClearVortex(true);'),
        ('void Sea_LandLoad()\n{', 'void Sea_LandLoad()\n{\n    if (GreatRecipe_Locked()) return;'),
        ('void Sea_MapLoad()\n{', 'void Sea_MapLoad()\n{\n    if (GreatRecipe_Locked()) return;'),
        ('\t// Quest check\n\tEvent(EVENT_SEA_LOGIN, "");', '\t// Quest check\n    GreatRecipe_PrepareSea(Login);\n\tEvent(EVENT_SEA_LOGIN, "");'),
        ('\tbSeaLoaded = true;', '\tbSeaLoaded = true;\n    GreatRecipe_SeaReady();'),
    ],
    "PROGRAM/sea_ai/AIShip.c": [
        ('void ShipDead(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{',
         'void ShipDead(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{\n    if (GreatRecipe_ClaimDeath(iDeadCharacterIndex)) return;'),
        ('void ShipTaken(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{',
         'void ShipTaken(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{\n    GreatRecipe_ClaimBoarding(iDeadCharacterIndex);'),
    ],
    "PROGRAM/store/storeutilite.c": [
        ('pStore.Colony == "Pirates") return TRADE_TYPE_AMMUNITION;',
         'pStore.Colony == "Pirates" && GreatRecipe_Unlocked()) return TRADE_TYPE_AMMUNITION;'),
    ],
    "PROGRAM/locations/locations_loader.c": [
        ('\tItem_OnLoadLocation(loc.id);', '\tItem_OnLoadLocation(loc.id);\n    GreatRecipe_LocationLoaded(loc);'),
    ],
    "PROGRAM/dialogs/russian/Tavern/Pirates_Tavern.c": [
        ('\t\t\t// ==> Проверяем поле состояния квестов.',
         '\t\t\t// ==> Проверяем поле состояния квестов.\n            link.l9 = "О жемчуге черепахи и Великом рецепте."; link.l9.go = "GreatRecipe_Offer";'),
        ('\t\t//шебека Синяя Птица', TAVERN_CASES + '\n\t\t//шебека Синяя Птица'),
    ],
    "PROGRAM/QuestBook/QuestBook_New.txt": [
        ('#QUEST PDM_Neputyovy_kaznachey', '#QUEST GreatRecipe\nВеликий рецепт\n#TEXT 1\n@<body>\n\n#QUEST PDM_Neputyovy_kaznachey'),
    ],
    "PROGRAM/scripts/ShipsUtilites.c": [
        ('\t// теперь сборка мусора\n\tfor (i = 0; i < REAL_SHIPS_QUANTITY; i++)',
         '\t// теперь сборка мусора\n    GreatRecipe_ReserveShips();\n\tfor (i = 0; i < REAL_SHIPS_QUANTITY; i++)'),
    ],
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def prepare(relative: str, incoming: bytes) -> bytes:
    if relative not in HOOKS:
        return incoming
    text = incoming.decode("utf-8").replace("\r\n", "\n")
    base = text
    # Strip only this exact layer, then bind the complete prior owner.
    applied = [after in text for before, after in EDITS[relative]]
    if any(applied):
        if not all(applied):
            raise ValueError(f"incomplete Great Recipe layer: {relative}")
        for before, after in reversed(EDITS[relative]):
            if base.count(after) != 1:
                raise ValueError(f"ambiguous Great Recipe layer: {relative}")
            base = base.replace(after, before, 1)
    if digest(base.encode("utf-8")) != NORMALIZED_HASHES[relative]:
        raise ValueError(f"unreviewed Great Recipe input: {relative}: {digest(base.encode('utf-8'))}")
    if any(applied):
        return incoming
    for before, after in EDITS[relative]:
        if text.count(before) != 1:
            raise ValueError(f"Great Recipe anchor drift: {relative}: {before[:75]!r}")
        text = text.replace(before, after, 1)
    return text.replace("\n", "\r\n").encode("utf-8")


def created() -> dict[str, bytes]:
    return {relative: (SOURCE / source).read_text().replace("\n", "\r\n").encode("utf-8")
            for relative, source in CREATED.items()}


def inputs() -> dict[Path, tuple[bytes, bytes]]:
    paths = [Path(__file__).resolve(), *(SOURCE / source for source in CREATED.values())]
    return {path: (data, data) for path in paths for data in [path.read_bytes()]}
