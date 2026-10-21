// Temporary-context adapter only; GEOM methods/fields injected verbatim.
// Private CPU fixture context. Production field declarations and five method
// bodies are injected verbatim by generate.py, never reimplemented here.
#include "ship_laser_geometry.hpp"
#include <atomic>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <new>
#include <sstream>
#include <string>
static long allocFail=-1,allocCount=0;
void* operator new(std::size_t n){++allocCount;if(allocFail==0){allocFail=-1;throw std::bad_alloc();}if(allocFail>0)--allocFail;if(void*p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](std::size_t n){return ::operator new(n);}
void operator delete(void*p)noexcept{std::free(p);}void operator delete[](void*p)noexcept{std::free(p);}
void operator delete(void*p,std::size_t)noexcept{std::free(p);}void operator delete[](void*p,std::size_t)noexcept{std::free(p);}
struct FakeService {
 struct Buffer {void*data=nullptr;bool index=false,locked=false;};
 Buffer buffers[128]{};int next=0,calls=0,failAt=-1;bool throws=false;
 bool fault(){++calls;if(calls!=failAt)return false;if(throws)throw 7;return true;}
 int create(bool index,int bytes){if(fault())return -1;assert(next<128);int n=next++;buffers[n]={std::malloc(bytes?bytes:1),index,false};assert(buffers[n].data);return n;}
 int CreateVertexBuffer(int,int bytes){return create(false,bytes);}int CreateIndexBuffer(int bytes){return create(true,bytes);}
 void*lock(int n,bool index){assert(n>=0&&buffers[n].data&&buffers[n].index==index&&!buffers[n].locked);if(fault())return nullptr;buffers[n].locked=true;return buffers[n].data;}
 void*LockVertexBuffer(int n){return lock(n,false);}void*LockIndexBuffer(int n){return lock(n,true);}
 void unlock(int n,bool index){assert(buffers[n].data&&buffers[n].index==index&&buffers[n].locked);buffers[n].locked=false;}
 void UnlockVertexBuffer(int n){unlock(n,false);}void UnlockIndexBuffer(int n){unlock(n,true);}
 void release(int n,bool index){assert(buffers[n].data&&buffers[n].index==index&&!buffers[n].locked);std::free(buffers[n].data);buffers[n].data=nullptr;}
 void ReleaseVertexBuffer(int n){release(n,false);}void ReleaseIndexBuffer(int n){release(n,true);}
 int live()const{int n=0;for(auto&b:buffers){assert(!b.locked);n+=b.data!=nullptr;}return n;}
 ~FakeService(){assert(live()==0);}
};
class GEOM {public:FakeService&srv;explicit GEOM(FakeService&s):srv(s){}
    struct HullOriginal {int type=0,stride=0;std::vector<uint8_t>vertices;std::vector<uint16_t>indices;storm_ship_hull::Broadphase broadphase;};
    struct HullDraw {int vb=-1,ib=-1,vertices=0,triangles=0,stride=0;};
    std::map<int,HullOriginal> hullOriginals;
    std::map<int,std::vector<HullDraw>> hullDraws;
    uint64_t hullOwner=0,hullSignature=0;
    inline static std::atomic<uint64_t> nextGeometryGeneration{0};
    const uint64_t geometryGeneration = ++nextGeometryGeneration;
    uint64_t collisionRevision = 0;
    bool CaptureHullObject(int objectIndex);
    void ReleaseHullDraws(std::map<int,std::vector<HullDraw>>&draws);
    std::vector<storm_ship_hull::Impact> hullCutInputs;
    storm_ship_laser::Mesh laserSurface;
    std::shared_ptr<const storm_ship_laser::Surface>laserVisualSurface;
    std::map<int,float>laserWoodThickness;
    storm_ship_hull::Broadphase laserBroadphase;
    std::vector<std::pair<int,uint32_t>>laserCollisionTriangles;
    std::vector<uint32_t>laserRayCandidates;
    double laserClosedVolume=0;
    bool laserPrepared=false,laserEnclosedFragments=true;
    std::vector<storm_ship_laser::Plane> laserPlanes;
    struct LaserFragment {uint64_t id=0;std::map<int,std::vector<HullDraw>>draws;storm_ship_laser::FragmentPiece physical;};
    struct LaserBatch {uint64_t id=0;std::vector<LaserFragment>fragments;};
    std::unique_ptr<storm_ship_laser::Projection>laserPending;
    std::map<int,std::vector<HullDraw>>laserPendingDraws;
    LaserBatch laserPendingBatch;
    uint64_t laserPendingOwner=0,laserPendingSerial=0;
    storm_ship_laser::FaceIndex laserRetainedSupport;
    std::vector<storm_ship_laser::FaceIndex>laserReleasedSupport;
    std::vector<LaserBatch>laserFragments;
    void ReleaseLaserFragments();
    uint64_t laserOwner=0,laserSerial=0;
    int laserTraceObject=-1,laserTraceTriangle=-1;
    float laserTraceA=0,laserTraceB=0;
    bool UploadLaserMesh(const storm_ship_laser::Mesh&,std::map<int,std::vector<HullDraw>>&);
    bool RebuildLaserSurface(bool upload=true);
    void RebuildLaserCollision();

 uint64_t worldOwner=0;
 bool StageLaserProjection(uint64_t,storm_ship_laser::Projection&&,storm_ship_laser::Result&);
 bool TraceLaserFragment(uint64_t,uint64_t,uint64_t,const storm_ship_hull::Vec3&,const storm_ship_hull::Vec3&,float&,storm_ship_hull::Vec3&) const;
 bool CaptureFragmentInput(uint64_t,uint64_t,uint64_t,const storm_ship_laser::Plane&,unsigned,storm_ship_laser::FragmentInput&) const;
 bool ReplaceLaserFragment(uint64_t,storm_ship_laser::FragmentProjection&&,std::vector<storm_ship_laser::Fragment>&);
 void CommitLaserProjection(uint64_t);void DiscardLaserProjection(uint64_t);
};
void GEOM::ReleaseHullDraws(std::map<int,std::vector<HullDraw>>&draws)
{
    for(auto&entry:draws)for(auto&draw:entry.second){if(draw.vb>=0)srv.ReleaseVertexBuffer(draw.vb);if(draw.ib>=0)srv.ReleaseIndexBuffer(draw.ib);}draws.clear();
}
bool GEOM::UploadLaserMesh(const storm_ship_laser::Mesh&mesh,std::map<int,std::vector<HullDraw>>&draws)
{
    int lockedVertex=-1,lockedIndex=-1;try{for(const auto&entry:mesh){auto&owned=draws[entry.first];owned.reserve(owned.size()+(entry.second.size()+21843)/21844);const auto&source=hullOriginals.at(entry.first);size_t first=0;
        while(first<entry.second.size()){const size_t count=std::min(size_t(21844),entry.second.size()-first);owned.emplace_back();auto&draw=owned.back();draw.stride=source.stride;draw.vertices=int(count*3);draw.triangles=int(count);
            draw.vb=srv.CreateVertexBuffer(source.type,draw.vertices*draw.stride);draw.ib=srv.CreateIndexBuffer(draw.vertices*2);
            auto*v=draw.vb>=0?static_cast<uint8_t*>(srv.LockVertexBuffer(draw.vb)):nullptr;if(v)lockedVertex=draw.vb;auto*i=draw.ib>=0?static_cast<uint16_t*>(srv.LockIndexBuffer(draw.ib)):nullptr;if(i)lockedIndex=draw.ib;
            if(!v||!i){if(v){srv.UnlockVertexBuffer(draw.vb);lockedVertex=-1;}if(i){srv.UnlockIndexBuffer(draw.ib);lockedIndex=-1;}ReleaseHullDraws(draws);return false;}
            for(size_t n=0;n<count;++n)for(unsigned c=0;c<3;++c){size_t index=n*3+c;storm_ship_hull::encode(entry.second[first+n][c],v+index*draw.stride,draw.stride);i[index]=uint16_t(index);}
            srv.UnlockVertexBuffer(draw.vb);lockedVertex=-1;srv.UnlockIndexBuffer(draw.ib);lockedIndex=-1;first+=count;
        }
    }return true;}catch(...){if(lockedVertex>=0)srv.UnlockVertexBuffer(lockedVertex);if(lockedIndex>=0)srv.UnlockIndexBuffer(lockedIndex);ReleaseHullDraws(draws);return false;}
}
bool GEOM::StageLaserProjection(uint64_t owner,storm_ship_laser::Projection&&projection,storm_ship_laser::Result&result)
{
    result={};if(!owner||worldOwner||(laserOwner&&owner!=laserOwner)||(hullOwner&&owner!=hullOwner)||projection.generation!=geometryGeneration||projection.revision!=collisionRevision||(laserPendingOwner&&laserPendingOwner!=owner))return false;
    DiscardLaserProjection(owner);if(!projection.result.changed){try{laserPending=std::make_unique<storm_ship_laser::Projection>(std::move(projection));laserPendingOwner=owner;return true;}catch(...){return false;}}
    if(!projection.kept||projection.result.fragments.size()!=projection.fragmentRender.size()||!storm_ship_laser::valid(projection.plane)||laserPlanes.size()>=storm_hull_damage::MaxSplitPlanes)return false;
    std::map<int,std::vector<HullDraw>>replacement;LaserBatch batch;try{
        laserPlanes.reserve(laserPlanes.size()+1);laserFragments.reserve(laserFragments.size()+1);batch.fragments.resize(projection.fragmentRender.size());auto pending=std::make_unique<storm_ship_laser::Projection>(std::move(projection));
        uint64_t serial=laserSerial;if(serial>UINT64_MAX-batch.fragments.size()-1)return false;batch.id=++serial;pending->result.fragment=batch.id;for(size_t n=0;n<batch.fragments.size();++n){pending->result.fragments[n].fragment=batch.fragments[n].id=++serial;batch.fragments[n].physical=storm_ship_laser::standaloneFragment(pending->fragmentRender[n]);}auto published=pending->result;
        if(!UploadLaserMesh(pending->render,replacement))return false;for(size_t n=0;n<batch.fragments.size();++n)if(!UploadLaserMesh(pending->fragmentRender[n],batch.fragments[n].draws)){for(auto&fragment:batch.fragments)ReleaseHullDraws(fragment.draws);ReleaseHullDraws(replacement);return false;}
        result=std::move(published);laserPending=std::move(pending);laserPendingDraws=std::move(replacement);laserPendingBatch=std::move(batch);laserPendingSerial=serial;laserPendingOwner=owner;return true;
    }catch(...){for(auto&fragment:batch.fragments)ReleaseHullDraws(fragment.draws);ReleaseHullDraws(replacement);return false;}
}
void GEOM::CommitLaserProjection(uint64_t owner)
{
    if(!laserPending||owner!=laserPendingOwner)return;if(laserPending->revision!=collisionRevision||laserPending->generation!=geometryGeneration){DiscardLaserProjection(owner);return;}if(!laserPending->result.changed){DiscardLaserProjection(owner);return;}
    auto projection=std::move(laserPending);ReleaseHullDraws(hullDraws);hullDraws=std::move(laserPendingDraws);laserSurface=std::move(projection->canonical);laserVisualSurface=std::move(projection->kept);laserClosedVolume=projection->keptVolume;laserEnclosedFragments=projection->enclosedFragments;laserPlanes.push_back(std::move(projection->plane));laserOwner=hullOwner=owner;
    laserBroadphase=std::move(projection->collision.broadphase);laserCollisionTriangles=std::move(projection->collision.references);laserRayCandidates.clear();laserTraceObject=laserTraceTriangle=-1;laserRetainedSupport=std::move(projection->retainedSupport);laserReleasedSupport=std::move(projection->releasedSupport);laserSerial=laserPendingSerial;laserFragments.push_back(std::move(laserPendingBatch));laserPendingOwner=laserPendingSerial=0;++collisionRevision;
}
void GEOM::DiscardLaserProjection(uint64_t owner)
{
    if(laserPendingOwner&&owner!=laserPendingOwner)return;ReleaseHullDraws(laserPendingDraws);for(auto&fragment:laserPendingBatch.fragments)ReleaseHullDraws(fragment.draws);laserPendingBatch={};laserPending.reset();laserPendingOwner=laserPendingSerial=0;
}
bool GEOM::TraceLaserFragment(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_hull::Vec3&start,const storm_ship_hull::Vec3&end,float&fraction,storm_ship_hull::Vec3&normal) const
{
    fraction=2;normal={};if(!owner||owner!=laserOwner||generation!=geometryGeneration||!token)return false;
    for(const auto&batch:laserFragments)for(const auto&fragment:batch.fragments)if(fragment.id==token)return storm_ship_laser::traceFragment(fragment.physical,start,end,fraction,normal);return false;
}
bool GEOM::CaptureFragmentInput(uint64_t owner,uint64_t generation,uint64_t token,const storm_ship_laser::Plane&plane,unsigned successorLimit,storm_ship_laser::FragmentInput&input) const
{
    if(!owner||owner!=laserOwner||generation!=geometryGeneration||!token||!successorLimit||!storm_ship_laser::valid(plane))return false;
    size_t active=0;const LaserFragment*found=nullptr;for(const auto&batch:laserFragments)for(const auto&fragment:batch.fragments){++active;if(fragment.id==token)found=&fragment;}
    if(!found||!found->physical.surface||active>storm_hull_damage::MaxFragmentBuffers)return false;
    try{storm_ship_laser::FragmentInput next;next.owner=owner;next.generation=generation;next.token=token;next.successorLimit=std::min(successorLimit,unsigned(storm_hull_damage::MaxFragmentBuffers-active+1));
        next.input.surface=found->physical.surface;next.input.plane=plane;next.input.fragmentLimit=next.successorLimit;next.input.generation=generation;next.input.revision=token;next.input.enclosedFragments=true;input=std::move(next);return true;
    }catch(...){return false;}
}
bool GEOM::ReplaceLaserFragment(uint64_t owner,storm_ship_laser::FragmentProjection&&projection,std::vector<storm_ship_laser::Fragment>&successors)
{
    if(!owner||owner!=laserOwner||projection.owner!=owner||projection.generation!=geometryGeneration||!projection.token||!projection.changed||laserPending)return false;
    size_t active=0,batchIndex=0,fragmentIndex=0;bool found=false;
    for(size_t b=0;b<laserFragments.size();++b)for(size_t f=0;f<laserFragments[b].fragments.size();++f){++active;if(laserFragments[b].fragments[f].id==projection.token){batchIndex=b;fragmentIndex=f;found=true;}}
    if(!found||active-1+projection.successors.size()>storm_hull_damage::MaxFragmentBuffers||projection.successors.size()>UINT64_MAX-laserSerial)return false;
    std::vector<LaserFragment>replacement;std::vector<storm_ship_laser::Fragment>published;
    try{replacement.resize(projection.successors.size());published.reserve(replacement.size());auto&batch=laserFragments[batchIndex];batch.fragments.reserve(batch.fragments.size()-1+replacement.size());
        uint64_t serial=laserSerial;for(size_t n=0;n<replacement.size();++n){auto&piece=projection.successors[n];if(!piece.surface)return false;replacement[n].id=++serial;replacement[n].physical=std::move(piece);auto properties=replacement[n].physical.properties;properties.fragment=replacement[n].id;published.push_back(properties);}
        for(auto&fragment:replacement)if(!UploadLaserMesh(fragment.physical.surface->outer,fragment.draws)){for(auto&prepared:replacement)ReleaseHullDraws(prepared.draws);return false;}
        // No allocation or upload after this boundary: retire exactly this token.
        ReleaseHullDraws(batch.fragments[fragmentIndex].draws);batch.fragments.erase(batch.fragments.begin()+fragmentIndex);for(auto&fragment:replacement)batch.fragments.push_back(std::move(fragment));
        if(batch.fragments.empty())laserFragments.erase(laserFragments.begin()+batchIndex);laserSerial=serial;successors=std::move(published);return true;
    }catch(...){for(auto&fragment:replacement)ReleaseHullDraws(fragment.draws);return false;}
}
using namespace storm_ship_laser;
Triangle triangle(float x){Triangle t{};t[0].position={x,0,0};t[1].position={x+1,0,0};t[2].position={x,1,0};for(auto&v:t)v.normal={0,0,1};return t;}
Mesh mesh(float x,size_t n=1){Mesh m;m[0].assign(n,triangle(x));return m;}
void fillIndex(FaceIndex&i,float x){i.references={{0,uint32_t(x)}};i.bounds={{x,0,0},{x+1,1,1}};i.broadphase.build(std::vector<Bounds>{i.bounds});}
Projection projection(GEOM&g){Projection p;p.generation=g.geometryGeneration;p.revision=g.collisionRevision;p.plane.normal={1,0,0};p.plane.distance=7;p.plane.beams.push_back({});p.plane.beams.clear();
 auto surface=std::make_shared<Surface>();surface->outer=mesh(7);p.kept=surface;p.canonical=mesh(7);p.render=mesh(7,21845);p.render[1]={triangle(8)};p.fragmentRender={mesh(9),mesh(10)};p.result.changed=true;p.result.fragments.resize(2);p.result.seam.resize(1);p.result.openings.resize(1);p.result.removedFraction=.2f;p.keptVolume=17;p.enclosedFragments=false;fillIndex(p.collision,7);fillIndex(p.retainedSupport,8);p.releasedSupport.resize(2);fillIndex(p.releasedSupport[0],9);fillIndex(p.releasedSupport[1],10);return p;}
void seed(GEOM&g){g.hullOriginals[0].stride=60;g.hullOriginals[1].stride=60;g.laserOwner=g.hullOwner=42;g.collisionRevision=3;g.laserSerial=90;g.laserClosedVolume=24;g.laserPrepared=true;g.laserSurface=mesh(1);auto s=std::make_shared<Surface>();s->outer=mesh(1);g.laserVisualSurface=s;g.laserPlanes.resize(2);g.laserPlanes[0].normal={1,0,0};g.laserPlanes[1].normal={0,1,0};g.laserPlanes[1].beams.resize(1);g.laserFragments.resize(1);g.laserFragments[0].id=80;g.laserFragments[0].fragments.resize(1);g.laserFragments[0].fragments[0].id=81;assert(g.UploadLaserMesh(mesh(2),g.hullDraws));assert(g.UploadLaserMesh(mesh(3),g.laserFragments[0].fragments[0].draws));g.laserCollisionTriangles={{0,10}};g.laserBroadphase.build(std::vector<Bounds>{{{1,0,0},{2,1,1}}});g.laserRayCandidates={5,6};g.laserTraceObject=2;g.laserTraceTriangle=10;g.laserTraceA=.2f;g.laserTraceB=.3f;fillIndex(g.laserRetainedSupport,1);g.laserReleasedSupport.resize(1);fillIndex(g.laserReleasedSupport[0],2);g.srv.calls=0;}
void outMesh(std::ostream&o,const Mesh&m){for(auto&e:m){o<<e.first<<':'<<e.second.size()<<'/';for(auto&t:e.second)for(auto&v:t)o<<v.position.x<<','<<v.position.y<<','<<v.position.z<<';';}}
void outDraws(std::ostream&o,const std::map<int,std::vector<GEOM::HullDraw>>&d){for(auto&e:d){o<<e.first<<':';for(auto&x:e.second)o<<x.vb<<','<<x.ib<<','<<x.vertices<<','<<x.triangles<<','<<x.stride<<';';}}
void outIndex(std::ostream&o,const FaceIndex&i){for(auto r:i.references)o<<r.first<<','<<r.second<<';';std::vector<uint32_t>hits;i.broadphase.queryBounds({{-100,-100,-100},{100,100,100}},hits);for(auto h:hits)o<<h<<',';o<<i.bounds.lo.x<<'/'<<i.bounds.hi.x;}
std::string state(GEOM&g){std::ostringstream o;o<<g.laserOwner<<'/'<<g.hullOwner<<'/'<<g.collisionRevision<<'/'<<g.laserSerial<<'/'<<g.laserClosedVolume<<'/'<<g.laserPrepared<<'/'<<g.laserEnclosedFragments<<'/'<<g.laserVisualSurface.get()<<'/'<<g.laserTraceObject<<'/'<<g.laserTraceTriangle<<'/'<<g.laserTraceA<<'/'<<g.laserTraceB;outDraws(o,g.hullDraws);outMesh(o,g.laserSurface);outMesh(o,g.laserVisualSurface->outer);for(auto&p:g.laserPlanes)o<<p.normal.x<<p.normal.y<<p.distance<<p.beams.size()<<';';for(auto&b:g.laserFragments){o<<b.id<<':';for(auto&f:b.fragments){o<<f.id<<':';outDraws(o,f.draws);}}for(auto r:g.laserCollisionTriangles)o<<r.first<<','<<r.second;for(auto r:g.laserRayCandidates)o<<r<<',';std::vector<uint32_t>hits;g.laserBroadphase.queryBounds({{-100,-100,-100},{100,100,100}},hits);for(auto h:hits)o<<h<<',';outIndex(o,g.laserRetainedSupport);for(auto&i:g.laserReleasedSupport)outIndex(o,i);return o.str();}
void cleanup(GEOM&g){g.DiscardLaserProjection(g.laserPendingOwner);g.ReleaseHullDraws(g.hullDraws);for(auto&b:g.laserFragments)for(auto&f:b.fragments)g.ReleaseHullDraws(f.draws);assert(g.srv.live()==0);}
void noPending(GEOM&g){assert(!g.laserPending&&g.laserPendingDraws.empty()&&g.laserPendingBatch.fragments.empty()&&g.laserPendingOwner==0&&g.laserPendingSerial==0);}
int stage_main(){int failureCases=0;int stageCalls=0;
 {FakeService s;GEOM g(s);seed(g);auto p=projection(g);auto before=state(g);Result r;assert(g.StageLaserProjection(42,std::move(p),r));stageCalls=s.calls;assert(stageCalls==20&&state(g)==before&&s.live()==14);auto*pending=g.laserPending.get();auto pendingSerial=g.laserPendingSerial;auto pendingResult=r.fragment;
  g.DiscardLaserProjection(43);g.CommitLaserProjection(43);assert(g.laserPending.get()==pending&&g.laserPendingSerial==pendingSerial&&s.live()==14&&state(g)==before);
  auto wrong=projection(g);Result rejected;assert(!g.StageLaserProjection(43,std::move(wrong),rejected)&&g.laserPending.get()==pending&&state(g)==before&&s.live()==14);
  long count=allocCount;allocFail=0;g.CommitLaserProjection(42);allocFail=-1;assert(allocCount==count);assert(!g.laserPending&&g.laserPendingOwner==0&&g.laserPendingSerial==0&&g.collisionRevision==4&&g.laserPlanes.size()==3&&g.laserFragments.size()==2&&g.laserSerial==93&&g.laserFragments.back().id==91&&r.fragment==91&&r.fragments[0].fragment==92&&r.fragments[1].fragment==93&&pendingResult==91&&g.laserSurface[0][0][0].position.x==7&&g.laserCollisionTriangles[0].second==7&&g.laserRayCandidates.empty()&&g.laserTraceObject==-1&&s.live()==12);
  auto committed=state(g);g.CommitLaserProjection(42);assert(state(g)==committed&&s.live()==12);cleanup(g);}
 for(bool throws:{false,true})for(int fail=1;fail<=stageCalls;++fail){FakeService s;GEOM g(s);seed(g);auto p=projection(g);auto before=state(g);Result r;r.changed=true;s.failAt=fail;s.throws=throws;assert(!g.StageLaserProjection(42,std::move(p),r));assert(!r.changed&&state(g)==before&&s.live()==4);noPending(g);cleanup(g);++failureCases;}
 for(int mode=0;mode<6;++mode){FakeService s;GEOM g(s);seed(g);auto p=projection(g);auto before=state(g);Result r;if(mode==0)p.generation++;if(mode==1)p.revision++;if(mode==2)g.worldOwner=9;if(mode==3)p.kept.reset();if(mode==4)p.fragmentRender.pop_back();if(mode==5)p.plane.normal={0,0,0};assert(!g.StageLaserProjection(42,std::move(p),r)&&state(g)==before&&s.calls==0&&s.live()==4);noPending(g);cleanup(g);}
 for(int mode=0;mode<2;++mode){FakeService s;GEOM g(s);seed(g);auto p=projection(g);Result r;assert(g.StageLaserProjection(42,std::move(p),r));if(mode==0)++g.collisionRevision;else ++g.laserPending->generation;auto before=state(g);g.CommitLaserProjection(42);assert(state(g)==before&&s.live()==4);noPending(g);cleanup(g);}
 {FakeService s;GEOM g(s);seed(g);Projection p;p.generation=g.geometryGeneration;p.revision=g.collisionRevision;auto before=state(g);Result r;assert(g.StageLaserProjection(42,std::move(p),r)&&state(g)==before&&s.calls==0);long count=allocCount;allocFail=0;g.CommitLaserProjection(42);allocFail=-1;assert(allocCount==count&&state(g)==before&&s.live()==4);noPending(g);cleanup(g);}
 {FakeService s;GEOM g(s);seed(g);Projection p;p.generation=g.geometryGeneration;p.revision=g.collisionRevision;auto before=state(g);Result r;allocFail=0;bool ok=g.StageLaserProjection(42,std::move(p),r);allocFail=-1;assert(!ok&&state(g)==before&&s.calls==0&&s.live()==4);noPending(g);cleanup(g);}
 {FakeService s;GEOM g(s);seed(g);auto p=projection(g);auto before=state(g);Result r;assert(g.StageLaserProjection(42,std::move(p),r));g.DiscardLaserProjection(42);assert(state(g)==before&&s.live()==4);noPending(g);cleanup(g);}
 int allocationCases=0;bool reachedSuccess=false;
 for(int fail=0;fail<500;++fail){FakeService s;GEOM g(s);seed(g);auto p=projection(g);auto before=state(g);Result r;allocFail=fail;bool ok=g.StageLaserProjection(42,std::move(p),r);bool injected=allocFail==-1;allocFail=-1;assert(state(g)==before);if(ok){assert(!injected);g.DiscardLaserProjection(42);assert(s.live()==4);reachedSuccess=true;cleanup(g);break;}assert(injected&&s.live()==4);noPending(g);cleanup(g);++allocationCases;}
 assert(reachedSuccess);std::printf("PASS actual five GEOM bodies: %d create/lock return/throw cases; %d allocation failure positions; owner, stale, no-op, publish-once, discard; Commit allocations=0; live handles at exit=0.\n",failureCases,allocationCases);return 0;
}


Vertex vertex(Vec3 p){Vertex v;v.position=p;return v;}
Mesh box(float radius=1,bool reverse=false){Mesh m;Vec3 p[8]={{-radius,-radius,-radius},{radius,-radius,-radius},{radius,radius,-radius},{-radius,radius,-radius},{-radius,-radius,radius},{radius,-radius,radius},{radius,radius,radius},{-radius,radius,radius}};int f[6][4]={{0,3,2,1},{4,5,6,7},{0,4,7,3},{1,2,6,5},{0,1,5,4},{3,7,6,2}};for(auto&q:f)for(auto ids:{std::array<int,3>{q[0],q[1],q[2]},std::array<int,3>{q[0],q[2],q[3]}}){Triangle t={vertex(p[ids[0]]),vertex(p[ids[1]]),vertex(p[ids[2]])};if(reverse)std::swap(t[1],t[2]);for(auto&v:t)v.normal=storm_ship_hull::normalized(storm_ship_hull::cross(t[1].position-t[0].position,t[2].position-t[0].position));m[0].push_back(t);}return m;}
Plane bore(float x=0,float y=0){Plane p{{0,0,-1},0};BeamSample b;b.start={x,y,1.01f};b.direction={0,0,-1};b.radius=.13f;b.depth=.4f;p.beams={b};return p;}
FragmentProjection secondary(GEOM&g){FragmentInput in;assert(g.CaptureFragmentInput(42,g.geometryGeneration,81,bore(),8,in));FragmentProjection p;assert(prepareFragment(in,p)&&p.changed);return p;}
void seedPhysical(GEOM&g){seed(g);auto shell=box();appendMesh(shell,box(.8f,true));g.laserFragments[0].fragments[0].physical=standaloneFragment(std::move(shell));}
void assertOld(GEOM&g,const std::string&before,int live){assert(state(g)==before&&g.srv.live()==live);float fraction;Vec3 normal;assert(g.TraceLaserFragment(42,g.geometryGeneration,81,{0,0,2},{0,0,-2},fraction,normal)&&std::abs(fraction-.25f)<1.e-6f&&normal.z>.99f);}
int main(){stage_main();int gpuCases=0,allocationCases=0;int gpuCalls=0;
 {FakeService s;GEOM g(s);seedPhysical(g);float f;Vec3 n;
  assert(!g.TraceLaserFragment(43,g.geometryGeneration,81,{0,0,2},{0,0,-2},f,n));assert(!g.TraceLaserFragment(42,g.geometryGeneration+1,81,{0,0,2},{0,0,-2},f,n));assert(!g.TraceLaserFragment(42,g.geometryGeneration,80,{0,0,2},{0,0,-2},f,n));
  assert(!g.TraceLaserFragment(42,g.geometryGeneration,81,{0,0,.5f},{0,0,-.5f},f,n)); // real hollow cavity
  auto p=secondary(g);assert(p.successors.size()==2);double volume=0;for(auto&piece:p.successors){assert(!piece.surface->timber&&!piece.surface->original&&piece.surface->cutters.empty()&&piece.surface->material->size()==1);volume+=closedCutVolume(piece.surface->outer);}assert(std::abs(volume-(8-8*.8*.8*.8))<.0001);
  const auto physical=g.laserFragments[0].fragments[0].physical.surface;const auto hull=g.laserVisualSurface;const auto parentBefore=state(g);std::vector<Fragment> ids;assert(g.ReplaceLaserFragment(42,std::move(p),ids)&&ids.size()==2);gpuCalls=s.calls;assert(ids[0].fragment>90&&ids[1].fragment>ids[0].fragment);assert(!g.TraceLaserFragment(42,g.geometryGeneration,81,{0,0,2},{0,0,-2},f,n));assert(g.laserVisualSurface==hull&&g.laserCollisionTriangles[0].second==10&&physical->outer.at(0).size()==24);
  assert(g.TraceLaserFragment(42,g.geometryGeneration,ids[0].fragment,{0,0,2},{0,0,-2},f,n)&&f>.65f); // aperture sees far shell, not regenerated front
  assert(!g.TraceLaserFragment(42,g.geometryGeneration,ids[0].fragment,{0,0,.5f},{0,0,-.5f},f,n));
  FragmentInput in;assert(g.CaptureFragmentInput(42,g.geometryGeneration,ids[0].fragment,bore(.5f),8,in));FragmentProjection next;assert(prepareFragment(in,next)&&next.changed);double nextVolume=0;for(auto&piece:next.successors)nextVolume+=closedCutVolume(piece.surface->outer);assert(std::abs(nextVolume-closedCutVolume(in.input.surface->outer))<.0001);
  assert(g.TraceLaserFragment(42,g.geometryGeneration,ids[0].fragment,{.5f,0,2},{.5f,0,-2},f,n)&&std::abs(f-.25f)<1.e-6f);
  const uint64_t other=ids[1].fragment;auto keepOther=g.laserFragments[0].fragments[1].physical.surface;assert(g.ReplaceLaserFragment(42,std::move(next),ids));assert(g.laserFragments[0].fragments[0].id==other&&g.laserFragments[0].fragments[0].physical.surface==keepOther);
  // Parent cuts may advance revision without invalidating immutable fragment tokens.
  auto primary=projection(g);Result result;assert(g.StageLaserProjection(42,std::move(primary),result));long allocations=allocCount;allocFail=0;g.CommitLaserProjection(42);allocFail=-1;assert(allocCount==allocations);assert(g.TraceLaserFragment(42,g.geometryGeneration,other,{0,0,2},{0,0,-2},f,n));
  // Stale owner, generation and retired token cannot mutate any state.
  auto unchanged=state(g);for(int mode=0;mode<3;++mode){FragmentProjection bad;bad.changed=true;bad.owner=42;bad.generation=g.geometryGeneration;bad.token=other;if(mode==0)bad.owner=43;if(mode==1)++bad.generation;if(mode==2)bad.token=81;assert(!g.ReplaceLaserFragment(42,std::move(bad),ids)&&state(g)==unchanged);}
  // Explicit empty successor outcome retires once, allocates/uploads nothing.
  FragmentProjection empty;empty.owner=42;empty.generation=g.geometryGeneration;empty.token=other;empty.changed=true;auto again=empty;int calls=s.calls;allocations=allocCount;allocFail=0;assert(g.ReplaceLaserFragment(42,std::move(empty),ids));allocFail=-1;assert(ids.empty()&&s.calls==calls&&allocCount==allocations);assert(!g.ReplaceLaserFragment(42,std::move(again),ids));cleanup(g);
 }
 for(bool throws:{false,true})for(int fail=1;fail<=gpuCalls;++fail){FakeService s;GEOM g(s);seedPhysical(g);auto p=secondary(g);auto before=state(g);auto old=g.laserFragments[0].fragments[0].physical.surface;std::vector<Fragment> ids;s.failAt=fail;s.throws=throws;assert(!g.ReplaceLaserFragment(42,std::move(p),ids));s.failAt=-1;assertOld(g,before,4);assert(g.laserFragments[0].fragments[0].physical.surface==old);cleanup(g);++gpuCases;}
 bool reached=false;for(int fail=0;fail<200;++fail){FakeService s;GEOM g(s);seedPhysical(g);auto p=secondary(g);auto before=state(g);auto old=g.laserFragments[0].fragments[0].physical.surface;std::vector<Fragment> ids;allocFail=fail;bool ok=g.ReplaceLaserFragment(42,std::move(p),ids);bool injected=allocFail==-1;allocFail=-1;if(ok){assert(!injected);reached=true;cleanup(g);break;}assert(injected);assertOld(g,before,4);assert(g.laserFragments[0].fragments[0].physical.surface==old);cleanup(g);++allocationCases;}assert(reached);
 // Capacity credits precisely the old token. A one-for-one replacement works
 // at the buffer ceiling; two successors are rejected before upload.
 {FakeService s;GEOM g(s);seedPhysical(g);auto&batch=g.laserFragments[0];while(batch.fragments.size()<storm_hull_damage::MaxFragmentBuffers){GEOM::LaserFragment f;f.id=1000+batch.fragments.size();f.physical=batch.fragments[0].physical;batch.fragments.push_back(std::move(f));}FragmentInput in;assert(g.CaptureFragmentInput(42,g.geometryGeneration,81,bore(),8,in)&&in.successorLimit==1);FragmentProjection p;p.owner=42;p.generation=g.geometryGeneration;p.token=81;p.changed=true;p.successors.push_back(batch.fragments[0].physical);std::vector<Fragment>ids;assert(g.ReplaceLaserFragment(42,std::move(p),ids)&&ids.size()==1);cleanup(g);}
 // Actual born timber must split successfully on an overlapping equal-radius
 // dose, conserve its closed physical material and retain its hollow cavity.
 {auto wood=timberSurface(box(),{{0,.1f}});auto cut=splitSurface(wood,bore(),{{0,.1f}});assert(cut.complete);auto piece=standaloneFragment(renderMesh(cut.detached));float f;Vec3 n;assert(piece.properties.surfaceArea>0);assert(!traceFragment(piece,{0,0,.5f},{0,0,-.5f},f,n));FragmentInput in;in.owner=42;in.generation=1;in.token=1;in.successorLimit=8;in.input.surface=piece.surface;in.input.fragmentLimit=8;in.input.plane=bore(.05f);FragmentProjection p;assert(prepareFragment(in,p)&&p.changed&&p.successors.size()==2);double total=0;for(const auto&next:p.successors){const auto volume=closedCutVolume(next.surface->outer);assert(volume>0);total+=volume;}assert(std::abs(total-closedCutVolume(piece.surface->outer))<1.e-8);assert(std::abs(closedCutVolume(piece.surface->outer)-.003828658575)<1.e-9);}
 // Complete actual GEOM transaction on a physically uploaded born timber token.
 {FakeService s;GEOM g(s);seed(g);auto wood=timberSurface(box(),{{0,.1f}});auto birth=splitSurface(wood,bore(),{{0,.1f}});assert(birth.complete);auto born=standaloneFragment(renderMesh(birth.detached));const double birthVolume=closedCutVolume(born.surface->outer);assert(birthVolume>0);
  auto&old=g.laserFragments[0].fragments[0];g.ReleaseHullDraws(old.draws);old.physical=std::move(born);assert(g.UploadLaserMesh(old.physical.surface->outer,old.draws));GEOM::LaserFragment sibling;sibling.id=82;sibling.physical=standaloneFragment(mesh(5));assert(g.UploadLaserMesh(sibling.physical.surface->outer,sibling.draws));g.laserFragments[0].fragments.push_back(std::move(sibling));
  const auto siblingPhysical=g.laserFragments[0].fragments[1].physical.surface;const auto hullPhysical=g.laserVisualSurface;const auto oldPhysical=g.laserFragments[0].fragments[0].physical.surface;FragmentInput in;assert(g.CaptureFragmentInput(42,g.geometryGeneration,81,bore(.05f),8,in));FragmentProjection first;assert(prepareFragment(in,first)&&first.changed&&first.successors.size()==2);double firstVolume=0;for(const auto&piece:first.successors){const double v=closedCutVolume(piece.surface->outer);assert(v>0);firstVolume+=v;}assert(std::abs(firstVolume-birthVolume)<1.e-8);
  const auto before=state(g);const auto live=s.live();s.calls=0;s.failAt=1;std::vector<Fragment>ids;assert(!g.ReplaceLaserFragment(42,std::move(first),ids));s.failAt=-1;float fraction;Vec3 normal;assert(state(g)==before&&s.live()==live&&g.laserFragments[0].fragments[0].physical.surface==oldPhysical);assert(g.TraceLaserFragment(42,g.geometryGeneration,81,{0,0,2},{0,0,.5f},fraction,normal)&&std::abs(fraction-2.f/3)<1.e-6f);
  assert(g.CaptureFragmentInput(42,g.geometryGeneration,81,bore(.05f),8,in));assert(prepareFragment(in,first)&&first.changed);assert(g.ReplaceLaserFragment(42,std::move(first),ids)&&ids.size()==2);const auto retained=ids[0].fragment,extracted=ids[1].fragment;assert(!g.TraceLaserFragment(42,g.geometryGeneration,81,{0,0,2},{0,0,.5f},fraction,normal));
  assert(g.CaptureFragmentInput(42,g.geometryGeneration,retained,bore(-.05f),8,in));const double priorVolume=closedCutVolume(in.input.surface->outer);FragmentProjection repeat;assert(prepareFragment(in,repeat)&&repeat.changed);double repeatVolume=0;for(const auto&piece:repeat.successors){const double v=closedCutVolume(piece.surface->outer);assert(v>0);repeatVolume+=v;}assert(std::abs(repeatVolume-priorVolume)<1.e-8);assert(g.ReplaceLaserFragment(42,std::move(repeat),ids));
  auto parent=projection(g);Result parentResult;assert(g.StageLaserProjection(42,std::move(parent),parentResult));g.CommitLaserProjection(42);bool foundSibling=false,foundExtracted=false;for(const auto&b:g.laserFragments)for(const auto&f:b.fragments){if(f.id==82){foundSibling=true;assert(f.physical.surface==siblingPhysical);}if(f.id==extracted){foundExtracted=true;assert(closedCutVolume(f.physical.surface->outer)>0);}}assert(foundSibling&&foundExtracted&&g.laserVisualSurface!=hullPhysical);
  std::printf("PASS actual thin timber Capture->prepare->Replace: birth=%.12f first=%.12f repeat=%.12f; GPU fault retains original exact trace; sibling and previous extracted token survive parent hull commit.\n",birthVolume,firstVolume,repeatVolume);cleanup(g);
 }
 std::printf("PASS actual GEOM secondary bodies: thin timber overlapping recut succeeds with two closed conserved successors; hollow sequential recuts/void/props, stale owner-generation-token, exact moved-local hit, parent cut preserves live pieces, GPU return+throw failures=%d, allocation failures=%d, empty retires once with allocations=0, capacity credits retired token.\n",gpuCases,allocationCases);return 0;
}
