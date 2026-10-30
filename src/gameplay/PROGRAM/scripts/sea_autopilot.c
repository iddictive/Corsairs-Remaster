// Player policy only: Characters, RealShips and the ordinary player economy
// remain authoritative. This fleet is never an ordinary traffic descriptor.
int SeaAutopilot_EncounterGeneration = 0;
#event_handler("SeaAutopilot_Encounter", "SeaAutopilot_Encounter");

void SeaAutopilot_CancelEncounter()
{
    SeaAutopilot_EncounterGeneration++;
    DeleteAttribute(pchar, "Ship.Autopilot.pendingEncounter");
}

bool SeaAutopilot_MapControl()
{
    if (bSeaActive || wdmLockReload || !IsEntity(&worldMap) || !SeaAutopilot_Eligible()) return false;
    return CheckAttribute(&worldMap, "isLoaded") && worldMap.isLoaded == "true" &&
        CheckAttribute(pchar, "Ship.Autopilot.active") && sti(pchar.Ship.Autopilot.active);
}

// Native encounter events run inside the map's Update. Defer its destruction
// to the event queue; repeated callbacks share one pending handoff.
bool SeaAutopilot_QueueEncounter(int index)
{
    if (index < 0 || !SeaAutopilot_MapControl()) return false;
    bool pending = CheckAttribute(pchar, "Ship.Autopilot.pendingEncounter");
    pchar.Ship.Autopilot.pendingEncounter = index;
    if (!pending)
    {
        SeaAutopilot_EncounterGeneration++;
        PostEvent("SeaAutopilot_Encounter", 1, "l", SeaAutopilot_EncounterGeneration);
    }
    return true;
}

void SeaAutopilot_Encounter()
{
    int generation = GetEventData();
    SeaAutopilot_EnterEncounter(generation);
}

void SeaAutopilot_EnterEncounter(int generation)
{
    if (generation != SeaAutopilot_EncounterGeneration || !CheckAttribute(pchar, "Ship.Autopilot.pendingEncounter")) return;
    int index = sti(pchar.Ship.Autopilot.pendingEncounter);
    DeleteAttribute(pchar, "Ship.Autopilot.pendingEncounter");
    if (!SeaAutopilot_MapControl() || !CheckAttribute(pchar, "eshipIndex") || sti(pchar.eshipIndex) != index) return;
    if (!wdmSetCurrentShipData(index)) { SeaAutopilot_Status("waiting", "encounter-expired"); return; }
    wdmReloadToSea();
}

int SeaAutopilot_Mode()
{
    if (!CheckAttribute(pchar, "Ship.Autopilot.mode")) return 0;
    return sti(pchar.Ship.Autopilot.mode);
}

void SeaAutopilot_Status(string status, string reason)
{
    int leg = 0; int port = -1;
    if (CheckAttribute(pchar, "Ship.Autopilot.leg")) leg = sti(pchar.Ship.Autopilot.leg);
    if (CheckAttribute(pchar, "Ship.Autopilot.port")) port = sti(pchar.Ship.Autopilot.port);
    if (!CheckAttribute(pchar, "Ship.Autopilot.status") || pchar.Ship.Autopilot.status != status ||
        !CheckAttribute(pchar, "Ship.Autopilot.blocked") || pchar.Ship.Autopilot.blocked != reason)
        Trace("SeaAutopilot mode=" + SeaAutopilot_Mode() + " leg=" + leg + " port=" + port + " status=" + status + " reason=" + reason);
    pchar.Ship.Autopilot.status = status;
    pchar.Ship.Autopilot.blocked = reason;
}

bool SeaAutopilot_Ship(ref captain)
{
    if (!FleetService_IsShip(captain) || LAi_IsDead(captain)) return false;
    if (CheckAttribute(captain, "ShipTaskLock") || CheckAttribute(captain, "SeaSurrender") ||
        CheckAttribute(captain, "Surrendered")) return false;
    int type = GetCharacterShipType(captain);
    return type >= 0 && type < REAL_SHIPS_QUANTITY && CheckAttribute(captain, "Ship.HP") && stf(captain.Ship.HP) > 0.0;
}

bool SeaAutopilot_Port(int port)
{
    if (port < 0 || port >= MAX_COLONIES || WdmTrafficPortLocator(port) == "") return false;
    if (!CheckAttribute(&Colonies[port], "nation") || Colonies[port].nation == "none") return false;
    int nation = sti(Colonies[port].nation);
    if (nation < 0 || nation >= MAX_NATIONS || GetNationRelation2MainCharacter(nation) == RELATION_ENEMY) return false;
    int store = FindStore(Colonies[port].id);
    return store >= 0 && store != SHIP_STORE;
}

bool SeaAutopilot_Endpoint(int port, ref endpoint)
{
    if (!SeaAutopilot_Port(port)) return false;
    aref resources;
    if (!GetEntity(&resources, "WdmStaticResources")) return false;
    return SendMessage(resources, "sas", "SeaLocator", endpoint, WdmTrafficPortLocator(port));
}

float SeaAutopilot_X()
{
    if (bSeaActive && bSeaLoaded) return Sea_MapX(stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z));
    return stf(worldMap.playerShipX);
}

float SeaAutopilot_Z()
{
    if (bSeaActive && bSeaLoaded) return Sea_MapZ(stf(pchar.Ship.Pos.x), stf(pchar.Ship.Pos.z));
    return stf(worldMap.playerShipZ);
}

bool SeaAutopilot_OwnTask()
{
    if (!CheckAttribute(pchar, "Ship.Autopilot.order") || !CheckAttribute(pchar, "SeaAI.Task")) return false;
    int task = sti(pchar.SeaAI.Task);
    if (task != sti(pchar.Ship.Autopilot.order)) return false;
    if (task == AITASK_MOVE)
        return CheckAttribute(pchar, "SeaAI.Task.Pos.x") && CheckAttribute(pchar, "SeaAI.Task.Pos.z") &&
            abs(stf(pchar.SeaAI.Task.Pos.x) - stf(pchar.Ship.Autopilot.orderX)) < 1.0 &&
            abs(stf(pchar.SeaAI.Task.Pos.z) - stf(pchar.Ship.Autopilot.orderZ)) < 1.0;
    if (task == AITASK_ATTACK)
        return CheckAttribute(pchar, "SeaAI.Task.Target") && CheckAttribute(pchar, "Ship.Autopilot.enemy") &&
            sti(pchar.SeaAI.Task.Target) == sti(pchar.Ship.Autopilot.enemy);
    return task == AITASK_DRIFT;
}

