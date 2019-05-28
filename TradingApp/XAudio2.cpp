#include "PreHeader.hpp"
#include <XAudio2.h>
#include <x3daudio.h>
#include "XAudio2.hpp"
#include <Application.hpp>
#include <Logger.hpp>
#include <XAudio2fx.h>
#include <xapofx.h>

#pragma comment(lib, "X3DAudio.lib")
#pragma comment(lib, "XAPOFX.lib")

using namespace EngineCore;
using namespace TradingApp;

static const char *XAudioErrorToString(HRESULT error);

class XAudioEngineImpl final : public XAudioEngine
{
    struct StoredAudio
    {
        IXAudio2SourceVoice *voice;
        IXAudio2SubmixVoice *submixVoice;
        AudioSourceDataType data;
        optional<PositioningInfo> positioning;
        WAVEFORMATEX format;
    };

    IXAudio2 *_xAudio{};
    IXAudio2MasteringVoice *_masteringVoice{};
    vector<StoredAudio> _voices{};
    X3DAUDIO_HANDLE _x3DInstance{};
    optional<PositioningInfo> _listenerPositioning{};
    UINT32 _channelsCount{};
    vector<IXAudio2SourceVoice *> _sourceVoicesPool{};
    vector<IXAudio2SubmixVoice *> _submixVoicesPool{};
    HMODULE _xAudioDll{};

public:
    ~XAudioEngineImpl() override
    {
        for (auto &voice : _voices)
        {
            if (voice.submixVoice)
            {
                voice.submixVoice->DestroyVoice();
            }
            voice.voice->DestroyVoice();
        }
        for (auto &voice : _submixVoicesPool)
        {
            voice->DestroyVoice();
        }
        for (auto &voice : _sourceVoicesPool)
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

        FreeLibrary(_xAudioDll);

        SENDLOG(Info, "Destroyed XAudioEngine\n");
    }

    XAudioEngineImpl()
    {
        CoInitialize(0);

        // workaround for a bug in XAudio2_7.dll, the library doesn't properly implement ref counting
        // and might be unloaded prematurely leading to a crash
        _xAudioDll = LoadLibraryA("XAudio2_7.dll");
        ASSUME(_xAudioDll);

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

        XAUDIO2_DEVICE_DETAILS details{};
        hr = _xAudio->GetDeviceDetails(0, &details);
        if (FAILED(hr))
        {
            _masteringVoice->DestroyVoice();
            _masteringVoice = nullptr;
            SENDLOG(Error, "IXAudio2::GetDeviceDetails failed, error %s\n", XAudioErrorToString(hr));
            return;
        }

        _channelsCount = details.OutputFormat.Format.nChannels;

        hr = X3DAudioInitialize(_channelsCount, X3DAUDIO_SPEED_OF_SOUND, _x3DInstance);
        if (FAILED(hr))
        {
            _masteringVoice->DestroyVoice();
            _masteringVoice = nullptr;
            SENDLOG(Error, "X3DAudioInitialize failed, error %s\n", XAudioErrorToString(hr));
            return;
        }
    }

    bool IsValid() const
    {
        return _masteringVoice != nullptr;
    }

