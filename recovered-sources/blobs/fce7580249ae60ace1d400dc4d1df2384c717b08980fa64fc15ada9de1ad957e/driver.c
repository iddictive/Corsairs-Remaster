#include "cases.c"
int RunnerResult;
void Main(){RunnerResult=LegacyIgnoredSaveResult();}
void RunnerAfterLoad(){RunnerResult=CheckMaterialSave();}
