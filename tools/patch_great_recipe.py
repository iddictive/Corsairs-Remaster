"""Compose Bermuda's recipe quest over reviewed final gameplay owners."""
from __future__ import annotations

import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SOURCE = ROOT / "gameplay"
NORMALIZED_HASHES = {
    "PROGRAM/seadogs.c": "00a62b3be2b684880c7be2d233330222f9793161ab078362e5468d8bd20f002c",
    "PROGRAM/sea_ai/sea.c": "7d6336aa7e2181a44cde67753b6aa6a394ef8e07e78adfd5f559eb4e2f9e35ca",
    "PROGRAM/sea_ai/AIShip.c": "d404ec17a822573a3b7bfc9a20fcf88083561bfaf76b6af2fc2fabd627b6385b",
    "PROGRAM/store/storeutilite.c": "4d7633799afaa87be06c9c51c31e6dc96fc4c5ddc23e4de37403d74656ff81fa",
    "PROGRAM/locations/locations_loader.c": "2c2e69a4af9431a9bdda8d0978f09dbd5a5e9713f4e48560f2e193077ea19ea9",
    "PROGRAM/dialogs/russian/Tavern/Pirates_Tavern.c": "a96d00ebacfecab2702a55e143ba88a96416a0fe189734161bbf55a041daab28",
    "PROGRAM/dialogs/russian/Habitue_dialog.c": "de7fc10028b0250a28403144b09338c6959ba45c1e88feae4dc9841f8e3b7034",
    "PROGRAM/QuestBook/QuestBook_New.txt": "ef54267b32f27962b710acdc9a116eac2cc17302fe2bdc67dfafbac687f255a0",
    "PROGRAM/scripts/ShipsUtilites.c": "2cd9ca95f3e8eb79b4ebe2371a2ce6812e9a1f8048bbba2c273cc267d77459e7",
    "PROGRAM/sea_ai/AISeaGoods.c": "7c8c098d522478ed6041838da73ddb7b5cdd3754553a6cd12c04a9d2a1804554",
    "PROGRAM/ITEMS/initItems.c": "08333bdc7dc469ac29c2678d8d40ddbddf8349f48b7ce0a7fff5219eea30e7ce",
    "RESOURCE/INI/texts/russian/ItemsDescribe.txt": "656c445b97680e5f5842650acecf6dc2e714e1bf487816316dcc513a29a2c535",
    "PROGRAM/worldmap/worldmap_globals.c": "f31e89898b25f4f9864e2cc54abd58b28f91d1b10146879d0987cd32b34cd8c1",
    "PROGRAM/worldmap/worldmap_events.c": "52598a4baec8bec7c66270aa156705043f9e0ab7d93ade5420ec9b0d8bcc613b",
    "PROGRAM/Loc_ai/LAi_fightparams.c": "e886b558dbbb64565baca70ab512064a5f7b82183a946bc516722bb96611836e",
    "RESOURCE/INI/interfaces/pictures.ini": "bd8774f43bc7a5fe2ec22825ea6d4a0d5f189d36946c92b79bf65b3e8b60eedb",
    "PROGRAM/Loc_ai/LAi_boarding.c": "ea8c00a89e783640199a11d1fd798344ba4e91bca70e6169a08968899891b632",
    "PROGRAM/interface/store.c": "9bf6299cc923045a9166afa2a0fa4be664a37bcf4faeccc9969186358a389a76",
    "PROGRAM/nations/nations.c": "f5918635df437b06166823970df1c120f1ac08d4ed21a719e53769644580f435",
    "PROGRAM/Loc_ai/LAi_groups.c": "a19ccce46c33974727abeb687694ce9af93282a0e93042ea86589e255ba41470",
    "PROGRAM/dialogs/russian/Mayor/Jackman.c": "58d50e63f43c27cb9b87bcd3913c5584dfb8504b5d8df5980d111a00a87e157a",
    "PROGRAM/dialogs/russian/Common_Mayor.c": "6b338fc115310528701af4900f9599a7c45c92d1f6b2865e0207ce6329125341",
    "PROGRAM/quests/quests_reaction.c": "b9588c174a3cf04f646e6e5fb03f5ba8583d46576f27298d626510a5e50397d4",
    "PROGRAM/ITEMS/itemLogic.c": "b977d9034f15c435f704b468634f9dd7e51a338ab41fb13c88811d7ff43b170a",
    "PROGRAM/ITEMS/items_utilite.c": "c141fb1bb21a38ee7d9c6d97a439a729ae85e7d1ae9b4d8151c1fa69a89c72cf",
    "PROGRAM/characters/QuestsUtilite.c": "bea1955c7e95618ad98a41191aade92eec2072fb784dfde8089bca33d89e9b36",
    "PROGRAM/weather/WhrSun.c": "b03af527509bbe563bccf728e651bfe2f7ac497ac7cd5a173f41a86f1e1281d8",
}
BASE_HASHES = {
    "PROGRAM/seadogs.c": ("36cfa57fd6d14fd3ab76894a91a3646d0467d7cf68a188c50ff84958b30de26d",),
    "PROGRAM/sea_ai/sea.c": ("354723a2b9226290fd55eb5a49b486f30f0d5e937e6f70ef27564147dd4a502c",),
    "PROGRAM/sea_ai/AIShip.c": ("3c893df554cf3e318d8801e308d9d40ef042eac9b7d4d164f70e46ad53c0589c",),
    "PROGRAM/store/storeutilite.c": ("4d7633799afaa87be06c9c51c31e6dc96fc4c5ddc23e4de37403d74656ff81fa",),
    "PROGRAM/locations/locations_loader.c": ("2e496b1da697478553b24f22145ced5fe5011a63e76ecb02faf8f21ac0693fd2",),
    "PROGRAM/dialogs/russian/Tavern/Pirates_Tavern.c": ("62b40971c7ede802cb47353990e99af909a14d33d869bcca86b8700edae85b06",),
    "PROGRAM/dialogs/russian/Habitue_dialog.c": ("ad61192b45501d54b6af727d74c976556e180a0483bbb9aa69ae032dd73b3dad",),
    "PROGRAM/QuestBook/QuestBook_New.txt": ("0b44d01978f7bc69d1fecb95138a1e66bcf194295b6ae18915cd3272a860f3cf",),
    "PROGRAM/scripts/ShipsUtilites.c": ("d4cb33dc34420e88cad1ebb5e784b4d06dd65783ea98506cfd71a01ea2c37183",),
    "PROGRAM/sea_ai/AISeaGoods.c": ("39050a04396461aaf2cb05d59d8d1269f67e48de2b75af5ccebcb76f8b44e162",),
    "PROGRAM/ITEMS/initItems.c": ("b84ff3fd7b9e49a63737fc8b91d39c851c311650e8dfccc55dd68b0d2973b4ce",),
    "RESOURCE/INI/texts/russian/ItemsDescribe.txt": ("5f334cb8437b243175d0ef80af90ee104efe5ef62a2e7c51ff737d635f1a6bc0",),
    "PROGRAM/worldmap/worldmap_globals.c": ("184ce121f96c9347ae8ea6c7304af77dee74cf15b117891c3f0e0b04f3ac6885",),
    "PROGRAM/worldmap/worldmap_events.c": ("29d8e42010ed08ee0e8861e2b2e101e2e6073823b0f0a653070812c64ac429b7",),
    "PROGRAM/Loc_ai/LAi_fightparams.c": ("5eed196c397d59475474b0f2c5964402167ba024dd33621cc10c785cd007f553",),
    "RESOURCE/INI/interfaces/pictures.ini": ("045d5d3e07be77ca6259dc64b826a68145c16fd3114e64c6337578205fea9634",),
    "PROGRAM/Loc_ai/LAi_boarding.c": ("ff47f627f7ab264585f751653d85949bee70f7c40a61b013ceb04118acc17934",),
    "PROGRAM/interface/store.c": ("8eb4174ffab3dd2db8b60797cbaf22bd40a668ad9f20eb0b7b7d018d2104d9ad",),
    "PROGRAM/nations/nations.c": ("5a9bf2d43cda277795e0350aafbc41103d5f3bf9c1ab732cec2409cf28d140a5",),
    "PROGRAM/Loc_ai/LAi_groups.c": ("ce21e56e14e3b0122705baaddd8bc30d2807084a927a0773411c3cfd16bcb252",),
    "PROGRAM/dialogs/russian/Mayor/Jackman.c": ("b8acc1b596b56e26fe57c71201cc94e976f1445c097de29fe48c1040eb92c3c5",),
    "PROGRAM/dialogs/russian/Common_Mayor.c": ("92b94fab040d31dbc9fbe17993904c5d452c8d53f676b0c8480bdf838f1d4b31",),
    "PROGRAM/quests/quests_reaction.c": ("8d8169d2f22e550c64aa5c1771fa035bcf518945cf6508c1f596b50e23242ffd",),
    "PROGRAM/ITEMS/itemLogic.c": ("8635ea2d76349ff573eb4e46f4e98ac14f28f27bb35de6a77e3a287a6d1364f9",),
    "PROGRAM/ITEMS/items_utilite.c": ("9c929be67e6c139ef65e6bbcf5bb18c78b6166bb54e559bebda85fa33d23e9ab",),
    "PROGRAM/characters/QuestsUtilite.c": ("5555e137c190e17cd440958e4ee4ee9d097415d8f07e12e741c5eb21aec59707",),
    "PROGRAM/weather/WhrSun.c": ("1c66f837695367ea53c5386219fd4df8d6a817575b84ee5a5806690ddb860a3e",),
}
HOOKS = frozenset(BASE_HASHES)
CREATED = {
    "PROGRAM/scripts/great_recipe.c": "great-recipe.c",
    "PROGRAM/dialogs/russian/GreatRecipe.c": "great-recipe-dialog.c",
    "PROGRAM/scripts/great_recipe_alliance.c": "great-recipe-alliance.c",
    "PROGRAM/dialogs/russian/GreatRecipeAlliance.c": "great-recipe-alliance-dialog.c",
    "PROGRAM/scripts/great_recipe_shore.c": "great-recipe-shore.c",
    "PROGRAM/dialogs/russian/GreatRecipeShore.c": "great-recipe-shore-dialog.c",
    "PROGRAM/dialogs/russian/GreatRecipeRumors.c": "great-recipe-rumors.c",
}

