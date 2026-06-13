#pragma once

#include <xaudio2.h>

#include <cstddef>
#include <map>
#include <vector>

namespace cable::audio {

enum class SoundId {
    ButtonClick,
    Confirm,
    Cancel,
    CableBlocked,
    LevelComplete
};

class AudioEngine {
public:
    ~AudioEngine();
    bool Initialize();
    bool EnsureProceduralSoundsCreated();
    void SetMasterVolume(float volume);
    void Play(SoundId id);
    void PlayCablePull();

    struct SoundBuffer {
        WAVEFORMATEX format{};
        std::vector<unsigned char> audio;
        float volume{1.0f};
    };

private:
    void PlayBuffer(const SoundBuffer& sound);
    void CleanupVoices();

    bool initialized_{false};
    bool soundsCreated_{false};
    std::size_t melodyIndex_{0};
    float masterVolume_{0.8f};
    ::IXAudio2* engine_{};
    ::IXAudio2MasteringVoice* masterVoice_{};
    std::map<SoundId, SoundBuffer> sounds_;
    std::vector<SoundBuffer> cablePullMelody_;
    std::vector<::IXAudio2SourceVoice*> activeVoices_;
};

} // namespace cable::audio