    virtual AudioSource AddAudio(AudioSourceDataType &&audioDataStream, bool isStartImmediately, const optional<PositioningInfo> &positioning) override
    {
        IXAudio2SourceVoice *voice = nullptr;

        WAVEFORMATEX format{};
        format.nChannels = 2;
        format.nSamplesPerSec = 44100;
        format.wFormatTag = WAVE_FORMAT_PCM;
        format.nAvgBytesPerSec = format.nSamplesPerSec * sizeof(ui16) * format.nChannels;
        format.nBlockAlign = sizeof(ui16) * format.nChannels;
        format.wBitsPerSample = sizeof(ui16) * 8;

        if (_sourceVoicesPool.size())
        {
            voice = _sourceVoicesPool.back();
            _sourceVoicesPool.pop_back();
        }
        else
        {
            HRESULT hr = _xAudio->CreateSourceVoice(&voice, &format, XAUDIO2_VOICE_NOSRC | XAUDIO2_VOICE_NOPITCH | XAUDIO2_VOICE_USEFILTER, 2.0f /* ??? */);
            if (FAILED(hr))
            {
                assert(voice == nullptr);
                SENDLOG(Error, "CreateSourceVoice failed, error %s\n", XAudioErrorToString(hr));
                return {};
            }
        }

        XAUDIO2_BUFFER audioDataBuffer{};
        audioDataBuffer.AudioBytes = (UINT32)audioDataStream.Size();
        audioDataBuffer.pAudioData = audioDataStream.CMemory();
        audioDataBuffer.Flags = XAUDIO2_END_OF_STREAM;
        HRESULT hr = voice->SubmitSourceBuffer(&audioDataBuffer);
        if (FAILED(hr))
        {
            voice->DestroyVoice();
            SENDLOG(Error, "SubmitSourceBuffer failed, error %s\n", XAudioErrorToString(hr));
            return {};
        }

        IXAudio2SubmixVoice *submixVoice = nullptr;
        if (positioning)
        {
            submixVoice = CreateSubmixVoice(voice);
        }

        _voices.emplace_back(StoredAudio{voice, submixVoice, move(audioDataStream), positioning, format});

        if (isStartImmediately)
        {
            hr = voice->Start();
            if (FAILED(hr))
            {
                SENDLOG(Error, "Failed to play audio source\n");
            }
        }

        return AssignAudioSource(voice);
    }

    virtual void RemoveAudio(AudioSource audio) override
    {
        IXAudio2SourceVoice *voice = ExtractAudioSource<IXAudio2SourceVoice>(audio);
        for (uiw index = 0, size = _voices.size(); index < size; ++index)
        {
            if (voice == _voices[index].voice)
            {
                auto &stored = _voices[index];

                if (_voices[index].submixVoice)
                {
                    stored.submixVoice->SetEffectChain(nullptr);
                    _submixVoicesPool.push_back(stored.submixVoice);
                    
                }
                stored.voice->ExitLoop();
                stored.voice->Stop();
                stored.voice->SetEffectChain(nullptr);
                stored.voice->FlushSourceBuffers();
                XAUDIO2_VOICE_STATE state;
                stored.voice->GetState(&state);
                if (state.BuffersQueued == 0)
                {
                    _sourceVoicesPool.push_back(stored.voice);
                }
                else
                {
                    stored.voice->DestroyVoice();
                }
                _voices.erase(_voices.begin() + index);
                return;
            }
        }
        HARDBREAK;
    }

    virtual void StartAudio(AudioSource audio) override
    {
        HRESULT hr = ExtractAudioSource<IXAudio2SourceVoice>(audio)->Start();
        if (FAILED(hr))
        {
            SENDLOG(Error, "Failed to play audio source\n");
        }
    }
    
    virtual void StopAudio(AudioSource audio) override
    {
        HRESULT hr = ExtractAudioSource<IXAudio2SourceVoice>(audio)->Stop();
        if (FAILED(hr))
        {
            SENDLOG(Error, "Failed to stop audio source\n");
        }
    }

    virtual void SetAudioPositioning(AudioSource audio, const optional<PositioningInfo> &positioning) override
    {
        auto handle = ExtractAudioSource<IXAudio2SourceVoice>(audio);
        for (auto &storedAudio : _voices)
        {
            if (storedAudio.voice == handle)
            {
                storedAudio.positioning = positioning;
                if (positioning)
                {
                    if (!storedAudio.submixVoice)
                    {
                        storedAudio.submixVoice = CreateSubmixVoice(storedAudio.voice);
                    }
                }
                else
                {
                    if (storedAudio.submixVoice)
                    {
                        storedAudio.voice->SetEffectChain(nullptr);
                        storedAudio.submixVoice->DestroyVoice();
                        storedAudio.submixVoice = nullptr;
                    }
                }
                return;
            }
        }
        HARDBREAK;
    }

