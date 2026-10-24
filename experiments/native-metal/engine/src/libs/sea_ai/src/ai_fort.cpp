#include "ai_fort.h"
#include "shared/messages.h"
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <optional>

static AIShip *FindFortTarget(AIFort::AI_FORT &fort, AICannon &cannon, float speed)
{
    auto *character = fort.GetACharacter();
    const CVECTOR muzzle = cannon.GetPos();
    const bool fixedFrame = AICannon::UsesFixedBallisticFrame(character);
    const float heightScale = fixedFrame ? AICannon::GetBallisticHeightScale(character) : 1.f;
    float angle = .35f; // Preserve the ordinary fort battery's existing range.
    if (fixedFrame)
    {
        auto *parameters = character->FindAClass(character, "Ship.Cannons");
        angle = manual_aim::maximumRangeElevation(muzzle.y / heightScale, speed,
            parameters->GetAttributeAsFloat("FireAngMin"), parameters->GetAttributeAsFloat("FireAngMax"),
            AIHelper::fGravity);
    }
    const float range = AICannon::CalcMaxFireDistance(muzzle.y / heightScale, speed, angle);
    AIShip *target = nullptr;
    float nearest = 1e10f;
    for (auto *ship : AIShip::AIShips)
    {
        if (ship->isDead() || !Helper.isEnemy(character, ship->GetACharacter())) continue;
        const float distance = ship->GetDistance(muzzle);
        if (distance > range || !(distance < nearest)) continue;
        if (fixedFrame)
        {
            CVECTOR destination = ship->GetPos();
            destination.y = muzzle.y + (destination.y - muzzle.y) / heightScale;
            if (!manual_aim::reachable(muzzle, destination, speed, AIHelper::fGravity)) continue;
        }
        nearest = distance;
        target = ship;
    }
    return target;
}

// This is fort policy, not ammunition balance. Keep a real ordinary share even
// beyond its reach: those guns reload or wait rather than acquiring dart range.
static float FortDartProbability(float distance, float ordinaryRange)
{
    const float relative = ordinaryRange > 0.f && std::isfinite(distance) ? distance / ordinaryRange : 0.f;
    const float t = std::clamp((relative - .35f) / 1.f, 0.f, 1.f);
    return .08f + .52f * t * t * (3.f - 2.f * t);
}

class FortCannonContext
{
    // These are precisely the derived launch fields written by the script
    // recalculation owner. Cache only this Execute/initialization, never across
    // frames, skills/perks changes, scene transitions, or restored games.
    static constexpr const char *fields[] = {"SpeedV0", "FireAngMin", "FireAngMax", "BallHeightMultiply",
        "BallTimeSpeedMultiply", "FireAngleSineMultiply"};
    struct Parameters
    {
        uint32_t type = 0, charge = 0, fixed = 0;
        float values[std::size(fields)]{};
        void Read(ATTRIBUTES *attributes)
        {
            type = attributes->GetAttributeAsDword("Type");
            charge = attributes->GetAttributeClass("Charge")->GetAttributeAsDword("Type");
            fixed = attributes->GetAttributeAsDword("FixedBallisticFrame");
            for (size_t n = 0; n < std::size(fields); ++n) values[n] = attributes->GetAttributeAsFloat(fields[n]);
        }
        void Apply(ATTRIBUTES *attributes) const
        {
            attributes->SetAttributeUseDword("Type", type);
            attributes->GetAttributeClass("Charge")->SetAttributeUseDword("Type", charge);
            attributes->SetAttributeUseDword("FixedBallisticFrame", fixed);
            for (size_t n = 0; n < std::size(fields); ++n) attributes->SetAttributeUseFloat(fields[n], values[n]);
        }
    };
    AIFort::AI_FORT &fort;
    ATTRIBUTES *character, *parameters, *charge, *fortState, *ammo, *profiles;
    Parameters original;
    std::string originalName;
    std::vector<Parameters> cache;
    uint32_t ordinal = 0;

