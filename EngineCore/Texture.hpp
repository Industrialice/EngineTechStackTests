#pragma once

#include "System.hpp"
#include "RendererDataResource.hpp"

namespace EngineCore
{
    // texture must be at least 1x1x1 and must have at least 1 mip level to be considered defined
    // an undefined texture can have a sampler and name, but cannot be used for rendering and other parameters will be defaulted
	class Texture : public RendererDataResource
	{
    public:
        enum class Dimensiont : ui8
        {
            Undefined, Tex2D, Tex3D, TexCube
        };

    protected:
        Texture(string_view name);
        Texture(ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name); // 2d texture
        Texture(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format, AccessMode access, string_view name); // 3d texture

        Texture(Texture &&) = delete;
        Texture &operator = (Texture &&) = delete;

	public:
        static shared_ptr<Texture> New(string_view name = "{unnamed}");
        static shared_ptr<Texture> New(ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat format, AccessMode access = AccessMode(), string_view name = "{unnamed}"); // 2d texture
        static shared_ptr<Texture> New(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format, AccessMode access = AccessMode(), string_view name = "{unnamed}"); // 3d texture
        static shared_ptr<Texture> New(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access = AccessMode(), string_view name = "{unnamed}"); // 2d texture
        static shared_ptr<Texture> New(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access = AccessMode(), string_view name = "{unnamed}"); // 3d texture

        template <typename T> bool Create(RendererArrayData<T> data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access = AccessMode(), optional<string_view> name = nullopt) // 2d texture
        {
            if (TextureSizeInBytes(width, height, 1, mipLevels, dataFormat) != data._sizeInBytes)
            {
                SOFTBREAK;
                return false;
            }
            return Create(move(data._data), dataFormat, width, height, mipLevels, textureFormat, access, name);
        }

        template <typename T> bool Create(RendererArrayData<T> data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access = AccessMode(), optional<string_view> name = nullopt) // 3d texture
        {
            if (TextureSizeInBytes(width, height, depth, mipLevels, dataFormat) != data._sizeInBytes)
            {
                SOFTBREAK;
                return false;
            }
            return Create(move(data._data), dataFormat, width, height, depth, mipLevels, textureFormat, access, name);
        }

        bool Create(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access = AccessMode(), optional<string_view> name = nullopt); // 2d texture
        bool Create(BufferOwnedData data, TextureDataFormat dataFormat, ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat textureFormat, AccessMode access = AccessMode(), optional<string_view> name = nullopt); // 3d texture

        bool Create(ui32 width, ui32 height, ui8 mipLevels, TextureDataFormat format, AccessMode access = AccessMode(), optional<string_view> name = nullopt); // 2d texture
        bool Create(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format, AccessMode access = AccessMode(), optional<string_view> name = nullopt); // 3d texture

        /*bool UpdateDataRegion(BufferOwnedData data, ui32 numberOfElements, ui32 updateStartOffset);

        ui8 *LockDataRegion(ui32 regionNumberOfElements, ui32 regionStartOffset, LockMode access);
        void UnlockDataRegion();*/

		~Texture() = default;
        const shared_ptr<class TextureSampler> &Sampler() const;
        void Sampler(const shared_ptr<class TextureSampler> &sampler); // pass nullptr to use default sampler
		ui32 Width() const;
		ui32 Height() const;
		ui8 Depth() const;
        ui8 MipLevelsCount() const;
        TextureDataFormat Format() const;
        Dimensiont Dimension() const;
        AccessMode Access() const;
        ui32 LockedRegionStart() const;
        ui32 LockedRegionEnd() const;
        bool IsLocked() const;
        bool IsRequestFullMipChain() const;
        void IsRequestFullMipChain(bool isRequest); // note that it won't change MipLevelsCount, any existing levels won't be changed
		void Name(string_view name);
		string_view Name() const;
        bool IsDefined() const; // must be at least 1x1x1, have at least 1 mip level and have format other than Undefined

        static ui32 FullChainMipLevelsCount(ui32 width, ui32 height, ui32 depth = 1);
        static ui32 TextureSizeInBytes(ui32 width, ui32 height, ui32 depth, ui8 mipLevels, TextureDataFormat format);
        static ui32 MipLevelSizeInBytes(ui32 width, ui32 height, ui32 depth, TextureDataFormat format, ui8 level = 0);
        static ui32 FormatSizeInBytes(TextureDataFormat format);
        static bool IsFormatDepthStencil(TextureDataFormat format);

    private:
        void MakeUndefined();

        shared_ptr<class TextureSampler> _sampler{};
        ui32 _lockedStart{}, _lockedEnd{}; // if _lockedStart == _lockedEnd, then the buffer isn't locked
        ui32 _width = 0, _height = 0, _depth = 0;
        ui8 _mipLevels = 1;
        bool _isRequestFullMipChain = false;
        TextureDataFormat _format = TextureDataFormat::Undefined;
        string _name{};
        CPUAccessMode _cpuAccessMode{};
        GPUAccessMode _gpuAccessMode{};
	};
}