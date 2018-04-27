#include "BasicHeader.hpp"
#include "BackendData.hpp"
#include <RenderTarget.hpp>
#include <Application.hpp>
#include <Logger.hpp>
#include "OpenGLRendererProxy.h"
#include "BackendData.hpp"

using namespace EngineCore;
using namespace OGLRenderer;

bool OpenGLRendererProxy::CheckRenderTargetBackendData(const EngineCore::RenderTarget &rt)
{
	const RendererFrontendData &frontendData = rt;
	const auto &colorTexture = rt.ColorTarget();
	const auto &depthStencilTexture = rt.DepthStencilTarget();

	bool updateRTBackendData = RendererBackendData(frontendData) == nullptr || RendererFrontendDataDirtyState(frontendData);
	bool updateCTBackendData = colorTexture != nullptr && (RendererBackendData(*colorTexture) == nullptr || RendererFrontendDataDirtyState(*colorTexture));
	bool updateDSBackendData = depthStencilTexture != nullptr && (RendererBackendData(*depthStencilTexture) == nullptr || RendererFrontendDataDirtyState(*depthStencilTexture));

	if (updateRTBackendData == false && updateCTBackendData == false && updateDSBackendData == false)
	{
		return false;
	}

	if (RendererBackendData(frontendData) == nullptr)
	{
        AllocateBackendData<RenderTargetBackendData>(frontendData);
	}
    RendererFrontendDataDirtyState(frontendData, false);

	auto &rtData = *RendererBackendData<RenderTargetBackendData>(frontendData);

	auto failedToUpdate = [&]
	{
		SOFTBREAK;
		glDeleteFramebuffers(1, &rtData.frameBufferName);
        DeleteBackendData(frontendData);
        return true;
	};

	if (colorTexture != nullptr && depthStencilTexture != nullptr)
	{
		if (colorTexture->Width() != depthStencilTexture->Width() || colorTexture->Height() != depthStencilTexture->Height())
		{
			SENDLOG(Error, "RenderTarget's( name: %*s ) color texture's( name: %*s ) and depth stencil texture's( name: %*s ) resolutions don't match\n", SVIEWARG(rt.Name()), SVIEWARG(colorTexture->Name()), SVIEWARG(depthStencilTexture->Name()));
			return failedToUpdate();
		}
	}
	else if (colorTexture == nullptr && depthStencilTexture == nullptr)
	{
		glDeleteFramebuffers(1, &rtData.frameBufferName);
		rtData.frameBufferName = 0;
		return true;
	}

	if (updateCTBackendData)
	{
		CheckTextureBackendData(*colorTexture);
		if (RendererBackendData(*colorTexture) == nullptr)
		{
			SENDLOG(Error, "RenderTarget( name : %*s ) is invalid because its color target( name: %*s ) is invalid\n", SVIEWARG(rt.Name()), SVIEWARG(colorTexture->Name()));
			return failedToUpdate();
		}
	}
	if (updateDSBackendData)
	{
        CheckTextureBackendData(*depthStencilTexture);
		if (RendererBackendData(*colorTexture) == nullptr)
		{
			SENDLOG(Error, "RenderTarget( name : %*s ) is invalid because its depth stencil target( name: %*s ) is invalid\n", SVIEWARG(rt.Name()), SVIEWARG(depthStencilTexture->Name()));
			return failedToUpdate();
		}
	}

	if (rtData.frameBufferName == 0)
	{
		glGenFramebuffers(1, &rtData.frameBufferName);
	}

	glBindFramebuffer(GL_FRAMEBUFFER, rtData.frameBufferName);

	if (colorTexture != nullptr)
	{
		assert(RendererBackendData(*colorTexture) != nullptr);
		auto &textureData = *RendererBackendData<TextureBackendData>(*colorTexture);
		assert(textureData.oglTexture != 0);

		glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, textureData.oglTexture, 0);
		static const GLenum DrawBuffers[] = { GL_COLOR_ATTACHMENT0 };
		glDrawBuffers(1, DrawBuffers);
	}

	if (depthStencilTexture != nullptr)
	{
		assert(RendererBackendData(*depthStencilTexture) != nullptr);
		auto &textureData = *RendererBackendData<TextureBackendData>(*depthStencilTexture);
		assert(textureData.oglTexture != 0);

		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, textureData.oglTexture);
	}

    return true;
}