void SeaAutopilot_Release()
{
    SeaAutopilot_CancelEncounter();
    if (bSeaActive && bSeaLoaded && SeaAutopilot_OwnTask())
    {
        Ship_SetTaskNone(PRIMARY_TASK, nMainCharacterIndex);
        SendMessage(&AISea, "llla", AI_MESSAGE_SHIP_SET_TASK, AITASK_NONE, PRIMARY_TASK, pchar);
    }
    pchar.Ship.Autopilot.active = 0;
    DeleteAttribute(pchar, "Ship.Autopilot.order");
}

void SeaAutopilot_Stop()
{
    SeaAutopilot_Release();
    pchar.Ship.Autopilot.mode = 0;
    SeaAutopilot_Status("manual", "");
}

void SeaAutopilot_SetMode(int mode)
{
    if (mode < 0 || mode > 2) return;
    if (mode == 0) { SeaAutopilot_Stop(); return; }
    if (SeaAutopilot_Mode() != mode)
    {
        SeaAutopilot_Release();
        pchar.Ship.Autopilot.mode = mode;
        DeleteAttribute(pchar, "Ship.Autopilot.port");
        DeleteAttribute(pchar, "Ship.Autopilot.lastPort");
        DeleteAttribute(pchar, "Ship.Autopilot.review");
        DeleteAttribute(pchar, "Ship.Autopilot.huntTarget");
        DeleteAttribute(pchar, "Ship.Autopilot.salePort");
        DeleteAttribute(pchar, "Ship.Autopilot.saleFloor");
    }
    pchar.Ship.Stopped = false;
    SeaAutopilot_Tick();
}

// Native restoration owns actor state; bind only the control projection.
void SeaAutopilot_Bind()
{
    SeaAutopilot_CancelEncounter();
    if (CheckAttribute(pchar, "Ship.Autopilot")) pchar.Ship.Autopilot.active = 0;
    if (SeaAutopilot_Mode() == 0) return;
    pchar.Ship.Stopped = false;
    SeaAutopilot_Tick();
}

bool SeaAutopilot_Eligible()
{
    if (SeaAutopilot_Mode() < 1 || SeaAutopilot_Mode() > 2 || !SeaAutopilot_Ship(pchar)) return false;
    if (bSeaReloadStarted || bAbordageStarted || sti(InterfaceStates.Launched)) return false;
    if (!bSeaActive && !IsEntity(&worldMap)) return false;
    if (!bSeaActive && (!CheckAttribute(&worldMap, "playerShipX") || !CheckAttribute(&worldMap, "playerShipZ"))) return false;
    if (bSeaActive && (!bSeaLoaded || !IsEntity(pchar))) return false;
    if (CheckAttribute(pchar, "ShipTaskLock") || CheckAttribute(pchar, "SeaSurrender") ||
        CheckAttribute(pchar, "Surrendered")) return false;
    if (bSeaActive && CheckAttribute(pchar, "Ship_SetTaskAbordage")) return false;
    if (bSeaActive && CheckAttribute(pchar, "Ship.Stopped") && sti(pchar.Ship.Stopped)) pchar.Ship.Stopped = false;
    if (bSeaActive && CheckAttribute(pchar, "SeaAI.Task") && (sti(pchar.SeaAI.Task) == AITASK_ABORDAGE ||
        sti(pchar.SeaAI.Task) == AITASK_BRANDER)) return false;
    int group = Group_FindGroup(PLAYER_GROUP);
    if (bSeaActive && group >= 0 && CheckAttribute(&AIGroups[group], "Task.Lock") && sti(AIGroups[group].Task.Lock)) return false;
    return true;
}

int SeaAutopilot_NearestPort()
{
    float x = SeaAutopilot_X(); float z = SeaAutopilot_Z();
    float nearest = 1000000000000.0;
    int selected = -1;
    for (int port = 0; port < MAX_COLONIES; port++)
    {
        if (!SeaAutopilot_Port(port)) continue;
        object endpoint;
        if (!SeaAutopilot_Endpoint(port, &endpoint)) continue;
        float dx = stf(endpoint.x) - x; float dz = stf(endpoint.z) - z;
        if (dx * dx + dz * dz >= nearest) continue;
        nearest = dx * dx + dz * dz; selected = port;
    }
    return selected;
}

bool SeaAutopilot_Route(int port)
{
    object endpoint;
    if (!SeaAutopilot_Endpoint(port, &endpoint))
    { SeaAutopilot_Status("waiting", "port-locator"); return false; }
    pchar.Ship.Autopilot.port = port;
    pchar.Ship.Autopilot.targetX = endpoint.x;
    pchar.Ship.Autopilot.targetZ = endpoint.z;
    int leg = 0;
    if (CheckAttribute(pchar, "Ship.Autopilot.leg")) leg = sti(pchar.Ship.Autopilot.leg);
    pchar.Ship.Autopilot.leg = leg + 1;
    pchar.Ship.Autopilot.settledLeg = -1;
    DeleteAttribute(pchar, "Ship.Autopilot.review");
    return true;
}

int SeaAutopilot_Reserve()
{
    return Treasurer_Setting("ReserveGold", 0, 0, 100000000);
}

bool SeaAutopilot_Good(ref store, int good)
{
    return WdmTrafficStoreGood(store, good) && Store_CanBuyGoods(store, good);
}

int SeaAutopilot_Free(int good)
{
    int room = 0;
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        ref captain = &Characters[index];
        if (!SeaAutopilot_Ship(captain)) continue;
        // Paid provisions are already aboard before a freight decision. The
        // player's remaining real hold is usable; NPC fill ratios are not caps.
        int weight = GetCargoMaxSpace(captain) - GetCargoLoad(captain);
        if (weight > 0) room += GetGoodQuantityByWeight(good, weight);
    }
    return room;
}

