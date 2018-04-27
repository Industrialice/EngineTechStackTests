#include "BasicHeader.hpp"
#include "BackendData.hpp"
#include <Shader.hpp>
#include <Application.hpp>
#include <Logger.hpp>
#include "OpenGLRendererProxy.h"

using namespace EngineCore;
using namespace OGLRenderer;

static auto AcquireSetMatrixUniformFunction(const Shader::Uniform &shaderUniform) -> optional<ShaderBackendData::SetMatrixUniformFunction>;
static auto AcquireSetUniformFunction(const Shader::Uniform &shaderUniform) -> optional<ShaderBackendData::SetUniformFunction>;

bool OpenGLRendererProxy::CheckShaderBackendData(const Shader &shader)
{
	if (RendererBackendData(shader) == nullptr)
	{
        AllocateBackendData<ShaderBackendData>(shader);
	}
    else if (RendererFrontendDataDirtyState(shader) == false)
    {
        return false;
    }

	RendererFrontendDataDirtyState(shader, false);

	GLuint program = 0;

	auto failedReturn = [&]()
	{
		SOFTBREAK;
		glDeleteProgram(program);
        DeleteBackendData(shader);
        return true;
	};

	HasGLErrors();

	auto &backendData = *RendererBackendData<ShaderBackendData>(shader);

	auto loadShader = [](GLenum type, string_view code) -> GLuint
	{
		GLuint shader = glCreateShader(type);

		const char *codes[1] = { code.data() };
		GLint lengths[1] = { (GLint)code.size() };

		glShaderSource(shader, 1, codes, lengths );
		glCompileShader(shader);

		GLint status = 0;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
		if (status != GL_TRUE)
		{
			GLint infoLen = 0;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
			if (infoLen > 0)
			{
				string textError(infoLen, '\0');
				glGetShaderInfoLog(shader, infoLen, 0, textError.data());
				SENDLOG(Error, "Failed to compile a shader, error %s\n", textError.data());
			}

			glDeleteShader(shader);
			HasGLErrors();
			return 0;
		}

		if (HasGLErrors())
		{
			SENDLOG(Error, "Encountered OpenGL errors while compiling a shader\n");
			glDeleteShader(shader);
			return 0;
		}

		SENDLOG(Info, "Compiled a new shader\n");
		return shader;
	};

	auto vs = loadShader(GL_VERTEX_SHADER, shader.VSCode());
	if (vs == 0)
	{
		SENDLOG(Error, "Failed to compile a vertex shader for %*s\n", SVIEWARG(shader.Name()));
		return failedReturn();
	}

	auto ps = loadShader(GL_FRAGMENT_SHADER, shader.PSCode());
	if (ps == 0)
	{
		SENDLOG(Error, "Failed to compile a pixel shader for %*s\n", SVIEWARG(shader.Name()));
		return failedReturn();
	}

	program = glCreateProgram();
	glAttachShader(program, vs);
	glAttachShader(program, ps);
	glLinkProgram(program);
	glDeleteShader(vs);
	glDeleteShader(ps);

	GLint status = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &status);
	if (status != GL_TRUE)
	{		
		GLint infoLen = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLen);
		if (infoLen > 0)
		{
			string textError(infoLen, '\0');
			glGetProgramInfoLog(program, infoLen, 0, textError.data());
			SENDLOG(Error, "Failed to compile shader %*s, error %s\n", SVIEWARG(shader.Name()), textError.data());
		}
		else
		{
			SENDLOG(Error, "Failed to compile shader %*s\n", SVIEWARG(shader.Name()));
		}

		return failedReturn();
	}

	if (HasGLErrors())
	{
		SENDLOG(Error, "Failed to compile shader %*s\n", SVIEWARG(shader.Name()));
		return failedReturn();
	}

    auto getOglUniformsInfo = [&shader, &program](const vector<Shader::Uniform> &uniforms, unique_ptr<ShaderBackendData::OGLUniform[]> &oglUniforms) -> bool
    {
        auto uniformsCount = uniforms.size();

        if (oglUniforms == nullptr && uniformsCount > 0)
        {
            oglUniforms.reset(new ShaderBackendData::OGLUniform[uniformsCount]);
        }

        for (auto uniformIndex = 0; uniformIndex < uniformsCount; ++uniformIndex)
        {
            const auto &uniform = uniforms[uniformIndex];

            string nullTerminatedName = string(uniform.name.data(), uniform.name.data() + uniform.name.size());
            GLint location = glGetUniformLocation(program, nullTerminatedName.c_str());
            if (location == -1)
            {
                SENDLOG(Warning, "Failed to locate uniform %s in shader %*s\n", nullTerminatedName.c_str(), SVIEWARG(shader.Name()));
            }
            oglUniforms[uniformIndex].location = location;

            if (uniform.elementHeight > 1)
            {
                auto func = AcquireSetMatrixUniformFunction(uniform);
                if (!func)
                {
                    SENDLOG(Error, "Failed to acquire set matrix uniform function for uniform %*s, shader %*s\n", SVIEWARG(uniform.name), SVIEWARG(shader.Name()));
                    return false;
                }
                oglUniforms[uniformIndex].setFuncAddress = (uiw)*func;
            }
            else
            {
                auto func = AcquireSetUniformFunction(uniform);
                if (!func)
                {
                    SENDLOG(Error, "Failed to acquire set uniform function for uniform %*s, shader %*s\n", SVIEWARG(uniform.name), SVIEWARG(shader.Name()));
                    return false;
                }
                oglUniforms[uniformIndex].setFuncAddress = (uiw)*func;
            }
        }

        return true;
    };

    if (false == getOglUniformsInfo(shader.Uniforms(), backendData.oglUniforms))
    {
        SENDLOG(Error, "Error has occured while resolving uniforms for shader %*s\n", SVIEWARG(shader.Name()));
        return failedReturn();
    }
    if (false == getOglUniformsInfo(shader.SystemUniforms(), backendData.systemOglUniforms))
    {
        SENDLOG(Error, "Error has occured while resolving uniforms for shader %*s\n", SVIEWARG(shader.Name()));
        return failedReturn();
    }

    auto attributesCount = shader.InputAttributes().size();

    if (backendData.attributeLocations == nullptr && attributesCount > 0)
    {
        backendData.attributeLocations.reset(new GLint[attributesCount]);
    }

    for (auto attributeIndex = 0; attributeIndex < attributesCount; ++attributeIndex)
    {
        const auto &attribute = shader.InputAttributes()[attributeIndex];
        GLint location = glGetAttribLocation(program, attribute.c_str());
        if (location == -1)
        {
            SENDLOG(Error, "Failed to locate shader input attribute %s in shader %*s\n", attribute.c_str(), SVIEWARG(shader.Name()));
            return failedReturn();
        }

        backendData.attributeLocations[attributeIndex] = location;
    }

	backendData.program = program;

    return true;
}

