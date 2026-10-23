//  файл для кораблей в порту, море и сторожевиков
void GenerateIslandShips(string sIslandID)
{
	//if (!TestRansackCaptain) return; // to_do
	
	int iColonyQuantity = sti(Islands[FindIsland(sIslandID)].colonyquantity);
	int iNation;
	int iShipsQuantity;
	int iChar;

	for (int i = 0; i< MAX_COLONIES; i++)
	{
		if (Colonies[i].island == sIslandID)
		{
			if (colonies[i].nation != "none" && !CheckAttribute(&colonies[i], "HasNoFort") && !CheckAttribute(&colonies[i], "DontSetShipInPort"))
			{
				if (!SeaTraffic_HarbourPresent(Colonies[i].id) &&
                    (!CheckAttribute(&colonies[i], "GenShipDate") || GetNpcQuestPastDayParam(&colonies[i], "GenShipDate") > 0))
				{
                    SaveCurrentNpcQuestDateParam(&colonies[i], "GenShipDate"); // дата заполнения
                    
					iNation = sti(Colonies[i].nation);
					float fChecker = frand(1.0);
					if (fChecker < 0.8)
					{
						iShipsQuantity = makeint(fChecker * 4.0);
						while (iShipsQuantity > 0)
						{
							iChar = GenerateCharacter(iNation, WITH_SHIP, "pofficer", MAN, -1, WARRIOR); //-1 - это 1 день; pofficer как заглушка
							PlaceCharacterShip(iChar, iNation, sIslandID, i);
							
                            characters[iChar].IslandShips = Colonies[i].id; // номер города, чтоб тереть по захвату города to_do
							if (iNation == PIRATE && GetNationRelation2Character(iNation, nMainCharacterIndex) == RELATION_FRIEND) //to_do: здесь особый стек капитанов
							{ // наш город
								characters[iChar].AlwaysFriend        = true;
								SetCharacterRelationBoth(iChar, GetMainCharacterIndex(), RELATION_FRIEND);
							}
							if(rand(1))	characters[iChar].Ship.Mode = "war";
							else		characters[iChar].Ship.Mode = "trade";
							if (rand(4) == 1 || GetCharacterShipClass(&characters[iChar]) == 1) SetRandGeraldSail(&characters[iChar], sti(characters[iChar].Nation));
							characters[iChar].AlwaysSandbankManeuver = true;  // тупым запрет тонуть об берег
							characters[iChar].AnalizeShips = true; //анализить вражеские корабли
							characters[iChar].location.from_sea = colonies[i].from_sea;
							SetCaptanModelByEncType(&characters[iChar], characters[iChar].Ship.Mode); // boal
							Fantom_SetCannons(&characters[iChar], characters[iChar].Ship.Mode);
							Fantom_SetBalls(&characters[iChar], characters[iChar].Ship.Mode);
							Fantom_SetGoods(&characters[iChar], characters[iChar].Ship.Mode);
							iShipsQuantity = iShipsQuantity - 1;
						}
					}
				}
			}
		}
	}
}

void PlaceCharacterShip(int iChar, int iNation, string sIslandID, int iColonyIdx)
{
	int iColonyNum = sti(colonies[iColonyIdx].num);
	
	string sColonyID = colonies[iColonyIdx].id;
	
	string sGroup = "IslandGroup" + iChar;
	
	Group_AddCharacter(sGroup, characters[iChar].id);
	Group_SetGroupCommander(sGroup, characters[iChar].id);	
	string sLocatorGroup = "IslandShips" + iColonyNum;
	string sLocator = "Ship_"+(rand(5)+1);
	Group_SetAddress(sGroup, sIslandID, sLocatorGroup, sLocator);

	//trace("sLocatorGroup is " + sLocatorGroup + " sLocator is " + sLocator);

	Ship_SetTaskDrift(SECONDARY_TASK, iChar);
	Group_SetTaskNone(sGroup);

	if (GetNationRelation2Character(iNation, nMainCharacterIndex) == RELATION_ENEMY)
	{
		Group_SetTaskAttack(sGroup, PLAYER_GROUP);
	}
}
// очистить порт
void ClearIslandShips(string _city)
{
	if (!bSeaActive)
	{
		SeaTraffic_ClearHarbour(_city);
		for (int i=GlobalCharacters; i<MAX_CHARACTERS; i++)
		{
			// отдельный код зачистки boal -->
			if (CheckAttribute(&characters[i], "IslandShips") && characters[i].IslandShips == _city)
			{
				InitCharacter(&characters[i], i);
			}
		}
		// потрем пустые группы
		Group_FreeAllDead();
	}
}
