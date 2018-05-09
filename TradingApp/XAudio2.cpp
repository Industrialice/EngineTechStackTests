#include "PreHeader.hpp"
#include <xaudio2.h>
#include "XAudio2.hpp"
#include <Application.hpp>
#include <Logger.hpp>

using namespace EngineCore;
using namespace TradingApp;

static const char *XAudioErrorToString(HRESULT error);

class XAudioEngineImpl final : public XAudioEngine
{
    IXAudio2 *_xAudio{};
    IXAudio2MasteringVoice *_masteringVoice{};
    vector<pair<IXAudio2SourceVoice *, unique_ptr<const ui8[]>>> _voices{};

public:
    ~XAudioEngineImpl() override
    {
        for (auto &voice : _voices)
        {
            voice.first->DestroyVoice();
        }
        if (_masteringVoice)
        {
            _masteringVoice->DestroyVoice();
        }
        if (_xAudio)
        {
            _xAudio->Release();
        }

        SENDLOG(Info, "Destroyed XAudioEngine\n");
    }

    XAudioEngineImpl()
    {
        HRESULT hr = XAudio2Create(&_xAudio, 0, XAUDIO2_DEFAULT_PROCESSOR);
        if (FAILED(hr))
        {
            assert(_xAudio == nullptr);
            SENDLOG(Error, "XAudio2Create failed, error %s\n", XAudioErrorToString(hr));
            return;
        }

        hr = _xAudio->CreateMasteringVoice(&_masteringVoice, XAUDIO2_DEFAULT_CHANNELS, 44100, 0, 0, nullptr);
        if (FAILED(hr))
        {
            assert(_masteringVoice == nullptr);
            SENDLOG(Error, "CreateMasteringVoice failed, error %s\n", XAudioErrorToString(hr));
            return;
        }

        _xAudio->StartEngine();
    }

    bool IsValid() const
    {
        return _masteringVoice != nullptr;
    }

    virtual AudioSource AddAudio(unique_ptr<const ui8[]> audioData, uiw dataStartOffset, uiw dataSize) override
    {
        IXAudio2SourceVoice *voice = nullptr;

        WAVEFORMATEX format{};
        format.nChannels = 2;
        format.nSamplesPerSec = 44100;
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nAvgBytesPerSec = format.nSamplesPerSec * sizeof(ui16) * format.nChannels;
        format.nBlockAlign = sizeof(ui16) * format.nChannels;
        format.wBitsPerSample = sizeof(ui16) * 8;

        HRESULT hr = _xAudio->CreateSourceVoice(&voice, &format, XAUDIO2_VOICE_NOSRC | XAUDIO2_VOICE_NOPITCH, 2.0f /* ??? */);
        if (FAILED(hr))
        {
            assert(voice == nullptr);
            SENDLOG(Error, "CreateSourceVoice failed, error %s\n", XAudioErrorToString(hr));
            return {};
        }

        XAUDIO2_BUFFER audioDataBuffer{};
        audioDataBuffer.AudioBytes = dataSize;
        audioDataBuffer.pAudioData = audioData.get() + dataStartOffset;
        hr = voice->SubmitSourceBuffer(&audioDataBuffer);
        if (FAILED(hr))
        {
            voice->DestroyVoice();
            SENDLOG(Error, "SubmitSourceBuffer failed, error %s\n", XAudioErrorToString(hr));
            return {};
        }

        _voices.emplace_back(pair(voice, move(audioData)));

        return AssignAudioSource(voice);
    }

    virtual void StartAudio(AudioSource audio) override
    {
        HRESULT hr = ((IXAudio2SourceVoice *)ExtractAudioSource(audio))->Start();
        if (FAILED(hr))
        {
            SENDLOG(Error, "Failed to play audio source\n");
        }
    }
    
    virtual void StopAudio(AudioSource audio) override
    {
        HRESULT hr = ((IXAudio2SourceVoice *)ExtractAudioSource(audio))->Stop();
        if (FAILED(hr))
        {
            SENDLOG(Error, "Failed to stop audio source\n");
        }
    }
};

auto XAudioEngine::AssignAudioSource(void *voice) -> AudioSource
{
    AudioSource source;
    source._source = voice;
    return source;
}

void *XAudioEngine::ExtractAudioSource(AudioSource audio)
{
    return audio._source;
}

unique_ptr<XAudioEngine> XAudioEngine::New()
{
    auto newEngine = make_unique<XAudioEngineImpl>();
    if (!newEngine->IsValid())
    {
        return {};
    }
    return newEngine;
}

const char *XAudioErrorToString(HRESULT error)
{
    switch (error)
    {
    case HRESULT(XAUDIO2_E_INVALID_CALL):			return "XAUDIO2_E_INVALID_CALL";
    case HRESULT(XAUDIO2_E_XMA_DECODER_ERROR):		return "XAUDIO2_E_XMA_DECODER_ERROR";
    case HRESULT(XAUDIO2_E_XAPO_CREATION_FAILED):	return "XAUDIO2_E_XAPO_CREATION_FAILED";
    case HRESULT(XAUDIO2_E_DEVICE_INVALIDATED):		return "XAUDIO2_E_DEVICE_INVALIDATED";
    case REGDB_E_CLASSNOTREG:						return "REGDB_E_CLASSNOTREG";
    case CLASS_E_NOAGGREGATION:						return "CLASS_E_NOAGGREGATION";
    case E_NOINTERFACE:								return "E_NOINTERFACE";
    case E_POINTER:									return "E_POINTER";
    case E_INVALIDARG:								return "E_INVALIDARG";
    case E_OUTOFMEMORY:								return "E_OUTOFMEMORY";
    default: return "UNKNOWN";
    }
}

bool XAudioEngine::AudioSource::IsValid() const
{
    return _source != nullptr;
}