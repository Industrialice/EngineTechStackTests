#include "BasicHeader.hpp"
#include "Texture.hpp"
#include "Application.hpp"
#include "Logger.hpp"
#include "Renderer.hpp"

using namespace EngineCore;

Texture::Texture(string_view name) : _name(name)
{}

Texture::Texture(ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name)
{
	Create(width, height, mipLevels, format, access, name);
}

bool Texture::Create(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access, optional<string_view> name)
{
    if (!Create(width, height, mipLevels, textureFormat, access, name))
    {
        return false;
    }
    return Application::GetRenderer().CreateTextureRegion(*this, move(data), dataFormat);
}

bool Texture::Create(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access, optional<string_view> name)
{
    if (!Create(width, height, depth, textureFormat, access, name))
    {
        return false;
    }
    return Application::GetRenderer().CreateTextureRegion(*this, move(data), dataFormat);
}

bool Texture::Create(ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat format, AccessMode access, optional<string_view> name)
{
    if (IsLocked())
    {
        SENDLOG(Error, "Create is called for texture %s, but it is locked\n", _name.c_str());
        return false;
    }

    if (name)
    {
        _name = *name;
    }

    bool isDefined = width >= 1 && height >= 1 && format != TextureDataFormat::Undefined;
    if (isDefined == false)
    {
        SENDLOG(Warning, "Making texture %s undefined\n", _name.c_str());
        MakeUndefined();
    }
    else
    {
        _width = width;
        _height = height;
        _depth = 1;
        _format = format;
        _mipLevels = mipLevels;
        _cpuAccessMode = access.cpuMode;
        _gpuAccessMode = access.gpuMode;
        _isRequestFullMipChain = false;
    }

    BackendDataMayBeDirty();

    return true;
}

Texture::Texture(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name)
{
	Create(width, height, depth, mipLevels, format, access, name);
}

shared_ptr<Texture> Texture::New(string_view name)
{
    struct Proxy : public Texture
    {
        Proxy(string_view name) : Texture(name) {}
    };
    return make_shared<Proxy>(name);
}

shared_ptr<Texture> Texture::New(ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name)
{
    struct Proxy : public Texture
    {
        Proxy(ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name) : Texture(width, height, mipLevels, format, access, name) {}
    };
    return make_shared<Proxy>(width, height, mipLevels, format, access, name);
}

shared_ptr<Texture> Texture::New(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name)
{
    struct Proxy : public Texture
    {
        Proxy(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name) : Texture(width, height, depth, mipLevels, format, access, name) {}
    };
    return make_shared<Proxy>(width, height, depth, mipLevels, format, access, name);
}

shared_ptr<Texture> Texture::New(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access, string_view name)
{
    auto texture = New(name);
    if (false == texture->Create(move(data), dataFormat, width, height, mipLevels, textureFormat, access))
    {
        return nullptr;
    }
    return texture;
}

shared_ptr<Texture> Texture::New(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access, string_view name)
{
    auto texture = New(name);
    if (false == texture->Create(move(data), dataFormat, width, height, depth, mipLevels, textureFormat, access))
    {
        return nullptr;
    }
    return texture;
}

bool Texture::Create(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format, AccessMode access, optional<string_view> name)
{
    if (IsLocked())
    {
        SENDLOG(Error, "Create is called for texture %s, but it is locked\n", _name.c_str());
        return false;
    }

    if (name)
    {
        _name = *name;
    }

    bool isDefined = width >= 1 && height >= 1 && depth >= 1 && format != TextureDataFormat::Undefined;
    if (isDefined == false)
    {
        SENDLOG(Warning, "Making texture %s undefined\n", _name.c_str());
        MakeUndefined();
    }
    else
    {
        _width = width;
        _height = height;
        _format = format;
        _depth = depth;
        _mipLevels = mipLevels;
        _cpuAccessMode = access.cpuMode;
        _gpuAccessMode = access.gpuMode;
        _isRequestFullMipChain = false;
    }

    BackendDataMayBeDirty();

    return true;
}

const shared_ptr<class TextureSampler> &Texture::Sampler() const
{
    return _sampler;
}

void Texture::Sampler(const shared_ptr<class TextureSampler> &sampler)
{
    _sampler = sampler;
    BackendDataMayBeDirty();
}

ui32 Texture::Width() const
{
	return _width;
}

ui32 Texture::Height() const
{
	return _height;
}

