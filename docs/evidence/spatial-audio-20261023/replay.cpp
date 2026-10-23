#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_DEVICE_IO
#define MA_NO_DECODING
#define MA_NO_ENCODING
#define MA_NO_RESOURCE_MANAGER
#include "third_party/miniaudio.h"
#include "audio_voice_policy.h"
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <limits>

static double render(const char* alias,float minimum,float maximum,float distance,bool spatial,float volume) {
    ma_engine engine{};
    auto config=ma_engine_config_init();
    config.noDevice=MA_TRUE;config.channels=2;config.sampleRate=48000;
    if(ma_engine_init(&config,&engine)!=MA_SUCCESS)std::exit(2);
    constexpr size_t frames=16384;
    std::vector<float> input(frames),output(frames*2);
    for(size_t i=0;i<frames;++i)input[i]=.25f*std::sin(float(i)*.0575958653f);
    ma_audio_buffer buffer{};
    auto bufferConfig=ma_audio_buffer_config_init(ma_format_f32,1,frames,input.data(),nullptr);
    if(ma_audio_buffer_init(&bufferConfig,&buffer)!=MA_SUCCESS)std::exit(3);
    ma_sound sound{};
    if(ma_sound_init_from_data_source(&engine,&buffer,spatial?0:MA_SOUND_FLAG_NO_SPATIALIZATION,nullptr,&sound)!=MA_SUCCESS)std::exit(4);
    if(minimum>=0)ma_sound_set_min_distance(&sound,minimum);
    if(maximum>=0)ma_sound_set_max_distance(&sound,maximum);
#ifdef AUDIO_RANGE_POLICY
    const auto policy=storm::metal::audio::SpatialPolicyForAlias(alias,ma_sound_get_min_distance(&sound),ma_sound_get_max_distance(&sound));
#else
    const auto policy=storm::metal::audio::SpatialPolicyForAlias(alias);
#endif
    ma_sound_set_attenuation_model(&sound,policy.linearFalloff?ma_attenuation_model_linear:ma_attenuation_model_exponential);
    ma_sound_set_rolloff(&sound,policy.rolloff);
    ma_sound_set_volume(&sound,volume*(spatial?policy.volumeGain:1.f));
    ma_sound_set_position(&sound,0,0,-distance);
    if(ma_sound_start(&sound)!=MA_SUCCESS)std::exit(5);
    if(ma_engine_read_pcm_frames(&engine,output.data(),frames,nullptr)!=MA_SUCCESS)std::exit(6);
    double energy=0;
    for(size_t i=frames;i<output.size();++i)energy+=double(output[i])*output[i];
    ma_sound_uninit(&sound);ma_audio_buffer_uninit(&buffer);ma_engine_uninit(&engine);
    return std::sqrt(energy/frames);
}
int main(){
    struct Case{const char*alias;float minimum,maximum,volume;bool spatial;};
    const Case cases[]={
      {"coll_ship2rock",30,350,1,true},{"coll_ship2ship",30,350,1,true},
      {"ship_aground",11,30,1,true},{"mast_fall",50,250,1,true},
      {"ball_splash",10,1200,1,true},{"ship_onfire",50,100,.30f,true},
      {"fort_cannon_fire",550,1600,1,true},{"tornado",0,500,1,true},
      {"thunder",1000,8000,1,true},{"storm_sea_water",-1,-1,1,true},
      {"waitress",-1,-1,1,true},{"ship_bow",0,0,0,true},
      {"stereo_ui",30,350,1,false}
    };
    for(const auto& c:cases){
      const float distances[]={0,c.minimum>0?c.minimum:1.f,c.maximum>0?c.maximum*.5f:100.f,c.maximum>0?c.maximum:1000.f,c.maximum>0?c.maximum+1:2000.f,5000,10000};
      for(float distance:distances)
        std::printf("{\"alias\":\"%s\",\"min\":%.9g,\"max\":%.9g,\"volume\":%.9g,\"spatial\":%s,\"distance\":%.9g,\"rms\":%.12g}\n",c.alias,c.minimum,c.maximum,c.volume,c.spatial?"true":"false",distance,render(c.alias,c.minimum,c.maximum,distance,c.spatial,c.volume));
    }
}
