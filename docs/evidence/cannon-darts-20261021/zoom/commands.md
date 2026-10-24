# Deck aim cap source verification

Portable command (repository root):

```sh
python3 experiments/native-metal/deck_aim_zoom_probe.py --engine-source experiments/native-metal/engine --gameplay-source src/gameplay
```

One-off cap probe compiled with `c++ -std=c++20 -Wall -Wextra -Werror -pedantic -Iexperiments/native-metal/engine/src/libs/sea_cameras/src <source> -o <temporary-executable>`. Source:

```cpp
#include "deck_aim_zoom.h"
#include <cassert>
#include <cmath>
#include <iostream>
using deck_aim::Zoom;
int checks = 0;
void check(bool ok) { ++checks; assert(ok); }
bool near(float a, float b) { return std::abs(a-b) < .0001f; }
void advance(Zoom &z, float seconds, bool down) {
    for (int i=0; i<int(std::lround(seconds*100)); ++i) z.update(.01f,down);
}
void tap(Zoom &z) { z.update(0,false); z.update(0,true); z.update(.06f,false); advance(z,.5f,false); }
int main() {
    Zoom z;
    tap(z); check(near(z.magnification(),5.f));
    z.setMaximum(10.f); check(near(z.magnification(),1.f));
    tap(z); check(near(z.magnification(),10.f));
    const float fov = 1.285f;
    check(near(std::tan(fov*.5f)/std::tan(z.perspective(fov)*.5f),10.f));
    z.setMaximum(10.f); check(near(z.magnification(),10.f));
    tap(z); check(near(z.magnification(),1.f));
    z.update(0,true); advance(z,.70f,true);
    const float mid=z.magnification(); check(mid>1.f && mid<10.f);
    z.update(0,false); advance(z,1.f,false); check(near(z.magnification(),mid));
    z.reset(); check(near(z.magnification(),1.f));
    z.update(0,true); advance(z,2.f,true); z.update(0,false);
    check(near(z.magnification(),1.f));
    tap(z); check(near(z.magnification(),10.f));
    z.reset(); z.update(0,false); z.update(0,true); advance(z,2.f,true);
    check(near(z.magnification(),10.f));
    z.update(0,false); z.update(0,true); advance(z,.70f,true);
    const float out=z.magnification(); check(out>1.f && out<10.f);
    z.update(0,false); advance(z,1.f,false); check(near(z.magnification(),out));
    z.setMaximum(5.f); check(near(z.magnification(),1.f));
    z.update(0,true); advance(z,2.f,true); z.update(0,false); advance(z,1.f,false);
    check(near(z.magnification(),1.f));
    tap(z); check(near(z.magnification(),5.f));
    for(int i=0;i<1000;++i) { z.update(.013f,(i%43)<27); check(z.magnification()>=1.f && z.magnification()<=5.00001f); }
    std::cout << checks << " actual-header cap/tap/hold/projection/reset/downgrade checks passed\n";
}
```

The executable and intermediates are disposable. `results.json`, `portable.log` and `cap.log` preserve results; no permanent test added. This verifies actual C++ source behavior and inherited camera gates in a stubbed harness, not native gameplay or visible renderer acceptance.
