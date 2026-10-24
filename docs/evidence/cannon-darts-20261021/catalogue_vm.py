#!/usr/bin/env python3
"""Task-local VM evidence: extract production functions, seed legacy state, roundtrip twice."""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
HERE = Path(__file__).resolve().parent
BINARY = ROOT / "experiments/native-metal/.cache/build/bin/script-vm-probe"
INSTALLED = Path("/Applications/Corsairs Iddictive Remaster.app/Contents/Resources/PROGRAM")


def public_path(path):
    path = Path(path)
    if path.is_relative_to(ROOT):
        return str(path.relative_to(ROOT))
    if path.is_relative_to(INSTALLED):
        return "<installed-game>/PROGRAM/" + str(path.relative_to(INSTALLED))
    raise ValueError("unbound provenance path")


def extract(path, name):
    try:
        text = path.read_text(encoding="utf-8-sig")
    except UnicodeDecodeError:
        text = path.read_text(encoding="cp1251")
    match = re.search(r"(?m)^(?:void|int|float|bool|ref|string)\s+" + name + r"\([^;\n]*\)[ \t]*(?://[^\n]*)?\s*\{", text)
    if not match:
        raise ValueError(f"missing function {name}: {path}")
    start = text.index("{", match.start())
    tail = text[start:]
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"', lambda m: " " * len(m[0]), tail)
    depth = 1
    end = 1
    while depth:
        depth += (masked[end] == "{") - (masked[end] == "}")
        end += 1
    return text[match.start():start + end]


