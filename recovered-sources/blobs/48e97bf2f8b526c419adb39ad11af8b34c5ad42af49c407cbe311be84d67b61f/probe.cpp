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
    struct LaserFragment {uint64_t id=0;std::map<int,std::vector<HullDraw>>draws;};
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
        uint64_t serial=laserSerial;batch.id=++serial;pending->result.fragment=batch.id;for(size_t n=0;n<batch.fragments.size();++n)pending->result.fragments[n].fragment=batch.fragments[n].id=++serial;auto published=pending->result;
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
int main(){int failureCases=0;int stageCalls=0;
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
 assert(reachedSuccess);std::printf("PASS actual five GEOM bodies: %d create/lock return/throw cases; %d allocation failure positions; owner, stale, no-op, publish-once, discard; Commit allocations=0; live handles at exit=0.\n",failureCases,allocationCases);
}