  public:
    explicit FortCannonContext(AIFort::AI_FORT &owner) : fort(owner)
    {
        character = fort.GetACharacter();
        parameters = character->CreateSubAClass(character, "Ship.Cannons");
        charge = parameters->GetAttributeClass("Charge");
        fortState = character->CreateSubAClass(character, "Fort.Cannons");
        ammo = character->CreateSubAClass(character, "Fort.Cannons.Ammo");
        profiles = character->FindAClass(character, "Fort.Cannons.AmmoTypes");
        original.Read(parameters);
        originalName = charge->GetValue();
    }
    ~FortCannonContext()
    {
        if (auto *active = fortState->GetAttributeClass("ActiveGun")) fortState->DeleteAttributeClassX(active);
        original.Apply(parameters);
        charge->SetValue(originalName);
    }
    ATTRIBUTES *Profile(uint32_t type) const
    {
        for (size_t i = 0; profiles && i < profiles->GetAttributesNum(); ++i)
        {
            auto *profile = profiles->GetAttributeClass(static_cast<uint32_t>(i));
            if (profile->GetAttributeAsDword() == type) return profile;
        }
        return nullptr;
    }
    ATTRIBUTES *FlaggedProfile(const char *flag) const
    {
        for (size_t i = 0; profiles && i < profiles->GetAttributesNum(); ++i)
        {
            auto *profile = profiles->GetAttributeClass(static_cast<uint32_t>(i));
            if (profile->GetAttributeAsDword(flag)) return profile;
        }
        return nullptr;
    }
    ATTRIBUTES *State(uint32_t index)
    {
        const auto key = std::string("gun") + std::to_string(index);
        auto *state = ammo->CreateSubAClass(ammo, key.c_str());
        if (!state->GetAttributeClass("Charge"))
            state->SetAttributeUseDword("Charge", FlaggedProfile("Default")->GetAttributeAsDword());
        return state;
    }
    void Apply(ATTRIBUTES *profile)
    {
        const uint32_t type = fort.GetCannonType(ordinal), id = profile->GetAttributeAsDword();
        charge->SetValue(profile->GetAttributeClass("Name")->GetValue());
        for (const auto &entry : cache)
            if (entry.type == type && entry.charge == id) { entry.Apply(parameters); return; }
        parameters->SetAttributeUseDword("Type", type);
        charge->SetAttributeUseDword("Type", id);
        core.Event(CANNON_RECALCULATE_PARAMETERS, "l", GetIndex(character));
        Parameters entry;
        entry.Read(parameters);
        cache.push_back(entry);
    }
    void Bind(uint32_t index)
    {
        ordinal = index;
        const auto key = std::string("gun") + std::to_string(index);
        fortState->SetAttribute("ActiveGun", std::string_view(key));
        auto *profile = Profile(State(index)->GetAttributeAsDword("Charge"));
        Apply(profile ? profile : FlaggedProfile("Default"));
    }
    float Speed() const { return parameters->GetAttributeAsFloat("SpeedV0"); }
    bool HasAnyTarget()
    {
        auto *current = Profile(State(ordinal)->GetAttributeAsDword("Charge"));
        bool found = false;
        for (size_t i = 0; profiles && i < profiles->GetAttributesNum() && !found; ++i)
        {
            Apply(profiles->GetAttributeClass(static_cast<uint32_t>(i)));
            found = FindFortTarget(fort, *fort.GetCannon(ordinal), Speed()) != nullptr;
        }
        Apply(current ? current : FlaggedProfile("Default"));
        return found;
    }
    bool PrepareNextCharge(bool refundReserved = false)
    {
        auto *state = State(ordinal);
        if (state->GetAttributeClass("NextCharge")) return true;
        const auto decline = [&]
        {
            // Absence means a legacy/unchanged charge. An explicit declined
            // draw must not silently reload the old dart on automatic Load.
            if (!refundReserved) state->SetAttributeUseDword("NextEmpty", true);
            return false;
        };
        auto *cargo = character->FindAClass(character, "Ship.Cargo.Goods");
        if (!cargo || (!cargo->GetAttributeAsDword("Powder") &&
            !(refundReserved && state->GetAttributeAsDword("Reserved")))) return decline();
        auto *baseline = FlaggedProfile("Baseline");
        auto *darts = FlaggedProfile("LongRange");
        auto *current = Profile(state->GetAttributeAsDword("Charge"));
        Apply(baseline);
        const CVECTOR muzzle = fort.GetCannon(ordinal)->GetPos();
        const float ordinaryRange = AICannon::CalcMaxFireDistance(muzzle.y, Speed(), .35f);
        float nearest = std::numeric_limits<float>::infinity();
        for (auto *ship : AIShip::AIShips)
            if (!ship->isDead() && Helper.isEnemy(character, ship->GetACharacter()))
                nearest = std::min(nearest, ship->GetDistance(muzzle));
        uint32_t intact = 0, plannedDarts = 0;
        for (uint32_t i = 0; i < fort.GetAllCannonsNum(); ++i)
        {
            auto *cannon = fort.GetCannon(i);
            if (cannon->isDamaged()) continue;
            ++intact;
            if (i == ordinal) continue;
            auto *other = State(i);
            const uint32_t dartId = darts->GetAttributeAsDword();
            if ((!cannon->isEmpty() && other->GetAttributeAsDword("Charge") == dartId) ||
                (other->GetAttributeClass("NextCharge") && other->GetAttributeAsDword("NextCharge") == dartId))
                ++plannedDarts;
        }
        const uint32_t dartLimit = std::max(1u, static_cast<uint32_t>(intact * .65f));
        const auto available = [&](ATTRIBUTES *profile)
        {
            const auto &name = profile->GetAttributeClass("Name")->GetValue();
            // A deliberate reload returns this gun's reserved round first.
            return cargo->GetAttributeAsDword(name.c_str()) > 0 ||
                (refundReserved && state->GetAttributeAsDword("Reserved") &&
                 state->GetAttributeAsDword("Charge") == profile->GetAttributeAsDword());
        };
        ATTRIBUTES *selected = nullptr;
        if (FRAND(1.f) < FortDartProbability(nearest, ordinaryRange) && plannedDarts < dartLimit && available(darts))
            selected = darts;
        else
        {
            float total = 0.f;
            for (size_t i = 0; i < profiles->GetAttributesNum(); ++i)
            {
                auto *profile = profiles->GetAttributeClass(static_cast<uint32_t>(i));
                if (!profile->GetAttributeAsDword("LongRange") && available(profile))
                    total += profile->GetAttributeAsDword("Rare") ? .35f : 1.f;
            }
            float draw = FRAND(total);
            for (size_t i = 0; total > 0.f && i < profiles->GetAttributesNum(); ++i)
            {
                auto *profile = profiles->GetAttributeClass(static_cast<uint32_t>(i));
                if (profile->GetAttributeAsDword("LongRange") || !available(profile)) continue;
                draw -= profile->GetAttributeAsDword("Rare") ? .35f : 1.f;
                if (draw <= 0.f) { selected = profile; break; }
            }
        }
        Apply(current ? current : FlaggedProfile("Default"));
        if (!selected) return decline();
        if (auto *empty = state->GetAttributeClass("NextEmpty")) state->DeleteAttributeClassX(empty);
        state->SetAttributeUseDword("NextCharge", selected->GetAttributeAsDword());
        return true;
    }
};


