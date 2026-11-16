int PHONK_WARHEAD_THERMOBARIC=6;
int PHONK_WARHEAD_TANDEM=7;
int PhonkWarhead_Id(int row)
{
	if (row == 0) return 0;
	if (row == 1) return 1;
	if (row == 2) return 2;
	if (row == 3) return 4;
	if (row == 4) return PHONK_WARHEAD_THERMOBARIC;
	if (row == 5) return PHONK_WARHEAD_TANDEM;
	if (row == 6) return 5;
	return -1;
}

string PhonkWarhead_Name(int id)
{
	if (id == 0) return "Ядерная";
	if (id == 1 || id == 3) return "Фугасная";
	if (id == 2) return "Кассетная";
	if (id == 4) return "Зажигательная";
	if (id == PHONK_WARHEAD_THERMOBARIC) return "Термобарическая";
	if (id == PHONK_WARHEAD_TANDEM) return "Тандемная";
	if (id == 5) return "Случайная";
	return "";
}

int TestWarheadMappings() {
 if (PhonkWarhead_Id(0)!=0) return 1;
 if (PhonkWarhead_Id(1)!=1) return 2;
 if (PhonkWarhead_Id(2)!=2) return 3;
 if (PhonkWarhead_Id(3)!=4) return 4;
 if (PhonkWarhead_Id(4)!=6) return 5;
 if (PhonkWarhead_Id(5)!=7) return 6;
 if (PhonkWarhead_Id(6)!=5) return 7;
 if (PhonkWarhead_Id(7)!=-1) return 8;
 if (PhonkWarhead_Id(-1)!=-1) return 9;
 if (PhonkWarhead_Name(7)!="Тандемная") return 10;
 if (PhonkWarhead_Name(3)!=PhonkWarhead_Name(1)) return 11;
 if (PhonkWarhead_Name(8)!="") return 12;
 return 0;
}
