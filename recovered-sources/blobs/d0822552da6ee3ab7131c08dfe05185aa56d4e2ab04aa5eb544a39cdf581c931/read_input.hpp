#pragma once
#include "ship_laser_geometry.hpp"
#include <fstream>
#include <stdexcept>
#include <cstring>

// Reader for the task-local CINPUT01 payload. Floats/triangles are bit-exact;
// SurfaceBase.index is the deterministic cache derived from its captured outer.
struct InputReader {
    std::ifstream file;
    explicit InputReader(const char *path):file(path,std::ios::binary){if(!file)throw std::runtime_error("input open");}
    void bytes(void *out,size_t n){file.read(static_cast<char *>(out),std::streamsize(n));if(!file)throw std::runtime_error("input read");}
    template<class T> T pod(){T v;static_assert(std::is_trivially_copyable_v<T>);bytes(&v,sizeof(v));return v;}
    size_t count(){auto n=pod<uint64_t>();if(n>100000000)throw std::runtime_error("input count");return size_t(n);}
    storm_ship_laser::Vec3 vec(){return {pod<float>(),pod<float>(),pod<float>()};}
    template<class T>std::vector<T> array(){std::vector<T> v(count());if(!v.empty())bytes(v.data(),v.size()*sizeof(T));return v;}
    storm_ship_laser::Plane plane(){
        storm_ship_laser::Plane p;p.normal=vec();p.distance=pod<float>();p.charred=pod<bool>();p.grain=vec();
        p.roughness=pod<float>();p.spacing=pod<float>();p.phase=pod<float>();p.beams=array<storm_ship_laser::BeamSample>();return p;
    }
    storm_ship_laser::Mesh mesh(){
        storm_ship_laser::Mesh m;const auto n=count();for(size_t i=0;i<n;++i){const auto object=pod<int>();m.emplace(object,array<storm_ship_laser::Triangle>());}return m;
    }
    storm_ship_laser::Surface surface(){
        using namespace storm_ship_laser;
        Surface s;s.outer=mesh();s.canonicalCaps=mesh();s.interior=mesh();s.finiteWalls=mesh();
        if(pod<bool>()){std::vector<Mesh> material;const auto n=count();for(size_t i=0;i<n;++i)material.push_back(mesh());s.material=std::make_shared<const std::vector<Mesh>>(std::move(material));}
        if(pod<bool>()){auto original=std::make_shared<SurfaceBase>();original->outer=mesh();original->canonicalCaps=mesh();original->interior=mesh();original->finiteWalls=mesh();original->index=indexFaces(original->outer);s.original=std::move(original);}
        s.cutters=array<BeamSample>();const auto n=count();for(size_t i=0;i<n;++i){if(pod<bool>())s.released.push_back(std::make_shared<const Mesh>(mesh()));else s.released.push_back({});}
        if(pod<bool>())s.timber=std::make_shared<const std::vector<MaterialPrism>>(array<MaterialPrism>());
        const auto legacy=count();for(size_t i=0;i<legacy;++i)s.legacy.push_back(plane());return s;
    }
    storm_ship_laser::Input input(){
        using namespace storm_ship_laser;
        char magic[8];bytes(magic,8);if(std::memcmp(magic,"CINPUT01",8)||pod<uint32_t>()!=sizeof(Triangle))throw std::runtime_error("input ABI");
        Input i;if(pod<bool>())i.surface=std::make_shared<const Surface>(surface());const auto n=count();
        for(size_t k=0;k<n;++k){const int object=pod<int>();const float thickness=pod<float>();i.wood.emplace(object,thickness);}
        i.plane=plane();i.fragmentLimit=pod<unsigned>();i.supportAxis=vec();i.hasSupportAxis=pod<bool>();i.enclosedFragments=pod<bool>();
        i.closedVolume=pod<double>();i.generation=pod<uint64_t>();i.revision=pod<uint64_t>();
        if(file.peek()!=std::char_traits<char>::eof())throw std::runtime_error("input trailing bytes");return i;
    }
};
