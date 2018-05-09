#pragma once

#include <System.hpp>
#include <Texture.hpp>
#include <RendererArray.hpp>

namespace OGLRenderer
{
    struct RendererBackendDataBase
    {
        void **backendDataPointer = 0;

        virtual ~RendererBackendDataBase() = default;

        enum class BackendDataType { RenderTarget, Texture, TextureSampler, Shader, Material, PipelineState, Array };
    };

	struct RenderTargetBackendData : public RendererBackendDataBase
	{
		GLuint frameBufferName = 0;

		virtual ~RenderTargetBackendData()
		{
			glDeleteFramebuffers(1, &frameBufferName);
		}

        static RendererBackendDataBase::BackendDataType Type() { return RendererBackendDataBase::BackendDataType::RenderTarget; }
	};

	struct TextureBackendData : public RendererBackendDataBase
	{
		GLuint oglTexture = 0;
        GLuint oglRenderBuffer = 0;
        GLenum oglTextureDimension = GL_INVALID_ENUM;
        EngineCore::RendererArray::BufferOwnedData data{};
		ui32 width = 0, height = 0, depth = 0;
        ui8 mipLevels = 1;
        bool isFullMipChainGenerated = false;
		EngineCore::TextureDataFormat format = EngineCore::TextureDataFormat::Undefined;
		EngineCore::Texture::Dimensiont dimension = EngineCore::Texture::Dimensiont::Undefined;

		virtual ~TextureBackendData()
		{
			glDeleteTextures(1, &oglTexture);
            glDeleteRenderbuffers(1, &oglRenderBuffer);
		}

        static RendererBackendDataBase::BackendDataType Type() { return RendererBackendDataBase::BackendDataType::Texture; }
	};

	struct TextureSamplerBackendData : public RendererBackendDataBase
	{
        GLuint oglSampler = 0;

		virtual ~TextureSamplerBackendData()
		{
            glDeleteSamplers(1, &oglSampler);
        }

        static RendererBackendDataBase::BackendDataType Type() { return RendererBackendDataBase::BackendDataType::TextureSampler; }
	};

	struct ShaderBackendData : public RendererBackendDataBase
	{
        using SetUniformFunction = void(*)(GLint location, GLsizei count, const void *values);
        using SetMatrixUniformFunction = void(*)(GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);

        struct OGLUniform
        {
            GLint location;
            uiw setFuncAddress;
        };

		GLuint program = 0;
        unique_ptr<OGLUniform[]> oglUniforms{};
        unique_ptr<OGLUniform[]> systemOglUniforms{};
        unique_ptr<GLint[]> attributeLocations{};

		virtual ~ShaderBackendData()
		{
			glDeleteProgram(program);
		}

        static RendererBackendDataBase::BackendDataType Type() { return RendererBackendDataBase::BackendDataType::Shader; }
	};

	struct MaterialBackendData : public RendererBackendDataBase
	{
        struct TextureUniform
        {
            TextureBackendData *textureBackendData{};
            TextureSamplerBackendData *textureSamplerBackendData{};
        };
        static_assert(std::is_trivially_copyable_v<TextureUniform>);

        unique_ptr<ui8[]> uniforms{};

        virtual ~MaterialBackendData() = default;

        static RendererBackendDataBase::BackendDataType Type() { return RendererBackendDataBase::BackendDataType::Material; }
	};

    struct PipelineStateBackendData : public RendererBackendDataBase
    {
        virtual ~PipelineStateBackendData() = default;

        static RendererBackendDataBase::BackendDataType Type() { return RendererBackendDataBase::BackendDataType::PipelineState; }
    };

    struct ArrayBackendData : public RendererBackendDataBase
    {
        EngineCore::RendererArray::BufferOwnedData data{};
        GLuint oglBuffer = 0;
        ui32 lockStart = 0, lockEnd = 0;

        virtual ~ArrayBackendData()
        {
            glDeleteBuffers(1, &oglBuffer);
        }

        static RendererBackendDataBase::BackendDataType Type() { return RendererBackendDataBase::BackendDataType::Array; }
    };
}