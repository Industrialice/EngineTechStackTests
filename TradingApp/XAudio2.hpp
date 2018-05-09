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

    protected:
        XAudioEngine() = default;
        static AudioSource AssignAudioSource(void *voice);
        static void *ExtractAudioSource(AudioSource audio);

    public:
        virtual ~XAudioEngine() = default;
        virtual AudioSource AddAudio(unique_ptr<const ui8[]> audioData, uiw dataStartOffset, uiw dataSize) = 0;
        virtual void StartAudio(AudioSource audio) = 0;
        virtual void StopAudio(AudioSource audio) = 0;

        static unique_ptr<XAudioEngine> New();
    };
}