    virtual f32 AudioLength(AudioSource audio) const override
    {
        auto handle = ExtractAudioSource<IXAudio2SourceVoice>(audio);
        for (auto &storedAudio : _voices)
        {
            if (storedAudio.voice == handle)
            {
                return storedAudio.data.Size() / (f32)storedAudio.format.nAvgBytesPerSec;
            }
        }
        return 0;
        HARDBREAK;
    }

    virtual f32 AudioPositionPercent(AudioSource audio) const override
    {
        auto handle = ExtractAudioSource<IXAudio2SourceVoice>(audio);
        for (auto &storedAudio : _voices)
        {
            if (storedAudio.voice == handle)
            {
                XAUDIO2_VOICE_STATE state;
                handle->GetState(&state);
                uiw numTotalFrames = storedAudio.data.Size() / (storedAudio.format.nChannels * sizeof(ui16));
                uiw playedFrames = (uiw)state.SamplesPlayed;
                return (f32)numTotalFrames / (f32)playedFrames * 100.0f;
            }
        }
        return 0;
        HARDBREAK;
    }

    virtual void SetListenerPositioning(const optional<PositioningInfo> &positioning) override
    {
        _listenerPositioning = positioning;
    }

    virtual void Update() override
    {
        if (!_listenerPositioning)
        {
            return;
        }

        const auto &lp = _listenerPositioning;

        X3DAUDIO_LISTENER listener{};
        listener.Position = {lp->position.x, lp->position.y, lp->position.z};
        listener.Velocity = {lp->velocity.x, lp->velocity.y, lp->velocity.z};
        listener.OrientFront = {lp->orientFront.x, lp->orientFront.y, lp->orientFront.z};
        listener.OrientTop = {lp->orientTop.x, lp->orientTop.y, lp->orientTop.z};

        FLOAT32 matrix[8]{};

        FLOAT32 delays[2]{};

        X3DAUDIO_DSP_SETTINGS dspSettings{};
        dspSettings.SrcChannelCount = 2;
        dspSettings.DstChannelCount = _channelsCount;
        dspSettings.pMatrixCoefficients = matrix;
        dspSettings.pDelayTimes = delays;

        for (const auto &audio : _voices)
        {
            if (!audio.positioning)
            {
                continue;
            }

            const auto &ap = audio.positioning;

            /*X3DAUDIO_CONE cone{};
            cone.InnerAngle = 0.0f;
            cone.OuterAngle = 0.0f;
            cone.InnerVolume = 0.0f;
            cone.OuterVolume = 1.0f;
            cone.InnerLPF = 0.0f;
            cone.OuterLPF = 1.0f;
            cone.InnerReverb = 0.0f;
            cone.OuterReverb = 1.0f;*/

            X3DAUDIO_DISTANCE_CURVE volumeCurve{};
            X3DAUDIO_DISTANCE_CURVE_POINT volumeCurvePoints[2];
            volumeCurvePoints[0].Distance = 0.0f;
            volumeCurvePoints[0].DSPSetting = 1.0f;
            volumeCurvePoints[1].Distance = 1.0f;
            volumeCurvePoints[1].DSPSetting = 1.0f;
            volumeCurve.PointCount = (UINT32)CountOf(volumeCurvePoints);
            volumeCurve.pPoints = volumeCurvePoints;

            X3DAUDIO_DISTANCE_CURVE reverbCurve{};
            X3DAUDIO_DISTANCE_CURVE_POINT reverbCurvePoints[2];
            reverbCurvePoints[0].Distance = 0.0f;
            reverbCurvePoints[0].DSPSetting = 0.5f;
            reverbCurvePoints[1].Distance = 1.0f;
            reverbCurvePoints[1].DSPSetting = 0.5f;
            reverbCurve.PointCount = (UINT32)CountOf(reverbCurvePoints);
            reverbCurve.pPoints = reverbCurvePoints;

            FLOAT32 channelAzimuths[8]{};

            X3DAUDIO_EMITTER emitter{};
            emitter.ChannelCount = 2;
            emitter.CurveDistanceScaler = FLT_MIN;
            emitter.Position = {ap->position.x, ap->position.y, ap->position.z};
            emitter.Velocity = {ap->velocity.x, ap->velocity.y, ap->velocity.z};
            emitter.OrientFront = {ap->orientFront.x, ap->orientFront.y, ap->orientFront.z};
            emitter.OrientTop = {ap->orientTop.x, ap->orientTop.y, ap->orientTop.z};
            emitter.pCone = nullptr;
            //emitter.pVolumeCurve = &volumeCurve;
            //emitter.pReverbCurve = &reverbCurve;
            emitter.CurveDistanceScaler = 1.0f;
            emitter.DopplerScaler = 1.0f;
            emitter.pChannelAzimuths = channelAzimuths;

            X3DAudioCalculate(_x3DInstance, &listener, &emitter, X3DAUDIO_CALCULATE_MATRIX | X3DAUDIO_CALCULATE_DOPPLER | X3DAUDIO_CALCULATE_LPF_DIRECT | X3DAUDIO_CALCULATE_REVERB, &dspSettings);

            HRESULT hr = audio.voice->SetOutputMatrix(_masteringVoice, 2, _channelsCount, dspSettings.pMatrixCoefficients);
            if (FAILED(hr))
            {
                SENDLOG(Error, "SetOutputMatrix failed, error %s\n", XAudioErrorToString(hr));
            }
            hr = audio.voice->SetFrequencyRatio(dspSettings.DopplerFactor);
            if (FAILED(hr))
            {
                SENDLOG(Error, "SetFrequencyRatio failed, error %s\n", XAudioErrorToString(hr));
            }
            /*FLOAT32 reverbMatrix[8];
            std::fill(std::begin(reverbMatrix), std::end(reverbMatrix), dspSettings.ReverbLevel);
            hr = audio.voice->SetOutputMatrix(audio.submixVoice, 2, _channelsCount, reverbMatrix);
            if (FAILED(hr))
            {
                SENDLOG(Error, "SetOutputMatrix failed, error %s\n", XAudioErrorToString(hr));
            }*/
            XAUDIO2_FILTER_PARAMETERS filterParameters = {LowPassFilter, 2.0f * sinf(X3DAUDIO_PI / 6.0f * dspSettings.LPFDirectCoefficient), 1.0f};
            hr = audio.voice->SetFilterParameters(&filterParameters);
            if (FAILED(hr))
            {
                SENDLOG(Error, "SetFilterParameters failed, error %s\n", XAudioErrorToString(hr));
            }
        }
    }

