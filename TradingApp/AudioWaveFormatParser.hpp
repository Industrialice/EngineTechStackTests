#pragma once

namespace TradingApp
{
    struct RiffWaveHeader
    {
        ui32 chunkId;
        ui32 chunkDataSize;
        ui32 typeId;
    };

    struct ChunkHeader
    {
        ui32 chunkId;
        ui32 chunkDataSize;
    };

    struct FormatChunk
    {
        // format ID
        ui16 formatTag;

        // number of channels, 1 = mono, 2 = stereo
        ui16 numChannels;

        // sampling rate
        ui32 samplesPerSec;

        // average bite rate
        ui32 averageBytesPerSec;

        // audio block size. ((mono) 1 = 8bit, 2 = 16bit), ((stereo) 2 = 8bit, 4 = 16bit)
        ui16 blockAlign;

        // Quantization rate, 8, 12, 16
        ui16 bitsPerSample;

        // Riff extension size, 34
        ui16 cbSize;

        // Number of output samples of audio per block
        ui16 samplesPerBlock;

        // Mapping of channels to spatial location
        ui32 channelMask;

        // Codec ID (for subformat encoding)
        ui8 subFormat[16];

        // Version information of the format
        ui32 versionInfo;

        // Subformat-specific setting information
        ui8 configData[4];

        // Reserved, 0
        ui32 reserved;
    };

    struct FactChunk
    {
        ui32 totalSamples;					// total samples per channel
        ui32 delaySamplesInputOverlap;		// samples of input and overlap delay
        ui32 delaySamplesInputOverlapEncoder; // samples of input and overlap and encoder delay
    };

    struct SampleLoop
    {
        ui32 identifier;
        ui32 type;
        ui32 start;
        ui32 end;
        ui32 fraction;
        ui32 playCount;
    };

    struct SampleChunk
    {
        ui32 manufacturer;
        ui32 product;
        ui32 samplePeriod;
        ui32 midiUnityNote;
        ui32 midiPitchFraction;
        ui32 smpteFormat;
        ui32 smpteOffset;
        ui32 sampleLoops;
        ui32 samplerData;
        SampleLoop sampleLoop[2];
    };

    struct WaveFormatInfo
    {
        RiffWaveHeader riffWaveHeader;
        ChunkHeader fmtChunkHeader;
        FormatChunk fmtChunk;
        ChunkHeader factChunkHeader;
        FactChunk factChunk;
        ChunkHeader sampleChunkHeader;
        SampleChunk sampleChunk;
        ChunkHeader dataChunkHeader;
        ui32 dataStartOffset;

        WaveFormatInfo()
        {
            MemOps::Set(this, 0, 1);
        }
    };

    optional<WaveFormatInfo> ParseWaveFormatHeader(const ui8 *sourceBufferData, ui32 sourceBufferDataSize);
}