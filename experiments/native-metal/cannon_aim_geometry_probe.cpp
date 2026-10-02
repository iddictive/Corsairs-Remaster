// Standalone regression probe; no engine initialization or external framework.
// Compile with C++20; include shared_headers/include and engine math/include.
// Run the generated math_probe executable. No engine libraries are needed.
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include "shared/sea_ai/manual_aim_volume_bridge.hpp"
#include <cstddef>
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

static_assert(sizeof(storm::sea_ai::manual_aim::AimVolumeSection)==32);
static_assert(alignof(storm::sea_ai::manual_aim::AimVolumeSection)==16);
static_assert(offsetof(storm::sea_ai::manual_aim::AimVolumeSection,progress)==12);
static_assert(offsetof(storm::sea_ai::manual_aim::AimVolumeSection,firstPlane)==16);
static_assert(offsetof(storm::sea_ai::manual_aim::AimVolumeSection,halfWidth)==24);
static_assert(sizeof(storm::sea_ai::manual_aim::AimVolumePlane)==16);
static_assert(alignof(storm::sea_ai::manual_aim::AimVolumePlane)==16);
static_assert(offsetof(storm::sea_ai::manual_aim::AimVolumePlane,offset1)==12);

namespace {
int checks=0,failures=0;
void check(bool ok,std::string_view name) {
    ++checks;
    if(!ok) {++failures;std::cerr<<"FAIL: "<<name<<'\n';}
}
bool near(double a,double b,double tolerance=1e-3) {
    return std::isfinite(a)&&std::isfinite(b)&&std::abs(a-b)<=tolerance;
}
bool near(CVECTOR a,CVECTOR b,double tolerance=1e-3) {
    return near(a.x,b.x,tolerance)&&near(a.y,b.y,tolerance)&&near(a.z,b.z,tolerance);
}
double lowAngle(double range,double height,double speed,double g=9.81) {
    const double b=g*height-speed*speed,c=range*range+height*height;
    const double t2=2*c/(-b+std::sqrt(b*b-g*g*c));
    return std::atan2(height+.5*g*t2,range);
}
double targetTime(double range,double height,double speed,double g=9.81) {
    const double b=g*height-speed*speed,c=range*range+height*height;
    return std::sqrt(2*c/(-b+std::sqrt(b*b-g*g*c)));
}
bool inside(const std::vector<manual_aim::Point>& h,manual_aim::Point p,float eps=2e-4f) {
    for(size_t i=0;i<h.size();++i)
        if(manual_aim::cross(h[i],h[(i+1)%h.size()],p)<-eps)return false;
    return true;
}
bool onBoundary(const std::vector<manual_aim::Point>& h,manual_aim::Point p,float eps=2e-4f) {
    for(size_t i=0;i<h.size();++i)
        if(std::abs(manual_aim::cross(h[i],h[(i+1)%h.size()],p))<eps)return true;
    return false;
}
}
int main() {
    constexpr float g=9.81f,pi=std::numbers::pi_v<float>;
    // Downward deck-camera regression: a camera ray is not the gun's range at
    // the same pitch. This nearby surface is reachable despite the old cap.
    const float pitch=-10.f*pi/180.f;
    const float cameraRay=18.f/-std::sin(pitch);
    const float lowCap=80.f*std::cos(pitch)*(80.f*std::sin(pitch)+std::sqrt(std::pow(80.f*std::sin(pitch),2.f)+2.f*g*4.f))/g;
    check(near(cameraRay,103.657869,.001),"deck camera sea pick distance");
    check(near(lowCap,20.754637,.001)&&cameraRay>lowCap,"old pitch-derived ray cap reproduced");
    check(manual_aim::reachable(CVECTOR(0.f,4.f,0.f),CVECTOR(0.f,0.f,cameraRay*std::cos(pitch)),80.f,g),"downward deck target is reachable");
    // Nominal impact is preserved by the RawAng/HeightMultiply warp.
    for(const auto data:std::array<std::array<float,4>,4>{{
          {100,-4,4,80},{100,26,4,80},{140,-8,8,40},{650,-4,4,80}}}) {
        const auto [r,h,y,v]=data;
        const CVECTOR from(0.f,y,0.f),to(0.f,y+h,r);
        check(manual_aim::reachable(from,to,v,g),"nominal reachable");
        const float angle=float(lowAngle(r,h,v,g)),t=float(targetTime(r,h,v,g));
        for(float scale:{.4f,.65f,1.f}) {
            const auto curve=manual_aim::trajectory(from,to,v,angle,0.f,scale,g);
            check(near(curve.at(t),to),"warped nominal intercept");
            const float waterTime=curve.timeToHeight(-3.f);
            check(std::isfinite(waterTime)&&waterTime>0.f&&near(curve.at(waterTime).y,-3.f),
                  "warped negative-height termination");
        }
    }
    const CVECTOR muzzle(0.f,4.f,0.f),target(0.f,0.f,100.f);
    check(!manual_aim::reachable(muzzle,CVECTOR(0.f,0.f,660.f),80.f,g),"660m unreachable");
    check(!manual_aim::reachable(muzzle,CVECTOR(0.f,0.f,1000.f),80.f,g),"1000m unreachable");
    // Accuracy=.6; energy corners have actual distinct water impacts.
    for(int sign:{-1,1}) {
        const float speed=80.f+sign*.2f*pi;
        const float angle=float(lowAngle(100,-4,80,g))+sign*pi*.025f;
        const auto curve=manual_aim::trajectory(muzzle,target,speed,angle,0.f,.4f,g);
        const float t=curve.timeToHeight(0.f);
        const double expected=sign<0?65.4613388888167:156.451196706681;
        check(near(curve.at(t).z,expected),"jitter correct water range");
        check(near(curve.at(t).y,0.f),"jitter water height");
        check(t>0.f&&near(curve.tangent(t),curve.velocity+curve.acceleration*t),"trajectory tangent");
    }
    // Mirroring X and azimuth leaves identical Y/Z on port and starboard.
    for(float angle:{-.2f,.1f,.7f}) for(float scale:{.4f,.65f}) {
        const auto a=manual_aim::trajectory(CVECTOR(5.f,4.f,-2.f),CVECTOR(110.f,0.f,40.f),80.f,angle,.06f,scale,g);
        const auto b=manual_aim::trajectory(CVECTOR(-5.f,4.f,-2.f),CVECTOR(-110.f,0.f,40.f),80.f,angle,-.06f,scale,g);
        const float ta=a.timeToHeight(-3.f),tb=b.timeToHeight(-3.f);
        check(near(ta,tb),"mirror lifetime");
        for(float u:{0.f,.25f,.5f,1.f}) {
            const auto p=a.at(ta*u),q=b.at(tb*u);
            check(near(CVECTOR(-p.x,p.y,p.z),q,2e-3),"mirror trajectory");
        }
    }
    // Independent expanded Rotate/HeightMultiply/Rotate-back engine oracle.
    for(float scale:{.4f,.65f,1.f}) for(float angle:{-.2f,.1f,.7f})
        for(float yawDelta:{-.05f,0.f,.05f}) for(float t:{.1f,1.f,6.f}) {
            const CVECTOR from(5.f,4.f,-2.f),to(110.f,30.f,40.f);
            const auto curve=manual_aim::trajectory(from,to,80.f,angle,yawDelta,scale,g);
            const CVECTOR d=to-from;
            const float raw=std::atan2(d.y,std::sqrt(d.x*d.x+d.z*d.z));
            const float sr=std::sin(raw),cr=std::cos(raw);
            const float x=80.f*t*std::cos(angle),y=80.f*t*std::sin(angle)-.5f*g*t*t;
            const float px=x*cr+y*sr,py=(-x*sr+y*cr)*scale;
            const float xx=px*cr-py*sr,yy=px*sr+py*cr;
            const float yaw=std::atan2(d.x,d.z)+yawDelta;
            const CVECTOR expected=from+CVECTOR(xx*std::sin(yaw),yy,xx*std::cos(yaw));
            check(near(curve.at(t),expected,1e-3),"independent engine warp oracle");
        }
    // Stable negative-v root avoids catastrophic cancellation.
    const manual_aim::Trajectory fastDown{{0.f,1.f,0.f},{0.f,-1e6f,0.f},{0.f,-g,0.f}};
    check(near(fastDown.timeToHeight(0.f),1e-6,1e-12),"stable downward root");
    check(near(fastDown.at(fastDown.timeToHeight(0.f)).y,0.f,1e-6),"downward root endpoint");
    const manual_aim::Trajectory fromWater{{0.f,0.f,0.f},{0.f,5.f,0.f},{0.f,-g,0.f}};
    check(near(fromWater.timeToHeight(0.f),10.f/g),"water-origin upward root");
    const manual_aim::Trajectory belowWater{{0.f,-1.f,0.f},{0.f,5.f,0.f},{0.f,-g,0.f}};
    check(belowWater.timeToHeight(0.f)==0.f,"already below plane is terminated");
    // Degenerate reachable inputs must fail closed.
    const float nan=std::numeric_limits<float>::quiet_NaN(),inf=std::numeric_limits<float>::infinity();
    check(!manual_aim::reachable(muzzle,muzzle,80.f,g),"zero displacement rejected");
    check(!manual_aim::reachable(muzzle,CVECTOR(0.f,40.f,0.f),80.f,g),"vertical displacement rejected");
    check(!manual_aim::reachable(muzzle,target,0.f,g),"zero speed rejected");
    check(!manual_aim::reachable(muzzle,target,80.f,0.f),"zero gravity rejected");
    check(!manual_aim::reachable(muzzle,CVECTOR(nan,0.f,10.f),80.f,g),"NaN coordinate rejected");
    check(!manual_aim::reachable(muzzle,target,nan,g),"NaN speed rejected");
    check(!manual_aim::reachable(muzzle,target,inf,g),"infinite speed rejected");
    // Convex hull order is independent of locator order and interior points.
    std::vector<manual_aim::Point> cloud{{-3,-2},{1,-3},{4,0},{2,3},{-2,2},{0,0},{1,1},{-3,-2}};
    const auto h=manual_aim::hull(cloud);
    check(h.size()==5,"hull removes duplicate and interior points");
    std::reverse(cloud.begin(),cloud.end());
    const auto reversed=manual_aim::hull(cloud);
    check(h.size()==reversed.size(),"hull locator order size");
    for(size_t i=0;i<h.size();++i)
        check(h[i].x==reversed[i].x&&h[i].y==reversed[i].y,"hull locator order vertices");
    std::vector<manual_aim::Point> mirrorCloud;
    for(auto p:cloud)mirrorCloud.push_back({-p.x,p.y});
    const auto mirror=manual_aim::hull(mirrorCloud);
    for(int i=0;i<48;++i) {
        const float a=2*pi*i/48;
        const auto p=manual_aim::boundary(h,{0,0},a);
        const auto q=manual_aim::boundary(mirror,{0,0},pi-a);
        check(inside(h,p)&&onBoundary(h,p),"angular contour inside and on hull");
        check(near(p.x,-q.x,2e-4)&&near(p.y,q.y,2e-4),"angular contour mirror");
        check(near(p.x*std::sin(a)-p.y*std::cos(a),0.f,2e-4),"angular contour stays on ray");
    }
    check(manual_aim::hull({}).empty(),"empty hull");
    check(manual_aim::hull({{1,1},{1,1}}).size()==1,"single-point hull");
    check(manual_aim::hull({{-1,0},{0,0},{1,0}}).size()==2,"collinear hull");
    const auto empty=manual_aim::boundary({}, {0,0},0.f);
    check(empty.x==0&&empty.y==0,"empty boundary finite center");
    const auto thin=manual_aim::boundary(manual_aim::hull({{-1,0},{0,0},{1,0}}),{0,0},0.f);
    check(near(thin.x,1.f)&&near(thin.y,0.f),"collinear boundary retains endpoint");
    const auto thinReverse=manual_aim::boundary(manual_aim::hull({{-1,0},{0,0},{1,0}}),{0,0},pi);
    check(near(thinReverse.x,-1.f)&&near(thinReverse.y,0.f),"collinear reverse boundary retains endpoint");
    // Slab-test AABB cases, including touching, direction reversal, and points.
    const CVECTOR lo(-1.f),hi(1.f);
    const auto box=[&](CVECTOR a,CVECTOR b){return manual_aim::segmentBox(a,b,lo,hi);};
    check(box(CVECTOR(-2.f,0.f,0.f),CVECTOR(2.f,0.f,0.f)),"segment crossing box");
    check(box(CVECTOR(2.f,0.f,0.f),CVECTOR(-2.f,0.f,0.f)),"reversed segment crossing box");
    check(!box(CVECTOR(2.f,2.f,0.f),CVECTOR(3.f,3.f,0.f)),"segment misses box");
    check(box(CVECTOR(-2.f,1.f,1.f),CVECTOR(2.f,1.f,1.f)),"segment grazes edge");
    check(box(CVECTOR(-2.f,0.f,0.f),CVECTOR(-1.f,0.f,0.f)),"segment touches endpoint");
    check(!box(CVECTOR(-2.f,1.0001f,0.f),CVECTOR(2.f,1.0001f,0.f)),"parallel outside rejected");
    check(box(CVECTOR(0.f),CVECTOR(0.f)),"inside degenerate segment");
    check(!box(CVECTOR(2.f),CVECTOR(2.f)),"outside degenerate segment");
    check(box(CVECTOR(-1.f),CVECTOR(-1.f)),"boundary degenerate segment");
    check(!box(CVECTOR(nan,0.f,0.f),CVECTOR(0.f)),"NaN segment rejected");
    check(!box(CVECTOR(inf,0.f,0.f),CVECTOR(0.f)),"infinite segment rejected");
    check(!manual_aim::segmentBox(CVECTOR(0.f),CVECTOR(0.f),CVECTOR(nan,0.f,0.f),hi),"NaN bounds rejected");

    // Thin tilted gunline: test the rendered radial ring, not merely its source hull.
    const auto padded=[](const std::vector<manual_aim::Point>& points,float pad) {
        std::vector<manual_aim::Point> corners;
        for(auto p:points)for(int k=0;k<4;++k)
            corners.push_back({p.x+((k&1)?pad:-pad),p.y+((k&2)?pad:-pad)});
        return manual_aim::hull(corners);
    };
    const auto containsWithin=[](const std::vector<manual_aim::Point>& contour,manual_aim::Point p,float tolerance) {
        if(contour.size()<3)return false;
        for(size_t i=0;i<contour.size();++i) {
            const auto a=contour[i],b=contour[(i+1)%contour.size()];
            const float edge=std::hypot(b.x-a.x,b.y-a.y);
            if(manual_aim::cross(a,b,p)<-tolerance*edge)return false;
        }
        return true;
    };
    const std::vector<manual_aim::Point> tiltedMuzzles{{-25.f,-2.1872166f},{0.f,0.f},{25.f,2.1872166f}};
    const auto tilted=padded(tiltedMuzzles,.08f);
    const auto adjacent=padded({{-22.f,-1.3f},{0.f,0.f},{22.f,1.3f}},.06f);
    const auto events=manual_aim::angularEvents(tilted,adjacent,20);
    std::vector<manual_aim::Point> tiltedRing,adjacentRing;
    for(float angle:events) {
        tiltedRing.push_back(manual_aim::boundary(tilted,{0,0},angle));
        adjacentRing.push_back(manual_aim::boundary(adjacent,{0,0},angle));
    }
    // Reconstruct the polygons from the same boundary calls used by the mesh.
    const auto actualTilted=manual_aim::hull(tiltedRing),actualAdjacent=manual_aim::hull(adjacentRing);
    for(auto p:tiltedMuzzles)check(containsWithin(actualTilted,p,1e-3f),"actual thin ring contains every muzzle");
    for(auto p:tilted)check(containsWithin(actualTilted,p,1e-3f),"actual thin ring contains every padded hull corner");
    for(auto p:adjacent)check(containsWithin(actualAdjacent,p,1e-3f),"adjacent angle union retains both contours");
    for(const auto* contour:{&tilted,&adjacent})for(auto corner:*contour) {
        const auto p=manual_aim::boundary(*contour,{0,0},std::atan2(corner.y,corner.x));
        check(near(p.x,corner.x,1e-3)&&near(p.y,corner.y,1e-3),"exact vertex angle reconstructs corner");
    }
    float minTilt=inf,maxTilt=-inf;
    for(auto p:tiltedRing){minTilt=std::min(minTilt,p.x);maxTilt=std::max(maxTilt,p.x);}
    check(minTilt<=-25.079f&&maxTilt>=25.079f,"rendered tilted ring retains 50m width plus padding");

    // A station plane has zero, one, or two admissible roots, in either direction.
    const CVECTOR stationAxis(1.f,0.f,0.f);
    const manual_aim::Trajectory forwardPath{{1.f,0.f,0.f},{2.f,0.f,0.f},{0.f,0.f,0.f}};
    const manual_aim::Trajectory reversePath{{5.f,0.f,0.f},{-1.f,0.f,0.f},{0.f,0.f,0.f}};
    auto times=manual_aim::planeTimes(forwardPath,stationAxis,5.f,3.f);
    check(times.size()==1&&near(times[0],2.f),"forward linear plane root");
    times=manual_aim::planeTimes(reversePath,stationAxis,3.f,4.f);
    check(times.size()==1&&near(times[0],2.f),"reverse linear plane root");
    const manual_aim::Trajectory turning{{0.f,0.f,0.f},{4.f,0.f,0.f},{-2.f,0.f,0.f}};
    times=manual_aim::planeTimes(turning,stationAxis,3.f,4.f);
    check(times.size()==2&&near(times[0],1.f)&&near(times[1],3.f),"both roots of turning path");
    times=manual_aim::planeTimes(turning,stationAxis,4.f,4.f);
    check(times.size()==1&&near(times[0],2.f),"turning tangency root exactly once");
    times=manual_aim::planeTimes(turning,stationAxis,0.f,4.f);
    check(times.size()==2&&near(times[0],0.f)&&near(times[1],4.f),"quadratic exact start and end boundary roots");
    check(manual_aim::planeTimes(turning,stationAxis,5.f,4.f).empty(),"unreached station has no roots");
    times=manual_aim::planeTimes(turning,stationAxis,3.f,2.f);
    check(times.size()==1&&near(times[0],1.f),"root beyond path end excluded");
    const manual_aim::Trajectory reverseTurning{{4.f,0.f,0.f},{-4.f,0.f,0.f},{2.f,0.f,0.f}};
    times=manual_aim::planeTimes(reverseTurning,stationAxis,1.f,4.f);
    check(times.size()==2&&near(times[0],1.f)&&near(times[1],3.f),"both roots of reverse turning path");
    const manual_aim::Trajectory parallelPath{{2.f,0.f,0.f},{0.f,1.f,0.f},{0.f,-1.f,0.f}};
    times=manual_aim::planeTimes(parallelPath,stationAxis,2.f,3.f);
    check(times.size()==2&&near(times[0],0.f)&&near(times[1],3.f),"path on plane preserves its endpoints");
    check(manual_aim::planeTimes(parallelPath,stationAxis,3.f,3.f).empty(),"parallel displaced path excluded");
    check(manual_aim::planeTimes(turning,stationAxis,nan,4.f).empty(),"NaN station rejected");
    check(manual_aim::planeTimes(turning,CVECTOR(nan,0.f,0.f),3.f,4.f).empty(),"NaN station axis rejected");
    check(manual_aim::planeTimes(turning,stationAxis,3.f,nan).empty(),"NaN lifetime rejected");
    check(manual_aim::planeTimes(turning,stationAxis,0.f,-1e-6f).empty(),"negative lifetime rejected");
    auto nonfinitePath=turning;nonfinitePath.acceleration.x=inf;
    check(manual_aim::planeTimes(nonfinitePath,stationAxis,0.f,4.f).empty(),"nonfinite quadratic rejected");

    // Reconstruct the controller's station sampling and loft rings for a legal
    // close shot. The bow gun's camera-axis station decreases as it flies.
    const CVECTOR closeCamera(0.f,12.f,-20.f),closeAxis(std::cos(pi/6),0.f,std::sin(pi/6));
    const CVECTOR closeTarget(20.f,0.f,-8.452995f),bow(5.f,4.f,20.f);
    const CVECTOR closeLateral(-closeAxis.z,0.f,closeAxis.x),closeUp=!(closeAxis^closeLateral);
    check(near((!(closeTarget-bow))|CVECTOR(1.f,0.f,0.f),.4628,1e-4),"close bow shot passes live traverse guard");
    check(manual_aim::reachable(bow,closeTarget,80.f,g),"close off-axis shot is reachable");
    struct MeshPath {manual_aim::Trajectory curve;float end;};
    std::vector<MeshPath> closePaths;
    for(CVECTOR origin:{CVECTOR(5.f,4.f,-20.f),CVECTOR(5.f,4.f,0.f),bow}) {
        const CVECTOR d=closeTarget-origin;
        const float range=std::hypot(d.x,d.z),angle=float(lowAngle(range,d.y,80.f,g));
        const auto curve=manual_aim::trajectory(origin,closeTarget,80.f,angle,0.f,.4f,g);
        closePaths.push_back({curve,curve.timeToHeight(0.f)});
    }
    const auto& bowPath=closePaths.back();
    check((bowPath.curve.origin|closeAxis)>(bowPath.curve.at(bowPath.end)|closeAxis),"bow station interval really reverses");
    std::vector<float> closeStations;
    float firstStation=inf,lastStation=-inf;
    for(const auto& path:closePaths) {
        const float a=path.curve.origin|closeAxis,b=path.curve.at(path.end)|closeAxis;
        firstStation=std::min({firstStation,a,b});lastStation=std::max({lastStation,a,b});
        closeStations.push_back(a);closeStations.push_back(b);
        const float acceleration=path.curve.acceleration|closeAxis;
        if(std::abs(acceleration)>1e-8f) {
            const float turn=-(path.curve.velocity|closeAxis)/acceleration;
            if(turn>0.f&&turn<path.end) {
                const float station=path.curve.at(turn)|closeAxis;
                closeStations.push_back(station);
                firstStation=std::min(firstStation,station);lastStation=std::max(lastStation,station);
            }
        }
    }
    for(int i=0;i<=32;++i)closeStations.push_back(firstStation+(lastStation-firstStation)*float(i)/32);
    std::sort(closeStations.begin(),closeStations.end());
    closeStations.erase(std::unique(closeStations.begin(),closeStations.end(),[](float a,float b){return std::abs(a-b)<.005f;}),closeStations.end());
    std::vector<manual_aim::Point> previousMeshHull;CVECTOR previousMeshCenter(0.f);
    bool hadMeshSection=false,sampledBow=false,meshRetainedBow=false;
    float nearestBowVertex=inf;
    for(float station:closeStations) {
        CVECTOR center(0.f);std::vector<CVECTOR> points;
        for(const auto& path:closePaths)for(float t:manual_aim::planeTimes(path.curve,closeAxis,station,path.end)) {
            const auto point=path.curve.at(t);
            sampledBow|=near(point,bow,1e-4);
            points.push_back(point);center+=point;
        }
        if(points.empty()){hadMeshSection=false;continue;}
        center*=1.f/points.size();
        const float u=(station-firstStation)/std::max(.01f,lastStation-firstStation);
        std::vector<manual_aim::Point> plane;
        for(auto point:points) {
            const auto d=point-center;plane.push_back({d|closeLateral,d|closeUp});
        }
        const auto sectionHull=padded(plane,.08f*(1.f-u));
        if(hadMeshSection) {
            const auto angles=manual_aim::angularEvents(previousMeshHull,sectionHull,20);
            for(float angle:angles)for(int which:{0,1}) {
                const auto p=manual_aim::boundary(which?sectionHull:previousMeshHull,{0.f,0.f},angle);
                const auto vertex=(which?center:previousMeshCenter)+closeLateral*p.x+closeUp*p.y;
                nearestBowVertex=std::min(nearestBowVertex,(vertex-bow).GetLength());
                meshRetainedBow|=near(vertex,bow,1e-3);
            }
        }
        previousMeshHull=sectionHull;previousMeshCenter=center;hadMeshSection=true;
    }
    check(sampledBow,"actual station-time samples retain close bow muzzle");
    check(meshRetainedBow,"actual reconstructed loft vertices retain close bow muzzle");
    check(std::isfinite(nearestBowVertex)&&nearestBowVertex<1e-3f,"mesh bow-vertex distance below one millimeter");
    check(manual_aim::finite(closeCamera),"close-case camera finite");


    // A real aperture is a missed receiver, not a cue to reproject the path.
    // The center shot threads a narrow hole at z40 and continues to sea z100;
    // an adjacent yaw sample hits the wall before reaching that same sea.
    const float apertureAngle=float(lowAngle(100,-4,80,g));
    const auto through=manual_aim::trajectory(CVECTOR(0.f,4.f,0.f),CVECTOR(0.f,0.f,100.f),80.f,apertureAngle,0.f,.4f,g);
    const auto beside=manual_aim::trajectory(through.origin,CVECTOR(0.f,0.f,100.f),80.f,apertureAngle,.02f,.4f,g);
    const auto wallTimes=manual_aim::planeTimes(through,CVECTOR(0.f,0.f,1.f),40.f,through.timeToHeight(0.f));
    check(wallTimes.size()==1,"aperture fixture has one wall crossing");
    const CVECTOR aperture=through.at(wallTimes.empty()?0.f:wallTimes.front());
    struct FixtureHit {bool valid=false;float fraction=2.f;CVECTOR point{0.f};int receiver=0;};
    const auto wallAndSea=[&](const CVECTOR& a,const CVECTOR& b) {
        FixtureHit hit;
        const auto offer=[&](float f,int receiver) {
            if(f>=0.f&&f<=1.f&&f<hit.fraction)hit={true,f,a+(b-a)*f,receiver};
        };
        if(a.z<40.f&&b.z>=40.f) {
            const float f=(40.f-a.z)/(b.z-a.z);const auto p=a+(b-a)*f;
            if(std::abs(p.x)>=.15f||std::abs(p.y-aperture.y)>=.25f)offer(f,1);
        }
        if(a.y>0.f&&b.y<=0.f)offer(a.y/(a.y-b.y),2);
        return hit;
    };
    for(int shot=0;shot<2;++shot) {
        const auto& curve=shot?beside:through;
        const auto saved=curve;
        const float limit=curve.timeToHeight(-2.f);constexpr int steps=64;
        int calls=0;bool sameTrajectory=true;
        const auto contact=manual_aim::firstContact(curve,limit,steps,[&](const CVECTOR& a,const CVECTOR& b) {
            sameTrajectory&=near(a,curve.at(limit*float(calls)/steps),1e-5);
            ++calls;sameTrajectory&=near(b,curve.at(limit*float(calls)/steps),1e-5);
            return wallAndSea(a,b);
        });
        check(sameTrajectory&&calls>0,"firstContact preserves every original ballistic segment");
        check(near(curve.origin,saved.origin)&&near(curve.velocity,saved.velocity)&&near(curve.acceleration,saved.acceleration),"firstContact leaves origin and trajectory unchanged");
        check(contact.hit.valid&&contact.hit.receiver==(shot?1:2),"aperture continues to sea while adjacent shot hits wall");
        check(near(contact.hit.point.z,shot?40.f:100.f,.05),"aperture and adjacent shot retain correct receiver distance");
        check(near(curve.at(contact.end),contact.hit.point,.002),"contact time remains on original trajectory");
        if(shot)check(contact.end<curve.timeToHeight(0.f),"wall contact precedes sea contact");
    }
    const auto bothReceivers=manual_aim::firstContact(beside,beside.timeToHeight(-2.f),1,wallAndSea);
    check(bothReceivers.hit.valid&&bothReceivers.hit.receiver==1,"earliest receiver wins even within one march segment");
    for(bool ready:{false,true}) {
        const float start=manual_aim::corridorOpacity(0.f,ready),middle=manual_aim::corridorOpacity(.5f,ready),end=manual_aim::corridorOpacity(1.f,ready);
        check(start>0.f&&start<middle&&middle<end&&end<=.045f,"corridor stays soft and strengthens toward impact");
        check(near(manual_aim::corridorOpacity(-1.f,ready),start,1e-8)&&near(manual_aim::corridorOpacity(2.f,ready),end,1e-8),"corridor opacity clamps outside progress range");
    }
    check(near(manual_aim::corridorOpacity(0.f,true),.009f,1e-8)&&near(manual_aim::corridorOpacity(1.f,true),.045f,1e-8),"ready corridor uses exact low-to-higher soft alpha bounds");


    // Verify emitted contact triangles, not just successful hole traces. Both
    // sampling and barycenter validation use the real ballistic march above.
    struct ContactHit {bool valid=false;int surface=-1;CVECTOR point{0.f};};
    struct ContactVertex {ContactHit hit;float yaw=0.f,elevation=0.f;};
    manual_aim::ContactBudget<4> contactBudget;
    int activeContactCell=-1;bool localContactPatch=false;
    std::vector<ContactVertex> contactCache;
    const auto contactSample=[&](float yaw,float elevation) {
        for(const auto& cached:contactCache)
            if(cached.yaw==yaw&&cached.elevation==elevation)return cached;
        if(!contactBudget.take(activeContactCell,localContactPatch))return ContactVertex{};
        const auto curve=manual_aim::trajectory(through.origin,CVECTOR(0.f,0.f,100.f),80.f,
            apertureAngle+elevation,yaw,.4f,g);
        const auto result=manual_aim::firstContact(curve,curve.timeToHeight(-2.f),64,wallAndSea);
        ContactVertex vertex{{result.hit.valid,result.hit.receiver,result.hit.point},yaw,elevation};
        contactCache.push_back(vertex);return vertex;
    };
    std::vector<std::array<ContactVertex,3>> emittedContacts;
    const auto emitContact=[&](const ContactVertex& a,const ContactVertex& b,const ContactVertex& c) {
        if(!a.hit.valid||!b.hit.valid||!c.hit.valid||a.hit.surface!=b.hit.surface||a.hit.surface!=c.hit.surface)return;
        const auto center=contactSample((a.yaw+b.yaw+c.yaw)/3.f,(a.elevation+b.elevation+c.elevation)/3.f);
        if(!center.hit.valid||center.hit.surface!=a.hit.surface)return;
        if(~((b.hit.point-a.hit.point)^(c.hit.point-a.hit.point))<=1e-10f)return;
        emittedContacts.push_back({a,b,c});
    };
    const auto apertureContact=contactSample(0.f,0.f);
    // Reserve a later solid cell's base center, corners, and four validation
    // centers before any optional refinement, matching the engine's contract.
    const auto wallCenter=contactSample(.02f,0.f);
    std::array<ContactVertex,4> wallCorners{{contactSample(.018f,-.002f),contactSample(.022f,-.002f),
        contactSample(.022f,.002f),contactSample(.018f,.002f)}};
    for(size_t i=0;i<4;++i) {
        const auto& a=wallCorners[i];const auto& b=wallCorners[(i+1)%4];
        contactSample((a.yaw+b.yaw+wallCenter.yaw)/3.f,(a.elevation+b.elevation+wallCenter.elevation)/3.f);
    }
    activeContactCell=0;localContactPatch=true;
    manual_aim::contactPatch(apertureContact,.001f,.001f,contactSample,emitContact);
    check(!emittedContacts.empty(),"detected aperture emits actual water contact triangles");
    bool allWater=true;
    for(const auto& triangle:emittedContacts)for(const auto& vertex:triangle)
        allWater&=vertex.hit.valid&&vertex.hit.surface==2&&near(vertex.hit.point.y,0.f,1e-5)&&vertex.hit.point.z>90.f;
    check(allWater&&!emittedContacts.empty(),"every emitted aperture triangle vertex lies on real downstream water");
    check(contactBudget.patch[0]<=8,"aperture patch and barycenter validations respect local rescue budget");
    while(contactBudget.take(0,false)){}
    while(contactBudget.take(0,true)){}
    check(contactBudget.detail[0]==16&&contactBudget.patch[0]==8&&
          !contactBudget.take(0,false)&&!contactBudget.take(0,true),"first contact cell exhausts exactly its independent detail and rescue budgets");
    activeContactCell=1;localContactPatch=false;emittedContacts.clear();
    for(size_t i=0;i<4;++i)emitContact(wallCorners[i],wallCorners[(i+1)%4],wallCenter);
    check(emittedContacts.size()==4,"later cell still emits all reserved solid-wall base triangles");
    check(contactBudget.detail[1]==0&&contactBudget.remaining(1)==16,"reserved base contacts need no later-cell optional budget");
    emittedContacts.clear();localContactPatch=true;
    manual_aim::contactPatch(wallCenter,.001f,.001f,contactSample,emitContact);
    check(!emittedContacts.empty()&&contactBudget.patch[1]>0&&contactBudget.patch[1]<=8,
          "later cell retains its own rescue capacity after earlier-cell starvation");
    bool allWall=true;
    for(const auto& triangle:emittedContacts)for(const auto& vertex:triangle)
        allWall&=vertex.hit.valid&&vertex.hit.surface==1&&near(vertex.hit.point.z,40.f,1e-5);
    check(allWall&&!emittedContacts.empty(),"later-cell emitted triangle vertices stay on real solid wall");
    check(contactBudget.take(-1,false)&&contactBudget.take(-1,true),"mandatory contacts bypass exhausted optional cell budgets");


    // Empty/far camera rays share a muzzle-centered legal boundary independent
    // of camera elevation; include the same float-boundary safety margin.
    const auto fireRange=[&](float height,float speed,float angle) {
        const float a=-g*.5f,b=speed*std::sin(angle);
        return speed*((-b-std::sqrt(b*b-4.f*a*height))/(2.f*a))*std::cos(angle);
    };
    struct RangeDisk {CVECTOR muzzle;float radius;};
    std::vector<RangeDisk> disks;
    for(CVECTOR p:{CVECTOR(-5.f,4.f,-25.f),CVECTOR(5.f,5.f,0.f),CVECTOR(7.f,6.f,25.f)}) {
        const float angle=manual_aim::maximumRangeElevation(p.y,60.f,-pi/18.f,pi/9.f,g);
        disks.push_back({p,fireRange(p.y,60.f,angle)});
    }
    const CVECTOR farCamera(12.f,18.f,-3.f),bearing(1.f,0.f,0.f);
    const auto commonLimit=[&](CVECTOR camera,CVECTOR flat,const std::vector<RangeDisk>& ranges) {
        float far=inf;
        for(const auto& disk:ranges)
            if(!manual_aim::farRayLimit(camera,disk.muzzle,flat,disk.radius,far))return -1.f;
        return far-std::max(.01f,far*1e-5f);
    };
    const float commonFar=commonLimit(farCamera,bearing,disks);
    const CVECTOR farTarget=CVECTOR(farCamera.x,0.f,farCamera.z)+bearing*commonFar;
    check(commonFar>0.f&&std::isfinite(commonFar),"common far-ray limit finite and positive");
    for(const auto& disk:disks) {
        check(std::hypot(farTarget.x-disk.muzzle.x,farTarget.z-disk.muzzle.z)<=disk.radius,
              "common far target stays inside every offset muzzle disk");
        check(manual_aim::reachable(disk.muzzle,farTarget,60.f,g),"common far target remains ballistically reachable for each gun");
    }
    std::vector<RangeDisk> mirroredDisks;
    for(auto disk:disks){disk.muzzle.x=-disk.muzzle.x;mirroredDisks.push_back(disk);}
    const CVECTOR mirroredCamera(-farCamera.x,farCamera.y,farCamera.z);
    const float mirroredFar=commonLimit(mirroredCamera,CVECTOR(-1.f,0.f,0.f),mirroredDisks);
    check(near(commonFar,mirroredFar,1e-4),"common legal far range is port-starboard symmetric");
    const float movedFar=commonLimit(farCamera+CVECTOR(9.f,50.f,0.f),bearing,disks);
    check(near(movedFar,commonFar-9.f,1e-3),"camera along-bearing offset shortens ray without moving legal boundary");
    for(float pitchDegrees:{-20.f,-5.f,0.f,15.f,40.f}) {
        const float a=pitchDegrees*pi/180.f;
        const CVECTOR ray(std::cos(a),std::sin(a),0.f),flat=!(CVECTOR(ray.x,0.f,ray.z));
        check(near(commonLimit(farCamera,flat,disks),commonFar,1e-4),"empty-ray pitch sweep preserves common far limit");
    }
    const CVECTOR shiftedCamera=farCamera+CVECTOR(0.f,0.f,8.f);
    const float shiftedFar=commonLimit(shiftedCamera,bearing,disks);
    const CVECTOR shiftedTarget=CVECTOR(shiftedCamera.x,0.f,shiftedCamera.z)+bearing*shiftedFar;
    for(const auto& disk:disks)
        check(std::hypot(shiftedTarget.x-disk.muzzle.x,shiftedTarget.z-disk.muzzle.z)<=disk.radius,
              "lateral camera offset retains every muzzle range constraint");
    for(auto pair:std::array<std::array<float,2>,2>{{{60.f,5.f},{45.f,4.f}}}) {
        const auto [speed,height]=pair;
        const float optimum=manual_aim::maximumRangeElevation(height,speed,-pi/18.f,pi/3.f,g);
        check(optimum>0.f&&optimum<pi/4.f,"positive muzzle height makes optimum strictly below45 degrees");
        check(near(optimum,std::atan2(speed,std::sqrt(speed*speed+2.f*g*height)),1e-6),"maximum range optimum matches analytic value");
        check(near(manual_aim::maximumRangeElevation(height,speed,-pi/18.f,pi/9.f,g),pi/9.f,1e-6),"maximum range elevation respects legal upper clamp");
        float far=inf;const float range=fireRange(height,speed,optimum);
        check(manual_aim::farRayLimit(CVECTOR(0.f,18.f,0.f),CVECTOR(0.f,height,0.f),bearing,range,far),"optimum range intersects camera bearing");
        far-=std::max(.01f,far*1e-5f);
        check(manual_aim::reachable(CVECTOR(0.f,height,0.f),CVECTOR(far,0.f,0.f),speed,g),"centimeter float-boundary margin preserves optimum reachability");
    }

    // The renderer clips against actual hull edge normals, not a fixed set of
    // directions whose bounding box fattens a thin rolled 50m gunline.
    const auto endContour=padded({{-18.f,-1.57479595f},{0.f,0.f},{18.f,1.57479595f}},.04f);
    const auto planes=manual_aim::slabPlanes(tilted,endContour);
    check(!planes.empty()&&planes.size()<=storm::sea_ai::manual_aim::maxVolumePlanesPerSlab,"exact slab planes fit bridge capacity");
    const auto slabInside=[](const std::vector<manual_aim::SlabPlane>& p,manual_aim::Point q,float u) {
        for(auto n:p)if(n.x*q.x+n.y*q.y>n.offset0*(1.f-u)+n.offset1*u+2e-4f)return false;
        return true;
    };
    for(auto p:tilted)check(slabInside(planes,p,0.f),"all near endpoint hull vertices satisfy slab planes");
    for(auto p:endContour)check(slabInside(planes,p,1.f),"all far endpoint hull vertices satisfy slab planes");
    for(auto a:tilted)for(auto b:endContour)
        check(slabInside(planes,{(a.x+b.x)*.5f,(a.y+b.y)*.5f},.5f),"middle interpolated contour remains inside exact slab");
    const float slope=2.1872166f/25.f;
    for(float u:{0.f,.5f,1.f}) {
        float bottom=-inf,top=inf;
        for(auto p:planes) {
            const float offset=p.offset0*(1.f-u)+p.offset1*u;
            if(p.y>1e-6f)top=std::min(top,offset/p.y);
            if(p.y<-1e-6f)bottom=std::max(bottom,offset/p.y);
            check(std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(offset),"slab coefficients remain finite");
        }
        const float expectedHalf=(.08f*(1.f-u)+.04f*u)*(1.f+slope);
        check(near(top,expectedHalf,2e-4)&&near(bottom,-expectedHalf,2e-4),"rolled slab preserves true centerline half-height");
        check(top-bottom<.18f,"thin rolled slab does not become a4.5m fat box");
        check(!slabInside(planes,{0.f,1.f},u),"fat-box interior point remains outside thin true slab");
    }
    const auto degenerateSlab=manual_aim::slabPlanes({{0.f,0.f}},{{-1.f,0.f},{1.f,0.f}});
    bool finiteDegenerate=!degenerateSlab.empty();
    for(auto p:degenerateSlab)finiteDegenerate&=std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.offset0)&&std::isfinite(p.offset1);
    check(finiteDegenerate,"point-to-line degenerate slab has finite planes");
    check(slabInside(degenerateSlab,{0.f,0.f},0.f)&&slabInside(degenerateSlab,{-1.f,0.f},1.f)&&slabInside(degenerateSlab,{1.f,0.f},1.f),"degenerate slab retains all true endpoint vertices");
    check(manual_aim::slabPlanes({},tilted).empty(),"empty endpoint yields no fabricated slab");

    // The user's invisible rectangular area chooses depth only, independently
    // each call. A thin off-center mast must not snap the actual shooting ray.
    const CVECTOR rangeCamera(10.f,20.f,-5.f),rangeForward(0.f,0.f,1.f);
    const CVECTOR rangeRight(1.f,0.f,0.f),rangeUp(0.f,1.f,0.f);
    const float tx=60.f/2048.f,ty=34.f/1285.f;
    const CVECTOR mastDelta(2.f,1.f,100.f);
    check(manual_aim::insideRangeAperture(mastDelta,rangeForward,rangeRight,rangeUp,tx,ty),
          "off-center thin mast intersects the actual rectangular range area");
    check(!manual_aim::insideRangeAperture(CVECTOR(3.f,0.f,100.f),rangeForward,rangeRight,rangeUp,tx,ty),
          "mast beyond rectangle releases without retained target");
    check(!manual_aim::insideRangeAperture(CVECTOR(0.f,2.7f,100.f),rangeForward,rangeRight,rangeUp,tx,ty),
          "shorter rectangle height matches user aspect ratio");
    check(!manual_aim::insideRangeAperture(CVECTOR(0.f,0.f,-100.f),rangeForward,rangeRight,rangeUp,tx,ty),
          "geometry behind camera never supplies range");
    const auto rangeTarget=manual_aim::centerRangeTarget(rangeCamera,rangeForward,mastDelta|rangeForward);
    check(near(rangeTarget,rangeCamera+CVECTOR(0.f,0.f,100.f)),"aperture changes depth without aiming toward side mast");
    check(near(!(rangeTarget-rangeCamera),rangeForward),"selected depth preserves exact center-plus direction");
    const float rollAngle=.7f;
    const CVECTOR rolledRight(std::cos(rollAngle),std::sin(rollAngle),0.f);
    const CVECTOR rolledUp(-std::sin(rollAngle),std::cos(rollAngle),0.f);
    const CVECTOR rolledMast=rolledRight*2.f+rolledUp+rangeForward*100.f;
    check(manual_aim::insideRangeAperture(rolledMast,rangeForward,rolledRight,rolledUp,tx,ty),
          "screen rectangle follows rolled camera basis");
    for(float depth:{40.f,500.f,100.f,500.f})
        check(near(manual_aim::centerRangeTarget(rangeCamera,rangeForward,depth),rangeCamera+rangeForward*depth),
              "repeated picks and closer obstruction are stateless");
    using RelationVertex=storm::sea_ai::manual_aim::AimVolumeRelationVertex;
    check(sizeof(RelationVertex)==16&&alignof(RelationVertex)==16&&offsetof(RelationVertex,color)==12,
          "relation-mask POD layout matches packed GPU record");
    check((storm::sea_ai::manual_aim::excludedReceiverColor&0xFF000000u)!=0,
          "own-ship contact exclusion is distinct from neutral receiver RGB");

    std::cout<<checks<<" checks, "<<failures<<" failures\n";
    return failures?1:0;
}
