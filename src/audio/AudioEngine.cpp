#include "audio/AudioEngine.h"

#include "resources/ResourceIds.h"

#include <algorithm>
#include <array>

#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <shlwapi.h>
#include <windows.h>
#include <xaudio2.h>

namespace cable::audio {
namespace {

struct CatalogEntry {
    SoundId id;
    int resourceId;
    float volume;
};

constexpr std::array<CatalogEntry, 6> Catalog{{
    {SoundId::ButtonClick, IDR_MP3_UI_BUTTON_CLICK, 0.45f},
    {SoundId::Confirm, IDR_MP3_UI_CONFIRM, 0.55f},
    {SoundId::Cancel, IDR_MP3_UI_CANCEL, 0.50f},
    {SoundId::CablePull, IDR_MP3_CABLE_PULL, 0.62f},
    {SoundId::CableBlocked, IDR_MP3_CABLE_BLOCKED, 0.58f},
    {SoundId::LevelComplete, IDR_MP3_LEVEL_COMPLETE, 0.72f}
}};

bool ResourceBytes(int resourceId, const unsigned char*& data, DWORD& size) {
    HMODULE module = GetModuleHandleW(nullptr);
    HRSRC resource = FindResourceW(module, MAKEINTRESOURCEW(resourceId), RT_RCDATA);
    if (!resource) {
        return false;
    }
    HGLOBAL loaded = LoadResource(module, resource);
    if (!loaded) {
        return false;
    }
    data = static_cast<const unsigned char*>(LockResource(loaded));
    size = SizeofResource(module, resource);
    return data && size > 0;
}

bool DecodeMp3Resource(int resourceId, AudioEngine::SoundBuffer& output) {
    const unsigned char* data = nullptr;
    DWORD size = 0;
    if (!ResourceBytes(resourceId, data, size)) {
        return false;
    }

    IStream* stream = SHCreateMemStream(data, size);
    if (!stream) {
        return false;
    }

    IMFByteStream* byteStream = nullptr;
    HRESULT hr = MFCreateMFByteStreamOnStreamEx(stream, &byteStream);
    stream->Release();
    if (FAILED(hr)) {
        return false;
    }

    IMFSourceReader* reader = nullptr;
    hr = MFCreateSourceReaderFromByteStream(byteStream, nullptr, &reader);
    byteStream->Release();
    if (FAILED(hr)) {
        return false;
    }

    IMFMediaType* requestedType = nullptr;
    hr = MFCreateMediaType(&requestedType);
    if (SUCCEEDED(hr)) {
        requestedType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        requestedType->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        hr = reader->SetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, nullptr, requestedType);
        requestedType->Release();
    }
    if (FAILED(hr)) {
        reader->Release();
        return false;
    }

    IMFMediaType* actualType = nullptr;
    hr = reader->GetCurrentMediaType(MF_SOURCE_READER_FIRST_AUDIO_STREAM, &actualType);
    if (FAILED(hr)) {
        reader->Release();
        return false;
    }

    WAVEFORMATEX* waveFormat = nullptr;
    UINT32 waveFormatSize = 0;
    hr = MFCreateWaveFormatExFromMFMediaType(actualType, &waveFormat, &waveFormatSize);
    actualType->Release();
    if (FAILED(hr)) {
        reader->Release();
        return false;
    }
    output.format.assign(reinterpret_cast<unsigned char*>(waveFormat), reinterpret_cast<unsigned char*>(waveFormat) + waveFormatSize);
    CoTaskMemFree(waveFormat);

    while (true) {
        DWORD flags = 0;
        IMFSample* sample = nullptr;
        hr = reader->ReadSample(MF_SOURCE_READER_FIRST_AUDIO_STREAM, 0, nullptr, &flags, nullptr, &sample);
        if (FAILED(hr)) {
            break;
        }
        if (flags & MF_SOURCE_READERF_ENDOFSTREAM) {
            break;
        }
        if (!sample) {
            continue;
        }
        IMFMediaBuffer* buffer = nullptr;
        if (SUCCEEDED(sample->ConvertToContiguousBuffer(&buffer))) {
            BYTE* bytes = nullptr;
            DWORD maxLength = 0;
            DWORD currentLength = 0;
            if (SUCCEEDED(buffer->Lock(&bytes, &maxLength, &currentLength))) {
                output.audio.insert(output.audio.end(), bytes, bytes + currentLength);
                buffer->Unlock();
            }
            buffer->Release();
        }
        sample->Release();
    }

    reader->Release();
    return !output.format.empty() && !output.audio.empty();
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
    if (initialized_) {
        MFShutdown();
    }
}

bool AudioEngine::Initialize() {
    if (initialized_) {
        return true;
    }
    initialized_ = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
    if (!initialized_) {
        return false;
    }
    if (FAILED(XAudio2Create(&engine_, 0, XAUDIO2_DEFAULT_PROCESSOR))) {
        return false;
    }
    if (FAILED(engine_->CreateMasteringVoice(&masterVoice_))) {
        return false;
    }
    masterVoice_->SetVolume(masterVolume_);
    for (const CatalogEntry& entry : Catalog) {
        SoundBuffer buffer;
        buffer.volume = entry.volume;
        if (DecodeMp3Resource(entry.resourceId, buffer)) {
            sounds_[entry.id] = std::move(buffer);
        }
    }
    return true;
}

void AudioEngine::SetMasterVolume(float volume) {
    masterVolume_ = std::clamp(volume, 0.0f, 1.0f);
    if (masterVoice_) {
        masterVoice_->SetVolume(masterVolume_);
    }
}

void AudioEngine::Play(SoundId id) {
    if (!engine_) {
        return;
    }
    CleanupVoices();
    auto it = sounds_.find(id);
    if (it == sounds_.end() || it->second.format.empty() || it->second.audio.empty()) {
        return;
    }

    IXAudio2SourceVoice* voice = nullptr;
    const WAVEFORMATEX* format = reinterpret_cast<const WAVEFORMATEX*>(it->second.format.data());
    if (FAILED(engine_->CreateSourceVoice(&voice, format))) {
        return;
    }
    XAUDIO2_BUFFER buffer{};
    buffer.AudioBytes = static_cast<UINT32>(it->second.audio.size());
    buffer.pAudioData = it->second.audio.data();
    buffer.Flags = XAUDIO2_END_OF_STREAM;
    if (FAILED(voice->SubmitSourceBuffer(&buffer))) {
        voice->DestroyVoice();
        return;
    }
    voice->SetVolume(it->second.volume);
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
