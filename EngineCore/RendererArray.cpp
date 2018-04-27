#include "BasicHeader.hpp"
#include "RendererArray.hpp"
#include "Application.hpp"
#include "Logger.hpp"
#include "Renderer.hpp"

using namespace EngineCore;

RendererArray::RendererArray(string_view name) : _name(name)
{}

bool RendererArray::UpdateDataRegion(BufferOwnedData data, ui32 numberOfElements, ui32 updateStartOffset)
{
    if (_type == Typet::Undefined)
    {
        SENDLOG(Error, "UpdateDataRegion called on undefined array\n");
        return false;
    }

    if (_cpuAccessMode.writeMode == CPUAccessMode::Mode::NotAllowed)
    {
        SENDLOG(Error, "UpdateDataRegion called on a non-writable array %s\n", _name.c_str());
        return false;
    }

    // TODO: validate size and offset

    Application::GetRenderer().UpdateArrayRegion(*this, move(data), numberOfElements * Stride(), updateStartOffset * Stride());
    return true;
}

byte *RendererArray::LockDataRegion(ui32 regionNumberOfElements, ui32 regionStartOffset, LockMode access)
{
    if (_type == Typet::Undefined)
    {
        SENDLOG(Error, "LockDataRegion called on the undefined array %s\n", _name.c_str());
        return nullptr;
    }

    if (_lockedStart != _lockedEnd)
    {
        SENDLOG(Error, "LockDataRegion called on already the locked array %s\n", _name.c_str());
        return nullptr;
    }

    if (regionNumberOfElements == 0)
    {
        SENDLOG(Error, "LockDataRegion with 0 number of elements is not allowed, array %s\n", _name.c_str());
        return nullptr;
    }

    ui32 regionEnd = regionNumberOfElements + regionStartOffset;
    if (regionEnd < regionNumberOfElements) // overflow
    {
        SENDLOG(Error, "LockDataRegion incorrect lock parameters regionNumberOfElements %u and regionStartOffset %u while locking array %s\n", regionNumberOfElements, regionStartOffset, _name.c_str());
        return nullptr;
    }

    if (regionEnd > _numberOfElements)
    {
        SENDLOG(Error, "LockDataRegion regionNumberOfElements %u + regionStartOffset %u = %u and exeeds array size %u while locking array %s\n", regionNumberOfElements, regionStartOffset, regionEnd, _numberOfElements, _name.c_str());
        return nullptr;
    }

    switch (access)
    {
    case LockMode::Read:
        if (_cpuAccessMode.readMode == CPUAccessMode::Mode::NotAllowed)
        {
            SENDLOG(Error, "LockDataRegion requested with read access, but array %s isn't readable\n", _name.c_str());
            return nullptr;
        }
    case LockMode::ReadWrite:
        if (_cpuAccessMode.readMode == CPUAccessMode::Mode::NotAllowed)
        {
            SENDLOG(Error, "LockDataRegion requested with readwrite access, but array %s isn't readable\n", _name.c_str());
            return nullptr;
        }
        if (_cpuAccessMode.writeMode == CPUAccessMode::Mode::NotAllowed)
        {
            SENDLOG(Error, "LockDataRegion requested with readwrite access, but array %s isn't writable\n", _name.c_str());
            return nullptr;
        }

        SENDLOG(Error, "LockDataRegion called with read access request which is currently unsupported, array %s\n", _name.c_str());
        return nullptr;
    case LockMode::Write:
        if (_cpuAccessMode.writeMode == CPUAccessMode::Mode::NotAllowed)
        {
            SENDLOG(Error, "LockDataRegion requested with write access, but array %s isn't writable\n", _name.c_str());
            return nullptr;
        }
        break;
    }

    _lockedStart = regionStartOffset;
    _lockedEnd = regionStartOffset + regionNumberOfElements;

    return Application::GetRenderer().LockArrayRegionForWrite(*this, regionNumberOfElements * Stride(), regionStartOffset * Stride());
}

void RendererArray::UnlockDataRegion()
{
    if (_lockedStart == _lockedEnd)
    {
        SENDLOG(Error, "UnlockDataRegion called on the unlocked array %s\n", _name.c_str());
        return;
    }

    Application::GetRenderer().UnlockArrayRegion(*this);
    _lockedStart = _lockedEnd = 0;
    BackendDataMayBeDirty();
}

ui32 RendererArray::NumberOfElements() const
{
    return _numberOfElements;
}

