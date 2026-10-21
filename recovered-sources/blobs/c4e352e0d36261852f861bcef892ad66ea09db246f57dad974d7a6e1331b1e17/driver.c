#include "cases.c"
int RunnerResult;
void Main(){RunnerResult=SeedMaterialSave();}
void RunnerAfterLoad(){RunnerResult=CheckMaterialSave();}
