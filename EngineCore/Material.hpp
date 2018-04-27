#pragma once

#include "System.hpp"
#include "Shader.hpp"

namespace EngineCore
{
	// TODO: the implementation is a prototype and is VERY suboptimal

	enum class PipelineTexture
	{
		CameraColorTarget0,
		CameraColorTarget1,
		CameraColorTarget2,
		CameraColorTarget3,
		CameraColorTarget4,
		CameraColorTarget5,
		CameraColorTarget6,
		CameraColorTarget7,
		CameraDepthTarget
	};

	class Material : public RendererFrontendData
	{
		friend Shader;

        using TextureUniformType = pair<shared_ptr<class Texture>, shared_ptr<class TextureSampler>>;
        //using PipelineTextureUniformType = pair<PipelineTexture, shared_ptr<class TextureSampler>>;
        using BoolUniformType = unique_ptr<bool[]>;
        using F32UniformType = unique_ptr<f32[]>;
        using I32UniformType = unique_ptr<i32[]>;
        using UI32UniformType = unique_ptr<ui32[]>;

		using MatUniform = optional<variant<
                TextureUniformType,
                //PipelineTextureUniformType,
                BoolUniformType,
                F32UniformType,
                I32UniformType,
                UI32UniformType
			>>;

        vector<MatUniform> _uniforms{};
        shared_ptr<Shader> _shader{};
        string _name{};

		Material(Material &&) = delete;
		Material &operator = (Material &&) = delete;

	protected:
		Material(const shared_ptr<Shader> &shader);

	public:
		static shared_ptr<Material> New(const shared_ptr<class Shader> &shader);

		void Name(string_view name);
		string_view Name() const;

		class uid
		{
			friend class Material;
			i32 _id = -1;

		public:
			bool IsValid() const;
		};

		const shared_ptr<Shader> &Shader() const;

		uid UniformNameToId(string_view name) const;

		optional<Shader::Uniform> UniformInfo(uid id) const;

		bool UniformF32(uid id, f32 value, ui32 offset = 0);
        bool UniformF32(uid id, initializer_list<f32> values, ui32 offset = 0);
		bool UniformF32(uid id, const f32 *values, ui32 count, ui32 offset = 0);

        bool UniformF32(string_view name, f32 value, ui32 offset = 0)
        {
            return UniformF32(UniformNameToId(name), value, offset);
        }
        bool UniformF32(string_view name, initializer_list<f32> values, ui32 offset = 0)
        {
            return UniformF32(UniformNameToId(name), values, offset);
        }
        bool UniformF32(string_view name, const f32 *values, ui32 count, ui32 offset = 0)
        {
            return UniformF32(UniformNameToId(name), values, count, offset);
        }

		bool UniformI32(uid id, i32 value, ui32 offset = 0);
        bool UniformI32(uid id, initializer_list<i32> values, ui32 offset = 0);
		bool UniformI32(uid id, const i32 *values, ui32 count, ui32 offset = 0);

        bool UniformI32(string_view name, i32 value, ui32 offset = 0)
        {
            return UniformI32(UniformNameToId(name), value, offset);
        }
        bool UniformI32(string_view name, initializer_list<i32> values, ui32 offset = 0)
        {
            return UniformI32(UniformNameToId(name), values, offset);
        }
        bool UniformI32(string_view name, const i32 *values, ui32 count, ui32 offset = 0)
        {
            return UniformI32(UniformNameToId(name), values, count, offset);
        }

		bool UniformUI32(uid id, ui32 value, ui32 offset = 0);
        bool UniformUI32(uid id, initializer_list<ui32> values, ui32 offset = 0);
		bool UniformUI32(uid id, const ui32 *values, ui32 count, ui32 offset = 0);

        bool UniformUI32(string_view name, ui32 value, ui32 offset = 0)
        {
            return UniformUI32(UniformNameToId(name), value, offset);
        }
        bool UniformUI32(string_view name, initializer_list<ui32> values, ui32 offset = 0)
        {
            return UniformUI32(UniformNameToId(name), values, offset);
        }
        bool UniformUI32(string_view name, const ui32 *values, ui32 count, ui32 offset = 0)
        {
            return UniformUI32(UniformNameToId(name), values, count, offset);
        }

		bool UniformBool(uid id, bool value, ui32 offset = 0);
        bool UniformBool(uid id, initializer_list<bool> values, ui32 offset = 0);
		bool UniformBool(uid id, const bool *values, ui32 count, ui32 offset = 0);

        bool UniformBool(string_view name, bool value, ui32 offset = 0)
        {
            return UniformBool(UniformNameToId(name), value, offset);
        }
        bool UniformBool(string_view name, initializer_list<bool> values, ui32 offset = 0)
        {
            return UniformBool(UniformNameToId(name), values, offset);
        }
        bool UniformBool(string_view name, const bool *values, ui32 count, ui32 offset = 0)
        {
            return UniformBool(UniformNameToId(name), values, count, offset);
        }

		bool UniformRaw(uid id, const byte *source, ui32 sizeInBytes, ui32 offset = 0);

        bool UniformRaw(string_view name, const byte *source, ui32 sizeInBytes, ui32 offset = 0)
        {
            return UniformRaw(UniformNameToId(name), source, sizeInBytes, offset);
        }

        // nullptr texture equals ResetUniformToDefaults for that id
		bool UniformTexture(uid id, const shared_ptr<class Texture> &texture, const shared_ptr<class TextureSampler> &sampler = nullptr, ui32 offset = 0);
		//bool UniformTexture(uid id, PipelineTexture texture, const shared_ptr<class TextureSampler> &sampler, ui32 offset = 0);

        bool UniformTexture(string_view name, const shared_ptr<class Texture> &texture, const shared_ptr<class TextureSampler> &sampler = nullptr, ui32 offset = 0)
        {
            return UniformTexture(UniformNameToId(name), texture, sampler, offset);
        }

        bool ResetUniformToDefaults(uid id);

        bool ResetUniformToDefaults(string_view name)
        {
            return ResetUniformToDefaults(UniformNameToId(name));
        }

		const vector<MatUniform> &CurrentUniforms() const;

	private:
		ui32 UniformSize(uid id) const;
		bool IsUniformOffsetInBounds(uid id, ui32 offset) const;
		template <typename T, Shader::Uniform::Type reqestedUniformType> T *GetUniformMemory(uid id);
		template <typename T, Shader::Uniform::Type reqestedUniformType> T *GetUniformMemoryChecked(uid id, ui32 count, ui32 offset);
        void CreateUniformMemory(uid id, const Shader::Uniform &uniform);
	};
}