bool SeaAutopilot_Profile(ref profile)
{
    aref resources;
    if (!GetEntity(&resources, "WdmStaticResources")) return false;
    if (!SendMessage(resources, "sa", "AutopilotSpeedProfile", profile)) return false;
    return CheckAttribute(profile, "mapMaxSpeed") && stf(profile.mapMaxSpeed) > 0.0 &&
        CheckAttribute(profile, "knotsToMeters") && stf(profile.knotsToMeters) > 0.0;
}

float SeaAutopilot_Days(ref origin, ref target, ref profile)
{
    float dx = stf(target.x) - stf(origin.x); float dz = stf(target.z) - stf(origin.z);
    float distance = sqrt(dx * dx + dz * dz);
    if (distance <= 12.0) return -1.0;
    float hours;
    string context = "map";
    if (bSeaActive)
    {
        context = "sea";
        if (!CheckAttribute(&Weather, "Time.speed") || stf(Weather.Time.speed) <= 0.0) return -1.0;
        float speed = 1000000.0;
        for (int slot = 0; slot < COMPANION_MAX; slot++)
        {
            int index = GetCompanionIndex(pchar, slot);
            if (index < 0 || !SeaAutopilot_Ship(&Characters[index])) continue;
            ref captain = &Characters[index];
            float available = FindShipSpeed(captain);
            if (CheckAttribute(captain, "Ship.MaxSpeedZ") && stf(captain.Ship.MaxSpeedZ) > 0.0)
                available = stf(captain.Ship.MaxSpeedZ);
            if (available < speed) speed = available;
        }
        if (speed <= 0.0 || speed == 1000000.0) return -1.0;
        dx = Sea_LocalX(stf(target.x), stf(target.z)) - Sea_LocalX(stf(origin.x), stf(origin.z));
        dz = Sea_LocalZ(stf(target.x), stf(target.z)) - Sea_LocalZ(stf(origin.x), stf(origin.z));
        hours = sqrt(dx * dx + dz * dz) / (speed * stf(profile.knotsToMeters)) * stf(Weather.Time.speed) / 3600.0;
    }
    else
    {
        if (!CheckAttribute(&worldMap, "date.hourPerSec") || stf(worldMap.date.hourPerSec) <= 0.0 ||
            !CheckAttribute(&worldMap, "shipSpeedOppositeWind") || !CheckAttribute(&worldMap, "shipSpeedOverWind")) return -1.0;
        float wind = stf(worldMap.shipSpeedOppositeWind);
        if (stf(worldMap.shipSpeedOverWind) < wind) wind = stf(worldMap.shipSpeedOverWind);
        if (wind <= 0.0) return -1.0;
        hours = distance / (stf(profile.mapMaxSpeed) * wind) * stf(worldMap.date.hourPerSec);
    }
    // Observed slower legs may raise a forecast, never counterfeit faster travel.
    string observation = context + "HoursPerUnit";
    if (CheckAttribute(pchar, "Ship.Autopilot.economy." + observation))
    {
        float observed = stf(pchar.Ship.Autopilot.economy.(observation)) * distance;
        if (observed > hours) hours = observed;
    }
    if (hours < 1.0) hours = 1.0;
    return hours / 24.0;
}

// Forecast only: the ordinary calendar/debt owner still accrues and pays wages.
void SeaAutopilot_Liabilities(float days, int monthly, ref result)
{
    result.wages = 0.0;
    result.reserve = CrewDebt_GetTotal("overdue");
    int month = GetDataMonth();
    if (!CheckAttribute(&NullCharacter, "SalayPayMonth") || sti(NullCharacter.SalayPayMonth) != month)
        result.reserve = sti(result.reserve) + monthly + CrewDebt_GetTotal("current");
    float left = GetMonthDays(month) - GetDataDay() + 1.0 - GetTime() / 24.0;
    bool first = true;
    while (days > 0.0)
    {
        float used = days;
        if (used > left) used = left;
        result.wages = stf(result.wages) + monthly * used / GetMonthDays(month);
        days -= used;
        if (days <= 0.0) break;
        result.reserve = sti(result.reserve) + monthly;
        if (first) result.reserve = sti(result.reserve) + CrewDebt_GetTotal("current");
        first = false;
        if (stf(result.reserve) >= sti(pchar.Money)) return;
        month++; if (month > 12) month = 1;
        left = GetMonthDays(month);
    }
}

