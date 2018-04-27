#include "BasicHeader.hpp"
#include <TextureSampler.hpp>
#include <Application.hpp>
#include <Logger.hpp>
#include "OpenGLRendererProxy.h"
#include "BackendData.hpp"

using namespace EngineCore;
using namespace OGLRenderer;

bool OpenGLRendererProxy::CheckTextureSamplerBackendData(const EngineCore::TextureSampler &sampler)
{
    if (RendererBackendData(sampler) == nullptr)
    {
        AllocateBackendData<TextureSamplerBackendData>(sampler);
    }
    else if (RendererFrontendDataDirtyState(sampler) == false)
    {
        return false;
    }

    RendererFrontendDataDirtyState(sampler, false);
    auto &samplerData = *RendererBackendData<TextureSamplerBackendData>(sampler);

    if (samplerData.oglSampler == 0)
    {
        glGenSamplers(1, &samplerData.oglSampler);
    }

    auto getFilterMode = [](TextureSampler::FilterMode mode, bool isFilterAcrossMips)
    {
        if (mode == TextureSampler::FilterMode::Linear || mode == TextureSampler::FilterMode::Anisotropic)
        {
            return isFilterAcrossMips ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR_MIPMAP_NEAREST;
        }
        return isFilterAcrossMips ? GL_NEAREST_MIPMAP_LINEAR : GL_NEAREST_MIPMAP_NEAREST;
    };

    auto getWrapMode = [](TextureSampler::SampleMode mode)
    {
        switch (mode)
        {
        case TextureSampler::SampleMode::Border:
            return GL_CLAMP_TO_BORDER;
        case TextureSampler::SampleMode::ClampToBorder:
            return GL_CLAMP_TO_EDGE;
        case TextureSampler::SampleMode::Mirror:
            return GL_MIRRORED_REPEAT;
        case TextureSampler::SampleMode::Mirror_once:
            return GL_MIRROR_CLAMP_TO_EDGE;
        case TextureSampler::SampleMode::Tile:
            return GL_REPEAT;
        }

        UNREACHABLE;
        return GL_INVALID_ENUM;
    };

    if (sampler.MagFilterMode() == TextureSampler::FilterMode::Anisotropic || sampler.MinFilterMode() == TextureSampler::FilterMode::Anisotropic)
    {
        GLint maxSupportedAnisotropy = 1;
        glGetIntegerv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxSupportedAnisotropy);
        glSamplerParameteri(samplerData.oglSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, std::min<GLint>(sampler.MaxAnisotropy(), maxSupportedAnisotropy));
    }
    else
    {
        glSamplerParameteri(samplerData.oglSampler, GL_TEXTURE_MAX_ANISOTROPY_EXT, 1);
    }
    
    GLenum magFilterMode = [&sampler]
    {
        switch (sampler.MagFilterMode())
        {
        case TextureSampler::FilterMode::Linear:
        case TextureSampler::FilterMode::Anisotropic:
            return GL_LINEAR;
        case TextureSampler::FilterMode::Nearest:
            return GL_NEAREST;
        }

        UNREACHABLE;
        return GL_INVALID_ENUM;
    }();

    glSamplerParameteri(samplerData.oglSampler, GL_TEXTURE_MIN_FILTER, getFilterMode(sampler.MinFilterMode(), sampler.MinLinearInterpolateMips()));
    glSamplerParameteri(samplerData.oglSampler, GL_TEXTURE_MAG_FILTER, magFilterMode);

    glSamplerParameteri(samplerData.oglSampler, GL_TEXTURE_WRAP_S, getWrapMode(sampler.XSampleMode()));
    glSamplerParameteri(samplerData.oglSampler, GL_TEXTURE_WRAP_T, getWrapMode(sampler.YSampleMode()));
    glSamplerParameteri(samplerData.oglSampler, GL_TEXTURE_WRAP_R, getWrapMode(sampler.ZSampleMode()));

    return true;
}