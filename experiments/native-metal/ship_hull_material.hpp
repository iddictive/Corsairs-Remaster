#pragma once
#include "ship_laser_geometry.hpp"
#include "ship_hull_material_repair.hpp"
#include "ship_hull_structure.hpp"
#include "ship_hull_structure_geometry.hpp"
#include "ship_hull_material_ablation.hpp"
#include <cctype>
#include <cstring>
#include <cstdlib>

// Current material is authoritative; weapon operations are transient inputs.
// ASCII wire data lives in normal saved attributes, whose outer SAVE codec
// already compresses strings. No native entity pointers or GPU data are saved.
namespace storm::ship::material
{
using namespace storm_ship_laser;
using storm_ship_laser::Plane;
inline constexpr size_t ChunkSize=32768;
// Persistence and every visual projection share the same authored node domain.
template<class Node> inline bool isPhysicalNode(Node*node){
    if(!node||!node->geo)return false;const char*name=node->GetName();if(!name)return true;
    constexpr char excluded[]="path";
    for(unsigned n=0;;++n){if(std::tolower(static_cast<unsigned char>(name[n]))!=excluded[n])return true;if(!excluded[n])return false;}
}
inline uint64_t revision(const char*value){if(!value||!*value)return 0;uint64_t result=0;for(const char*p=value;*p;++p){if(*p<'0'||*p>'9'||result>(UINT64_MAX-unsigned(*p-'0'))/10)return 0;result=result*10+unsigned(*p-'0');}return result;}
template<class Attr> inline bool readTail(Attr*slot,std::vector<Plane>&tail){
    const auto count=slot->GetAttributeAsDword("tail");if(count>32)return false;std::vector<Plane>next;
    for(unsigned n=0;n<count;++n){auto*p=slot->GetAttributeClass("t"+std::to_string(n));if(!p)return false;Plane field;
        field.normal={p->GetAttributeAsFloat("x"),p->GetAttributeAsFloat("y"),p->GetAttributeAsFloat("z")};field.distance=p->GetAttributeAsFloat("d");field.charred=p->GetAttributeAsDword("charred",1)!=0;field.releaseEnclosed=p->GetAttributeAsDword("enclosed",1)!=0;
        field.grain={p->GetAttributeAsFloat("gx"),p->GetAttributeAsFloat("gy",1.f),p->GetAttributeAsFloat("gz")};field.roughness=p->GetAttributeAsFloat("roughness");field.spacing=p->GetAttributeAsFloat("spacing",1.f);field.phase=p->GetAttributeAsFloat("phase");
        if(!storm_hull_damage::readBeamSamples(p,field.beams,storm_hull_damage::BeamCoordinates::NodeLocal)||!storm_ship_laser::valid(field))return false;next.push_back(std::move(field));}tail=std::move(next);return true;
}
template<class Attr> inline void writeTail(Attr*slot,const std::vector<Plane>&tail){
    for(size_t n=0;n<tail.size();++n){const auto&field=tail[n];auto*p=slot->VerifyAttributeClass("t"+std::to_string(n));
        p->SetAttributeUseDword("enclosed",field.releaseEnclosed);
        storm_hull_damage::writeFloat(p,"x",field.normal.x);storm_hull_damage::writeFloat(p,"y",field.normal.y);storm_hull_damage::writeFloat(p,"z",field.normal.z);storm_hull_damage::writeFloat(p,"d",field.distance);p->SetAttributeUseDword("charred",field.charred);
        storm_hull_damage::writeFloat(p,"gx",field.grain.x);storm_hull_damage::writeFloat(p,"gy",field.grain.y);storm_hull_damage::writeFloat(p,"gz",field.grain.z);storm_hull_damage::writeFloat(p,"roughness",field.roughness);storm_hull_damage::writeFloat(p,"spacing",field.spacing);storm_hull_damage::writeFloat(p,"phase",field.phase);storm_hull_damage::writeBeamSamples(p,field.beams);}
    slot->SetAttributeUseDword("tail",uint32_t(tail.size()));
}
template<class Attr> inline bool stateHeader(Attr*state,const std::string&model,uint64_t&version){
    if(!state||state->GetAttributeAsDword("version")!=1||!state->GetAttribute("model")||std::strcmp(model.c_str(),state->GetAttribute("model")))return false;
    version=revision(state->GetAttribute("revision"));return version!=0;
}
template<class Attr> inline Attr* metadata(Attr*state,uint64_t version){Attr*best=nullptr;uint64_t newest=0;for(const char*key:{"metaA","metaB"})if(auto*slot=state->GetAttributeClass(key)){const auto r=revision(slot->GetAttribute("revision"));if(r&&r<=version&&r>newest){best=slot;newest=r;}}return best;}
template<class Attr> inline Attr* writableMetadata(Attr*state){auto*a=state->GetAttributeClass("metaA");auto*b=state->GetAttributeClass("metaB");const auto committed=revision(state->GetAttribute("revision"));const auto ar=a?revision(a->GetAttribute("revision")):0,br=b?revision(b->GetAttribute("revision")):0;return state->VerifyAttributeClass(ar&&ar<=committed&&(!(br&&br<=committed)||ar>=br)?"metaB":"metaA");}
// Two publication slots retain the preceding checkpoint until the root revision
// marker is written last. Readers choose the newest node slot <= that marker.
template<class Attr> inline bool readNode(Attr*state,const std::string&model,unsigned index,const std::string&name,uint64_t&version,std::string&payload,std::vector<Plane>*tail=nullptr){
    if(!stateHeader(state,model,version))return false;(void)index;
    Attr*best=nullptr;uint64_t newest=0;unsigned matches=0;
    for(size_t n=0;n<state->GetAttributesNum();++n){auto*node=state->GetAttributeClass(uint32_t(n));const char*key=node->GetThisName();if(!key||key[0]!='n'||!key[1])continue;
        bool numbered=true;for(const char*p=key+1;*p;++p)numbered&=*p>='0'&&*p<='9';if(!numbered)continue;
        Attr*current=nullptr;uint64_t rnew=0;for(const char*slotName:{"a","b"})if(auto*slot=node->GetAttributeClass(slotName)){const auto r=revision(slot->GetAttribute("revision"));if(r&&r<=version&&r>rnew){rnew=r;current=slot;}}
        if(current&&current->GetAttribute("name")&&!std::strcmp(name.c_str(),current->GetAttribute("name"))){++matches;if(rnew>newest){newest=rnew;best=current;}}}
    if(matches!=1)return false;
    if(!best)return false;auto*data=best;const char*reference=best->GetAttribute("blob");if(reference){if(std::strcmp(reference,"dataA")&&std::strcmp(reference,"dataB"))return false;data=best->GetParent()->GetAttributeClass(reference);if(!data||!best->GetAttribute("payload")||!data->GetAttribute("id")||std::strcmp(best->GetAttribute("payload"),data->GetAttribute("id")))return false;}
    const auto count=data->GetAttributeAsDword("chunks");const auto bytes=revision(data->GetAttribute("bytes"));
    if(!count||count!=(bytes+ChunkSize-1)/ChunkSize||bytes>std::numeric_limits<uint32_t>::max())return false;
    std::string next;next.reserve(size_t(bytes));for(unsigned n=0;n<count;++n){const char*part=data->GetAttribute("p"+std::to_string(n));if(!part)return false;const size_t length=std::strlen(part);if(length!=std::min(ChunkSize,size_t(bytes)-next.size()))return false;next.append(part,length);}
    if(tail&&!readTail(best,*tail))return false;payload=std::move(next);return true;
}
template<class Attr> inline void writeNode(Attr*state,unsigned index,const std::string&name,uint64_t version,const std::string&payload,const std::vector<Plane>&tail={}){
    Attr*node=nullptr;unsigned next=index;
    for(size_t n=0;n<state->GetAttributesNum();++n){auto*candidate=state->GetAttributeClass(uint32_t(n));const char*key=candidate->GetThisName();if(!key||key[0]!='n'||!key[1])continue;
        bool numbered=true;uint64_t number=0;for(const char*p=key+1;*p;++p){if(*p<'0'||*p>'9'){numbered=false;break;}number=number*10+unsigned(*p-'0');if(number>=UINT32_MAX){numbered=false;break;}}if(!numbered)continue;next=std::max(next,unsigned(number+1));
        Attr*latest=nullptr;uint64_t newest=0;for(const char*slotName:{"a","b"})if(auto*slot=candidate->GetAttributeClass(slotName)){const auto r=revision(slot->GetAttribute("revision"));if(r>newest){newest=r;latest=slot;}}
        if(latest&&latest->GetAttribute("name")&&!std::strcmp(name.c_str(),latest->GetAttribute("name")))node=candidate;
    }
    if(!node)node=state->VerifyAttributeClass("n"+std::to_string(next));
    auto*a=node->GetAttributeClass("a");auto*b=node->GetAttributeClass("b");
    const auto ar=a?revision(a->GetAttribute("revision")):0,br=b?revision(b->GetAttribute("revision")):0;
    const auto committedRevision=revision(state->GetAttribute("revision"));
    auto*committed=ar&&ar<=committedRevision&&(!(br&&br<=committedRevision)||ar>=br)?a:br&&br<=committedRevision?b:nullptr;
    auto*slot=node->VerifyAttributeClass(committed==a?"b":"a");
    slot->SetAttribute("name",name.c_str());
    const auto id=payload.size()>16?payload.substr(payload.size()-16):payload;Attr*data=nullptr;const char*dataKey=nullptr;
    for(const char*key:{"dataA","dataB"})if(auto*entry=node->GetAttributeClass(key);entry&&entry->GetAttribute("id")&&!std::strcmp(id.c_str(),entry->GetAttribute("id"))){data=entry;dataKey=key;break;}
    if(!data){const char*used=committed?committed->GetAttribute("blob"):nullptr;dataKey=used&&std::strcmp(used,"dataA")==0?"dataB":"dataA";data=node->VerifyAttributeClass(dataKey);data->SetAttribute("id","");
        const size_t count=(payload.size()+ChunkSize-1)/ChunkSize;for(size_t n=0;n<count;++n)data->SetAttribute("p"+std::to_string(n),payload.substr(n*ChunkSize,ChunkSize).c_str());
        data->SetAttributeUseDword("chunks",uint32_t(count));data->SetAttribute("bytes",std::to_string(payload.size()).c_str());data->SetAttribute("id",id.c_str());}
    slot->SetAttribute("blob",dataKey);slot->SetAttribute("payload",id.c_str());slot->SetAttribute("revision",std::to_string(version).c_str());
    writeTail(slot,tail);
}
inline uint64_t hashBytes(const void*data,size_t size,uint64_t hash=1469598103934665603ull){
    const auto*p=static_cast<const uint8_t*>(data);for(size_t n=0;n<size;++n){hash^=p[n];hash*=1099511628211ull;}return hash;
}
inline std::string pristinePayload(uint64_t content){constexpr char digits[]="0123456789abcdef";std::string result="P";for(unsigned n=0;n<16;++n)result+=digits[(content>>((15-n)*4))&15];return result;}
inline std::array<uint32_t,15> vertexWords(const Vertex&v){
    std::array<uint32_t,15> words{};const float floats[14]={v.position.x,v.position.y,v.position.z,v.normal.x,v.normal.y,v.normal.z,
        v.uv[0],v.uv[1],v.uv[2],v.uv[3],v.uv[4],v.uv[5],v.uv[6],v.uv[7]};
    for(unsigned n=0;n<6;++n)std::memcpy(&words[n],&floats[n],4);words[6]=v.color;
    for(unsigned n=6;n<14;++n)std::memcpy(&words[n+1],&floats[n],4);return words;
}
inline double area(const Mesh&mesh){double result=0;for(const auto&o:mesh)for(const auto&t:o.second){storm_geometry_work::checkpoint();result+=storm_ship_hull::area(t);}return result;}
inline uint64_t fingerprint(const Mesh&authored,const std::map<int,float>&wood){
    uint64_t result=1469598103934665603ull;for(const auto&o:authored){result=hashBytes(&o.first,sizeof(o.first),result);
        for(const auto&t:o.second)for(const auto&v:t){storm_geometry_work::checkpoint();const auto words=vertexWords(v);result=hashBytes(words.data(),sizeof(words),result);}}
    for(const auto&w:wood){result=hashBytes(&w.first,sizeof(w.first),result);result=hashBytes(&w.second,sizeof(w.second),result);}return result;
}
inline std::shared_ptr<const SurfaceBase> supportSource(const Mesh&authored){
    auto source=std::make_shared<SurfaceBase>();source->outer=authored;source->index=indexFaces(source->outer);return source;
}
inline std::shared_ptr<const MaterialCellSource> cellSource(const Surface&authored,const std::map<int,float>&wood){
    auto result=std::make_shared<MaterialCellSource>();
    const repair_detail::AuthoredLookup lookup(authored);
    if(authored.timber)for(const auto&prism:*authored.timber){storm_geometry_work::checkpoint();result->prisms.push_back(prism);Mesh solid;const auto found=lookup.source.find(repair_detail::sourceKey(prism));if(found==lookup.source.end())throw std::runtime_error("missing authored timber cell");const Triangle*appearance=found->second;
        const Vec3 center=(prism.vertices[0]+prism.vertices[1]+prism.vertices[2]+prism.vertices[3]+prism.vertices[4]+prism.vertices[5])*(1.f/6.f);
        constexpr unsigned faces[8][3]={{0,1,2},{3,5,4},{0,3,4},{0,4,1},{1,4,5},{1,5,2},{2,5,0},{0,5,3}};
        for(const auto&f:faces){Triangle t;for(unsigned n=0;n<3;++n){t[n]=(*appearance)[f[n]%3];t[n].position=prism.vertices[f[n]];}auto normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));
            if(storm_ship_hull::dot(normal,center-t[0].position)>0){std::swap(t[1],t[2]);normal=normal*-1.f;}for(auto&v:t)v.normal=normal;solid[prism.object].push_back(t);}
        result->bounds.push_back(bounds(solid));result->solids.push_back(std::make_shared<const Mesh>(std::move(solid)));}
    std::set<int>objects;for(const auto&o:authored.outer)objects.insert(o.first);
    for(auto&solid:materialSolids(authored,objects,wood)){result->bounds.push_back(bounds(solid));result->solids.push_back(std::make_shared<const Mesh>(std::move(solid)));}
    result->index.build(result->bounds);return result;
}
template<class Planes> inline bool clipCell(const Mesh&solid,const Planes&planes,Mesh&kept){
    // Coarse failed-cell fields often contain a complete timber source, or
    // only overlap its broadphase box. Neither case needs section Booleans.
    // Use the exact splitter's side rule; touching/partial solids still take
    // the established closed-boundary path below.
    bool contained=true,any=false;
    for(const auto&plane:planes){bool separated=true;
        for(const auto&o:solid)for(const auto&t:o.second)for(const auto&v:t){
            const auto side=planarSide(plane,v.position);any=true;
            separated&=side>0;contained&=side<0;
        }
        if(any&&separated){kept=solid;return true;}
    }
    if(any&&contained){kept.clear();return true;}
    Mesh removed;if(!subtractVolume(solid,planes,kept,removed))return false;if(removed.empty())return true;
    for(size_t n=0;n<planes.size();++n){std::vector<storm_ship_hull::Plane>limits;for(size_t p=0;p<planes.size();++p)if(p!=n)limits.push_back(planes[p]);Mesh inside,outside;
        if(!solidSectionCaps(solid,planes[n],limits,{},false,inside,outside))return false;appendMesh(kept,outside);}
    for(auto it=kept.begin();it!=kept.end();)if(it->second.empty())it=kept.erase(it);else ++it;return true;
}
inline bool clipCells(Surface&surface,const Plane&field){
    if(!surface.cells)return false;auto next=std::make_shared<MaterialCells>(*surface.cells);
    if(field.beams.empty()){
        for(uint32_t id=0;id<next->source->solids.size();++id){const auto&solid=materialCell(*next,id);if(solid.empty())continue;auto divided=split(solid,field,nullptr,true);if(!divided.complete)return false;next->changed[id]=std::make_shared<const Mesh>(std::move(divided.kept));}
    }else for(const auto&beam:field.beams){std::vector<uint32_t>ids;next->source->index.queryBounds(beamBounds(beam),ids);const auto planes=beamPlanes(beam);
        for(auto id:ids){storm_geometry_work::checkpoint();const auto&solid=materialCell(*next,id);if(solid.empty())continue;Mesh kept;if(!clipCell(solid,planes,kept))return false;
            next->changed[id]=std::make_shared<const Mesh>(std::move(kept));}}
    surface.cells=std::move(next);return true;
}
inline void repairVolumes(Surface&surface){std::vector<Mesh>volumes;if(surface.cells)for(uint32_t n=0;n<surface.cells->source->solids.size();++n){const auto&solid=materialCell(*surface.cells,n);if(!solid.empty())volumes.push_back(solid);}surface.material=std::make_shared<const std::vector<Mesh>>(std::move(volumes));}
inline uint64_t openingSource(const SurfaceBase&source,uint32_t id,Vec3 point){const auto ref=source.index.references.at(id);const auto&t=source.outer.at(ref.first).at(ref.second);const auto box=repair_detail::faceBounds(t);
    const unsigned band=std::min(15u,unsigned(std::max(0.f,point.y-box.lo.y)*16.f/std::max(.000001f,box.hi.y-box.lo.y)));return uint64_t(id)*16+band;}
