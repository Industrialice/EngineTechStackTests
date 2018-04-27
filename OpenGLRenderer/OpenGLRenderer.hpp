#pragma once

#include "Material.hpp"
#include "Texture.hpp"
#include "Renderer.hpp"
#include "OpenGLContext.hpp"

namespace OGLRenderer
{
	class OpenGLRenderer : public EngineCore::Renderer
	{
	protected:
		OpenGLRenderer() = default;

	public:
		struct Window
		{
			#ifdef WINPLATFORM
			HDC windowContext = 0;
			#endif
		};

		static shared_ptr<OpenGLRenderer> New(bool isFullscreen, EngineCore::TextureDataFormat colorFormat, ui32 depth, ui32 stencil, Window window);
	};
}