    IXAudio2SubmixVoice *CreateSubmixVoice(IXAudio2SourceVoice *voice)
    {
        ASSUME(voice);

        return nullptr; // TODO: fix submix voices so we can use reverb effect (do we need them though?)

        /*IUnknown *effect;
        HRESULT hr = CreateFX(__uuidof(FXEQ), &effect);
        if (FAILED(hr))
        {
            SENDLOG(Error, "CreateFX failed, error %s\n", XAudioErrorToString(hr));
            return nullptr;
        }

        XAUDIO2_EFFECT_DESCRIPTOR effectsDescr[] =
        {
            {effect, true, _channelsCount}
        };

        XAUDIO2_EFFECT_CHAIN effectChain =
        {
            1, effectsDescr
        };

        IXAudio2SubmixVoice *submix;
        hr = _xAudio->CreateSubmixVoice(&submix, 2, 44100, 0, 0, nullptr, &effectChain);
        if (FAILED(hr))
        {
            effect->Release();
            SENDLOG(Error, "CreateSubmixVoice failed, error %s\n", XAudioErrorToString(hr));
            return nullptr;
        }

        hr = voice->SetEffectChain(&effectChain);

        effect->Release();

        if (FAILED(hr))
        {
            submix->DestroyVoice();
            SENDLOG(Error, "SetEffectChain failed, error %s\n", XAudioErrorToString(hr));
            return nullptr;
        }

        return submix;*/
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