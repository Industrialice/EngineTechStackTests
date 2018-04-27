#pragma once

#include "System.hpp"
#include "RendererDataResource.hpp"

namespace EngineCore
{
    class RendererArray : public RendererDataResource
    {
    public:
        enum class Typet { Undefined, VertexArray, IndexArray, ComputeBuffer };

    protected:
        RendererArray(string_view name);
        RendererArray(RendererArray &&) = delete;
        RendererArray &operator = (RendererArray &&) = delete;

    public:
        template <typename T> bool UpdateDataRegion(RendererArrayData<T> data, ui32 updateStartOffset)
        {
            return UpdateDataRegion(move(data._data), data.NumberOfElements(), updateStartOffset);
        }

        bool UpdateDataRegion(BufferOwnedData data, ui32 numberOfElements, ui32 updateStartOffset);

        byte *LockDataRegion(ui32 regionNumberOfElements, ui32 regionStartOffset, LockMode access);
        void UnlockDataRegion();

        ui32 NumberOfElements() const;
        ui32 Stride() const;
        Typet Type() const;
        ui32 LockedRegionStart() const;
        ui32 LockedRegionEnd() const;
        bool IsLocked() const;
        AccessMode Access() const;
        void Name(string_view name);
        string_view Name() const;

    protected:
        ui32 _numberOfElements{};
        ui32 _stride{};
        ui32 _lockedStart{}, _lockedEnd{}; // if _lockedStart == _lockedEnd, then the array isn't locked
        Typet _type = Typet::Undefined;
        CPUAccessMode _cpuAccessMode{};
        GPUAccessMode _gpuAccessMode{};
        string _name{};
    };

    class RendererVertexArray : public RendererArray
    {
    protected:
        RendererVertexArray(string_view name);

    public:
        static shared_ptr<RendererVertexArray> New(string_view name = "{unnamed}");
        static shared_ptr<RendererVertexArray> New(BufferOwnedData data, ui32 numberOfElements, ui32 stride, AccessMode access = AccessMode(), string_view name = "{unnamed}");

        template <typename T> static shared_ptr<RendererVertexArray> New(RendererArrayData<T> data, AccessMode access = AccessMode(), string_view name = "{unnamed}")
        {
            const auto &vertexArray = New(name);
            if (vertexArray->Create(move(data), access))
            {
                return vertexArray;
            }
            return nullptr;
        }

        template <typename T> bool Create(RendererArrayData<T> data, AccessMode access = AccessMode(), optional<string_view> name = nullopt)
        {
            return Create(move(data._data), data.NumberOfElements(), (ui32)sizeof(T), access, name);
        }

        bool Create(BufferOwnedData data, ui32 numberOfElements, ui32 stride, AccessMode access = AccessMode(), optional<string_view> name = nullopt);
    };

    class RendererIndexArray : public RendererArray
    {
    protected:
        RendererIndexArray(string_view name);

    public:
        enum class IndexTypet { Undefined, UI8, UI16, UI32 };

    private:
        template <typename T> struct TranslateIndexType
        {
            static IndexTypet Translate()
            {
                static_assert(false, "Cannot translate this native type to IndexTypet");
                return IndexTypet::Undefined;
            }
        };

        template <> struct TranslateIndexType<ui8> { static IndexTypet Translate() { return IndexTypet::UI8; } };
        template <> struct TranslateIndexType<ui16> { static IndexTypet Translate() { return IndexTypet::UI16; } };
        template <> struct TranslateIndexType<ui32> { static IndexTypet Translate() { return IndexTypet::UI32; } };

    public:
        static shared_ptr<RendererIndexArray> New(string_view name = "{unnamed}");
        static shared_ptr<RendererIndexArray> New(BufferOwnedData data, ui32 numberOfElements, IndexTypet indexType, AccessMode access = AccessMode(), string_view name = "{unnamed}");

        template <typename T> static shared_ptr<RendererIndexArray> New(RendererArrayData<T> data, AccessMode access = AccessMode(), string_view name = "{unnamed}")
        {
            const auto &indexArray = New(name);
            if (indexArray->Create(move(data), access))
            {
                return indexArray;
            }
            return nullptr;
        }

        template <typename T> bool Create(RendererArrayData<T> data, AccessMode access = AccessMode(), optional<string_view> name = nullopt)
        {
            return Create(move(data._data), data.NumberOfElements(), TranslateIndexType<T>::Translate(), access, name);
        }

        bool Create(BufferOwnedData data, ui32 numberOfElements, IndexTypet indexType, AccessMode access = AccessMode(), optional<string_view> name = nullopt);
        IndexTypet IndexType() const;
    };

    class RendererComputeBuffer : public RendererArray
    {
    };
}