// Character attributes outlive a sea scene. Keep each physical cannon's
// damage here as well as in the existing native sea-save stream.
static void PersistFortCannons(AIFort::AI_FORT &fort, bool restore)
{
    auto *character = fort.GetACharacter();
    auto *state = character->CreateSubAClass(character, "Fort.Cannons");
    auto *damage = state->CreateSubAClass(state, "Damage");
    bool hit = false;
    for (uint32_t i = 0; i < fort.GetAllCannonsNum(); ++i)
    {
        const auto key = std::string("gun") + std::to_string(i);
        auto *cannon = fort.GetCannon(i);
        if (restore && damage->GetAttribute(key.c_str()))
            cannon->SetDamage(std::clamp(damage->GetAttributeAsFloat(key.c_str()), 0.0f, 1.0f));
        damage->SetAttributeUseFloat(key.c_str(), cannon->GetDamage());
        hit = hit || cannon->GetDamage() > 0.0f;
    }
    state->SetAttributeUseDword("Hit", hit);
    state->SetAttributeUseDword("Destroyed", fort.GetDamagedCannonsNum());
}

AIFort *AIFort::pAIFort = nullptr;

void AIFort::ReconcileWorldCuts(ISLAND_BASE &island, int32_t owner)
{
    for (auto *fort : aForts)
    {
        bool changed = false;
        for (uint32_t n = 0; n < fort->GetAllCannonsNum(); ++n)
        {
            auto *cannon = fort->GetCannon(n);
            if (!cannon->isDamaged() && !island.IsWorldPositionRetained(fort->GetModelEID(), cannon->GetPos()))
            { cannon->SetDamage(1.f); changed = true; }
        }
        if (!changed) continue;
        PersistFortCannons(*fort, false);
        core.Event("SeaPhonk_FortGeometry", "al", fort->GetACharacter(), owner);
    }
}

