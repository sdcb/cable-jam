#pragma once

#include <xaudio2.h>

#include <map>
#include <vector>

namespace cable::audio {

enum class SoundId {
    ButtonClick,
    Confirm,
    Cancel,
    CablePull,
    CableBlocked,
    LevelComplete
};

class AudioEngine {
public:
    ~AudioEngine();
    bool Initialize();
    void SetMasterVolume(float volume);
    void Play(SoundId id);

    struct SoundBuffer {
        std::vector<unsigned char> format;
        std::vector<unsigned char> audio;
        float volume{1.0f};
    };

private:
    void CleanupVoices();

    bool initialized_{false};
    float masterVolume_{0.8f};
    ::IXAudio2* engine_{};
    ::IXAudio2MasteringVoice* masterVoice_{};
    std::map<SoundId, SoundBuffer> sounds_;
    std::vector<::IXAudio2SourceVoice*> activeVoices_;
};

} // namespace cable::audio
