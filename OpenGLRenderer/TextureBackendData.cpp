#include "BasicHeader.hpp"
#include <Texture.hpp>
#include <Application.hpp>
#include <Logger.hpp>
#include "OpenGLRendererProxy.h"
#include "BackendData.hpp"

using namespace EngineCore;
using namespace OGLRenderer;

static inline GLenum TextureFormatToInternalOGL(TextureDataFormat format)
{
    switch (format)
    {
    case TextureDataFormat::Undefined:
        return GL_INVALID_ENUM;
    case TextureDataFormat::R8G8B8A8:
        return GL_RGBA8;
    case TextureDataFormat::B8G8R8A8:
        return GL_INVALID_ENUM;
    case TextureDataFormat::R8G8B8:
        return GL_RGB8;
    case TextureDataFormat::B8G8R8:
        return GL_INVALID_ENUM;
    case TextureDataFormat::R8G8B8X8:
    case TextureDataFormat::B8G8R8X8:
        return GL_INVALID_ENUM;
    case TextureDataFormat::R4G4B4A4:
        return GL_RGBA4;
    case TextureDataFormat::B4G4R4A4:
        return GL_INVALID_ENUM;
    case TextureDataFormat::R5G6B5:
        return GL_RGB565;
    case TextureDataFormat::B5G6R5:
        return GL_INVALID_ENUM;
    case TextureDataFormat::R32_Float:
        return GL_R32F;
    case TextureDataFormat::R32G32_Float:
        return GL_RG32F;
    case TextureDataFormat::R32G32B32_Float:
        return GL_RGB32F;
    case TextureDataFormat::R32G32B32A32_Float:
        return GL_RGBA32F;
    case TextureDataFormat::R16_Float:
        return GL_R16F;
    case TextureDataFormat::R16G16_Float:
        return GL_RG16F;
    case TextureDataFormat::R16G16B16_Float:
        return GL_RGB16F;
    case TextureDataFormat::R16G16B16A16_Float:
        return GL_RGBA16F;
    case TextureDataFormat::D32:
        return GL_DEPTH_COMPONENT32;
    case TextureDataFormat::D24S8:
        return GL_DEPTH24_STENCIL8;
    case TextureDataFormat::D24X8:
        return GL_DEPTH_COMPONENT24;
    }

    UNREACHABLE;
    return GL_INVALID_ENUM;
}

static inline std::pair<GLenum, GLenum> TextureDataFormatToOGLFormatType(TextureDataFormat format)
{
    constexpr auto &topair = make_pair<GLenum, GLenum>;

    switch (format)
    {
    case TextureDataFormat::Undefined:
        return topair(GL_INVALID_ENUM, GL_INVALID_ENUM);
    case TextureDataFormat::R8G8B8A8:
        return topair(GL_RGBA, GL_UNSIGNED_BYTE);
    case TextureDataFormat::B8G8R8A8:
        return topair(GL_BGRA, GL_UNSIGNED_BYTE);
    case TextureDataFormat::R8G8B8:
        return topair(GL_RGB, GL_UNSIGNED_BYTE);
    case TextureDataFormat::B8G8R8:
        return topair(GL_BGR, GL_UNSIGNED_BYTE);
    case TextureDataFormat::R8G8B8X8:
        return topair(GL_INVALID_ENUM, GL_INVALID_ENUM);
    case TextureDataFormat::B8G8R8X8:
        return topair(GL_INVALID_ENUM, GL_INVALID_ENUM);
    case TextureDataFormat::R4G4B4A4:
        return topair(GL_RGBA, GL_UNSIGNED_SHORT_4_4_4_4);
    case TextureDataFormat::B4G4R4A4:
        return topair(GL_BGRA, GL_UNSIGNED_SHORT_4_4_4_4);
    case TextureDataFormat::R5G6B5:
        return topair(GL_RGB, GL_UNSIGNED_SHORT_5_6_5);
    case TextureDataFormat::B5G6R5:
        return topair(GL_BGR, GL_UNSIGNED_SHORT_5_6_5);
    case TextureDataFormat::R32_Float:
        return topair(GL_R, GL_FLOAT);
    case TextureDataFormat::R32G32_Float:
        return topair(GL_RG, GL_FLOAT);
    case TextureDataFormat::R32G32B32_Float:
        return topair(GL_RGB, GL_FLOAT);
    case TextureDataFormat::R32G32B32A32_Float:
        return topair(GL_RGBA, GL_FLOAT);
    case TextureDataFormat::R16_Float:
        return topair(GL_R, GL_UNSIGNED_BYTE);
    case TextureDataFormat::R16G16_Float:
        return topair(GL_RG, GL_UNSIGNED_BYTE);
    case TextureDataFormat::R16G16B16_Float:
        return topair(GL_RGB, GL_UNSIGNED_BYTE);
    case TextureDataFormat::R16G16B16A16_Float:
        return topair(GL_RGBA, GL_UNSIGNED_BYTE);
    case TextureDataFormat::D32:
        return topair(GL_INVALID_ENUM, GL_INVALID_ENUM);
    case TextureDataFormat::D24S8:
        return topair(GL_INVALID_ENUM, GL_INVALID_ENUM);
    case TextureDataFormat::D24X8:
        return topair(GL_INVALID_ENUM, GL_INVALID_ENUM);
    }

    UNREACHABLE;
    return topair(GL_INVALID_ENUM, GL_INVALID_ENUM);
}

