// Bounded evidence probe. Links the real Release animation objects; never starts the engine.
#include "animation_imp.h"
#include "animation_service_imp.h"
#include "an_file.h"
#include <algorithm>
#include <bit>
#include <chrono>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <vector>

// AnimationServiceImp is not constructed. Its teardown symbol must never run.
void AnimationServiceImp::DeleteAnimation(AnimationImp *) { std::abort(); }

template<class T> void read(std::ifstream &f, T *p, size_t n = 1) {
    if (!f.read(reinterpret_cast<char *>(p), sizeof(T) * n)) throw std::runtime_error("short rig read");
}

AnimationInfo *load(const std::filesystem::path &path, const char *name) {
    std::ifstream f(path, std::ios::binary);
    ANFILE::HEADER h{}; read(f, &h);
    if (h.nJoints <= 0 || h.nJoints > 256 || h.nFrames <= 18 || h.nFrames > 100000)
        throw std::runtime_error("invalid rig header");
    auto *info = new AnimationInfo(name);
    info->SetNumFrames(h.nFrames); info->SetFPS(h.framesPerSec); info->CreateBones(h.nJoints);
    std::vector<int32_t> parents(h.nJoints); read(f, parents.data(), parents.size());
    for (int i = 1; i < h.nJoints; ++i) {
        if (parents[i] < 0 || parents[i] >= i) throw std::runtime_error("non-topological rig");
        info->GetBone(i).SetParent(&info->GetBone(parents[i]));
    }
    std::vector<CVECTOR> positions(std::max(h.nJoints, h.nFrames));
    read(f, positions.data(), h.nJoints);
    for (int i = 0; i < h.nJoints; ++i) info->GetBone(i).SetNumFrames(h.nFrames, positions[i], i == 0);
    read(f, positions.data(), h.nFrames); info->GetBone(0).SetPositions(positions.data(), h.nFrames);
    std::vector<Quaternion> angles(h.nFrames);
    for (int i = 0; i < h.nJoints; ++i) {
        read(f, angles.data(), angles.size()); info->GetBone(i).SetAngles(angles.data(), h.nFrames);
    }
    if (f.peek() != EOF) throw std::runtime_error("unexpected rig tail");
    for (int i = 0; i < h.nJoints; ++i) info->GetBone(i).BuildStartMatrix();
    for (int i = 0; i < h.nJoints; ++i) info->GetBone(i).start.Transposition();
    // Both installed descriptors define walk frames 2..18. Events are deliberately omitted:
    // this probe measures animation/palette work and does not model script callbacks.
    auto *walk = info->AddAction("walk", 2, 18); walk->SetLoop(true);
    std::cout << "rig=" << name << " bones=" << h.nJoints << " frames=" << h.nFrames
              << " fps=" << h.framesPerSec << " walk=2:18 loop=true\n";
    return info;
}

uint64_t paletteHash(const AnimationImp &a) {
    uint64_t hash = 1469598103934665603ULL;
    for (int b = 0; b < a.GetNumBones(); ++b) for (float v : a.GetAnimationMatrix(b).matrix) {
        if (!std::isfinite(v)) throw std::runtime_error("nonfinite palette");
        hash ^= std::bit_cast<uint32_t>(v); hash *= 1099511628211ULL;
    }
    return hash;
}

void seed(std::vector<AnimationImp *> &instances, int players) {
    for (size_t i = 0; i < instances.size(); ++i) for (int p = 0; p < players; ++p) {
        auto &a = instances[i]->Player(p);
        if (!a.SetAction("walk") || !a.Play()) throw std::runtime_error("action setup failed");
        a.SetBlend(p == 0 ? .65f : .35f); a.SetPosition(float((i * 17 + p * 31) % 97) / 97.f);
    }
}

volatile double consumeSink;
int main(int argc, char **argv) try {
    if (argc != 2) return 64;
    std::cout << std::setprecision(12);
    for (const char *rig : {"man", "man2"}) {
        auto *info = load(std::filesystem::path(argv[1]) / (std::string(rig) + ".an"), rig);
        for (int count : {1, 32}) for (int players : {1, 2}) {
            std::vector<AnimationImp *> instances;
            // Deliberately process-owned lifetime: no service registry/teardown is instantiated.
            for (int i = 0; i < count; ++i) instances.push_back(new AnimationImp(i, info));
            seed(instances, players);
            uint64_t sequence = 1469598103934665603ULL;
            for (int step = 0; step < 120; ++step) for (auto *a : instances) {
                a->Execute(16); sequence ^= paletteHash(*a); sequence *= 1099511628211ULL;
            }
            const uint64_t before = paletteHash(*instances.front());
            if (count > 1) {
                instances.back()->Execute(37);
                if (before != paletteHash(*instances.front())) throw std::runtime_error("palette cross-instance mutation");
                if (before == paletteHash(*instances.back())) throw std::runtime_error("instance phases unexpectedly equal");
            }
            std::cout << "quality rig=" << rig << " instances=" << count << " players=" << players
                      << " sequence_hash=" << std::hex << sequence << std::dec
                      << " finite=pass isolation=" << (count > 1 ? "pass" : "not-applicable") << '\n';
            std::vector<double> samples;
            constexpr int steps = 4000;
            for (int round = -1; round < 7; ++round) {
                seed(instances, players);
                double checksum = 0;
                const auto start = std::chrono::steady_clock::now();
                for (int step = 0; step < steps; ++step) for (auto *a : instances) {
                    a->Execute(16);
                    // Consume the complete real palette, like a downstream skinning upload.
                    for (int bone = 0; bone < a->GetNumBones(); ++bone)
                        for (float v : a->GetAnimationMatrix(bone).matrix) checksum += v;
                }
                const double ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                consumeSink = checksum;
                if (!std::isfinite(checksum)) throw std::runtime_error("nonfinite timed palette");
                if (round >= 0) {
                    samples.push_back(ms);
                    std::cout << "sample rig=" << rig << " instances=" << count << " players=" << players
                              << " round=" << round << " steps=" << steps << " ms=" << ms << " checksum=" << checksum << '\n';
                }
            }
            std::sort(samples.begin(), samples.end());
            std::cout << "median rig=" << rig << " instances=" << count << " players=" << players
                      << " us_per_instance_step=" << samples[3] * 1000 / (steps * count) << '\n';
        }
    }
    return 0;
} catch (const std::exception &e) { std::cerr << e.what() << '\n'; return 1; }
