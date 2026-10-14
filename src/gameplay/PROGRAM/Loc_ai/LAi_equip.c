// Registry-backed random equipment. Item identities and stats belong to Items.
bool NPC_IsRandomSupplyItem(aref item)
{
    if (!CheckAttribute(item, "id") || !CheckAttribute(item, "rare")) return false;
    if (CheckAttribute(item, "quest") || CheckAttribute(item, "unique") || CheckAttribute(item, "gold")) return false;
    if (!CheckAttribute(item, "price") || sti(item.price) <= 0) return false;
    if (CheckAttribute(item, "ItemType"))
    {
        if (item.ItemType == "QUESTITEMS" || item.ItemType == "MAP" || item.ItemType == "BOOK") return false;
    }
    return stf(item.rare) > 0.0;
}

int NPC_WeaponQualityWeight(aref item, int rank)
{
    if (!CheckAttribute(item, "quality")) return 0;
    if (CheckAttribute(item, "minlevel") && sti(item.minlevel) > rank) return 0;
    int weight = 0;
    switch (item.quality)
    {
        case "poor": weight = 30 - rank; break;
        case "ordinary": if (rank >= 5) weight = 30 - abs(rank - 12); break;
        case "good": if (rank >= 15) weight = 45 - abs(rank - 25); break;
        case "excellent": if (rank >= 30) weight = rank - 25; break;
    }
    if (weight < 1)
    {
        if (item.quality == "poor" || (item.quality == "ordinary" && rank >= 5) ||
            (item.quality == "good" && rank >= 15) || (item.quality == "excellent" && rank >= 30)) weight = 1;
    }
    return weight;
}

string NPC_SelectRegistryWeapon(int rank, string group, string fencing, bool musket, int maxCharges)
{
    string chosen = "";
    int total = 0;
    // Musket animations need a real gun even on inexperienced fighters.
    int qualityRank = rank;
    if (group == GUN_ITEM_TYPE && musket && qualityRank < 5) qualityRank = 5;
    for (int i = 0; i < ITEMS_QUANTITY; i++)
    {
        aref item; makearef(item, Items[i]);
        if (!NPC_IsRandomSupplyItem(item) || !CheckAttribute(item, "groupID")) continue;
        if (item.groupID != group) continue;
        // This admission floor never changes the NPC or an item's real level gate.
        if (CheckAttribute(item, "minlevel") && sti(item.minlevel) > rank) continue;
        int weight = NPC_WeaponQualityWeight(item, qualityRank);
        if (weight <= 0) continue;
        if (group == GUN_ITEM_TYPE)
        {
            if (isMushket(item.id) != musket) continue;
            if (!CheckAttribute(item, "chargeQ") || sti(item.chargeQ) > maxCharges) continue;
        }
        if (group == BLADE_ITEM_TYPE && fencing != "" && CheckAttribute(item, "FencingType"))
        {
            if (item.FencingType == fencing) weight = weight * 4;
        }
        // Retain registry rarity, including a reachable tail for rare weapons.
        weight = makeint(weight * stf(item.rare) * 1000.0);
        if (weight < 1) weight = 1;
        total += weight;
        if (rand(total - 1) < weight) chosen = item.id;
    }
    if (chosen == "") return "";
    return GetGeneratedItem(chosen);
}

string NPC_PreferredFencing(ref chr)
{
    string result = SKILL_FENCING;
    int best = GetCharacterSkill(chr, result);
    if (GetCharacterSkill(chr, SKILL_F_LIGHT) > best)
    {
        result = SKILL_F_LIGHT;
        best = GetCharacterSkill(chr, result);
    }
    if (GetCharacterSkill(chr, SKILL_F_HEAVY) > best) result = SKILL_F_HEAVY;
    return result;
}

string LAi_NPC_SelectWeapon(ref chr, string group, int rank)
{
    if (group == BLADE_ITEM_TYPE)
        return NPC_SelectRegistryWeapon(rank, group, NPC_PreferredFencing(chr), false, 1);
    if (group != GUN_ITEM_TYPE) return "";
    int charges = 1;
    if (CheckCharacterPerk(chr, "Gunman")) charges = 2;
    if (CheckCharacterPerk(chr, "GunProfessional")) charges = 4;
    return NPC_SelectRegistryWeapon(rank, group, "", HasSubStr(chr.model.animation, "mushketer"), charges);
}

