#pragma once

namespace EngineCore
{
    enum class TextureDataFormat : ui8
    {
        Undefined,
        R8G8B8A8, B8G8R8A8,
        R8G8B8, B8G8R8,
        R8G8B8X8, B8G8R8X8,
        R4G4B4A4, B4G4R4A4,
        R5G6B5, B5G6R5,
        R32_Float, R32G32_Float, R32G32B32_Float, R32G32B32A32_Float,
        R16_Float, R16G16_Float, R16G16B16_Float, R16G16B16A16_Float,
        D32 = 128, D24S8, D24X8
    };

    using BufferOwnedData = unique_ptr<byte[], function<void(void *)>>;

    template <typename T> class RendererArrayData
    {
        //static_assert(is_pod_v<T>);

        friend class RendererArray;
        friend class RendererVertexArray;
        friend class RendererIndexArray;
        friend class RendererComputeBuffer;
        friend class Texture;

        BufferOwnedData _data{};
        ui32 _sizeInBytes{};

    public:
        RendererArrayData(RendererArrayData &&) = default;
        RendererArrayData &operator = (RendererArrayData &&) = default;

        RendererArrayData(initializer_list<T> values) : _data(new byte[values.size() * sizeof(T)], [](void *p) {delete[] p; }), _sizeInBytes(ui32(values.size() * sizeof(T)))
        {
            memcpy(_data.get(), values.begin(), sizeof(T) * values.size());
        }

        RendererArrayData(BufferOwnedData data, ui32 numberOfElements) : _data(move(data)), _sizeInBytes(numberOfElements * sizeof(T))
        {}

        RendererArrayData(const T *data, ui32 numberOfElements) : _data(new byte[numberOfElements * sizeof(T)], [](void *p) {delete[] p; }), _sizeInBytes(numberOfElements * sizeof(T))
        {
            memcpy(_data.get(), data, numberOfElements * sizeof(T));
        }

        ui32 NumberOfElements() const { return _sizeInBytes / (ui32)sizeof(T); }
    };

    class RendererDataResource : public RendererFrontendData
    {
    public:
        enum class LockMode { ReadWrite, Read, Write };

        using BufferOwnedData = unique_ptr<byte[], function<void(void *)>>;

        struct CPUAccessMode
        {
            enum class Mode { NotAllowed, RareFull, RarePartial, FrequentFull, FrequentPartial };
            Mode writeMode = Mode::NotAllowed;
            Mode readMode = Mode::NotAllowed;
        };

        struct GPUAccessMode
        {
            bool isAllowUnorderedWrite = false;
            bool isAllowUnorderedRead = false;
        };

        struct AccessMode
        {
            CPUAccessMode cpuMode = CPUAccessMode();
            GPUAccessMode gpuMode = GPUAccessMode();
        };
    };
}