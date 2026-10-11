#pragma once
#include <chrono>

namespace {
float WeatherRainIntensity(float rain)
{
    // The native wet/dry admission also owns emission; sub-threshold front tails
    // must not keep visible drops alive after the condition has become dry.
    return std::clamp((rain-.03f)/.97f,0.f,1.f);
}
}

// Included by weather.cpp after WEATHER's declarations. No renderer-side weather owner.
void WEATHER::SaveWeatherEngine()
{
    if (ATTRIBUTES::IsLive(weatherState)) weatherEngine.save(weatherState);
}

uint64_t WEATHER::ProcessMessage(MESSAGE &message)
{
    const auto code=message.Long();
    if (code==1464336385) {
        weatherState=message.AttributePointer();
        const auto season=static_cast<unsigned>(message.Long());
        if (!weatherEngine.load(weatherState))
            weatherEngine.initialize(static_cast<uint32_t>(std::chrono::steady_clock::now().time_since_epoch().count()),season);
        const int year=message.Long();
        const unsigned day=static_cast<unsigned>(message.Long());
        const float hour=message.Float();
        weatherEngine.calendarHours=storm_weather::Engine::calendar(year,season,day,hour);
        weatherEngine.mapX=message.Float(); weatherEngine.mapZ=message.Float();
        auto *palette=weatherState->GetAttributeClass("palette");
        weatherPaletteValid=palette!=nullptr;
        for (unsigned hour=0;hour<24;++hour) {
            char key[8]; snprintf(key,sizeof(key),"h%u",hour);
            auto *entry=palette?palette->GetAttributeClass(key):nullptr;
            weatherPaletteValid &= entry!=nullptr;
            weatherPalette[hour]={entry?entry->GetAttributeAsDword("fog"):0u,
                                  entry?entry->GetAttributeAsDword("ambient"):0u,
                                  entry?entry->GetAttributeAsDword("sun"):0u};
        }
        weatherEngine.month=season; weatherRainStatus=weatherStormStatus=-1;
        SaveWeatherEngine(); UpdateVisualState(0);
    } else if (code==1464336386) {
        const auto field=message.Long(); auto *out=message.ScriptVariablePointer();
        if (out) {
            const auto c=weatherEngine.local(); float value=0.f;
            switch(field) {
            case 0: value=c.angle; break; case 1: value=c.speed; break;
            case 2: value=c.rain; break; case 3: value=c.storm; break;
            case 4: value=visualState.fogDensity; break; case 5: value=c.coverage; break;
            }
            out->Set(value);
        }
    } else if (code==1464336387) {
        const float rain=message.Float(), storm=message.Float(), wind=message.Float(), duration=message.Float();
        const bool tornado=message.Long()!=0, lock=message.Long()!=0;
        weatherEngine.request(rain,storm,wind,duration,tornado,lock); SaveWeatherEngine();
    } else if (code==1464336388) { weatherOutside=message.Long()!=0;
    } else if (code==1464336389) { SaveWeatherEngine();
    } else if (code==1464336390) { weatherEngine.locked=false; SaveWeatherEngine();
    } else if (code==1464336391) {
        weatherEngine.mapX=message.Float(); weatherEngine.mapZ=message.Float();
    }
    return 0;
}

void WEATHER::AdvanceWeatherHours(float hours)
{
    if (ATTRIBUTES::IsLive(weatherState)) weatherEngine.advanceHours(hours);
}

void WEATHER::UpdateWeatherEngine(uint32_t deltaTime)
{
    if (!ATTRIBUTES::IsLive(weatherState)) { weatherState=nullptr; return; }
    const float dt=static_cast<float>(deltaTime)*.001f;
    AdvanceWeatherHours(static_cast<float>(deltaTime)*fFloats[whf_time_speed]);
    // Lightning cadence and checkpoint I/O stay on the entity simulation clock.
    weatherEngine.lightningRemaining-=dt; weatherCheckpoint+=dt;
    if (weatherCheckpoint>=5.f) { weatherCheckpoint=0.f; SaveWeatherEngine(); }
    const auto c=weatherEngine.local();
    const int rain=WeatherRainIntensity(c.rain)>0.f, storm=c.storm>.5f;
    if (rain!=weatherRainStatus || storm!=weatherStormStatus) {
        weatherRainStatus=rain; weatherStormStatus=storm;
        core.Event("NativeWeatherChanged","ll",rain,storm);
    }
    if (!weatherOutside) return;
    if (weatherEngine.lightning()) {
        CVECTOR position, angle; float fov; pRS->GetCamera(position,angle,fov);
        const float azimuth=weatherEngine.random()*PIm2, distance=1200.f+weatherEngine.random()*2400.f;
        core.Event("NativeWeatherLightning","fffl",position.x+distance*sinf(azimuth),600.f,
                   position.z+distance*cosf(azimuth),static_cast<int32_t>(weatherEngine.random()*4.f));
    }
    if (weatherEngine.tornado()) core.Event("NativeWeatherTornado","f",300.f);
}

void WEATHER::ApplyWeatherConditions()
{
    if (!weatherState) return;
    const auto c=weatherEngine.local();
    fFloats[whf_wind_angle]=c.angle; fFloats[whf_wind_speed]=c.speed;
    fFloats[whf_fog_density]=.000005f+c.rain*.00004f;
    visualRainTarget=WeatherRainIntensity(c.rain)*5000.f;
    const float hour=storm_weather_visual::wrapDayHour(fFloats[whf_time_counter]);
    const int first=static_cast<int>(hour), next=(first+1)%24;
    const float fraction=hour-first;
    const auto color=[this,fraction,first,next](unsigned channel,uint32_t fallback) {
        const uint32_t start=weatherPaletteValid?weatherPalette[first][channel]:fallback;
        const uint32_t end=weatherPaletteValid?weatherPalette[next][channel]:start;
        uint32_t out=0;
        for(unsigned shift=0;shift<24;shift+=8) {
            float low=float((start>>shift)&255u), high=float((end>>shift)&255u);
            out|=uint32_t(low+(high-low)*fraction)<<shift;
        } return out;
    };
    dwColors[whc_fog_color]=color(0,dwColors[whc_fog_color]);
    dwColors[whc_sun_ambient]=color(1,dwColors[whc_sun_ambient]);
    dwColors[whc_sun_color]=color(2,dwColors[whc_sun_color]);
}
