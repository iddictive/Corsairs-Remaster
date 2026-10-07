// Sea confirmation window; ordinary leave-battle behavior is preserved.
string totalInfo = "";
bool mastRepairMode = false;
bool mastRepairConfirmed = false;

void InitInterface(string iniName)
{
    mastRepairMode = CheckAttribute(&BI_MastRepairQuote, "captain");
    StartAboveForm(true);
    SendMessage(&GameInterface, "ls", MSG_INTERFACE_INIT, iniName);
    if (mastRepairMode)
    {
        SetFormatedText("INFO_TEXT", BI_MastRepairText());
        Button_SetText("B_OK", "#Ремонт");
        Button_SetText("B_CANCEL", "#Отмена");
    }
    else
    {
        CalculateInfoData();
        SetFormatedText("INFO_TEXT", totalInfo + "\n\n" + XI_ConvertString("MapWhatYouWantToDo"));
    }
    SetEventHandler("InterfaceBreak", "ProcessBreakExit", 0);
    SetEventHandler("exitCancel", "ProcessCancelExit", 0);
    SetEventHandler("ievnt_command", "ProcCommand", 0);
    SetEventHandler("evntDoPostExit", "DoPostExit", 0);
    EI_CreateFrame("INFO_BORDERS", 245,154,555,330);
    PlaySound("Encounter_Map_1");
}

void ProcessBreakExit()
{
    IDoExit(RC_INTERFACE_ANY_EXIT);
}

void ProcessCancelExit()
{
    IDoExit(RC_INTERFACE_ANY_EXIT);
}

void IDoExit(int exitCode)
{
    if (mastRepairMode && !mastRepairConfirmed) BI_MastRepairCancel();
    DelEventHandler("InterfaceBreak", "ProcessBreakExit");
    DelEventHandler("exitCancel", "ProcessCancelExit");
    DelEventHandler("ievnt_command", "ProcCommand");
    DelEventHandler("evntDoPostExit", "DoPostExit");
    EndAboveForm(true);
    interfaceResultCommand = exitCode;
    EndCancelInterface(true);
}

void ProcCommand()
{
    string comName = GetEventData();
    string nodName = GetEventData();
    switch(nodName)
    {
    case "B_OK":
        if(comName == "activate" || comName == "click")
        {
            if (mastRepairMode)
            {
                mastRepairConfirmed = true;
                BI_MastRepairAccept();
            }
            else
            {
                KillCompanions();
                ChangeShowIntarface();
            }
            IDoExit(RC_INTERFACE_ANY_EXIT);
        }
        if(comName == "downstep")
        {
            if(GetSelectable("B_CANCEL")) SetCurrentNode("B_CANCEL");
        }
        break;
    case "B_CANCEL":
        if(comName == "activate" || comName == "click") IDoExit(RC_INTERFACE_ANY_EXIT);
        if(comName == "upstep")
        {
            if(GetSelectable("B_OK")) SetCurrentNode("B_OK");
        }
        break;
    }
}

void DoPostExit()
{
    int exitCode = GetEventData();
    IDoExit(exitCode);
}

void CalculateInfoData()
{
    aref rootItems;
    string sEnd;
    int cn, i;
    ref chr;
    makearef(rootItems, pchar.CheckEnemyCompanionDistance);
    if (GetAttributesNum(rootItems) > 1)
    {
        totalInfo = "Наши корабли ";
        sEnd = " находятся в контакте с противником.";
    }
    else
    {
        totalInfo = "Наш корабль ";
        sEnd = " находится в контакте с противником.";
    }
    cn = sti(GetAttributeValue(GetAttributeN(rootItems, 0)));
    if (cn != -1)
    {
        chr = GetCharacter(cn);
        totalInfo += XI_ConvertString(RealShips[sti(chr.Ship.Type)].BaseName) + " '" + chr.Ship.Name + "'";
    }
    for (i = 1; i < GetAttributesNum(rootItems); i++)
    {
        cn = sti(GetAttributeValue(GetAttributeN(rootItems, i)));
        if (cn != -1)
        {
            chr = GetCharacter(cn);
            totalInfo += ", " + XI_ConvertString(RealShips[sti(chr.Ship.Type)].BaseName) + " '" + chr.Ship.Name + "'";
        }
    }
    totalInfo += sEnd;
}

void KillCompanions()
{
    aref rootItems;
    int cn, i;
    ref chr;
    makearef(rootItems, pchar.CheckEnemyCompanionDistance);
    for (i = 0; i < GetAttributesNum(rootItems); i++)
    {
        cn = sti(GetAttributeValue(GetAttributeN(rootItems, i)));
        if (cn != -1)
        {
            chr = GetCharacter(cn);
            RemoveCharacterCompanion(PChar, chr);
            if (!CheckAttribute(chr, "PGGAi")) chr.LifeDay = 0;
            else
            {
                chr.PGGAi.IsPGG = true;
                chr.RebirthPhantom = true;
                chr.PGGAi.location.town = PGG_FindRandomTownByNation(sti(chr.nation));
                PGG_ChangeRelation2MainCharacter(chr, -40);
            }
            chr.location = "";
            chr.location.group = "";
            chr.location.locator = "";
        }
    }
    PChar.GenQuest.CallFunctionParam = pchar.CheckEnemyCompanionType;
    DoQuestCheckDelay("CallFunctionParam", 1.6);
}
