#include "PreHeader.hpp"
#include "SoundCache.hpp"
#include <map>
#include <Logger.hpp>
#include <Application.hpp>
#include "AudioWaveFormatParser.hpp"

using namespace TradingApp;

class SoundCacheImpl : public SoundCache
{
    std::map<FilePath, shared_ptr<MemoryMappedFile>> _storedMappings{};

    virtual optional<MemoryStreamFromDataHolder<XAudioEngine::AudioSourceDataType::localSize>> GetAudioStream(const FilePath &pnn) override
    {
        shared_ptr<MemoryMappedFile> mapping;

        auto it = _storedMappings.find(pnn);
        if (it != _storedMappings.end())
        {
            mapping = it->second;
        }
        else
        {
            auto f = File(pnn, FileOpenMode::OpenExisting, FileProcMode::Read);
            if (!f.IsOpened())
            {
                SENDLOG(Error, "Failed to open audio file " PTHSTR "\n", pnn.PlatformPath().data());
                return {};
            }

            auto mappedFile = MemoryMappedFile(f);
            if (!mappedFile.IsOpened())
            {
                SENDLOG(Error, "Failed to map audio file " PTHSTR "\n", pnn.PlatformPath().data());
                return {};
            }

            auto parsedFile = ParseWaveFormatHeader(mappedFile.CMemory(), (ui32)mappedFile.Size());
            if (!parsedFile)
            {
                SENDLOG(Error, "ParseWaveFormatHeader for audio file " PTHSTR " failed\n", pnn.PlatformPath().data());
                return {};
            }

            mappedFile = MemoryMappedFile(f, parsedFile->dataStartOffset, parsedFile->riffWaveHeader.chunkDataSize);
            ASSUME(mappedFile.IsOpened() && mappedFile.Size() == parsedFile->riffWaveHeader.chunkDataSize);

            mapping = make_shared<MemoryMappedFile>(move(mappedFile));

            _storedMappings[pnn] = mapping;
        }

        auto size = mapping->Size();

        using HolderType = DataHolder<XAudioEngine::AudioSourceDataType::localSize>;
        auto data = HolderType(move(mapping));
        auto provideData = [](const HolderType &data)
        {
            return data.Get<shared_ptr<MemoryMappedFile>>()->CMemory();
        };

        return XAudioEngine::AudioSourceDataType::New<decltype(mapping)>(move(data), size, provideData);
    }
    
    virtual void Update() override
    {
    }
};

unique_ptr<SoundCache> SoundCache::New()
{
    return make_unique<SoundCacheImpl>();
}
