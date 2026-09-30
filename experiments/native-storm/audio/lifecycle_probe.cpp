#include "third_party/miniaudio.h"

#include <chrono>
#include <cstdio>
#include <thread>

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        std::fprintf(stderr, "usage: native-audio-probe FILE\n");
        return 2;
    }

    ma_engine engine{};
    ma_engine_config config = ma_engine_config_init();
    if (ma_engine_init(&config, &engine) != MA_SUCCESS)
    {
        std::fprintf(stderr, "engine init failed\n");
        return 3;
    }

    ma_sound sound{};
    const ma_result loaded = ma_sound_init_from_file(
        &engine, argv[1], MA_SOUND_FLAG_STREAM | MA_SOUND_FLAG_NO_SPATIALIZATION, nullptr, nullptr, &sound);
    if (loaded != MA_SUCCESS)
    {
        std::fprintf(stderr, "decode failed: %d\n", loaded);
        ma_engine_uninit(&engine);
        return 4;
    }

    ma_sound_set_volume(&sound, 0.08f);
    ma_sound_set_looping(&sound, MA_TRUE);
    if (ma_sound_start(&sound) != MA_SUCCESS)
    {
        std::fprintf(stderr, "start failed\n");
        ma_sound_uninit(&sound);
        ma_engine_uninit(&engine);
        return 5;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(180));
    const bool started = ma_sound_is_playing(&sound) == MA_TRUE;
    ma_sound_stop(&sound);
    const bool stopped = ma_sound_is_playing(&sound) == MA_FALSE;
    const bool seeked = ma_sound_seek_to_pcm_frame(&sound, 0) == MA_SUCCESS;
    ma_sound_set_looping(&sound, MA_FALSE);
    ma_sound_uninit(&sound);
    ma_engine_uninit(&engine);

    if (!started || !stopped || !seeked)
    {
        std::fprintf(stderr, "lifecycle failed: started=%d stopped=%d seeked=%d\n", started, stopped, seeked);
        return 6;
    }
    std::puts("PASS: decoded, played, stopped, sought, and released");
    return 0;
}
