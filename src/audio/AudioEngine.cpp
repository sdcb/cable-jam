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
constexpr std::array<float, 7> NoteFrequencies{
    523.25f, // Do C5
    587.33f, // Re D5
    659.25f, // Mi E5
    698.46f, // Fa F5
    783.99f, // Sol G5
    880.00f, // La A5
    987.77f  // Si B5
};

enum class WaveShape {
    Sine,
    Square,
    Triangle
};

struct Tone {
    float frequency;
    WaveShape shape;
    float seconds;
};

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

float WaveSample(WaveShape shape, float phase) {
    switch (shape) {
    case WaveShape::Sine:
        return std::sin(phase);
    case WaveShape::Square:
        return std::sin(phase) >= 0.0f ? 1.0f : -1.0f;
    case WaveShape::Triangle:
        return (2.0f / Pi) * std::asin(std::sin(phase));
    }
    return 0.0f;
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
        const float sample = WaveSample(tone.shape, phase) * Envelope(i, sampleCount) * gain;
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

WaveShape ShapeForPlugStyle(int plugStyle) {
    switch (plugStyle % 3) {
    case 0:
        return WaveShape::Sine;
    case 1:
        return WaveShape::Square;
    default:
        return WaveShape::Triangle;
    }
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

    sounds_[SoundId::ButtonClick] = MakeTone({880.0f, WaveShape::Sine, 0.045f}, 0.38f, 0.42f);
    sounds_[SoundId::Confirm] = MakeTone({660.0f, WaveShape::Triangle, 0.075f}, 0.44f, 0.50f);
    sounds_[SoundId::Cancel] = MakeTone({220.0f, WaveShape::Triangle, 0.095f}, 0.40f, 0.46f);
    sounds_[SoundId::CableBlocked] = MakeTone({150.0f, WaveShape::Square, 0.115f}, 0.26f, 0.42f);

    constexpr std::array<Tone, 4> complete{
        Tone{523.25f, WaveShape::Sine, 0.075f},
        Tone{659.25f, WaveShape::Sine, 0.075f},
        Tone{783.99f, WaveShape::Sine, 0.075f},
        Tone{1046.50f, WaveShape::Sine, 0.130f}
    };
    sounds_[SoundId::LevelComplete] = MakeSequence(complete, 0.38f, 0.64f);

    for (std::size_t color = 0; color < NoteFrequencies.size(); ++color) {
        for (int style = 0; style < 3; ++style) {
            cablePullSounds_[color][static_cast<std::size_t>(style)] =
                MakeTone({NoteFrequencies[color], ShapeForPlugStyle(style), 0.105f}, style == 1 ? 0.26f : 0.34f, 0.62f);
        }
    }

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

void AudioEngine::PlayCablePull(int colorIndex, int plugStyle) {
    if (!soundsCreated_) {
        return;
    }
    const auto color = static_cast<std::size_t>((colorIndex % 7 + 7) % 7);
    const auto style = static_cast<std::size_t>((plugStyle % 3 + 3) % 3);
    PlayBuffer(cablePullSounds_[color][style]);
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
