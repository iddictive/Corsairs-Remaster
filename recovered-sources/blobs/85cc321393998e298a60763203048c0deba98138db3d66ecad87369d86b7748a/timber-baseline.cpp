#define main secondary_main
#include "probe.cpp"
#undef main
double signedVolume(const Mesh&m){double v=0;for(const auto&o:m)for(const auto&t:o.second)v+=double(storm_ship_hull::dot(t[0].position,storm_ship_hull::cross(t[1].position,t[2].position)))/6;return v;}
void report(const char*name,const Mesh&m){std::printf("%s closed=%.12f signed=%.12f faces=%zu\n",name,closedCutVolume(m),signedVolume(m),m.at(0).size());}
int main(){auto wood=timberSurface(box(),{{0,.1f}});auto first=bore();auto second=bore(.05f);auto cut=splitSurface(wood,first,{{0,.1f}});assert(cut.complete);const auto physical=renderMesh(cut.detached);report("born",physical);
 auto standalone=standaloneFragment(physical);auto recut=splitSurface(*standalone.surface,second,{});assert(recut.complete);auto kept=renderMesh(recut.kept),removed=renderMesh(recut.detached);report("standalone-kept",kept);report("standalone-removed",removed);
 auto primary=splitSurface(wood,second,{{0,.1f}});assert(primary.complete);report("primary-kept",renderMesh(primary.kept));report("primary-removed",renderMesh(primary.detached));
 // Apply the existing cutter-plane normalization to the complete standalone
 // physical boundary, so coincident inherited outer walls participate too.
 assert(compactFiniteWalls(kept,second.beams)&&compactFiniteWalls(removed,second.beams));report("normalized-kept",kept);report("normalized-removed",removed);
 std::fflush(stdout);
}
