#pragma once

#include "XAudio2.hpp"

namespace TradingApp
{
    class SoundCache
    {
    public:
        virtual optional<MemoryStreamFromDataHolder<XAudioEngine::AudioSourceDataType::localSize>> GetAudioStream(const FilePath &pnn) = 0;
        virtual void Update() = 0;

        static unique_ptr<SoundCache> New();
    };
}