AIFort::AIFort()
{
    dtFiredTimer.Setup(FRAND(1.0f), 1.0f);
    pLastTraceFort = nullptr;
    pShipsLights = nullptr;
    pAIFort = this;
}

AIFort::~AIFort()
{
    pAIFort = nullptr;
    for (auto &aFort : aForts)
        STORM_DELETE(aFort);
}

bool AIFort::Init()
{
    return true;
}

void AIFort::SetDevice()
{
}

float AIFort::GetSpeedV0(uint32_t dwFortIndex)
{
    ATTRIBUTES *pACannon, *pACharacter;
    pACharacter = aForts[dwFortIndex]->GetACharacter();
    pACannon = pACharacter->FindAClass(pACharacter, "Ship.Cannons");
    Assert(pACannon);
    return pACannon->GetAttributeAsFloat("SpeedV0");
}

// float fAngF = 0.0f;
// float yyy = 0.0f;

void AIFort::Execute(uint32_t Delta_Time)
{
    /*CVECTOR vSrc = CVECTOR(-10000.0f * sinf(fAngF), yyy, -10000.0f * cosf(fAngF));
    CVECTOR vDst = -CVECTOR(vSrc.x, yyy, vSrc.z);
    vDst.y = yyy;
    Trace(vSrc, vDst);*/
    fMinCannonDamageDistance = AttributesPointer->GetAttributeAsFloat("MinCannonDamageDistance");

    const auto fDeltaTime = static_cast<float>(Delta_Time) * 0.001f;
    if (!aForts.size())
        return;

    const auto bFiredTimer = dtFiredTimer.Update(fDeltaTime);

    for (uint32_t k = 0; k < aForts.size(); k++)
    {
        auto *pF = aForts[k];
        if (pF->isDead())
            continue;
        if (!pF->isNormalMode())
            continue;

        std::optional<FortCannonContext> context;
        const auto iMax = pF->GetAllCannonsNum(); // boal fix
        for (uint32_t i = 0; i < iMax; i++)
        {
            auto *pC = pF->GetCannon(i);
            if (!pC->isDamaged() && (pC->IsFireDue(fDeltaTime) ||
                (bFiredTimer && (pC->isReady2Fire() || pC->isEmpty()))))
            {
                if (!context) context.emplace(*pF);
                context->Bind(i);
                if (!pC->isFired() && bFiredTimer)
                {
                    auto *target = pC->isReady2Fire() ? FindFortTarget(*pF, *pC, context->Speed()) : nullptr;
                    if (target)
                    {
                        if (pC->Fire(context->Speed(), target->GetPos())) context->PrepareNextCharge();
                    }
                    else if (context->HasAnyTarget() && context->PrepareNextCharge(true))
                    {
                        // An unreachable loaded shell is returned, not fired
                        // with another shell's range. Reroll once per reload,
                        // never every firing timer while the gun is charging.
                        pC->Recharge();
                    }
                }
            }
            pC->Execute(fDeltaTime);
        }
    }
}

void AIFort::Realize(uint32_t Delta_Time)
{
    /*entid_t eidIsland;
    float fCurrentImmersion = 0.0f;
    if (core.FindClass(&eidIsland, "Island", 0))
    {
      ISLAND_BASE * pIsland = (ISLAND_BASE *)core.GetEntityPointer(eidIsland);
      fCurrentImmersion = pIsland->GetCurrentImmersion();
    }*/

    /*for (uint32_t k=0; k<aForts.size(); k++)
    {
      CMatrix mTemp;
      mTemp.BuildPosition(0.0f, -fCurrentImmersion, 0.0f);
      aForts[k]->GetModel()->mtx = aForts[k]->mOldMatrix * mTemp;

      pShipsLights->SetLights(&aForts[k]->tmpObject);
      aForts[k]->GetModel()->Realize(Delta_Time);
      pShipsLights->UnSetLights(&aForts[k]->tmpObject);

      aForts[k]->GetModel()->mtx = aForts[k]->mOldMatrix;
    }*/

    /*   // boal del_cheat
  #ifndef XBOX
    if (core.Controls->GetDebugAsyncKeyState('X') < 0)
    {
      CMatrix mI; mI.SetIdentity();
      AIHelper::pRS->SetTransform(D3DTS_WORLD, mI);

      int32_t iNumCannons = 0;
      for (uint32_t k=0; k<aForts.size(); k++)
      {
        AI_FORT    * pF = aForts[k];
        for (uint32_t i=0; i<pF->GetAllCannonsNum(); i++)
        {
          AICannon * pC = pF->GetCannon(i);
          CVECTOR vPos = pC->GetPos();
          CMatrix mRot(0.0f, pC->GetDirY(), 0.0f);
          AIHelper::pRS->DrawVector(vPos, vPos + mRot * CVECTOR(0.0f, 0.0f, 3.0f), 0xFFFFFFFF);
          AIHelper::pRS->DrawSphere(vPos, 0.3f, 0xFF00FF00);
          iNumCannons++;
        }
      }
    }
  #endif
  */
}

