object Saved;
int Seed(){Saved.value=42;return 0;}
int Restore(){if(sti(Saved.value)!=42)return 1;return 0;}
