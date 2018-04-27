#include "BasicHeader.hpp"
#include "OpenGLContextWindows.hpp"
#include <Application.hpp>
#include <Logger.hpp>

class OpenGLContextWindowsImpl final : public OGLRenderer::OpenGLContextWindows
{
	HDC _windowContext = 0;
	HGLRC _oglContext = 0;

	virtual void MakeCurrent() override
	{
		wglMakeCurrent(_windowContext, _oglContext);
	}
	
	virtual void SwapBuffers() override
	{
		::SwapBuffers(_windowContext);
	}
		
	virtual HDC WindowContext() const override
	{
		return _windowContext;
	}
	
	virtual HGLRC OpenGLContext() const override
	{
		return _oglContext;
	}

public:
	virtual ~OpenGLContextWindowsImpl()
	{
		wglMakeCurrent(_windowContext, nullptr);

		if (_oglContext != NULL)
		{
			wglDeleteContext(_oglContext);
			_oglContext = NULL;
		}
		if (_windowContext != NULL)
		{
			ReleaseDC(EngineCore::Application::GetMainWindow().hwnd, _windowContext);
			_windowContext = NULL;
		}

		SENDLOG(Info, "OpenGL Windows context's destroyed\n");
	}

	OpenGLContextWindowsImpl()
	{}

	bool Create(bool isFullscreen, EngineCore::TextureDataFormat colorFormat, ui32 depth, ui32 stencil, HDC windowContext) // TODO: handle function arguments
	{
		assert(_windowContext == 0 && _oglContext == 0);

		static const PIXELFORMATDESCRIPTOR pfd =
		{
			sizeof(PIXELFORMATDESCRIPTOR),  /* size */
			1,                              /* version */
			PFD_SUPPORT_OPENGL | PFD_DRAW_TO_WINDOW | PFD_DOUBLEBUFFER, /* support double-buffering */
			PFD_TYPE_RGBA,                  /* color type */
			32,                             /* prefered color depth */
			0, 0, 0, 0, 0, 0,               /* color bits (ignored) */
			0,                              /* no alpha buffer */
			0,                              /* alpha bits (ignored) */
			0,                              /* no accumulation buffer */
			0, 0, 0, 0,                     /* accum bits (ignored) */
			24,                             /* depth buffer */
			8,                              /* stencil buffer */
			0,                              /* no auxiliary buffers */
			PFD_MAIN_PLANE,                 /* main layer */
			0,                              /* reserved */
			0, 0, 0,                        /* no layer, visible, damage masks */
		};

		unsigned int chosenPixelFormat = ChoosePixelFormat(windowContext, &pfd);
		if (chosenPixelFormat == 0)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, ChoosePixelFormat failed\n");
			return false;
		}

		if (SetPixelFormat(windowContext, chosenPixelFormat, &pfd) == FALSE)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, SetPixelFormat failed\n");
			return false;
		}

		HGLRC oldContext = wglCreateContext(windowContext);
		if (oldContext == nullptr)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, wglCreateContext for oldContext had failed\n");
			return false;
		}

		unique_ptr<HGLRC, void(*)(HGLRC *)> oldContextPtr{ &oldContext,
			[](HGLRC *context)
			{
				if (*context) wglDeleteContext(*context);
			} };

		if (wglMakeCurrent(windowContext, oldContext) == FALSE)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, wglMakeCurrent for oldContext failed\n");
			return false;
		}

		GLenum glewInitResult = glewInit();
		if (glewInitResult != GLEW_OK)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, glewInit failed\n");
			return false;
		}

		if (wglMakeCurrent(NULL, NULL) == FALSE)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, wglMakeCurrent for NULL failed\n");
			return false;
		}

		static constexpr float ctxFloatAttribs[] = { 0, 0 };
		static constexpr int ctxIntAttribs[] =
		{
			WGL_DRAW_TO_WINDOW_ARB, 1,
			WGL_SUPPORT_OPENGL_ARB, 1,
			WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
			WGL_COLOR_BITS_ARB, 32,
			//WGL_ALPHA_BITS_ARB, 0,
			WGL_DEPTH_BITS_ARB, 24,
			WGL_STENCIL_BITS_ARB, 8,
			WGL_DOUBLE_BUFFER_ARB, 1,
			WGL_STEREO_ARB, 0,
			WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
			WGL_SAMPLE_BUFFERS_ARB, 1,
			WGL_SAMPLES_ARB, 4,
			0, 0
		};

		int pickedPixelFormat = 0;
		for (;;)
		{
			unsigned int numFormats = 0;
			int result = wglChoosePixelFormatARB(windowContext, ctxIntAttribs, ctxFloatAttribs, 1, &pickedPixelFormat, &numFormats);

			if (result && numFormats > 0)
			{
				break;
			}

			SENDLOG(Critical, "Windows OpenGL context init failed, wglChoosePixelFormatARB failed\n");
			return false;
		}

		const int arbContextFlags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB
			#ifdef DEBUG
			| WGL_CONTEXT_DEBUG_BIT_ARB
			#endif
			;

		static const int contextAtts[] =
		{
			WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
			WGL_CONTEXT_MINOR_VERSION_ARB, 4,
			WGL_CONTEXT_FLAGS_ARB, arbContextFlags,
			WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
			0, 0
		};

		HGLRC hglrc = wglCreateContextAttribsARB(windowContext, nullptr, contextAtts); // TODO: should probably delete hglrc when it isn't needed anymore
		if (hglrc == nullptr)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, hglrc is null\n");
			return false;
		}

		if (wglMakeCurrent(windowContext, hglrc) != TRUE)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, can't make current\n");
			return false;
		}

		if (glGetError() != GL_NO_ERROR)
		{
			SENDLOG(Critical, "Windows OpenGL context init failed, some OpenGL errors have occured during initialization\n");
			return false;
		}
        
        // TODO: doesn't work
        glEnable(GL_MULTISAMPLE);
        glEnable(GL_MULTISAMPLE_ARB);
        glEnable(GL_MULTISAMPLE_EXT);

		_oglContext = hglrc;
		_windowContext = windowContext;

		return true;
	}
};

auto OGLRenderer::OpenGLContextWindows::New(bool isFullscreen, EngineCore::TextureDataFormat colorFormat, ui32 depth, ui32 stencil, OpenGLRenderer::Window window) -> unique_ptr<class OpenGLContext>
{
	auto context = make_unique<OpenGLContextWindowsImpl>();
	if (context->Create(isFullscreen, colorFormat, depth, stencil, window.windowContext) == false)
	{
		return nullptr;
	}
	return context;
}