TAVERN_CASES = '''
        case "GreatRecipe_Offer":
            dialog.text = "Слыхал о Кровавой Бороде? Не о том, что за долей гонится. Этот жжёт суда ради смеха. Недавно рыбаки подняли шлюпку: в ней ни одного целого человека, а в борту железная скорлупа, будто жемчуг. Кто-то клянётся, что видел его корабль у Бермуд при большой луне. Кто-то — остров, которого вчера ещё не было. Пьяные байки? Может быть. Только мой брат с того рейса не вернулся.";
            link.l1 = "Что за Борода?"; link.l1.go = "GreatRecipe_Story";
            link.l2 = "Кто видел остров? Где искать?"; link.l2.go = "GreatRecipe_Rumor";
            link.l3 = "Почему ты назвал железо жемчугом?"; link.l3.go = "GreatRecipe_Ammo";
            link.l4 = "Тебе нужен брат или рецепт?"; link.l4.go = "GreatRecipe_Price";
            if (GreatRecipe_Phase() == "" && CheckAttribute(pchar, "Ship.Type") && sti(pchar.Ship.Type) != SHIP_NOTUSED)
            { link.l5 = "Проверю слухи. Если найду что-нибудь — принесу."; link.l5.go = "GreatRecipe_Accept"; }
            if (GreatRecipe_Phase() == "search") dialog.text = "Том с рыбацкой шхуны говорит — ясная луна, открытая вода у Бермуд. А его матрос божится, что это был корабль, не остров. Оба трезвые. За точность их слов я головой не ручаюсь.";
            if (GreatRecipe_Phase() == "pearl_search")
            {
                dialog.text = "Ты вернулся! Скажи, было там что-нибудь, кроме воды?";
                if (GetCharacterItem(pchar, "GreatRecipeRedPearl") > 0)
                { link.l5 = "Вот красная жемчужина с его клеймом."; link.l5.go = "GreatRecipe_TurnIn"; }
            }
            if (GreatRecipe_Phase() == "production") dialog.text = "Паскаль уже договорился с мастерами. Дай им неделю: это не пули отлить. Потом в магазине будет по небольшой партии каждый день. Не заставляй людей работать с порохом впопыхах.";
            if (GreatRecipe_Phase() == "won") dialog.text = "Мастера разобрались с жемчугом. Паскаль держит партии в магазине. Надеюсь, с твоего борта эта дрянь полетит в тех, кто её заслужил.";
            if (GreatRecipe_Phase() == "lost") dialog.text = "Мне тоже снилось море после брата. Только один портовый сторож видел красные паруса в грозу у Бермуд. Ночь была лунная, пока не затянуло тучами... Сам уж решай, верить ли ему.";
            if (GreatRecipe_AllianceStage() == "parley")
            { link.l8 = "Борода обещал освободить пленников и оставить берег в покое. Обсудим возмещение семьям?"; link.l8.go = "GreatRecipe_Restitution"; }
            link.l9 = "Не хочу связываться с этой историей."; link.l9.go = "exit";
        break;
        case "GreatRecipe_Story":
            dialog.text = "Перехватывает мирных, сдирает с судов паруса, а потом кружит, пока люди просят воды. Не торгуется и не требует выкупа. Слышал, держит людей в трюме, заставляет служить — и смотрит, кто сломается первым. Мне это рассказал калека без кисти. Остальным рассказчикам я бы не поверил.";
            link.l1 = "Капитан, которому убийство в удовольствие..."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_Rumor":
            dialog.text = "Не рисуй крест на карте с моих слов. Они шли мимо Бермуд, далеко от берега; море светилось, было тихо. Впереди показалась темнота, как остров. Потом вода пошла кругом. Один уверяет — видел мачты внутри, другой ничего не помнит. После такой ночи я бы тоже напился.";
            link.l1 = "Поищу в открытой воде, при ясной луне."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_Ammo":
            dialog.text = "Рыбаки так прозвали скорлупу: круглая, гладкая, внутри железные зубы. Говорят, рвётся уже за бортом — и людей срезает, и корпус распарывает. Ни наш кузнец, ни Паскаль такого не делали. Будь у нас этот секрет, берег бы не дрожал от каждого красного паруса.";
            link.l1 = "Сначала надо понять, что внутри."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_Price":
            dialog.text = "Брата рецепт не вернёт. Но я устал выкупать чужих детей из рук людей, которые служили этому ублюдку. Хочу, чтоб однажды у берега оказался капитан с такими же ядрами — и повернул их против него. А если найдёшь след брата... просто скажи правду.";
            link.l1 = "Пустых обещаний не дам."; link.l1.go = "GreatRecipe_Offer";
        break;
        case "GreatRecipe_TurnIn":
            if (GreatRecipe_TurnInPearl()) dialog.text = "Клеймо настоящее. Смотри — шов, а под ним порох и железная рубка... Паскаль найдёт мастеров. Через неделю зайди в магазин; больше двух сотен в запасе держать не станем. Я не хочу, чтобы пол-Бермуд взлетело в воздух.";
            else dialog.text = "Покажи саму жемчужину. По рассказу мастер её не соберёт.";
            link.l1 = "Береги людей, Лаки."; link.l1.go = "exit";
        break;
        case "GreatRecipe_Accept":
            if (GreatRecipe_Accept()) dialog.text = "Хорошего тебе ветра. Узнаешь правду — вернись и расскажи. Я давно не жду добрых вестей, но ждать не перестал.";
            else dialog.text = "Сейчас за это дело не взяться. Сначала нужен собственный корабль и свобода.";
            link.l1 = "До встречи."; link.l1.go = "exit";
        break;
        case "GreatRecipe_Restitution":
            dialog.text = GreatRecipe_AllianceAction(NPChar.id, "restitution");
            link.l1 = "Поговорю с Джекменом."; link.l1.go = "exit";
        break;
'''