bool AIFort::CreateState(ENTITY_STATE_GEN *state_gen)
{
    return true;
}

bool AIFort::LoadState(ENTITY_STATE *state)
{
    return true;
}

bool AIFort::AddFort(ATTRIBUTES *pIslandAP, ATTRIBUTES *pFortLabelAP, ATTRIBUTES *pFortCharacter, entid_t eidModel,
                     entid_t eidBlot)
{
    Assert(pFortLabelAP);

    auto *pFortAP = pFortLabelAP->FindAClass(pFortLabelAP, "fort");
    Assert(pFortAP);
    auto *const pModelAP = pFortAP->FindAClass(pFortAP, "model");
    Assert(pModelAP);
    auto *const pLocatorsAP = pFortAP->FindAClass(pFortAP, "locators");
    Assert(pLocatorsAP);
    auto *const pModelsDirAP = pIslandAP->FindAClass(pIslandAP, "filespath.models");
    Assert(pModelsDirAP);

    const char *pModelName = pModelAP->GetThisAttr();
    Assert(pModelName);
    const char *pLocatorsName = pLocatorsAP->GetThisAttr();
    Assert(pLocatorsName);
    const char *pModelsDir = pModelsDirAP->GetThisAttr();
    Assert(pModelsDir);

    auto *pFort = new AI_FORT(pFortLabelAP);
    pFort->SetACharacter(pFortCharacter);
    pFort->SetModelEID(eidModel);
    pFort->SetBlotEID(eidBlot);
    // pFort->pFortLabelAP = pFortLabelAP;
    aForts.push_back(pFort);

    auto *pACannonsType1 = pFortCharacter->FindAClass(pFortCharacter, "Fort.Cannons.Type.1");
    Assert(pACannonsType1);
    auto *pACannonsType2 = pFortCharacter->FindAClass(pFortCharacter, "Fort.Cannons.Type.2");
    Assert(pACannonsType2);
    auto *pACannonsType3 = pFortCharacter->FindAClass(pFortCharacter, "Fort.Cannons.Type.3");
    Assert(pACannonsType3);
    pFort->dwCannonType = pACannonsType1->GetAttributeAsDword();
    pFort->dwCulverinType = pACannonsType2->GetAttributeAsDword();
    pFort->dwMortarType = pACannonsType3->GetAttributeAsDword();

    pFort->mOldMatrix = pFort->GetModel()->mtx;

    ScanFortForCannons(pFort, pModelsDir, pLocatorsName);

    auto *pALights = pFortCharacter->FindAClass(GetACharacter(), "ship.lights");
    auto *pAFlares = pFortCharacter->FindAClass(GetACharacter(), "ship.flares");

    const auto bLights = (pALights) ? pALights->GetAttributeAsDword() != 0 : false;
    const auto bFlares = (pAFlares) ? pAFlares->GetAttributeAsDword() != 0 : false;

    const auto eidTmp = core.GetEntityId("shiplights");
    pShipsLights = static_cast<IShipLights *>(core.GetEntityPointer(eidTmp));
    Assert(pShipsLights);

    pShipsLights->AddLights(&pFort->tmpObject, pFort->GetModel(), bLights, bFlares);
    pShipsLights->ProcessStage(Stage::execute, 0);

    core.Event(FORT_CREATE, "al", pFortCharacter, pFort->GetAllCannonsNum());

    return true;
}

