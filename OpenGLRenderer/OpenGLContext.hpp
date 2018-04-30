#pragma once

namespace OGLRenderer
{
	class OpenGLContext
	{
	public:		
		virtual ~OpenGLContext() = default;
		virtual void MakeCurrent() = 0;
		virtual void SwapBuffers() = 0;
        virtual void *ContextPointer() = 0;
	};
}