bool SeaAutopilot_Plan(int origin, int destination, ref market, ref stock, ref profile, int monthly, int food, ref plan)
{
    DeleteAttribute(plan, "");
    object from, to;
    if (!SeaAutopilot_Endpoint(origin, &from) || !SeaAutopilot_Endpoint(destination, &to)) { plan.failStep = 1; return false; }
    float days = SeaAutopilot_Days(&from, &to, profile);
    if (days <= 0.0) { plan.failStep = 2; return false; }
    int seller = FindStore(Colonies[origin].id); int buyer = FindStore(Colonies[destination].id);
    object liabilities; SeaAutopilot_Liabilities(days, monthly, &liabilities);
    int reserve = SeaAutopilot_Reserve() + sti(liabilities.reserve);
    int budget = sti(pchar.Money) - reserve;
    if (budget <= 0) { plan.failStep = 3; plan.budget = budget; plan.reserve = reserve; return false; }
    plan.origin = origin; plan.destination = destination; plan.days = days; plan.reserve = reserve;
    plan.cost = 0; plan.revenue = 0.0; plan.advantage = 0.0;
    plan.upkeep = stf(liabilities.wages) + days * food * stf(stock.foodPrice) / sti(Goods[GOOD_FOOD].Units);
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0 || !SeaAutopilot_Ship(&Characters[index])) continue;
        ref captain = &Characters[index]; string hull = "ship" + slot;
        plan.(hull).index = index; plan.(hull).id = captain.id; plan.(hull).type = captain.Ship.Type;
        plan.(hull).room = GetCargoMaxSpace(captain) - GetCargoLoad(captain);
        // Reserve hold capacity and cash for voyage food replenishment
        int neededFood = makeint((days + 5.0) * GetShipDailyFood(captain, GetCrewQuantity(captain)));
        int heldFood = GetCargoGoods(captain, GOOD_FOOD);
        if (heldFood < neededFood)
        {
            int extraFood = neededFood - heldFood;
            int foodWeight = GetGoodWeightByType(GOOD_FOOD, extraFood);
            if (foodWeight > sti(plan.(hull).room)) return false;
            int foodCost = makeint(extraFood * 1.0 * stf(stock.foodPrice) / sti(Goods[GOOD_FOOD].Units));
            plan.(hull).room = sti(plan.(hull).room) - foodWeight;
            budget -= foodCost;
            if (budget <= 0) return false;
        }
    }
    object quotes;
    for (int good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
    {
        string name = Goods[good].name;
        if (good == GOOD_RUM || !SeaAutopilot_Good(&Stores[seller], good) || !WdmTrafficStoreGood(&Stores[buyer], good)) continue;
        quotes.(name).buy = stock.(name).buy;
        quotes.(name).sell = GetStoreGoodsPrice(&Stores[buyer], good, PRICE_TYPE_SELL, pchar, 1);
        quotes.(name).quantity = sti(stock.(name).quantity);
        int improvement = sti(quotes.(name).sell) - sti(stock.(name).sell);
        if (improvement > 0)
        {
            for (slot = 0; slot < COMPANION_MAX; slot++)
            {
                index = GetCompanionIndex(pchar, slot);
                if (index < 0 || !SeaAutopilot_Ship(&Characters[index])) continue;
                plan.advantage = stf(plan.advantage) + improvement * 1.0 * SeaAutopilot_SaleQuantity(&Characters[index], good) / sti(Goods[good].Units);
            }
        }
    }
    // Bounded margin-per-weight packing; every debit consumes the SAME budget,
    // per-hull room and source stock. This is a feasible plan, not a global solver.
    for (int rank = GOOD_CHOCOLATE; rank <= GOOD_SILVER; rank++)
    {
        int selected = -1; float density = 0.0;
        for (good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
        {
            name = Goods[good].name;
            if (!CheckAttribute(quotes, name + ".buy") || CheckAttribute(quotes, name + ".used") || sti(quotes.(name).quantity) < 1) continue;
            float margin = (sti(quotes.(name).sell) - sti(quotes.(name).buy)) / stf(Goods[good].Weight);
            if (sti(quotes.(name).buy) <= 0 || margin <= density) continue;
            density = margin; selected = good;
        }
        if (selected < 0) break;
        good = selected; name = Goods[good].name; quotes.(name).used = 1;
        for (slot = 0; slot < COMPANION_MAX; slot++)
        {
            hull = "ship" + slot;
            if (!CheckAttribute(plan, hull + ".index")) continue;
            captain = &Characters[sti(plan.(hull).index)];
            if (!SeaAutopilot_TradeCargo(captain, good)) continue;
            int quantity = GetGoodQuantityByWeight(good, sti(plan.(hull).room));
            if (quantity > sti(quotes.(name).quantity)) quantity = sti(quotes.(name).quantity);
            int price = sti(quotes.(name).buy); int units = sti(Goods[good].Units);
            if (price * 1.0 * quantity / units > budget) quantity = makeint(budget * 1.0 * units / price);
            if (quantity < 1) continue;
            float exact = price * 1.0 * quantity / units;
            int cost = makeint(exact); if (cost < exact) cost++;
            if (cost < 1 || cost > budget) continue;
            plan.(hull).goods.(name) = quantity;
            plan.(hull).room = sti(plan.(hull).room) - GetGoodWeightByType(good, quantity);
            budget -= cost; plan.cost = sti(plan.cost) + cost;
            plan.revenue = stf(plan.revenue) + makeint(sti(quotes.(name).sell) * 1.0 * quantity / units);
            quotes.(name).quantity = sti(quotes.(name).quantity) - quantity;
        }
    }
    plan.grossTradeProfit = stf(plan.revenue) - sti(plan.cost) + stf(plan.advantage);
    plan.net = plan.grossTradeProfit - stf(plan.upkeep);
    if (stf(plan.net) > 0.0)
        plan.score = stf(plan.net) / days;
    else
        plan.score = (stf(plan.grossTradeProfit) - days * food * stf(stock.foodPrice) / sti(Goods[GOOD_FOOD].Units)) / (days * 10.0) - 1000.0;
    return stf(plan.grossTradeProfit) > 0.0;
}

int SeaAutopilot_NextPort(int origin)
{
    object profile;
    if (!SeaAutopilot_Profile(&profile))
    { pchar.Ship.Autopilot.plan.blocked = "navigation-profile"; return -1; }
    int seller = FindStore(Colonies[origin].id);
    object market; WdmTrafficBuildMarket(&market);
    object stock;
    stock.foodPrice = GetStoreGoodsPrice(&Stores[seller], GOOD_FOOD, PRICE_TYPE_BUY, pchar, 1);
    int monthly = 0; int food = 0;
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0 || !SeaAutopilot_Ship(&Characters[index])) continue;
        monthly += GetSalaryForShip(&Characters[index]);
        food += GetShipDailyFood(&Characters[index], GetCrewQuantity(&Characters[index]));
    }
    for (int good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
    {
        string name = Goods[good].name;
        stock.(name).buy = GetStoreGoodsPrice(&Stores[seller], good, PRICE_TYPE_BUY, pchar, 1);
        stock.(name).sell = GetStoreGoodsPrice(&Stores[seller], good, PRICE_TYPE_SELL, pchar, 1);
        stock.(name).quantity = GetStoreGoodsQuantity(&Stores[seller], good);
    }
    int lastPort = -1;
    if (CheckAttribute(pchar, "Ship.Autopilot.lastPort")) lastPort = sti(pchar.Ship.Autopilot.lastPort);
    if (lastPort == origin) lastPort = -1;
    int selected = -1; float bestScore = -10000000.0;
    int selectedNet = -1; float bestNetScore = 0.0;
    int fallback = -1; float bestFallbackScore = -10000000.0;
    int fallbackNet = -1; float bestFallbackNetScore = 0.0;
    aref saved;
    DeleteAttribute(pchar, "Ship.Autopilot.plan");
    DeleteAttribute(pchar, "Ship.Autopilot.fallbackPlan");
    pchar.Ship.Autopilot.plan.blocked = "no-affordable-profitable-route";
    for (int port = 0; port < MAX_COLONIES; port++)
    {
        if (port == origin || !SeaAutopilot_Port(port)) continue;
        object plan;
        if (!SeaAutopilot_Plan(origin, port, &market, &stock, &profile, monthly, food, &plan)) continue;
        bool isLast = (lastPort >= 0 && port == lastPort);
        if (stf(plan.net) > 0.0)
        {
            if (!isLast)
            {
                if (stf(plan.score) > bestNetScore)
                {
                    bestNetScore = stf(plan.score);
                    selectedNet = port;
                    makearef(saved, pchar.Ship.Autopilot.plan); CopyAttributes(saved, &plan);
                }
            }
            else
            {
                if (stf(plan.score) > bestFallbackNetScore)
                {
                    bestFallbackNetScore = stf(plan.score);
                    fallbackNet = port;
                    makearef(saved, pchar.Ship.Autopilot.fallbackPlan); CopyAttributes(saved, &plan);
                }
            }
        }
        else
        {
            if (!isLast)
            {
                if (stf(plan.score) > bestScore)
                {
                    bestScore = stf(plan.score);
                    if (selectedNet < 0)
                    {
                        selected = port;
                        makearef(saved, pchar.Ship.Autopilot.plan); CopyAttributes(saved, &plan);
                    }
                }
            }
            else
            {
                if (stf(plan.score) > bestFallbackScore)
                {
                    bestFallbackScore = stf(plan.score);
                    if (fallbackNet < 0)
                    {
                        fallback = port;
                        makearef(saved, pchar.Ship.Autopilot.fallbackPlan); CopyAttributes(saved, &plan);
                    }
                }
            }
        }
    }
    if (selectedNet >= 0)
    {
        DeleteAttribute(pchar, "Ship.Autopilot.fallbackPlan");
        return selectedNet;
    }
    aref fbPlan;
    if (fallbackNet >= 0)
    {
        makearef(saved, pchar.Ship.Autopilot.plan);
        makearef(fbPlan, pchar.Ship.Autopilot.fallbackPlan);
        CopyAttributes(saved, fbPlan);
        DeleteAttribute(pchar, "Ship.Autopilot.fallbackPlan");
        return fallbackNet;
    }
    if (selected >= 0)
    {
        DeleteAttribute(pchar, "Ship.Autopilot.fallbackPlan");
        return selected;
    }
    if (fallback >= 0)
    {
        makearef(saved, pchar.Ship.Autopilot.plan);
        makearef(fbPlan, pchar.Ship.Autopilot.fallbackPlan);
        CopyAttributes(saved, fbPlan);
        DeleteAttribute(pchar, "Ship.Autopilot.fallbackPlan");
        return fallback;
    }
    DeleteAttribute(pchar, "Ship.Autopilot.fallbackPlan");
    return -1;
}

