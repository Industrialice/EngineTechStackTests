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

    public:
        virtual ~XAudioEngine() = default;
        virtual AudioSource AddAudio(const ui8 *audioData, uiw dataSize) = 0;

        static unique_ptr<XAudioEngine> New();
    };
}