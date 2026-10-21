#define main archived_main
#include "/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/world-cut-surface/reopen-production-radius220/coverage-repair/coverage-and-roles.cpp"
#undef main
int main(){
 auto original=box();storm_ship_laser::Plane p{{0,0,1},0};storm_hull_damage::BeamSample b;b.start={0,0,-1};b.direction={0,0,1};b.depth=.3f;p.beams={b};
 auto split=storm_ship_laser::splitSurface({original,{},{}},p,{});assert(split.complete);auto kept=storm_ship_laser::canonicalMesh(split.kept),detached=storm_ship_laser::canonicalMesh(split.detached);
 std::cout<<"ship box volumes "<<volume(original)<<" "<<volume(kept)<<" "<<volume(detached)<<" collision "<<ray(kept,{0,0,-2},{0,0,2})<<std::endl;
 assert(std::abs(volume(original)-volume(kept)-volume(detached))<1e-5);assert(std::abs(ray(kept,{0,0,-2},{0,0,2})-.325)<1e-6);assert(ray(kept,{.8f,0,-2},{.8f,0,2})==.25);
 const auto skinArea=[](const Mesh&m){double area=0;for(const auto&o:m)for(const auto&t:o.second)area+=storm_ship_hull::area(t);return area;};
 const double aperture=skinArea(split.detached.outer);assert(std::abs(aperture-8*.16*.16*std::tan(M_PI/8))<1e-6);
 auto deeper=p;deeper.beams[0].start[2]=-.7f;auto deep=storm_ship_laser::splitSurface(split.kept,deeper,{});assert(deep.complete);assert(skinArea(deep.detached.outer)<1e-8);
 auto adjacent=p;adjacent.beams[0].start[0]=.1f;auto overlap=storm_ship_laser::splitSurface(split.kept,adjacent,{});assert(overlap.complete);
 auto unionField=p;unionField.beams.push_back(adjacent.beams[0]);auto unionSplit=storm_ship_laser::splitSurface({original,{},{}},unionField,{});
 assert(std::abs(aperture+skinArea(overlap.detached.outer)-skinArea(unionSplit.detached.outer))<1e-6);
 std::cout<<"finite aperture="<<aperture<<" repeated depth adds zero / overlapping entrances charge union once PASS"<<std::endl;
 assert(storm_ship_laser::beamIntersectsBox(b,{0,0,-.9f},{.3f,.65f,.3f}));assert(!storm_ship_laser::beamIntersectsBox(b,{5,0,-.9f},{.3f,.65f,.3f}));assert(!storm_ship_laser::beamIntersectsBox(b,{0,0,4},{.3f,.65f,.3f}));
 Cut world;world.kind=Cut::Beam;world.beams={b};Mesh output;assert(reconstructWorld(original,{}, {world},output));assert(std::abs(ray(output,{0,0,-2},{0,0,2})-.325)<1e-6);assert(std::abs(volume(original)-volume(output)-volume(detached))<1e-5);
 // Two deliberately separated samples must not cut the material between them.
 p.beams[0].start={-.65f,0,-1};b.start={.65f,0,-1};p.beams.push_back(b);split=storm_ship_laser::splitSurface({original,{},{}},p,{});kept=storm_ship_laser::canonicalMesh(split.kept);assert(ray(kept,{0,0,-2},{0,0,2})==.25);assert(std::abs(ray(kept,{.65f,0,-2},{.65f,0,2})-.325)<1e-6);
 world.beams=p.beams;assert(reconstructWorld(original,{}, {world},output));assert(ray(output,{0,0,-2},{0,0,2})==.25);
 auto prior=output;world.beams[0].depth=NAN;assert(!reconstructWorld(original,{}, {world},output));assert(sameMesh(prior,output));
 // Old halfspaces and their ordering continue to compose with new finite cuts.
 Cut old;old.normal={1,0,0};old.distance=.9;world.beams=p.beams;assert(reconstructWorld(original,{}, {old,world},output));assert(ray(output,{0,0,-2},{0,0,2})==.25);
 std::cout<<"local cavity / untouched center / legacy / invalid rollback / bounded crew PASS"<<std::endl;
 const std::string root="/REQUIRED_EXTERNAL_INPUT/Corsairs/.local-archives/metal-renderer-evidence/world-cut-surface/reopen-production-radius220/";
 for(const auto&name:{"Antigua-Antigua.bin","Cumana-Cumana.bin"}){auto source=load((root+name).c_str());const auto &t=source.begin()->second[source.begin()->second.size()/2];auto point=(t[0].position+t[1].position+t[2].position)*(1.f/3);auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));auto start=point+normal*.001f;
 b.start={start.x,start.y,start.z};b.direction={-normal.x,-normal.y,-normal.z};b.depth=.3f;world.beams={b};world.field=1;Mesh outer,caps;std::vector<CapPatch>patches;std::set<int>terrain;for(const auto&o:source)terrain.insert(o.first);assert(reconstructWorld(source,terrain,{world},output,&outer,&caps,&patches));
 const auto encode=[](int material,const Triangle&t){std::string key(sizeof(material)+sizeof(t),'\0');std::memcpy(key.data(),&material,sizeof(material));std::memcpy(key.data()+sizeof(material),t.data(),sizeof(t));return key;};std::unordered_set<std::string>retained;for(const auto&o:outer)for(const auto&t:o.second)retained.insert(encode(o.first,t));size_t exact=0;
 for(const auto&o:source)for(const auto&t:o.second){bool remote=false;for(const auto&p:spherePlanes(world)){bool outside=true;for(const auto&v:t)outside&=storm_ship_hull::dot(p.normal,v.position)>p.distance;remote|=outside;}if(remote){assert(retained.count(encode(o.first,t)));++exact;}}
 const auto from=point+normal*2,to=point-normal*2;double before=ray(source,from,to),after=ray(output,from,to);std::cout<<name<<" tris="<<triangleCount(source)<<" unchanged="<<exact<<" caps="<<triangleCount(caps)<<" hit="<<before<<" -> "<<after<<std::endl;assert(after>before+.02&&after<before+.1);assert(exact>triangleCount(source)*.99);
 }
}