// Cargo commodities only. Delivery flags apply across the player's fleet;
// configured per-hull stocks and crew supplies remain aboard.
bool SeaAutopilot_TradeCargo(ref captain, int good)
{
    if (good < GOOD_CHOCOLATE || good > GOOD_SILVER || good == GOOD_RUM) return false;
    string name = Goods[good].name;
    if (CheckAttribute(captain, "Ship.Cargo.Goods." + name + ".isquest") ||
        CheckAttribute(pchar, "Ship.Cargo.Goods." + name + ".isquest")) return false;
    return true;
}

int SeaAutopilot_SaleQuantity(ref captain, int good)
{
    if (!SeaAutopilot_TradeCargo(captain, good)) return 0;
    string name = Goods[good].name;
    int quantity = GetCargoGoods(captain, good);
    if (CheckAttribute(captain, "TransferGoods." + name))
    {
        int reserve = sti(captain.TransferGoods.(name));
        if (reserve > 0) quantity -= reserve;
    }
    if (quantity < 0) quantity = 0;
    return quantity;
}

// Existing cargo has no reliable acquisition cost. Compare actual destination
// bids with today's local bid, then reuse the ordinary distance-weighted route.
int SeaAutopilot_SalePort(int origin)
{
    int seller = FindStore(Colonies[origin].id);
    object cargo;
    for (int good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
    {
        string name = Goods[good].name;
        int quantity = 0;
        for (int slot = 0; slot < COMPANION_MAX; slot++)
        {
            int index = GetCompanionIndex(pchar, slot);
            if (index < 0 || !SeaAutopilot_Ship(&Characters[index])) continue;
            quantity += SeaAutopilot_SaleQuantity(&Characters[index], good);
        }
        cargo.(name) = quantity;
    }
    int selected = -1; int total = 0;
    for (int port = 0; port < MAX_COLONIES; port++)
    {
        if (port == origin || !SeaAutopilot_Port(port)) continue;
        int buyer = FindStore(Colonies[port].id);
        float advantage = 0.0;
        for (good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
        {
            name = Goods[good].name;
            quantity = sti(cargo.(name));
            if (quantity < 1) continue;
            if (!WdmTrafficStoreGood(&Stores[buyer], good)) continue;
            int bid = GetStoreGoodsPrice(&Stores[buyer], good, PRICE_TYPE_SELL, pchar, 1);
            int local = GetStoreGoodsPrice(&Stores[seller], good, PRICE_TYPE_SELL, pchar, 1);
            if (bid <= local) continue;
            advantage += (bid - local) * 1.0 * quantity / sti(Goods[good].Units);
        }
        if (advantage <= 0.0) continue;
        if (advantage > 100000.0) advantage = 100000.0;
        int weight = makeint(WdmTrafficRouteWeight(origin, port, 1) * (1.0 + advantage / 100.0));
        if (weight < 1) continue;
        total += weight;
        if (rand(total - 1) < weight) selected = port;
    }
    return selected;
}

void SeaAutopilot_PlanSale(int origin, int destination)
{
    pchar.Ship.Autopilot.salePort = destination;
    DeleteAttribute(pchar, "Ship.Autopilot.saleFloor");
    if (origin == destination) return;
    int store = FindStore(Colonies[origin].id);
    for (int good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
    {
        string name = Goods[good].name;
        pchar.Ship.Autopilot.saleFloor.(name) = GetStoreGoodsPrice(&Stores[store], good, PRICE_TYPE_SELL, pchar, 1) + 1;
    }
}

void SeaAutopilot_Sell(int port)
{
    if (SeaAutopilot_Mode() != 1 || !CheckAttribute(pchar, "Ship.Autopilot.salePort") ||
        sti(pchar.Ship.Autopilot.salePort) != port) return;
    int store = FindStore(Colonies[port].id);
    if (store < 0 || store == SHIP_STORE) return;
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        ref captain = &Characters[index];
        if (!SeaAutopilot_Ship(captain)) continue;
        for (int good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
        {
            string name = Goods[good].name;
            if (!WdmTrafficStoreGood(&Stores[store], good)) continue;
            int qty = SeaAutopilot_SaleQuantity(captain, good);
            if (qty > 0)
                Store_TradePlayerShip(captain, &Stores[store], good, qty, false, 0);
        }
    }
    DeleteAttribute(pchar, "Ship.Autopilot.salePort");
    DeleteAttribute(pchar, "Ship.Autopilot.saleFloor");
}

void SeaAutopilot_Buy(int origin, int destination)
{
    if (!CheckAttribute(pchar, "Ship.Autopilot.plan.origin") ||
        sti(pchar.Ship.Autopilot.plan.origin) != origin || sti(pchar.Ship.Autopilot.plan.destination) != destination ||
        CheckAttribute(pchar, "Ship.Autopilot.plan.executed")) return;
    aref plan; makearef(plan, pchar.Ship.Autopilot.plan);
    int seller = FindStore(Colonies[origin].id);
    int before = sti(pchar.Money); int total = 0;
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        string hull = "ship" + slot;
        if (!CheckAttribute(plan, hull + ".index")) continue;
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0 || index != sti(plan.(hull).index)) continue;
        ref captain = &Characters[index];
        if (!SeaAutopilot_Ship(captain) || captain.id != plan.(hull).id || sti(captain.Ship.Type) != sti(plan.(hull).type)) continue;
        for (int good = GOOD_CHOCOLATE; good <= GOOD_SILVER; good++)
        {
            string name = Goods[good].name;
            if (!CheckAttribute(plan, hull + ".goods." + name) || !SeaAutopilot_TradeCargo(captain, good) ||
                !SeaAutopilot_Good(&Stores[seller], good)) continue;
            int quantity = sti(plan.(hull).goods.(name));
            int bought = Store_TradePlayerShip(captain, &Stores[seller], good, quantity, true, sti(plan.reserve));
            total += bought;
            if (bought != quantity) plan.changed = 1;
        }
    }
    plan.actualCost = before - sti(pchar.Money); plan.bought = total; plan.executed = 1;
}

// Replenishment is an ordinary finite purchase, not NPC service or a second
// food clock. No crew, hull or sail state is manufactured here.
void SeaAutopilot_Resupply(int port)
{
    int store = FindStore(Colonies[port].id);
    if (store < 0 || store == SHIP_STORE) return;
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        ref captain = &Characters[index];
        if (!SeaAutopilot_Ship(captain)) continue;
        int food = 30 * GetShipDailyFood(captain, GetCrewQuantity(captain));
        if (SeaAutopilot_Good(&Stores[store], GOOD_FOOD))
            Store_TradePlayerShip(captain, &Stores[store], GOOD_FOOD, food - GetCargoGoods(captain, GOOD_FOOD), true, SeaAutopilot_Reserve());
        int shot = Ship_GetCompanionAmmoMinimum(captain) * 6;
        if (shot < 1) continue;
        if (SeaAutopilot_Good(&Stores[store], GOOD_BALLS))
            Store_TradePlayerShip(captain, &Stores[store], GOOD_BALLS, shot - GetCargoGoods(captain, GOOD_BALLS), true, SeaAutopilot_Reserve());
        if (SeaAutopilot_Good(&Stores[store], GOOD_POWDER))
            Store_TradePlayerShip(captain, &Stores[store], GOOD_POWDER, shot - GetCargoGoods(captain, GOOD_POWDER), true, SeaAutopilot_Reserve());
    }
}

