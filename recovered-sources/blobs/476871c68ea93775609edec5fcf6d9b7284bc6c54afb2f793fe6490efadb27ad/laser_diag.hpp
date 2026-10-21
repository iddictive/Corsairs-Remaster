#pragma once
// Task-local diagnostics only. No engine state or geometry is modified here.
#include <atomic>
#include <cstdarg>
#include <cstdio>
#include <fstream>
#include <mutex>
#include <stdexcept>
#include <type_traits>

namespace laser_diag {
inline std::mutex mutex;
inline uint64_t capturedBytes = 0, logBytes = 0;
inline bool capturedAccepted = false, capturedRejected = false;
inline constexpr uint64_t CaptureLimit = 960ull << 20, LogLimit = 4ull << 20;
inline void log(const char *format, ...) noexcept {
    try {
        std::lock_guard lock(mutex);
        if (logBytes >= LogLimit) return;
        static FILE *file = std::fopen("laser-verdicts.tsv", "w");
        if (!file) return;
        va_list arguments; va_start(arguments, format);
        const int count = std::vfprintf(file, format, arguments); va_end(arguments);
        std::fputc('\n', file); std::fflush(file);
        if (count > 0) logBytes += uint64_t(count) + 1;
    } catch (...) {}
}
struct Writer {
    std::ofstream file;
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
template<class Work> uint64_t stamp(const Work &work) {
    uint64_t h=1469598103934665603ull;
    const auto bytes=[&](const auto &v){const auto *p=reinterpret_cast<const unsigned char *>(&v);for(size_t n=0;n<sizeof(v);++n){h^=p[n];h*=1099511628211ull;}};
    bytes(work.epoch);bytes(work.model);bytes(work.request.predecessors);
    for(const auto &beam:work.request.plane.beams)bytes(beam);
    for(const auto &target:work.targets){bytes(target.node);bytes(target.input.generation);bytes(target.input.revision);}
    return h;
}
template<class Work> void capture(const Work &work,bool accepted,const char *reason) noexcept {
    bool wrote=false;uint64_t total=0;
    try {
        std::lock_guard lock(mutex);
        bool &captured=accepted?capturedAccepted:capturedRejected;
        if(captured)return;captured=true;
        const std::string prefix=accepted?"first-accepted":"first-rejected";
        Writer meta(prefix+".work.bin");meta.bytes("CWORK001",8);meta.string(reason);meta.pod(stamp(work));
        meta.pod(work.epoch);meta.pod(work.model);meta.string(work.modelName);meta.pod(work.valid);
        const auto &r=work.request;meta.plane(r.plane);meta.pod(r.owner);meta.pod(r.attacker);meta.pod(r.payPartialDamage);
        meta.count(r.predecessors);meta.vec(r.impulse);meta.pod(r.impactRadius);meta.pod(bool(r.impact));
        if(r.impact){meta.vec(r.impact->point);meta.vec(r.impact->normal);meta.pod(r.impact->energy);meta.pod(r.impact->age);meta.pod(r.impact->other);}
        meta.vectors(work.originalContour);meta.vectors(work.contour);meta.vectors(work.originalKeel);meta.pod(work.originalArea);
        meta.count(work.targets.size());
        for(const auto &t:work.targets){
            meta.pod(t.node);meta.string(t.name);meta.vec(t.local.Vx());meta.vec(t.local.Vy());meta.vec(t.local.Vz());meta.vec(t.local.Pos());
            meta.pod(t.input.generation);meta.pod(t.input.revision);meta.pod(t.input.fragmentLimit);
            meta.pod(t.projection.result.changed);meta.count(t.projection.result.fragments.size());
            Writer input(prefix+"-node-"+std::to_string(t.node)+".input.bin");input.input(t.input);
        }
        wrote=true;total=capturedBytes;
    } catch (...) { log("CAPTURE_ERROR\t%llu\t%d\t%s",(unsigned long long)stamp(work),accepted,reason);return; }
    if(wrote)log("CAPTURE\t%llu\t%d\t%s\t%llu",(unsigned long long)stamp(work),accepted,reason,(unsigned long long)total);
}
} // namespace laser_diag
