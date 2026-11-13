// Offline conversion of JangaFX Ground Explosion VDB frames to dense RG8.
// Usage: convert_cluster_vdb input.vdb output.rg8 [ground|midair]
// R = density [0,2]; G = flames [0,8] for ground, [0,2] for midair.
// Fixed source domains keep frames registered when active VDB bounds change.
#include <openvdb/openvdb.h>
#include <openvdb/tools/Interpolation.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

int main(int argc, char **argv) {
    if (argc != 3 && argc != 4) { std::fprintf(stderr, "usage: %s input.vdb output.rg8 [ground|midair]\n", argv[0]); return 2; }
    try {
        const std::string profile = argc == 4 ? argv[3] : "ground";
        if (profile != "ground" && profile != "midair")
            throw std::runtime_error("unknown volume profile");
        const bool midair = profile == "midair";
        const float flameScale = midair ? 2.f : 8.f;
        openvdb::initialize();
        openvdb::io::File file(argv[1]); file.open();
        auto density = openvdb::gridPtrCast<openvdb::FloatGrid>(file.readGrid("density"));
        auto flame = openvdb::gridPtrCast<openvdb::FloatGrid>(file.readGrid("flames"));
        if (!density || !flame) throw std::runtime_error("expected density and flames float grids");
        auto d = density->getConstAccessor(), f = flame->getConstAccessor();
        constexpr int size = 64;
        std::vector<std::uint8_t> pixels(size * size * size * 2);
        float maxDensity = 0, maxFlame = 0;
        for (int z = 0; z < size; ++z) for (int y = 0; y < size; ++y) for (int x = 0; x < size; ++x) {
            openvdb::Vec3d p((x + .5) * (midair ? 193. : 287.) / size,
                             (z + .5) * (midair ? 385. : 287.) / size,
                             (y + .5) * (midair ? 385. : 319.) / size);
            const float a = openvdb::tools::BoxSampler::sample(d, p), b = openvdb::tools::BoxSampler::sample(f, p);
            if (!std::isfinite(a) || !std::isfinite(b)) throw std::runtime_error("nonfinite voxel");
            maxDensity = std::max(maxDensity, a); maxFlame = std::max(maxFlame, b);
            const auto i = size_t(((z * size + y) * size + x) * 2);
            pixels[i] = std::uint8_t(std::lround(std::clamp(a / 2.f, 0.f, 1.f) * 255.f));
            pixels[i + 1] = std::uint8_t(std::lround(std::clamp(b / flameScale, 0.f, 1.f) * 255.f));
        }
        if (maxDensity > 2.f || maxFlame > flameScale)
            throw std::runtime_error("voxel exceeds RG8 channel range; choose a new encoding");
        std::ofstream out(argv[2], std::ios::binary); out.write(reinterpret_cast<const char *>(pixels.data()), pixels.size());
        if (!out) throw std::runtime_error("output write failed");
        std::printf("64^3 RG8: density max %.4f, flames max %.4f\n", maxDensity, maxFlame);
        return 0;
    } catch (const std::exception &e) { std::fprintf(stderr, "VDB conversion: %s\n", e.what()); return 1; }
}
