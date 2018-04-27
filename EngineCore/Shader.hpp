#pragma once

#include "System.hpp"

namespace EngineCore
{
	// uniforms between different sharder stages are separated
	// uniforms are accessed solely by name + offset( 0 by default ), if uniforms in different shaders have the same name, they will be updated together
	// should contain GlobalUniformTexture and such, they will be used if there's no uniform provided by a material( only for uniforms with no default value )
	// or they'll be used as initial default values for uniforms with default values
	// TODO: the implementation is a prototype and is VERY suboptimal

	class Shader : public RendererFrontendData
	{
	public:
		struct Uniform
		{
			enum class Type : ui16
			{ 
				Texture,
				F32,
				I32,
				UI32,
				Bool
			};

			// TODO: default values
			string_view name;
			ui16 elementWidth, elementHeight, elementsCount; // mat4x3[2] will have elementWidth == 3, elementHeight == 4 and elementsCount == 2, elementWidth and elementHeight must be equal 1 for non-vector types
			Type type;
		};

	private:
		friend class Material;

        vector<Uniform> _uniforms{};
        vector<Uniform> _systemUniforms{};
        vector<string> _inputAttributes{};
        string _uniformNames{};
        string _vsSource{}, _psSource{};
        string _name{};

		Shader(Shader &&) = delete;
		Shader &operator = (Shader &&) = delete;

	protected:
		Shader(string_view name, string_view vsCode, string_view psCode, const Uniform *uniforms, ui32 uniformsCount, const string_view *inputAttributes, ui32 inputAttributesCount, const Uniform *systemUniforms, ui32 systemUniformsCount);

	public:
		static shared_ptr<Shader> New(string_view name, string_view vsCode, string_view psCode, const Uniform *uniforms = nullptr, ui32 uniformsCount = 0, const string_view *inputAttributes = nullptr, ui32 inputAttributesCount = 0, const Uniform *systemUniforms = nullptr, ui32 systemUniformsCount = 0);

		string_view VSCode() const;
		string_view PSCode() const;
		const vector<Uniform> &Uniforms() const;
        const vector<Uniform> &SystemUniforms() const;
        const vector<string> &InputAttributes() const;
		string_view Name() const;
	};
}