void AIFort::AddFortHit(int32_t iCharacterIndex, CVECTOR &vHitPos, float radius, CANNON_TRACE_BASE *island)
{
    const bool airburst = radius > 0.0f;
    const float maxDistance = airburst ? radius : fMinCannonDamageDistance;
    uint32_t i, j, iMax;
    for (i = 0; i < aForts.size(); i++)
    {
        auto *pF = aForts[i];

        if (airburst && (!pF->GetModel() || !pF->GetACharacter() || !pF->isNormalMode() ||
                        GetIndex(pF->GetACharacter()) == iCharacterIndex))
            continue;

        iMax = pF->GetAllCannonsNum(); // boal fix
        for (j = 0; j < iMax; j++)
        {
            auto *pC = pF->GetCannon(j);

            if (pC->isDamaged())
                continue;

            auto vPos = pC->GetPos();
            const auto fDistance = sqrtf(~(vPos - vHitPos));
            if (fDistance > maxDistance)
                continue;

            if (airburst)
            {
                // Test the exposed muzzle, not a locator embedded in masonry.
                const CVECTOR muzzle = vPos + pC->GetDir() * 3.0f;
                if (Trace(vHitPos, muzzle) < 1.0f || (island && island->Trace(vHitPos, muzzle) < 1.0f))
                    continue;
            }

            // VDATA * pVData = core.Event(FORT_CANNON_DAMAGE, "llallfffff", iCharacterIndex,
            // GetIndex(pF->GetACharacter()), pF->pFortLabelAP, pF->GetAllCannonsNum(), pF->GetDamagedCannonsNum(),
            // vPos.x, vPos.y, vPos.z, fDistance, pC->GetDamage()); Assert(pVData);
            auto *pVData = core.Event(FORT_CANNON_DAMAGE, "llallfffff", iCharacterIndex, GetIndex(pF->GetACharacter()),
                                      pF->pFortLabelAP, iMax, pF->GetDamagedCannonsNum(), vPos.x, vPos.y, vPos.z,
                                      fDistance, pC->GetDamage());
            Assert(pVData);

            pC->SetDamage(pVData->GetFloat());
            PersistFortCannons(*pF, false);
        }
    }
}

AIFort::AI_FORT *AIFort::FindFort(entid_t eidModel)
{
    for (auto &aFort : aForts)
    {
        if (core.GetEntityPointer(aFort->GetModelEID()) == core.GetEntityPointer(eidModel))
            return aFort;
    }
    return nullptr;
}

uint64_t AIFort::ProcessMessage(MESSAGE &message)
{
    CVECTOR vHit;
    int32_t iCharacterIndex;
    entid_t eidFortModel, eidBlot;
    ATTRIBUTES *pFortAPLabel, *pCharacter, *pIslandAP;
    AI_FORT *pFort = nullptr;

    switch (message.Long())
    {
    case AI_MESSAGE_FORT_HIT:
        iCharacterIndex = message.Long();
        vHit.x = message.Float();
        vHit.y = message.Float();
        vHit.z = message.Float();
        AddFortHit(iCharacterIndex, vHit);
        break;
    case AI_MESSAGE_ADD_FORT:
        pIslandAP = message.AttributePointer();
        pFortAPLabel = message.AttributePointer();
        pCharacter = message.AttributePointer();
        eidFortModel = message.EntityID();
        eidBlot = message.EntityID();
        AddFort(pIslandAP, pFortAPLabel, pCharacter, eidFortModel, eidBlot);
        Helper.AddCharacter(pCharacter, pCharacter);
        break;
    case AI_MESSAGE_FORT_SET_LIGHTS:
        pFort = FindFort(message.EntityID());
        if (pShipsLights && pFort)
            pShipsLights->SetLights(&pFort->tmpObject);
        break;
    case AI_MESSAGE_FORT_UNSET_LIGHTS:
        pFort = FindFort(message.EntityID());
        if (pShipsLights && pFort)
            pShipsLights->UnSetLights(&pFort->tmpObject);
        break;
    }
    return 0;
}

uint32_t AIFort::AttributeChanged(ATTRIBUTES *pAttribute)
{
    return 0;
}

bool AIFort::Mount(ATTRIBUTES *pAttribute)
{
    return true;
}

