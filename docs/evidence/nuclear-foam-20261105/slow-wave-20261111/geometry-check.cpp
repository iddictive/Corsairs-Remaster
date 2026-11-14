#include "sea_nuclear_wave.hpp"
#include <cmath>
#include <cstdio>

int main() {
    for (float scale : {1.f, 3.f, 10.f})
        for (float age : {0.1f, 1.f, 5.f, 20.f})
            for (int a = 0; a < 24; ++a)
                for (int i = -20; i <= 20; ++i) {
                    SeaNuclearWave wave{12.f, -13.f, 0.f, 0.f, true, scale};
                    wave.Advance(age);
                    const float angle = a * 6.28318530718f / 24.f;
                    const float r = wave.Front() + i * wave.width * scale / 20.f;
                    const auto s = EvaluateSeaNuclearWave(wave, wave.x + r * std::cos(angle), wave.z + r * std::sin(angle));
                    std::printf("%.9g %.9g %.9g %.9g %.9g\n", s.height, s.slopeX, s.slopeZ, s.foam, s.load);
                }
}
