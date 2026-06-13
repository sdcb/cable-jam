#include "audio/AudioEngine.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <span>

namespace cable::audio {
namespace {

constexpr int SampleRate = 44100;
constexpr int ChannelCount = 1;
constexpr int BitsPerSample = 16;
constexpr float Pi = 3.1415926535f;

struct Tone {
    float frequency;
    float seconds;
};

constexpr float G4 = 392.00f;
constexpr float A4 = 440.00f;
constexpr float B4 = 493.88f;
constexpr float C5 = 523.25f;
constexpr float D5 = 587.33f;
constexpr float Ds5 = 622.25f;
constexpr float E5 = 659.25f;
constexpr float F5 = 698.46f;
constexpr float Fs5 = 739.99f;
constexpr float G5 = 783.99f;
constexpr float A5 = 880.00f;
constexpr float C6 = 1046.50f;

constexpr auto AmericanPatrolMelody = std::to_array<float>({
    G4,
    C5, C5, C5, B4, C5, D5,
    E5, E5, E5, Ds5, E5, F5,
    G5, G5, G5, Fs5, G5, C6,
    G5, G5, G5, E5,
    F5, F5, E5, D5, F5,
    E5, E5, D5, C5, E5,
    D5, A4, B4, C5,
    D5, G4, A4, B4,
    C5, C5, C5, B4, C5, D5,
    E5, E5, E5, Ds5, E5, F5,
    G5, G5, G5, Fs5, G5, C6,
    G5, G5, G5, C5,
    A5, G5, F5, E5,
    D5, C5, B4, C5,
    D5, E5, F5, E5, D5,
    C5, G4, A4, B4, C5, D5, E5
});

WAVEFORMATEX PcmFormat() {
    WAVEFORMATEX format{};
    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = ChannelCount;
    format.nSamplesPerSec = SampleRate;
    format.wBitsPerSample = BitsPerSample;
    format.nBlockAlign = static_cast<WORD>(format.nChannels * format.wBitsPerSample / 8);
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    return format;
}

float Envelope(int sample, int sampleCount) {
    const int edge = std::max(1, static_cast<int>(SampleRate * 0.006f));
    const float attack = std::min(1.0f, static_cast<float>(sample) / static_cast<float>(edge));
    const float release = std::min(1.0f, static_cast<float>(sampleCount - sample - 1) / static_cast<float>(edge));
    return std::clamp(std::min(attack, release), 0.0f, 1.0f);
}

void AppendTone(std::vector<unsigned char>& audio, Tone tone, float gain) {
    const int sampleCount = std::max(1, static_cast<int>(tone.seconds * SampleRate));
    for (int i = 0; i < sampleCount; ++i) {
        const float time = static_cast<float>(i) / static_cast<float>(SampleRate);
        const float phase = 2.0f * Pi * tone.frequency * time;
        const float sample = std::sin(phase) * Envelope(i, sampleCount) * gain;
        const auto value = static_cast<std::int16_t>(std::clamp(sample, -1.0f, 1.0f) * 32767.0f);
        audio.push_back(static_cast<unsigned char>(value & 0xff));
        audio.push_back(static_cast<unsigned char>((value >> 8) & 0xff));
    }
}

AudioEngine::SoundBuffer MakeTone(Tone tone, float gain, float volume) {
    AudioEngine::SoundBuffer buffer;
    buffer.format = PcmFormat();
    buffer.volume = volume;
    AppendTone(buffer.audio, tone, gain);
    return buffer;
}

AudioEngine::SoundBuffer MakeSequence(std::span<const Tone> tones, float gain, float volume) {
    AudioEngine::SoundBuffer buffer;
    buffer.format = PcmFormat();
    buffer.volume = volume;
    for (Tone tone : tones) {
        AppendTone(buffer.audio, tone, gain);
    }
    return buffer;
}

} // namespace

AudioEngine::~AudioEngine() {
    for (IXAudio2SourceVoice* voice : activeVoices_) {
        voice->DestroyVoice();
    }
    activeVoices_.clear();
    if (masterVoice_) {
        masterVoice_->DestroyVoice();
        masterVoice_ = nullptr;
    }
    if (engine_) {
        engine_->Release();
        engine_ = nullptr;
    }
}

bool AudioEngine::Initialize() {
    if (initialized_) {
        return true;
    }
    if (FAILED(XAudio2Create(&engine_, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        return false;
    }
    if (FAILED(engine_->CreateMasteringVoice(&masterVoice_))) {
        engine_->Release();
        engine_ = nullptr;
        return false;
    }
    masterVoice_->SetVolume(masterVolume_);
    initialized_ = true;
    return true;
}

bool AudioEngine::EnsureProceduralSoundsCreated() {
    if (soundsCreated_) {
        return true;
    }
    if (!initialized_ && !Initialize()) {
        return false;
    }

    sounds_[SoundId::ButtonClick] = MakeTone({880.0f, 0.045f}, 0.30f, 0.42f);
    sounds_[SoundId::Confirm] = MakeTone({660.0f, 0.075f}, 0.34f, 0.50f);
    sounds_[SoundId::Cancel] = MakeTone({220.0f, 0.095f}, 0.32f, 0.46f);
    sounds_[SoundId::CableBlocked] = MakeTone({150.0f, 0.115f}, 0.26f, 0.42f);

    constexpr std::array<Tone, 4> complete{
        Tone{C5, 0.075f},
        Tone{E5, 0.075f},
        Tone{G5, 0.075f},
        Tone{C6, 0.130f}
    };
    sounds_[SoundId::LevelComplete] = MakeSequence(complete, 0.38f, 0.64f);

    cablePullMelody_.clear();
    cablePullMelody_.reserve(AmericanPatrolMelody.size());
    for (float frequency : AmericanPatrolMelody) {
        cablePullMelody_.push_back(MakeTone({frequency, 0.115f}, 0.34f, 0.60f));
    }
    melodyIndex_ = 0;

    soundsCreated_ = true;
    return true;
}

void AudioEngine::SetMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    if (masterVoice_) {
        masterVoice_->SetVolume(masterVolume_);
    }
}

void AudioEngine::Play(SoundId id) {
    if (!soundsCreated_) {
        return;
    }
    auto it = sounds_.find(id);
    if (it != sounds_.end()) {
        PlayBuffer(it->second);
    }
}

void AudioEngine::PlayCablePull() {
    if (!soundsCreated_ || cablePullMelody_.empty()) {
        return;
    }
    PlayBuffer(cablePullMelody_[melodyIndex_]);
    melodyIndex_ = (melodyIndex_ + 1) % cablePullMelody_.size();
}

void AudioEngine::PlayBuffer(const SoundBuffer& sound) {
    if (!engine_ || sound.audio.empty()) {
        return;
    }
    CleanupVoices();

    IXAudio2SourceVoice* voice = nullptr;
    if (FAILED(engine_->CreateSourceVoice(&voice, &sound.format))) {
        return;
    }
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(sound.audio.size());
    buffer.pAudioData = sound.audio.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (FAILED(voice->SubmitSourceBuffer(&buffer))) {
        voice->DestroyVoice();
        return;
    }
    voice->SetVolume(sound.volume);
    if (FAILED(voice->Start())) {
        voice->DestroyVoice();
        return;
    }
    activeVoices_.push_back(voice);
}

void AudioEngine::CleanupVoices() {
    activeVoices_.erase(
        std::remove_if(activeVoices_.begin(), activeVoices_.end(), [](IXAudio2SourceVoice* voice) {
            XAUDIO2_VOICE_STATE state{};
            voice->GetState(&state);
            if (state.BuffersQueued == 0) {
                voice->DestroyVoice();
                return true;
            }
            return false;
        }),
        activeVoices_.end());
}

} // namespace cable::audio