EDITS = {
    "PROGRAM/seadogs.c": [
        ('#include "interactive.c"', '#include "interactive.c"\n#include "scripts\\great_recipe.c"'),
        ('#include "scripts\\great_recipe.c"', '#include "scripts\\great_recipe.c"\n#include "scripts\\great_recipe_alliance.c"'),
        ('#include "scripts\\great_recipe_alliance.c"', '#include "scripts\\great_recipe_alliance.c"\n#include "scripts\\great_recipe_shore.c"'),
        ('void GameOverE()\n{', 'void GameOverE()\n{\n    if (GreatRecipe_ClaimDeath(nMainCharacterIndex)) return;'),
        ('void GameOver(string sName)\n{', 'void GameOver(string sName)\n{\n    if (sName != "MainMenu" && GreatRecipe_ClaimDeath(nMainCharacterIndex)) return;'),
        ('void OnLoad()\n{', 'void OnLoad()\n{\n    GreatRecipe_PreLoadActors();'),
        ('\tCrewDebt_Ensure();', '\tCrewDebt_Ensure();\n    GreatRecipe_OnLoad();'),
    ],
    "PROGRAM/sea_ai/sea.c": [
        ('void DeleteSeaEnvironment()\n{', 'void DeleteSeaEnvironment()\n{\n    GreatRecipe_ClearVortex(true);'),
        ('void Sea_LandLoad()\n{', 'void Sea_LandLoad()\n{\n    if (GreatRecipe_Locked()) return;'),
        ('void Sea_MapLoad()\n{', 'void Sea_MapLoad()\n{\n    if (GreatRecipe_Locked()) return;'),
        ('\t// Quest check\n\tEvent(EVENT_SEA_LOGIN, "");', '\t// Quest check\n    GreatRecipe_PrepareSea(Login);\n\tEvent(EVENT_SEA_LOGIN, "");'),
        ('\tbSeaLoaded = true;', '\tbSeaLoaded = true;\n    GreatRecipe_SeaReady();'),
        ('\t\tEncounter_DeleteDeadQuestMapEncounters();', '\t\tEncounter_DeleteDeadQuestMapEncounters();\n    GreatRecipe_AfterSceneUnload();'),
        ('\n            rFantom.RealEncounterType = iEncounterType;//boal', '\n            rFantom.RealEncounterType = iEncounterType;//boal\n            if (CheckAttribute(rEncounter, "GreatRecipeBarrel")) rFantom.GreatRecipeBarrel = true;'),
        ('\t\trEncounter = GetMapEncounterRef(sti(rRawGroup.type));', '\t\trEncounter = GetMapEncounterRef(sti(rRawGroup.type));\n        if (CheckAttribute(rEncounter, "CharacterID") && (GreatRecipe_Phase() == "whirlpool" || GreatRecipe_Phase() == "assault") && rEncounter.CharacterID == "GreatRecipeDreamBeard") continue;'),
        ('\t\t\t\tSetCharacterRelationBoth(iCharacterIndex, nMainCharacterIndex, iRelation);', '\t\t\t\tSetCharacterRelationBoth(iCharacterIndex, nMainCharacterIndex, iRelation);\n                GreatRecipe_AllianceRegisterPair(iCharacterIndex, iRelation);'),
    ],
    "PROGRAM/sea_ai/AIShip.c": [
        ('\tif(iFantomType == ENCOUNTER_TYPE_BARREL)\n\t{',
         '\tif (iFantomType == ENCOUNTER_TYPE_BARREL && CheckAttribute(rChar, "GreatRecipeBarrel"))\n    { GreatRecipe_SpawnBarrel(rChar, true); iFantomType = -1; }\n\tif(iFantomType == ENCOUNTER_TYPE_BARREL)\n\t{'),
        ('void Ship_ChangeCharge(ref rCharacter, int iNewChargeType)\n{',
         'void Ship_ChangeCharge(ref rCharacter, int iNewChargeType)\n{\n    if (CheckAttribute(rCharacter, "Ship.GreatRecipeAirburstOnly")) iNewChargeType = GOOD_AIRBURST;'),
        ('\t\tShipDead(sti(rOurCharacter.index), iKillStatus, iKillerCharacterIndex);',
         '\t\tif (GreatRecipe_ClaimHullDefeat(rOurCharacter)) return;\n\t\tShipDead(sti(rOurCharacter.index), iKillStatus, iKillerCharacterIndex);'),
        ('\tbool bIsCompanion = IsCompanion(rCharacter);',
         '\tif (CheckAttribute(rCharacter, "Ship.GreatRecipeAirburstOnly")) rCharacter.ShipCannonChargeType = GOOD_AIRBURST;\n\tbool bIsCompanion = IsCompanion(rCharacter);'),
        ('void ShipDead(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{',
         'void ShipDead(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{\n    if (GreatRecipe_ClaimDeath(iDeadCharacterIndex)) return;'),
        ('void ShipTaken(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{',
         'void ShipTaken(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{\n    GreatRecipe_ClaimBoarding(iDeadCharacterIndex);'),
        ('    GreatRecipe_ClaimBoarding(iDeadCharacterIndex);',
         '    GreatRecipe_ClaimBoarding(iDeadCharacterIndex);\n    if (iDeadCharacterIndex == nMainCharacterIndex && GreatRecipe_Phase() == "capturing") return;'),
        ('void ShipTakenFree(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{', 'void ShipTakenFree(int iDeadCharacterIndex, int iKillStatus, int iKillerCharacterIndex)\n{\n    GreatRecipe_AllianceReleaseCaptain(GetCharacter(iDeadCharacterIndex));\n    GreatRecipe_ClaimBoarding(iDeadCharacterIndex);'),
        ('    LAi_SetCurHP(rDead, 0.0); // умер', '    if (!GreatRecipe_AllianceKeepCaptain(rDead)) LAi_SetCurHP(rDead, 0.0); // умер', 2),
    ],
    "PROGRAM/store/storeutilite.c": [
        ('pStore.Colony == "Pirates") return TRADE_TYPE_AMMUNITION;',
         'pStore.Colony == "Pirates" && GreatRecipe_Unlocked()) return TRADE_TYPE_AMMUNITION;'),
        ('    pStore.Goods.Airburst.Quantity = quantity;', '    if (pStore.Colony == "Pirates" && !GreatRecipe_Unlocked()) quantity = 0;\n    pStore.Goods.Airburst.Quantity = quantity;'),
        ('        pStore.Goods.Airburst.TradeType = Store_AirburstTradeType(pStore);', '        pStore.Goods.Airburst.TradeType = Store_AirburstTradeType(pStore);\n        if (pStore.Colony == "Pirates" && sti(pStore.Goods.Airburst.Quantity) > 200) pStore.Goods.Airburst.Quantity = 200;'),
        ('\tif (!WdmRecoveryAdmitDay(recoveryColony, recoveryOffset)) return;', '\tif (!WdmRecoveryAdmitDay(recoveryColony, recoveryOffset)) return;\n    GreatRecipe_StoreDaily(pStore, recoveryOffset);'),
        ('\t\ttmpstr = Goods[i].name;\n\t\tif (!CheckAttribute(gref,tmpstr) ) continue;', '\t\ttmpstr = Goods[i].name;\n        if (i == GOOD_AIRBURST && pStore.Colony == "Pirates") continue;\n\t\tif (!CheckAttribute(gref,tmpstr) ) continue;'),
    ],
    "PROGRAM/locations/locations_loader.c": [
        ('\tLoginCharactersInLocation(loc);', '\tGreatRecipe_AllianceResidence(loc);\n\tLoginCharactersInLocation(loc);'),
        ('\tItem_OnLoadLocation(loc.id);', '\tItem_OnLoadLocation(loc.id);\n    GreatRecipe_LocationLoaded(loc);'),
        ('\tItem_OnLoadLocation(loc.id);', '    GreatRecipe_PreLoadItems(loc);\n\tItem_OnLoadLocation(loc.id);'),
        ('\tDeleteClass(&locCamera);', '\tDeleteClass(&locCamera);\n    GreatRecipe_AfterSceneUnload();'),
    ],
    "PROGRAM/dialogs/russian/Tavern/Pirates_Tavern.c": [
        ('\t\t\t// ==> Проверяем поле состояния квестов.',
         '\t\t\t// ==> Проверяем поле состояния квестов.\n            link.l9 = "О жемчуге черепахи и Великом рецепте."; link.l9.go = "GreatRecipe_Offer";'),
        ('\t\t//шебека Синяя Птица', TAVERN_CASES + '\n\t\t//шебека Синяя Птица'),
    ],
    "PROGRAM/dialogs/russian/Habitue_dialog.c": [
        ('#include "DIALOGS\\russian\\Rumours\\Common_rumours.c"  //homo 25/06/06', '#include "DIALOGS\\russian\\Rumours\\Common_rumours.c"  //homo 25/06/06\n#include "DIALOGS\\russian\\GreatRecipeRumors.c"'),
        ('\tswitch (Dialog.CurrentNode)', '\tif (GreatRecipe_ProcessTavernRumor(Dialog.CurrentNode, Link)) return;\n\tswitch (Dialog.CurrentNode)'),
        ('\t\t\t\t\tdialog.text = "О! Здравая идея!";', '\t\t\t\t\tif (rand(1) == 0)\n\t\t\t\t\t{\n\t\t\t\t\t\tGreatRecipe_ProcessTavernRumor("GreatRecipe_TavernRumor", Link);\n\t\t\t\t\t\tbreak;\n\t\t\t\t\t}\n\t\t\t\t\tdialog.text = "О! Здравая идея!";'),
    ],
    "PROGRAM/QuestBook/QuestBook_New.txt": [
        ('#QUEST PDM_Neputyovy_kaznachey', '#QUEST GreatRecipe\nВеликий рецепт\n#TEXT 1\n@<body>\n\n#QUEST PDM_Neputyovy_kaznachey'),
    ],
    "PROGRAM/scripts/ShipsUtilites.c": [
        ('\t// теперь сборка мусора\n\tfor (i = 0; i < REAL_SHIPS_QUANTITY; i++)',
         '\t// теперь сборка мусора\n    GreatRecipe_ReserveShips();\n\tfor (i = 0; i < REAL_SHIPS_QUANTITY; i++)'),
    ],
    "PROGRAM/sea_ai/AISeaGoods.c": [
        ('\tswitch (sGood)\n\t{', '\tswitch (sGood)\n\t{\n        case "great_recipe_barrel": return GreatRecipe_PickupBarrel(iCharacterIndex); break;'),
    ],
    "PROGRAM/ITEMS/initItems.c": [
        ('\treturn n;', '\tn = GreatRecipe_InitPearl(n);\n\treturn n;'),
    ],
    "RESOURCE/INI/texts/russian/ItemsDescribe.txt": [
        ('itmname_jewelry11\t{Большая жемчужина}', 'itmname_GreatRecipeRedPearl {Красная жемчужина Бороды}\nitmdescr_GreatRecipeRedPearl {Тяжёлая кроваво-красная жемчужина из капитанского сундука. Лаки Росин узнает клеймо и поможет наладить изготовление шрапнельных бомб на Бермудах.}\n\nitmname_jewelry11\t{Большая жемчужина}'),
        ('itmname_jewelry11\t{Большая жемчужина}', 'itmname_GreatRecipeOil {Масло для блока}\nitmdescr_GreatRecipeOil {Бутылка масла для заклинившего блока грота-шкота. Одного масла мало: нужен новый нагель.}\nitmname_GreatRecipePin {Дубовый нагель}\nitmdescr_GreatRecipePin {Крепкий дубовый нагель взамен разбитой оси блока. Перед установкой смазать маслом.}\nitmname_GreatRecipeLinen {Чистое полотно}\nitmdescr_GreatRecipeLinen {Чистое полотно для перевязки раны Диего.}\nitmname_GreatRecipeWater {Кипячёная вода}\nitmdescr_GreatRecipeWater {Кипячёная остывшая вода для промывания раны Диего.}\nitmname_GreatRecipeFuse {Сухой красный фитиль}\nitmdescr_GreatRecipeFuse {Сухой фитиль с красной нитью для заряда с замедленным разрывом. Сверить его с капитанской накладной.}\n\nitmname_jewelry11\t{Большая жемчужина}'),
        ('itmname_jewelry11\t{Большая жемчужина}', 'itmname_GreatRecipeCacheKey {Ключ от тайника}\nitmdescr_GreatRecipeCacheKey {Ржавый ключ из ящика у восточных скал. Открывает проход к тайнику старой команды Бороды.}\nitmname_GreatRecipeCacheLedger {Судовая книга старой команды}\nitmdescr_GreatRecipeCacheLedger {Промасленная книга с невыплаченными долями и подписью Ратли под последней выдачей пороха. Борода приказал принести её в каюту.}\n\nitmname_jewelry11\t{Большая жемчужина}'),
    ],
    "PROGRAM/ITEMS/items_utilite.c": [
        ('void QuestCheckTakeItem(aref _location, string _itemId)\n{', 'void QuestCheckTakeItem(aref _location, string _itemId)\n{\n    GreatRecipe_TakeSupply(_location, _itemId);'),
        ('void QuestCheckUseButton(aref _location, string _locator, string _itemId) /// <<< квестовые действия при установке предметов в button <<<\n{', 'void QuestCheckUseButton(aref _location, string _locator, string _itemId) /// <<< квестовые действия при установке предметов в button <<<\n{\n    GreatRecipe_UseSupply(_location, _locator, _itemId);'),
    ],
    "PROGRAM/ITEMS/itemLogic.c": [
        ('\tint itemN;\n\n\tif (findsubstr(_locator, "button", 0) != -1)', '\tint itemN;\n    int recipeItemStart = ItemsForLocators_start;\n    int recipeItemEnd = ItemsForLocators_end;\n    if (GreatRecipe_Phase() == "captive" && (_location.id == "GreatRecipeHold" || _location.id == "GreatRecipeMess" || _location.id == "GreatRecipeDeck"))\n    { recipeItemStart = 0; recipeItemEnd = ITEMS_QUANTITY; }\n\n\tif (findsubstr(_locator, "button", 0) != -1)'),
        ('itemN=ItemsForLocators_start; itemN<ItemsForLocators_end; itemN++', 'itemN=recipeItemStart; itemN<recipeItemEnd; itemN++', 2),
        ('\tmakearef(activeLocation, Locations[sti(chr.itemLocationIndex)]);\n\n\tmakearef (aloc, activeLocation.items);', '\tmakearef(activeLocation, Locations[sti(chr.itemLocationIndex)]);\n    if (!GreatRecipe_CanUseSupply(activeLocation, chr.activeLocator, Items[activeItem].id)) return;\n\n\tmakearef (aloc, activeLocation.items);'),
    ],
    "PROGRAM/worldmap/worldmap_globals.c": [
        ('\tencID = worldMap.EncounterID1;\n\treturn res;\n}\n\n//Торговец', '\tencID = worldMap.EncounterID1;\n    if (res) GreatRecipe_MarkRecoveryBarrel(worldMap.EncounterID1);\n\treturn res;\n}\n\n//Торговец'),
    ],
    "PROGRAM/worldmap/worldmap_events.c": [
        ('\tEnvironment.time = GetEventData();', '\tEnvironment.time = GetEventData();\n    GreatRecipe_MapCalendar();'),
    ],
    "PROGRAM/Loc_ai/LAi_fightparams.c": [
        ('\tif(CheckAttribute(enemy, "cirassId"))\n\t{\n\t\tdmg = dmg * (1.0 - stf(Items[sti(enemy.cirassId)].CirassLevel));\n\t}',
         '\tfloat recipeArmor = 0.0;\n    if (CheckAttribute(enemy, "cirassId")) recipeArmor = stf(Items[sti(enemy.cirassId)].CirassLevel);\n    dmg = GreatRecipe_BeardBladeDamage(attack, enemy, dmg, recipeArmor);'),
        ('\tdamage = LAi_LimitMainCharacterFirearmDamage(enemy, damage);', '\tdamage = LAi_LimitMainCharacterFirearmDamage(enemy, damage);\n    damage = GreatRecipe_BeardFireDamage(attack, enemy, damage);', 2),
        ('    dmg = GreatRecipe_BeardBladeDamage(attack, enemy, dmg, recipeArmor);', '    dmg = GreatRecipe_BeardBladeDamage(attack, enemy, dmg, recipeArmor);\n    dmg = GreatRecipe_ShoreBladeDamage(attack, enemy, dmg);'),
    ],
    "RESOURCE/INI/interfaces/pictures.ini": [
        ('[TABS]', '[FACE128_2006]\nsTextureName = portraits\\128\\face_2006.tga\nwTextureWidth = 128\nwTextureHeight = 128\npicture = face,0,0,128,128\n\n[TABS]'),
    ],
    "PROGRAM/Loc_ai/LAi_boarding.c": [
        ('\t\t\tLAi_SetHP(chr, ihp, ihp);\n\t\t}\n\t}\n\t//ставим вражеских мушкетеров', '\t\t\tLAi_SetHP(chr, ihp, ihp);\n\t\t}\n        GreatRecipe_BoardingFighter(chr, boarding_enemy);\n\t}\n\t//ставим вражеских мушкетеров'),
        ('        GreatRecipe_BoardingFighter(chr, boarding_enemy);', '        GreatRecipe_BoardingFighter(chr, boarding_enemy);\n        GreatRecipe_AllianceArmCaptain(chr, boarding_enemy);'),
        ('\t\t\t\telse SetMushketerParamFortEnemy(chr);\n\t\t\t\tAddCharHP(chr, boarding_enemy_hp); // влияение опыта и морали в НР', '\t\t\t\telse SetMushketerParamFortEnemy(chr);\n\t\t\t\tAddCharHP(chr, boarding_enemy_hp); // влияение опыта и морали в НР\n                GreatRecipe_BoardingFighter(chr, boarding_enemy);'),
    ],
    "PROGRAM/interface/store.c": [
        ('\tnTradeQuantity = sti(GameInterface.qty_edit.str);', '\tnTradeQuantity = sti(GameInterface.qty_edit.str);\n    if (BuyOrSell == -1)\n    {\n        nTradeQuantity = GreatRecipe_StoreSaleLimit(refStore, iCurGoodsIdx, nTradeQuantity);\n        GameInterface.qty_edit.str = nTradeQuantity;\n        iStoreQty = GetStoreGoodsQuantity(refStore, iCurGoodsIdx);\n    }'),
        ('\t\t    // проверка на колво доступное <--\n\t\t    SetFormatedText("QTY_Result", XI_ConvertString("SELL")', '\t\t    // проверка на колво доступное <--\n            GameInterface.qty_edit.str = GreatRecipe_StoreSaleLimit(refStore, iCurGoodsIdx, sti(GameInterface.qty_edit.str));\n            iWeight = GetGoodWeightByType(iCurGoodsIdx, sti(GameInterface.qty_edit.str));\n\t\t    SetFormatedText("QTY_Result", XI_ConvertString("SELL")'),
    ],
    "PROGRAM/nations/nations.c": [
        ('void SetCharacterRelation(int iCharacterIndex1, int iCharacterIndex2, int iRelationType)\n{', 'void SetCharacterRelation(int iCharacterIndex1, int iCharacterIndex2, int iRelationType)\n{\n    GreatRecipe_AllianceRelationWritten(iCharacterIndex1, iCharacterIndex2);'),
        ('\treturn NationsRelations2MainCharacter[iNation];', '\treturn GreatRecipe_AllianceRelation(iNation, NationsRelations2MainCharacter[iNation]);', 2),
        ('\t// character 2 character relation\n', '\tif (iMCI == sti(rCharacter1.index) && GreatRecipe_AllianceHasPair(rCharacter2)) return GreatRecipe_AlliancePairRelation(rCharacter2);\n\tif (iMCI == sti(rCharacter2.index) && GreatRecipe_AllianceHasPair(rCharacter1)) return GreatRecipe_AlliancePairRelation(rCharacter1);\n\t// character 2 character relation\n'),
        ('\t\treturn NationsRelations2MainCharacter[iNation2];', '\t\treturn GreatRecipe_AllianceActorRelation(rCharacter2, NationsRelations2MainCharacter[iNation2]);'),
        ('\t\treturn NationsRelations2MainCharacter[iNation1];', '\t\treturn GreatRecipe_AllianceActorRelation(rCharacter1, NationsRelations2MainCharacter[iNation1]);'),
    ],
    "PROGRAM/Loc_ai/LAi_groups.c": [
        ('void LAi_group_SetRelation(string group1, string group2, string curRelation)\n{', 'void LAi_group_SetRelation(string group1, string group2, string curRelation)\n{\n    curRelation = GreatRecipe_AllianceLandRelation(group1, group2, curRelation);'),
    ],
    "PROGRAM/quests/quests_reaction.c": [
        ('\tDoQuestCheckDelay("NationUpdate", 3.0);\n\tUpdateRelations();', '\tDoQuestCheckDelay("NationUpdate", 3.0);\n    GreatRecipe_AllianceRefreshLand();\n\tUpdateRelations();'),
        ('    DoQuestCheckDelay("NationUpdate", 3.0);\n    UpdateRelations();', '    DoQuestCheckDelay("NationUpdate", 3.0);\n    GreatRecipe_AllianceRefreshLand();\n    UpdateRelations();', 4),
    ],
    "PROGRAM/dialogs/russian/Mayor/Jackman.c": [
        ('\tswitch(Dialog.CurrentNode)\n', '    if (GreatRecipe_AllianceStage() == "parley" && (Dialog.CurrentNode == "First time" || Dialog.CurrentNode == "I_know_you_good"))\n    { link.l8 = "Лаки согласен засвидетельствовать мир с Бородой. Обсудим договор?"; link.l8.go = "GreatRecipe_Treaty"; }\n\tswitch(Dialog.CurrentNode)\n'),
        ('\t\t// ----------------------------------- Диалог первый - первая встреча', '        case "GreatRecipe_Treaty":\n            dialog.text = GreatRecipe_AllianceAction(NPChar.id, "treaty");\n            link.l1 = "Передам Бороде твои условия."; link.l1.go = "Exit";\n        break;\n\t\t// ----------------------------------- Диалог первый - первая встреча'),
    ],
    "PROGRAM/dialogs/russian/Common_Mayor.c": [
        ('        case "City":', '        case "City":\n            if (GreatRecipe_AllianceRefuseCapture(NPChar.City)) { DialogExit(); break; }'),
        ('        case "City_patent":', '        case "City_patent":\n            if (GreatRecipe_AllianceRefuseCapture(NPChar.City)) { DialogExit(); break; }'),
        ('        case "City_nation":', '        case "City_nation":\n            if (GreatRecipe_AllianceRefuseCapture(NPChar.City)) { DialogExit(); break; }'),
    ],
    "PROGRAM/characters/QuestsUtilite.c": [
        ('bool SetLocationQuestBox(ref _location, string _locatorName)\n{', 'bool SetLocationQuestBox(ref _location, string _locatorName)\n{\n    if (GreatRecipe_ShoreQuestBox(_location, _locatorName)) return true;'),
    ],
    "PROGRAM/weather/WhrSun.c": [
        ('\n}\n\nvoid MoveSunGlowToLayers', '\n    if (GreatRecipe_ShoreMoonScene())\n    {\n        SunGlow.Moon = true;\n        SunGlow.Glow.MoonTexture = "weather\\sun\\glow\\moon\\moon15.tga";\n        SunGlow.Glow.MoonSize = 300;\n        SunGlow.Glow.Size = 300;\n        SunGlow.Reflection.Texture = "weather\\sun\\reflection\\refl_night.tga";\n    }\n}\n\nvoid MoveSunGlowToLayers'),
    ],
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def prepare(relative: str, incoming: bytes) -> bytes:
    if relative not in HOOKS:
        return incoming
    text = incoming.decode("utf-8").replace("\r\n", "\n")
    base = text
    if relative.endswith("Pirates_Tavern.c") and TAVERN_CASES not in base:
        marker = '\n        case "GreatRecipe_Offer":\n'
        if marker in base:
            start = base.index(marker)
            end = base.index('\t\t//шебека Синяя Птица', start)
            previous = base[start:end]
            if digest(previous.encode()) != "85f4ec25bae1b626fbdea28ec5b8730d722f399e076040a73a3df7f368aa9195":
                raise ValueError("unreviewed prior recipe dialogue")
            base = base[:start] + base[end:]
    # Strip only this exact layer, then bind the complete prior owner.
    # Older delivered layers are admitted only by their complete reviewed base,
    # so adding a hook upgrades them without trusting a partial/foreign owner.
    for edit in reversed(EDITS[relative]):
        before, after = edit[:2]
        count = edit[2] if len(edit) == 3 else 1
        if after in base:
            if base.count(after) != count:
                raise ValueError(f"ambiguous Great Recipe layer: {relative}")
            base = base.replace(after, before, count)
    if digest(base.encode("utf-8")) != NORMALIZED_HASHES[relative]:
        raise ValueError(f"unreviewed Great Recipe input: {relative}: {digest(base.encode('utf-8'))}")
    text = base
    for edit in EDITS[relative]:
        before, after = edit[:2]
        count = edit[2] if len(edit) == 3 else 1
        if text.count(before) != count:
            raise ValueError(f"Great Recipe anchor drift: {relative}: {before[:75]!r}")
        text = text.replace(before, after, count)
    result = text.replace("\n", "\r\n").encode("utf-8")
    if result == incoming:
        return incoming
    return result


def created() -> dict[str, bytes]:
    return {relative: (SOURCE / source).read_text().replace("\n", "\r\n").encode("utf-8")
            for relative, source in CREATED.items()}


def inputs() -> dict[Path, tuple[bytes, bytes]]:
    paths = [Path(__file__).resolve(), *(SOURCE / source for source in CREATED.values())]
    return {path: (data, data) for path in paths for data in [path.read_bytes()]}
