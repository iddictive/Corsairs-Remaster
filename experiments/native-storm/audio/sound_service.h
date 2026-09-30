#pragma once

#include "v_sound_service.h"
#include "third_party/miniaudio.h"

#include <array>
#include <string>
#include <unordered_map>
#include <vector>

class INIFILE;

class SoundService final : public VSoundService
{
  public:
    SoundService();
    ~SoundService() override;

    bool Init() override;
    uint32_t RunSection() override { return SECTION_EXECUTE; }
    void RunStart() override;
    void RunEnd() override {}

    TSD_ID SoundPlay(const char *, eSoundType, eVolumeType, bool, bool, bool, int32_t, const CVECTOR *, float,
                     float, int32_t, float, int32_t) override;
    TSD_ID SoundDuplicate(TSD_ID) override;
    void SoundSet3DParam(TSD_ID, eSoundMessage, const void *) override;
    void SoundStop(TSD_ID, int32_t) override;
    void SoundRelease(TSD_ID) override;
    void SoundSetVolume(TSD_ID, float) override;
    bool SoundIsPlaying(TSD_ID) override;
    int32_t SoundGetPosition(TSD_ID) override;
    void SoundRestart(TSD_ID) override;
    void SoundResume(TSD_ID, int32_t) override;
    void SetMasterVolume(float, float, float) override;
    void GetMasterVolume(float *, float *, float *) override;
    void SetPitch(float) override;
    float GetPitch() override { return pitch_; }
    void SetCameraPosition(const CVECTOR &) override;
    void SetCameraOrientation(const CVECTOR &, const CVECTOR &) override;
    void ResetScheme() override;
    bool SetScheme(const char *) override;
    bool AddScheme(const char *) override;
    void SetEnabled(bool) override;
    void LoadAliasFile(const char *) override;
    void SetActiveWithFade(bool) override;

  private:
    struct Slot {
        ma_sound sound{};
        std::string path;
        eSoundType type{};
        eVolumeType volumeType{};
        float volume{1.0f};
        uint16_t stamp{};
        bool initialized{};
        bool looped{};
        bool paused{};
        bool terminalStopping{};
        bool focusSuspended{};
        bool volumeSuspended{};
    };

    static constexpr size_t kSlots = 4095;
    std::array<Slot, kSlots> slots_{};
    ma_engine engine_{};
    bool initialized_{};
    bool enabled_{true};
    float fxVolume_{0.5f};
    float musicVolume_{0.5f};
    float speechVolume_{0.5f};
    float pitch_{1.0f};

    struct Choice { std::string name; float weight{1}; };
    struct Alias { std::vector<Choice> choices; float minDistance{-1}; float maxDistance{-1}; float volume{-1}; int priority{128}; };
    struct SchemeChannel { std::string name; int minDelay{}; int maxDelay{}; int remaining{}; float volume{1}; bool looped{}; TSD_ID id{}; };
    std::unordered_map<std::string, Alias> aliases_;
    std::vector<SchemeChannel> schemes_;

    Slot *Find(TSD_ID);
    void Release(Slot &);
    float EffectiveVolume(const Slot &) const;
    void ApplyVolumeLifecycle(Slot &, bool allowStart = true);
    std::string ResolvePath(const char *) const;
    void InitAliases();
    void AddAlias(INIFILE &, const char *);
    std::string Select(const Alias &) const;
    bool AddSchemeChannel(const char *, bool);
    void ProcessSchemes();
};