auto AcquireSetMatrixUniformFunction(const Shader::Uniform &shaderUniform) -> optional<ShaderBackendData::SetMatrixUniformFunction>
{
    assert(shaderUniform.elementHeight > 1);

    optional<ShaderBackendData::SetMatrixUniformFunction> func;

    switch (shaderUniform.type)
    {
    case Shader::Uniform::Type::F32:
        break;
    case Shader::Uniform::Type::Bool:
    case Shader::Uniform::Type::I32:
    case Shader::Uniform::Type::UI32:
    case Shader::Uniform::Type::Texture:
        SENDLOG(Error, "Uniform %*s is a matrix, but matrices are allowed only for F32 types in OpenGL backend\n", SVIEWARG(shaderUniform.name));
        return nullopt;
    }

    auto widthTwo = [&shaderUniform, &func]()
    {
        switch (shaderUniform.elementHeight)
        {
        case 2:
            func = glUniformMatrix2fv;
            break;
        case 3:
            func = glUniformMatrix3x2fv;
            break;
        case 4:
            func = glUniformMatrix4x2fv;
            break;
        default:
            SENDLOG(Error, "Uniform %*s has incorrect matrix height %u\n", SVIEWARG(shaderUniform.name), shaderUniform.elementWidth);
        }

        return func;
    };

    auto widthThree = [&shaderUniform, &func]()
    {
        switch (shaderUniform.elementHeight)
        {
        case 2:
            func = glUniformMatrix2x3fv;
            break;
        case 3:
            func = glUniformMatrix3fv;
            break;
        case 4:
            func = glUniformMatrix4x3fv;
            break;
        default:
            SENDLOG(Error, "Uniform %*s has incorrect matrix height %u\n", SVIEWARG(shaderUniform.name), shaderUniform.elementWidth);
        }

        return func;
    };

    auto widthFour = [&shaderUniform, &func]()
    {
        switch (shaderUniform.elementHeight)
        {
        case 2:
            func = glUniformMatrix2x4fv;
            break;
        case 3:
            func = glUniformMatrix3x4fv;
            break;
        case 4:
            func = glUniformMatrix4fv;
            break;
        default:
            SENDLOG(Error, "Uniform %*s has incorrect matrix height %u\n", SVIEWARG(shaderUniform.name), shaderUniform.elementWidth);
        }

        return func;
    };

    switch (shaderUniform.elementWidth)
    {
    case 2:
        widthTwo();
        break;
    case 3:
        widthThree();
        break;
    case 4:
        widthFour();
        break;
    default:
        SENDLOG(Error, "Uniform %*s has incorrect matrix width %u\n", SVIEWARG(shaderUniform.name), shaderUniform.elementWidth);
    }

    return func;
}

