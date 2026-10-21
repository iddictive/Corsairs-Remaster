#pragma once
#include <type_traits>
namespace postfix_capture {
inline constexpr uint64_t CaptureLimit=64ull<<20;
struct Writer {
    std::ofstream file;
    uint64_t capturedBytes=0;
    explicit Writer(const std::string &name):file(name,std::ios::binary) {
        if (!file) throw std::runtime_error("capture open");
    }
    void bytes(const void *data, size_t size) {
        if (size > CaptureLimit - capturedBytes) throw std::runtime_error("capture cap");
        file.write(static_cast<const char *>(data),std::streamsize(size));
        if (!file) throw std::runtime_error("capture write");
        capturedBytes += size;
    }
    template<class T> void pod(const T &value) {
        static_assert(std::is_trivially_copyable_v<T>); bytes(&value,sizeof(value));
    }
    void count(size_t value) { pod(uint64_t(value)); }
    void string(const std::string &value) { count(value.size());bytes(value.data(),value.size()); }
    template<class T> void vec(const T &v) { pod(v.x);pod(v.y);pod(v.z); }
    template<class T> void array(const std::vector<T> &v) {
        count(v.size()); if (!v.empty()) bytes(v.data(),v.size()*sizeof(T));
    }
    template<class T> void vectors(const std::vector<T> &v) { count(v.size());for(const auto &p:v)vec(p); }
    template<class T> void plane(const T &p) {
        vec(p.normal);pod(p.distance);pod(p.charred);vec(p.grain);
        pod(p.roughness);pod(p.spacing);pod(p.phase);array(p.beams);
    }
    void mesh(const storm_ship_laser::Mesh &m) {
        count(m.size());for(const auto &[object,triangles]:m){pod(object);array(triangles);}
    }
    void surface(const storm_ship_laser::Surface &s) {
        mesh(s.outer);mesh(s.canonicalCaps);mesh(s.interior);mesh(s.finiteWalls);
        pod(bool(s.material));if(s.material){count(s.material->size());for(const auto &m:*s.material)mesh(m);}
        pod(bool(s.original));if(s.original){mesh(s.original->outer);mesh(s.original->canonicalCaps);mesh(s.original->interior);mesh(s.original->finiteWalls);}
        array(s.cutters);count(s.released.size());for(const auto &p:s.released){pod(bool(p));if(p)mesh(*p);}
        pod(bool(s.timber));if(s.timber)array(*s.timber);
        count(s.legacy.size());for(const auto &p:s.legacy)plane(p);
    }
    void input(const storm_ship_laser::Input &i) {
        bytes("CINPUT01",8);pod(uint32_t(sizeof(storm_ship_laser::Triangle)));
        pod(bool(i.surface));if(i.surface)surface(*i.surface);
        count(i.wood.size());for(const auto &[object,thickness]:i.wood){pod(object);pod(thickness);}
        plane(i.plane);pod(i.fragmentLimit);vec(i.supportAxis);pod(i.hasSupportAxis);pod(i.enclosedFragments);
        pod(i.closedVolume);pod(i.generation);pod(i.revision);
    }
};
}
