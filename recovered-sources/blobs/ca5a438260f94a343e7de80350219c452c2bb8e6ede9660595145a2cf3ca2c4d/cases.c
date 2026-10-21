object SaveMaterialFixture;
string ExpectedMaterialPayload;
string OversizedSaveFixture;

int SeedMaterialSave()
{
    ExpectedMaterialPayload = "";
    for (int n = 0; n < 512; n++) ExpectedMaterialPayload = ExpectedMaterialPayload + "0123456789abcdef";
    SaveMaterialFixture.quest.stage = 7;
    SaveMaterialFixture.inventory.gold = 12345;
    SaveMaterialFixture.Ship.HullMaterial.version = 1;
    SaveMaterialFixture.Ship.HullMaterial.model = "fixture";
    SaveMaterialFixture.Ship.HullMaterial.revision = "2";
    SaveMaterialFixture.Ship.HullMaterial.metaA.revision = "1";
    SaveMaterialFixture.Ship.HullMaterial.metaA.remaining = "0.9";
    SaveMaterialFixture.Ship.HullMaterial.metaB.revision = "2";
    SaveMaterialFixture.Ship.HullMaterial.metaB.remaining = "0.8";
    SaveMaterialFixture.Ship.HullMaterial.n0.a.revision = "1";
    SaveMaterialFixture.Ship.HullMaterial.n0.a.name = "hull";
    SaveMaterialFixture.Ship.HullMaterial.n0.a.blob = "dataA";
    SaveMaterialFixture.Ship.HullMaterial.n0.a.payload = "old-id";
    SaveMaterialFixture.Ship.HullMaterial.n0.b.revision = "2";
    SaveMaterialFixture.Ship.HullMaterial.n0.b.name = "hull";
    SaveMaterialFixture.Ship.HullMaterial.n0.b.blob = "dataB";
    SaveMaterialFixture.Ship.HullMaterial.n0.b.payload = "new-id";
    SaveMaterialFixture.Ship.HullMaterial.n0.b.tail = 0;
    SaveMaterialFixture.Ship.HullMaterial.n0.dataA.id = "old-id";
    SaveMaterialFixture.Ship.HullMaterial.n0.dataA.bytes = 8192;
    SaveMaterialFixture.Ship.HullMaterial.n0.dataA.chunks = 1;
    SaveMaterialFixture.Ship.HullMaterial.n0.dataA.p0 = ExpectedMaterialPayload;
    SaveMaterialFixture.Ship.HullMaterial.n0.dataB.id = "new-id";
    SaveMaterialFixture.Ship.HullMaterial.n0.dataB.bytes = 8192;
    SaveMaterialFixture.Ship.HullMaterial.n0.dataB.chunks = 1;
    SaveMaterialFixture.Ship.HullMaterial.n0.dataB.p0 = ExpectedMaterialPayload;
    return 0;
}

int CheckMaterialSave()
{
    if (SaveMaterialFixture.quest.stage != "7") return 1;
    if (SaveMaterialFixture.inventory.gold != "12345") return 2;
    if (CheckAttribute(&SaveMaterialFixture, "Ship.HullMaterial.n0.a")) return 3;
    if (CheckAttribute(&SaveMaterialFixture, "Ship.HullMaterial.n0.dataA")) return 4;
    if (CheckAttribute(&SaveMaterialFixture, "Ship.HullMaterial.metaA")) return 5;
    if (!CheckAttribute(&SaveMaterialFixture, "Ship.HullMaterial.n0.b")) return 6;
    if (SaveMaterialFixture.Ship.HullMaterial.n0.dataB.p0 != ExpectedMaterialPayload) return 7;
    if (SaveMaterialFixture.Ship.HullMaterial.metaB.remaining != "0.8") return 8;
    return 0;
}

int AtomicSaveSeed()
{
    SeedMaterialSave();
    if (!SaveEngineState("atomic-state.bin")) return 91;
    return 0;
}

int AtomicSaveReject()
{
    SeedMaterialSave();
    OversizedSaveFixture = "0123456789abcdef";
    for (int n = 0; n < 23; n++) OversizedSaveFixture = OversizedSaveFixture + OversizedSaveFixture;
    if (SaveEngineState("atomic-state.bin")) return 92;
    OversizedSaveFixture = "";
    return 0;
}

void OnLoad() {}

int LegacyIgnoredSaveResult()
{
    SeedMaterialSave();
    int before = 17;
    SaveEngineState("legacy-ignored-first.bin");
    SaveEngineState("legacy-ignored-second.bin");
    int after = before * 3 + 5;
    if (after != 56) return 93;
    if (SaveMaterialFixture.quest.stage != "7") return 94;
    return 0;
}

object WorldSaveFixture;

int SeedWorldSave()
{
    SeedMaterialSave();
    WorldSaveFixture.Islands.i0.name = "PuertoRico";
    WorldSaveFixture.Islands.i0.owner = 42;
    WorldSaveFixture.Islands.i0.PhonkWorldCuts.version = 3;
    WorldSaveFixture.Islands.i0.PhonkWorldCuts.count = 1;
    WorldSaveFixture.Islands.i0.PhonkWorldMaterial.version = 1;
    WorldSaveFixture.Islands.i0.PhonkWorldMaterial.model = "world";
    WorldSaveFixture.Islands.i0.PhonkWorldMaterial.payload = ExpectedMaterialPayload;
    WorldSaveFixture.Islands.i1.name = "SaintMartin";
    WorldSaveFixture.Islands.i1.PHONKWORLDCUTS.count = 7;
    WorldSaveFixture.Islands.i1.phonkworldmaterial.payload = ExpectedMaterialPayload;
    WorldSaveFixture.Islands.i2.name = "untouched";
    WorldSaveFixture.Islands.i2.PhonkWorldCutsExtra = "keep";
    WorldSaveFixture.Islands.i2.note = "PhonkWorldMaterial";
    WorldSaveFixture.quest.PhonkWorldCuts.count = 2;
    WorldSaveFixture.quest.progress = 5;
    return 0;
}

int CheckWorldSave()
{
    int material = CheckMaterialSave();
    if (material != 0) return material;
    aref island;
    for (int i = 0; i < 3; i++)
    {
        string key = "i" + i;
        makearef(island, WorldSaveFixture.Islands.(key));
        if (CheckAttribute(island, "PhonkWorldCuts")) return 101;
        if (CheckAttribute(island, "PhonkWorldMaterial")) return 102;
    }
    if (WorldSaveFixture.Islands.i0.name != "PuertoRico") return 103;
    if (WorldSaveFixture.Islands.i0.owner != "42") return 104;
    if (WorldSaveFixture.Islands.i1.name != "SaintMartin") return 105;
    if (WorldSaveFixture.Islands.i2.PhonkWorldCutsExtra != "keep") return 106;
    if (WorldSaveFixture.Islands.i2.note != "PhonkWorldMaterial") return 107;
    if (CheckAttribute(&WorldSaveFixture, "quest.PhonkWorldCuts")) return 108;
    if (WorldSaveFixture.quest.progress != "5") return 109;
    return 0;
}
