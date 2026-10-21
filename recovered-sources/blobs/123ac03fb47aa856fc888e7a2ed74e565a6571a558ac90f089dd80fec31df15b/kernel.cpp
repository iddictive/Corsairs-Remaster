#define main exploratory_main
#include "topology.cpp"
#undef main
size_t count(const Mesh&m){size_t n=0;for(auto&o:m)n+=o.second.size();return n;}
bool close(double a,double b){return std::abs(a-b)<.00005;}
std::vector<Surface>carve(Surface&surface,unsigned doses,bool extract){auto fields=ring();std::vector<Surface>chunks;
 for(unsigned first=0;first<doses;first+=8){Plane p{{0,0,-1},0};for(unsigned n=first;n<std::min(first+8,doses);++n)p.beams.push_back(fields.beams[n]);auto before=surface;auto split=splitSurface(surface,p,{});assert(split.complete);assert(closedCutVolume(canonicalMesh(split.kept))>0);if(extract){auto separated=detachEnclosed(before,split.kept);for(auto&chunk:separated)chunks.push_back(std::move(chunk));}surface=std::move(split.kept);}
 return chunks;
}
#ifndef KERNEL_HELPERS_ONLY
int main(){std::cout.precision(12);auto started=std::chrono::steady_clock::now();Surface solid{box(),{},{}},solidNotch=solid;auto none=carve(solidNotch,18,true);assert(none.empty());assert(closedCutVolume(canonicalMesh(solidNotch))>0);
 auto chunks=carve(solid,24,true);assert(chunks.size()==1);const double anchored=closedCutVolume(canonicalMesh(solid)),chunk=closedCutVolume(canonicalMesh(chunks[0]));assert(close(anchored,4.933197811537871)&&close(chunk,1.1572045060692417));std::cout<<"solid_ring anchored="<<anchored<<" chunk="<<chunk<<" total="<<anchored+chunk<<" triangles="<<count(canonicalMesh(solid))<<" partial_notch_attached PASS\n";
 Mesh shell=box();appendMesh(shell,box(.8f,true));Surface hollow{shell,{},{}},hollowNotch=hollow;none=carve(hollowNotch,18,true);assert(none.empty());auto hollowChunks=carve(hollow,24,true);assert(hollowChunks.size()==2);double separated=0;for(auto&piece:hollowChunks){double v=closedCutVolume(canonicalMesh(piece));assert(close(v,.11572045060692418));separated+=v;}double remaining=closedCutVolume(canonicalMesh(hollow));assert(close(remaining,3.290639562307574));assert(close(remaining+separated,3.5220804635214225));std::cout<<"hollow_ring anchored="<<remaining<<" chunks="<<separated<<" cavity_preserved partial_notch_attached PASS\n";
 BeamSample b;b.start={5,5,5};b.direction={0,0,-1};b.depth=.5;Plane miss{{0,0,-1},0};miss.beams={b};auto missed=splitSurface(solid,miss,{});assert(missed.complete);assert(close(closedCutVolume(canonicalMesh(missed.kept)),anchored));assert(detachEnclosed(solid,missed.kept).empty());
 b.start={.85f,.85f,1.01f};b.depth=.4f;miss.beams={b};auto further=splitSurface(solid,miss,{});assert(further.complete);assert(closedCutVolume(canonicalMesh(further.kept))>0);assert(closedCutVolume(canonicalMesh(further.kept))<anchored);std::cout<<"released prior state -> next cut / no-contact PASS\n";
 auto legacy=splitSurface({box(),{},{}},{{0,0,1},.8f,true},{});assert(legacy.complete);b.start={0,0,.81f};b.depth=.4f;miss.beams={b};auto afterLegacy=splitSurface(legacy.kept,miss,{});assert(afterLegacy.complete);assert(closedCutVolume(canonicalMesh(afterLegacy.kept))>0);std::cout<<"legacy plane -> finite material PASS\n";
 std::cout<<"elapsed_ms="<<std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count()<<std::endl;
}

#endif
