#include "sea_nuclear_wave.hpp"
#include <vector>
#include <algorithm>
#include <cassert>
#include <cstdio>
struct Mini {std::vector<SeaNuclearWave> nuclearWaves; bool nuclearWavesActive=false; void StartNuclearWave(float,float,unsigned,bool,float);};
void Mini::StartNuclearWave(float x, float z, unsigned /*slot*/, bool active, float lengthScale)
{
    if (!active || !std::isfinite(x) || !std::isfinite(z)) return;
    const float scale = std::isfinite(lengthScale) ? std::clamp(lengthScale, 1.f, 10.f) : 1.f;
    const auto liveCount = std::count_if(nuclearWaves.begin(), nuclearWaves.end(),
        [](const auto &wave) { return !wave.retiring; });
    if (liveCount >= SeaNuclearWave::maxActiveWaves) {
        // Keep the newest impact and let the oldest surface disappear smoothly.
        // A bounded retirement reserve handles ordinary sustained bursts.
        if (nuclearWaves.size() >= 2 * SeaNuclearWave::maxActiveWaves) {
            const auto retiring = std::find_if(nuclearWaves.begin(), nuclearWaves.end(),
                [](const auto &wave) { return wave.retiring; });
            if (retiring != nuclearWaves.end()) nuclearWaves.erase(retiring);
        }
        const auto oldest = std::find_if(nuclearWaves.begin(), nuclearWaves.end(),
            [](const auto &wave) { return !wave.retiring; });
        if (oldest != nuclearWaves.end()) {
            oldest->retiring = true;
            oldest->retirementRemaining = SeaNuclearWave::retirementTime;
        }
    }
    nuclearWaves.push_back({x, z, 0.f, 0.f, true, scale});
    nuclearWavesActive = true;
}

int main(){Mini m; size_t max=0; for(int frame=0;frame<12000;frame++){m.StartNuclearWave(frame,0,0,true,10); assert(m.nuclearWaves.back().x==frame && m.nuclearWaves.back().active);for(auto &w:m.nuclearWaves)w.Advance(.016f);std::erase_if(m.nuclearWaves,[](auto&w){return !w.active;});max=std::max(max,m.nuclearWaves.size());assert(max<=256);}printf("launches=12000 newest=12000 max=%zu steady=%zu\n",max,m.nuclearWaves.size());Mini b;for(int i=0;i<300;i++){b.StartNuclearWave(0,0,0,true,1);assert(b.nuclearWaves.back().active);}assert(b.nuclearWaves.size()<=256);printf("coincident=300 active=%zu newest=1\n",b.nuclearWaves.size());Mini q;for(int i=0;i<128;i++){q.StartNuclearWave(i*100,0,0,true,1);q.nuclearWaves.back().Advance(2);}float prev=q.nuclearWaves[0].amplitude;q.StartNuclearWave(99999,0,0,true,1);float at=q.nuclearWaves[0].amplitude;q.nuclearWaves[0].Advance(.001f);printf("fade prev=%.6f at=%.6f plus_1ms=%.6f\n",prev,at,q.nuclearWaves[0].amplitude);assert(prev==at&&std::abs(at-q.nuclearWaves[0].amplitude)<.01f);}
