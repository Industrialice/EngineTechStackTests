#include "BasicHeader.hpp"
#include "Material.hpp"
#include "Application.hpp"
#include "Logger.hpp"
#include "Texture.hpp"
#include "TextureSampler.hpp"

using namespace EngineCore;

Material::Material(const shared_ptr<class Shader> &shader)
{
	assert(shader != nullptr);

	_shader = shader;

    _uniforms.resize(shader->Uniforms().size());
}

shared_ptr<Material> Material::New(const shared_ptr<class Shader> &shader)
{			
	struct Proxy : public Material
	{
        Proxy(const shared_ptr<class Shader> &shader) : Material(shader) {}
	};

	if (shader == nullptr)
	{
		SENDLOG(Error, "Cannot create material with nullptr shader\n");
		return nullptr;
	}

	return make_shared<Proxy>(shader);
}

void Material::Name(string_view name)
{
	_name = name;
}

string_view Material::Name() const
{
	if (_name.empty())
	{
		return "{empty}";
	}
	return _name;
}

const shared_ptr<Shader> &Material::Shader() const
{
	return _shader;
}

auto Material::UniformNameToId(string_view name) const -> uid
{
	auto searchResult = std::find_if(_shader->Uniforms().begin(), _shader->Uniforms().end(), 
		[name](const Shader::Uniform &value)
		{
			return value.name == name;
		});

	if (searchResult == _shader->Uniforms().end())
	{
		return uid();
	}

	uid id;
	id._id = (i32)(searchResult - _shader->Uniforms().begin());

	return id;
}

auto Material::UniformInfo(uid id) const -> optional<Shader::Uniform>
{
	if (id.IsValid() == false)
	{
		return nullopt;
	}

	return _shader->Uniforms()[id._id];
}

bool Material::UniformF32(uid id, f32 value, ui32 offset)
{
	return UniformF32(id, &value, 1, offset);
}

bool Material::UniformF32(uid id, initializer_list<f32> values, ui32 offset)
{
    return UniformF32(id, values.begin(), (ui32)values.size(), offset);
}

bool Material::UniformF32(uid id, const f32 *values, ui32 count, ui32 offset)
{
	auto *memory = GetUniformMemoryChecked<F32UniformType, Shader::Uniform::Type::F32>(id, count, offset);
	if (memory == nullptr)
	{
		return false;
	}

	MemCpy(memory->get() + offset, values, count);
	BackendDataMayBeDirty();
	return true;
}

bool Material::UniformI32(uid id, i32 value, ui32 offset)
{
	return UniformI32(id, &value, 1, offset);
}

bool Material::UniformI32(uid id, initializer_list<i32> values, ui32 offset)
{
    return UniformI32(id, values.begin(), (ui32)values.size(), offset);
}

bool Material::UniformI32(uid id, const i32 *values, ui32 count, ui32 offset)
{
	auto *memory = GetUniformMemoryChecked<I32UniformType, Shader::Uniform::Type::I32>(id, count, offset);
	if (memory == nullptr)
	{
		return false;
	}

	MemCpy(memory->get() + offset, values, count);
	BackendDataMayBeDirty();
	return true;
}

bool Material::UniformUI32(uid id, ui32 value, ui32 offset)
{
	return UniformUI32(id, &value, 1, offset);
}

bool Material::UniformUI32(uid id, initializer_list<ui32> values, ui32 offset)
{
    return UniformUI32(id, values.begin(), (ui32)values.size(), offset);
}

bool Material::UniformUI32(uid id, const ui32 *values, ui32 count, ui32 offset)
{
	auto *memory = GetUniformMemoryChecked<UI32UniformType, Shader::Uniform::Type::UI32>(id, count, offset);
	if (memory == nullptr)
	{
		return false;
	}

	MemCpy(memory->get() + offset, values, count);
	BackendDataMayBeDirty();
	return true;
}

bool Material::UniformBool(uid id, bool value, ui32 offset)
{
	return UniformBool(id, &value, 1, offset);
}

bool Material::UniformBool(uid id, initializer_list<bool> values, ui32 offset)
{
    return UniformBool(id, values.begin(), (ui32)values.size(), offset);
}

bool Material::UniformBool(uid id, const bool *values, ui32 count, ui32 offset)
{
	auto *memory = GetUniformMemoryChecked<BoolUniformType, Shader::Uniform::Type::Bool>(id, count, offset);
	if (memory == nullptr)
	{
		return false;
	}

	MemCpy(memory->get() + offset, values, count);
	BackendDataMayBeDirty();
	return true;
}

bool Material::UniformRaw(uid id, const ui8 *source, ui32 sizeInBytes, ui32 offset)
{
	if (id.IsValid() == false)
	{
		return false;
	}

	NOIMPL;
	return false;
}

bool Material::UniformTexture(uid id, const shared_ptr<Texture> &texture, const shared_ptr<class TextureSampler> &sampler, ui32 offset)
{
    if (texture == nullptr)
    {
        return ResetUniformToDefaults(id);
    }

	auto *memory = GetUniformMemoryChecked<TextureUniformType, Shader::Uniform::Type::Texture>(id, 1, offset);
	if (memory == nullptr)
	{
		return false;
	}

	memory->first = texture;
    memory->second = sampler;

	BackendDataMayBeDirty();
	return true;
}

