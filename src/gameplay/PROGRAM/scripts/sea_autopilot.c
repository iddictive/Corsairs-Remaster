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
        DeleteAttribute(pchar, "Ship.Autopilot.review");
    }
    SeaAutopilot_Tick();
}

// Native restoration owns actor state; bind only the control projection.
void SeaAutopilot_Bind()
{
    SeaAutopilot_CancelEncounter();
    if (CheckAttribute(pchar, "Ship.Autopilot")) pchar.Ship.Autopilot.active = 0;
    if (SeaAutopilot_Mode() == 0) return;
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
    if (bSeaActive && CheckAttribute(pchar, "Ship.Stopped") && sti(pchar.Ship.Stopped)) return false;
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
        // The existing traffic fill fraction leaves headroom for paid supplies.
        int weight = makeint(GetCargoMaxSpace(captain) * WDM_TRAFFIC_FREIGHT_FILL) - GetCargoLoad(captain);
        if (weight > 0) room += GetGoodQuantityByWeight(good, weight);
    }
    return room;
}

int SeaAutopilot_NextPort(int origin)
{
    int seller = FindStore(Colonies[origin].id);
    object market; WdmTrafficBuildMarket(&market);
    int selected = -1; int total = 0;
    for (int port = 0; port < MAX_COLONIES; port++)
    {
        if (port == origin || !SeaAutopilot_Port(port)) continue;
        int buyer = FindStore(Colonies[port].id);
        float profit = 0.0;
        for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
        {
            if (!SeaAutopilot_Good(&Stores[seller], good) || !WdmTrafficStoreGood(&Stores[buyer], good)) continue;
            int buy = GetStoreGoodsPrice(&Stores[seller], good, PRICE_TYPE_BUY, pchar, 1);
            int sell = GetStoreGoodsPrice(&Stores[buyer], good, PRICE_TYPE_SELL, pchar, 1);
            if (buy <= 0 || sell <= buy) continue;
            int quantity = WdmTrafficTradeSupply(&Stores[seller], good, WdmTrafficMarketDemand(&market, buyer, good));
            int room = SeaAutopilot_Free(good);
            if (quantity > room) quantity = room;
            int budget = sti(pchar.Money) - SeaAutopilot_Reserve();
            if (buy * 1.0 * quantity / sti(Goods[good].Units) > budget)
                quantity = makeint(budget * 1.0 * sti(Goods[good].Units) / buy);
            if (quantity > 0) profit += (sell - buy) * 1.0 * quantity / sti(Goods[good].Units);
        }
        if (profit <= 0.0) continue;
        if (profit > 100000.0) profit = 100000.0;
        int weight = makeint(WdmTrafficRouteWeight(origin, port, 1) * (1.0 + profit / 100.0));
        if (weight < 1) continue;
        total += weight;
        if (rand(total - 1) < weight) selected = port;
    }
    return selected;
}

// Only our purchased quantities are eligible for automatic sale. Hull identity
// and current aboard quantity are rechecked; losses never become profits.
void SeaAutopilot_Sell(int port)
{
    int store = FindStore(Colonies[port].id);
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        ref captain = &Characters[index];
        if (!SeaAutopilot_Ship(captain)) continue;
        string key = "ship" + index;
        if (!CheckAttribute(pchar, "Ship.Autopilot.freight." + key + ".id")) continue;
        aref receipt; makearef(receipt, pchar.Ship.Autopilot.freight.(key));
        if (receipt.id != captain.id || sti(receipt.type) != sti(captain.Ship.Type))
        { DeleteAttribute(pchar, "Ship.Autopilot.freight." + key); continue; }
        for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
        {
            string name = Goods[good].name;
            if (!CheckAttribute(receipt, "goods." + name) || !WdmTrafficStoreGood(&Stores[store], good)) continue;
            int quantity = sti(receipt.goods.(name));
            int held = GetCargoGoods(captain, good);
            if (quantity > held) quantity = held;
            int sold = Store_TradePlayerShip(captain, &Stores[store], good, quantity, false, 0);
            receipt.goods.(name) = quantity - sold;
        }
    }
}

