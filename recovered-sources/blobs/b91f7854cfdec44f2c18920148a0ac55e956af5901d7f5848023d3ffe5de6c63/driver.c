#include "cases.c"
int RunnerResult;
void Main(){RunnerResult=SeedWorldSave();}
void RunnerAfterLoad(){RunnerResult=CheckWorldSave();}