/*bool Material::UniformTexture(uid id, PipelineTexture texture, const shared_ptr<class TextureSampler> &sampler, ui32 offset)
{
	auto *memory = GetUniformMemoryChecked<PipelineTextureUniformType, Shader::Uniform::Type::Texture>(id, 1, offset);
	if (memory == nullptr)
	{
		return false;
	}

	memory->first = texture;
    memory->second = sampler;

	BackendDataMayBeDirty();
	return true;
}*/

bool Material::ResetUniformToDefaults(uid id)
{
    if (id.IsValid() == false)
    {
        return false;
    }

    _uniforms[id._id] = nullopt;
    return true;
}

auto Material::CurrentUniforms() const -> const vector<MatUniform> &
{
	return _uniforms;
}

ui32 Material::UniformSize(uid id) const
{
	assert(id.IsValid());
	return _shader->Uniforms()[id._id].elementWidth * _shader->Uniforms()[id._id].elementHeight * _shader->Uniforms()[id._id].elementsCount;
}

bool Material::IsUniformOffsetInBounds(uid id, ui32 offset) const
{
	ui32 uniformSize = UniformSize(id);
	if (offset > uniformSize)
	{
		auto matName = Name();
		auto uniName = UniformInfo(id)->name;
		SENDLOG(Error, "Out of bounds uniform access, material %*s, uniform %*s offset %u\n", SVIEWARG(matName), SVIEWARG(uniName), offset);
		return false;
	}
	return true;
}

template <typename T, Shader::Uniform::Type reqestedUniformType> inline T *Material::GetUniformMemory(uid id)
{
	assert(id.IsValid());
    if (_uniforms[id._id] == nullopt)
    {
        const auto &shaderUniform = _shader->Uniforms()[id._id];
        if (reqestedUniformType != shaderUniform.type)
        {
            auto matName = Name();
            auto uniName = UniformInfo(id)->name;
            SENDLOG(Error, "Types missmatch while accessing a uniform, material %*s, uniform %*s\n", SVIEWARG(matName), SVIEWARG(uniName));
            return nullptr;
        }
        CreateUniformMemory(id, shaderUniform);
    }

    assert(_uniforms[id._id].has_value());

	T *value = std::get_if<T>(&*_uniforms[id._id]);
	if (value == nullptr)
	{
		auto matName = Name();
		auto uniName = UniformInfo(id)->name;
		SENDLOG(Error, "Types missmatch while accessing a uniform, material %*s, uniform %*s\n", SVIEWARG(matName), SVIEWARG(uniName));
		return nullptr;
	}

	return value;
}

template <typename T, Shader::Uniform::Type reqestedUniformType> inline T * Material::GetUniformMemoryChecked(uid id, ui32 count, ui32 offset)
{
	if (id.IsValid() == false)
	{
		return nullptr;
	}

	if (IsUniformOffsetInBounds(id, count + offset) == false)
	{
		return nullptr;
	}

	return GetUniformMemory<T, reqestedUniformType>(id);
}

bool Material::uid::IsValid() const
{
	return _id >= 0;
}

template <typename T> void ToIdentity(T *memory, T nonZeroValue, ui32 width, ui32 height)
{
    for (ui32 heightIndex = 0; heightIndex < std::min<ui32>(width, height); ++heightIndex)
    {
        memory[heightIndex * width + heightIndex] = nonZeroValue;
    }
}

void Material::CreateUniformMemory(uid id, const Shader::Uniform &uniform)
{
    switch (uniform.type)
    {
    case Shader::Uniform::Type::Texture:
        _uniforms[id._id] = TextureUniformType{};
        return;
    case Shader::Uniform::Type::Bool:
    {
        auto memory = make_unique<bool[]>(uniform.elementWidth * uniform.elementHeight * uniform.elementsCount); // will fill out the array with false
        ToIdentity<bool>(memory.get(), true, uniform.elementWidth, uniform.elementHeight);
        _uniforms[id._id] = move(memory);
        return;
    }
    case Shader::Uniform::Type::F32:
    {
        auto memory = make_unique<f32[]>(uniform.elementWidth * uniform.elementHeight * uniform.elementsCount); // will fill out the array with 0.0f
        ToIdentity<f32>(memory.get(), 1.0f, uniform.elementWidth, uniform.elementHeight);
        _uniforms[id._id] = move(memory);
        return;
    }
    case Shader::Uniform::Type::I32:
    {
        auto memory = make_unique<i32[]>(uniform.elementWidth * uniform.elementHeight * uniform.elementsCount); // will fill out the array with 0
        ToIdentity<i32>(memory.get(), 1, uniform.elementWidth, uniform.elementHeight);
        _uniforms[id._id] = move(memory);
        return;
    }
    case Shader::Uniform::Type::UI32:
    {
        auto memory = make_unique<ui32[]>(uniform.elementWidth * uniform.elementHeight * uniform.elementsCount); // will fill out the array with 0
        ToIdentity<ui32>(memory.get(), 1, uniform.elementWidth, uniform.elementHeight);
        _uniforms[id._id] = move(memory);
        return;
    }
    }

    UNREACHABLE;
}