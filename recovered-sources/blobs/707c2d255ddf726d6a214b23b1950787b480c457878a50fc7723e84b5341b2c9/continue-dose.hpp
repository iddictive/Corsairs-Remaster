void continueDose(storm_world_cut::Cut&cut,const storm_world_cut::Cut&pending)
{
    if(cut.kind!=storm_world_cut::Cut::Beam||pending.kind!=storm_world_cut::Cut::Beam)return;
    for(auto&sample:cut.beams)for(const auto&prior:pending.beams){
        const auto direction=storm_ship_laser::beamDirection(sample),other=storm_ship_laser::beamDirection(prior);
        if(storm_ship_hull::length(direction-other)>.0001f||std::abs(sample.radius-prior.radius)>.00001f||
            storm_ship_hull::length(storm_ship_laser::beamTangent(sample)-storm_ship_laser::beamTangent(prior))>.0001f)continue;
        const auto start=storm_ship_laser::beamStart(sample),old=storm_ship_laser::beamStart(prior),delta=start-old;
        const float tolerance=std::max(.0001f,2*std::numeric_limits<float>::epsilon()*std::max({std::abs(start.x),std::abs(start.y),std::abs(start.z),1.f}));
        const float along=storm_ship_hull::dot(delta,other);
        if(along < -tolerance||along>prior.depth+tolerance||storm_ship_hull::length(delta-other*along)>tolerance)continue;
        const auto end=old+other*prior.depth;
        if(storm_ship_hull::dot(end-start,direction)>tolerance)sample.start={end.x,end.y,end.z};
    }
}