auto AcquireSetUniformFunction(const Shader::Uniform &shaderUniform) -> optional<ShaderBackendData::SetUniformFunction>
{
    assert(shaderUniform.elementHeight == 1);

    optional<ShaderBackendData::SetUniformFunction> func;

    switch (shaderUniform.type)
    {
    case Shader::Uniform::Type::Bool:
        if (shaderUniform.elementWidth == 1)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform1iv;
        }
        else if (shaderUniform.elementWidth == 2)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform2iv;
        }
        else if (shaderUniform.elementWidth == 3)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform3iv;
        }
        else
        {
            assert(shaderUniform.elementWidth == 4);
            func = (ShaderBackendData::SetUniformFunction)glUniform4iv;
        }
        break;
    case Shader::Uniform::Type::F32:
        if (shaderUniform.elementWidth == 1)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform1fv;
        }
        else if (shaderUniform.elementWidth == 2)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform2fv;
        }
        else if (shaderUniform.elementWidth == 3)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform3fv;
        }
        else
        {
            assert(shaderUniform.elementWidth == 4);
            func = (ShaderBackendData::SetUniformFunction)glUniform4fv;
        }
        break;
    case Shader::Uniform::Type::I32:
        if (shaderUniform.elementWidth == 1)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform1iv;
        }
        else if (shaderUniform.elementWidth == 2)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform2iv;
        }
        else if (shaderUniform.elementWidth == 3)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform3iv;
        }
        else
        {
            assert(shaderUniform.elementWidth == 4);
            func = (ShaderBackendData::SetUniformFunction)glUniform4iv;
        }
        break;
    case Shader::Uniform::Type::UI32:
        if (shaderUniform.elementWidth == 1)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform1uiv;
        }
        else if (shaderUniform.elementWidth == 2)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform2uiv;
        }
        else if (shaderUniform.elementWidth == 3)
        {
            func = (ShaderBackendData::SetUniformFunction)glUniform3uiv;
        }
        else
        {
            assert(shaderUniform.elementWidth == 4);
            func = (ShaderBackendData::SetUniformFunction)glUniform4uiv;
        }
        break;
    case Shader::Uniform::Type::Texture:
        assert(shaderUniform.elementWidth == 1 && shaderUniform.elementHeight == 1);
        assert(shaderUniform.elementsCount == 1); // texture arrays are currently unsupported
        func = (ShaderBackendData::SetUniformFunction)glUniform1i;
        break;
    }

    return func;
}