void NPC_SupplyEquippedGun(ref chr, int rank)
{
    if (!CheckAttribute(chr, "equip.gun")) return;
    int ammunition = 6 + rank / 3;
    if (ammunition > 24) ammunition = 24;
    if (GetCharacterItem(chr, "bullet") == 0) TakeNItems(chr, "bullet", ammunition);
    if (GetCharacterItem(chr, "GunPowder") == 0) TakeNItems(chr, "GunPowder", ammunition);
}

void NPC_ReplaceOwnedEquipment(ref chr, string group, string itemID)
{
    if (CheckAttribute(chr, "NPCGeneration.equip." + group))
    {
        string previous = chr.NPCGeneration.equip.(group);
        // Never remove an authored replacement or unrelated carried items.
        if (GetCharacterEquipByGroup(chr, group) == previous)
        {
            RemoveCharacterEquip(chr, group);
            if (CheckCharacterItem(chr, previous)) RemoveItems(chr, previous, 1);
        }
        DeleteAttribute(chr, "NPCGeneration.equip." + group);
    }
    if (itemID == "") return;
    GiveItem2Character(chr, itemID);
    EquipCharacterByItem(chr, itemID);
    if (CheckAttribute(chr, "NPCGeneration.role") && GetCharacterEquipByGroup(chr, group) == itemID)
        chr.NPCGeneration.equip.(group) = itemID;
}

void LAi_NPC_Equip(ref chr, int rank, bool isWeapons, bool isGun)
{
    DelBakSkillAttr(chr);
    string kind = NPC_GetGenerationRole(chr);
    if (CheckAttribute(chr, "quest.officertype")) kind = chr.quest.officertype;
    LAi_NPC_EquipPerk(chr, kind);
    bool musket = HasSubStr(chr.model.animation, "mushketer");
    // Stock towngirl animations do not admit weapon equipment.
    if (chr.sex == "woman") return;
    if (musket && !CheckAttribute(chr, "NPCGeneration.role")) return;
    if (isWeapons && !musket && !CheckAttribute(chr, "DontChangeBlade"))
    {
        string blade = LAi_NPC_SelectWeapon(chr, BLADE_ITEM_TYPE, rank);
        NPC_ReplaceOwnedEquipment(chr, BLADE_ITEM_TYPE, blade);
    }
    else if (CheckAttribute(chr, "NPCGeneration.role") && !CheckAttribute(chr, "DontChangeBlade"))
        NPC_ReplaceOwnedEquipment(chr, BLADE_ITEM_TYPE, "");
    if (!isWeapons && !musket) isGun = false;
    if (isGun && !CheckAttribute(chr, "DontChangeGun"))
    {
        int chance = 10 + rank * 2;
        if (kind == "soldier" || kind == "cannoner" || kind == "fighter" || kind == "war" || kind == "pirate") chance += 20;
        if (chance > 90) chance = 90;
        if (musket || rand(99) < chance)
        {
            string gun = LAi_NPC_SelectWeapon(chr, GUN_ITEM_TYPE, rank);
            NPC_ReplaceOwnedEquipment(chr, GUN_ITEM_TYPE, gun);
        }
    }
    else if (CheckAttribute(chr, "NPCGeneration.role") && !CheckAttribute(chr, "DontChangeGun"))
        NPC_ReplaceOwnedEquipment(chr, GUN_ITEM_TYPE, "");
    // Supply actual equipped guns only, without accumulating ammo on re-equips.
    NPC_SupplyEquippedGun(chr, rank);
}

// Public selectors are also consumed by PGG and the ordinary soldier generator.
string LAi_NPC_EquipBladeSelection(int rank)
{
    return NPC_SelectRegistryWeapon(rank, BLADE_ITEM_TYPE, "", false, 1);
}

string LAi_Soldier_EquipBlade()
{
    return NPC_SelectRegistryWeapon(10, BLADE_ITEM_TYPE, SKILL_FENCING, false, 1);
}

string NPC_PerkGroup(aref perk)
{
    if (CheckAttribute(perk, "NPCGroup")) return perk.NPCGroup;
    if (CheckAttribute(perk, "OfficerType")) return perk.OfficerType;
    if (CheckAttribute(perk, "BaseType") && perk.BaseType == "self") return "fighter";
    return "ship";
}

