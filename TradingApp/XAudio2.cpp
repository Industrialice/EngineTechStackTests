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
    vector<IXAudio2SourceVoice *> _voices{};

public:
    ~XAudioEngineImpl() override
    {
        for (auto &voice : _voices)
        {
            voice->DestroyVoice();
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

    virtual AudioSource AddAudio(const ui8 *audioData, uiw dataSize) override
    {
        IXAudio2SourceVoice *voice = nullptr;

        WAVEFORMATEX format{};
        format.nChannels = 0;
        format.nSamplesPerSec = 0;
        format.wFormatTag = WAVE_FORMAT_IEEE_FLOAT;
        format.nAvgBytesPerSec = format.nSamplesPerSec * sizeof(f32) * format.nChannels;
        format.nBlockAlign = sizeof(f32) * format.nChannels;
        format.wBitsPerSample = sizeof(f32) * 8;

        HRESULT hr = _xAudio->CreateSourceVoice(&voice, &format, XAUDIO2_VOICE_NOSRC | XAUDIO2_VOICE_NOPITCH, 2.0f /* ??? */);
        if (FAILED(hr))
        {
            assert(voice == nullptr);
            SENDLOG(Error, "CreateSourceVoice failed, error %s\n", XAudioErrorToString(hr));
            return {};
        }

        _voices.emplace_back(voice);

        return AssignAudioSource(voice);
    }
};

auto XAudioEngine::AssignAudioSource(void *voice) -> AudioSource
{
    AudioSource source;
    source._source = voice;
    return source;
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
