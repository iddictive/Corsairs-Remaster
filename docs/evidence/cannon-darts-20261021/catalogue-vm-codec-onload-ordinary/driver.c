#include "cases.c"
int RunnerResult;
void Main(){RunnerResult=SeedLegacy();}
void RunnerAfterLoad(){RunnerResult=UpgradeAndCheck();}