def main():
    output = HERE / (sys.argv[1] if len(sys.argv) > 1 else "catalogue-vm")
    if output.exists() and any(output.iterdir()):
        raise ValueError("retain the first populated VM result; choose an explicit new output name")
    output.mkdir(exist_ok=True)
    inputs = {
        ROOT / "src/gameplay/PROGRAM/store/goods.h": ["Goods_GetDisplayIndex", "GetGoodWeightByType"],
        ROOT / "src/gameplay/PROGRAM/store/initGoods.c": ["NavalAmmo_InitDamage", "Airburst_InitGoods", "Darts_InitGoods"],
        ROOT / "src/gameplay/PROGRAM/store/storeutilite.c": ["Store_InitAirburst", "Store_EnsureAirburst", "Store_AirburstTradeType", "Store_DartsTradeType", "Store_InitDarts", "Store_EnsureDarts", "Store_CanBuyGoods", "GetStoreGoodsQuantity", "GetStoreGoodsPrice", "SetStoreGoods", "RemoveStoreGoods"],
        ROOT / "src/gameplay/PROGRAM/Loc_ai/LAi_utilites.c": ["GetCityNation"],
        ROOT / "src/gameplay/PROGRAM/worldmap/worldmap_encgen.c": ["WdmRecoveryQuote"],
        ROOT / "src/gameplay/PROGRAM/battle_interface/BattleInterface.c": ["BI_GetChargeQuantity"],
        ROOT / "src/gameplay/PROGRAM/interface/store.c": ["ADD_BUTTON", "ChangeQTY_EDIT"],
        ROOT / "src/gameplay/PROGRAM/characters/characterUtilite.c": ["GetCargoGoods", "SetCharacterGoods", "RecalculateCargoLoad", "GetCargoMaxSpace", "GetCrewQuantity"],
        INSTALLED / "colonies/colonies.c": ["FindColony"],
    }
    goods = (ROOT / "src/gameplay/PROGRAM/store/goods.h").read_text()
    definitions = "\n".join(re.findall(r"(?m)^#define\s+GOOD\w+[^\n]*", goods))
    store_defs = (INSTALLED / "store/store.h").read_text()
    definitions += "\n" + "\n".join(re.findall(r"(?m)^#define\s+TRADE_TYPE_\w+\s+\d+[^\n]*", store_defs))
    init_path = ROOT / "src/gameplay/PROGRAM/store/initGoods.c"
    init_functions = "\n\n".join(extract(init_path, name) for name in inputs[init_path])
    (output / "store").mkdir()
    (output / "store/initGoods.c").write_text(init_functions)
    extracted = "\n\n".join(extract(path, name) for path, names in inputs.items() if path != init_path for name in names)
    load_path = ROOT / "src/gameplay/PROGRAM/seadogs.c"
    load_source = load_path.read_text()
    load_start = load_source.index("void OnLoad()")
    load_body = load_source.index("{", load_start) + 1
    load_end = load_source.index(r'if (!LoadSegment("scripts\shop_rotation.c"))', load_body)
    migration = load_source[load_body:load_end]
    onload = "void OnLoad()\n{\n    if (OnLoadRuns == 0) { PriorGoods = GetArraySize(&Goods); PriorSelector = GetArraySize(&BI_ChargeState); }\n" + migration + "    OnLoadRuns++;\n}\n"
    support = r'''
#define PIRATE 4
#define MAX_COLONIES 4
#define CANNON_TYPE_NONECANNON 1000
#define SHIP_NOTUSED -1
#define SKILL_COMMERCE "Commerce"
#define PRICE_TYPE_BUY 0
#define PRICE_TYPE_SELL 1
#define STORE_QUANTITY 8
extern void Airburst_InitGoods();
extern void Darts_InitGoods();
object Goods[52];
int BI_ChargeState[6];
object Colonies[4];
object Stores[8];
object Characters[2];
object RealShips[2];
object DummyCannon;
object GameInterface;
ref pchar, refStore, refCharacter;
int iShipCapacity, iTotalSpace, BuyOrSell, iShipQty, iStoreQty, iShipPrice, iStorePrice, iUnits, iCurGoodsIdx;
float fShipWeight, fStoreWeight, fWeight;
bool bShowChangeWin;
int UpgradeRuns = 0;
int OnLoadRuns = 0;
int PriorGoods = 0;
int PriorSelector = 0;
void GreatRecipe_PreLoadActors() { }
bool GreatRecipe_Unlocked() { return false; }
bool SeaPhonk_HasUnlimitedAmmo(ref c) { return false; }
float GetSummonSkillFromNameToOld(ref c, string skill) { return stf(c.Commerce); }
bool CheckOfficersPerk(ref c, string perk) { return CheckAttribute(c, "Perks." + perk); }
ref GetCharacter(int index) { return &Characters[index]; }
ref GetCannonByType(int cannon) { return &DummyCannon; }
int GetCannonsNum(ref c) { return 0; }
bool GetRemovable(ref c) { return true; }
void SetShipWeight() { fShipWeight = 0; fStoreWeight = 0; }
void SetFormatedText(string node, string text) { }
string XI_ConvertString(string text) { return text; }
string NewStr() { return " "; }
string its(int value) { return "" + value; }
int GreatRecipe_StoreSaleLimit(ref store, int good, int quantity) { return quantity; }
void SetVariable() { }
void ShowFoodInfo() { }
void ShowItemInfo() { }
void ShowGoodsInfo(int good) { }
void ProbeCharge() { BI_GetChargeQuantity(); }
#event_handler("probe_charge", "ProbeCharge");
'''
    cases = r'''
int SeedLegacy()
{
    for (int i = 0; i < 52; i++)
    {
        Goods[i].Name = "Legacy" + i;
        Goods[i].Units = 20;
        Goods[i].Weight = 1;
        Goods[i].LegacyMarker = 1000 + i;
    }
    Goods[0].Name = "Balls";
    Goods[0].Weight = 2;
    Goods[0].DamageHull = 11.5;
    Goods[1].Name = "Grapes";
    Goods[2].Name = "Knippels";
    Goods[3].Name = "Bombs";
    Goods[3].Cost = 60;
    Goods[3].DamageHull = 19.5;
    Goods[51].Name = "Airburst";
    Colonies[0].id = "Pirates";
    Colonies[1].id = "LeFrancois";
    Colonies[2].id = "PortRoyal";
    Colonies[3].id = "LaVega";
    for (i = 0; i < 4; i++) Colonies[i].nation = PIRATE;
    Colonies[2].nation = 1;
    Stores[0].Colony = "Pirates";
    Stores[0].Goods.Bombs.Norm = 256;
    Stores[1].Colony = "LeFrancois";
    Stores[1].Goods.Bombs.Norm = 1000;
    Stores[2].Colony = "PortRoyal";
    Stores[2].Goods.Bombs.Norm = 80;
    Stores[3].Colony = "LeFrancois";
    Stores[3].Goods.Darts.Quantity = 0;
    Stores[3].Goods.Darts.Norm = 17;
    Stores[3].Goods.Darts.NotUsed = true;
    Stores[3].Goods.Darts.RndPriceModify = 0.43;
    Stores[3].Goods.Darts.NormPriceModify = 0.21;
    Stores[4].Colony = "none";
    Stores[5].Colony = "GeneratedAtSea";
    for (i = 4; i < 7; i++)
    {
        Stores[i].Goods.Darts.Quantity = 777;
        Stores[i].Goods.Darts.TradeType = TRADE_TYPE_AMMUNITION;
    }
    Stores[7].Colony = "LaVega";
    Stores[7].Goods.Darts.Quantity = 0;
    Stores[7].Goods.Darts.Norm = 24;
    Stores[7].Goods.Darts.NotUsed = false;
    Stores[7].Goods.Darts.RndPriceModify = 0.29;
    Stores[7].Goods.Darts.NormPriceModify = 0.13;
    RealShips[0].Capacity = 1000;
    for (i = 0; i < 2; i++)
    {
        Characters[i].index = i;
        Characters[i].Ship.Cannons.Type = CANNON_TYPE_NONECANNON;
        Characters[i].Ship.Type = 0;
        Characters[i].Ship.Crew.Quantity = 10;
        Characters[i].Ship.Cargo.Goods.Balls = 40;
        Characters[i].Ship.Cargo.Goods.Airburst = 9;
        Characters[i].Commerce = 0;
    }
    Characters[1].Ship.Cargo.Goods.Darts = 37;
    if (GetArraySize(&Goods) != 52 || GetArraySize(&BI_ChargeState) != 6) return 1;
    trace("legacy_seed_goods52_selector6");
    return 0;
}

int UpgradeAndCheck()
{
    if (OnLoadRuns <= UpgradeRuns) OnLoad();
    if (PriorGoods != 52 || PriorSelector != 6 || OnLoadRuns != UpgradeRuns + 1) return 2;
    int i;
    UpgradeRuns++;
    if (GetArraySize(&Goods) != 53) return 3;
    for (i = 0; i < 52; i++) { if (sti(Goods[i].LegacyMarker) != 1000 + i) return 4; }
    if (Goods[52].Name != "Darts" || sti(Goods[52].Cost) != 120) return 5;
    if (abs(stf(Goods[2].DamageHull) - 5.75) > 0.0001) return 6;
    if (GetCargoGoods(&Characters[0], GOOD_BALLS) != 40 || GetCargoGoods(&Characters[0], GOOD_AIRBURST) != 9) return 7;
    if (GetCargoGoods(&Characters[0], GOOD_DARTS) != 0 || GetCargoGoods(&Characters[1], GOOD_DARTS) != 37) return 8;
    for (i = 0; i < 3; i++)
    {
        int stockQuantity = GetStoreGoodsQuantity(&Stores[i], GOOD_DARTS);
        if (stockQuantity < 400 || stockQuantity > 600) return 9;
    }
    if (!Store_CanBuyGoods(&Stores[0], GOOD_DARTS) || !Store_CanBuyGoods(&Stores[1], GOOD_DARTS)) return 10;
    if (Store_CanBuyGoods(&Stores[2], GOOD_DARTS)) return 11;
    if (sti(Stores[2].Goods.Darts.TradeType) != TRADE_TYPE_CONTRABAND) return 12;
    if (Store_CanBuyGoods(&Stores[3], GOOD_DARTS)) return 13;
    if (sti(Stores[3].Goods.Darts.Quantity) != 0 || sti(Stores[3].Goods.Darts.Norm) != 17 || !sti(Stores[3].Goods.Darts.NotUsed)) return 14;
    if (abs(stf(Stores[3].Goods.Darts.RndPriceModify) - 0.43) > 0.0001 || abs(stf(Stores[3].Goods.Darts.NormPriceModify) - 0.21) > 0.0001) return 15;
    for (i = 4; i < 7; i++)
    {
        if (Store_CanBuyGoods(&Stores[i], GOOD_DARTS)) return 16;
        if (sti(Stores[i].Goods.Darts.TradeType) != TRADE_TYPE_CONTRABAND || sti(Stores[i].Goods.Darts.Quantity) != 777) return 17;
    }
    if (GetStoreGoodsQuantity(&Stores[7], GOOD_DARTS) != 0 || sti(Stores[7].Goods.Darts.Norm) != 24 || sti(Stores[7].Goods.Darts.NotUsed)) return 18;
    Colonies[1].nation = 1;
    if (Store_CanBuyGoods(&Stores[1], GOOD_DARTS)) return 19;
    if (sti(Stores[1].Goods.Darts.TradeType) != TRADE_TYPE_CONTRABAND || sti(Stores[1].Goods.Darts.Quantity) < 400 || sti(Stores[1].Goods.Darts.Quantity) > 600) return 20;
    Colonies[1].nation = PIRATE;
    if (!Store_CanBuyGoods(&Stores[1], GOOD_DARTS)) return 21;
    ref ch = &Characters[0];
    Stores[0].Goods.Darts.RndPriceModify = 0.0;
    Stores[0].Goods.Darts.NormPriceModify = 0.0;
    int lowPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    Stores[0].Goods.Darts.RndPriceModify = 0.15;
    Stores[0].Goods.Darts.NormPriceModify = 0.15;
    int highPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    if (highPrice <= lowPrice || lowPrice <= 0) return 22;
    ch.Commerce = 10;
    int skilledPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    ch.Perks.AdvancedCommerce = true;
    int perkPrice = GetStoreGoodsPrice(&Stores[0], GOOD_DARTS, PRICE_TYPE_BUY, ch, 1);
    if (skilledPrice >= highPrice || perkPrice >= skilledPrice) return 23;
    DeleteAttribute(ch, "Perks");
    ch.Commerce = 0;
    Stores[0].Goods.Bombs.TradeType = TRADE_TYPE_AMMUNITION;
    Stores[0].Goods.Bombs.RndPriceModify = 0.15;
    Stores[0].Goods.Bombs.NormPriceModify = 0.15;
    int bombPrice = GetStoreGoodsPrice(&Stores[0], GOOD_BOMBS, PRICE_TYPE_BUY, ch, 1);
    if (abs(highPrice - 2 * bombPrice) > 1) return 24;
    SetCharacterGoods(ch, GOOD_DARTS, 21);
    if (GetCargoGoods(ch, GOOD_DARTS) != 21 || sti(ch.Ship.Cargo.Load) != 17) return 25;
    SetCharacterGoods(ch, GOOD_DARTS, 0);
    Event("probe_charge", "l", 1);
    if (GetArraySize(&BI_ChargeState) != 7 || BI_ChargeState[0] != 6 || BI_ChargeState[5] != 37 || BI_ChargeState[6] != 9) return 26;
    object seen;
    for (i = 0; i < 53; i++)
    {
        int good = Goods_GetDisplayIndex(i);
        string key = "good" + good;
        if (good < 0 || good > 52 || CheckAttribute(&seen, key)) return 27;
        seen.(key) = true;
    }
    if (Goods_GetDisplayIndex(0) != 0 || Goods_GetDisplayIndex(3) != 3 || Goods_GetDisplayIndex(6) != 4) return 28;
    int first = Goods_GetDisplayIndex(4);
    int second = Goods_GetDisplayIndex(5);
    if (first != GOOD_DARTS || second != GOOD_AIRBURST) return 29;
    pchar = &Characters[0];
    refCharacter = pchar;
    refStore = &Stores[0];
    pchar.money = 10000;
    iCurGoodsIdx = GOOD_DARTS;
    iUnits = 20;
    fWeight = 1;
    iStoreQty = 12;
    iShipQty = 0;
    iShipPrice = highPrice;
    iStorePrice = 50;
    iShipCapacity = 1000;
    iTotalSpace = 50000;
    bShowChangeWin = true;
    BuyOrSell = 0;
    ADD_BUTTON();
    if (sti(GameInterface.qty_edit.str) != 12 || BuyOrSell != 1) return 30;
    int partialBill = makeint(iShipPrice * stf(GameInterface.qty_edit.str) / iUnits + 0.5);
    if (partialBill != 105) return 31;
    GameInterface.qty_edit.str = 1;
    ChangeQTY_EDIT();
    if (sti(GameInterface.qty_edit.str) != 1 || BuyOrSell != 1) return 32;
    trace("upgrade_round=" + UpgradeRuns + ";prices=" + lowPrice + "," + highPrice + "," + skilledPrice + "," + perkPrice + ";catalogue_specials=" + first + "," + second + ";selector5=37;selector6=9");
    return 0;
}

int DirectUpgrade()
{
    int result = SeedLegacy();
    if (result != 0) return result;
    result = UpgradeAndCheck();
    if (result != 0) return result;
    return UpgradeAndCheck();
}
'''
    (output / "cases.c").write_text(definitions + support + extracted + "\n" + onload + cases)
    direct = len(sys.argv) > 2 and sys.argv[2] == "direct"
    manifest = {
        "binary_sha256": hashlib.sha256(BINARY.read_bytes()).hexdigest(),
        "extracted_functions": {public_path(path): {"source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "names": names} for path, names in inputs.items()},
        "onload_migration_slice": {"source": public_path(load_path), "source_sha256": hashlib.sha256(load_path.read_bytes()).hexdigest(), "first_line": load_source[:load_body].count("\n") + 1, "last_line": load_source[:load_end].count("\n"), "slice_sha256": hashlib.sha256(migration.encode()).hexdigest(), "scope": "Original OnLoad prefix through store migration, with fixture counters; later scene/audio/quest loading excluded"},
        "support_stubs": ["GreatRecipe_PreLoadActors no-op", "GreatRecipe_Unlocked false", "SeaPhonk_HasUnlimitedAmmo false: ordinary-ammo acceptance only; Phonk separately verified", "commerce skill/perk projection from fixture character", "GetCharacter array lookup", "unused cannons lookup/count", "UI presentation callbacks and unconstrained capacity; quantity/goods logic extracted unchanged"],
        "fixture_population": "4 colonies,8 stores,2 characters; Goods52 and BI_ChargeState6 serialized before first upgrade",
        "execution_mode": "single-session repeated migration" if direct else "save-codec rounds",
        "claim_ceiling": "actual extracted catalogue/store/price/cargo/selector functions; codec is exercised only in save-codec mode, not accepted unless error log is empty; not whole game OnLoad, rendered UI or player acceptance",
    }
    consumers = {}
    for name in ("store", "ship", "storage", "transfer_main", "GoodsTransfer", "ColonyCapture"):
        path = ROOT / "src/gameplay/PROGRAM/interface" / (name + ".c")
        text = path.read_text(encoding="utf-8-sig")
        markers = list(re.finditer(r"Goods_GetDisplayIndex\(displayIndex\)", text))
        if len(markers) != 1:
            raise ValueError(f"expected one catalogue consumer: {path}")
        if not re.search(r"\.index\s*=\s*i\s*;", text[markers[0].end():markers[0].end() + 3000]):
            raise ValueError(f"missing original-good transaction index: {path}")
        consumers[str(path.relative_to(ROOT))] = {"source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(), "display_mapping_line": text[:markers[0].start()].count("\n") + 1, "row_index": "i, canonical good ID"}
    manifest["source_consumer_checks"] = consumers
    manifest["required_catalogue_prefix"] = [0, 1, 2, 3, 52, 51, 4]
    manifest["covered_cases"] = ["legacy array52 append53 without losing row markers", "repeat migration preserves disabled/depleted stock and price modifiers", "all pirate/current-colony nation + nation flips", "foreign/mobile/missing-colony fake ammunition rows rejected", "fresh stock bounds400–600", "dynamic prices and commerce perks, twice bombs base price", "cargo read/write/weight without pack rounding loss", "legacy selector6 grows7; slot5 Darts,6 Airburst", "53 unique catalogue IDs and six shared consumer transaction indices", "actual store ADD_BUTTON/ChangeQTY_EDIT accepts stock12 and editable quantity1 with Units20"]
    manifest["cases_sha256"] = hashlib.sha256((output / "cases.c").read_bytes()).hexdigest()
    manifest["dynamic_segment"] = {"path": "store/initGoods.c", "sha256": hashlib.sha256((output / "store/initGoods.c").read_bytes()).hexdigest()}
    manifest["trade_definitions"] = {"source": "<installed-game>/PROGRAM/store/store.h", "source_sha256": hashlib.sha256((INSTALLED / "store/store.h").read_bytes()).hexdigest()}
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    if direct:
        command = [str(BINARY), str(output), "DirectUpgrade", "0"]
    else:
        command = [str(BINARY), str(output), "SeedLegacy", "2", "UpgradeAndCheck"]
    result = subprocess.run(command, capture_output=True, text=True, timeout=30)
    (output / "run.log").write_text(result.stdout + result.stderr)
    (output / "result.json").write_text(json.dumps({"command": [public_path(BINARY), public_path(output), *command[2:]], "returncode": result.returncode, "stdout": result.stdout, "stderr": result.stderr}, indent=2) + "\n")
    print(result.stdout + result.stderr)
    raise SystemExit(result.returncode)


if __name__ == "__main__":
    main()