int NPC_PerkRoleWeight(string kind, string group)
{
    if (group == "self") group = "fighter";
    if (kind == "fantom" || kind == "") return 3;
    if (group == kind) return 12;
    if (kind == "trade")
    {
        if (group == "treasurer") return 12;
        if (group == "navigator") return 8;
        if (group == "carpenter" || group == "doctor") return 3;
        return 1;
    }
    if (kind == "war" || kind == "pirate")
    {
        if (group == "cannoner" || group == "boatswain" || group == "fighter") return 8;
        return 3;
    }
    if (kind == "merchant" || kind == "noble")
    {
        if (group == "treasurer") return 10;
        if (group == "fighter") return 2;
        return 1;
    }
    if (kind == "soldier")
    {
        if (group == "fighter") return 12;
        return 1;
    }
    if (kind == "sailor")
    {
        if (group == "navigator" || group == "boatswain") return 8;
        return 2;
    }
    if (kind == "citizen" || kind == "monk")
    {
        if (group == "fighter") return 2;
        return 1;
    }
    if (group == "fighter") return 2;
    return 1;
}

bool NPC_PerkConditionsMet(ref chr, aref perk)
{
    if (!CheckAttribute(perk, "condition")) return true;
    aref conditions; makearef(conditions, perk.condition);
    for (int i = 0; i < GetAttributesNum(conditions); i++)
    {
        aref entry = GetAttributeN(conditions, i);
        bool needed = sti(GetAttributeValue(entry)) != 0;
        if (CheckCharacterPerk(chr, GetAttributeName(entry)) != needed) return false;
    }
    return true;
}

bool NPC_PerkCanGenerate(aref perk)
{
    if (CheckAttribute(perk, "PlayerOnly") && sti(perk.PlayerOnly)) return false;
    if (CheckAttribute(perk, "Hiden") && sti(perk.Hiden)) return false;
    if (CheckAttribute(perk, "Free") && sti(perk.Free)) return false;
    return CheckAttribute(perk, "BaseType");
}

void LAi_NPC_EquipPerk(ref chr, string kind)
{
    if (!CheckAttribute(&ChrPerksList, "list")) return;
    int rank = sti(chr.rank);
    int budget = 1 + rank / 2;
    if (kind == "citizen" || kind == "monk" || kind == "merchant" || kind == "noble") budget = 1 + rank / 8;
    if (budget > 26) budget = 26;
    aref registry; makearef(registry, ChrPerksList.list);
    int learned = 0;
    for (int i = 0; i < GetAttributesNum(registry); i++)
    {
        aref perk = GetAttributeN(registry, i);
        if (NPC_PerkCanGenerate(perk) && CheckCharacterPerk(chr, GetAttributeName(perk))) learned++;
    }
    // Each pass learns one currently eligible perk; dependencies stay truthful.
    while (learned < budget)
    {
        string chosen = "";
        int total = 0;
        for (int j = 0; j < GetAttributesNum(registry); j++)
        {
            aref candidate = GetAttributeN(registry, j);
            string name = GetAttributeName(candidate);
            if (!NPC_PerkCanGenerate(candidate) || CheckCharacterPerk(chr, name)) continue;
            if (!NPC_PerkConditionsMet(chr, candidate)) continue;
            int weight = NPC_PerkRoleWeight(kind, NPC_PerkGroup(candidate));
            total += weight;
            if (rand(total - 1) < weight) chosen = name;
        }
        if (chosen == "") break;
        SetCharacterPerk(chr, chosen);
        learned++;
    }
}

string NPC_PocketItemRole(string role)
{
    if (role == "soldier" || role == "war") return "Solder";
    if (role == "cannoner" || role == "boatswain") return "Solder_o";
    if (role == "fighter" || role == "pirate" || role == "sailor") return "Warrior";
    return "Citizen";
}

