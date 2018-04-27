#pragma once

namespace EngineCore
{
	class Shader;
}

namespace EngineCore::ShadersManager
{
	shared_ptr<Shader> FindShaderByName(string_view name);
}