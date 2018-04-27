#include "BasicHeader.hpp"
#include "OpenGLRendererProxy.h"
#include "BackendData.hpp"
#include <Material.hpp>
#include <Texture.hpp>
#include <TextureSampler.hpp>
#include <Application.hpp>
#include <Logger.hpp>

using namespace EngineCore;
using namespace OGLRenderer;

bool OpenGLRendererProxy::CheckMaterialBackendData(const Material &material)
{
	if (RendererBackendData(material) == nullptr)
	{
        AllocateBackendData<MaterialBackendData>(material);
	}
    else if (RendererFrontendDataDirtyState(material) == false)
    {
        return false;
    }

    RendererFrontendDataDirtyState(material, false);
	auto &backendData = *RendererBackendData<MaterialBackendData>(material);

	auto uniformsCount = material.CurrentUniforms().size();

	auto getUniformSizeOf = [](const Shader::Uniform &uniform) -> uiw
	{
		switch (uniform.type)
		{
		case Shader::Uniform::Type::Bool:
			return 4 * uniform.elementWidth * uniform.elementHeight * uniform.elementsCount;
		case Shader::Uniform::Type::F32:
			return 4 * uniform.elementWidth * uniform.elementHeight * uniform.elementsCount;
		case Shader::Uniform::Type::I32:
			return 4 * uniform.elementWidth * uniform.elementHeight * uniform.elementsCount;
		case Shader::Uniform::Type::UI32:
			return 4 * uniform.elementWidth * uniform.elementHeight * uniform.elementsCount;
		case Shader::Uniform::Type::Texture:
			assert(uniform.elementWidth == 1 && uniform.elementHeight == 1);
			return sizeof(MaterialBackendData::TextureUniform) * uniform.elementsCount;
		}

		UNREACHABLE;
		return 0;
	};

	if (backendData.uniforms == nullptr && uniformsCount > 0)
	{
		uiw uniformsSizeOf = 0;

		for (const auto &uniform : material.Shader()->Uniforms())
		{
			uniformsSizeOf += getUniformSizeOf(uniform);
		}

		backendData.uniforms.reset((byte *)new uiw[uniformsSizeOf]);
	}

	byte *uniformsMemory = backendData.uniforms.get();

	for (ui32 uniformIndex = 0; uniformIndex < uniformsCount; ++uniformIndex)
	{
		const auto &matUniform = material.CurrentUniforms()[uniformIndex];
		const auto &shaderUniforms = material.Shader()->Uniforms();
		const auto &shaderUniform = shaderUniforms[uniformIndex];

        switch (shaderUniform.type)
        {
        case Shader::Uniform::Type::Bool:
            assert(shaderUniform.elementHeight == 1);
            if (matUniform == nullopt)
            {
                memset(uniformsMemory, 0, shaderUniform.elementWidth * shaderUniform.elementsCount * sizeof(i32));
            }
            else
            {
                auto boolArray = std::get_if<unique_ptr<bool[]>>(&*matUniform);
                assert(boolArray);
                for (ui32 boolIndex = 0; boolIndex < (ui32)(shaderUniform.elementWidth * shaderUniform.elementsCount); ++boolIndex)
                {
                    ((ui32 *)uniformsMemory)[boolIndex] = boolArray->operator[](boolIndex) ? 1 : 0;
                }
            }
            break;
        case Shader::Uniform::Type::F32:
            if (matUniform == nullopt)
            {
                f32 *f32Memory = (f32 *)uniformsMemory;
                for (ui32 rowIndex = 0; rowIndex < shaderUniform.elementHeight; ++rowIndex)
                {
                    for (ui32 columnIndex = 0; columnIndex < shaderUniform.elementWidth; ++columnIndex)
                    {
                        f32Memory[rowIndex * shaderUniform.elementWidth + columnIndex] = (rowIndex == columnIndex) ? 1.0f : 0.0f;
                    }
                }
            }
            else
            {
                auto f32Array = std::get_if<unique_ptr<f32[]>>(&*matUniform);
                assert(f32Array);
                memcpy(uniformsMemory, f32Array->get(), shaderUniform.elementWidth * shaderUniform.elementHeight * shaderUniform.elementsCount * sizeof(f32));
            }
            break;
        case Shader::Uniform::Type::I32:
            assert(shaderUniform.elementHeight == 1);
            if (matUniform == nullopt)
            {
                memset(uniformsMemory, 0, shaderUniform.elementWidth * shaderUniform.elementsCount * sizeof(i32));
            }
            else
            {
                auto i32Array = std::get_if<unique_ptr<i32[]>>(&*matUniform);
                assert(i32Array);
                memcpy(uniformsMemory, i32Array->get(), shaderUniform.elementWidth * shaderUniform.elementsCount * sizeof(i32));
            }
            break;
        case Shader::Uniform::Type::UI32:
            assert(shaderUniform.elementHeight == 1);
            if (matUniform == nullopt)
            {
                memset(uniformsMemory, 0, shaderUniform.elementWidth * shaderUniform.elementsCount * sizeof(ui32));
            }
            else
            {
                auto ui32Array = std::get_if<unique_ptr<ui32[]>>(&*matUniform);
                assert(ui32Array);
                memcpy(uniformsMemory, ui32Array->get(), shaderUniform.elementWidth * shaderUniform.elementsCount * sizeof(ui32));
            }
            break;
        case Shader::Uniform::Type::Texture:
            assert(shaderUniform.elementHeight == 1);
            assert(shaderUniform.elementsCount == 1); // texture arrays aren't supported yet
            if (matUniform == nullopt)
            {
                MaterialBackendData::TextureUniform texSampler{};
                memcpy(uniformsMemory, &texSampler, sizeof(texSampler));
            }
            else
            {
                auto uniform = std::get_if<pair<shared_ptr<class Texture>, shared_ptr<class TextureSampler>>>(&*matUniform);
                assert(uniform);
                assert(uniform->first); // default textures are currently unsupported
                CheckTextureBackendData(*uniform->first);
                auto sampler = uniform->second;
                if (sampler == nullptr)
                {
                    sampler = uniform->first->Sampler();
                }
                assert(sampler); // default texture samplers are currently unsupported
                CheckTextureSamplerBackendData(*sampler);
                MaterialBackendData::TextureUniform textureUniform{RendererBackendData<TextureBackendData>(*uniform->first), RendererBackendData<TextureSamplerBackendData>(*sampler)};
                assert(textureUniform.textureBackendData && textureUniform.textureSamplerBackendData); // TODO: handle failed update, use default textures
                memcpy(uniformsMemory, &textureUniform, sizeof(textureUniform));
            }
            break;
        }

		uniformsMemory += getUniformSizeOf(shaderUniform);
	}

    return true;
}