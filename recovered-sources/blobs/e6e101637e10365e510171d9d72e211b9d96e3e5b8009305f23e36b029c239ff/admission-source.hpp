struct GEOM { enum { VISIBLE=1,COLLISION_ENABLE=2 }; struct {int nobjects=2;}rhead; struct Object {int flags=3,ntriangles=12;storm_ship_hull::Vec3 center{};float radius=1.74f;}; Object object[2]; std::set<int>worldTerrainObjects{0}; bool WorldCutAffects(const storm_world_cut::Cut&,bool)const; };
bool GEOM::WorldCutAffects(const storm_world_cut::Cut&cut,bool terrainModel) const
{
    for(int o=0;o<rhead.nobjects;++o){const auto&ob=object[o];
        if(!(ob.flags&(VISIBLE|COLLISION_ENABLE))||!ob.ntriangles||(terrainModel&&worldTerrainObjects.count(o)))continue;
        const storm_ship_hull::Vec3 center{ob.center.x,ob.center.y,ob.center.z},extent{ob.radius+1.f,ob.radius+1.f,ob.radius+1.f};
        if(storm_world_cut::affects({center-extent,center+extent},{cut}))return true;}
    return false;
}
