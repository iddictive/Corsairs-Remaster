#define main ignored_main
#include "probe.cpp"
#undef main
int main(){auto source=load("/tmp/corsairs-world-cut-kNlDBY/fort.bin");auto bounds=storm_ship_laser::bounds(source);Cut cut;cut.kind=Cut::Sphere;cut.center=(bounds.lo+bounds.hi)*.5f;cut.radius=12;auto plane=spherePlanes(cut)[2];Mesh solid,untouched;assert(prepare(source,{}, {cut},solid,untouched));
 auto original=storm_ship_laser::planarSplit(source,{plane.normal,plane.distance,false},false,true).seam;
 using K=std::pair<PositionKey,PositionKey>;std::set<K>outer;for(auto e:original){auto a=storm_ship_laser::positionKey(e.a.position),b=storm_ship_laser::positionKey(e.b.position);auto p=std::minmax(a,b);outer.insert({p.first,p.second});}
 auto basis=storm_ship_hull::frame(plane.normal);const auto cross=[&](Vec3 a,Vec3 b,Vec3 c){return double(storm_ship_hull::dot(storm_ship_hull::cross(b-a,c-a),plane.normal));};
 for(auto mesh:{source,solid}){auto seams=storm_ship_laser::planarSplit(mesh,{plane.normal,plane.distance,false},false,true).seam;unsigned count=0;for(size_t i=0;i<seams.size();++i)for(size_t j=i+1;j<seams.size();++j){auto a=seams[i].a.position,b=seams[i].b.position,c=seams[j].a.position,d=seams[j].b.position;if(cross(a,b,c)*cross(a,b,d)<-1e-6&&cross(c,d,a)*cross(c,d,b)<-1e-6){++count;std::cout<<"cross ";for(auto e:{seams[i],seams[j]}){auto x=storm_ship_laser::positionKey(e.a.position),y=storm_ship_laser::positionKey(e.b.position);auto p=std::minmax(x,y);std::cout<<(outer.count({p.first,p.second})?"OUTER ":"INNER ")<<e.a.position.x<<","<<e.a.position.y<<","<<e.a.position.z<<" -> "<<e.b.position.x<<","<<e.b.position.y<<","<<e.b.position.z<<" / ";}std::cout<<"\n";}}
 std::cout<<"mesh="<<triangleCount(mesh)<<" seam="<<seams.size()<<" crossings="<<count<<"\n";}
}
