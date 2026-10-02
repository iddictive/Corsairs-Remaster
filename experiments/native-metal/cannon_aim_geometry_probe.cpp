// Standalone regression probe; no engine initialization or external framework.
// Compile with C++20; include shared_headers/include and engine math/include.
// Run the generated math_probe executable. No engine libraries are needed.
#include "shared/sea_ai/manual_aim_geometry.hpp"
#include <iostream>
#include <limits>
#include <numbers>
#include <string_view>

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

    std::cout<<checks<<" checks, "<<failures<<" failures\n";
    return failures?1:0;
}