ui8 Texture::Depth() const
{
	return _depth;
}

ui8 Texture::MipLevelsCount() const
{
	return _mipLevels;
}

auto Texture::Format() const -> TextureDataFormat
{
	return _format;
}

auto Texture::Dimension() const -> Dimensiont
{
    if (IsDefined() == false)
    {
        return Dimensiont::Undefined;
    }
    if (_depth == 1)
    {
        return Dimensiont::Tex2D;
    }
    return Dimensiont::Tex3D;
}

auto Texture::Access() const -> AccessMode
{
    return {_cpuAccessMode, _gpuAccessMode};
}

ui32 Texture::LockedRegionStart() const
{
    return _lockedStart;
}

ui32 Texture::LockedRegionEnd() const
{
    return _lockedEnd;
}

bool Texture::IsLocked() const
{
    return _lockedStart != _lockedEnd;
}

bool Texture::IsRequestFullMipChain() const
{
    return _isRequestFullMipChain;
}

void Texture::IsRequestFullMipChain(bool isRequest)
{
    if (IsDefined() == false)
    {
        SENDLOG(Error, "IsRequestFullMipChain requested on an undefined texture %s\n", _name.c_str());
        return;
    }

    if (_isRequestFullMipChain != isRequest)
    {
        _isRequestFullMipChain = isRequest;
        BackendDataMayBeDirty();
    }
}

void Texture::Name(string_view name)
{
	_name = name;
}

string_view Texture::Name() const
{
	if (_name.empty())
	{
		return "{empty}";
	}
	return _name;
}

bool Texture::IsDefined() const
{
    return _width >= 1 && _height >= 1 && _depth >= 1 && _mipLevels >= 1;
}

ui32 Texture::FullChainMipLevelsCount(ui32 width, ui32 height, ui32 depth) // could I use log2 instead?
{
    ui32 count = 0;
    while (width > 0 || height > 0 || depth > 0)
    {
        ++count;
        width /= 2;
        height /= 2;
        depth /= 2;
    }
    return count;
}

ui32 Texture::TextureSizeInBytes(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format)
{
    ui32 size = 0;
    for (ui32 level = 0; level < mipLevels; ++level)
    {
        size += MipLevelSizeInBytes(width, height, depth, format, level);
    }
    return size;
}

ui32 Texture::MipLevelSizeInBytes(ui32 width, ui32 height, ui32 depth, TextureDataFormat format, ui8 level)
{
    ui32 level0Size = width * height * depth * FormatSizeInBytes(format);
    ui32 div = 1 << (level * 2); // TODO: NPOT and non-square
    return level0Size / div;
}

ui32 Texture::FormatSizeInBytes(TextureDataFormat format)
{
    switch (format)
    {
    case TextureDataFormat::Undefined:
        return 0;
    case TextureDataFormat::R8G8B8A8:
    case TextureDataFormat::B8G8R8A8:
    case TextureDataFormat::R8G8B8X8:
    case TextureDataFormat::B8G8R8X8:
    case TextureDataFormat::R32_Float:
    case TextureDataFormat::D32:
    case TextureDataFormat::D24S8:
    case TextureDataFormat::D24X8:
    case TextureDataFormat::R16G16_Float:
        return 4;
    case TextureDataFormat::R8G8B8:
    case TextureDataFormat::B8G8R8:
        return 3;
    case TextureDataFormat::R4G4B4A4:
    case TextureDataFormat::B4G4R4A4:
    case TextureDataFormat::R5G6B5:
    case TextureDataFormat::B5G6R5:
    case TextureDataFormat::R16_Float:
        return 2;
    case TextureDataFormat::R32G32B32_Float:
        return 12;
    case TextureDataFormat::R32G32B32A32_Float:
        return 16;
    case TextureDataFormat::R16G16B16_Float:
        return 6;
    case TextureDataFormat::R16G16B16A16_Float:
    case TextureDataFormat::R32G32_Float:
        return 8;
    }

    UNREACHABLE;
    return 0;
}

bool Texture::IsFormatDepthStencil(TextureDataFormat format)
{
    return (ui8)format >= 128;
}

void Texture::MakeUndefined()
{
    _width = 0;
    _height = 0;
    _depth = 0;
    _mipLevels = 0;
    _format = TextureDataFormat::Undefined;
    _cpuAccessMode = {};
    _gpuAccessMode = {};
    _isRequestFullMipChain = false;
}
