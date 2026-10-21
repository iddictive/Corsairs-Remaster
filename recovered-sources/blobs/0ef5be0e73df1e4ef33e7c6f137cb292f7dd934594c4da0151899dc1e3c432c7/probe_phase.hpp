#include <chrono>
#include <cstdio>
struct ProbePhase {const char*name;std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();~ProbePhase(){std::fprintf(stderr,"PHASE %s %.3f ms\n",name,std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count());}};