void SeaAutopilot_Buy(int origin, int destination)
{
    int seller = FindStore(Colonies[origin].id);
    int buyer = FindStore(Colonies[destination].id);
    object market; WdmTrafficBuildMarket(&market);
    for (int good = GOOD_FOOD; good <= GOOD_SILVER; good++)
    {
        if (!SeaAutopilot_Good(&Stores[seller], good) || !WdmTrafficStoreGood(&Stores[buyer], good)) continue;
        if (GetStoreGoodsPrice(&Stores[buyer], good, PRICE_TYPE_SELL, pchar, 1) <=
            GetStoreGoodsPrice(&Stores[seller], good, PRICE_TYPE_BUY, pchar, 1)) continue;
        int remaining = WdmTrafficTradeSupply(&Stores[seller], good, WdmTrafficMarketDemand(&market, buyer, good));
        for (int slot = 0; slot < COMPANION_MAX; slot++)
        {
            if (remaining < 1) break;
            int index = GetCompanionIndex(pchar, slot);
            if (index < 0) continue;
            ref captain = &Characters[index];
            if (!SeaAutopilot_Ship(captain)) continue;
            int weight = makeint(GetCargoMaxSpace(captain) * WDM_TRAFFIC_FREIGHT_FILL) - GetCargoLoad(captain);
            int quantity = 0;
            if (weight > 0) quantity = GetGoodQuantityByWeight(good, weight);
            if (quantity > remaining) quantity = remaining;
            int bought = Store_TradePlayerShip(captain, &Stores[seller], good, quantity, true, SeaAutopilot_Reserve());
            if (bought < 1) continue;
            string key = "ship" + index; string name = Goods[good].name;
            aref receipt; makearef(receipt, pchar.Ship.Autopilot.freight.(key));
            if (!CheckAttribute(receipt, "id") || receipt.id != captain.id || sti(receipt.type) != sti(captain.Ship.Type))
                DeleteAttribute(receipt, "");
            receipt.id = captain.id; receipt.type = captain.Ship.Type;
            int previous = 0;
            if (CheckAttribute(receipt, "goods." + name)) previous = sti(receipt.goods.(name));
            receipt.goods.(name) = previous + bought;
            remaining -= bought;
        }
    }
}

// Replenishment is an ordinary finite purchase, not NPC service or a second
// food clock. No crew, hull or sail state is manufactured here.
void SeaAutopilot_Resupply(int port)
{
    int store = FindStore(Colonies[port].id);
    for (int slot = 0; slot < COMPANION_MAX; slot++)
    {
        int index = GetCompanionIndex(pchar, slot);
        if (index < 0) continue;
        ref captain = &Characters[index];
        if (!SeaAutopilot_Ship(captain)) continue;
        int food = 14 * GetShipDailyFood(captain, GetCrewQuantity(captain));
        if (SeaAutopilot_Good(&Stores[store], GOOD_FOOD))
            Store_TradePlayerShip(captain, &Stores[store], GOOD_FOOD, food - GetCargoGoods(captain, GOOD_FOOD), true, SeaAutopilot_Reserve());
        int shot = Ship_GetCompanionAmmoMinimum(captain) * 4;
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
            pchar.Ship.Autopilot.active = 1; SeaAutopilot_Status("hunting", ""); return;
        }
    }
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
    if (SeaAutopilot_Mode() == 1) destination = SeaAutopilot_NextPort(port);
    else
    {
        int total = 0;
        for (int candidate = 0; candidate < MAX_COLONIES; candidate++)
        {
            if (candidate == port || !SeaAutopilot_Port(candidate)) continue;
            int weight = makeint(WdmTrafficRouteWeight(port, candidate, 3));
            total += weight;
            if (weight > 0 && rand(total - 1) < weight) destination = candidate;
        }
    }
    if (destination < 0) { SeaAutopilot_Status("waiting", "market-or-cash"); return; }
    object endpoint;
    if (!SeaAutopilot_Endpoint(destination, &endpoint)) { SeaAutopilot_Status("waiting", "port-locator"); return; }
    if (SeaAutopilot_Mode() == 1) SeaAutopilot_Buy(port, destination);
    if (!SeaAutopilot_Route(destination)) return;
    pchar.Ship.Autopilot.active = 1; SeaAutopilot_Course(); SeaAutopilot_Status("sailing", "");
}