void NPC_GeneratePocketInventory(ref chr)
{
    if (!CheckAttribute(chr, "NPCGeneration.role") || CheckAttribute(chr, "NPCGeneration.pocketsGenerated")) return;
    string role = NPC_PocketItemRole(NPC_GetGenerationRole(chr));
    if (chr.sex == "woman") role = "Citizen_f";
    int rank = sti(chr.rank);
    int limit = 1 + rank / 10;
    if (limit > 6) limit = 6;
    int kinds = 0;
    bool hasMedicine = false;
    string medicine = "";
    int medicineWeight = 0;
    for (int i = 0; i < ITEMS_QUANTITY; i++)
    {
        aref item; makearef(item, Items[i]);
        if (!NPC_IsRandomSupplyItem(item)) continue;
        if (CheckAttribute(item, "groupID")) continue;
        if (CheckAttribute(item, "minlevel") && sti(item.minlevel) > rank) continue;
        if (CheckAttribute(item, "potion.health") && stf(item.potion.health) > 0.0)
        {
            if (GetCharacterItem(chr, item.id) > 0) hasMedicine = true;
            int weight = makeint(stf(item.rare) * 1000.0);
            if (weight < 1) weight = 1;
            medicineWeight += weight;
            if (rand(medicineWeight - 1) < weight) medicine = item.id;
        }
        if (kinds >= limit || !CheckAttribute(item, role + ".rare")) continue;
        aref policy; makearef(policy, item.(role));
        if (!CheckAttribute(policy, "min") || !CheckAttribute(policy, "max")) continue;
        if (rand(999) >= makeint(stf(policy.rare) * 1000.0)) continue;
        int quantity = sti(policy.min);
        int maximum = sti(policy.max);
        if (quantity < 0 || maximum < quantity) continue;
        quantity += rand(maximum - quantity);
        if (quantity <= 0) continue;
        AddItems(chr, item.id, quantity);
        if (CheckAttribute(item, "potion.health") && stf(item.potion.health) > 0.0) hasMedicine = true;
        kinds++;
    }
    // A small usable medicine supply, selected by the registry's own rarity.
    if (!hasMedicine && medicine != "")
    {
        int medicineQuantity = 1 + rank / 15;
        if (medicineQuantity > 4) medicineQuantity = 4;
        AddItems(chr, medicine, medicineQuantity);
    }
    chr.NPCGeneration.pocketsGenerated = true;
}

string GetEnglishMushketName()
{
	string gun = "mushket";
	switch(rand(2))
	{
		case 0: gun = "mushket"; break;
		case 1: gun = "mushket_SeaCarbine"; break;
		case 2: gun = "mushket_english"; break;
		case 3: gun = "mushket"; break;
		case 4: gun = "mushket_SeaCarbine"; break;
		case 5: gun = "mushket_english"; break;
	}

	return gun;
}

string GetFranceMushketName()
{
	string gun = "mushket";
	switch(rand(2))
	{
		case 0: gun = "mushket"; break;
		case 1: gun = "mushket2"; break;
		case 2: gun = "mushket_france"; break;
		case 3: gun = "mushket"; break;
		case 4: gun = "mushket2"; break;
		case 5: gun = "mushket_france"; break;
	}

	return gun;
}

string GetHollandMushketName()
{
	string gun = "mushket";
	switch(rand(2))
	{
		case 0: gun = "mushket"; break;
		case 1: gun = "mushket2"; break;
		case 2: gun = "mushket_holland"; break;
		case 3: gun = "mushket"; break;
		case 4: gun = "mushket2"; break;
		case 5: gun = "mushket_holland"; break;
	}

	return gun;
}

string GetSpanishMushketName()
{
	string gun = "mushket";
	switch(rand(2))
	{
		case 0: gun = "mushket"; break;
		case 1: gun = "mushket2"; break;
		case 2: gun = "mushket_spanish"; break;
		case 3: gun = "mushket"; break;
		case 4: gun = "mushket2"; break;
		case 5: gun = "mushket_spanish"; break;
	}

	return gun;
}

string GetPirateMushketName()
{
	string gun = "mushket";
	switch(rand(2))
	{
		case 0: gun = "mushket"; break;
		case 1: gun = "mushket2"; break;
		case 2: gun = "mushket_drob"; break;
		case 3: gun = "mushket"; break;
		case 4: gun = "mushket2"; break;
		case 5: gun = "mushket_drob"; break;
	}

	return gun;
}

bool isMushket(string name)
{
	if(HasSubStr(name, "mushket")) return true;
	return false;
}