inline bool tagOpenings(const Surface&surface,const std::map<int,float>&wood,std::vector<Opening>&openings){
    if(!surface.supportOriginal)return false;const auto&source=*surface.supportOriginal;std::vector<uint32_t>ids;const Vec3 margin{.0002f,.0002f,.0002f};
    for(auto&opening:openings){storm_geometry_work::checkpoint();source.index.broadphase.queryBounds({opening.point-margin,opening.point+margin},ids);uint32_t best=UINT32_MAX;
        for(auto id:ids){const auto ref=source.index.references[id];if(wood.count(ref.first)&&pointOnFace(opening.point,source.outer.at(ref.first)[ref.second]))best=std::min(best,id);}
        if(best==UINT32_MAX){opening.area=0;continue;}opening.source=openingSource(source,best,opening.point);}return true;
}
inline bool currentOpenings(const Surface&surface,const std::map<int,float>&wood,std::vector<Opening>&output){
    if(!surface.supportOriginal)return false;const auto&source=*surface.supportOriginal;const repair_detail::MeshLookup current(surface.outer);std::map<uint64_t,Opening>samples;
    for(uint32_t id=0;id<source.index.references.size();++id){const auto ref=source.index.references[id];if(!wood.count(ref.first))continue;std::vector<Triangle>missing;
        if(!repair_detail::missing(source.outer.at(ref.first)[ref.second],ref.first,current,missing))return false;
        for(const auto&t:missing){storm_geometry_work::checkpoint();const float measured=storm_ship_hull::area(t);if(measured<=1.e-10f)continue;const auto point=(t[0].position+t[1].position+t[2].position)*(1.f/3.f);const auto key=openingSource(source,id,point);auto&sample=samples[key];
            sample.point=(sample.point*sample.area+point*measured)*(1.f/(sample.area+measured));sample.area+=measured;sample.source=key;}}
    for(auto&sample:samples)output.push_back(std::move(sample.second));return true;
}
inline Surface rebase(Surface current){
    // Preserve role boundaries and immutable support provenance. The new cap
    // solid is the ACTUAL remaining physical boundary; thickness never regrows.
    current.original=std::make_shared<const SurfaceBase>(SurfaceBase{current.outer,current.canonicalCaps,current.interior,current.finiteWalls,{}});
    Mesh released;for(const auto&piece:current.released)appendMesh(released,*piece);current.released.clear();
    if(!released.empty())current.released.push_back(std::make_shared<const Mesh>(std::move(released)));
    current.cutters.clear();current.legacy.clear();if(current.cells){current.timber.reset();current.material.reset();}current.checkpointed=true;return current;
}
struct Writer {
    std::vector<uint8_t>bytes;
    void word(uint32_t n){for(unsigned b=0;b<4;++b)bytes.push_back(uint8_t(n>>(b*8)));}
    void wide(uint64_t n){word(uint32_t(n));word(uint32_t(n>>32));}
    void mesh(const Mesh&mesh){word(uint32_t(mesh.size()));for(const auto&o:mesh){storm_geometry_work::checkpoint();word(uint32_t(o.first));
        std::map<std::array<uint32_t,15>,uint32_t>map;std::vector<std::array<uint32_t,15>>vertices;std::vector<uint32_t>indices;
        for(const auto&t:o.second)for(const auto&v:t){storm_geometry_work::checkpoint();const auto words=vertexWords(v);auto found=map.emplace(words,uint32_t(vertices.size()));if(found.second)vertices.push_back(words);indices.push_back(found.first->second);}
        word(uint32_t(vertices.size()));word(uint32_t(o.second.size()));for(const auto&v:vertices)for(auto n:v)word(n);for(auto n:indices)word(n);}}
};
inline std::string encode(const Surface&surface,uint64_t content,std::shared_ptr<const storm_ship_structure::State> structure={}){
    if(!structure)structure=surface.structure;
    Writer writer;writer.word(structure?0x334d4843:0x314d4843);writer.wide(content);
    for(const auto*mesh:{&surface.outer,&surface.canonicalCaps,&surface.interior,&surface.finiteWalls})writer.mesh(*mesh);
    Mesh released;for(const auto&piece:surface.released)appendMesh(released,*piece);writer.mesh(released);
    writer.word(surface.cells?uint32_t(surface.cells->changed.size()):0);if(surface.cells)for(const auto&entry:surface.cells->changed){writer.word(entry.first);writer.mesh(*entry.second);}
    if(structure){
        if(!storm_ship_structure::detail::validState(*structure))throw std::runtime_error("invalid hull structure checkpoint");
        writer.wide(structure->layout->fingerprint);writer.word(uint32_t(structure->cellHealth.size()));
        for(size_t n=0;n<structure->cellHealth.size();++n){uint32_t health;std::memcpy(&health,&structure->cellHealth[n],4);writer.word(health);writer.word(structure->alive[n]);}
        writer.word(uint32_t(structure->bondHealth.size()));for(float value:structure->bondHealth){uint32_t health;std::memcpy(&health,&value,4);writer.word(health);}
    }
    writer.wide(hashBytes(writer.bytes.data(),writer.bytes.size()));
    constexpr char digits[]="0123456789abcdef";std::string encoded;encoded.resize(writer.bytes.size()*2);
    for(size_t n=0;n<writer.bytes.size();++n){storm_geometry_work::checkpoint();encoded[n*2]=digits[writer.bytes[n]>>4];encoded[n*2+1]=digits[writer.bytes[n]&15];}return encoded;
}
struct Reader {
    std::vector<uint8_t>bytes;size_t at=0;bool valid=true;
    uint32_t word(){if(at+4>bytes.size()){valid=false;return 0;}uint32_t n=0;for(unsigned b=0;b<4;++b)n|=uint32_t(bytes[at++])<<(b*8);return n;}
    uint64_t wide(){const uint64_t lo=word(),hi=word();return lo|(hi<<32);}
    bool mesh(Mesh&mesh){const auto objects=word();if(objects>(bytes.size()-at)/12)return false;
        for(uint32_t o=0;o<objects;++o){storm_geometry_work::checkpoint();const int object=int32_t(word());const auto nv=word(),nt=word();
            if(!valid||object<0||mesh.count(object)||uint64_t(nv)*60+uint64_t(nt)*12>bytes.size()-at)return false;
            std::vector<Vertex>vertices;vertices.reserve(nv);for(uint32_t n=0;n<nv;++n){Vertex v;std::array<uint32_t,15>w{};for(auto&x:w)x=word();
                float f[14];for(unsigned i=0;i<6;++i)std::memcpy(&f[i],&w[i],4);v.color=w[6];for(unsigned i=6;i<14;++i)std::memcpy(&f[i],&w[i+1],4);
                for(float x:f)if(!std::isfinite(x))return false;v.position={f[0],f[1],f[2]};v.normal={f[3],f[4],f[5]};for(unsigned i=0;i<8;++i)v.uv[i]=f[i+6];vertices.push_back(v);}
            auto&triangles=mesh[object];triangles.reserve(nt);for(uint32_t n=0;n<nt;++n){Triangle t;for(auto&v:t){auto i=word();if(!valid||i>=nv)return false;v=vertices[i];}triangles.push_back(t);}}
        return valid;
    }
};
inline bool decode(const std::string&encoded,uint64_t content,std::shared_ptr<const SurfaceBase>support,std::shared_ptr<const MaterialCellSource>source,Surface&output,const std::map<int,float>&wood={}){
    if(encoded.size()%2||encoded.size()<40)return false;Reader reader;reader.bytes.reserve(encoded.size()/2);
    const auto digit=[](char c){return c>='0'&&c<='9'?c-'0':c>='a'&&c<='f'?c-'a'+10:-1;};
    for(size_t n=0;n<encoded.size();n+=2){storm_geometry_work::checkpoint();const int a=digit(encoded[n]),b=digit(encoded[n+1]);if(a<0||b<0)return false;reader.bytes.push_back(uint8_t(a*16+b));}
    const auto format=reader.word();if((format!=0x314d4843&&format!=0x324d4843&&format!=0x334d4843)||reader.wide()!=content)return false;Surface next;Mesh released;
    for(auto*mesh:{&next.outer,&next.canonicalCaps,&next.interior,&next.finiteWalls,&released})if(!reader.mesh(*mesh))return false;
    auto cells=std::make_shared<MaterialCells>();cells->source=std::move(source);const auto changed=reader.word();if(changed>cells->source->solids.size())return false;
    for(uint32_t n=0;n<changed;++n){const auto id=reader.word();Mesh solid;if(id>=cells->source->solids.size()||cells->changed.count(id)||!reader.mesh(solid))return false;cells->changed[id]=std::make_shared<const Mesh>(std::move(solid));}next.cells=std::move(cells);
    next.supportOriginal=std::move(support);
    if(format==0x324d4843||format==0x334d4843){
        if(!next.supportOriginal)return false;
        const auto reference=timberSurface(next.supportOriginal->outer,wood);
        const auto layout=storm_ship_structure::build(reference,wood);
        if(!layout||reader.wide()!=layout->fingerprint)return false;
        const auto count=reader.word();if(count!=layout->cells.size()||uint64_t(count)*8>reader.bytes.size()-reader.at)return false;
        auto structure=std::make_shared<storm_ship_structure::State>(storm_ship_structure::seed(layout,next,wood));
        for(size_t n=0;n<count;++n){const uint32_t value=reader.word(),alive=reader.word();if(alive>1)return false;std::memcpy(&structure->cellHealth[n],&value,4);structure->alive[n]=uint8_t(alive);}
        const auto bonds=reader.word();if(bonds!=layout->bonds.size()||uint64_t(bonds)*4>reader.bytes.size()-reader.at)return false;
        for(size_t n=0;n<bonds;++n){const uint32_t value=reader.word();std::memcpy(&structure->bondHealth[n],&value,4);}
        if(!storm_ship_structure::detail::validState(*structure))return false;
        next.structure=std::move(structure);
    }
    const auto end=reader.at;const auto checksum=reader.wide();if(!reader.valid||reader.at!=reader.bytes.size()||checksum!=hashBytes(reader.bytes.data(),end))return false;
    if(!released.empty())next.released.push_back(std::make_shared<const Mesh>(std::move(released)));
    output=rebase(std::move(next));return true;
}
// The whole-hull specific work domain uses an area-density proxy, as detached
// bodies already do. Capacity is local retained section / authored hull area;
// no energy is spent outside the weapon's explicit finite contact footprint.
inline Plane workField(const Input&input){Plane field=input.plane;double whole=input.structuralArea;
    if(whole<=1.e-9)return field;
    for(auto&beam:field.beams)if(beam.specificWork>0&&beam.contactStrength>0){
        const auto planes=beamPlanes(beam);const auto normal=beamDirection(beam),point=beamStart(beam)+normal*(beam.depth*.5f);
        const storm_ship_hull::Plane section{normal,storm_ship_hull::dot(normal,point)};std::vector<storm_ship_hull::Plane>limits(planes.begin()+2,planes.end());Mesh inside,outside;
        if(input.surface->cells){std::vector<uint32_t>ids;input.surface->cells->source->index.queryBounds(beamBounds(beam),ids);for(auto id:ids){const auto&solid=materialCell(*input.surface->cells,id);if(!solid.empty())solidSectionCaps(solid,section,limits,point,false,inside,outside);}}
        else if(input.surface->material)for(const auto&solid:*input.surface->material)solidSectionCaps(solid,section,limits,point,false,inside,outside);
        if(input.surface->timber)timberSectionCaps(*input.surface->timber,input.surface->legacy,section,limits,point,false,inside,outside);
        double retained=area(inside);if(retained<=1.e-9){Mesh patch,unused;std::set<int>objects;subtractVolume(input.surface->outer,planes,unused,patch,&objects);retained=area(patch);}
        const double capacity=beam.contactStrength*retained/whole;
        if(capacity>0&&beam.specificWork<capacity)beam.depth*=float(beam.specificWork/(beam.specificWork+capacity));
    }return field;
}
inline bool prepareExact(const Input&input,Projection&output,bool resolved=false){
    if(!input.physicalContact){Projection unchanged;unchanged.generation=input.generation;unchanged.revision=input.revision;unchanged.materialRevision=input.materialRevision;unchanged.plane=input.plane;output=std::move(unchanged);return true;}
    if(!input.surface)return false;Input adjusted=input;if(!resolved)adjusted.plane=workField(input);adjusted.structuralFragmentsOnly=true;Projection next;
    if(!storm_ship_laser::prepare(adjusted,next))return false;
    next.materialRevision=input.materialRevision;
    next.plane.releaseEnclosed=input.enclosedFragments;
    if(next.result.changed){auto retained=*next.kept;retained.cells=input.surface->cells;if(!clipCells(retained,adjusted.plane))return false;
        std::vector<std::shared_ptr<const Mesh>>released;for(const auto&piece:retained.released)if(std::find(input.surface->released.begin(),input.surface->released.end(),piece)==input.surface->released.end())released.push_back(piece);
        if(!trimReleasedCells(retained,released))return false;
        if(!tagOpenings(retained,input.wood,next.result.openings))return false;
        const auto previous=input.structure?input.structure:input.surface->structure;
        if(previous&&!resolved){auto state=storm_ship_structure::seed(previous->layout,retained,input.wood);
            for(size_t n=0;n<state.cellHealth.size();++n){state.cellHealth[n]=std::min(state.cellHealth[n],previous->cellHealth[n]);state.alive[n]&=previous->alive[n];}
            for(size_t n=0;n<state.bondHealth.size();++n)state.bondHealth[n]=std::min(state.bondHealth[n],previous->bondHealth[n]);
            next.structure=std::make_shared<const storm_ship_structure::State>(std::move(state));retained.structure=next.structure;next.result.stateChanged=true;}
        if(input.materialTailCount>=31||next.structure){retained=rebase(std::move(retained));next.materialPayload=encode(retained,input.fingerprint,next.structure);}
        if(next.retainedSupport.references.empty())next.retainedSupport=indexFaces(retained.outer);
        if(retained.supportOriginal)next.supportCandidates.reserve(retained.supportOriginal->index.references.size());next.supportNearby.reserve(next.retainedSupport.references.size());
        next.kept=std::make_shared<const Surface>(std::move(retained));}
    output=std::move(next);return true;
}
// A graph failure owns a finite region of real timber. Update its sparse
// closed-cell cache once, independently of decorative/render shell closure.
inline bool fractureCells(Surface&surface,const std::vector<BeamSample>&fields,const std::map<int,float>&wood){
    if(!surface.cells)return false;auto next=std::make_shared<MaterialCells>(*surface.cells);std::vector<uint32_t>ids;
    for(const auto&field:fields){next->source->index.queryBounds(beamBounds(field),ids);const auto planes=beamPlanes(field);
        for(auto id:ids){storm_geometry_work::checkpoint();const auto&solid=materialCell(*next,id);if(solid.empty()||!wood.count(solid.begin()->first))continue;
            Mesh retained;if(!clipCell(solid,planes,retained))return false;next->changed[id]=std::make_shared<const Mesh>(std::move(retained));}}
    surface.cells=std::move(next);return true;
}
inline bool preparePhysicalCache(Surface&surface){auto cells=convex::ready(surface.cells,surface.structure?surface.structure->layout:nullptr);if(!cells)return false;surface.cells=std::move(cells);ablation::prepareRoleIndices(surface);return true;}
inline bool releaseSections(const Surface*before,Surface&current,unsigned allowance,const std::map<int,float>&wood,std::vector<Surface>&pieces,bool&deferred){
    if(!storm_hull_damage::EnableHullSectionRelease){(void)before;(void)current;(void)allowance;(void)wood;pieces.clear();deferred=false;return true;}
    bool handled=false;if(!structure_geometry::releaseConnected(current,allowance,wood,pieces,deferred,handled))return false;if(handled)return true;
    const auto cells=current.cells;const auto structure=current.structure;const size_t old=current.released.size();
    const auto authored=!before&&current.supportOriginal?timberSurface(current.supportOriginal->outer,wood):Surface{};
    auto separated=detachEnclosed(before?*before:authored,current);current.cells=cells;current.structure=structure;
    if(current.released.size()!=old+separated.size())return false;const size_t take=std::min(size_t(allowance),separated.size());deferred=take<separated.size();
    for(size_t n=take;n<separated.size();++n)for(unsigned role=0;role<4;++role)appendMesh(surfaceRole(current,role),surfaceRole(separated[n],role));
    current.released.resize(old+take);separated.resize(take);pieces=std::move(separated);
    std::vector<std::shared_ptr<const Mesh>>masks(current.released.begin()+old,current.released.end());return trimReleasedCells(current,masks);
}
inline bool prepare(const Input&input,Projection&output,bool resolved=false){
    const auto structure=input.structure?input.structure:input.surface?input.surface->structure:nullptr;
    if(input.releaseOnly&&(!structure||!structure->layout||structure->layout->cells.empty()||input.wood.empty())){Projection next;next.generation=input.generation;next.revision=input.revision;next.materialRevision=input.materialRevision;next.plane=input.plane;next.kept=input.surface;output=std::move(next);return true;}
    // Resolved CHM1 tails are exact historical operations. A non-structural
    // receiver and legacy plane retain that same established geometry path.
    if(!input.releaseOnly&&(resolved||input.plane.beams.empty()||input.wood.empty()||!structure||!structure->layout||structure->layout->cells.empty()))return prepareExact(input,output,resolved);
    if(!input.surface||!valid(input.plane))return false;Projection next;next.generation=input.generation;next.revision=input.revision;next.materialRevision=input.materialRevision;next.plane=input.plane;next.enclosedFragments=input.enclosedFragments;next.kept=input.surface;next.structure=structure;
    next.hasDeferredSections=input.hasDeferredSections||structure_geometry::hasDeferred(*structure)||(input.surface->cells&&convex::compatible(*input.surface->cells)&&convex::hasDetached(*input.surface->cells->convex));
    if(!input.physicalContact&&!input.releaseOnly){output=std::move(next);return true;}
    if(input.releaseOnly&&(!next.hasDeferredSections||!input.fragmentLimit)){output=std::move(next);return true;}
    const bool thermal=input.plane.charred&&std::all_of(input.plane.beams.begin(),input.plane.beams.end(),[](const auto&beam){return beam.specificWork==0&&beam.contactStrength==0&&beam.crossSection.empty();});
    storm_ship_structure::DamageResult damage;structure_geometry::Realization geometry;bool fractured=false,mechanicalGeometry=false;
    if(thermal||input.releaseOnly){ablation::LocalResult local;
        if(input.releaseOnly)local.kept=*input.surface;
        else if(!ablation::cut(input,local,[](const auto&solid,const auto&planes,auto&kept){return clipCell(solid,planes,kept);}))return false;
        bool split=input.releaseOnly;if(local.changed&&!split){if(local.kept.cells&&convex::compatible(*local.kept.cells))split=convex::hasDetached(*local.kept.cells->convex);else if(!ablation::newlySplit(input,local.kept,split))return false;}std::vector<Surface>localPieces;bool localDeferred=false;
        if(split){
            if(!releaseSections(input.releaseOnly?nullptr:input.surface.get(),local.kept,input.fragmentLimit,input.wood,localPieces,localDeferred))return false;
            // Exact local islands can occupy only part of a coarse cell. The
            // graph must reconcile their actual lost source material too.
            local.changedIDs.clear();for(const auto&entry:local.kept.cells->changed){const auto old=input.surface->cells->changed.find(entry.first);if(old==input.surface->cells->changed.end()||old->second!=entry.second)local.changedIDs.push_back(entry.first);}
            for(const auto&piece:localPieces){local.changed=true;local.removedArea+=area(canonicalMesh(piece));for(const auto&o:piece.outer)for(const auto&t:o.second)local.openings.push_back({(t[0].position+t[1].position+t[2].position)*(1.f/3.f),storm_ship_hull::area(t)});}}
        // Physical removal pays the lost capacity exactly once. It does not
        // also spend the same thermal volume as a latent graph work impulse.
        if(local.changed)damage=storm_ship_structure::reconcile(*structure,*input.surface,local.kept,input.wood,&local.changedIDs);else damage.state=*structure;
        structure_geometry::includeDeferred(damage);
        auto actual=std::make_shared<Surface>(std::move(local.kept));Input separated=input;separated.surface=actual;separated.fragmentLimit-=unsigned(localPieces.size());
        if(!structure_geometry::realize(separated,damage,geometry))return false;const bool graphFracture=geometry.changed;fractured=graphFracture||!localPieces.empty();
        if(!graphFracture)geometry.kept=std::move(*actual);
        geometry.pieces.insert(geometry.pieces.end(),std::make_move_iterator(localPieces.begin()),std::make_move_iterator(localPieces.end()));
        geometry.changed=graphFracture||local.changed;geometry.deferred|=localDeferred;geometry.removedArea+=local.removedArea;
        geometry.openings.insert(geometry.openings.end(),std::make_move_iterator(local.openings.begin()),std::make_move_iterator(local.openings.end()));
        geometry.seam.insert(geometry.seam.end(),std::make_move_iterator(local.seam.begin()),std::make_move_iterator(local.seam.end()));
        if(!fractured)next.changedObjects=std::move(local.changedObjects);
    }else{
        damage=storm_ship_structure::damage(*structure,input.plane,*input.surface,input.wood,input.structuralArea,!input.realizeMechanicalCrush);
        Input physicalInput=input;ablation::LocalResult local;std::vector<Surface>localPieces;bool localDeferred=false;
        if(input.realizeMechanicalCrush){const auto fields=ablation::crushFields(input,damage);
            if(!fields.empty()&&!ablation::cutFields(input,fields,local,[](const auto&solid,const auto&planes,auto&kept){return clipCell(solid,planes,kept);}))return false;
            if(local.changed){bool split=false;if(local.kept.cells&&convex::compatible(*local.kept.cells))split=convex::hasDetached(*local.kept.cells->convex);else if(!ablation::newlySplit(input,local.kept,split))return false;
                if(split){if(!releaseSections(input.surface.get(),local.kept,input.fragmentLimit,input.wood,localPieces,localDeferred))return false;
                    local.changedIDs.clear();for(const auto&entry:local.kept.cells->changed){const auto old=input.surface->cells->changed.find(entry.first);if(old==input.surface->cells->changed.end()||old->second!=entry.second)local.changedIDs.push_back(entry.first);}
                    for(const auto&piece:localPieces){local.removedArea+=area(canonicalMesh(piece));for(const auto&o:piece.outer)for(const auto&t:o.second)local.openings.push_back({(t[0].position+t[1].position+t[2].position)*(1.f/3.f),storm_ship_hull::area(t)});}}
                // Work already reduced graph health. Refresh actual remaining
                // sections by min(capacity, paid health), never subtract again.
                auto refreshed=storm_ship_structure::reconcile(damage.state,*input.surface,local.kept,input.wood,&local.changedIDs,true);
                damage.state=std::move(refreshed.state);damage.brokenBonds.insert(damage.brokenBonds.end(),refreshed.brokenBonds.begin(),refreshed.brokenBonds.end());
                physicalInput.surface=std::make_shared<const Surface>(std::move(local.kept));physicalInput.fragmentLimit-=unsigned(localPieces.size());mechanicalGeometry=true;
            }
            storm_ship_structure::finalizeDamage(damage);
        }
        structure_geometry::includeDeferred(damage);
        if(!damage.changed&&damage.brokenCells.empty()&&damage.releasedCells.empty()){output=std::move(next);return true;}
        if(!structure_geometry::realize(physicalInput,damage,geometry))return false;fractured=geometry.changed||!localPieces.empty();
        if(mechanicalGeometry){if(!geometry.changed)geometry.kept=*physicalInput.surface;
            geometry.changed=true;geometry.deferred|=localDeferred;geometry.removedArea+=local.removedArea;
            geometry.pieces.insert(geometry.pieces.end(),std::make_move_iterator(localPieces.begin()),std::make_move_iterator(localPieces.end()));
            geometry.openings.insert(geometry.openings.end(),std::make_move_iterator(local.openings.begin()),std::make_move_iterator(local.openings.end()));
            geometry.seam.insert(geometry.seam.end(),std::make_move_iterator(local.seam.begin()),std::make_move_iterator(local.seam.end()));
            if(!fractured)next.changedObjects=std::move(local.changedObjects);
        }
    }
    next.result.stateChanged=damage.changed;next.hasDeferredSections=geometry.deferred||(!input.releaseOnly&&input.hasDeferredSections);
    if(!geometry.changed){next.structure=std::make_shared<const storm_ship_structure::State>(std::move(damage.state));
        if(input.materialTailCount>=31||(next.hasDeferredSections&&damage.changed))next.materialPayload=encode(*input.surface,input.fingerprint,next.structure);
        output=std::move(next);return true;}
    // Failed mechanical regions and disconnected sections transfer to bodies.
    // Both cease belonging to the hull's actual finite material-cell cache.
    if(!geometry.removedCells.empty()&&!fractureCells(geometry.kept,geometry.removedCells,input.wood))return false;
    if(!tagOpenings(geometry.kept,input.wood,geometry.openings))return false;
    if(fractured){auto physical=storm_ship_structure::seed(damage.state.layout,geometry.kept,input.wood);damage.state.contact=std::move(physical.contact);damage.state.physicalCellHealth=std::move(physical.physicalCellHealth);damage.state.physicalBondHealth=std::move(physical.physicalBondHealth);}
    next.hasDeferredSections|=structure_geometry::hasDeferred(damage.state);
    next.structure=std::make_shared<const storm_ship_structure::State>(std::move(damage.state));geometry.kept.structure=next.structure;
    if(fractured||mechanicalGeometry||input.materialTailCount>=31){geometry.kept=rebase(std::move(geometry.kept));next.materialPayload=encode(geometry.kept,input.fingerprint,next.structure);}
    next.canonical=canonicalMesh(geometry.kept);next.render=renderMesh(geometry.kept);next.result.changed=true;next.result.keptBounds=bounds(next.canonical);next.result.seam=std::move(geometry.seam);next.result.openings=std::move(geometry.openings);
    const double keptArea=area(next.canonical),total=keptArea+geometry.removedArea;next.result.removedFraction=float(std::clamp(geometry.removedArea/std::max(total,1.e-7),0.,1.));
    if(input.hasSupport)next.result.supportSevered=beamSeversSupport(*input.surface,geometry.kept,input.support);
    for(const auto&piece:geometry.pieces){next.result.fragments.push_back(fragmentProperties(canonicalMesh(piece)));next.fragmentRender.push_back(renderMesh(piece));next.fragmentWalls.push_back(renderWalls(piece));}
    // A through-hole opens the outer envelope into the empty hold. Its filled
    // volume is no material invariant: the closed timber cells own the wood.
    // Leave keptVolume/volumeBased unset rather than rejecting a valid breach.
    next.collision=indexFaces(next.canonical);next.materialCollision=indexFaces(next.render);next.materialCandidates.reserve(next.materialCollision.references.size());next.retainedSupport=indexFaces(geometry.kept.outer);
    if(geometry.kept.supportOriginal)next.supportCandidates.reserve(geometry.kept.supportOriginal->index.references.size());next.supportNearby.reserve(next.retainedSupport.references.size());for(const auto&piece:geometry.kept.released)next.releasedSupport.push_back(indexFaces(*piece));
    next.kept=std::make_shared<const Surface>(std::move(geometry.kept));output=std::move(next);return true;
}
namespace convex {
inline ContactEdit finishContactEdit(const ContactSnapshot&snapshot,
    const std::shared_ptr<const MaterialCells>&cells,storm_ship_structure::DamageResult damage,
    const std::shared_ptr<PhysicalDelta>&delta,const ContactLimits&limits){
    ContactEdit result;result.next=snapshot;
    const auto&cache=*cells->convex;const auto&layout=*damage.state.layout;
    if(cache.source->layout!=damage.state.layout||cache.supportMeasures.size()!=layout.cells.size())return result;
    structure_geometry::includeDeferred(damage);
    const std::set<uint32_t>failed(damage.brokenCells.begin(),damage.brokenCells.end());
    const std::set<uint32_t>released(damage.releasedCells.begin(),damage.releasedCells.end());
    auto mask=snapshot.retainedSupports?std::make_shared<std::vector<uint8_t>>(*snapshot.retainedSupports):
        std::make_shared<std::vector<uint8_t>>(layout.cells.size(),1);
    std::set<uint32_t>assigned;
    const auto append=[&](const std::vector<uint32_t>&supports){
        if(!storm_hull_damage::EnableHullSectionRelease)return;
        Body body;body.reference=cells->convex;
        for(auto id:supports)if(id<mask->size()&&mask->at(id)&&assigned.insert(id).second){
            body.supports.push_back(id);mask->at(id)=0;
            const auto&sources=cache.source->supportSources[id];body.sources.insert(body.sources.end(),sources.begin(),sources.end());
        }
        if(body.supports.empty())return;
        std::sort(body.supports.begin(),body.supports.end());std::sort(body.sources.begin(),body.sources.end());
        body.sources.erase(std::unique(body.sources.begin(),body.sources.end()),body.sources.end());
        measureBody(body);if(body.volume>1.e-12)delta->released.push_back(std::move(body));
    };
    for(const auto&component:damage.components){std::vector<uint32_t>owned;for(auto id:component)if(released.count(id))owned.push_back(id);append(owned);}
    std::set<uint32_t>visited;
    for(auto first:failed){if(!visited.insert(first).second)continue;std::vector<uint32_t>group{first};
        for(size_t at=0;at<group.size();++at){const auto id=group[at];for(auto edge:layout.cells[id].bonds){
            const auto&bond=layout.bonds[edge];const auto other=bond.a==id?bond.b:bond.a;
            if(failed.count(other)&&edge<damage.state.physicalBondHealth.size()&&damage.state.physicalBondHealth[edge]>1.e-6f&&visited.insert(other).second)group.push_back(other);
        }}append(group);
    }
    if(delta->released.size()>limits.bodyGroups)return result;
    if(!damage.changed&&delta->changedCells.empty()&&delta->released.empty()){result.complete=true;return result;}
    for(const auto&body:delta->released)delta->changedCells.insert(delta->changedCells.end(),body.sources.begin(),body.sources.end());
    std::sort(delta->changedCells.begin(),delta->changedCells.end());
    delta->changedCells.erase(std::unique(delta->changedCells.begin(),delta->changedCells.end()),delta->changedCells.end());
    delta->after={cells,std::make_shared<const storm_ship_structure::State>(std::move(damage.state)),snapshot.revision+1,
        delta->released.empty()?snapshot.retainedSupports:mask};
    result.complete=true;result.next=delta->after;result.changedCells=delta->changedCells;
    result.detached=delta->released;result.spentWork=damage.spentWork;result.remainingWork=damage.remainingWork;
    result.physicalDelta=delta;return result;
}
// A broad, strong contact can transfer already partitioned support volumes
// without clipping thousands of authored triangles on the contact thread.
inline ContactEdit containedContactEdit(const ContactSnapshot&snapshot,const Plane&field,double wholeArea,const ContactLimits&limits){
    if(!storm_hull_damage::EnableHullSectionRelease){ContactEdit result;result.next=snapshot;return result;}
    ContactEdit result;result.next=snapshot;const auto&cache=*snapshot.cells->convex;const auto&layout=*snapshot.strength->layout;
    if(cache.source->layout!=snapshot.strength->layout||cache.supportMeasures.size()!=layout.cells.size()||layout.cells.size()>limits.graphVisits)return result;
    std::vector<std::vector<storm_ship_structure::detail::Contact>>contacts(field.beams.size());std::vector<uint32_t>ids;
    for(size_t n=0;n<field.beams.size();++n){const auto&beam=field.beams[n];if(beam.specificWork<=0||beam.contactStrength<=0)return result;const auto planes=beamPlanes(beam);storm_ship_structure::queryCells(layout,beamBounds(beam),ids);
        for(auto id:ids){if(snapshot.retainedSupports&&!snapshot.retainedSupports->at(id))continue;const auto&measure=cache.supportMeasures[id];if(measure.volume<=1.e-12)continue;const auto box=layout.cells[id].bounds;bool contained=true;for(const auto&plane:planes){const Vec3 extreme{plane.normal.x>0?box.hi.x:box.lo.x,plane.normal.y>0?box.hi.y:box.lo.y,plane.normal.z>0?box.hi.z:box.lo.z};if(storm_ship_hull::dot(plane.normal,extreme)>plane.distance+1.e-6f){contained=false;break;}}if(!contained)continue;
            const Vec3 start{beam.start[0],beam.start[1],beam.start[2]},direction{beam.direction[0],beam.direction[1],beam.direction[2]};const Vec3 front{direction.x>0?box.lo.x:box.hi.x,direction.y>0?box.lo.y:box.hi.y,direction.z>0?box.lo.z:box.hi.z};contacts[n].push_back({id,measure.volume,storm_ship_hull::dot(front-start,direction)});}
        std::sort(contacts[n].begin(),contacts[n].end(),[](const auto&a,const auto&b){return a.travel==b.travel?a.cell<b.cell:a.travel<b.travel;});}
    auto damage=storm_ship_structure::detail::damagePatches(*snapshot.strength,field,wholeArea,true,&contacts);structure_geometry::includeDeferred(damage);
    std::set<uint32_t>failed(damage.brokenCells.begin(),damage.brokenCells.end()),released(damage.releasedCells.begin(),damage.releasedCells.end());
    if(failed.empty()&&released.empty())return result;
    for(const auto&crush:damage.crushed)if(!failed.count(crush.cell)&&!released.count(crush.cell))return result;
    auto delta=std::make_shared<PhysicalDelta>();delta->before=snapshot;delta->field=field;
    return finishContactEdit(snapshot,snapshot.cells,std::move(damage),delta,limits);
}
inline ContactEdit tryContactEdit(const ContactSnapshot&snapshot,const Plane&field,const std::map<int,float>&wood,double wholeArea,const ContactLimits&limits){
    ContactEdit result;result.next=snapshot;if(!snapshot.cells||!compatible(*snapshot.cells)||!snapshot.strength||!snapshot.strength->layout||!valid(field)||field.beams.empty()||snapshot.revision==UINT64_MAX)return result;
    const auto&layout=*snapshot.strength->layout;if(snapshot.retainedSupports&&snapshot.retainedSupports->size()!=layout.cells.size())return result;
    auto broad=containedContactEdit(snapshot,field,wholeArea,limits);if(broad.complete)return broad;
    // A pending ownership mask cannot be fed into the unmasked narrow cache.
    // Its worker publication must finish first; no old wood is resurrected.
    if(snapshot.retainedSupports)return result;
    if(layout.cells.size()>limits.graphVisits)return result;
    size_t candidates=0,pieces=0,tetrahedra=0;std::vector<uint32_t>ids;const auto material=snapshot.strength->contact?snapshot.strength->contact:layout.contact;if(!material)return result;
    for(const auto&beam:field.beams){const auto box=beamBounds(beam);snapshot.cells->source->index.queryBounds(box,ids);candidates+=ids.size();if(candidates>limits.candidateCells)return result;
        for(auto id:ids)if(const auto&cell=snapshot.cells->convex->cells[id];cell&&cell->pieces)for(const auto&piece:*cell->pieces)if(overlapBounds(piece.bounds,box)){if(++pieces>limits.convexPieces)return result;}
        tetrahedra+=storm_ship_structure::detail::queryMaterial(*material,box).size();if(tetrahedra>limits.tetrahedra)return result;}
    auto delta=std::make_shared<PhysicalDelta>();delta->before=snapshot;delta->field=field;auto cells=snapshot.cells;auto strength=snapshot.strength;
    auto damage=storm_ship_structure::damage(*strength,field,wholeArea,false);const double spentWork=damage.spentWork,remainingWork=damage.remainingWork;
    Input input;input.plane=field;input.structure=strength;input.structuralArea=wholeArea;const auto crushed=ablation::crushFields(input,damage);std::vector<Cut>crushCuts;for(const auto&cut:crushed)crushCuts.push_back({cut.planes,cut.bounds,cut.origin,false});
    if(!crushCuts.empty()){auto editResult=edit(cells,crushCuts,limits,false,false);if(!editResult.complete)return result;if(!editResult.changed.empty()){
            Surface before,after;before.cells=cells;after.cells=editResult.cells;auto physical=storm_ship_structure::reconcile(damage.state,before,after,wood,&editResult.changed,true);damage.state=std::move(physical.state);damage.changed|=physical.changed;damage.brokenBonds.insert(damage.brokenBonds.end(),physical.brokenBonds.begin(),physical.brokenBonds.end());
            cells=editResult.cells;delta->caps=std::move(editResult.caps);delta->changedCells=std::move(editResult.changed);delta->ablation=std::move(crushCuts);}}
    storm_ship_structure::finalizeDamage(damage);
    if(!limits.passes)return result;
    damage.spentWork=spentWork;damage.remainingWork=remainingWork;
    return finishContactEdit(snapshot,cells,std::move(damage),delta,limits);
}
// Combined frame contacts keep their order. A later crush must remove an
// earlier new wall before the worker publishes the accumulated render mesh.
inline bool contactCaps(const PhysicalDelta&delta,Mesh&caps){
    if(delta.steps.empty()){caps=delta.caps;return true;}
    Mesh current;
    for(const auto&step:delta.steps){if(!step)return false;
        for(const auto&cut:step->ablation){Mesh kept,removed;if(!subtractVolume(current,cut.planes,kept,removed))return false;current=std::move(kept);}
        Mesh addition;if(!contactCaps(*step,addition))return false;appendMesh(current,addition);
    }
    caps=std::move(current);return true;
}
inline bool projectContactDelta(const Input&input,const PhysicalDelta&delta,Projection&output){
    if(!input.surface||input.surface->cells!=delta.before.cells||!delta.after.cells||!compatible(*delta.after.cells)||!delta.after.strength)return false;
    Projection next;next.generation=input.generation;next.revision=input.revision;next.materialRevision=input.materialRevision;next.plane=delta.field;next.enclosedFragments=input.enclosedFragments;next.structure=delta.after.strength;next.kept=input.surface;next.result.stateChanged=true;
    if(delta.changedCells.empty()&&delta.released.empty()){next.materialPayload=encode(*input.surface,input.fingerprint,next.structure);output=std::move(next);return true;}
    Mesh ownershipCaps;auto afterCells=materialize(delta.after,ownershipCaps);if(!afterCells)return false;std::vector<Body>bodies;for(const auto&body:delta.released)bodies.push_back(expandBody(body));
    Surface retained=*input.surface;ablation::LocalResult removed;std::vector<ablation::ConvexField>fields;for(const auto&cut:delta.ablation)fields.push_back({cut.planes,cut.bounds,cut.origin,cut.charred});
    if(!fields.empty())for(unsigned role=0;role<4;++role)if(!ablation::cutRole(surfaceRole(*input.surface,role),fields,input.wood,role,surfaceRole(retained,role),removed))return false;
    Mesh caps;if(!contactCaps(delta,caps))return false;appendMesh(retained.finiteWalls,caps);std::vector<Surface>groups;
    if(!bodies.empty()){if(!partitionSurface(retained,*afterCells,bodies,input.wood,groups,&next.changedObjects))return false;for(unsigned role=0;role<4;++role)surfaceRole(retained,role)=std::move(surfaceRole(groups[0],role));retained.decorations=std::move(groups[0].decorations);
        appendMesh(retained.finiteWalls,ownershipCaps);
        for(size_t n=0;n<bodies.size();++n){const auto&body=bodies[n];auto&piece=groups[n+1];const auto bodyIndex=pieceIndex(body.pieces);for(size_t p=0;p<body.pieces.size();++p)for(const auto&face:body.pieces[p].faces)if(face.fresh)for(const auto&polygon:externalFace(body.pieces,p,face,&bodyIndex))storm_ship_hull::fan(polygon,piece.finiteWalls[face.object]);
            const auto canonical=canonicalMesh(piece);removed.removedArea+=area(canonical);for(const auto&o:piece.outer)for(const auto&t:o.second)removed.openings.push_back({(t[0].position+t[1].position+t[2].position)*(1.f/3.f),storm_ship_hull::area(t)});
            next.result.fragments.push_back(fragmentProperties(canonical));next.fragmentRender.push_back(renderMesh(piece));next.fragmentWalls.push_back(renderWalls(piece));retained.released.push_back(std::make_shared<const Mesh>(renderMesh(piece)));}}
    else{next.changedObjects=removed.changedObjects;for(auto id:delta.changedCells)if(id<delta.after.cells->source->prisms.size())next.changedObjects.insert(delta.after.cells->source->prisms[id].object);}
    retained.cells=std::move(afterCells);if(delta.after.retainedSupports){auto measured=input.surface->structure?storm_ship_structure::reconcile(*input.surface->structure,*input.surface,retained,input.wood,&delta.changedCells,true).state:storm_ship_structure::seed(next.structure->layout,retained,input.wood);auto paid=std::make_shared<storm_ship_structure::State>(*next.structure);paid->contact=std::move(measured.contact);paid->physicalCellHealth=std::move(measured.physicalCellHealth);paid->physicalBondHealth=std::move(measured.physicalBondHealth);for(size_t n=0;n<paid->cellHealth.size();++n)paid->cellHealth[n]=std::min(paid->cellHealth[n],paid->physicalCellHealth[n]);for(size_t n=0;n<paid->bondHealth.size();++n)paid->bondHealth[n]=std::min(paid->bondHealth[n],paid->physicalBondHealth[n]);next.structure=std::move(paid);}retained.structure=next.structure;if(!tagOpenings(retained,input.wood,removed.openings))return false;retained=rebase(std::move(retained));
    next.changedObjects.insert(removed.changedObjects.begin(),removed.changedObjects.end());next.canonical=canonicalMesh(retained);next.render=renderMesh(retained);next.result.changed=true;next.result.keptBounds=bounds(next.canonical);next.result.openings=std::move(removed.openings);const double kept=area(next.canonical);next.result.removedFraction=float(std::clamp(removed.removedArea/std::max(1.e-7,kept+removed.removedArea),0.,1.));
    if(input.hasSupport)next.result.supportSevered=beamSeversSupport(*input.surface,retained,input.support);next.hasDeferredSections=hasDetached(*retained.cells->convex)||structure_geometry::hasDeferred(*next.structure);next.materialPayload=encode(retained,input.fingerprint,next.structure);
    next.collision=indexFaces(next.canonical);next.materialCollision=indexFaces(next.render);next.materialCandidates.reserve(next.materialCollision.references.size());next.retainedSupport=indexFaces(retained.outer);if(retained.supportOriginal)next.supportCandidates.reserve(retained.supportOriginal->index.references.size());next.supportNearby.reserve(next.retainedSupport.references.size());for(const auto&piece:retained.released)next.releasedSupport.push_back(indexFaces(*piece));next.kept=std::make_shared<const Surface>(std::move(retained));output=std::move(next);return true;
}
}
}