ui32 RendererArray::Stride() const
{
    return _stride;
}

auto RendererArray::Type() const -> Typet
{
    return _type;
}

ui32 RendererArray::LockedRegionStart() const
{
    return _lockedStart;
}

ui32 RendererArray::LockedRegionEnd() const
{
    return _lockedEnd;
}

bool RendererArray::IsLocked() const
{
    return _lockedStart != _lockedEnd;
}

auto RendererArray::Access() const -> AccessMode
{
    if (_type == Typet::Undefined)
    {
        SOFTBREAK;
    }
    return {_cpuAccessMode, _gpuAccessMode};
}

void RendererArray::Name(string_view name)
{
    _name = name;
}

string_view RendererArray::Name() const
{
    return _name;
}

///////////////////
// VertexArray //
/////////////////

RendererVertexArray::RendererVertexArray(string_view name) : RendererArray(name)
{}

shared_ptr<RendererVertexArray> RendererVertexArray::New(string_view name)
{
    struct Proxy : public RendererVertexArray
    {
        Proxy(string_view name) : RendererVertexArray(name) {}
    };
    return make_shared<Proxy>(name);
}

shared_ptr<RendererVertexArray> RendererVertexArray::New(BufferOwnedData data, ui32 numberOfElements, ui32 stride, AccessMode access, string_view name)
{
    auto vertexArray = New(name);
    if (vertexArray->Create(move(data), numberOfElements, stride, access))
    {
        return vertexArray;
    }
    return nullptr;
}

bool RendererVertexArray::Create(BufferOwnedData data, ui32 numberOfElements, ui32 stride, AccessMode access, optional<string_view> name)
{
    if (_lockedStart != _lockedEnd)
    {
        SENDLOG(Error, "Create called when vertex array %s was still locked\n", _name.c_str());
        return false;
    }

    if (name)
    {
        _name = *name;
    }

    _type = Typet::VertexArray;
    _numberOfElements = numberOfElements;
    _stride = stride;
    _cpuAccessMode = access.cpuMode;
    _gpuAccessMode = access.gpuMode;
    Application::GetRenderer().CreateArrayRegion(*this, move(data));
    BackendDataMayBeDirty();
    return true;
}

//////////////////
// IndexArray //
////////////////

RendererIndexArray::RendererIndexArray(string_view name) : RendererArray(name)
{}

shared_ptr<RendererIndexArray> RendererIndexArray::New(string_view name)
{
    struct Proxy : public RendererIndexArray
    {
        Proxy(string_view name) : RendererIndexArray(name) {}
    };
    return make_shared<Proxy>(name);
}

shared_ptr<RendererIndexArray> RendererIndexArray::New(BufferOwnedData data, ui32 numberOfElements, IndexTypet indexType, AccessMode access, string_view name)
{
    auto indexArray = New(name);
    if (indexArray->Create(move(data), numberOfElements, indexType, access))
    {
        return indexArray;
    }
    return nullptr;
}

bool RendererIndexArray::Create(BufferOwnedData data, ui32 numberOfElements, IndexTypet indexType, AccessMode access, optional<string_view> name)
{
    if (_lockedStart != _lockedEnd)
    {
        SENDLOG(Error, "Create called when index array %s was still locked\n", _name.c_str());
        return false;
    }

    if (name)
    {
        _name = *name;
    }

    _type = Typet::IndexArray;
    _numberOfElements = numberOfElements;
    _cpuAccessMode = access.cpuMode;
    _gpuAccessMode = access.gpuMode;
    switch (indexType)
    {
    case IndexTypet::UI8:
        _stride = 1;
        break;
    case IndexTypet::UI16:
        _stride = 2;
        break;
    case IndexTypet::UI32:
        _stride = 4;
        break;
    case IndexTypet::Undefined:
        SENDLOG(Error, "Invalid input parameter for index array %s, indexType is IndexTypet::Undefined\n", _name.c_str());
        return false;
    }
    Application::GetRenderer().CreateArrayRegion(*this, move(data));
    BackendDataMayBeDirty();
    return true;
}

auto RendererIndexArray::IndexType() const -> IndexTypet
{
    if (_type != Typet::IndexArray)
    {
        SOFTBREAK;
        return IndexTypet::Undefined;
    }
    switch (_stride)
    {
    case 1:
        return IndexTypet::UI8;
    case 2:
        return IndexTypet::UI16;
    case 4:
        return IndexTypet::UI32;
    default:
        return IndexTypet::Undefined;
    }
}