int SeaAutopilot_Enemy()
{
    int selected = -1; float nearest = 36000000.0;
    for (int ship = 0; ship < iNumShips; ship++)
    {
        int index = Ships[ship];
        if (index < 0 || index >= TOTAL_CHARACTERS || index == nMainCharacterIndex || index == iFortCommander) continue;
        ref enemy = &Characters[index];
        if (IsCompanion(enemy) || !IsEntity(enemy) || !WdmFleetSeaAlive(enemy) ||
            GetRelation(nMainCharacterIndex, index) != RELATION_ENEMY) continue;
        if (CheckAttribute(enemy, "SeaSurrender") || CheckAttribute(enemy, "Surrendered")) continue;
        float dx = stf(enemy.Ship.Pos.x) - stf(pchar.Ship.Pos.x);
        float dz = stf(enemy.Ship.Pos.z) - stf(pchar.Ship.Pos.z);
        if (dx * dx + dz * dz >= nearest) continue;
        nearest = dx * dx + dz * dz; selected = index;
    }
    return selected;
}

bool SeaAutopilot_HuntEligible(aref fleet)
{
    if (!WdmTrafficIsOrdinary(fleet) || CheckAttribute(fleet, "qID") ||
        CheckAttribute(fleet, "sinking") || !CheckAttribute(fleet, "x") || !CheckAttribute(fleet, "z")) return false;
    if (CheckAttribute(fleet, "trafficRetired") && sti(fleet.trafficRetired)) return false;
    if (CheckAttribute(fleet, "liveTime") && stf(fleet.liveTime) <= 0.0) return false;
    if (CheckAttribute(fleet, "deleteAlpha") && stf(fleet.deleteAlpha) < 1.0) return false;
    if (CheckAttribute(fleet, "trafficLifecycle") && fleet.trafficLifecycle == "service") return false;
    return !GreatRecipe_MapImportBlocked(GetAttributeName(fleet)) &&
        GetNationRelation2MainCharacter(sti(fleet.trafficNation)) == RELATION_ENEMY;
}

