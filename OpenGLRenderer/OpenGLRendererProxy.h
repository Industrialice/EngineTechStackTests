#pragma once

#include "OpenGLRenderer.hpp"
#include "BackendData.hpp"

namespace OGLRenderer
{
    class OpenGLRendererProxy : public OpenGLRenderer
    {
    protected:
        static bool HasGLErrors();

        // if true has been returned from a Check* function, it means the backdata had been updated
        bool CheckMaterialBackendData(const EngineCore::Material &material);
        bool CheckRenderTargetBackendData(const EngineCore::RenderTarget &rt);
        bool CheckShaderBackendData(const EngineCore::Shader &shader);
        bool CheckTextureSamplerBackendData(const EngineCore::TextureSampler &sampler);
        bool CheckTextureBackendData(const EngineCore::Texture &texture);
        bool CreateTextureRegion(const EngineCore::Texture &texture, EngineCore::BufferOwnedData data, EngineCore::TextureDataFormat dataFormat);

        template <typename T> T *AllocateBackendData(const EngineCore::RendererFrontendData &frontendData)
        {
            return (T *)AllocateBackendData(frontendData, T::Type());
        }

        virtual void *AllocateBackendData(const EngineCore::RendererFrontendData &frontendData, RendererBackendDataBase::BackendDataType type) = 0;
        virtual void DeleteBackendData(const EngineCore::RendererFrontendData &frontendData) = 0;
    };
}