bool AIFort::ScanFortForCannons(AI_FORT *pFort, const char *pModelsDir, const char *pLocatorsName) const
{
    entid_t model_id;
    GEOS::LABEL label;
    GEOS::INFO info;
    NODE *pNode;
    // std::string        sLocatorsName;

    const auto path = std::filesystem::path() / pModelsDir / pLocatorsName;
    const auto pathStr = path.string();
    // MessageBoxA(NULL, (LPCSTR)path.c_str(), "", MB_OK); //~!~
    // sLocatorsName.Format("%s\\%s", pModelsDir, pLocatorsName);
    model_id = core.CreateEntity("MODELR");
    core.Send_Message(model_id, "ls", MSG_MODEL_LOAD_GEO, (char *)pathStr.c_str());

    auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(model_id));
    Assert(pModel);

    // search and add cannons & culverins
    uint32_t dwIdx = 0;
    while (pNode = pModel->GetNode(dwIdx))
    {
        pNode->geo->GetInfo(info);
        for (int32_t i = 0; i < info.nlabels; i++)
        {
            pNode->geo->GetLabel(i, label);
            AICannon *pCannon = nullptr;

            if (storm::iEquals(label.name, "cannon", 6) && pFort->dwCannonType != -1)
            {
                pFort->aCannons.push_back(AICannon{});
                pCannon = &pFort->aCannons.back();
                // pCannon = &pFort->aCannons[pFort->aCannons.Add()];
            }
            if (storm::iEquals(label.name, "culverin", 8) && pFort->dwCulverinType != -1)
            {
                pFort->aCulverins.push_back(AICannon{});
                pCannon = &pFort->aCulverins.back();
                // pCannon = &pFort->aCulverins[pFort->aCulverins.Add()];
            }
            if (storm::iEquals(label.name, "mortar", 6) && pFort->dwMortarType != -1)
            {
                pFort->aMortars.push_back(AICannon{});
                pCannon = &pFort->aMortars.back();
                // pCannon = &pFort->aMortars[pFort->aMortars.Add()];
                pCannon->SetType(AICannon::CANNONTYPE::CANNONTYPE_MORTAR);
            }

            if (pCannon)
            {
                pCannon->Init(pFort, GetId(), label);
            }
        }
        dwIdx++;
    }

    PersistFortCannons(*pFort, true);
    // Locator files can interleave cannon kinds. Only the final grouped order
    // matches the native Save/Load loops and is a stable per-gun state key.
    FortCannonContext context(*pFort);
    auto *character = pFort->GetACharacter();
    const bool restoring = character->FindAClass(character, "Fort.Cannons")->GetAttributeAsDword("Restoring");
    for (uint32_t i = 0; i < pFort->GetAllCannonsNum(); ++i)
    {
        context.Bind(i);
        if (restoring || pFort->GetCannon(i)->isDamaged()) continue;
        context.PrepareNextCharge();
        pFort->GetCannon(i)->QuickRecharge(true);
    }

    core.EraseEntity(model_id);
    return true;
}

bool AIFort::isAttack(AIGroup *pGroup)
{
    return false;
}

bool AIFort::isDead()
{
    return false;
}

float AIFort::GetPower()
{
    return 10000.0f;
}

ATTRIBUTES *AIFort::GetACharacter()
{
    if (pLastTraceFort)
        return pLastTraceFort->GetACharacter();
    return nullptr;
}

float AIFort::Trace(const CVECTOR &vSrc, const CVECTOR &vDst)
{
    pLastTraceFort = nullptr;

    auto fBestRes = 2.0f;
    auto fBestDistance = 1e10f;
    for (uint32_t i = 0; i < GetNumForts(); i++)
    {
        auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(GetFort(i)->GetModelEID()));
        Assert(pModel);
        const auto fRes = pModel->Trace(vSrc, vDst);
        if (fRes > 1.0f)
            continue;
        const auto fDistance = fRes * sqrtf(~(vDst - vSrc));
        if (fDistance < fBestDistance)
        {
            pLastTraceFort = GetFort(i);
            fBestDistance = fDistance;
            fBestRes = fRes;
        }
    }

    return fBestRes;
}

float AIFort::Cannon_Trace(int32_t iBallOwner, const CVECTOR &vSrc, const CVECTOR &vDst)
{
    float fBestRes = 2.0;
    for (uint32_t i = 0; i < GetNumForts(); i++)
    {
        auto *pModel = static_cast<MODEL *>(core.GetEntityPointer(GetFort(i)->GetModelEID()));

        const float fRes = pModel->Trace(vSrc, vDst);
        if (fRes < fBestRes)
            fBestRes = fRes;
        if (fRes <= 1.0f)
        {
            const CVECTOR vTemp = vSrc + (vDst - vSrc) * fRes;
            core.Event(BALL_FORT_HIT, "lfff", iBallOwner, vTemp.x, vTemp.y, vTemp.z);

            const CVECTOR vDir = !(vDst - vSrc);
            core.Send_Message(GetFort(i)->GetBlotEID(), "lffffff", MSG_BLOTS_HIT, vTemp.x, vTemp.y, vTemp.z, vDir.x,
                              vDir.y, vDir.z);
        }
    }
    return fBestRes;
}