// Follow the existing fleet's identity and live position, never a synthetic
// encounter or a port standing in for an enemy. Admission keeps its normal queue.
bool SeaAutopilot_HuntMap()
{
    if (bSeaActive || SeaAutopilot_Mode() != 2) return false;
    int previous = -1;
    if (CheckAttribute(&worldMap, "encounter.cur")) previous = sti(worldMap.encounter.cur);
    string selected = "";
    int selectedIndex = -1;
    string current = "";
    if (CheckAttribute(pchar, "Ship.Autopilot.huntTarget")) current = pchar.Ship.Autopilot.huntTarget;
    float nearest = 1000000000000.0;
    int count = wdmGetNumberShipEncounters();
    for (int cursor = -1; cursor < count; cursor++)
    {
        int i = cursor;
        if (cursor < 0)
        {
            if (current == "" || !CheckAttribute(pchar, "Ship.Autopilot.huntIndex")) continue;
            i = sti(pchar.Ship.Autopilot.huntIndex);
            if (i < 0 || i >= count) continue;
        }
        if (CheckAttribute(pchar, "SkipEshipIndex") && sti(pchar.SkipEshipIndex) == i) continue;
        if (!wdmSetCurrentShipData(i)) continue;
        string id = worldMap.encounter.id;
        if (id == "" || !CheckAttribute(&worldMap, "encounters." + id)) continue;
        aref fleet; makearef(fleet, worldMap.encounters.(id));
        if (!SeaAutopilot_HuntEligible(fleet)) continue;
        float dx = stf(worldMap.encounter.x) - SeaAutopilot_X();
        float dz = stf(worldMap.encounter.z) - SeaAutopilot_Z();
        if (id != current && dx * dx + dz * dz >= nearest) continue;
        nearest = dx * dx + dz * dz; selected = id; selectedIndex = i;
        if (id == current) break;
    }
    if (selected == "")
    {
        if (current != "" && CheckAttribute(pchar, "Ship.Autopilot.port"))
            SeaAutopilot_Route(sti(pchar.Ship.Autopilot.port));
        DeleteAttribute(pchar, "Ship.Autopilot.huntTarget");
        if (previous >= 0) wdmSetCurrentShipData(previous);
        return false;
    }
    // An enemy coordinate cannot inherit a port's settlement authority.
    DeleteAttribute(pchar, "Ship.Autopilot.port");
    pchar.Ship.Autopilot.huntTarget = selected;
    pchar.Ship.Autopilot.huntIndex = selectedIndex;
    pchar.Ship.Autopilot.targetX = worldMap.encounters.(selected).x;
    pchar.Ship.Autopilot.targetZ = worldMap.encounters.(selected).z;
    pchar.Ship.Autopilot.active = 1;
    SeaAutopilot_Status("hunting", "");
    if (nearest <= 625.0 && wdmSetCurrentShipData(selectedIndex) && worldMap.encounter.id == selected)
    {
        pchar.eshipIndex = selectedIndex;
        SeaAutopilot_QueueEncounter(selectedIndex);
        return true;
    }
    if (previous >= 0) wdmSetCurrentShipData(previous);
    return true;
}

bool SeaAutopilot_Combat()
{
    if (!CheckAttribute(pchar, "SeaAI.Task") || !CheckAttribute(pchar, "SeaAI.Task.Target")) return false;
    int task = sti(pchar.SeaAI.Task);
    if (task != AITASK_ATTACK && task != AITASK_RUNAWAY) return false;
    int enemy = sti(pchar.SeaAI.Task.Target);
    return enemy >= 0 && enemy < TOTAL_CHARACTERS && WdmFleetSeaAlive(&Characters[enemy]) &&
        GetRelation(nMainCharacterIndex, enemy) == RELATION_ENEMY;
}

void SeaAutopilot_Course()
{
    if (!bSeaActive || !bSeaLoaded) return;
    float x = Sea_LocalX(stf(pchar.Ship.Autopilot.targetX), stf(pchar.Ship.Autopilot.targetZ));
    float z = Sea_LocalZ(stf(pchar.Ship.Autopilot.targetX), stf(pchar.Ship.Autopilot.targetZ));
    if (SeaAutopilot_OwnTask() && sti(pchar.Ship.Autopilot.order) == AITASK_MOVE &&
        abs(stf(pchar.Ship.Autopilot.orderX) - x) < 1.0 && abs(stf(pchar.Ship.Autopilot.orderZ) - z) < 1.0) return;
    pchar.Ship.Autopilot.order = AITASK_MOVE;
    pchar.Ship.Autopilot.orderX = x; pchar.Ship.Autopilot.orderZ = z;
    Ship_SetTaskMove(PRIMARY_TASK, nMainCharacterIndex, x, z);
}

void SeaAutopilot_Hold()
{
    if (bSeaActive && (!SeaAutopilot_OwnTask() || sti(pchar.Ship.Autopilot.order) != AITASK_DRIFT))
    {
        pchar.Ship.Autopilot.order = AITASK_DRIFT;
        Ship_SetTaskDrift(PRIMARY_TASK, nMainCharacterIndex);
    }
    pchar.Ship.Autopilot.active = 1;
}

