#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

// Meteorology follows elapsed calendar hours, independently of the hour of day.
// Units retain Storm's wind-speed convention, so sailing/AI keep the same contract.
namespace storm_weather {
struct Conditions {
    float coverage=.45f, rain=0.f, speed=10.f, angle=0.f, storm=0.f;
};
struct Engine {
    // Calendar and position are projections of the existing Environment/worldMap
    // owners, rebound at scene entry. They are not a second saved weather clock.
    double calendarHours=0.0;
    float mapX=0.f, mapZ=0.f;
    uint32_t seed=1;
    Conditions from, to, current;
    float elapsed=0.f, duration=300.f, requestRemaining=0.f, lightningRemaining=15.f;
    float requestedRain=0.f, requestedStorm=0.f, requestedWind=0.f;
    bool locked=false, requestedTornado=false, tornadoEmitted=false;
    unsigned month=1;
    static double calendar(int year, unsigned month, unsigned day, float hour) {
        constexpr unsigned before[]={0,0,31,59,90,120,151,181,212,243,273,304,334};
        // WorldMap's existing calendar uses a 28-day February, including leap years.
        return (double(year)*365.0+before[std::clamp(month,1u,12u)]+std::max(day,1u)-1.0)*24.0+hour;
    }
    Conditions sample(float x, float z) const {
        Conditions c=current;
        if (!std::isfinite(x) || !std::isfinite(z)) return c;
        // A continuous advected synoptic field: broad overlapping fronts, no map
        // particle RNG, event lifetime or additional persisted region simulation.
        const double phase=calendarHours*.035;
        const double px=x*.0055+phase, pz=z*.0065-phase*.7;
        const float field=.50f+.27f*std::sin(px+.65f*std::sin(pz*.6f))
                              +.23f*std::cos(pz+.5f*std::cos(px*.7f));
        float wet=std::clamp((field-.22f)/.62f,0.f,1.f);
        wet=wet*wet*(3.f-2.f*wet);
        // Explicit quest/event requests retain their existing whole-scene contract.
        const float requestBlend=locked?1.f:std::clamp(requestRemaining/12.f,0.f,1.f);
        wet+=(1.f-wet)*requestBlend;
        c.coverage=std::clamp(current.coverage*(.25f+.75f*wet),0.f,1.f);
        c.rain*=wet;
        float core=std::clamp((wet-.35f)/.65f,0.f,1.f);
        core=core*core*(3.f-2.f*core);
        c.storm*=core;
        c.speed*=.70f+.30f*wet;
        return c;
    }
    Conditions local() const { return sample(mapX,mapZ); }
    float random() {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        return float(seed >> 8) / 16777216.f;
    }
    void nextFront() {
        from=current; elapsed=0.f; duration=240.f+random()*480.f;
        const bool wetSeason=month>=5 && month<=10;
        to.coverage=.18f+random()*.80f;
        to.rain=random()<(wetSeason?.48f:.24f) ? .12f+random()*.88f : 0.f;
        if (to.rain>0.f) to.coverage=std::max(to.coverage,.65f+to.rain*.30f);
        to.storm=to.rain>.78f && random()<.30f ? .65f+random()*.35f : 0.f;
        to.speed=4.f+random()*13.f+to.storm*17.f;
        to.angle=current.angle+(random()-.5f)*1.4f;
    }
    void initialize(uint32_t value, unsigned season) {
        seed=value?value:1; month=season; current.angle=random()*6.2831853f;
        current.speed=7.f+random()*8.f; from=to=current; nextFront();
    }
    // A finite event fades back into the continuing weather; locks last until released.
    void request(float rain, float storm, float wind, float seconds, bool tornado, bool lock) {
        requestedRain=std::clamp(rain,0.f,1.f); requestedStorm=std::clamp(storm,0.f,1.f);
        requestedWind=std::clamp(wind,0.f,40.f); requestRemaining=std::max(0.f,seconds);
        locked=lock; requestedTornado=tornado; tornadoEmitted=false;
    }
    void release() { locked=false; requestRemaining=0.f; requestedTornado=false; }
    void advanceHours(float hours) {
        if (!std::isfinite(hours) || hours<=0.f) return;
        calendarHours+=hours;
        // Version-1 saves retain their model coordinate: 350 units per game hour.
        // Consume the full calendar interval, including accelerated map travel.
        double remaining=double(hours)*350.0;
        while (remaining>0.0) {
            const float step=static_cast<float>(std::min(remaining,1.0));
            updateStep(step); remaining-=step;
        }
    }
  private:
    void updateStep(float dt) {
        elapsed+=dt;
        if (elapsed>=duration) {
            const float remainder=elapsed-duration;
            nextFront(); elapsed=remainder;
        }
        const float t=std::clamp(elapsed/duration,0.f,1.f), blend=t*t*(3.f-2.f*t);
        Conditions target;
        target.coverage=from.coverage+(to.coverage-from.coverage)*blend;
        target.rain=from.rain+(to.rain-from.rain)*blend;
        target.speed=from.speed+(to.speed-from.speed)*blend;
        target.angle=from.angle+std::remainder(to.angle-from.angle,6.2831853f)*blend;
        target.storm=from.storm+(to.storm-from.storm)*blend;
        if (locked || requestRemaining>0.f) {
            target.rain=requestedRain; target.storm=requestedStorm; target.speed=requestedWind;
            target.coverage=std::max(target.coverage,.55f+requestedRain*.42f);
            if (!locked) requestRemaining=std::max(0.f,requestRemaining-dt);
        }
        if (!locked && requestRemaining<=0.f) requestedTornado=false;
        const float a=1.f-std::exp(-dt/12.f);
        current.coverage+=(target.coverage-current.coverage)*a;
        current.rain+=(target.rain-current.rain)*a;
        current.speed+=(target.speed-current.speed)*a;
        current.angle+=std::remainder(target.angle-current.angle,6.2831853f)*a;
        current.storm+=(target.storm-current.storm)*a;
    }
  public:
    bool lightning() {
        if (current.rain<.35f || current.storm<.15f || lightningRemaining>0.f) return false;
        lightningRemaining=4.f+random()*16.f; return true;
    }
    bool tornado() {
        if (!requestedTornado || tornadoEmitted || current.storm<.5f) return false;
        tornadoEmitted=true; return true;
    }
    template<class Attributes> void save(Attributes *a) const {
        a->SetAttributeUseDword("version",1); a->SetAttributeUseDword("seed",seed);
        a->SetAttributeUseDword("month",month);
        a->SetAttributeUseFloat("elapsed",elapsed); a->SetAttributeUseFloat("duration",duration);
        a->SetAttributeUseFloat("remaining",requestRemaining);
        a->SetAttributeUseFloat("lightning",lightningRemaining);
        a->SetAttributeUseFloat("rain",requestedRain); a->SetAttributeUseFloat("storm",requestedStorm);
        a->SetAttributeUseFloat("wind",requestedWind);
        a->SetAttributeUseDword("locked",locked); a->SetAttributeUseDword("tornado",requestedTornado);
        a->SetAttributeUseDword("emitted",tornadoEmitted);
        saveConditions(a->VerifyAttributeClass("from"),from);
        saveConditions(a->VerifyAttributeClass("to"),to);
        saveConditions(a->VerifyAttributeClass("current"),current);
    }
    template<class Attributes> bool load(Attributes *a) {
        if (!a || a->GetAttributeAsDword("version")!=1) return false;
        seed=a->GetAttributeAsDword("seed",1); if (!seed) seed=1;
        month=a->GetAttributeAsDword("month",1);
        elapsed=a->GetAttributeAsFloat("elapsed"); duration=std::max(1.f,a->GetAttributeAsFloat("duration",300.f));
        requestRemaining=a->GetAttributeAsFloat("remaining"); lightningRemaining=a->GetAttributeAsFloat("lightning",15.f);
        requestedRain=a->GetAttributeAsFloat("rain"); requestedStorm=a->GetAttributeAsFloat("storm");
        requestedWind=a->GetAttributeAsFloat("wind"); locked=a->GetAttributeAsDword("locked")!=0;
        requestedTornado=a->GetAttributeAsDword("tornado")!=0; tornadoEmitted=a->GetAttributeAsDword("emitted")!=0;
        from=loadConditions(a->GetAttributeClass("from")); to=loadConditions(a->GetAttributeClass("to"));
        current=loadConditions(a->GetAttributeClass("current")); return true;
    }
  private:
    template<class Attributes> static void saveConditions(Attributes *a, const Conditions &c) {
        a->SetAttributeUseFloat("coverage",c.coverage); a->SetAttributeUseFloat("rain",c.rain);
        a->SetAttributeUseFloat("speed",c.speed); a->SetAttributeUseFloat("angle",c.angle);
        a->SetAttributeUseFloat("storm",c.storm);
    }
    template<class Attributes> static Conditions loadConditions(Attributes *a) {
        if (!a) return {};
        return {a->GetAttributeAsFloat("coverage",.45f),a->GetAttributeAsFloat("rain"),
                a->GetAttributeAsFloat("speed",10.f),a->GetAttributeAsFloat("angle"),a->GetAttributeAsFloat("storm")};
    }
};
} // namespace storm_weather