AIFort::AI_FORT *AIFort::FindFort(ATTRIBUTES *pACharacter)
{
    for (uint32_t i = 0; i < GetNumForts(); i++)
    {
        if (GetFort(i)->GetACharacter() == pACharacter)
            return GetFort(i);
    }
    return nullptr;
}

CVECTOR AIFort::AI_FORT::GetAttackPoint(VAI_INNEROBJ *pObj)
{
    const float fDistance = pObj->GetMaxFireDistance();
    const uint32_t iMax = GetAllCannonsNum(); // boal fix
    for (uint32_t i = 0; i < iMax; i++)
    {
        AICannon *pC = GetCannon(i);
        if (pC->isDamaged())
            continue;

        const CVECTOR vCPos = pC->GetPos();
        const float fDirY = pC->GetDirY();

        const CVECTOR vRes = vCPos + fDistance / 1.4f * CVECTOR(sinf(fDirY), 0.0f, cosf(fDirY));

        // check for ship can place here
        // pObj->

        return (vRes - GetPos());
    }

    return 0.0f;
}

void AIFort::Save(CSaveLoad *pSL)
{
    pSL->SaveFloat(fMinCannonDamageDistance);
    for (auto &aFort : aForts)
        aFort->Save(pSL);
}

void AIFort::Load(CSaveLoad *pSL)
{
    fMinCannonDamageDistance = pSL->LoadFloat();
    for (auto &aFort : aForts)
        aFort->Load(pSL, GetId());
}

void AIFort::AI_FORT::Save(CSaveLoad *pSL)
{
    PersistFortCannons(*this, false);
    pSL->SaveVector(vPos);

    uint32_t i;
    for (i = 0; i < aCannons.size(); i++)
        aCannons[i].Save(pSL);
    for (i = 0; i < aCulverins.size(); i++)
        aCulverins[i].Save(pSL);
    for (i = 0; i < aMortars.size(); i++)
        aMortars[i].Save(pSL);
}

void AIFort::AI_FORT::Load(CSaveLoad *pSL, entid_t eid)
{
    vPos = pSL->LoadVector();

    uint32_t i;
    for (i = 0; i < aCannons.size(); i++)
    {
        aCannons[i].Load(pSL, this, eid);
        if (aCannons[i].isDamaged())
        {
            const CVECTOR vPos = aCannons[i].GetPos();
            core.Event(FORT_LOADDMGCANNON, "fff", vPos.x, vPos.y, vPos.z);
        }
    }

    for (i = 0; i < aCulverins.size(); i++)
    {
        aCulverins[i].Load(pSL, this, eid);
        if (aCulverins[i].isDamaged())
        {
            const CVECTOR vPos = aCulverins[i].GetPos();
            core.Event(FORT_LOADDMGCANNON, "fff", vPos.x, vPos.y, vPos.z);
        }
    }

    for (i = 0; i < aMortars.size(); i++)
    {
        aMortars[i].Load(pSL, this, eid);
        if (aMortars[i].isDamaged())
        {
            const CVECTOR vPos = aMortars[i].GetPos();
            core.Event(FORT_LOADDMGCANNON, "fff", vPos.x, vPos.y, vPos.z);
        }
    }
    // Legacy saves have no ammo map and therefore retain their original bomb
    // interpretation. Current saves restore the same map without consuming a
    // second round during scene reconstruction or changing the native codec.
    FortCannonContext context(*this);
    for (i = 0; i < GetAllCannonsNum(); ++i)
        context.State(i)->SetAttributeUseDword("Reserved", !GetCannon(i)->isEmpty() && !GetCannon(i)->isDamaged());
    PersistFortCannons(*this, false);
}

void AIFort::Fire(const CVECTOR &vPos)
{
    for (auto &aFort : aForts)
        pShipsLights->AddDynamicLights(&aFort->tmpObject, vPos);
}
