#include "sound_service.h"

#include "core.h"
#include "v_file_service.h"
#include "vma.hpp"
#include "dx9render.h"
#include "math3d/matrix.h"

#include <algorithm>
#include <filesystem>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <cmath>

CREATE_SERVICE(SoundService)

SoundService::SoundService() = default;

SoundService::~SoundService()
{
    for (auto &slot : slots_) Release(slot);
    if (initialized_) ma_engine_uninit(&engine_);
}

bool SoundService::Init()
{
    const ma_engine_config config = ma_engine_config_init();
    initialized_ = ma_engine_init(&config, &engine_) == MA_SUCCESS;
    core.Trace(initialized_ ? "Native audio: miniaudio initialized" : "Native audio: device initialization failed");
    if (initialized_) InitAliases();
    return initialized_;
}

void SoundService::RunStart()
{
    if (!initialized_) return;
    if (auto *rs = static_cast<VDX9RENDER *>(core.GetService("dx9render")))
    {
        CMatrix view;
        rs->GetTransform(D3DTS_VIEW, (D3DMATRIX *)&view);
        view.Transposition();
        const CVECTOR nose = view.Vz();
        const CVECTOR head = view.Vy();
        const CVECTOR pos = view.Pos();
        if (std::isfinite(pos.x) && std::isfinite(pos.y) && std::isfinite(pos.z))
        {
            SetCameraPosition(pos);
            SetCameraOrientation(nose, head);
        }
    }
    for (auto &slot : slots_)
    {
        if (!slot.initialized) continue;
        if (slot.terminalStopping && !ma_sound_is_playing(&slot.sound)) { Release(slot); continue; }
        if (!slot.looped && !slot.paused && !slot.focusSuspended && ma_sound_at_end(&slot.sound)) Release(slot);
    }
    ProcessSchemes();
}

