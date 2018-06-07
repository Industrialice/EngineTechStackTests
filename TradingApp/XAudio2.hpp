#pragma once

namespace TradingApp
{
    class XAudioEngine
    {
    public:
        class AudioSource
        {
            friend XAudioEngine;
            void *_source{};

        public:
            bool IsValid() const;
        };

        struct PositioningInfo
        {
            Vector3 position;
            Vector3 velocity;
            Vector3 orientFront{0, 0, 1};
            Vector3 orientTop{0, 1, 0};
        };

    protected:
        XAudioEngine() = default;
        static AudioSource AssignAudioSource(void *voice);
        template <typename T> static T *ExtractAudioSource(AudioSource audio)
        {
            return (T *)audio._source;
        }

    public:
        using AudioSourceDataType = MemoryStreamFromDataHolder<32>;

        virtual ~XAudioEngine() = default;
        virtual AudioSource AddAudio(AudioSourceDataType &&audioDataStream, bool isStartImmediately, const optional<PositioningInfo> &positioning) = 0;
        virtual void RemoveAudio(AudioSource audio) = 0;
        virtual void StartAudio(AudioSource audio) = 0;
        virtual void StopAudio(AudioSource audio) = 0;
        virtual void SetAudioPositioning(AudioSource audio, const optional<PositioningInfo> &positioning) = 0;
        virtual f32 AudioLength(AudioSource audio) const = 0;
        virtual f32 AudioPositionPercent(AudioSource audio) const = 0;
        virtual void SetListenerPositioning(const optional<PositioningInfo> &positioning) = 0;
        virtual void Update() = 0;

        static unique_ptr<XAudioEngine> New();
    };
}