static inline GLenum TextureDimensionToOGL(Texture::Dimensiont dim)
{
    switch (dim)
    {
    case Texture::Dimensiont::Tex2D:
        return GL_TEXTURE_2D;
    case Texture::Dimensiont::Tex3D:
        return GL_TEXTURE_3D;
    case Texture::Dimensiont::TexCube:
        return GL_TEXTURE_CUBE_MAP;
    case Texture::Dimensiont::Undefined:
        return GL_INVALID_ENUM;
    }

    UNREACHABLE;
    return GL_INVALID_ENUM;
}

bool OpenGLRendererProxy::CheckTextureBackendData(const Texture &texture)
{
	if (RendererBackendData(texture) == nullptr)
	{
        AllocateBackendData<TextureBackendData>(texture);
	}
    else if (RendererFrontendDataDirtyState(texture) == false)
    {
        return false;
    }
    RendererFrontendDataDirtyState(texture, false);

	auto &texData = *RendererBackendData<TextureBackendData>(texture);

	auto failedToUpdate = [&]
	{
		SOFTBREAK;
        DeleteBackendData(texture);
        return true;
	};

    if (texture.IsDefined() == false)
    {
        SENDLOG(Error, "Texture %*s isn't defined\n", SVIEWARG(texture.Name()));
        return failedToUpdate();
    }

	if (Texture::IsFormatDepthStencil(texture.Format()) && texture.Dimension() != Texture::Dimensiont::Tex2D)
	{
		SENDLOG(Error, "Texture( name: %s ) uses depth-stencil format, but hasn't 2D dimension\n", texture.Name().data());
		return failedToUpdate();
	}

    // TODO: add complete formats support
	if (texData.width != texture.Width() || texData.height != texture.Height() || texData.depth != texture.Depth() || texData.format != texture.Format() || texData.dimension != texture.Dimension() || (texData.oglRenderBuffer == 0 && texData.oglTexture == 0))
	{
		if (Texture::IsFormatDepthStencil(texture.Format()))
		{
			optional<GLint> glFormat;
			switch (texture.Format())
			{
            case TextureDataFormat::D24X8:
				glFormat = GL_DEPTH_COMPONENT24;
				break;
			case TextureDataFormat::D24S8:
                glFormat = GL_DEPTH24_STENCIL8;
				break;
			case TextureDataFormat::D32:
				glFormat = GL_DEPTH_COMPONENT32;
				break;
			default:
				glFormat = nullopt;
			}

			if (glFormat == nullopt)
			{
				return failedToUpdate();
			}

			glGenRenderbuffers(1, &texData.oglRenderBuffer);
			glBindRenderbuffer(GL_RENDERBUFFER, texData.oglRenderBuffer);
			glRenderbufferStorage(GL_RENDERBUFFER, *glFormat, texture.Width(), texture.Height());
		}
		else
		{
			GLint glFormat;
			switch (texture.Format())
			{
			case TextureDataFormat::R16G16B16_Float:
				glFormat = GL_RGB16;
				break;
			case TextureDataFormat::R16G16B16A16_Float:
				glFormat = GL_RGBA16;
				break;
			case TextureDataFormat::R16_Float:
				glFormat = GL_R16;
				break;
			case TextureDataFormat::R32G32B32_Float:
				glFormat = GL_RGB32F;
				break;
			case TextureDataFormat::R32G32B32A32_Float:
				glFormat = GL_RGBA32F;
				break;
			case TextureDataFormat::R32_Float:
				glFormat = GL_R32F;
				break;
			case TextureDataFormat::R8G8B8:
				glFormat = GL_RGB8;
				break;
			case TextureDataFormat::R8G8B8A8:
				glFormat = GL_RGBA8;
				break;
			default:
				return failedToUpdate();
			}

			glGenTextures(1, &texData.oglTexture);
            GLenum textureType = GL_INVALID_ENUM;
			
			switch (texture.Dimension())
			{
            case Texture::Dimensiont::Tex2D:
                textureType = GL_TEXTURE_2D;
				glBindTexture(GL_TEXTURE_2D, texData.oglTexture);
				glTexImage2D(GL_TEXTURE_2D, 0, glFormat, texture.Width(), texture.Height(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
				break;
			case Texture::Dimensiont::Tex3D:
                textureType = GL_TEXTURE_3D;
				glBindTexture(GL_TEXTURE_3D, texData.oglTexture);
				glTexImage3D(GL_TEXTURE_3D, 0, glFormat, texture.Width(), texture.Height(), texture.Depth(), 0, GL_RGB, GL_UNSIGNED_BYTE, nullptr);
				break;
			case Texture::Dimensiont::TexCube:
				NOIMPL;
				break;
			case Texture::Dimensiont::Undefined:
				return failedToUpdate();
			}

            // the texture should still be bound
            glTexParameteri(textureType, GL_TEXTURE_BASE_LEVEL, 0);
            glTexParameteri(textureType, GL_TEXTURE_MAX_LEVEL, texture.MipLevelsCount() - 1);
		}

        texData.oglTextureDimension = TextureDimensionToOGL(texture.Dimension());
        texData.width = texture.Width();
        texData.height = texture.Height();
        texData.depth = texture.Depth();
        texData.format = texture.Format();
        texData.dimension = texture.Dimension();
        texData.mipLevels = 1;
        texData.data = nullptr;
        texData.isFullMipChainGenerated = false;
	}

    HasGLErrors();

    if (texture.IsRequestFullMipChain() && texData.isFullMipChainGenerated == false)
    {
        assert(Texture::IsFormatDepthStencil(texture.Format()) == false);

        glBindTexture(texData.oglTextureDimension, texData.oglTexture);

        glTexParameteri(texData.oglTextureDimension, GL_TEXTURE_BASE_LEVEL, texData.mipLevels - 1);
        glTexParameteri(texData.oglTextureDimension, GL_TEXTURE_MAX_LEVEL, 1000);

        glGenerateMipmap(texData.oglTextureDimension);

        if (HasGLErrors() == false)
        {
            texData.isFullMipChainGenerated = true;
        }
        else
        {
            glTexParameteri(texData.oglTextureDimension, GL_TEXTURE_MAX_LEVEL, texData.mipLevels - 1);
        }

        glTexParameteri(texData.oglTextureDimension, GL_TEXTURE_BASE_LEVEL, 0);
    }

    return true;
}

bool OpenGLRendererProxy::CreateTextureRegion(const Texture &texture, BufferOwnedData data, TextureDataFormat dataFormat)
{
    HasGLErrors();

    if (RendererBackendData(texture) == nullptr)
    {
        AllocateBackendData<TextureBackendData>(texture);
    }
    RendererFrontendDataDirtyState(texture, false);

    auto &textureData = *RendererBackendData<TextureBackendData>(texture);

    bool isDepthStencilTexture = Texture::IsFormatDepthStencil(texture.Format());

    if (isDepthStencilTexture)
    {
        glDeleteTextures(1, &textureData.oglTexture);
        if (textureData.oglRenderBuffer == 0)
        {
            glGenRenderbuffers(1, &textureData.oglRenderBuffer);
        }
    }
    else
    {
        glDeleteRenderbuffers(1, &textureData.oglRenderBuffer);
        if (textureData.oglTexture == 0)
        {
            glGenTextures(1, &textureData.oglTexture);
        }
    }

    auto failedReturn = [&textureData]() mutable -> bool
    {
        glDeleteTextures(1, &textureData.oglTexture);
        textureData.oglTexture = 0;
        glDeleteRenderbuffers(1, &textureData.oglRenderBuffer);
        textureData.oglRenderBuffer = 0;
        return false;
    };

    if (isDepthStencilTexture)
    {
        if (texture.Dimension() != Texture::Dimensiont::Tex2D)
        {
            SENDLOG(Error, "Depthstencil texture dimension must be Tex2D\n");
            return failedReturn();
        }
        if (texture.Depth() != 1)
        {
            SENDLOG(Error, "Depthstencil depth must be 1\n");
            return failedReturn();
        }
        if (texture.MipLevelsCount() != 1)
        {
            SENDLOG(Error, "Depthstencil mip levels count must be 1\n");
            return failedReturn();
        }
        if (data != nullptr)
        {
            SENDLOG(Error, "Depthstencil texture's manual data is currently forbidden\n"); // TODO: unforbid it
            return failedReturn();
        }
    }

    GLenum internalFormat = TextureFormatToInternalOGL(texture.Format());
    if (internalFormat == GL_INVALID_ENUM)
    {
        SENDLOG(Error, "Requested texture format is unsupported\n");
        return failedReturn();
    }

    auto oglDataFormatAndType = TextureDataFormatToOGLFormatType(dataFormat);
    if (data != nullptr && (oglDataFormatAndType.first == GL_INVALID_ENUM || oglDataFormatAndType.second == GL_INVALID_ENUM))
    {
        SENDLOG(Error, "Invalid texture data format\n");
        return failedReturn();
    }

    ui32 elementSize = Texture::FormatSizeInBytes(texture.Format());
    ui8 mipLevels = texture.MipLevelsCount();
    const byte *dataSource = data.get();

    if (isDepthStencilTexture)
    {
        glBindRenderbuffer(GL_RENDERBUFFER, textureData.oglRenderBuffer);
        glRenderbufferStorage(GL_RENDERBUFFER, internalFormat, texture.Width(), texture.Height());
    }
    else
    {
        GLenum type = TextureDimensionToOGL(texture.Dimension());
        if (type == GL_INVALID_ENUM)
        {
            SENDLOG(Error, "Invalid texture dimension\n");
            return failedReturn();
        }

        glBindTexture(type, textureData.oglTexture);

        if (type == GL_TEXTURE_2D)
        {
            ui32 levelWidth = texture.Width();
            ui32 levelHeight = texture.Height();
            for (ui8 level = 0; level < mipLevels; ++level)
            {
                ui32 levelSize = Texture::MipLevelSizeInBytes(texture.Width(), texture.Height(), texture.Depth(), texture.Format(), level);
                glTexImage2D(type, level, internalFormat, levelWidth, levelHeight, 0, oglDataFormatAndType.first, oglDataFormatAndType.second, dataSource);

                dataSource += levelSize;

                levelWidth /= 2; // TODO: NPOT textures
                levelHeight /= 2;
            }
        }
        else
        {
            NOIMPL;
        }

        glTexParameteri(type, GL_TEXTURE_BASE_LEVEL, 0);
        glTexParameteri(type, GL_TEXTURE_MAX_LEVEL, mipLevels - 1);
    }

    textureData.depth = texture.Depth();
    textureData.dimension = texture.Dimension();
    textureData.format = texture.Format();
    textureData.height = texture.Height();
    textureData.mipLevels = texture.MipLevelsCount();
    textureData.width = texture.Width();
    textureData.data = move(data);
    textureData.oglTextureDimension = TextureDimensionToOGL(texture.Dimension());

    return HasGLErrors() == false;
}