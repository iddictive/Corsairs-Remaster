#include "cases.c"
int RunnerResult;
void Main(){RunnerResult=BindPaletteFromOldState();}
void RunnerAfterLoad(){RunnerResult=CheckPalette();}
