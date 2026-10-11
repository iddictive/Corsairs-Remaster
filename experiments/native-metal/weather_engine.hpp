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
struct Front {
    uint32_t id=0;
    float x=0.f, z=0.f, radius=0.f, coverage=0.f, rain=0.f, storm=0.f, windAngle=0.f, age=0.f;
};
struct Engine {
    // Calendar and position are projections of the existing Environment/worldMap
    // owners, rebound at scene entry. They are not a second saved weather clock.
    double calendarHours=0.0;
    float mapX=0.f, mapZ=0.f;
    uint32_t climateSeed=1;
    float worldSizeX=2000.f, worldSizeZ=2000.f;
    static constexpr unsigned frontCount=12;
    // Map travel advances one calendar hour per real second. Weather systems
    // evolve over days, so their coherent cohort clock is slower than that chart.
    static constexpr double cohortHours=16.0;
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
    void setWorldSize(float x, float z) {
        if (std::isfinite(x) && std::isfinite(z) && x>0.f && z>0.f) { worldSizeX=x; worldSizeZ=z; }
    }
    bool front(unsigned index, Front &out) const {
        if (index>=frontCount || !std::isfinite(calendarHours)) return false;
        const double cohortClock=calendarHours/cohortHours;
        const auto birth=static_cast<int64_t>(std::floor(cohortClock))-index;
        const float age=static_cast<float>(cohortClock-double(birth));
        const uint32_t id=static_cast<uint32_t>(birth);
        const auto value=[this,id](uint32_t channel) {
            uint32_t h=id^climateSeed^(channel*0x9e3779b9u);
            h^=h>>16; h*=0x7feb352du; h^=h>>15; h*=0x846ca68bu; h^=h>>16;
            return float(h>>8)/16777216.f;
        };
        const float growth=smooth(age/1.5f), decay=smooth((12.f-age)/3.f);
        const float life=growth*decay;
        const float direction=.35f+.70f*value(1), drift=80.f+80.f*value(2);
        // Transport the same moisture cell under its wind, with smooth birth/death.
        const float travel=(age-6.f)*drift;
        out.id=id; out.age=age*static_cast<float>(cohortHours); out.windAngle=direction;
        out.x=(value(3)-.5f)*worldSizeX+std::sin(direction)*travel;
        out.z=(value(4)-.5f)*worldSizeZ+std::cos(direction)*travel;
        out.radius=std::min(worldSizeX,worldSizeZ)*(.12f+.10f*value(5));
        out.coverage=(.45f+.50f*value(6))*life;
        // Season belongs to the cell's birth, so month rollover cannot switch it.
        double yearHour=std::fmod(double(birth)*cohortHours,365.0*24.0);
        if (yearHour<0.0) yearHour+=365.0*24.0;
        const bool wetSeason=yearHour>=120.0*24.0 && yearHour<304.0*24.0;
        const float moisture=value(7)<(wetSeason?.48f:.24f)?(.30f+.70f*value(8)):0.f;
        // A slow synoptic cloud system contains short convective showers, not
        // days of uninterrupted rain. Parcel phases use the same saved seed/time.
        const float showerPhase=std::fmod(out.age+24.f*value(9),24.f);
        const float shower=smooth(showerPhase/2.f)*smooth((8.f-showerPhase)/2.f);
        out.rain=moisture*life*shower;
        // Deep moist convection creates a thunderstorm core; an independent
        // second lottery used to hide most otherwise suitable rain systems.
        out.storm=smooth((moisture-.40f)/.30f)*life*shower;
        return true;
    }
    Conditions sample(float x, float z) const {
        Conditions c=current;
        if (!std::isfinite(x) || !std::isfinite(z)) return c;
        float cloud=.08f+.12f*current.coverage, rain=0.f, storm=0.f;
        float windX=.15f*std::sin(current.angle), windZ=.15f*std::cos(current.angle);
        for (unsigned i=0;i<frontCount;++i) {
            Front f; if (!front(i,f)) continue;
            const float dx=(x-f.x)/f.radius, dz=(z-f.z)/f.radius;
            const float kernel=smooth(1.f-dx*dx-dz*dz);
            const float cover=f.coverage*kernel;
            cloud=1.f-(1.f-cloud)*(1.f-cover);
            rain=1.f-(1.f-rain)*(1.f-f.rain*kernel*kernel);
            storm=std::max(storm,f.storm*kernel*kernel);
            windX+=cover*std::sin(f.windAngle); windZ+=cover*std::cos(f.windAngle);
        }
        // Current rain/storm retain only the existing smoothed explicit-event overlay.
        const float requestBlend=locked?1.f:std::clamp(requestRemaining/12.f,0.f,1.f);
        c.coverage=cloud+(current.coverage-cloud)*requestBlend;
        c.rain=1.f-(1.f-std::clamp(current.rain,0.f,1.f))*(1.f-rain*(1.f-requestBlend));
        c.storm=std::max(current.storm,storm*(1.f-requestBlend));
        c.speed=current.speed+18.f*storm*(1.f-requestBlend);
        const float direction=std::atan2(windX,windZ);
        c.angle=direction+std::remainder(current.angle-direction,6.2831853f)*requestBlend;
        return c;
    }
    Conditions local() const { return sample(mapX,mapZ); }
    float random() {
        seed ^= seed << 13; seed ^= seed >> 17; seed ^= seed << 5;
        return float(seed >> 8) / 16777216.f;
    }
    void nextFront() {
        from=current; elapsed=0.f; duration=240.f+random()*480.f;
        to.coverage=.18f+random()*.80f;
        // Ordinary precipitation is owned by the regional cells, never a global gate.
        to.rain=0.f; to.storm=0.f;
        to.speed=4.f+random()*13.f;
        to.angle=current.angle+(random()-.5f)*1.4f;
    }
    void initialize(uint32_t value, unsigned season) {
        seed=value?value:1; climateSeed=seed; month=season; current.angle=random()*6.2831853f;
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
    static float smooth(float value) {
        const float t=std::clamp(value,0.f,1.f); return t*t*(3.f-2.f*t);
    }
    void updateStep(float dt) {
        elapsed+=dt;
        if (elapsed>=duration) {
            const float remainder=elapsed-duration;
            nextFront(); elapsed=remainder;
        }
        const float t=std::clamp(elapsed/duration,0.f,1.f), blend=t*t*(3.f-2.f*t);
        Conditions target;
        target.coverage=from.coverage+(to.coverage-from.coverage)*blend;
        target.rain=0.f;
        target.speed=from.speed+(to.speed-from.speed)*blend;
        target.angle=from.angle+std::remainder(to.angle-from.angle,6.2831853f)*blend;
        target.storm=0.f;
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
        const auto c=local();
        if (c.rain<.35f || c.storm<.15f || lightningRemaining>0.f) return false;
        lightningRemaining=4.f+random()*16.f; return true;
    }
    bool tornado() {
        if (!requestedTornado || tornadoEmitted || current.storm<.5f) return false;
        tornadoEmitted=true; return true;
    }
    template<class Attributes> void save(Attributes *a) const {
        a->SetAttributeUseDword("version",1); a->SetAttributeUseDword("seed",seed);
        a->SetAttributeUseDword("climateSeed",climateSeed);
        a->SetAttributeUseFloat("worldSizeX",worldSizeX); a->SetAttributeUseFloat("worldSizeZ",worldSizeZ);
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
        climateSeed=a->GetAttributeAsDword("climateSeed",seed); if (!climateSeed) climateSeed=seed;
        setWorldSize(a->GetAttributeAsFloat("worldSizeX",2000.f),a->GetAttributeAsFloat("worldSizeZ",2000.f));
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
