#pragma once

#include <BasicHeader.hpp>

#include <GL/glew.h>

#ifdef WINPLATFORM
#include <GL/wglew.h>
#pragma comment(lib, "Opengl32.lib")
#endif

#ifdef _WIN64
#pragma comment(lib, "x64\\glew32.lib")
#else
#pragma comment(lib, "Win32\\glew32.lib")
#endif