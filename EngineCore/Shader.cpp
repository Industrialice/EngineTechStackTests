#include "BasicHeader.hpp"
#include "Shader.hpp"
#include "Application.hpp"
#include "Logger.hpp"

using namespace EngineCore;

static auto FindUniformByName(string_view name, const vector<Shader::Uniform> &uniforms);
static bool CheckSystemUniform(Shader &shader, string_view uniformName, ui32 expectedWidth, ui32 expectedHeight, Shader::Uniform::Type expectedType);

Shader::Shader(string_view name, string_view vsCode, string_view psCode, const Uniform *uniforms, ui32 uniformsCount, const string_view *inputAttributes, ui32 inputAttributesCount, const Uniform *systemUniforms, ui32 systemUniformsCount) : _uniforms(uniforms, uniforms + uniformsCount), _inputAttributes(inputAttributes, inputAttributes + inputAttributesCount), _systemUniforms(systemUniforms, systemUniforms + systemUniformsCount)
{
	_name = name;
	_vsSource = vsCode;
	_psSource = psCode;
}

shared_ptr<Shader> Shader::New(string_view name, string_view vsCode, string_view psCode, const Uniform *uniforms, ui32 uniformsCount, const string_view *inputAttributes, ui32 inputAttributesCount, const Uniform *systemUniforms, ui32 systemUniformsCount)
{
	struct Proxy : public Shader
	{
        Proxy(string_view name, string_view vsCode, string_view psCode, const Uniform *uniforms, ui32 uniformsCount, const string_view *inputAttributes, ui32 inputAttributesCount, const Uniform *systemUniforms, ui32 systemUniformsCount) : Shader(name, vsCode, psCode, uniforms, uniformsCount, inputAttributes, inputAttributesCount, systemUniforms, systemUniformsCount) {}
	};

	assert(uniforms != nullptr || uniformsCount == 0);

	// TODO: make sure that the name appears only once
	for (ui32 uniformIndex = 0; uniformIndex < uniformsCount; ++uniformIndex)
	{
		const auto &uniform = uniforms[uniformIndex];

		if (uniform.elementWidth < 1 || uniform.elementWidth > 4)
		{
			SENDLOG(Error, "Trying to create shader with incorrect elementWidth %u, uniform name %*s shader name %*s\n", uniform.elementWidth, SVIEWARG(uniform.name), SVIEWARG(name));
			return nullptr;
		}

        if (uniform.elementHeight < 1 || uniform.elementHeight > 4)
        {
            SENDLOG(Error, "Trying to create shader with incorrect elementHeight %u, uniform name %*s shader name %*s\n", uniform.elementHeight, SVIEWARG(uniform.name), SVIEWARG(name));
            return nullptr;
        }

        if (uniform.elementHeight > 1 && uniform.elementWidth < 2)
        {
            SENDLOG(Error, "Trying to create shader with incorrect uniform, matrices with nx1 sizes aren't allowed, uniform name %*s shader name %*s\n", SVIEWARG(uniform.name), SVIEWARG(name));
            return nullptr;
        }

		if (uniform.elementsCount < 1)
		{
			SENDLOG(Error, "Trying to create shader with elementsCount equals 0, uniform name %*s shader name %*s\n", SVIEWARG(uniform.name), SVIEWARG(name));
			return nullptr;
		}

		if (uniform.type == Uniform::Type::Texture)
		{
			if (uniform.elementWidth != 1 || uniform.elementHeight != 1)
			{
				SENDLOG(Error, "Trying to create shader with either Texture uniform where elementWidth and/or elementHeight isn't 1, shader name %*s\n", SVIEWARG(name));
				return nullptr;
			}
		}
	}

    auto shaderPtr = make_shared<Proxy>(name, vsCode, psCode, uniforms, uniformsCount, inputAttributes, inputAttributesCount, systemUniforms, systemUniformsCount);;

    if (false == CheckSystemUniform(*shaderPtr, "_ModelMatrix", 3, 4, Shader::Uniform::Type::F32))
    {
        return nullptr;
    }
    if (false == CheckSystemUniform(*shaderPtr, "_ViewMatrix", 3, 4, Shader::Uniform::Type::F32))
    {
        return nullptr;
    }
    if (false == CheckSystemUniform(*shaderPtr, "_ProjectionMatrix", 4, 4, Shader::Uniform::Type::F32))
    {
        return nullptr;
    }
    if (false == CheckSystemUniform(*shaderPtr, "_ViewProjectionMatrix", 4, 4, Shader::Uniform::Type::F32))
    {
        return nullptr;
    }
    if (false == CheckSystemUniform(*shaderPtr, "_CameraPosition", 3, 1, Shader::Uniform::Type::F32))
    {
        return nullptr;
    }

    return shaderPtr;
}

string_view Shader::VSCode() const
{
	return _vsSource;
}

string_view Shader::PSCode() const
{
	return _psSource;
}

auto EngineCore::Shader::Uniforms() const -> const vector<Uniform> &
{
	return _uniforms;
}

auto Shader::SystemUniforms() const -> const vector<Uniform> &
{
    return _systemUniforms;
}

const vector<string> &Shader::InputAttributes() const
{
    return _inputAttributes;
}

string_view Shader::Name() const
{
	if (_name.empty())
	{
		return "{empty}";
	}
	return _name;
}

auto FindUniformByName(string_view name, const vector<Shader::Uniform> &uniforms)
{
    auto findFunc = [name](const Shader::Uniform &uniform)
    {
        return uniform.name == name;
    };
    return std::find_if(uniforms.begin(), uniforms.end(), findFunc);
}


bool CheckSystemUniform(Shader &shader, string_view uniformName, ui32 expectedWidth, ui32 expectedHeight, Shader::Uniform::Type expectedType)
{
    auto uniformIt = FindUniformByName(uniformName, shader.SystemUniforms());
    if (uniformIt != shader.SystemUniforms().end())
    {
        if (uniformIt->elementHeight != expectedHeight || uniformIt->elementWidth != expectedWidth)
        {
            SENDLOG(Error, "%*s system uniform expects size %ux%u, but Shader %*s provides size %ux%u\n", SVIEWARG(uniformName), (ui32)expectedHeight, (ui32)expectedWidth, SVIEWARG(shader.Name()), (ui32)uniformIt->elementHeight, (ui32)uniformIt->elementWidth);
            return false;
        }

        if (uniformIt->type != expectedType)
        {
            // TODO: print string of the type
            SENDLOG(Error, "%*s system uniform in shader %*s has unexpected type\n", SVIEWARG(uniformName), SVIEWARG(shader.Name()));
            return false;
        }
    }

    return true;
}