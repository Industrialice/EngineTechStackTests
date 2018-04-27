#pragma once

#include <Texture.hpp>
#include "OpenGLContext.hpp"
#include "OpenGLRenderer.hpp"

namespace OGLRenderer
{
	class OpenGLContextWindows : public OpenGLContext
	{
	public:
		virtual HDC WindowContext() const = 0;
		virtual HGLRC OpenGLContext() const = 0;
		static unique_ptr<class OpenGLContext> New(bool isFullscreen, EngineCore::TextureDataFormat colorFormat, ui32 depth, ui32 stencil, OpenGLRenderer::Window window);
	};
}