void SeaAutopilot_Tick()
{
    if (SeaAutopilot_Mode() == 0) return;
    pchar.Ship.Autopilot.active = 0;
    if (!SeaAutopilot_Eligible()) { SeaAutopilot_Status("yielded", "runtime-or-orders"); return; }
    if (bSeaActive && SeaAutopilot_Combat())
    { pchar.Ship.Autopilot.active = 1; SeaAutopilot_Status("combat", ""); return; }
    if (bSeaActive && CheckAttribute(pchar, "SeaAI.Task") && !SeaAutopilot_OwnTask() &&
        sti(pchar.SeaAI.Task) != AITASK_NONE && sti(pchar.SeaAI.Task) != AITASK_DRIFT)
    { SeaAutopilot_Status("yielded", "player-task"); return; }
    if (bSeaActive && SeaAutopilot_Mode() == 2)
    {
        int enemy = SeaAutopilot_Enemy();
        if (enemy >= 0)
        {
            pchar.Ship.Autopilot.order = AITASK_ATTACK; pchar.Ship.Autopilot.enemy = enemy;
            Ship_SetTaskAttack(PRIMARY_TASK, nMainCharacterIndex, enemy);
            string enemyGroup = Ship_GetGroupID(&Characters[enemy]);
            if (enemyGroup != "") Group_SetTaskAttack(PLAYER_GROUP, enemyGroup);
            pchar.Ship.Autopilot.active = 1; SeaAutopilot_Status("hunting", ""); return;
        }
    }
    if (SeaAutopilot_HuntMap()) return;
    if (!CheckAttribute(pchar, "Ship.Autopilot.port"))
    {
        int nearest = SeaAutopilot_NearestPort();
        if (nearest < 0 || !SeaAutopilot_Route(nearest)) { SeaAutopilot_Status("waiting", "no-port"); return; }
    }
    if (!CheckAttribute(pchar, "Ship.Autopilot.targetX") || !CheckAttribute(pchar, "Ship.Autopilot.targetZ") ||
        !CheckAttribute(pchar, "Ship.Autopilot.leg") || !CheckAttribute(pchar, "Ship.Autopilot.settledLeg"))
    {
        DeleteAttribute(pchar, "Ship.Autopilot.port");
        SeaAutopilot_Status("waiting", "invalid-saved-route"); return;
    }
    int port = sti(pchar.Ship.Autopilot.port);
    if (!SeaAutopilot_Port(port))
    { DeleteAttribute(pchar, "Ship.Autopilot.port"); SeaAutopilot_Status("waiting", "port-access"); return; }
    float dx = SeaAutopilot_X() - stf(pchar.Ship.Autopilot.targetX);
    float dz = SeaAutopilot_Z() - stf(pchar.Ship.Autopilot.targetZ);
    if (dx * dx + dz * dz > 144.0)
    {
        pchar.Ship.Autopilot.active = 1; SeaAutopilot_Course(); SeaAutopilot_Status("sailing", ""); return;
    }
    if (sti(pchar.Ship.Autopilot.settledLeg) != sti(pchar.Ship.Autopilot.leg))
    {
        SeaAutopilot_Sell(port);
        SeaAutopilot_Resupply(port);
        pchar.Ship.Autopilot.settledLeg = pchar.Ship.Autopilot.leg;
    }
    SeaAutopilot_Hold();
    aref review; makearef(review, pchar.Ship.Autopilot.review);
    if (CheckAttribute(review, "year") && WdmTrafficElapsed(review, "hour") < 24)
    { SeaAutopilot_Status("waiting", "market"); return; }
    WdmTrafficStamp(review);
    int destination = -1;
    if (SeaAutopilot_Mode() == 1)
    {
        // Do not leave on a separate tiny sale-only leg while paid freight fits.
        // Existing best-local cargo may be sold here; remaining cargo joins the
        // same destination plan as new purchases.
        if (SeaAutopilot_SalePort(port) < 0)
        {
            SeaAutopilot_PlanSale(port, port);
            SeaAutopilot_Sell(port);
            SeaAutopilot_Resupply(port);
        }
        destination = SeaAutopilot_NextPort(port);
    }
    else
    {
        int total = 0;
        int otherTotal = 0;
        int lastPort = -1;
        if (CheckAttribute(pchar, "Ship.Autopilot.lastPort")) lastPort = sti(pchar.Ship.Autopilot.lastPort);
        if (lastPort == port) lastPort = -1;
        for (int candidate = 0; candidate < MAX_COLONIES; candidate++)
        {
            if (candidate == port || !SeaAutopilot_Port(candidate)) continue;
            int weight = makeint(WdmTrafficRouteWeight(port, candidate, 3));
            total += weight;
            if (candidate != lastPort) otherTotal += weight;
            if (weight > 0 && rand(total - 1) < weight) destination = candidate;
        }
        if (lastPort >= 0 && otherTotal > 0 && destination == lastPort)
        {
            int roll = rand(otherTotal - 1);
            for (candidate = 0; candidate < MAX_COLONIES; candidate++)
            {
                if (candidate == port || candidate == lastPort || !SeaAutopilot_Port(candidate)) continue;
                int w = makeint(WdmTrafficRouteWeight(port, candidate, 3));
                roll -= w;
                if (roll < 0) { destination = candidate; break; }
            }
        }
    }
    if (destination < 0)
    {
        string reason = "market-or-cash";
        if (SeaAutopilot_Mode() == 1 && SeaAutopilot_Free(GOOD_SUGAR) == 0) reason = "cargo-reserved-or-unsaleable";
        if (SeaAutopilot_Mode() == 1 && CheckAttribute(pchar, "Ship.Autopilot.plan.blocked")) reason = pchar.Ship.Autopilot.plan.blocked;
        SeaAutopilot_Status("waiting", reason); return;
    }
    object endpoint;
    if (!SeaAutopilot_Endpoint(destination, &endpoint)) { SeaAutopilot_Status("waiting", "port-locator"); return; }
    if (SeaAutopilot_Mode() == 1)
    {
        SeaAutopilot_Buy(port, destination);
        SeaAutopilot_PlanSale(port, destination);
    }
    pchar.Ship.Autopilot.lastPort = port;
    if (!SeaAutopilot_Route(destination)) return;
    pchar.Ship.Autopilot.active = 1; SeaAutopilot_Course(); SeaAutopilot_Status("sailing", "");
}
