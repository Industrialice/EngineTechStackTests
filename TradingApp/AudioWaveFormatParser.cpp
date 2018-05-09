#include "PreHeader.hpp"
#include "AudioWaveFormatParser.hpp"
#include <Application.hpp>
#include <Logger.hpp>

// AT9 is an "extensible" type wave format

static constexpr ui32 ChunkIdRIFF = 0x46464952;
static constexpr ui32 ChunkTypeWAVE = 0x45564157;
static constexpr ui32 ChunkIdfmt = 0x20746D66;
static constexpr ui32 ChunkIdfact = 0x74636166;
static constexpr ui32 ChunkIddata = 0x61746164;
static constexpr ui32 ChunkIdsmpl = 0x6C706D73;
static constexpr ui32 StreamLoopinfoMax = 2;

auto TradingApp::ParseWaveFormatHeader(const ui8 *sourceBufferData, ui32 sourceBufferDataSize) -> optional<WaveFormatInfo>
{
    WaveFormatInfo output{};
    ui32 currByte = 0;
    ui32 currentChunkDataSize = 0;

    // first get the RIFF chunk to make sure we have the correct file type
    memcpy(&output.riffWaveHeader, &sourceBufferData[currByte], sizeof(output.riffWaveHeader));

    currByte += sizeof(output.riffWaveHeader);

    // Check for "RIFF" in the ChunkID
    if (output.riffWaveHeader.chunkId != ChunkIdRIFF)
    {
        SENDLOG(Error, "Beginning of wave file was not \"RIFF\"\n");
        return {};
    }

    // Check to see if we've found the "WAVE" chunk (apparently there could be more than one "RIFF" chunk?)
    if (output.riffWaveHeader.typeId != ChunkTypeWAVE)
    {
        SENDLOG(Error, "First wave RIFF chunk is not a \"WAVE\" type\n");
        return {};
    }

    // Now read the other chunk headers to get file information
    while (currByte < sourceBufferDataSize)
    {
        // Read the next chunk header
        ChunkHeader chunkHeader;
        memcpy(&chunkHeader, &sourceBufferData[currByte], sizeof(ChunkHeader));

        // Offset the byte index byt he sizeof chunk header
        currByte += sizeof(ChunkHeader);

        // Now read which type of chunk this is and get the header info
        switch (chunkHeader.chunkId)
        {
        case ChunkIdfmt:
        {
            output.fmtChunkHeader = chunkHeader;

            memcpy(&output.fmtChunk, &sourceBufferData[currByte], sizeof(FormatChunk));

            currByte += sizeof(FormatChunk);

            // The rest of the data in this chunk is unknown so skip it
            currentChunkDataSize = (currentChunkDataSize < sizeof(FormatChunk)) ? 0 : currentChunkDataSize - sizeof(FormatChunk);
        }
        break;

        case ChunkIdfact:
        {
            output.factChunkHeader = chunkHeader;
            memcpy(&output.factChunk, &sourceBufferData[currByte], sizeof(FactChunk));

            currByte += sizeof(FactChunk);

            // The rest of the data in this chunk is unknown so skip it
            currentChunkDataSize = (currentChunkDataSize < sizeof(FactChunk)) ? 0 : currentChunkDataSize - sizeof(FactChunk);
        }
        break;

        case ChunkIddata:
        {
            output.dataChunkHeader = chunkHeader;

            // This is where bit-stream data starts in the AT9 file
            output.dataStartOffset = currByte;

            currentChunkDataSize = output.dataChunkHeader.chunkDataSize;
        }
        break;

        case ChunkIdsmpl:
        {
            output.sampleChunkHeader = chunkHeader;

            ui32 sampleChunkSize = sizeof(SampleChunk) - sizeof(SampleLoop)* StreamLoopinfoMax;

            memcpy(&output.sampleChunk, &sourceBufferData[currByte], sampleChunkSize);

            currByte += sampleChunkSize;

            // Read loop information
            ui32 readSize = 0;
            if (output.sampleChunk.sampleLoops <= StreamLoopinfoMax)
            {
                readSize = sizeof(SampleLoop)* output.sampleChunk.sampleLoops;
                memcpy(&output.sampleChunk.sampleLoop, &sourceBufferData[currByte], readSize);
            }
            else
            {
                readSize = sizeof(SampleLoop)* StreamLoopinfoMax;
                memcpy(&output.sampleChunk.sampleLoop, &sourceBufferData[currByte], readSize);
            }

            currByte += readSize;

            currentChunkDataSize = (currentChunkDataSize < sizeof(SampleChunk)) ? 0 : currentChunkDataSize - sizeof(SampleChunk);
        }
        break;

        default:
        {
            //SENDLOG(Error, "Wave file contained unknown RIFF chunk type\n");
        }
        }

        // Offset the byte read index by the current chunk's data size
        currByte += currentChunkDataSize;
    }

    return output;
}