static std::string Lower(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string SoundService::ResolvePath(const char *name) const
{
    if (!name || !*name) return {};
    std::string value(name);
    std::replace(value.begin(), value.end(), '\\', '/');
    std::filesystem::path path(value);
    const std::string lower = Lower(value);
    if (!path.is_absolute() && lower.rfind("resource/sounds/", 0) != 0) path = std::filesystem::path("resource/sounds") / path;
    return path.lexically_normal().string();
}

float SoundService::EffectiveVolume(const Slot &slot) const
{
    float group = fxVolume_;
    if (slot.volumeType == VOLUME_MUSIC) group = musicVolume_;
    else if (slot.volumeType == VOLUME_SPEECH) group = speechVolume_;
    return enabled_ ? slot.volume * slot.mixGain * group : 0.0f;
}

size_t SoundService::ActiveVoiceCount(storm::metal::audio::VoiceClass voiceClass) const
{
    return static_cast<size_t>(std::count_if(slots_.begin(), slots_.end(), [voiceClass](const Slot &slot) {
        return slot.initialized && !slot.terminalStopping && slot.voiceClass == voiceClass;
    }));
}

void SoundService::RebalanceCannonVoices()
{
    const float gain = storm::metal::audio::CannonMixGain(
        ActiveVoiceCount(storm::metal::audio::VoiceClass::cannon));
    for (auto &slot : slots_)
    {
        if (!slot.initialized || slot.voiceClass != storm::metal::audio::VoiceClass::cannon)
            continue;
        slot.mixGain = gain;
        ma_sound_set_volume(&slot.sound, EffectiveVolume(slot));
    }
}

void SoundService::PrepareChargeReplacement()
{
    for (auto &slot : slots_)
    {
        if (!slot.initialized || slot.voiceClass != storm::metal::audio::VoiceClass::chargeSelection)
            continue;
        if (slot.terminalStopping)
        {
            Release(slot);
            continue;
        }
        ma_sound_stop_with_fade_in_milliseconds(&slot.sound,
                                                 storm::metal::audio::kChargeReplacementFadeMs);
        slot.terminalStopping = true;
    }
}

void SoundService::ApplyVolumeLifecycle(Slot &slot, bool allowStart)
{
    const float effective = EffectiveVolume(slot);
    ma_sound_set_volume(&slot.sound, effective);
    if (!slot.looped || slot.terminalStopping)
        return;
    constexpr float EPSILON = 0.001f;
    constexpr ma_uint64 FADE_MS = 200;
    if (effective <= EPSILON)
    {
        if (!slot.volumeSuspended)
        {
            ma_sound_stop_with_fade_in_milliseconds(&slot.sound, FADE_MS);
            slot.volumeSuspended = true;
        }
        return;
    }
    if (!allowStart || slot.paused || slot.focusSuspended)
        return;
    if (slot.volumeSuspended)
    {
        ma_sound_reset_stop_time_and_fade(&slot.sound);
        ma_sound_seek_to_pcm_frame(&slot.sound, 0);
        ma_sound_set_fade_in_milliseconds(&slot.sound, 0.f, effective, FADE_MS);
        slot.volumeSuspended = false;
    }
    if (!ma_sound_is_playing(&slot.sound))
        ma_sound_start(&slot.sound);
}

TSD_ID SoundService::SoundPlay(const char *name, eSoundType type, eVolumeType volumeType, bool simpleCache,
                               bool looped, bool, int32_t fadeMs, const CVECTOR *position, float minDistance,
                               float maxDistance, int32_t, float volume, int32_t)
{
    if (!initialized_) return SOUND_INVALID_ID;
    std::string selected = name ? name : "";
    const std::string aliasKey = Lower(selected);
    const auto voiceClass = storm::metal::audio::ClassifyAlias(aliasKey);
    const bool spatial = type == PCM_3D || type == MP3_3D;
    const bool clusteredImpact = spatial && position && !looped && !simpleCache && volumeType == VOLUME_FX &&
        storm::metal::audio::IsClusteredImpactAlias(aliasKey);
    const auto impactStarted = clusteredImpact ? std::chrono::steady_clock::now() :
        std::chrono::steady_clock::time_point{};
    if (clusteredImpact)
    {
        for (const auto &slot : slots_)
        {
            if (!slot.initialized || slot.terminalStopping || slot.paused || slot.focusSuspended ||
                slot.type != type || slot.impactAlias != aliasKey || ma_sound_at_end(&slot.sound))
                continue;
            const float dx = slot.impactPosition.x - position->x;
            const float dy = slot.impactPosition.y - position->y;
            const float dz = slot.impactPosition.z - position->z;
            if (dx * dx + dy * dy + dz * dz > storm::metal::audio::kImpactClusterRadiusSquared)
                continue;
            if (impactStarted - slot.impactStarted <
                std::chrono::milliseconds(storm::metal::audio::kImpactClusterIntervalMs))
                return SOUND_INVALID_ID;
        }
    }
    if (voiceClass == storm::metal::audio::VoiceClass::cannon &&
        !storm::metal::audio::AdmitCannonVoice(ActiveVoiceCount(voiceClass)))
        return SOUND_INVALID_ID;
    if (voiceClass == storm::metal::audio::VoiceClass::chargeSelection)
        PrepareChargeReplacement();
    if (selected.find('\\') == std::string::npos && selected.find('/') == std::string::npos)
    {
        const auto found = aliases_.find(Lower(selected));
        if (found != aliases_.end())
        {
            selected = Select(found->second);
            if (found->second.minDistance >= 0) minDistance = found->second.minDistance;
            if (found->second.maxDistance >= 0) maxDistance = found->second.maxDistance;
            if (found->second.volume >= 0) volume = found->second.volume;
        }
    }
    const std::string path = ResolvePath(selected.c_str());
    if (path.empty()) return SOUND_INVALID_ID;
    auto it = std::find_if(slots_.begin(), slots_.end(), [](const Slot &slot) { return !slot.initialized; });
    if (it == slots_.end()) return SOUND_INVALID_ID;

    // Short PCM effects must be decoded before start. Streaming every sound
    // adds decoder/file latency between the animation event and the first sample.
    ma_uint32 flags = (type == MP3_STEREO || type == MP3_3D || looped || volumeType == VOLUME_MUSIC)
                          ? MA_SOUND_FLAG_STREAM
                          : 0;
    if (!spatial) flags |= MA_SOUND_FLAG_NO_SPATIALIZATION;
    if (ma_sound_init_from_file(&engine_, path.c_str(), flags, nullptr, nullptr, &it->sound) != MA_SUCCESS)
        return SOUND_INVALID_ID;

    const size_t index = static_cast<size_t>(it - slots_.begin());
    const TSD_ID id = TSD_ID::createId(static_cast<uint16_t>(index));
    it->path = path;
    if (clusteredImpact)
    {
        it->impactAlias = aliasKey;
        it->impactPosition = *position;
        it->impactStarted = impactStarted;
    }
    it->type = type;
    it->volumeType = volumeType;
    it->volume = volume;
    it->voiceClass = voiceClass;
    it->stamp = id.stamp();
    it->initialized = true;
    it->looped = looped;
    it->paused = simpleCache;
    ma_sound_set_looping(&it->sound, looped ? MA_TRUE : MA_FALSE);
    ma_sound_set_pitch(&it->sound, pitch_);
    ma_sound_set_volume(&it->sound, EffectiveVolume(*it));
    if (spatial)
    {
        ma_sound_set_attenuation_model(&it->sound, ma_attenuation_model_exponential);
        ma_sound_set_rolloff(&it->sound, 0.45f);
    }
    if (spatial && position) ma_sound_set_position(&it->sound, position->x, position->y, position->z);
    if (spatial && minDistance >= 0) ma_sound_set_min_distance(&it->sound, minDistance);
    if (spatial && maxDistance >= 0) ma_sound_set_max_distance(&it->sound, maxDistance);
    if (fadeMs > 0) ma_sound_set_fade_in_milliseconds(&it->sound, 0, EffectiveVolume(*it), fadeMs);
    // Looped sounds created at effective volume zero are kept stopped until SoundSetVolume raises them.
    // This avoids silent streaming of ambient ship loops at game start.
    it->volumeSuspended = looped && EffectiveVolume(*it) <= 0.001f;
    if (voiceClass == storm::metal::audio::VoiceClass::cannon)
        RebalanceCannonVoices();
    if (!simpleCache && !it->volumeSuspended && ma_sound_start(&it->sound) != MA_SUCCESS) { Release(*it); return SOUND_INVALID_ID; }
    return id;
}

SoundService::Slot *SoundService::Find(TSD_ID id)
{
    if (static_cast<int32_t>(id) == SOUND_INVALID_ID) return nullptr;
    const auto index = id.index();
    if (index >= slots_.size()) return nullptr;
    auto &slot = slots_[index];
    return slot.initialized && slot.stamp == id.stamp() ? &slot : nullptr;
}

void SoundService::Release(Slot &slot)
{
    const bool rebalanceCannons = slot.initialized &&
        slot.voiceClass == storm::metal::audio::VoiceClass::cannon;
    if (slot.initialized) ma_sound_uninit(&slot.sound);
    slot = {};
    if (rebalanceCannons) RebalanceCannonVoices();
}

TSD_ID SoundService::SoundDuplicate(TSD_ID source)
{
    auto *slot = Find(source);
    if (!slot) return SOUND_INVALID_ID;
    return SoundPlay(slot->path.c_str(), slot->type, slot->volumeType, false, slot->looped, false, 0, nullptr, -1, -1,
                     0, slot->volume, 128);
}

void SoundService::SoundSet3DParam(TSD_ID id, eSoundMessage message, const void *value)
{
    auto *slot = Find(id); if (!slot || !value) return;
    if (message == SM_POSITION) { const auto &p = *static_cast<const CVECTOR *>(value); ma_sound_set_position(&slot->sound, p.x, p.y, p.z); }
    else if (message == SM_MIN_DISTANCE) ma_sound_set_min_distance(&slot->sound, *static_cast<const float *>(value));
    else if (message == SM_MAX_DISTANCE) ma_sound_set_max_distance(&slot->sound, *static_cast<const float *>(value));
}

void SoundService::SoundStop(TSD_ID id, int32_t fadeMs)
{
    if (static_cast<int32_t>(id) == SOUND_INVALID_ID) { for (auto &s : slots_) if (s.initialized) SoundStop(TSD_ID(static_cast<uint32_t>((static_cast<uint32_t>(s.stamp) << 16) | static_cast<uint32_t>(&s - slots_.data() + 1))), fadeMs); return; }
    auto *slot = Find(id); if (!slot) return;
    if (fadeMs > 0) { ma_sound_stop_with_fade_in_milliseconds(&slot->sound, fadeMs); slot->terminalStopping = true; }
    else { ma_sound_stop(&slot->sound); Release(*slot); }
}

void SoundService::SoundRelease(TSD_ID id) { if (auto *slot = Find(id)) Release(*slot); }
void SoundService::SoundSetVolume(TSD_ID id, float volume)
{
    auto *s = Find(id);
    if (!s) return;
    s->volume = volume;
    ApplyVolumeLifecycle(*s);
    if (s->voiceClass == storm::metal::audio::VoiceClass::cannon)
        RebalanceCannonVoices();
}
bool SoundService::SoundIsPlaying(TSD_ID id) { auto *s = Find(id); return s && ma_sound_is_playing(&s->sound); }
int32_t SoundService::SoundGetPosition(TSD_ID id) { float seconds{}; auto *s = Find(id); return s && ma_sound_get_cursor_in_seconds(&s->sound, &seconds) == MA_SUCCESS ? static_cast<int32_t>(seconds * 1000) : 0; }
void SoundService::SoundRestart(TSD_ID id) { if (auto *s = Find(id)) { s->paused = false; s->terminalStopping = false; ma_sound_reset_stop_time_and_fade(&s->sound); ma_sound_seek_to_pcm_frame(&s->sound, 0); ApplyVolumeLifecycle(*s); } }
void SoundService::SoundResume(TSD_ID id, int32_t fadeMs) { if (auto *s = Find(id)) { s->paused = false; s->terminalStopping = false; ma_sound_reset_stop_time_and_fade(&s->sound); if (fadeMs > 0) ma_sound_set_fade_in_milliseconds(&s->sound, 0, EffectiveVolume(*s), fadeMs); ApplyVolumeLifecycle(*s); } }

void SoundService::SetMasterVolume(float fx, float music, float speech)
{
    fxVolume_ = fx; musicVolume_ = music; speechVolume_ = speech;
    RebalanceCannonVoices();
    for (auto &slot : slots_) if (slot.initialized) ApplyVolumeLifecycle(slot);
}
void SoundService::GetMasterVolume(float *fx, float *music, float *speech) { if (fx) *fx = fxVolume_; if (music) *music = musicVolume_; if (speech) *speech = speechVolume_; }
void SoundService::SetPitch(float pitch) { pitch_ = pitch; for (auto &s : slots_) if (s.initialized) ma_sound_set_pitch(&s.sound, pitch); }
void SoundService::SetCameraPosition(const CVECTOR &p) { if (initialized_) ma_engine_listener_set_position(&engine_, 0, p.x, p.y, p.z); }
void SoundService::SetCameraOrientation(const CVECTOR &nose, const CVECTOR &head) { if (initialized_) { ma_engine_listener_set_direction(&engine_, 0, nose.x, nose.y, nose.z); ma_engine_listener_set_world_up(&engine_, 0, head.x, head.y, head.z); } }
void SoundService::SetEnabled(bool enabled) { enabled_ = enabled; RebalanceCannonVoices(); for (auto &s : slots_) if (s.initialized) ApplyVolumeLifecycle(s); }
void SoundService::SetActiveWithFade(bool active)
{
    if (!initialized_) return;
    for (auto &s : slots_)
    {
        if (!s.initialized || s.terminalStopping) continue;
        if (active && s.focusSuspended) { s.focusSuspended = false; ApplyVolumeLifecycle(s); }
        else if (!active && ma_sound_is_playing(&s.sound)) { ma_sound_stop(&s.sound); s.focusSuspended = true; }
    }
}

std::string SoundService::Select(const Alias &alias) const
{
    float total = 0; for (const auto &choice : alias.choices) total += std::max(0.0f, choice.weight);
    if (total <= 0 || alias.choices.empty()) return {};
    float pick = total * (static_cast<float>(std::rand()) / static_cast<float>(RAND_MAX));
    for (const auto &choice : alias.choices) { pick -= std::max(0.0f, choice.weight); if (pick <= 0) return choice.name; }
    return alias.choices.back().name;
}

void SoundService::AddAlias(INIFILE &ini, const char *section)
{
    if (!section) return;
    Alias alias;
    alias.maxDistance = ini.GetFloat(section, "maxDistance", -1.0f);
    alias.minDistance = ini.GetFloat(section, "minDistance", -1.0f);
    alias.volume = ini.GetFloat(section, "volume", -1.0f);
    alias.priority = ini.GetInt(section, "prior", 128);
    char value[COMMON_STRING_LENGTH]{};
    auto add = [&](const char *text) { std::string s(text); float weight = 1; const auto comma = s.find(','); if (comma != std::string::npos) { weight = std::strtof(s.c_str() + comma + 1, nullptr); s.resize(comma); } alias.choices.push_back({s, weight}); };
    if (ini.ReadString(section, "name", value, sizeof(value), "")) { add(value); while (ini.ReadStringNext(section, "name", value, sizeof(value))) add(value); }
    if (!alias.choices.empty()) aliases_[Lower(section)] = std::move(alias);
}

void SoundService::LoadAliasFile(const char *filename)
{
    std::string path = "resource\\ini\\aliases\\"; path += filename ? filename : "";
    auto ini = fio->OpenIniFile(path.c_str()); if (!ini) return;
    char section[128]{};
    if (ini->GetSectionName(section, sizeof(section))) { AddAlias(*ini, section); while (ini->GetSectionNameNext(section, sizeof(section))) AddAlias(*ini, section); }
}

void SoundService::InitAliases()
{
    for (const auto &name : fio->_GetPathsOrFilenamesByMask("resource\\ini\\aliases\\", "*.ini", false)) LoadAliasFile(name.c_str());
    core.Trace("Native audio: loaded %zu aliases", aliases_.size());
}

void SoundService::ResetScheme() { for (auto &channel : schemes_) if (static_cast<int32_t>(channel.id) != SOUND_INVALID_ID) SoundRelease(channel.id); schemes_.clear(); }
bool SoundService::SetScheme(const char *name) { ResetScheme(); return AddScheme(name); }

bool SoundService::AddSchemeChannel(const char *text, bool looped)
{
    if (!text) return false; SchemeChannel channel; std::string value(text); const auto comma = value.find(',');
    channel.name = comma == std::string::npos ? value : value.substr(0, comma); channel.looped = looped;
    channel.minDelay = 0; channel.maxDelay = 0x7fffffff; channel.volume = 1;
    if (comma != std::string::npos) { int lo{}, hi{}; float volume{}; const int n = std::sscanf(value.c_str() + comma + 1, "%d, %d, %f", &lo, &hi, &volume); if (n == 1) { channel.maxDelay = lo * 1000; } else if (n >= 2) { channel.minDelay = lo * 1000; channel.maxDelay = hi * 1000; } if (n == 3) channel.volume = volume; }
    channel.remaining = looped ? 0 : channel.minDelay; schemes_.push_back(std::move(channel)); return true;
}

bool SoundService::AddScheme(const char *name)
{
    auto ini = fio->OpenIniFile("resource\\ini\\sound_scheme.ini"); if (!ini || !name) return false;
    char value[COMMON_STRING_LENGTH]{};
    if (ini->ReadString(name, "ch", value, sizeof(value), "")) { AddSchemeChannel(value, false); while (ini->ReadStringNext(name, "ch", value, sizeof(value))) AddSchemeChannel(value, false); }
    if (ini->ReadString(name, "ch_loop", value, sizeof(value), "")) { AddSchemeChannel(value, true); while (ini->ReadStringNext(name, "ch_loop", value, sizeof(value))) AddSchemeChannel(value, true); }
    return true;
}

void SoundService::ProcessSchemes()
{
    const int elapsed = static_cast<int>(core.GetDeltaTime());
    for (auto &channel : schemes_) { if (channel.looped && (Find(channel.id) || SoundIsPlaying(channel.id))) continue; channel.remaining -= elapsed; if (channel.remaining > 0) continue; channel.id = SoundPlay(channel.name.c_str(), PCM_STEREO, VOLUME_FX, false, channel.looped, false, 0, nullptr, -1, -1, 0, channel.volume, 128); if (!channel.looped) { const int64_t span = std::max<int64_t>(0, static_cast<int64_t>(channel.maxDelay) - channel.minDelay); const int64_t offset = span ? static_cast<int64_t>((static_cast<double>(std::rand()) / RAND_MAX) * span) : 0; channel.remaining = static_cast<int32_t>(static_cast<int64_t>(channel.minDelay) + offset); } else channel.remaining = static_cast<int32_t>(channel.id) == SOUND_INVALID_ID ? 1000 : 0; }
}
